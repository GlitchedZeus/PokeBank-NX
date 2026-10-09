#pragma once

#include "Pokemon/PersonalInfo4DP.h"
#include "Pokemon/PersonalInfo4HGSS.h"
#include "Pokemon/PersonalInfo4PT.h"

#include <cstdint>
#include <string_view>

namespace Legality::Gen4Form {

// Pinned PKHeX FormInfo.FormChange species that can change form while retaining
// the same encounter identity. Future-generation form indices are still blocked
// separately by the exact-game native formCount check below.
constexpr bool formChangeableSpecies(uint16_t speciesId) noexcept {
    switch (speciesId) {
        case 386: // Deoxys
        case 412: // Burmy
        case 479: // Rotom
        case 483: // Dialga
        case 484: // Palkia
        case 487: // Giratina
        case 492: // Shaymin
        case 493: // Arceus
            return true;
        default:
            return false;
    }
}

constexpr bool formCompatible(uint16_t speciesId, uint8_t encounterForm,
                              uint8_t currentForm) noexcept {
    return encounterForm == currentForm || formChangeableSpecies(speciesId);
}

inline const Pokemon::PersonalRecord* basePersonal(
    std::string_view exactGameId, uint16_t species) noexcept {
    if (species == 0 || species > 493)
        return nullptr;
    if (exactGameId == "diamond_nds" || exactGameId == "pearl_nds")
        return &Pokemon::getPersonalInfo4DP(species, 0);
    if (exactGameId == "platinum_nds")
        return &Pokemon::getPersonalInfo4PT(species, 0);
    if (exactGameId == "heartgold_nds" || exactGameId == "soulsilver_nds")
        return &Pokemon::getPersonalInfo4HGSS(species, 0);
    return nullptr;
}

inline uint8_t formCount(std::string_view exactGameId,
                         uint16_t species) noexcept {
    const auto* personal = basePersonal(exactGameId, species);
    return personal ? personal->formCount : 0;
}

inline bool isFormValid(std::string_view exactGameId, uint16_t species,
                        uint8_t form) noexcept {
    const uint8_t count = formCount(exactGameId, species);
    return count != 0 && form < count;
}

} // namespace Legality::Gen4Form
