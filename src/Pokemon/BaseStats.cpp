#include <cstdint>
#include <cstddef>

#include "Pokemon/BaseStats.h"

// Forward declarations for Names namespace functions
namespace Names {
    extern const char* getSpeciesName(uint16_t speciesId);
    extern const char* getItemName(uint16_t itemId);
    extern const char* getNatureName(uint8_t natureId);
}

namespace Pokemon {
    const BaseStats* getBaseStats(uint16_t speciesId) {
        if (speciesId >= BASE_STATS_COUNT) {
            static const BaseStats empty = {0, 0, 0, 0, 0, 0};
            return &empty;
        }
        return &BASE_STATS_TABLE[speciesId];
    }

    // Wrapper functions that forward to Names namespace
    const char* getSpeciesName(uint16_t speciesId) {
        return Names::getSpeciesName(speciesId);
    }

    const char* getItemName(uint16_t itemId) {
        return Names::getItemName(itemId);
    }

    const char* getNatureName(uint8_t natureId) {
        return Names::getNatureName(natureId);
    }
}
