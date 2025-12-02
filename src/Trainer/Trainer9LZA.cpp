/**
 * Trainer9LZA.cpp - Generation 9 Trainer Implementation
 *
 * This file implements the Trainer9LZA class for Pokemon Sword/Shield save files.
 * Handles Gen 9-specific block parsing, Pokemon encryption/decryption, and
 * save file serialization.
 */
#include <algorithm>
#include <cstring>

#include "Trainer/Trainer9LZA.h"
#include "Trainer/Inventory9LZA.h"
#include "Utils/Logger.h"

using namespace Utils;

namespace Trainer {
    // ========================================
    // Block Parsing Methods
    // ========================================

    void Trainer9LZA::parseBlock(const Block& block)
    {
        switch (block.key) {
            case MY_STATUS9_LZA:
                parseMyStatusBlock(block);
                break;
            case PARTY9_LZA:
                parsePartyBlock(block);
                break;
            case MISC9_LZA:
                parseMiscBlock(block);
                break;
            // case TRAINER_CARD:
            //     parseTrainerCardBlock(block);
            //     break;
            case ITEM9_LZA:
                parseItemBlock(block);
                break;
            case BOX9_LZA:
                parseBoxBlock(block);
                break;
            case BOX_LAYOUT9_LZA:
                parseBoxLayoutBlock(block);
                break;
            // Additional blocks can be handled here
            default:
                // Unknown block - skip
                break;
        }
    }

    void Trainer9LZA::parseMyStatusBlock(const Block& block)
    {
        /**
         * MY_STATUS Block Structure:
         * 0x00: ID32 (4 bytes) - Combined TID16 and SID16
         *
         * ID32 format: SID16 << 16 | TID16
         * Display TID: ID32 % 1000000
         * Display SID: ID32 / 1000000
         */
        if (block.data.size() < 0x00 + 4) {
            logInfoToFile("Insufficient data for UInt32 at offset 0x00 in MY_STATUS block");
            return;
        }

        this->ID32 = readUInt32LittleEndian(&block.data[0x00]);
        this->TID16 = readUInt16LittleEndian(&block.data[0x00]);
        this->SID16 = readUInt16LittleEndian(&block.data[0x02]);
        this->TID = this->ID32 % 1000000;
        this->SID = this->ID32 / 1000000;
        size_t nameLength = std::min(static_cast<size_t>(0x10), static_cast<size_t>(0x1A));
        auto nameSpan = std::span<const uint8_t>(block.data.data() + 0x10, nameLength);
        this->trainerName = utf16ToUtf8(getString(nameSpan.data(), nameLength));
        logInfoToFile("Parsed Trainer Name", this->trainerName.c_str());
    }

    void Trainer9LZA::parsePartyBlock(const Block& block)
    {
        /**
         * PARTY Block Structure (Pokemon Legends Z-A):
         * Pokemon stored with gaps between slots:
         * - Slot 0: offset 0 (SIZE_PARTY9_LZA bytes of data + GAP_BOX_SLOT9_LZA gap)
         * - Slot 1: offset PARTY_SLOT_SIZE9_LZA (480 bytes)
         * - Slot 2: offset 2 * PARTY_SLOT_SIZE9_LZA
         * - ... up to 6 slots
         *
         * Each slot spans PARTY_SLOT_SIZE9_LZA bytes (480 bytes), but only the first
         * SIZE_PARTY9_LZA bytes (344 bytes) contain Pokemon data. The remaining gap
         * (GAP_BOX_SLOT9_LZA = 0x88 bytes) is unused/padding.
         */
        const std::span<const std::byte> blockSpan(reinterpret_cast<const std::byte*>(block.data.data()), block.data.size());

        for (size_t slot = 0; slot < MAX_PARTY_SLOTS; ++slot)
        {
            // Calculate offset to this slot (includes gap from previous slots)
            const size_t offset = slot * PARTY_SLOT_SIZE9_LZA;
            if (offset + SIZE_PARTY9_LZA > block.data.size())
                break;

            // Extract only the Pokemon data portion (SIZE_PARTY9_LZA bytes)
            std::span<const std::byte> slotSpan = blockSpan.subspan(offset, SIZE_PARTY9_LZA);

            // Check if slot has valid Pokemon data (non-zero species)
            // The species ID is at offset 0x08 after decryption, but we can check
            // for an all-zero slot to skip empty slots
            bool isEmptySlot = true;
            for (size_t i = 0; i < SIZE_PARTY9_LZA && i < slotSpan.size(); ++i) {
                if (slotSpan[i] != std::byte{0}) {
                    isEmptySlot = false;
                    break;
                }
            }

            if (!isEmptySlot) {
                // Decrypt and create Pokemon9LZA object as unique_ptr
                // Pokemon9LZA constructor handles decryption automatically
                party.push_back(std::make_unique<Pokemon9LZA>(slotSpan));
            }
        }
    }

