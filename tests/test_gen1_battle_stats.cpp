#include "Integration/Gen1/Gen1StagedPokemonEditor.h"

#include <array>
#include <cassert>
#include <iostream>

using PokeVault::Integration::Gen1::StagedPokemonEditor;

int main() {
    // Default Add-draft Bulbasaur: level 5, four stored DVs = 8, zero Stat Exp.
    // HP DV derives to zero because all four stored DVs are even.
    const auto bulbasaur = StagedPokemonEditor::calculateBattleStats(
        1, 5, std::array<uint8_t,4>{8,8,8,8}, std::array<uint16_t,5>{0,0,0,0,0});
    assert((bulbasaur.asArray() == std::array<uint16_t,5>{19,10,10,10,12}));

    // Published Generation I Pikachu fixture: HP uses derived DV 7 and the
    // ceil(sqrt(Stat Exp))/4 contribution. Five stats only; Special is unified.
    const auto pikachu = StagedPokemonEditor::calculateBattleStats(
        25, 81, std::array<uint8_t,4>{8,13,5,9},
        std::array<uint16_t,5>{22850,23140,17280,24795,19625});
    assert((pikachu.asArray() == std::array<uint16_t,5>{189,137,101,190,128}));

    // Invalid draft identities fail closed rather than displaying fake zero-derived values as valid.
    assert((StagedPokemonEditor::calculateBattleStats(0, 5, {8,8,8,8}, {0,0,0,0,0}).asArray() ==
            std::array<uint16_t,5>{0,0,0,0,0}));
    assert((StagedPokemonEditor::calculateBattleStats(1, 0, {8,8,8,8}, {0,0,0,0,0}).asArray() ==
            std::array<uint16_t,5>{0,0,0,0,0}));
    assert((StagedPokemonEditor::calculateBattleStats(1, 5, {16,8,8,8}, {0,0,0,0,0}).asArray() ==
            std::array<uint16_t,5>{0,0,0,0,0}));

    std::cout << "Gen I exact five-stat battle preview: PASS\n";
    return 0;
}
