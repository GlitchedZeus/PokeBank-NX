#pragma once

// The name a Pokemon carries when it has no nickname, in its own language, formatted the way each
// generation's games store it. Mirrors PKHeX SpeciesName.GetSpeciesNameGeneration (8e3eb3d4,
// PKHeX.Core/PKM/Util/SpeciesName.cs:114-200):
//   Gen III/IV: Japanese and Korean as listed; every other language upper case, French with its
//               E/I accents stripped (StringConverter4Util.StripDiacriticsFR4), and Farfetch'd with
//               a straight apostrophe.
//   Gen V:      the listed name, except Farfetch'd uses a straight apostrophe in English, Italian
//               and Spanish (Gen V has no slanted apostrophe).
// Gen I/II formatting (no spaces, MR.MIME period) is not covered here yet.

#include "Enums/LanguageID.h"
#include "Names/NameLanguage.h"
#include "Names/SpeciesNames.h"
#include "Utils/StringHelpers.h"

#include <cstdint>
#include <string>

namespace Names {

inline std::u16string defaultSpeciesName(uint16_t species, uint8_t language, uint8_t generation) {
    using Enums::LanguageID;
    auto value = Utils::utf8ToUtf16(
        getSpeciesNameLocalized(species, languageIndexFor(static_cast<LanguageID>(language))));

    if (generation >= 5) {
        if (language == static_cast<uint8_t>(LanguageID::English) ||
            language == static_cast<uint8_t>(LanguageID::Italian) ||
            language == static_cast<uint8_t>(LanguageID::Spanish))
            for (auto& ch : value) if (ch == u'’') ch = u'\'';
        return value;
    }

    if (language == static_cast<uint8_t>(LanguageID::Japanese) ||
        language == static_cast<uint8_t>(LanguageID::Korean))
        return value;

    for (auto& ch : value) {
        if (ch >= u'a' && ch <= u'z') ch = static_cast<char16_t>(ch - (u'a' - u'A'));
        if (language == static_cast<uint8_t>(LanguageID::French)) {
            switch (ch) {
                case u'É': case u'È': case u'Ê': case u'Ë':
                case u'é': case u'è': case u'ê': case u'ë': ch = u'E'; break;
                case u'Î': case u'Ï': case u'î': case u'ï': ch = u'I'; break;
                default: break;
            }
        }
        if (ch == u'’') ch = u'\'';
    }
    return value;
}

}  // namespace Names
