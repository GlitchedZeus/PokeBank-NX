#ifndef TRAINER_INVENTORY_H
#define TRAINER_INVENTORY_H

#include <cstdint>

namespace Trainer {
    struct InventoryItem {
        uint16_t itemId;
        uint16_t count;
        bool isNew;
        bool isFavorite;
    };
}

#endif