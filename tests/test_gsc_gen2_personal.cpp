#include "Integration/Gen2/Gen2PersonalData.h"

#include <cassert>
#include <iostream>

using namespace PokeVault::Integration::Gen2;

int main() {
    // Always-male: Nidoran-M (#32).
    assert(personalRecord(32) && personalRecord(32)->genderRatio == 0);
    assert(genderFromAttackDV(32, 0) == PokemonGender::Male);
    assert(genderFromAttackDV(32, 15) == PokemonGender::Male);

    // Always-female: Nidoran-F (#29).
    assert(personalRecord(29) && personalRecord(29)->genderRatio == 254);
    assert(genderFromAttackDV(29, 0) == PokemonGender::Female);
    assert(genderFromAttackDV(29, 15) == PokemonGender::Female);

    // Genderless: Magnemite (#81).
    assert(personalRecord(81) && personalRecord(81)->genderRatio == 255);
    assert(genderFromAttackDV(81, 0) == PokemonGender::Genderless);
    assert(genderFromAttackDV(81, 15) == PokemonGender::Genderless);

    // 12.5% female: Eevee (#133), threshold 31 >> 4 == 1.
    assert(personalRecord(133) && personalRecord(133)->genderRatio == 31);
    assert(genderFromAttackDV(133, 1) == PokemonGender::Female);
    assert(genderFromAttackDV(133, 2) == PokemonGender::Male);

    // 25% female: Growlithe (#58), threshold 63 >> 4 == 3.
    assert(personalRecord(58) && personalRecord(58)->genderRatio == 63);
    assert(genderFromAttackDV(58, 3) == PokemonGender::Female);
    assert(genderFromAttackDV(58, 4) == PokemonGender::Male);

    // 50% female: Pikachu (#25), threshold 127 >> 4 == 7.
    assert(personalRecord(25) && personalRecord(25)->genderRatio == 127);
    assert(genderFromAttackDV(25, 7) == PokemonGender::Female);
    assert(genderFromAttackDV(25, 8) == PokemonGender::Male);

    // 75% female: Clefairy (#35), threshold 191 >> 4 == 11.
    assert(personalRecord(35) && personalRecord(35)->genderRatio == 191);
    assert(genderFromAttackDV(35, 11) == PokemonGender::Female);
    assert(genderFromAttackDV(35, 12) == PokemonGender::Male);

    // Fail closed when the species/Attack DV is outside Gen II bounds.
    assert(personalRecord(0) == nullptr);
    assert(personalRecord(252) == nullptr);
    assert(genderFromAttackDV(0, 7) == PokemonGender::Unknown);
    assert(genderFromAttackDV(25, 16) == PokemonGender::Unknown);

    std::cout << "Gen II personal/gender oracle tests: PASS\n";
}
