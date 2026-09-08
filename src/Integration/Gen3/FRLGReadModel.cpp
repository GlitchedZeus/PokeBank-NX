#include "Integration/Gen3/FRLGReadModel.h"

#include "Trainer/Inventory3FRLG.h"
#include "Utils/Gen3Text.h"
#include "Utils/StringHelpers.h"

#include <algorithm>
#include <array>
#include <string>

namespace PokeVault::Integration::Gen3::Detail {
    namespace {
        constexpr size_t kSectorDataSize = 0xF80;
        constexpr uint16_t kHighestGen3ItemId = 376;
        constexpr uint16_t kHighestLegalStackCount = 999;

        uint16_t read16(std::span<const uint8_t> bytes, size_t offset) noexcept {
            return static_cast<uint16_t>(bytes[offset]) |
                   static_cast<uint16_t>(bytes[offset + 1] << 8);
        }

        uint32_t read32(std::span<const uint8_t> bytes, size_t offset) noexcept {
            return static_cast<uint32_t>(read16(bytes, offset)) |
                   (static_cast<uint32_t>(read16(bytes, offset + 2)) << 16);
        }

        bool readLogical(std::span<const uint8_t> source,
                         const std::array<size_t, 14>& sectors,
                         uint8_t firstSector, size_t logical,
                         std::span<uint8_t> output) noexcept {
            for (size_t index = 0; index < output.size(); ++index) {
                const size_t position = logical + index;
                const size_t sectorId = static_cast<size_t>(firstSector) +
                                        position / kSectorDataSize;
                if (sectorId >= sectors.size()) return false;
                const size_t absolute = sectors[sectorId] + position % kSectorDataSize;
                if (absolute >= source.size()) return false;
                output[index] = source[absolute];
            }
            return true;
        }

        std::string decodeTrainerName(std::span<const uint8_t> bytes) {
            std::u16string text;
            for (uint8_t value : bytes) {
                if (value == Utils::GEN3_TERMINATOR) break;
                if (const char16_t character = Utils::gen3ToChar(value))
                    text.push_back(character);
            }
            while (!text.empty() && text.back() == u' ') text.pop_back();
            return Utils::utf16ToUtf8(text);
        }

        InventoryPouch pouchType(size_t index) noexcept {
            constexpr std::array<InventoryPouch, 6> order = {
                InventoryPouch::Items,
                InventoryPouch::KeyItems,
                InventoryPouch::PokeBalls,
                InventoryPouch::TMCase,
                InventoryPouch::BerryPouch,
                InventoryPouch::PCItems,
            };
            return order[index];
        }
    }

    FRLGReadModelResult readFRLGModel(
        std::span<const uint8_t> source,
        const std::array<size_t, 14>& logicalSectorOffsets) noexcept {
        FRLGReadModelResult result;

        std::array<uint8_t, 16> trainerBytes{};
        if (!readLogical(source, logicalSectorOffsets, 0, 0, trainerBytes)) {
            result.error = SaveError::CoreRejected;
            return result;
        }
        result.trainer.name = decodeTrainerName(
            std::span<const uint8_t>(trainerBytes.data(), 7));
        result.trainer.gender = static_cast<uint8_t>(trainerBytes[8] & 1);
        result.trainer.tid16 = read16(trainerBytes, 0x0A);
        result.trainer.sid16 = read16(trainerBytes, 0x0C);
        result.trainer.id32 = read32(trainerBytes, 0x0A);

        std::array<uint8_t, 4> keyBytes{};
        std::array<uint8_t, 4> moneyBytes{};
        if (!readLogical(source, logicalSectorOffsets, 0, 0xF20, keyBytes) ||
            !readLogical(source, logicalSectorOffsets, 1, 0x290, moneyBytes)) {
            result.error = SaveError::CoreRejected;
            return result;
        }
        const uint32_t securityKey = read32(keyBytes, 0);
        result.trainer.money = read32(moneyBytes, 0) ^ securityKey;
        if (result.trainer.money > 999999) {
            result.error = SaveError::CoreRejected;
            return result;
        }

        result.inventory.reserve(Trainer::POUCH_COUNT3_FRLG);
        const uint16_t key16 = static_cast<uint16_t>(securityKey);
        for (size_t pouchIndex = 0; pouchIndex < Trainer::POUCH_COUNT3_FRLG; ++pouchIndex) {
            const auto& definition = Trainer::getPouchInfo3FRLG(
                static_cast<Trainer::PouchType3FRLG>(pouchIndex));
            InventoryPouchRecord pouch;
            pouch.pouch = pouchType(pouchIndex);
            pouch.name = definition.name;
            pouch.capacity = static_cast<uint16_t>(definition.maxSlots);
            pouch.countEncrypted = definition.keyed;
            pouch.items.reserve(static_cast<size_t>(definition.maxSlots));

            for (int slot = 0; slot < definition.maxSlots; ++slot) {
                std::array<uint8_t, 4> entry{};
                if (!readLogical(source, logicalSectorOffsets, 1,
                        static_cast<size_t>(definition.offset + slot * 4), entry)) {
                    result.error = SaveError::InvalidInventory;
                    result.inventory.clear();
                    return result;
                }
                const uint16_t itemId = read16(entry, 0);
                uint16_t count = read16(entry, 2);
                if (definition.keyed) count ^= key16;
                // Empty slots are ignored even when their stored keyed count is non-zero.
                if (itemId == 0 || count == 0) continue;
                if (itemId > kHighestGen3ItemId || count > kHighestLegalStackCount) {
                    result.error = SaveError::InvalidInventory;
                    result.inventory.clear();
                    return result;
                }
                pouch.items.push_back({itemId, count});
            }
            result.inventory.push_back(std::move(pouch));
        }
        return result;
    }
}
