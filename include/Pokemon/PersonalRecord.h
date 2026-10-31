#ifndef POKEMON_PERSONAL_RECORD_H
#define POKEMON_PERSONAL_RECORD_H

#include <cstddef>
#include <cstdint>

namespace Pokemon {
    struct PersonalRecord {
        uint8_t hp, atk, def, spe, spa, spd;
        uint8_t type1;
        uint8_t type2;
        uint16_t ability1;
        uint16_t ability2;
        uint16_t abilityHidden;
        uint8_t genderRatio;
        uint8_t baseFriendship;
        uint8_t growthRate;
        uint8_t catchRate;
        uint8_t formCount;
        uint16_t formIndex;
    };

    inline constexpr uint8_t PERSONAL_TYPE_NONE = 255;
    extern const PersonalRecord PERSONAL_RECORD_EMPTY;
}

#endif
