/**
 * Trainer8SWSH.cpp - Generation 8 Trainer Implementation
 *
 * This file implements the Trainer8SWSH class for Pokemon Sword/Shield save files.
 * Handles Gen 8-specific block parsing, Pokemon encryption/decryption, and
 * save file serialization.
 */
#include <algorithm>
#include <cstring>

#include "Trainer/Trainer8SWSH.h"
#include "Trainer/Inventory8SWSH.h"
#include "Utils/Logger.h"

// ========================================
// Block Parsing Methods
// ========================================

void Trainer8SWSH::parseBlock(const Block& block)
{
    switch (block.key) {
        case BlockKeys8SWSH::MY_STATUS:
            parseMyStatusBlock(block);
            break;
        case BlockKeys8SWSH::PARTY:
            parsePartyBlock(block);
            break;
        case BlockKeys8SWSH::MISC:
            parseMiscBlock(block);
            break;
        case BlockKeys8SWSH::TRAINER_CARD:
            parseTrainerCardBlock(block);
            break;
        case BlockKeys8SWSH::ITEM:
            parseItemBlock(block);
            break;
        case BlockKeys8SWSH::BOX:
            parseBoxBlock(block);
            break;
        case BlockKeys8SWSH::BOX_LAYOUT:
            parseBoxLayoutBlock(block);
            break;
        // Additional blocks can be handled here
        default:
            // Unknown block - skip
            break;
    }
}

void Trainer8SWSH::parseMyStatusBlock(const Block& block)
{
    /**
     * MY_STATUS Block Structure:
     * 0xA0: ID32 (4 bytes) - Combined TID16 and SID16
     *
     * ID32 format: SID16 << 16 | TID16
     * Display TID: ID32 % 1000000
     * Display SID: ID32 / 1000000
     */
    if (block.data.size() < 0xA0 + 4) {
        logInfoToFile("Insufficient data for UInt32 at offset 0xA0 in MY_STATUS block");
        return;
    }

    this->ID32 = readUInt32LittleEndian(&block.data[0xA0]);
    this->TID16 = readUInt16LittleEndian(&block.data[0xA0]);
    this->SID16 = readUInt16LittleEndian(&block.data[0xA2]);
    this->TID = this->ID32 % 1000000;
    this->SID = this->ID32 / 1000000;
}

void Trainer8SWSH::parsePartyBlock(const Block& block)
{
    /**
     * PARTY Block Structure:
     * Pokemon stored sequentially at offsets:
     * - Slot 0: offset 0
     * - Slot 1: offset SIZE_PARTY8_SWSH (344 bytes)
     * - Slot 2: offset 2 * SIZE_PARTY8_SWSH
     * - ... up to 6 slots
     *
     * Each slot is SIZE_PARTY8_SWSH bytes (344 bytes for party Pokemon).
     * Empty slots are zeroed out.
     */
    const std::span<const std::byte> blockSpan(
        reinterpret_cast<const std::byte*>(block.data.data()),
        block.data.size()
    );

    for (size_t slot = 0; slot < MAX_PARTY_SLOTS; ++slot)
    {
        const size_t offset = slot * SIZE_PARTY8_SWSH;
        if (offset + SIZE_PARTY8_SWSH > block.data.size())
            break;

        std::span<const std::byte> slotSpan = blockSpan.subspan(offset, SIZE_PARTY8_SWSH);

        // Check if slot has valid Pokemon data (non-zero species)
        // The species ID is at offset 0x08 after decryption, but we can check
        // for an all-zero slot to skip empty slots
        bool isEmptySlot = true;
        for (size_t i = 0; i < SIZE_PARTY8_SWSH && i < slotSpan.size(); ++i) {
            if (slotSpan[i] != std::byte{0}) {
                isEmptySlot = false;
                break;
            }
        }

        if (!isEmptySlot) {
            // Decrypt and create Pokemon8SWSH object as unique_ptr
            // Pokemon8SWSH constructor handles decryption automatically
            party.push_back(std::make_unique<Pokemon8SWSH>(slotSpan));
        }
    }
}

void Trainer8SWSH::parseMiscBlock(const Block& block)
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

void Trainer8SWSH::parseTrainerCardBlock(const Block& block)
{
    /**
     * TRAINER_CARD Block Structure:
     * 0x00: Trainer Name (26 bytes, UTF-16LE)
     * 0x1C: Trainer ID (4 bytes) - Legacy trainer ID format
     */
    // Parse trainer ID
    this->trainerID = block.data.size() >= 0x1C + 4
        ? readInt32LittleEndian(block.data.data() + 0x1C)
        : 0;

    // Parse trainer name (UTF-16LE string)
    size_t nameLength = std::min(static_cast<size_t>(0x1A), block.data.size());
    this->trainerName = utf16ToUtf8(getString(block.data.data(), nameLength));
}

