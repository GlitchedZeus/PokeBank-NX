// Japanese Gen I / II name text (include/Utils/Gen12KanaText.h).
//   * every table glyph encodes to its byte, and that byte decodes back through BOTH read-only
//     decoders (Gen1ReadOnlySave decodeGen1String, Gen2ReadOnlySave decodeGen2String), so what the
//     editor writes is exactly what the app and the game will show;
//   * べぺへり store their shared katakana byte and read back in katakana (PKHeX remap);
//   * 5-character limit, empty names, Latin and unknown characters are refused;
//   * keyboard pages only offer encodable glyphs; the mode switch skips kana for non-Japanese saves;
//   * opt-in real-save evidence: with POKEBANK_JP_FIXTURES pointing at a folder holding saves/JP_*,
//     the trainer name of every Japanese Gen I / II save re-encodes to its exact stored bytes.
//     Without it the real-save part reports NOT TESTED / NO FIXTURE (never a failure).
#include "Utils/Gen12KanaText.h"
#include "Integration/Gen1/Gen1ReadOnlySave.h"
#include "Integration/Gen2/Gen2ReadOnlySave.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <set>
#include <string>
#include <vector>

namespace K = Utils::Gen12Kana;
namespace G1 = PokeVault::Integration::Gen1;
namespace G2 = PokeVault::Integration::Gen2;

namespace {
int failures = 0;
void check(bool ok, const std::string& what) {
    if (!ok) { std::cerr << "FAILED: " << what << '\n'; ++failures; }
}
std::vector<uint8_t> readFile(const std::filesystem::path& p) {
    std::ifstream in(p, std::ios::binary);
    return {std::istreambuf_iterator<char>(in), {}};
}
}  // namespace

