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

    // Keyboard pages.
    check(K::nextMode(K::KeyboardMode::Latin, false) == K::KeyboardMode::Latin, "no kana pages for other languages");
    check(K::nextMode(K::KeyboardMode::Latin, true) == K::KeyboardMode::Hiragana &&
              K::nextMode(K::KeyboardMode::Hiragana, true) == K::KeyboardMode::Katakana &&
              K::nextMode(K::KeyboardMode::Katakana, true) == K::KeyboardMode::Latin,
          "ABC -> あ -> ア -> ABC");
    for (const auto mode : {K::KeyboardMode::Hiragana, K::KeyboardMode::Katakana}) {
        const auto page = K::keyboardPage(mode);
        check(page.size() > 60, "kana page is populated");
        for (const auto key : page)
            check(K::encode(key, 5, out) == K::Result::Ok, "every key encodes: " + std::string(key));
    }
    const auto hira = K::keyboardPage(K::KeyboardMode::Hiragana);
    const auto kata = K::keyboardPage(K::KeyboardMode::Katakana);
    const auto has = [](const std::vector<std::string_view>& p, std::string_view s) {
        for (auto k : p) if (k == s) return true;
        return false;
    };
    check(has(hira, "べ") && !has(hira, "ベ") && has(hira, "あ") && !has(hira, "ア"), "Hiragana page");
    check(has(kata, "ベ") && !has(kata, "べ") && has(kata, "ア") && !has(kata, "あ"), "Katakana page");
    check(has(hira, "ー") && has(kata, "ー") && has(hira, "０") && has(kata, "♀"), "symbols on both pages");

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
