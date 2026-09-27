#ifndef NAMES_NAMELANGUAGE_H
#define NAMES_NAMELANGUAGE_H

#include <cstddef>
#include <cstdint>

#include "Enums/LanguageID.h"

namespace Names {

    // Generated name-table order. This is deliberately not Enums::LanguageID numbering.
    enum class NameLanguage : uint8_t {
        Japanese = 0,
        English = 1,
        French = 2,
        Italian = 3,
        German = 4,
        Spanish = 5,
        Korean = 6,
        ChineseSimplified = 7,
        ChineseTraditional = 8,
    };

    inline constexpr std::size_t LANGUAGE_COUNT = 9;
    inline constexpr std::size_t LANGUAGE_INDEX_ENGLISH = 1;

    // Map an entity/save language byte to the generated name-table index. Unsupported or
    // unset values fall back to English rather than indexing out of range.
    inline constexpr std::size_t languageIndexFor(Enums::LanguageID languageId) noexcept {
        switch (languageId) {
            case Enums::LanguageID::Japanese: return static_cast<std::size_t>(NameLanguage::Japanese);
            case Enums::LanguageID::English: return static_cast<std::size_t>(NameLanguage::English);
            case Enums::LanguageID::French: return static_cast<std::size_t>(NameLanguage::French);
            case Enums::LanguageID::Italian: return static_cast<std::size_t>(NameLanguage::Italian);
            case Enums::LanguageID::German: return static_cast<std::size_t>(NameLanguage::German);
            case Enums::LanguageID::Spanish:
            case Enums::LanguageID::SpanishL:
                return static_cast<std::size_t>(NameLanguage::Spanish);
            case Enums::LanguageID::Korean: return static_cast<std::size_t>(NameLanguage::Korean);
            case Enums::LanguageID::ChineseSimplified:
                return static_cast<std::size_t>(NameLanguage::ChineseSimplified);
            case Enums::LanguageID::ChineseTraditional:
                return static_cast<std::size_t>(NameLanguage::ChineseTraditional);
            default:
                return LANGUAGE_INDEX_ENGLISH;
        }
    }
}

#endif