    void Trainer9LZA::parseMiscBlock(const Block& block)
    {
        /**
         * MISC Block Structure:
         * 0x04: Money (4 bytes) - Trainer's currency amount
         */
        if (block.data.size() < 0x04 + 4) {
            return;
        }

        this->money = readUInt32LittleEndian(&block.data[0x04]);
    }

    // void Trainer9LZA::parseTrainerCardBlock(const Block& block)
    // {
    //     /**
    //      * TRAINER_CARD Block Structure:
    //      * 0x00: Trainer Name (26 bytes, UTF-16LE)
    //      * 0x1C: Trainer ID (4 bytes) - Legacy trainer ID format
    //      */
    //     // Parse trainer ID
    //     this->trainerID = block.data.size() >= 0x1C + 4
    //         ? readInt32LittleEndian(block.data.data() + 0x1C)
    //         : 0;

    //     // Parse trainer name (UTF-16LE string)
    //     size_t nameLength = std::min(static_cast<size_t>(0x1A), block.data.size());
    //     this->trainerName = utf16ToUtf8(getString(block.data.data(), nameLength));
    // }

    void Trainer9LZA::parseItemBlock(const Block& block)
    {
        /**
         * ITEM Block Structure (Gen 9 Legends Z-A):
         * Items are stored BY ITEM ID as index!
         * - Item at ID X is at offset (X * 0x10)
         * - Block size: 0xBB80 bytes (47,872 bytes)
         * - Each item: 0x10 bytes (16 bytes)
         *
         * Structure per item (16 bytes):
         * 0x00-0x03: Pouch ID (uint32) - which pouch this belongs to
         * 0x04-0x07: Count (int32) - quantity
         * 0x08-0x0B: Flags (uint32) - isNew, isFavorite, etc.
         * 0x0C-0x0F: Padding
         */

        // Initialize items vector with pouches for each type
        items.resize(static_cast<size_t>(POUCH_COUNT9_LZA));

        // For each pouch, iterate through valid item IDs
        for (int i = 0; i < static_cast<int>(POUCH_COUNT9_LZA); i++) {
            PouchType9LZA pouchType = static_cast<PouchType9LZA>(i);
            items[i].clear();

            // Get the list of valid item IDs for this pouch
            const auto& validIds = getValidItemIds9LZA(pouchType);

            for (uint16_t itemId : validIds) {
                // Calculate offset: itemID * 0x10
                size_t offset = itemId * ITEM_SIZE9_LZA;

                // Make sure we're within bounds
                if (offset + ITEM_SIZE9_LZA > block.data.size()) {
                    continue;
                }

                // Read the item data at this index
                InventoryItem9LZA item = InventoryItem9LZA::fromBytes(itemId, &block.data[offset]);

                // Only add if count > 0
                if (item.count > 0) {
                    items[i].push_back(item);
                }

                // Add all items, even with count 0
                // items[p].push_back(item9.toInventoryItem());
            }
        }
    }

    void Trainer9LZA::parseBoxBlock(const Block& block)
    {
        /**
         * BOX Block Structure:
         * Pokemon stored with gaps between slots:
         * - Box 0, Slot 0: offset 0 (SIZE_PARTY9_LZA bytes of data + GAP_BOX_SLOT9_LZA gap)
         * - Box 0, Slot 1: offset BOX_SLOT_SIZE9_LZA (408 bytes)
         * - ... Box 0, Slot 29: offset 29 * BOX_SLOT_SIZE9_LZA
         * - Box 1, Slot 0: offset 30 * BOX_SLOT_SIZE9_LZA
         * - ... etc for all 32 boxes
         *
         * Each slot spans BOX_SLOT_SIZE9_LZA bytes (408 bytes), but only the first
         * SIZE_PARTY9_LZA bytes (344 bytes) contain Pokemon data. The remaining gap
         * (GAP_BOX_SLOT9_LZA = 0x40 bytes) is unused/padding.
         *
         * Total size: 32 boxes * 30 slots * 408 bytes = 391,680 bytes
         */
        const std::span<const std::byte> blockSpan(
            reinterpret_cast<const std::byte*>(block.data.data()),
            block.data.size()
        );

        for (size_t boxIndex = 0; boxIndex < BOX_COUNT9_LZA; ++boxIndex) {
            for (size_t slot = 0; slot < BOX_SLOTS; ++slot) {
                // Calculate offset: (boxIndex * slots per box + slot) * bytes per slot (including gap)
                const size_t offset = (boxIndex * BOX_SLOTS + slot) * BOX_SLOT_SIZE9_LZA;
                if (offset + SIZE_PARTY9_LZA > block.data.size()) {
                    break;
                }

                // Extract only the Pokemon data portion (SIZE_PARTY9_LZA bytes)
                std::span<const std::byte> slotSpan = blockSpan.subspan(offset, SIZE_PARTY9_LZA);

                // Check if slot has a Pokemon (non-zero data)
                bool isEmptySlot = true;
                for (size_t i = 0; i < SIZE_PARTY9_LZA && i < slotSpan.size(); ++i) {
                    if (slotSpan[i] != std::byte{0}) {
                        isEmptySlot = false;
                        break;
                    }
                }

                if (!isEmptySlot) {
                    // Decrypt and create Pokemon9LZA object
                    boxes[boxIndex][slot] = std::make_unique<Pokemon9LZA>(slotSpan);
                } else {
                    // Empty slot
                    boxes[boxIndex][slot] = nullptr;
                }
            }
        }
    }