void Trainer8SWSH::parseItemBlock(const Block& block)
{
    /**
     * ITEM Block Structure:
     * Multiple "pouches" (categories) of items:
     * - Medicine
     * - Balls
     * - Battle Items
     * - Berries
     * - TMs/TRs
     * - Treasures
     * - Ingredients
     * - Key Items
     * - Other
     *
     * Each pouch has a fixed offset and maximum item count.
     * Items are stored as 4-byte values: (count << 16) | itemId
     */
    // Initialize items vector with pouches for each type
    items8SWSH.resize(static_cast<size_t>(PouchType8SWSH::Count));

    // Load each pouch
    for (int p = 0; p < static_cast<int>(PouchType8SWSH::Count); p++) {
        PouchType8SWSH pouchType = static_cast<PouchType8SWSH>(p);
        const PouchInfo8SWSH& info = getPouchInfo(pouchType);

        std::vector<InventoryItem8SWSH> pouch;
        pouch.reserve(info.maxCount);

        // Read items from block data
        for (int i = 0; i < info.maxCount; i++) {
            size_t offset = info.offset + (i * 4);
            if (offset + 4 <= block.data.size()) {
                uint32_t itemValue = readUInt32LittleEndian(&block.data[offset]);
                InventoryItem8SWSH item = InventoryItem8SWSH::fromValue(itemValue);

                // Only add items with valid IDs (non-zero)
                if (item.itemId != 0) {
                    pouch.push_back(item);
                }
            }
        }

        items8SWSH[p] = std::move(pouch);
    }
}

void Trainer8SWSH::parseBoxBlock(const Block& block)
{
    /**
     * BOX Block Structure:
     * Pokemon stored sequentially for all boxes and slots:
     * - Box 0, Slot 0: offset 0
     * - Box 0, Slot 1: offset SIZE_PARTY8_SWSH
     * - ... Box 0, Slot 29: offset 29 * SIZE_PARTY8_SWSH
     * - Box 1, Slot 0: offset 30 * SIZE_PARTY8_SWSH
     * - ... etc for all 32 boxes
     *
     * Total size: 32 boxes * 30 slots * 344 bytes = 331,776 bytes
     */
    const std::span<const std::byte> blockSpan(
        reinterpret_cast<const std::byte*>(block.data.data()),
        block.data.size()
    );

    for (size_t boxIndex = 0; boxIndex < BOX_COUNT8_SWSH; ++boxIndex) {
        for (size_t slot = 0; slot < BOX_SLOTS; ++slot) {
            // Calculate offset: (boxIndex * slots per box + slot) * bytes per pokemon
            const size_t offset = (boxIndex * BOX_SLOTS + slot) * SIZE_PARTY8_SWSH;
            if (offset + SIZE_PARTY8_SWSH > block.data.size()) {
                break;
            }

            std::span<const std::byte> slotSpan = blockSpan.subspan(offset, SIZE_PARTY8_SWSH);

            // Check if slot has a Pokemon (non-zero data)
            bool isEmptySlot = true;
            for (size_t i = 0; i < SIZE_PARTY8_SWSH && i < slotSpan.size(); ++i) {
                if (slotSpan[i] != std::byte{0}) {
                    isEmptySlot = false;
                    break;
                }
            }

            if (!isEmptySlot) {
                // Decrypt and create Pokemon8SWSH object
                boxes[boxIndex][slot] = std::make_unique<Pokemon8SWSH>(slotSpan);
            } else {
                // Empty slot
                boxes[boxIndex][slot] = nullptr;
            }
        }
    }
}

