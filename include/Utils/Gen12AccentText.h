#pragma once

// European accented letters in Generation I/II international names (M1 D4).
//
// Sources (PKHeX 8e3eb3d4, PKHeX.Core/PKM/Strings):
//   StringConverter1.TableEN  - Gen I uses ONE table for every non-Japanese release, so every
//                               accent below has a single byte.
//   StringConverter2.TableEN, TableFRE (French, German), TableITA (Italian, Spanish) - Gen II
//                               has three tables that disagree on 0xBE-0xBF, 0xC6-0xCC and
//                               0xD0-0xD6 (e.g. 0xC6 is È in EN/ITA but ë in FRE).
// A Gen II save does not record which European release wrote it, so a Gen II name may only use
// the accents whose byte is the same in all three tables. Any other accent is refused, never
// guessed: writing it under the wrong table would show a different letter in the game.

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace Utils {

struct Gen12Accent {
    char16_t ch;
    uint8_t byte;
    bool gen2AllReleases;  // identical byte in StringConverter2 TableEN, TableFRE and TableITA
};

inline constexpr Gen12Accent kGen12Accents[] = {
    {u'à', 0xBA, true},  {u'è', 0xBB, true},  {u'é', 0xBC, true},  {u'ù', 0xBD, true},
    {u'À', 0xBE, false}, {u'Á', 0xBF, false},
    {u'Ä', 0xC0, true},  {u'Ö', 0xC1, true},  {u'Ü', 0xC2, true},
    {u'ä', 0xC3, true},  {u'ö', 0xC4, true},  {u'ü', 0xC5, true},
    {u'È', 0xC6, false}, {u'É', 0xC7, false}, {u'Ì', 0xC8, false}, {u'Í', 0xC9, false},
    {u'Ñ', 0xCA, false}, {u'Ò', 0xCB, false}, {u'Ó', 0xCC, false},
    {u'Ù', 0xCD, true},  {u'Ú', 0xCE, true},  {u'á', 0xCF, true},
    {u'ì', 0xD0, false}, {u'í', 0xD1, false}, {u'ñ', 0xD2, false},
    {u'ò', 0xD3, false}, {u'ó', 0xD4, false}, {u'ú', 0xD5, false},
};

enum class Gen12AccentResult { NotAccent, Encoded, ReleaseDependent };

// Reads one two-byte UTF-8 Latin-1 letter (U+00C0-U+00FF) at text[at]. On a match, `consumed`
// is 2 and `byte` is the native byte. Gen II refuses accents whose byte differs by release.
inline Gen12AccentResult gen12AccentByte(std::string_view text, std::size_t at, bool gen2,
                                         uint8_t& byte, std::size_t& consumed) noexcept {
    consumed = 0;
    if (at + 1 >= text.size() || static_cast<unsigned char>(text[at]) != 0xC3) return Gen12AccentResult::NotAccent;
    const unsigned char tail = static_cast<unsigned char>(text[at + 1]);
    if (tail < 0x80 || tail > 0xBF) return Gen12AccentResult::NotAccent;
    const char16_t ch = static_cast<char16_t>(0xC0 + (tail - 0x80));
    for (const auto& accent : kGen12Accents) {
        if (accent.ch != ch) continue;
        if (gen2 && !accent.gen2AllReleases) return Gen12AccentResult::ReleaseDependent;
        byte = accent.byte;
        consumed = 2;
        return Gen12AccentResult::Encoded;
    }
    return Gen12AccentResult::NotAccent;
}

}  // namespace Utils