    void Trainer9LZA::parseBoxLayoutBlock(const Block& block)
    {
        /**
         * BOX_LAYOUT Block Structure:
         * Box names stored sequentially:
         * - Box 0 name: offset 0 (34 bytes, UTF-16LE)
         * - Box 1 name: offset 34
         * - ... for all 32 boxes
         *
         * Each name is BOX_NAME_LENGTH_Gen9 bytes (34 bytes).
         */
        for (size_t boxIndex = 0; boxIndex < BOX_COUNT9_LZA; ++boxIndex) {
            size_t offset = boxIndex * BOX_NAME_LENGTH9_LZA;
            if (offset + BOX_NAME_LENGTH9_LZA <= block.data.size()) {
                // Extract box name (UTF-16LE string)
                std::u16string boxNameU16 = getString(
                    block.data.data() + offset,
                    BOX_NAME_LENGTH9_LZA
                );
                std::string boxName = utf16ToUtf8(boxNameU16);

                // If box name is empty, use default
                if (boxName.empty()) {
                    boxName = "Box " + std::to_string(boxIndex + 1);
                }

                boxNames[boxIndex] = boxName;
            } else {
                // Default name if data is insufficient
                boxNames[boxIndex] = "Box " + std::to_string(boxIndex + 1);
            }
        }
    }

    // ========================================
    // Block Update Methods
    // ========================================

    void Trainer9LZA::updatePartyBlock()
    {
        /**
         * Updates the PARTY block with modified Pokemon data.
         *
         * Process:
         * 1. Find the PARTY block
         * 2. Ensure block is large enough (6 slots * PARTY_SLOT_SIZE9_LZA)
         * 3. For each party Pokemon:
         *    a. Get encryption constant from Pokemon data
         *    b. Encrypt Pokemon data using encryptArray9
         *    c. Write encrypted data to block at correct offset (with gaps)
         * 4. Zero out empty slots
         */
        for (auto& block : blocks) {
            if (block.key == PARTY9_LZA) {
                // Ensure the block data is large enough (including gaps)
                size_t requiredSize = MAX_PARTY_SLOTS * PARTY_SLOT_SIZE9_LZA;
                if (block.data.size() < requiredSize) {
                    block.data.resize(requiredSize, 0);
                }

                // Write each party Pokemon
                for (size_t i = 0; i < party.size() && i < MAX_PARTY_SLOTS; ++i) {
                    // Calculate offset with gaps
                    const size_t offset = i * PARTY_SLOT_SIZE9_LZA;

                    if (party[i] && party[i]->speciesID() != 0) {
                        // Pokemon exists - encrypt and write
                        const Pokemon::Pokemon* pokemon = party[i].get();
                        uint32_t ec = readUInt32LittleEndian(
                            reinterpret_cast<const uint8_t*>(pokemon->getData().data())
                        );

                        // Create span of decrypted Pokemon data
                        std::span<const std::byte> decryptedSpan(
                            pokemon->getData().data(),
                            pokemon->getDataSize()
                        );

                        // Encrypt the Pokemon data
                        std::byte* encryptedData = encryptArray9LZA(decryptedSpan, ec);

                        // Write encrypted data to block (only SIZE_PARTY9_LZA bytes)
                        std::memcpy(&block.data[offset], encryptedData, pokemon->getDataSize());

                        // Zero out the gap after the Pokemon data
                        std::memset(&block.data[offset + SIZE_PARTY9_LZA], 0, GAP_BOX_SLOT9_LZA);

                        // Clean up encrypted buffer
                        delete[] encryptedData;
                    } else {
                        // Empty slot - write zeros for entire slot (data + gap)
                        std::memset(&block.data[offset], 0, PARTY_SLOT_SIZE9_LZA);
                    }
                }

                // Zero out any remaining slots
                for (size_t i = party.size(); i < MAX_PARTY_SLOTS; ++i) {
                    const size_t offset = i * PARTY_SLOT_SIZE9_LZA;
                    std::memset(&block.data[offset], 0, PARTY_SLOT_SIZE9_LZA);
                }

                break;
            }
        }
    }

