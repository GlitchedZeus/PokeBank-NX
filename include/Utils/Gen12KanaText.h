#pragma once
// Japanese Gen I / II name text (owner Oct 28: Japanese editing with a kana keyboard switch).
//
// The byte table is PKHeX StringConverter1.TableJP (8e3eb3d4); StringConverter2.TableJP differs only
// in control codes that names never use, so one encoder serves both generations. The decoders in
// Gen1ReadOnlySave.cpp (jpGlyph) and Gen2ReadOnlySave.cpp read the same table; the round trip of
// every byte encoded here is pinned by tests/test_gen12_kana_text.cpp.
//
// Gen I / II store べ, ぺ, へ and り with their katakana glyphs (bytes 0x3D, 0x47, 0xCD, 0xD8); like
// PKHeX TryGetUserFriendlyRemap, typing the hiragana form stores that shared byte, and the save then
// reads back the katakana form, exactly as the game shows it.
//
// Names hold 5 characters in Japanese Gen I / II (6-byte fields with the 0x50 terminator). Nothing
// here touches a save: it only turns typed text into native bytes, or refuses it.

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace Utils::Gen12Kana {

inline constexpr std::size_t kMaxNameChars = 5;
inline constexpr uint8_t kTerminator = 0x50;

struct Glyph { uint8_t byte; std::string_view text; };

// Every name glyph of the Japanese table. 0x5D (in-game trade OT marker) and control codes are left
// out on purpose: they are not characters a player can type.
inline constexpr Glyph kGlyphs[] = {
    {0x05,"ガ"},{0x06,"ギ"},{0x07,"グ"},{0x08,"ゲ"},{0x09,"ゴ"},{0x0A,"ザ"},{0x0B,"ジ"},{0x0C,"ズ"},{0x0D,"ゼ"},{0x0E,"ゾ"},
    {0x0F,"ダ"},{0x10,"ヂ"},{0x11,"ヅ"},{0x12,"デ"},{0x13,"ド"},{0x19,"バ"},{0x1A,"ビ"},{0x1B,"ブ"},{0x1C,"ボ"},
    {0x26,"が"},{0x27,"ぎ"},{0x28,"ぐ"},{0x29,"げ"},{0x2A,"ご"},{0x2B,"ざ"},{0x2C,"じ"},{0x2D,"ず"},{0x2E,"ぜ"},{0x2F,"ぞ"},
    {0x30,"だ"},{0x31,"ぢ"},{0x32,"づ"},{0x33,"で"},{0x34,"ど"},{0x3A,"ば"},{0x3B,"び"},{0x3C,"ぶ"},{0x3D,"ベ"},{0x3E,"ぼ"},
    {0x40,"パ"},{0x41,"ピ"},{0x42,"プ"},{0x43,"ポ"},{0x44,"ぱ"},{0x45,"ぴ"},{0x46,"ぷ"},{0x47,"ペ"},{0x48,"ぽ"},
    {0x6E,"ぃ"},{0x6F,"ぅ"},{0x70,"「"},{0x71,"」"},{0x72,"『"},{0x73,"』"},{0x74,"・"},{0x75,"⋯"},{0x76,"ぁ"},{0x77,"ぇ"},
    {0x78,"ぉ"},{0x7F,"　"},
    {0x80,"ア"},{0x81,"イ"},{0x82,"ウ"},{0x83,"エ"},{0x84,"オ"},{0x85,"カ"},{0x86,"キ"},{0x87,"ク"},{0x88,"ケ"},{0x89,"コ"},
    {0x8A,"サ"},{0x8B,"シ"},{0x8C,"ス"},{0x8D,"セ"},{0x8E,"ソ"},{0x8F,"タ"},{0x90,"チ"},{0x91,"ツ"},{0x92,"テ"},{0x93,"ト"},
    {0x94,"ナ"},{0x95,"ニ"},{0x96,"ヌ"},{0x97,"ネ"},{0x98,"ノ"},{0x99,"ハ"},{0x9A,"ヒ"},{0x9B,"フ"},{0x9C,"ホ"},{0x9D,"マ"},
    {0x9E,"ミ"},{0x9F,"ム"},{0xA0,"メ"},{0xA1,"モ"},{0xA2,"ヤ"},{0xA3,"ユ"},{0xA4,"ヨ"},{0xA5,"ラ"},{0xA6,"ル"},{0xA7,"レ"},
    {0xA8,"ロ"},{0xA9,"ワ"},{0xAA,"ヲ"},{0xAB,"ン"},{0xAC,"ッ"},{0xAD,"ャ"},{0xAE,"ュ"},{0xAF,"ョ"},{0xB0,"ィ"},
    {0xB1,"あ"},{0xB2,"い"},{0xB3,"う"},{0xB4,"え"},{0xB5,"お"},{0xB6,"か"},{0xB7,"き"},{0xB8,"く"},{0xB9,"け"},{0xBA,"こ"},
    {0xBB,"さ"},{0xBC,"し"},{0xBD,"す"},{0xBE,"せ"},{0xBF,"そ"},{0xC0,"た"},{0xC1,"ち"},{0xC2,"つ"},{0xC3,"て"},{0xC4,"と"},
    {0xC5,"な"},{0xC6,"に"},{0xC7,"ぬ"},{0xC8,"ね"},{0xC9,"の"},{0xCA,"は"},{0xCB,"ひ"},{0xCC,"ふ"},{0xCD,"ヘ"},{0xCE,"ほ"},
    {0xCF,"ま"},{0xD0,"み"},{0xD1,"む"},{0xD2,"め"},{0xD3,"も"},{0xD4,"や"},{0xD5,"ゆ"},{0xD6,"よ"},{0xD7,"ら"},{0xD8,"リ"},
    {0xD9,"る"},{0xDA,"れ"},{0xDB,"ろ"},{0xDC,"わ"},{0xDD,"を"},{0xDE,"ん"},{0xDF,"っ"},{0xE0,"ゃ"},{0xE1,"ゅ"},{0xE2,"ょ"},
    {0xE3,"ー"},{0xE4,"ﾟ"},{0xE5,"ﾞ"},{0xE6,"？"},{0xE7,"！"},{0xE8,"。"},{0xE9,"ァ"},{0xEA,"ゥ"},{0xEB,"ェ"},{0xEF,"♂"},
    {0xF0,"¥"},{0xF1,"×"},{0xF2,"．"},{0xF3,"／"},{0xF4,"ォ"},{0xF5,"♀"},
    {0xF6,"０"},{0xF7,"１"},{0xF8,"２"},{0xF9,"３"},{0xFA,"４"},{0xFB,"５"},{0xFC,"６"},{0xFD,"７"},{0xFE,"８"},{0xFF,"９"},
    // Hiragana typed for the four shared glyphs (PKHeX StringConverter1.Hiragana remap).
    // Must stay last: keyboardPage() treats the final kRemapCount entries as input-only remaps.
    {0x3D,"べ"},{0x47,"ぺ"},{0xCD,"へ"},{0xD8,"り"},
};
inline constexpr std::size_t kGlyphCount = sizeof(kGlyphs) / sizeof(kGlyphs[0]);
inline constexpr std::size_t kRemapCount = 4;

enum class Result : uint8_t { Ok, Empty, TooLong, Unsupported };

// Encodes one UTF-8 name into `out` (filled with 0x50 first). `maxChars` counts characters.
// Longest match first, so every multi-byte UTF-8 character is matched whole.
[[nodiscard]] inline Result encode(std::string_view text, std::size_t maxChars, std::vector<uint8_t>& out,
                                   std::string* badChar = nullptr) {
    out.assign(maxChars + 1, kTerminator);
    if (text.empty()) return Result::Empty;
    std::size_t count = 0;
    for (std::size_t i = 0; i < text.size();) {
        const Glyph* hit = nullptr;
        for (const auto& g : kGlyphs) {
            if (g.text.size() > text.size() - i) continue;
            if (text.compare(i, g.text.size(), g.text) == 0 && (!hit || g.text.size() > hit->text.size())) hit = &g;
        }
        if (!hit) {
            if (badChar) {
                const unsigned char c = static_cast<unsigned char>(text[i]);
                const std::size_t len = c >= 0xF0 ? 4 : c >= 0xE0 ? 3 : c >= 0xC0 ? 2 : 1;
                *badChar = std::string(text.substr(i, len));
            }
            return Result::Unsupported;
        }
        if (count >= maxChars) return Result::TooLong;
        out[count++] = hit->byte;
        i += hit->text.size();
    }
    return Result::Ok;
}

[[nodiscard]] inline std::string_view resultText(Result r) noexcept {
    switch (r) {
        case Result::Ok: return "";
        case Result::Empty: return "Name is empty";
        case Result::TooLong: return "Japanese names hold 5 characters";
        case Result::Unsupported: return "This character is not in the Japanese Gen I/II font";
    }
    return "";
}

// Keyboard pages for the Japanese mode switch (ABC -> Hiragana -> Katakana). Each lists only glyphs
// the table above encodes; the shared べぺへり appear on the Hiragana page in hiragana form.
enum class KeyboardMode : uint8_t { Latin, Hiragana, Katakana };

[[nodiscard]] inline KeyboardMode nextMode(KeyboardMode m, bool japanese) noexcept {
    if (!japanese) return KeyboardMode::Latin;
    return m == KeyboardMode::Latin ? KeyboardMode::Hiragana
         : m == KeyboardMode::Hiragana ? KeyboardMode::Katakana : KeyboardMode::Latin;
}

[[nodiscard]] inline std::string_view modeLabel(KeyboardMode m) noexcept {
    return m == KeyboardMode::Hiragana ? "あ" : m == KeyboardMode::Katakana ? "ア" : "ABC";
}

namespace detail {
inline bool isHiragana(std::string_view t) {
    // U+3041..U+3096 encode as E3 81 81..E3 82 96.
    if (t.size() != 3 || static_cast<unsigned char>(t[0]) != 0xE3) return false;
    const unsigned b1 = static_cast<unsigned char>(t[1]), b2 = static_cast<unsigned char>(t[2]);
    const unsigned cp = 0x3000 + ((b1 & 0x3F) << 6) + (b2 & 0x3F);
    return cp >= 0x3041 && cp <= 0x3096;
}
inline bool isKatakana(std::string_view t) {
    if (t.size() != 3 || static_cast<unsigned char>(t[0]) != 0xE3) return false;
    const unsigned b1 = static_cast<unsigned char>(t[1]), b2 = static_cast<unsigned char>(t[2]);
    const unsigned cp = 0x3000 + ((b1 & 0x3F) << 6) + (b2 & 0x3F);
    return cp >= 0x30A1 && cp <= 0x30FA;
}
}  // namespace detail

// Characters of one keyboard page, in table order. Symbols and digits are on both kana pages.
[[nodiscard]] inline std::vector<std::string_view> keyboardPage(KeyboardMode mode) {
    std::vector<std::string_view> out;
    if (mode == KeyboardMode::Latin) return out;   // the existing Latin keyboard is used as is
    for (std::size_t i = 0; i < kGlyphCount; ++i) {
        const auto& g = kGlyphs[i];
        const bool hira = detail::isHiragana(g.text), kata = detail::isKatakana(g.text);
        const bool remapEntry = i >= kGlyphCount - kRemapCount;
        const bool symbol = !hira && !kata;
        if (mode == KeyboardMode::Hiragana) {
            // The shared bytes are katakana in the table; the page offers their hiragana remaps.
            if (hira || (symbol && !remapEntry)) out.push_back(g.text);
        } else if (kata || symbol) {
            out.push_back(g.text);
        }
    }
    return out;
}

}  // namespace Utils::Gen12Kana
