/**
 * Trainer9.h - Generation 9 Trainer/Save File Data Management
 *
 * This file defines the Trainer9 class for Pokemon Generation 9 games:
 * - Pokemon Scarlet/Violet (SV)
 * - Pokemon Legends: Z-A (ZA)
 *
 * Trainer9 implements generation-specific logic for:
 * - PK9 Pokemon storage (party and boxes)
 * - Gen 9 block keys
 * - Gen 9 encryption (encryptArray9/decryptArray9)
 * - Gen 9-specific save file structure
 */

#ifndef TRAINER_TRAINER9_H
#define TRAINER_TRAINER9_H

#include <cstring>

#include "Trainer/Trainer.h"
#include "PKM/PK9.h"
#include "Encryption/Gen9Encryption.h"

// ========================================
// Generation 9 Save File Block Keys
// ========================================

namespace Gen9BlockKeys {
    constexpr size_t MY_STATUS = 0xE3E89BD1;        // Trainer Details
    constexpr size_t PARTY = 0x3AA1A9AD;            // Party Data
    constexpr size_t MISC = 0x4F35D0DD;             // Money
    // constexpr size_t TRAINER_CARD = 0x874da6fa;     // Trainer Card
    constexpr size_t PLAY_TIME = 0xEDAFF794;        // Time Played
    constexpr size_t ITEM = 0x21C9BD44;             // Items
    constexpr size_t BOX = 0x0d66012c;              // Box Data
    constexpr size_t BOX_LAYOUT = 0x19722c89;       // Box Names
    // constexpr size_t BOX_WALLPAPERS = 0x2EB1B190;   // Box Wallpapers
}

// Generation 9 constants
constexpr size_t BOX_COUNT_GEN9 = 32;       // Number of boxes in Legends: Z-A
constexpr size_t BOX_NAME_LENGTH_GEN9 = 0x22; // 34 bytes per box name (UTF-16LE)

/**
 * Trainer9 - Generation 9 Trainer Class
 *
 * Inherits from Trainer base class and implements Gen 9-specific save file format.
 * Handles automatic decryption on construction and provides Gen 9-specific
 * encryption when updating blocks.
 *
 * Gen 9 Specific Features:
 * - 32 boxes with 30 slots each
 * - PK9 Pokemon format (344 bytes party, 328 bytes stored)
 * - Block-based save file structure
 * - Gen 9 encryption algorithm
 *
 * Save File Structure (Legends: Z-A):
 * - File: "main" (approximately 2.94MB)
 * - Format: Multiple blocks identified by key values
 * - Encryption: Block-level encryption + Pokemon encryption
 */
class Trainer9 final : public Trainer
{
public:
    // ========================================
    // Constructor
    // ========================================

    /**
     * Constructs a Trainer9 object from save file blocks.
     *
     * Process:
     * 1. Parses blocks to extract trainer info
     * 2. Decrypts and loads party Pokemon (PK9)
     * 3. Decrypts and loads box Pokemon (PK9)
     * 4. Loads items and box names
     *
     * @param blocks Save file blocks parsed from Gen 9 save file
     */
    explicit Trainer9(std::vector<Block> blocks) : Trainer(std::move(blocks))
    {
        party.reserve(MAX_PARTY_SLOTS);
        boxes.resize(BOX_COUNT_GEN9);
        boxNames.resize(BOX_COUNT_GEN9);

        // Parse all blocks to extract data
        for (const auto& block : this->blocks) {
            parseBlock(block);
        }
    }

    /// Destructor - cleanup handled by base class and unique_ptrs
    ~Trainer9() override = default;

    // Delete copy operations
    Trainer9(const Trainer9&) = delete;
    Trainer9& operator=(const Trainer9&) = delete;

    // Allow move operations
    Trainer9(Trainer9&&) noexcept = default;
    Trainer9& operator=(Trainer9&&) noexcept = default;

    // ========================================
    // Implementation of Pure Virtual Methods
    // ========================================

    /**
     * Updates the PARTY_KEY block with modified Pokemon data.
     * Uses Gen 9 encryption (encryptArray9).
     */
    void updatePartyBlock() override;

    /**
     * Updates the BOX_KEY block with modified Pokemon data.
     * Uses Gen 9 encryption (encryptArray9).
     */
    void updateBoxBlock() override;

    /**
     * Updates the ITEM_KEY block with modified inventory data.
     */
    void updateItemBlock() override;

    /**
     * Gets the number of boxes available in Gen 9.
     * @return 32 (Legends: Z-A has 32 boxes)
     */
    size_t getBoxCount() const noexcept override {
        return BOX_COUNT_GEN9;
    }

    /**
     * Gets the number of Pokemon currently in the party.
     * @return Party size (0-6)
     */
    size_t getPartySize() const noexcept override {
        return party.size();
    }

    /**
     * Gets the game group for Gen 9 trainers.
     * @return GameVersion::SWSH (Legends: Z-A group)
     */
    GameVersion getGameGroup() const noexcept override {
        return GameVersion::ZA;
    }

private:
    /**
     * Parses a single block to extract relevant data.
     * Called during construction for each block in the save file.
     *
     * @param block The block to parse
     */
    void parseBlock(const Block& block);

    /**
     * Parses the MY_STATUS block to extract trainer ID.
     * Location: ID32 at offset 0xA0 (4 bytes)
     */
    void parseMyStatusBlock(const Block& block);

    /**
     * Parses the PARTY block to extract party Pokemon.
     * Format: 6 slots of SIZE_8PARTY (344 bytes each)
     */
    void parsePartyBlock(const Block& block);

    /**
     * Parses the MISC block to extract money.
     * Location: Money at offset 0x04 (4 bytes)
     */
    void parseMiscBlock(const Block& block);

    // THERE'S NO TRAINER CARD PARSING IN GEN 9
    // /**
    //  * Parses the TRAINER_CARD block to extract trainer name.
    //  * Location: Name at offset 0x00 (26 bytes, UTF-16LE)
    //  * Location: Trainer ID at offset 0x1C (4 bytes)
    //  */
    // void parseTrainerCardBlock(const Block& block);

    /**
     * Parses the ITEM block to extract inventory items.
     * Format: Multiple pouches with variable item counts
     */
    void parseItemBlock(const Block& block);

    /**
     * Parses the BOX block to extract box Pokemon.
     * Format: 32 boxes * 30 slots * SIZE_9PARTY (344 bytes)
     */
    void parseBoxBlock(const Block& block);

    /**
     * Parses the BOX_LAYOUT block to extract box names.
     * Format: 32 names * BOX_NAME_LENGTH (34 bytes, UTF-16LE)
     */
    void parseBoxLayoutBlock(const Block& block);
};

#endif
