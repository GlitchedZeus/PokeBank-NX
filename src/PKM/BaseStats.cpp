#include <cstdint>
#include <cstddef>

#include "PKM/BaseStats.h"

const BaseStats* getBaseStats(uint16_t speciesId) {
    if (speciesId >= BASE_STATS_COUNT) {
        static const BaseStats empty = {0, 0, 0, 0, 0, 0};
        return &empty;
    }
    return &BASE_STATS_TABLE[speciesId];
}
