#include <cstdint>
#include <cstddef>

#include "Pokemon/BaseStatsGen7.h"

// Forward declarations for Names namespace functions
namespace Names {
    extern const char* getSpeciesName(uint16_t speciesId);
    extern const char* getItemName(uint16_t itemId);
    extern const char* getNatureName(uint8_t natureId);
}

namespace Pokemon {
    const BaseStatsGen7* getBaseStatsGen7(uint16_t speciesId) {
        if (speciesId >= BASE_STATS_COUNT_GEN7) {
            static const BaseStatsGen7 empty = {0, 0, 0, 0, 0, 0};
            return &empty;
        }
        return &BASE_STATS_TABLE_GEN7[speciesId];
    }

    // Wrapper functions that forward to Names namespace
    const char* getSpeciesNameGen7(uint16_t speciesId) {
        return Names::getSpeciesName(speciesId);
    }

    const char* getItemNameGen7(uint16_t itemId) {
        return Names::getItemName(itemId);
    }

    const char* getNatureNameGen7(uint8_t natureId) {
        return Names::getNatureName(natureId);
    }
}