    void Trainer9LZA::updateBoxBlock()
    {
        /**
         * Updates the BOX block with modified Pokemon data.
         *
         * Process similar to updatePartyBlock, but for all boxes:
         * 1. Find the BOX block
         * 2. Ensure block is large enough (32 boxes * 30 slots * BOX_SLOT_SIZE9_LZA)
         * 3. For each box and slot:
         *    a. If Pokemon exists, encrypt and write
         *    b. If slot is empty, write zeros
         */
        for (auto& block : blocks) {
            if (block.key == BOX9_LZA) {
                // Ensure the block data is large enough for all boxes (including gaps)
                size_t requiredSize = BOX_COUNT9_LZA * BOX_SLOTS * BOX_SLOT_SIZE9_LZA;
                if (block.data.size() < requiredSize) {
                    block.data.resize(requiredSize, 0);
                }

                // Write each Pokemon back to the block
                for (size_t boxIndex = 0; boxIndex < BOX_COUNT9_LZA; ++boxIndex) {
                    for (size_t slot = 0; slot < BOX_SLOTS; ++slot) {
                        // Calculate offset with gaps
                        const size_t offset = (boxIndex * BOX_SLOTS + slot) * BOX_SLOT_SIZE9_LZA;

                        if (boxes[boxIndex][slot]) {
                            // Pokemon exists - encrypt and write
                            const auto& pokemon = boxes[boxIndex][slot];

                            // Get the Encryption Constant (used as seed for encryption)
                            uint32_t ec = readUInt32LittleEndian(
                                reinterpret_cast<const uint8_t*>(pokemon->getData().data())
                            );

                            // Create span of decrypted Pokemon data
                            std::span<const std::byte> decryptedSpan(
                                pokemon->getData().data(),
                                pokemon->getDataSize()
                            );

                            // Encrypt the Pokemon data
                            std::byte* encryptedData = encryptArray9LZA(decryptedSpan, ec);

                            // Write encrypted data to block (only SIZE_PARTY9_LZA bytes)
                            std::memcpy(&block.data[offset], encryptedData, pokemon->getDataSize());

                            // Zero out the gap after the Pokemon data
                            std::memset(&block.data[offset + SIZE_PARTY9_LZA], 0, GAP_BOX_SLOT9_LZA);

                            // Clean up encrypted buffer
                            delete[] encryptedData;
                        } else {
                            // Empty slot - write zeros for entire slot (data + gap)
                            std::memset(&block.data[offset], 0, BOX_SLOT_SIZE9_LZA);
                        }
                    }
                }
                break;
            }
        }
    }

    void Trainer9LZA::updateItemBlock()
    {
        /**
         * Updates the ITEM block with modified inventory data.
         *
         * Process:
         * 1. Find the ITEM block
         * 2. Ensure block is large enough (0xBB80 bytes)
         * 3. Write items to their indexed positions (itemID * 0x10)
         *
         * Gen 9 stores items BY ITEM ID as index
         * Block size: 0xBB80 bytes (47,872 bytes)
         * Item size: 0x10 bytes (16 bytes)
         */

        for (auto& block : blocks) {
            if (block.key == ITEM9_LZA) {
                // Ensure the block data is large enough
                if (block.data.size() < ITEM_BLOCK_SIZE9_LZA) {
                    block.data.resize(ITEM_BLOCK_SIZE9_LZA, 0);
                }

                // First, zero out the entire block
                std::memset(block.data.data(), 0, ITEM_BLOCK_SIZE9_LZA);

                // Write each pouch's items to their indexed positions
                for (int i = 0; i < static_cast<int>(POUCH_COUNT9_LZA); i++) {
                    PouchType9LZA pouchType = static_cast<PouchType9LZA>(i);
                    const auto& pouch = items[i];

                    // Write all items from this pouch
                    for (const auto& item : pouch) {
                        uint16_t itemId = item.itemId;

                        // Calculate offset: itemID * 0x10
                        size_t offset = itemId * ITEM_SIZE9_LZA;

                        // Make sure we're within bounds
                        if (offset + ITEM_SIZE9_LZA > block.data.size()) {
                            continue;
                        }

                        // Create InventoryItem9 with pouch ID
                        InventoryItem9LZA item9;
                        item9.pouchId = static_cast<uint32_t>(pouchType);
                        item9.itemId = itemId;
                        item9.count = static_cast<int32_t>(item.count);
                        item9.flags = 0;
                        if (item.isNew) item9.flags |= 0x1;
                        if (item.isFavorite) item9.flags |= 0x2;

                        // Write to block at indexed position
                        item9.toBytes(&block.data[offset]);
                    }
                }

                break;
            }
        }
    }
}
