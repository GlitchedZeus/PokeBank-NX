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
    /**
     * Helper function to search for a species in a form-specific array.
     * Returns nullptr if not found.
     */
    static const BaseStatsGen7* searchFormArray(
        const BaseStatsGen7* array,
        size_t arraySize,
        uint16_t speciesId)
    {
        for (size_t i = 0; i < arraySize; i++) {
            if (array[i].id == speciesId) {
                return &array[i];
            }
        }
        return nullptr;
    }

    const BaseStatsGen7* getBaseStatsGen7(uint16_t speciesId, uint8_t form) {
        static const BaseStatsGen7 empty = {0, 0, 0, 0, 0, 0, 0};

        // Gen 7 only has Alolan forms for regional variants (form 1)
        if (form == 1) {
            switch (speciesId) {
                // Alolan forms (Gen 7)
                case 19: case 20: case 26: case 27: case 28: case 37: case 38:
                case 50: case 51: case 52: case 53: case 74: case 75: case 76:
                case 88: case 89: case 103: case 105: {
                    const BaseStatsGen7* stats = searchFormArray(
                        BASE_STATS_TABLE_ALOLAN_GEN7,
                        sizeof(BASE_STATS_TABLE_ALOLAN_GEN7) / sizeof(BASE_STATS_TABLE_ALOLAN_GEN7[0]),
                        speciesId
                    );
                    if (stats) return stats;
                    break;
                }
            }
        }

        // The table is dense only for Kanto 0..151. Meltan/Melmetal are sparse rows
        // appended after Mew, so BASE_STATS_COUNT_GEN7 is not a valid dex-id ceiling.
        if (speciesId <= 151) {
            return &BASE_STATS_TABLE_GEN7[speciesId];
        }
        if (form == 0) {
            const BaseStatsGen7* sparse = searchFormArray(
                BASE_STATS_TABLE_GEN7, BASE_STATS_COUNT_GEN7, speciesId);
            if (sparse) return sparse;
        }
        return &empty;
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
