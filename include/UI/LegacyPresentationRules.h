#ifndef UI_LEGACY_PRESENTATION_RULES_H
#define UI_LEGACY_PRESENTATION_RULES_H

#include "Enums/GameVersion.h"
#include "Integration/Gen1/Gen1ReadOnlyInventory.h"
#include "Integration/Gen2/Gen2ReadOnlyInventory.h"

namespace UI {

enum class PokemonDetailsLayout : unsigned char {
    Modern,
    Generation1,
    Generation2,
};

constexpr PokemonDetailsLayout pokemonDetailsLayoutFor(
    Enums::GameVersion group) noexcept {
    switch (group) {
        case Enums::GameVersion::RBY: return PokemonDetailsLayout::Generation1;
        case Enums::GameVersion::GSC: return PokemonDetailsLayout::Generation2;
        default: return PokemonDetailsLayout::Modern;
    }
}

// Only legacy groups whose pouch count is not represented by the existing modern/Gen III
// navigation switches return a non-zero value here. A zero result means "preserve the existing
// generation-specific navigation path" rather than "there are no categories".
constexpr int legacyInventoryCategoryCount(Enums::GameVersion group) noexcept {
    switch (group) {
        case Enums::GameVersion::RBY:
            return static_cast<int>(PokeVault::Integration::Gen1::kInventoryCategoryCount);
        case Enums::GameVersion::GSC:
            return static_cast<int>(PokeVault::Integration::Gen2::kInventoryPocketCount);
        default:
            return 0;
    }
}

} // namespace UI

#endif
