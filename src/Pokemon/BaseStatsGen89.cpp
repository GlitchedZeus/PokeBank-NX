#include <cstdint>
#include <cstddef>

#include "Pokemon/BaseStatsGen89.h"

// Forward declarations for Names namespace functions
namespace Names {
    extern const char* getSpeciesName(uint16_t speciesId);
    extern const char* getItemName(uint16_t itemId);
    extern const char* getNatureName(uint8_t natureId);
    extern const char* getAbilityName(uint8_t abilityId);
}

namespace Pokemon {
    const BaseStatsGen89* getBaseStatsGen89(uint16_t speciesId) {
        if (speciesId >= BASE_STATS_COUNT_GEN89) {
            static const BaseStatsGen89 empty = {0, 0, 0, 0, 0, 0};
            return &empty;
        }
        return &BASE_STATS_TABLE_GEN89[speciesId];
    }

    // Wrapper functions that forward to Names namespace
    const char* getSpeciesNameGen89(uint16_t speciesId) {
        return Names::getSpeciesName(speciesId);
    }

    const char* getItemNameGen89(uint16_t itemId) {
        return Names::getItemName(itemId);
    }

    const char* getNatureNameGen89(uint8_t natureId) {
        return Names::getNatureName(natureId);
    }

    const char* getAbilityNameGen89(uint8_t abilityId) {
        return Names::getAbilityName(abilityId);
    }
}
