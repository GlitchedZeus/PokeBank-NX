// Gen III PID gender accessor (PokemonRecord::gender) for Home cards and summaries.
// kGen3GenderRatio is cross-checked byte-for-byte against vendor/PKSM-Core personals/personal3
// (+0x8) and against PokeBank's PKHeX-derived PersonalInfo table; PID-threshold cases follow PKSM
// PK3::gender() / PKHeX EntityGender, including fixed-gender and genderless species.
#include "Integration/Gen3/PKSMGen3Adapter.h"
#include "Pokemon/PersonalInfoTable.h"

#include <cassert>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>

using namespace PokeVault::Integration::Gen3;

int main() {
    std::ifstream in("vendor/PKSM-Core/personals/personal3", std::ios::binary);
    assert(in);
    const std::vector<uint8_t> personal3{std::istreambuf_iterator<char>(in), {}};
    assert(personal3.size() == 5031 && personal3.size() / 13 == kGen3GenderRatio.size());
    for (uint16_t species = 1; species <= 386; ++species) {
        assert(kGen3GenderRatio[species] == personal3[species * 13 + 8]);
        assert(kGen3GenderRatio[species] == Pokemon::getPersonalInfo(species, 0).genderRatio);
    }

    static_assert(gen3GenderFromPid(32, 0x00) == Gen3Gender::Male);        // Nidoran M (0)
    static_assert(gen3GenderFromPid(29, 0xFF) == Gen3Gender::Female);      // Nidoran F (254)
    static_assert(gen3GenderFromPid(113, 0x12345600) == Gen3Gender::Female); // Chansey
    static_assert(gen3GenderFromPid(81, 0x7F) == Gen3Gender::Genderless);  // Magnemite (255)
    static_assert(gen3GenderFromPid(0, 0) == Gen3Gender::Unknown);
    static_assert(gen3GenderFromPid(387, 0) == Gen3Gender::Unknown);
    // Pikachu 127: low byte below the ratio is female, at/above is male; only the low byte counts.
    static_assert(gen3GenderFromPid(25, 0x0000007E) == Gen3Gender::Female);
    static_assert(gen3GenderFromPid(25, 0x0000007F) == Gen3Gender::Male);
    static_assert(gen3GenderFromPid(25, 0xFFFFFF00) == Gen3Gender::Female);
    // Bulbasaur 31 (87.5% male).
    static_assert(gen3GenderFromPid(1, 30) == Gen3Gender::Female);
    static_assert(gen3GenderFromPid(1, 31) == Gen3Gender::Male);

    PokemonRecord record;
    record.species = 25;
    record.pid = 0x12345610;
    assert(record.gender() == Gen3Gender::Female);
    record.pid = 0x123456F0;
    assert(record.gender() == Gen3Gender::Male);
    record.species = 81;
    assert(record.gender() == Gen3Gender::Genderless);

    std::cout << "Gen III PID gender accessor (PKSM personal3 + PersonalInfo cross-check): PASS\n";
}