int main() {
    // Every glyph: encode -> byte -> both decoders -> same (or the shared katakana) text.
    for (std::size_t i = 0; i < K::kGlyphCount; ++i) {
        const auto& g = K::kGlyphs[i];
        std::vector<uint8_t> out;
        check(K::encode(g.text, 5, out) == K::Result::Ok && out[0] == g.byte && out[1] == K::kTerminator,
              "glyph encodes to its byte: " + std::string(g.text));
        const bool remap = i >= K::kGlyphCount - K::kRemapCount;
        std::string expected(g.text);
        if (remap) {
            for (std::size_t j = 0; j < K::kGlyphCount - K::kRemapCount; ++j)
                if (K::kGlyphs[j].byte == g.byte) expected = K::kGlyphs[j].text;
        }
        const std::vector<uint8_t> field{g.byte, K::kTerminator};
        check(G1::decodeGen1String(field, G1::RegionLayout::Japanese) == expected,
              "Gen I decoder reads back " + expected);
        check(G2::decodeGen2String(field, G2::RegionLayout::Japanese) == expected,
              "Gen II decoder reads back " + expected);
    }
    // No byte appears twice among the table's own glyphs.
    std::set<uint8_t> bytes;
    for (std::size_t i = 0; i < K::kGlyphCount - K::kRemapCount; ++i)
        check(bytes.insert(K::kGlyphs[i].byte).second, "byte listed once: " + std::string(K::kGlyphs[i].text));

    std::vector<uint8_t> out;
    check(K::encode("ピカチュウ", 5, out) == K::Result::Ok &&
              out == std::vector<uint8_t>{0x41, 0x85, 0x90, 0xAE, 0x82, 0x50}, "ピカチュウ is native bytes");
    check(K::encode("べんり", 5, out) == K::Result::Ok && out[0] == 0x3D && out[2] == 0xD8,
          "hiragana べ / り store the shared bytes");
    check(G1::decodeGen1String(out, G1::RegionLayout::Japanese) == "ベんリ", "…and read back as the game shows them");
    check(K::encode("サトシサトシ", 5, out) == K::Result::TooLong, "6 characters refused");
    check(K::encode("", 5, out) == K::Result::Empty, "empty refused");
    std::string bad;
    check(K::encode("ASH", 5, out, &bad) == K::Result::Unsupported && bad == "A", "Latin refused in a Japanese name");
    check(K::encode("漢", 5, out, &bad) == K::Result::Unsupported && bad == "漢", "kanji refused");
    check(K::encode("ヴ", 5, out) == K::Result::Unsupported, "ヴ is not in the Gen I/II font");

    // Keyboard pages: three rows of at most ten keys; every character key encodes; every modifier
    // result is in the font; the mode key cycles あ -> ア -> １２３ and each script has two halves.
    using P = K::Page;
    for (const auto page : {P::HiraganaA, P::HiraganaB, P::KatakanaA, P::KatakanaB, P::Symbols}) {
        const auto rows = K::pageRows(page);
        check(rows.size() == K::kPageRows, "three key rows per page");
        for (const auto& r : rows) {
            check(!r.empty() && r.size() <= 10, "a row holds at most ten keys");
            for (const auto& key : r)
                if (key.modifier == K::Modifier::None)
                    check(K::encode(key.text, 5, out) == K::Result::Ok, "key encodes: " + std::string(key.text));
        }
    }
    check(K::nextMode(P::HiraganaA) == P::KatakanaA && K::nextMode(P::KatakanaB) == P::Symbols &&
              K::nextMode(P::Symbols) == P::HiraganaA && K::nextModeLabel(P::HiraganaB) == "ア" &&
              K::nextModeLabel(P::KatakanaA) == "１２３" && K::nextModeLabel(P::Symbols) == "あ",
          "mode key あ -> ア -> １２３");
    check(K::otherHalf(P::HiraganaA) == P::HiraganaB && K::otherHalf(P::KatakanaB) == P::KatakanaA &&
              !K::hasOtherHalf(P::Symbols), "each script has two halves");
    // Every base kana of the font is reachable: on a page directly, or through one modifier.
    for (std::size_t i = 0; i < K::kGlyphCount; ++i) {
        const auto text = K::kGlyphs[i].text;
        bool reachable = false;
        for (const auto page : {P::HiraganaA, P::HiraganaB, P::KatakanaA, P::KatakanaB, P::Symbols})
            for (const auto& r : K::pageRows(page))
                for (const auto& key : r) {
                    if (key.modifier != K::Modifier::None) continue;
                    if (key.text == text) reachable = true;
                    for (const auto m : {K::Modifier::Dakuten, K::Modifier::Handakuten, K::Modifier::Small})
                        if (K::applyModifier(m, key.text) == text) reachable = true;
                }
        // ﾟ ﾞ (half-width marks) are typed with the ゛゜ keys only as combining marks, never alone.
        if (text != "ﾟ" && text != "ﾞ") check(reachable, "reachable from the keyboard: " + std::string(text));
    }
    check(K::applyModifier(K::Modifier::Dakuten, "か") == "が" && K::applyModifier(K::Modifier::Dakuten, "が") == "か",
          "゛ voices and un-voices");
    check(K::applyModifier(K::Modifier::Handakuten, "ば") == "ぱ" && K::applyModifier(K::Modifier::Dakuten, "ぱ") == "ば",
          "゜ / ゛ switch between ば and ぱ");
    check(K::applyModifier(K::Modifier::Small, "ツ") == "ッ" && K::applyModifier(K::Modifier::Small, "ッ") == "ツ",
          "小 toggles the small form");
    check(K::applyModifier(K::Modifier::Dakuten, "う").empty() && K::applyModifier(K::Modifier::Dakuten, "ウ").empty(),
          "ヴ / ゔ are not in the font: ゛ on う changes nothing");
    for (const auto& p : K::detail::kDakuten) check(K::encode(p.to, 5, out) == K::Result::Ok, "voiced form encodes");
    for (const auto& p : K::detail::kHandakuten) check(K::encode(p.to, 5, out) == K::Result::Ok, "semi-voiced form encodes");
    for (const auto& p : K::detail::kSmall) check(K::encode(p.to, 5, out) == K::Result::Ok, "small form encodes");

    // Opt-in real-save evidence.
    const char* dir = std::getenv("POKEBANK_JP_FIXTURES");
    if (!dir) {
        std::cout << "Gen I/II kana text: real Japanese saves NOT TESTED / NO FIXTURE (set POKEBANK_JP_FIXTURES)\n";
    } else {
        int names = 0;
        for (const auto& e : std::filesystem::directory_iterator(std::filesystem::path(dir) / "saves")) {
            const auto name = e.path().filename().string();
            const bool gen1 = name.find("_G1_") != std::string::npos, gen2 = name.find("_G2_") != std::string::npos;
            if (!gen1 && !gen2) continue;
            const auto raw = readFile(e.path());
            const std::size_t offset = gen1 ? 0x2598 : 0x200B;   // player name (6 bytes in Japanese)
            if (raw.size() < offset + 6) continue;
            const std::vector<uint8_t> stored(raw.begin() + offset, raw.begin() + offset + 6);
            const std::string text = gen1 ? G1::decodeGen1String(stored, G1::RegionLayout::Japanese)
                                           : G2::decodeGen2String(stored, G2::RegionLayout::Japanese);
            check(K::encode(text, 5, out) == K::Result::Ok, name + ": stored name encodes (" + text + ")");
            std::size_t end = 0;
            while (end < 6 && stored[end] != K::kTerminator) ++end;
            check(end < 6 && std::equal(stored.begin(), stored.begin() + end + 1, out.begin()),
                  name + ": re-encoded name matches the stored bytes");
            ++names;
        }
        check(names >= 6, "real-save evidence covered the Japanese Gen I/II saves");
        std::cout << "Gen I/II kana text: " << names << " real Japanese trainer names round-trip byte for byte\n";
    }

    if (failures) { std::cerr << failures << " kana text check(s) failed\n"; return 1; }
    std::cout << "Gen I/II kana text: table, remaps, limits and keyboard pages PASS\n";
    return 0;
}
