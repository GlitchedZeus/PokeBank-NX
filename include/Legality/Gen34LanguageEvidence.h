#ifndef LEGALITY_GEN34_LANGUAGE_EVIDENCE_H
#define LEGALITY_GEN34_LANGUAGE_EVIDENCE_H

#include <cstdint>

namespace Legality::Gen34Language {

    enum class Result : uint8_t {
        Unresolved,
        Valid,
        Invalid,
    };

    // PKHeX reference: Legal.GetMaxLanguageID / LanguageVerifier at
    // 6501f0ab46e8f8ca048539dbaf8cae8cb104e722.
    //
    // PKSE/PokeBank currently exposes 0 for some unwired language fields. Keep
    // that representation conservative here: an unwired value is unresolved,
    // never proof of illegality. Gen III/IV wired values use the source-backed
    // generation maxima and reject the unused language id 6.
    constexpr Result classify(uint8_t generation, uint8_t language) noexcept {
        if (generation != 3 && generation != 4)
            return Result::Unresolved;
        if (language == 0)
            return Result::Unresolved;
        if (language == 6)
            return Result::Invalid;

        const uint8_t maxLanguage = generation == 3 ? 7 : 8;
        return language <= maxLanguage ? Result::Valid : Result::Invalid;
    }

    constexpr bool isInvalid(uint8_t generation, uint8_t language) noexcept {
        return classify(generation, language) == Result::Invalid;
    }

} // namespace Legality::Gen34Language

#endif // LEGALITY_GEN34_LANGUAGE_EVIDENCE_H
