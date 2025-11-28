/**
 * Trainer8.h - Generation 8 Trainer/Save File Data Management
 *
 * This file defines the Trainer8 class for Pokemon Generation 8 games:
 * - Pokemon Sword/Shield
 * - Pokemon Brilliant Diamond/Shining Pearl (BDSP)
 * - Pokemon Legends: Arceus (PLA, though it uses PA8)
 *
 * Trainer8 implements generation-specific logic for:
 * - PK8 Pokemon storage (party and boxes)
 * - Gen 8 block keys
 * - Gen 8 encryption (encryptArray8/decryptArray8)
 * - Gen 8-specific save file structure
 */

#ifndef TRAINER_TRAINER8_H
#define TRAINER_TRAINER8_H

#include <cstring>

#include "Trainer/Trainer.h"
#include "PKM/PK8.h"
#include "Encryption/Gen8Encryption.h"

// ========================================
// Generation 8 Save File Block Keys
// ========================================

namespace Gen8BlockKeys {
    constexpr size_t MY_STATUS = 0xf25c070e;        // Trainer Details
    constexpr size_t PARTY = 0x2985fe5d;            // Party Data
    constexpr size_t MISC = 0x1b882b09;             // Money
    constexpr size_t TRAINER_CARD = 0x874da6fa;     // Trainer Card
    constexpr size_t PLAY_TIME = 0x8cbbfd90;        // Time Played
    constexpr size_t ITEM = 0x1177c2c4;             // Items
    constexpr size_t BOX = 0x0d66012c;              // Box Data
    constexpr size_t BOX_LAYOUT = 0x19722c89;       // Box Names
    // constexpr size_t BOX_WALLPAPERS = 0x2EB1B190;   // Box Wallpapers
}

// Generation 8 constants
constexpr size_t BOX_COUNT_GEN8 = 32;       // Number of boxes in Sword/Shield
constexpr size_t BOX_NAME_LENGTH_GEN8 = 0x22; // 34 bytes per box name (UTF-16LE)

/**
 * Trainer8 - Generation 8 Trainer Class
 *
 * Inherits from Trainer base class and implements Gen 8-specific save file format.
 * Handles automatic decryption on construction and provides Gen 8-specific
 * encryption when updating blocks.
 *
 * Gen 8 Specific Features:
 * - 32 boxes with 30 slots each
 * - PK8 Pokemon format (344 bytes party, 328 bytes stored)
 * - Block-based save file structure
 * - Gen 8 encryption algorithm
 *
 * Save File Structure (Sword/Shield):
 * - File: "main" (approximately 1.6MB)
 * - Format: Multiple blocks identified by key values
 * - Encryption: Block-level encryption + Pokemon encryption
 */
class Trainer8 final : public Trainer
{
public:
    // ========================================
    // Constructor
    // ========================================

    /**
     * Constructs a Trainer8 object from save file blocks.
     *
     * Process:
     * 1. Parses blocks to extract trainer info
     * 2. Decrypts and loads party Pokemon (PK8)
     * 3. Decrypts and loads box Pokemon (PK8)
     * 4. Loads items and box names
     *
     * @param blocks Save file blocks parsed from Gen 8 save file
     */
    explicit Trainer8(std::vector<Block> blocks) : Trainer(std::move(blocks))
    {
        party.reserve(MAX_PARTY_SLOTS);
        boxes.resize(BOX_COUNT_GEN8);
        boxNames.resize(BOX_COUNT_GEN8);

        // Parse all blocks to extract data
        for (const auto& block : this->blocks) {
            parseBlock(block);
        }
    }

    /// Destructor - cleanup handled by base class and unique_ptrs
    ~Trainer8() override = default;

    // Delete copy operations
    Trainer8(const Trainer8&) = delete;
    Trainer8& operator=(const Trainer8&) = delete;

    // Allow move operations
    Trainer8(Trainer8&&) noexcept = default;
    Trainer8& operator=(Trainer8&&) noexcept = default;

    // ========================================
    // Implementation of Pure Virtual Methods
    // ========================================

    /**
     * Updates the PARTY_KEY block with modified Pokemon data.
     * Uses Gen 8 encryption (encryptArray8).
     */
    void updatePartyBlock() override;

    /**
     * Updates the BOX_KEY block with modified Pokemon data.
     * Uses Gen 8 encryption (encryptArray8).
     */
    void updateBoxBlock() override;

    /**
     * Updates the ITEM_KEY block with modified inventory data.
     */
    void updateItemBlock() override;

    /**
     * Gets the number of boxes available in Gen 8.
     * @return 32 (Sword/Shield has 32 boxes)
     */
    size_t getBoxCount() const noexcept override {
        return BOX_COUNT_GEN8;
    }

    /**
     * Gets the number of Pokemon currently in the party.
     * @return Party size (0-6)
     */
    size_t getPartySize() const noexcept override {
        return party.size();
    }

    /**
     * Gets the game group for Gen 8 trainers.
     * @return GameVersion::SWSH (Sword/Shield group)
     */
    GameVersion getGameGroup() const noexcept override {
        return GameVersion::SWSH;
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

    /**
     * Parses the TRAINER_CARD block to extract trainer name.
     * Location: Name at offset 0x00 (26 bytes, UTF-16LE)
     * Location: Trainer ID at offset 0x1C (4 bytes)
     */
    void parseTrainerCardBlock(const Block& block);

    /**
     * Parses the ITEM block to extract inventory items.
     * Format: Multiple pouches with variable item counts
     */
    void parseItemBlock(const Block& block);

    /**
     * Parses the BOX block to extract box Pokemon.
     * Format: 32 boxes * 30 slots * SIZE_8PARTY (344 bytes)
     */
    void parseBoxBlock(const Block& block);

    /**
     * Parses the BOX_LAYOUT block to extract box names.
     * Format: 32 names * BOX_NAME_LENGTH (34 bytes, UTF-16LE)
     */
    void parseBoxLayoutBlock(const Block& block);
};

#endif
