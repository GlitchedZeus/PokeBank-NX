#pragma once

// Gen III-origin native PK4 species may be a Gen IV evolution, even when
// its stored National species ID did not exist in the GBA games. Invert
// the pinned PokeAPI Gen IV evolution chain before claiming impossibility.
// Does not reconstruct evolution level, location, timing, gender or trade.

#include "Legality/Gen34EggMoveEvidence.h"
#include <cstdint>

namespace Legality::Gen34PalParkSpecies {

enum class Evidence : uint8_t {
    Unknown,
    Gen3NativeSpecies,
    Gen4EvolutionFromGen3,
    NoGen3Ancestor,
};

// An already-evolved Gen IV Pokemon could have entered Pal Park as its
// nearest existing Gen III ancestor. That ancestor's full GBA compatibility
// pool (including its older pre-evolutions) is a positive candidate for
// retained moves; it does not prove which stage actually crossed Pal Park.
constexpr uint16_t nearestGen3Ancestor(uint16_t species) noexcept {
    if(species==0 || species>493)return 0;
    if(species<=386)return species;
    uint16_t current=species;
    for(int depth=0;depth<12;++depth) {
        const uint16_t parent=Gen34EggMove::preEvolution(
            "platinum_nds",current);
        if(parent==0 || parent==current)return 0;
        if(parent<=386)return parent;
        current=parent;
    }
    return 0;
}

constexpr Evidence classify(uint16_t species) noexcept {
    if(species==0 || species>493)
        return Evidence::Unknown; // Defer to storage-format domain.
    if(species<=386)
        return Evidence::Gen3NativeSpecies;

    uint16_t current=species;
    for(int depth=0;depth<12;++depth) {
        const uint16_t parent=Gen34EggMove::preEvolution(
            "platinum_nds",current);
        if(parent==0)
            return Evidence::NoGen3Ancestor;
        if(parent==current)
            return Evidence::Unknown; // Defensive against corrupt source.
        if(parent<=386)
            return Evidence::Gen4EvolutionFromGen3;
        current=parent;
    }
    return Evidence::Unknown; // Incomplete/cyclic lineage not Invalid.
}

} // namespace Legality::Gen34PalParkSpecies
