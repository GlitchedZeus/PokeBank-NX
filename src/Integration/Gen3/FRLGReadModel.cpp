#include "Integration/Gen3/FRLGReadModel.h"

#include "Trainer/Inventory3FRLG.h"
#include "Utils/Gen3Text.h"
#include "Utils/StringHelpers.h"

#include <algorithm>
#include <array>
#include <cstdio>
#include <string>

namespace PokeVault::Integration::Gen3::Detail {
    namespace {
        constexpr size_t kSectorDataSize = 0xF80;
        constexpr uint16_t kHighestGen3ItemId = 376;
        constexpr uint16_t kHighestLegalStackCount = 999;

        // Ruby/Sapphire and Emerald use the same four-byte Gen III item entry layout as FRLG
        // (little-endian u16 item id + little-endian u16 quantity), but their pouch offsets and
        // capacities are different. These offsets/capacities are the SavRS/SavE layouts from the
        // project's pinned PKSM-Core revision aa22d7a4f87c0351baf7da5962ba5acd01039a7c.
        // Ruby/Sapphire store bag quantities plainly. Emerald XOR-obfuscates bag quantities with
        // the low 16 bits of the security key at section 0 + 0xAC. PC item quantities are plaintext
        // in all three games.
        struct RSEPouchDefinition {
            InventoryPouch pouch;
            const char* name;
            size_t offset;
            uint16_t maxSlots;
            bool keyed;
        };

        constexpr std::array<RSEPouchDefinition, 6> kRSPouches{{
            {InventoryPouch::Items,      "Items",      0x0560, 20, false},
            {InventoryPouch::KeyItems,   "Key Items",  0x05B0, 20, false},
            {InventoryPouch::PokeBalls,  "Poké Balls", 0x0600, 16, false},
            {InventoryPouch::TMCase,     "TM/HM",      0x0640, 64, false},
            {InventoryPouch::BerryPouch, "Berries",    0x0740, 46, false},
            {InventoryPouch::PCItems,    "PC Items",   0x0498, 50, false},
        }};

