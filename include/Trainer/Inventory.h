#ifndef TRAINER_INVENTORY_H
#define TRAINER_INVENTORY_H

#include <cstdint>

namespace Trainer {
    struct InventoryItem {
        uint16_t itemId = 0;
        uint16_t count = 0;
        bool isNew = false;
        bool isFavorite = false;
    };
}

#endif