#pragma once

#include "Pokemon/PersonalInfo4DP.h"
#include "Pokemon/PersonalInfo4HGSS.h"
#include "Pokemon/PersonalInfo4PT.h"

#include <cstdint>
#include <string_view>

namespace Legality::Gen4Form {

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
