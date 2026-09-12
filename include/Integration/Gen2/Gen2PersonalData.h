#ifndef POKEBANK_GEN2_PERSONAL_DATA_H
#define POKEBANK_GEN2_PERSONAL_DATA_H

#include <cstdint>

namespace PokeVault::Integration::Gen2 {

// Selective read-only port of PKSM-Core PersonalGSC data at
// aa22d7a4f87c0351baf7da5962ba5acd01039a7c, personals/personal2.
// PKHeX 77dcd3a7895bceaafbbff12d25bdf77c1acd8ca5 independently confirms
// PersonalInfo2's Gen II gender ratio field and Attack-DV threshold semantics.
struct PersonalRecord {
    uint8_t hp = 0;
    uint8_t attack = 0;
    uint8_t defense = 0;
    uint8_t speed = 0;
    uint8_t specialAttack = 0;
    uint8_t specialDefense = 0;
    uint8_t rawType1 = 0;
    uint8_t rawType2 = 0;
    uint8_t genderRatio = 0xFF;
    uint8_t experienceGrowth = 0;
};

enum class PokemonGender : uint8_t {
    Male = 0,
    Female = 1,
    Genderless = 2,
    Unknown = 3,
};

const PersonalRecord* personalRecord(uint16_t species) noexcept;

// Gen II retail rule: special ratios 0/254/255 are male/female/genderless.
// Otherwise female iff Attack DV <= (genderRatio >> 4), male above it.
PokemonGender genderFromAttackDV(uint16_t species, uint8_t attackDV) noexcept;

} // namespace PokeVault::Integration::Gen2

#endif
