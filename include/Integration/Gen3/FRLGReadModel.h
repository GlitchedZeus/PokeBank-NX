#ifndef POKEBANK_INTEGRATION_GEN3_FRLG_READ_MODEL_H
#define POKEBANK_INTEGRATION_GEN3_FRLG_READ_MODEL_H

#include "Integration/Gen3/PKSMGen3Adapter.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace PokeVault::Integration::Gen3::Detail {
    struct FRLGReadModelResult {
        TrainerRecord trainer;
        std::vector<InventoryPouchRecord> inventory;
        // Inventory is secondary to structural save validation. FRLG keeps its accepted strict
        // behavior; RSE may report this false while the structurally valid save remains openable.
        bool inventoryAvailable = true;
        std::string inventoryError;
        SaveError error = SaveError::None;
    };

    [[nodiscard]] FRLGReadModelResult readFRLGModel(
        std::span<const uint8_t> source,
        const std::array<size_t, 14>& logicalSectorOffsets) noexcept;

    // Ruby/Sapphire/Emerald immutable trainer + inventory view. RSE pouch offsets/capacities and
    // Emerald quantity obfuscation are handled separately from FRLG; source bytes are never written.
    [[nodiscard]] FRLGReadModelResult readRSEModel(
        std::span<const uint8_t> source,
        const std::array<size_t, 14>& logicalSectorOffsets,
        bool emerald) noexcept;
}

#endif
