#ifndef POKEBANK_INTEGRATION_GEN3_FRLG_READ_MODEL_H
#define POKEBANK_INTEGRATION_GEN3_FRLG_READ_MODEL_H

#include "Integration/Gen3/PKSMGen3Adapter.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace PokeVault::Integration::Gen3::Detail {
    struct FRLGReadModelResult {
        TrainerRecord trainer;
        std::vector<InventoryPouchRecord> inventory;
        SaveError error = SaveError::None;
    };

    [[nodiscard]] FRLGReadModelResult readFRLGModel(
        std::span<const uint8_t> source,
        const std::array<size_t, 14>& logicalSectorOffsets) noexcept;

    // Ruby/Sapphire/Emerald trainer-only immutable view. RSE inventory is intentionally not
    // exposed during this read-only milestone; callers receive an empty inventory vector.
    [[nodiscard]] FRLGReadModelResult readRSEModel(
        std::span<const uint8_t> source,
        const std::array<size_t, 14>& logicalSectorOffsets,
        bool emerald) noexcept;
}

#endif
