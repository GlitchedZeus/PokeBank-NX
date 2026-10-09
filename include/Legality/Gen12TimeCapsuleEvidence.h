#pragma once

#include <array>
#include <cstdint>

namespace Legality::Gen12TimeCapsule {

constexpr bool isGen2EvolutionOfGen1Species(uint16_t species) noexcept {
    switch (species) {
        case 169: // Crobat
        case 182: // Bellossom
        case 186: // Politoed
        case 196: // Espeon
        case 197: // Umbreon
        case 199: // Slowking
        case 208: // Steelix
        case 212: // Scizor
        case 230: // Kingdra
        case 233: // Porygon2
        case 242: // Blissey
            return true;
        default:
            return false;
    }
}

// Mirrors PKHeX GBRestrictions.CanVisitGen1 for entities that can exist in a Gen II format:
// either the species itself existed in Gen I, or it can devolve to a Gen I species.
constexpr bool canVisitGen1Species(uint16_t species) noexcept {
    return species >= 1 && (species <= 151 || isGen2EvolutionOfGen1Species(species));
}

// Whether a Gen II-format entity could have originated from Gen I before evolving/learning in Gen II.
// PKHeX rejects eggs and Gen II entities with caught-data when inferring a Gen I origin.
constexpr bool couldOriginateGen1(uint16_t species, bool isEgg, uint16_t caughtData) noexcept {
    return !isEgg && caughtData == 0 && canVisitGen1Species(species);
}

// Whether the entity's CURRENT Gen II species/moves are acceptable to send through Time Capsule
// into a Gen I cartridge. This is compatibility evidence, not proof that the trade occurred.
constexpr bool canCurrentlyTradeToGen1(
    uint16_t species, bool isEgg, const std::array<uint16_t, 4>& moves) noexcept {
    if (isEgg || species == 0 || species > 151)
        return false;
    for (const uint16_t move : moves) {
        if (move > 165)
            return false;
    }
    return true;
}

} // namespace Legality::Gen12TimeCapsule
