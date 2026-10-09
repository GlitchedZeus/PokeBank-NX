#pragma once
#include "Legality/Gen2StaticEncounter.h"
#include <cstdint>
#include <string_view>

namespace Legality::Gen2CrystalEeveeGift {
// Pinned Crystal static Eevee at location16 level20; original receipt
// data survives Gen II stone/friendship evolution. Information only.
// PK2 lacks unique acquisition game/trade/evolution chronology.
enum class Evidence : uint8_t { Unresolved, StoneEvolution, FriendshipEvolution };

constexpr Evidence evolutionType(uint16_t species) noexcept {
    switch (species) {
        case 134: case 135: case 136:
            return Evidence::StoneEvolution;
        case 196: case 197:
            return Evidence::FriendshipEvolution;
        default: return Evidence::Unresolved;
    }
}
inline Evidence analyze(std::string_view exactContainerGame,
                        uint16_t species, uint8_t currentLevel,
                        uint16_t caughtData, bool isEgg, bool isShiny) noexcept {
    const auto kind=evolutionType(species);
    if(exactContainerGame!="crystal_gbc" || isEgg || caughtData==0 ||
       kind==Evidence::Unresolved)
        return Evidence::Unresolved;
    // Evolution stones may be applied at the gift's current level.
    // Friendship evolution requires a later distinct level-up event.
    if(currentLevel < (kind==Evidence::StoneEvolution ? 20 : 21))
        return Evidence::Unresolved;
    if(!Gen2Static::matches(
            exactContainerGame,133,20,caughtData,false,isShiny))
        return Evidence::Unresolved;
    return kind;
}
} // namespace Legality::Gen2CrystalEeveeGift