        constexpr std::array<RSEPouchDefinition, 6> kEmeraldPouches{{
            {InventoryPouch::Items,      "Items",      0x0560, 30, true},
            {InventoryPouch::KeyItems,   "Key Items",  0x05D8, 30, true},
            {InventoryPouch::PokeBalls,  "Poké Balls", 0x0650, 16, true},
            {InventoryPouch::TMCase,     "TM/HM",      0x0690, 64, true},
            {InventoryPouch::BerryPouch, "Berries",    0x0790, 46, true},
            {InventoryPouch::PCItems,    "PC Items",   0x0498, 50, false},
        }};

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
                if (const char16_t character = Utils::gen3ToChar(value)) text.push_back(character);
            }
            while (!text.empty() && text.back() == u' ') text.pop_back();
            return Utils::utf16ToUtf8(text);
        }

        bool readTrainerIdentity(std::span<const uint8_t> source,
                                 const std::array<size_t, 14>& sectors,
                                 TrainerRecord& trainer) noexcept {
            std::array<uint8_t, 16> trainerBytes{};
            if (!readLogical(source, sectors, 0, 0, trainerBytes)) return false;
            trainer.name = decodeTrainerName(std::span<const uint8_t>(trainerBytes.data(), 7));
            trainer.gender = static_cast<uint8_t>(trainerBytes[8] & 1);
            trainer.tid16 = read16(trainerBytes, 0x0A);
            trainer.sid16 = read16(trainerBytes, 0x0C);
            trainer.id32 = read32(trainerBytes, 0x0A);
            return true;
        }

        InventoryPouch pouchType(size_t index) noexcept {
            constexpr std::array<InventoryPouch, 6> order = {
                InventoryPouch::Items, InventoryPouch::KeyItems, InventoryPouch::PokeBalls,
                InventoryPouch::TMCase, InventoryPouch::BerryPouch, InventoryPouch::PCItems,
            };
            return order[index];
        }

        void reportRSEInventoryFailure(bool emerald, const RSEPouchDefinition& definition,
                                       uint16_t slot, size_t logicalOffset, uint16_t itemId,
                                       uint16_t rawCount, uint16_t decodedCount, uint16_t key16,
                                       const char* reason) noexcept {
            // Ruby and Sapphire share the same byte-level save family, so this low-level reader
            // cannot distinguish the exact release. The UI/discovery layer logs the selected
            // source identity separately. Emerald is byte-distinct and can be named exactly here.
            const char* game = emerald ? "emerald_gba" : "ruby_or_sapphire_gba";
            std::fprintf(stderr,
                "RSE inventory validation failure: game=%s pouch=%s slot=%u offset=0x%zX "
                "item=%u raw_count=%u decoded_count=%u key16=0x%04X reason=%s\n",
                game, definition.name, static_cast<unsigned>(slot), logicalOffset,
                static_cast<unsigned>(itemId), static_cast<unsigned>(rawCount),
                static_cast<unsigned>(decodedCount), static_cast<unsigned>(key16), reason);
        }

        template <size_t N>
        bool readRSEInventory(std::span<const uint8_t> source,
                              const std::array<size_t, 14>& sectors,
                              const std::array<RSEPouchDefinition, N>& definitions,
                              uint16_t key16, bool emerald,
                              std::vector<InventoryPouchRecord>& inventory) noexcept {
            inventory.clear();
            inventory.reserve(N);
            for (const auto& definition : definitions) {
                InventoryPouchRecord pouch;
                pouch.pouch = definition.pouch;
                pouch.name = definition.name;
                pouch.capacity = definition.maxSlots;
                pouch.countEncrypted = definition.keyed;
                pouch.items.reserve(definition.maxSlots);

                for (uint16_t slot = 0; slot < definition.maxSlots; ++slot) {
                    const size_t logicalOffset = definition.offset + static_cast<size_t>(slot) * 4;
                    std::array<uint8_t, 4> entry{};
                    if (!readLogical(source, sectors, 1, logicalOffset, entry)) {
                        reportRSEInventoryFailure(emerald, definition, slot, logicalOffset,
                                                  0, 0, 0, key16, "logical read failed");
                        inventory.clear();
                        return false;
                    }
                    const uint16_t itemId = read16(entry, 0);
                    const uint16_t rawCount = read16(entry, 2);
                    uint16_t count = rawCount;
                    if (definition.keyed) count ^= key16;

                    // Empty item ids are empty slots regardless of the stored count word. This is
                    // especially important for keyed Emerald slots, whose stored quantity word can
                    // be nonzero even when the slot is empty.
                    if (itemId == 0) continue;

                    // PKHeX's Gen III pouch loader accepts every fixed-width slot first and treats
                    // count==0 entries as non-owned/clearable state. Real saves can therefore carry
                    // a stale nonzero item id in an unused slot. Ignore that stale slot instead of
                    // rejecting the inventory (and never reject the whole save for it).
                    if (count == 0) continue;

                    // Keep semantic validation for genuinely implausible non-empty entries, but an
                    // inventory-only failure is optional-model failure. readRSEModel() deliberately
                    // leaves the critical save result valid so trainer/party/boxes remain usable.
                    if (itemId > kHighestGen3ItemId) {
                        reportRSEInventoryFailure(emerald, definition, slot, logicalOffset,
                                                  itemId, rawCount, count, key16,
                                                  "item id exceeds Gen III range");
                        inventory.clear();
                        return false;
                    }
                    if (count > kHighestLegalStackCount) {
                        reportRSEInventoryFailure(emerald, definition, slot, logicalOffset,
                                                  itemId, rawCount, count, key16,
                                                  "decoded quantity exceeds supported stack range");
                        inventory.clear();
                        return false;
                    }
                    pouch.items.push_back({itemId, count});
                }
                inventory.push_back(std::move(pouch));
            }
            return true;
        }
    }

    FRLGReadModelResult readFRLGModel(
        std::span<const uint8_t> source,
        const std::array<size_t, 14>& logicalSectorOffsets) noexcept {
        FRLGReadModelResult result;
        if (!readTrainerIdentity(source, logicalSectorOffsets, result.trainer)) {
            result.error = SaveError::CoreRejected;
            return result;
        }

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

    FRLGReadModelResult readRSEModel(
        std::span<const uint8_t> source,
        const std::array<size_t, 14>& logicalSectorOffsets,
        bool emerald) noexcept {
        FRLGReadModelResult result;
        if (!readTrainerIdentity(source, logicalSectorOffsets, result.trainer)) {
            result.error = SaveError::CoreRejected;
            return result;
        }

        std::array<uint8_t, 4> moneyBytes{};
        if (!readLogical(source, logicalSectorOffsets, 1, 0x490, moneyBytes)) {
            result.error = SaveError::CoreRejected;
            return result;
        }
        uint32_t securityKey = 0;
        if (emerald) {
            std::array<uint8_t, 4> keyBytes{};
            if (!readLogical(source, logicalSectorOffsets, 0, 0xAC, keyBytes)) {
                result.error = SaveError::CoreRejected;
                return result;
            }
            securityKey = read32(keyBytes, 0);
        }
        result.trainer.money = read32(moneyBytes, 0) ^ securityKey;
        if (result.trainer.money > 999999) {
            result.error = SaveError::CoreRejected;
            return result;
        }

        const uint16_t key16 = static_cast<uint16_t>(securityKey);
        const bool inventoryOk = emerald
            ? readRSEInventory(source, logicalSectorOffsets, kEmeraldPouches, key16, true,
                               result.inventory)
            : readRSEInventory(source, logicalSectorOffsets, kRSPouches, 0, false,
                               result.inventory);
        if (!inventoryOk) {
            // Inventory is a secondary read model, not a save-integrity gate. Slot/sector/signature/
            // checksum/family/trainer validation has already succeeded before this point. Keep the
            // structurally valid save openable and signal inventory unavailability with an empty
            // inventory vector. A successfully decoded (even completely empty) bag has six pouches.
            result.inventory.clear();
        }
        return result;
    }
}
