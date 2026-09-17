#include "Integration/Gen1/Gen1StagedPokemonEditor.h"
#include "Integration/Gen2/Gen2PersonalData.h"
#include "UI/ClassicGameContext.h"

#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>

int main() {
    using PokeVault::Integration::Gen1::StagedPokemonEditor;
    namespace Gen2 = PokeVault::Integration::Gen2;
    namespace Classic = PokeBank::UIModel::ClassicGameContext;

    // The shared picker must use the native table for the generation being edited.
    const auto bulbasaur = StagedPokemonEditor::personalTypes(1);
    const auto charmeleon = StagedPokemonEditor::personalTypes(5);
    const auto squirtle = StagedPokemonEditor::personalTypes(7);
    const auto pikachu = StagedPokemonEditor::personalTypes(25);

    assert((bulbasaur == std::array<uint8_t, 2>{22, 3}));      // Grass / Poison
    assert((charmeleon == std::array<uint8_t, 2>{20, 20}));   // Fire
    assert((squirtle == std::array<uint8_t, 2>{21, 21}));     // Water
    assert((pikachu == std::array<uint8_t, 2>{23, 23}));      // Electric

    assert(Classic::normalizedTypeSpriteId(bulbasaur[0]) == 11);
    assert(Classic::normalizedTypeSpriteId(bulbasaur[1]) == 3);
    assert(Classic::normalizedTypeSpriteId(charmeleon[0]) == 9);
    assert(Classic::normalizedTypeSpriteId(squirtle[0]) == 10);
    assert(Classic::normalizedTypeSpriteId(pikachu[0]) == 12);

    // GSC must not silently reuse RBY typing. Magnemite is Electric/Steel in Gen II.
    const auto* gscBulbasaur = Gen2::personalRecord(1);
    const auto* gscMagnemite = Gen2::personalRecord(81);
    const auto* gscPikachu = Gen2::personalRecord(25);
    assert(gscBulbasaur && gscMagnemite && gscPikachu);
    assert(gscBulbasaur->rawType1 == 22 && gscBulbasaur->rawType2 == 3);
    assert(gscMagnemite->rawType1 == 23 && gscMagnemite->rawType2 == 9);
    assert(gscPikachu->rawType1 == 23 && gscPikachu->rawType2 == 23);
    assert(Classic::normalizedTypeSpriteId(gscMagnemite->rawType2) == 8);

    std::cout << "Classic species-picker generation-correct type contract: PASS\n";
    return 0;
}
