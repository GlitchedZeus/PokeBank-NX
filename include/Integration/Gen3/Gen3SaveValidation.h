#ifndef POKEBANK_INTEGRATION_GEN3_GEN3_SAVE_VALIDATION_H
#define POKEBANK_INTEGRATION_GEN3_GEN3_SAVE_VALIDATION_H

#include "Integration/Gen3/PKSMGen3Adapter.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>

namespace PokeVault::Integration::Gen3::Detail {
    inline constexpr size_t kSaveSize = 0x20000;
    inline constexpr size_t kSectorSize = 0x1000;
    inline constexpr size_t kSectorDataSize = 0xF80;
    inline constexpr size_t kSectorCount = 14;
    inline constexpr size_t kSlotBases[2] = {0x0000, 0xE000};
    inline constexpr uint32_t kSectorSignature = 0x08012025;
    inline constexpr std::array<size_t, kSectorCount> kChunkLengths = {
        0xF2C, 0xF80, 0xF80, 0xF80, 0xF08, 0xF80, 0xF80,
        0xF80, 0xF80, 0xF80, 0xF80, 0xF80, 0xF80, 0x7D0,
    };

    inline uint16_t read16(std::span<const uint8_t> bytes, size_t offset) noexcept {
        return static_cast<uint16_t>(bytes[offset]) |
               static_cast<uint16_t>(bytes[offset + 1] << 8);
    }

    inline uint32_t read32(std::span<const uint8_t> bytes, size_t offset) noexcept {
        return static_cast<uint32_t>(read16(bytes, offset)) |
               (static_cast<uint32_t>(read16(bytes, offset + 2)) << 16);
    }

    inline uint64_t read64(std::span<const uint8_t> bytes, size_t offset) noexcept {
        return static_cast<uint64_t>(read32(bytes, offset)) |
               (static_cast<uint64_t>(read32(bytes, offset + 4)) << 32);
    }

    inline uint16_t sectorChecksum(std::span<const uint8_t> data) noexcept {
        uint32_t sum = 0;
        for (size_t offset = 0; offset < data.size(); offset += 4) {
            uint32_t word = data[offset];
            if (offset + 1 < data.size()) word |= static_cast<uint32_t>(data[offset + 1]) << 8;
            if (offset + 2 < data.size()) word |= static_cast<uint32_t>(data[offset + 2]) << 16;
            if (offset + 3 < data.size()) word |= static_cast<uint32_t>(data[offset + 3]) << 24;
            sum += word;
        }
        return static_cast<uint16_t>((sum & 0xFFFFu) + (sum >> 16));
    }

    struct SlotValidation {
        bool valid = false;
        SaveError error = SaveError::None;
        uint32_t counter = 0;
        std::array<size_t, kSectorCount> logicalSectorOffsets{};
    };

    inline SlotValidation validateSlot(std::span<const uint8_t> bytes, uint8_t slot) noexcept {
        SlotValidation result;
        result.logicalSectorOffsets.fill(std::numeric_limits<size_t>::max());
        bool counterSet = false;
        const size_t slotBase = kSlotBases[slot];

        for (size_t physical = 0; physical < kSectorCount; ++physical) {
            const size_t offset = slotBase + physical * kSectorSize;
            const uint16_t id = read16(bytes, offset + 0xFF4);
            if (id >= kSectorCount) {
                result.error = SaveError::InvalidSectorId;
                return result;
            }
            if (result.logicalSectorOffsets[id] != std::numeric_limits<size_t>::max()) {
                result.error = SaveError::DuplicateSector;
                return result;
            }
            if (read32(bytes, offset + 0xFF8) != kSectorSignature) {
                result.error = SaveError::BadSectorSignature;
                return result;
            }
            const uint32_t counter = read32(bytes, offset + 0xFFC);
            if (!counterSet) {
                result.counter = counter;
                counterSet = true;
            } else if (counter != result.counter) {
                result.error = SaveError::MismatchedSectorCounters;
                return result;
            }
            if (read16(bytes, offset + 0xFF6) !=
                sectorChecksum(bytes.subspan(offset, kChunkLengths[id]))) {
                result.error = SaveError::BadSectorChecksum;
                return result;
            }
            result.logicalSectorOffsets[id] = offset;
        }

        if (std::ranges::find(result.logicalSectorOffsets,
                std::numeric_limits<size_t>::max()) != result.logicalSectorOffsets.end()) {
            result.error = SaveError::MissingSector;
            return result;
        }
        result.valid = true;
        return result;
    }

    inline bool counterIsNewer(uint32_t lhs, uint32_t rhs) noexcept {
        return static_cast<int32_t>(lhs - rhs) > 0;
    }

    inline uint8_t selectActiveSlot(const SlotValidation (&slots)[2]) noexcept {
        if (!slots[0].valid) return 1;
        if (!slots[1].valid) return 0;
        return counterIsNewer(slots[1].counter, slots[0].counter) ? 1 : 0;
    }

    enum class SaveFamily : uint8_t { RubySapphire, Emerald, FireRedLeafGreen };

    inline SaveFamily detectFamily(std::span<const uint8_t> bytes,
                                   const SlotValidation& selected) noexcept {
        const size_t block0 = selected.logicalSectorOffsets[0];
        const uint32_t marker = read32(bytes, block0 + 0xAC);
        if (marker == 1) return SaveFamily::FireRedLeafGreen;
        if (marker != 0) return SaveFamily::Emerald;
        // A zero Emerald security key is possible in principle. Preserve the same secondary
        // discriminator used by pinned PKSM-Core for data that reaches Emerald-only block-0 space.
        if (read64(bytes, block0 + 0xEE0) != 0 || read64(bytes, block0 + 0xEE8) != 0)
            return SaveFamily::Emerald;
        return SaveFamily::RubySapphire;
    }

    inline bool sourceGameSupported(SourceGame game) noexcept {
        switch (game) {
            case SourceGame::FireRedGBA:
            case SourceGame::LeafGreenGBA:
            case SourceGame::RubyGBA:
            case SourceGame::SapphireGBA:
            case SourceGame::EmeraldGBA:
                return true;
        }
        return false;
    }

    inline bool isFRLG(SourceGame game) noexcept {
        return game == SourceGame::FireRedGBA || game == SourceGame::LeafGreenGBA;
    }

    inline bool isRSE(SourceGame game) noexcept {
        return game == SourceGame::RubyGBA || game == SourceGame::SapphireGBA ||
               game == SourceGame::EmeraldGBA;
    }

    inline bool familyMatches(SourceGame game, SaveFamily family) noexcept {
        if (isFRLG(game)) return family == SaveFamily::FireRedLeafGreen;
        if (game == SourceGame::RubyGBA || game == SourceGame::SapphireGBA)
            return family == SaveFamily::RubySapphire;
        if (game == SourceGame::EmeraldGBA) return family == SaveFamily::Emerald;
        return false;
    }

    inline size_t partyCountOffset(SourceGame game) noexcept {
        return isFRLG(game) ? 0x34 : 0x234;
    }

    inline size_t partyDataOffset(SourceGame game) noexcept {
        return isFRLG(game) ? 0x38 : 0x238;
    }
}

#endif
