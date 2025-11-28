/**
 * Trainer.h - Base Trainer/Save File Data Management
 *
 * This file defines the abstract base Trainer class and related structures for
 * managing Pokemon save file data. This base class provides a unified interface for accessing trainer data
 * across all generations.
 *
 * Derived classes (Trainer7, Trainer8, etc.) implement generation-specific data
 * formats, encryption, and Pokemon storage.
 */

#ifndef TRAINER_TRAINER_H
#define TRAINER_TRAINER_H

#include <cstdint>
#include <vector>
#include <span>
#include <array>
#include <memory>
#include <string>

#include "Utils/Block.h"
#include "Utils/Logger.h"
#include "Utils/Utilities.h"
#include "Utils/StringHelpers.h"
#include "Trainer/Inventory.h"
#include "PKM/PKM.h"
#include "Enums/GameVersion.h"

// Pokemon storage constants (common across generations)
constexpr size_t MAX_PARTY_SLOTS = 6;  // Maximum Pokemon in party
constexpr size_t BOX_SLOTS = 30;       // Pokemon per box (6x5 grid)

// ========================================
// Name Lookup Functions
// ========================================

/// Converts a Species ID to its name string
const char* getSpeciesName(uint16_t speciesId);

/// Converts an Item ID to its name string
const char* getItemName(uint16_t itemId);

/// Gets total number of items
size_t getItemCount();

/// Converts a Nature ID to its name string
const char* getNatureName(uint8_t natureId);

/// Converts an Ability ID to its name string
const char* getAbilityName(uint16_t abilityId);

/// Base stats structure
struct BaseStats {
    uint8_t hp;
    uint8_t atk;
    uint8_t def;
    uint8_t spe;
    uint8_t spa;
    uint8_t spd;
};

/// Gets base stats for a given species ID
const BaseStats* getBaseStats(uint16_t speciesId);

/**
 * Trainer - Abstract base class for save file data
 *
 * This class provides the foundation for all generation-specific Trainer classes.
 * It defines the common interface that all save file formats must implement,
 * while allowing each generation to handle its own data layout, encryption, and
 * Pokemon storage.
 *
 * Common Data (stored in base class):
 * - Trainer information (ID, name, money)
 * - Item inventory
 * - Box names
 * - Save file blocks
 *
 * Generation-Specific Data (implemented in derived classes):
 * - Party Pokemon storage (PK7 vs PK8 vs PK9)
 * - Box Pokemon storage
 * - Block key constants
 * - Encryption methods
 *
 * Usage Pattern:
 * 1. Derived class receives save file blocks
 * 2. Constructor parses blocks and populates data
 * 3. User modifies Pokemon, items, or trainer info
 * 4. Update methods serialize changes back to blocks
 * 5. Blocks are re-encrypted and saved to file
 */
class Trainer
{
protected:
    /**
     * All save file blocks for re-serialization.
     * Blocks contain encrypted data segments identified by key values.
     */
    std::vector<Block> blocks;

public:
    // ========================================
    // Common Trainer Data
    // ========================================

    /// Trainer ID (legacy format, varies by game)
    int32_t trainerID;

    /// Trainer name (UTF-8 string)
    std::string trainerName;

    /// Money/currency amount
    uint32_t money;

    /// Trainer ID (32-bit format: SID16 << 16 | TID16)
    uint32_t ID32;

    /// Trainer ID (16-bit visible ID)
    uint16_t TID16;

    /// Secret ID (16-bit hidden ID)
    uint16_t SID16;

    /// Display Trainer ID (Gen 7+)
    uint32_t TID;

    /// Display Secret ID (Gen 7+)
    uint32_t SID;

    /// Items organized by pouch type (Medicine, Balls, etc.)
    std::vector<std::vector<InventoryItem>> items;

    /// Names of each box (UTF-8 strings)
    std::vector<std::string> boxNames;

    /// Party Pokemon (1-6 Pokemon) - stored polymorphically
    std::vector<std::unique_ptr<PKM>> party;

    /// Box Pokemon storage [box_index][slot_index] - stored polymorphically
    /// nullptr = empty slot
    std::vector<std::array<std::unique_ptr<PKM>, BOX_SLOTS>> boxes;

    // ========================================
    // Constructors and Destructor
    // ========================================

    /**
     * Constructs a Trainer object from save file blocks.
     * Derived classes call this constructor to initialize common data.
     *
     * @param blocks Save file blocks parsed from the save file
     */
    explicit Trainer(std::vector<Block> blocks) : blocks(std::move(blocks)) {}

    /// Virtual destructor to ensure proper cleanup in derived classes
    virtual ~Trainer() = default;

    // Delete copy operations to prevent accidental copies of save data
    Trainer(const Trainer&) = delete;
    Trainer& operator=(const Trainer&) = delete;

    // Allow move operations for efficient transfers
    Trainer(Trainer&&) noexcept = default;
    Trainer& operator=(Trainer&&) noexcept = default;

    // ========================================
    // Pure Virtual Methods (Must Implement)
    // ========================================

    /**
     * Updates the party block with modified Pokemon data.
     * Each generation implements this with generation-specific encryption.
     */
    virtual void updatePartyBlock() = 0;

    /**
     * Updates the box blocks with modified Pokemon data.
     * Each generation implements this with generation-specific encryption.
     */
    virtual void updateBoxBlock() = 0;

    /**
     * Updates the item block with modified inventory data.
     * Item structure varies slightly between generations.
     */
    virtual void updateItemBlock() = 0;

    /**
     * Gets the number of boxes available in this generation.
     * @return Number of boxes (e.g., 32 for Sword/Shield, 40 for Let's Go)
     */
    virtual size_t getBoxCount() const noexcept = 0;

    /**
     * Gets the number of Pokemon currently in the party.
     * @return Party size (0-6)
     */
    virtual size_t getPartySize() const noexcept = 0;

    /**
     * Gets the game group for this trainer type.
     * Used for type dispatch without RTTI (required for Nintendo Switch builds).
     * @return GameVersion group (GG, SWSH, etc.)
     */
    virtual GameVersion getGameGroup() const noexcept = 0;

    // ========================================
    // Common Methods
    // ========================================

    /**
     * Gets the blocks for serialization back to save file.
     * @return Const reference to blocks vector
     */
    const std::vector<Block>& getBlocks() const {
        return blocks;
    }

protected:
    /**
     * Default constructor for derived classes.
     * Protected to prevent direct instantiation of base class.
     */
    Trainer() = default;
};

#endif
