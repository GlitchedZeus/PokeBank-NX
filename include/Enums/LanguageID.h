#ifndef ENUMS_ENUMS_H
#define ENUMS_ENUMS_H

#include <cstdint>
#include <cstddef>

#include "Enums/GameVersion.h"

namespace Enums {
    enum class LanguageID {
        Hacked,
        Japanese,
        English,
        French,
        Italian,
        German,
        UNUSED_6,
        Spanish,
        Korean,
        ChineseSimplified,
        ChineseTraditional,
        SpanishL
    };

    inline const char* getLanguageName(uint8_t id) {
        static const char* const names[] = {
            "-", "Japanese", "English", "French", "Italian", "German",
            "-", "Spanish", "Korean", "Chinese (S)", "Chinese (T)", "Spanish (LATAM)"
        };
        return id < (sizeof(names) / sizeof(names[0])) ? names[id] : "-";
    }

    // Current PokeBank NX groups only. This is the PKSE 1.2 / PKHeX language-availability rule
    // narrowed to groups that actually exist before the Gen IV expansion.
    inline constexpr bool groupHasLanguage(GameVersion group, uint8_t languageId) noexcept {
        const auto language = static_cast<LanguageID>(languageId);
        if (language == LanguageID::Hacked || language == LanguageID::UNUSED_6) return false;
        if (language <= LanguageID::Spanish) return true;

        switch (group) {
            case GameVersion::RBY:
            case GameVersion::FRLG:
                return false; // Gen I/III never shipped in Korean/Chinese/Spanish-LATAM.
            case GameVersion::GSC:
                return language == LanguageID::Korean;
            case GameVersion::ZA:
                return true;
            case GameVersion::GG:
            case GameVersion::SWSH:
            case GameVersion::BDSP:
            case GameVersion::PLA:
            case GameVersion::SV:
            case GameVersion::Gen7B:
            case GameVersion::Gen8:
            case GameVersion::Gen9:
                return language == LanguageID::Korean ||
                       language == LanguageID::ChineseSimplified ||
                       language == LanguageID::ChineseTraditional;
            default:
                return false;
        }
    }

    inline constexpr uint8_t safeLanguageForGroup(GameVersion group, uint8_t languageId) noexcept {
        return groupHasLanguage(group, languageId)
            ? languageId : static_cast<uint8_t>(LanguageID::English);
    }
}

#endif