void Trainer8SWSH::parseBoxLayoutBlock(const Block& block)
{
    /**
     * BOX_LAYOUT Block Structure:
     * Box names stored sequentially:
     * - Box 0 name: offset 0 (34 bytes, UTF-16LE)
     * - Box 1 name: offset 34
     * - ... for all 32 boxes
     *
     * Each name is BOX_NAME_LENGTH8_SWSH bytes (34 bytes).
     */
    for (size_t boxIndex = 0; boxIndex < BOX_COUNT8_SWSH; ++boxIndex) {
        size_t offset = boxIndex * BOX_NAME_LENGTH8_SWSH;
        if (offset + BOX_NAME_LENGTH8_SWSH <= block.data.size()) {
            // Extract box name (UTF-16LE string)
            std::u16string boxNameU16 = getString(
                block.data.data() + offset,
                BOX_NAME_LENGTH8_SWSH
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

void Trainer8SWSH::updatePartyBlock()
{
    /**
     * Updates the PARTY block with modified Pokemon data.
     *
     * Process:
     * 1. Find the PARTY block
     * 2. Ensure block is large enough (6 slots * SIZE_PARTY8_SWSH)
     * 3. For each party Pokemon:
     *    a. Get encryption constant from Pokemon data
     *    b. Encrypt Pokemon data using encryptArray8
     *    c. Write encrypted data to block
     * 4. Zero out empty slots
     */
    for (auto& block : blocks) {
        if (block.key == BlockKeys8SWSH::PARTY) {
            // Ensure the block data is large enough
            size_t requiredSize = MAX_PARTY_SLOTS * SIZE_PARTY8_SWSH;
            if (block.data.size() < requiredSize) {
                block.data.resize(requiredSize, 0);
            }

            // Write each party Pokemon
            for (size_t i = 0; i < party.size() && i < MAX_PARTY_SLOTS; ++i) {
                const size_t offset = i * SIZE_PARTY8_SWSH;

                if (party[i] && party[i]->speciesID() != 0) {
                    // Pokemon exists - encrypt and write
                    const Pokemon* pokemon = party[i].get();
                    uint32_t ec = readUInt32LittleEndian(
                        reinterpret_cast<const uint8_t*>(pokemon->getData().data())
                    );

                    // Create span of decrypted Pokemon data
                    std::span<const std::byte> decryptedSpan(
                        pokemon->getData().data(),
                        pokemon->getDataSize()
                    );

                    // Encrypt the Pokemon data
                    std::byte* encryptedData = encryptArray8SWSH(decryptedSpan, ec);

                    // Write encrypted data to block
                    std::memcpy(&block.data[offset], encryptedData, pokemon->getDataSize());

                    // Clean up encrypted buffer
                    delete[] encryptedData;
                } else {
                    // Empty slot - write zeros
                    std::memset(&block.data[offset], 0, SIZE_PARTY8_SWSH);
                }
            }

            // Zero out any remaining slots
            for (size_t i = party.size(); i < MAX_PARTY_SLOTS; ++i) {
                const size_t offset = i * SIZE_PARTY8_SWSH;
                std::memset(&block.data[offset], 0, SIZE_PARTY8_SWSH);
            }

            break;
        }
    }
}

void Trainer8SWSH::updateBoxBlock()
{
    /**
     * Updates the BOX block with modified Pokemon data.
     *
     * Process similar to updatePartyBlock, but for all boxes:
     * 1. Find the BOX block
     * 2. Ensure block is large enough (32 boxes * 30 slots * SIZE_PARTY8_SWSH)
     * 3. For each box and slot:
     *    a. If Pokemon exists, encrypt and write
     *    b. If slot is empty, write zeros
     */
    for (auto& block : blocks) {
        if (block.key == BlockKeys8SWSH::BOX) {
            // Ensure the block data is large enough for all boxes
            size_t requiredSize = BOX_COUNT8_SWSH * BOX_SLOTS * SIZE_PARTY8_SWSH;
            if (block.data.size() < requiredSize) {
                block.data.resize(requiredSize, 0);
            }

            // Write each Pokemon back to the block
            for (size_t boxIndex = 0; boxIndex < BOX_COUNT8_SWSH; ++boxIndex) {
                for (size_t slot = 0; slot < BOX_SLOTS; ++slot) {
                    const size_t offset = (boxIndex * BOX_SLOTS + slot) * SIZE_PARTY8_SWSH;

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
                        std::byte* encryptedData = encryptArray8SWSH(decryptedSpan, ec);

                        // Write encrypted data to block
                        std::memcpy(&block.data[offset], encryptedData, pokemon->getDataSize());

                        // Clean up encrypted buffer
                        delete[] encryptedData;
                    } else {
                        // Empty slot - write zeros
                        std::memset(&block.data[offset], 0, SIZE_PARTY8_SWSH);
                    }
                }
            }
            break;
        }
    }
}

void Trainer8SWSH::updateItemBlock()
{
    /**
     * Updates the ITEM block with modified inventory data.
     *
     * Process:
     * 1. Find the ITEM block
     * 2. Ensure block is large enough for all pouches
     * 3. For each pouch:
     *    a. Write items to their designated offsets
     *    b. Zero out remaining slots
     */
    for (auto& block : blocks8SWSH) {
        if (block.key == BlockKeys8SWSH::ITEM) {
            // Ensure the block data is large enough
            size_t maxSize = 4856; // Sum of all pouch sizes * 4 bytes per item
            if (block.data.size() < maxSize) {
                block.data.resize(maxSize, 0);
            }

            // Write each pouch back to the block
            for (int p = 0; p < static_cast<int>(PouchType8SWSH::Count); p++) {
                PouchType8SWSH pouchType = static_cast<PouchType8SWSH>(p);
                const PouchInfo8SWSH& info = getPouchInfo(pouchType);
                const auto& pouch = items8SWSH[p];

                // Write items to block
                int itemIndex = 0;
                for (const auto& item : pouch) {
                    size_t offset = info.offset + (itemIndex * 4);
                    if (offset + 4 <= block.data.size()) {
                        uint32_t itemValue = item.toValue();
                        writeUInt32LittleEndian(&block.data[offset], itemValue);
                    }
                    itemIndex++;
                }

                // Zero out remaining slots in this pouch
                for (int i = itemIndex; i < info.maxCount; i++) {
                    size_t offset = info.offset + (i * 4);
                    if (offset + 4 <= block.data.size()) {
                        writeUInt32LittleEndian(&block.data[offset], 0);
                    }
                }
            }
            break;
        }
    }
}
