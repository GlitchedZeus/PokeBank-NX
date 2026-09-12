#include "UI/LegacyPresentationRules.h"

#include <cassert>
#include <iostream>

int main() {
    using Enums::GameVersion;
    using UI::PokemonDetailsLayout;

    static_assert(PokeVault::Integration::Gen1::kInventoryCategoryCount == 2);
    static_assert(PokeVault::Integration::Gen2::kInventoryPocketCount == 5);

    assert(UI::legacyInventoryCategoryCount(GameVersion::RBY) == 2);
    assert(UI::legacyInventoryCategoryCount(GameVersion::GSC) == 5);
    assert(UI::legacyInventoryCategoryCount(GameVersion::FRLG) == 0);

    assert(UI::pokemonDetailsLayoutFor(GameVersion::RBY) == PokemonDetailsLayout::Generation1);
    assert(UI::pokemonDetailsLayoutFor(GameVersion::GSC) == PokemonDetailsLayout::Generation2);
    assert(UI::pokemonDetailsLayoutFor(GameVersion::FRLG) == PokemonDetailsLayout::Modern);
    assert(UI::pokemonDetailsLayoutFor(GameVersion::SV) == PokemonDetailsLayout::Modern);

    std::cout << "GSC legacy UI presentation rules: PASS\n";
}
