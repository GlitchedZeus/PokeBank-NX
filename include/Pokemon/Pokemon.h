/**
 * Pokemon.h - Base Pokemon Data Class
 *
 * This file defines the abstract base class for all Pokemon data structures.
 * This base class provides a unified interface for accessing Pokemon data across all generations.
 *
 * Derived classes (PK7, PK8, PK9, etc.) implement generation-specific data
 * formats and storage layouts.
 */

#ifndef POKEMON_POKEMON_H
#define POKEMON_POKEMON_H

#include <cstdint>
#include <cstddef>
#include <vector>
#include <span>
#include <string>
#include <memory>

namespace Pokemon {
    /**
     * PKM - Abstract base class for Pokemon entity data
     *
     * This class provides the foundation for all generation-specific Pokemon classes.
     * It defines the common interface that all Pokemon formats must implement,
     * while allowing each generation to handle its own data layout and encryption.
     *
     * Memory Layout:
     * - Each derived class manages its own byte buffer containing Pokemon data
     * - Data may be encrypted or decrypted depending on the context
     * - Size varies by generation (e.g., PK8 = 344 bytes, PK7 = 260 bytes)
     *
     * Usage Pattern:
     * 1. Derived class receives encrypted data
     * 2. Constructor decrypts and stores data in internal buffer
     * 3. Getters/setters access decrypted data directly
     * 4. When saving, data is re-encrypted using generation-specific methods
     */
    class Pokemon {
    protected:
        /**
         * Internal buffer storing the Pokemon's decrypted data.
         * This buffer is managed by the derived class and should contain
         * all Pokemon information in its decrypted form for easy access.
         */
        std::byte* buffer = nullptr;

        /**
         * Span view of the decrypted Pokemon data.
         * Provides safe, bounds-checked access to the data buffer.
         */
        std::span<std::byte> data;

        /**
         * Actual size of the Pokemon data in bytes.
         * This varies by generation:
         * - Gen 8 (PK8): 344 bytes (party) or 328 bytes (stored)
         * - Gen 7 LGP/E (PK7): 260 bytes
         * - Gen 9 (PK9): varies by format
         */
        size_t dataSize;

    public:
        // Virtual destructor to ensure proper cleanup in derived classes
        virtual ~Pokemon() {
            if (buffer) {
                delete[] buffer;
                buffer = nullptr;
            }
        }

        // Delete copy operations to prevent accidental copies of Pokemon data
        Pokemon(const Pokemon&) = delete;
        Pokemon& operator=(const Pokemon&) = delete;

        // Allow move operations for efficient transfers
        Pokemon(Pokemon&&) noexcept = default;
        Pokemon& operator=(Pokemon&&) noexcept = default;

        // ========================================
        // Core Data Properties (Pure Virtual)
        // ========================================

        /**
         * Gets the Pokemon's Species ID.
         * @return Species ID (e.g., 1 = Bulbasaur, 25 = Pikachu, 133 = Eevee)
         */
        virtual uint16_t speciesID() const noexcept = 0;

        /**
         * Gets the Pokemon's species name as a string.
         * @return Species name (e.g., "Pikachu", "Eevee")
         */
        virtual const char* species() const noexcept = 0;

        /**
         * Gets the Pokemon's nickname (custom name set by trainer).
         * @return Nickname as UTF-16 string
         */
        virtual std::u16string nickname() const = 0;

        /**
         * Gets the form ID.
         * @return Form ID (0 = no form)
         */
        virtual uint16_t formID() const noexcept = 0;

        /**
         * Gets the Pokemon's form name as a string.
         * @return Form name (e.g., "Alolan", "Galarian")
         */
        // virtual const char* form() const noexcept = 0;

        /**
         * Gets the held item ID.
         * @return Item ID (0 = no item)
         */
        virtual uint16_t heldItem() const noexcept = 0;

        /**
         * Gets the Pokemon's original trainer ID (32-bit format).
         * @return Trainer ID32 value
         */
        virtual uint32_t id32() const noexcept = 0;

        /**
         * Gets the Pokemon's current experience points.
         * @return Experience value
         */
        virtual uint32_t exp() const noexcept = 0;

        /**
         * Gets the Pokemon's ability ID.
         * @return Ability ID
         */
        virtual uint16_t ability() const noexcept = 0;

        /**
         * Gets the Pokemon's nature (affects stat growth).
         * @return Nature ID (0-24)
         */
        virtual uint8_t nature() const noexcept = 0;

        /**
         * Gets the Pokemon's stat nature.
         * @return Nature ID (0-24)
         */
        virtual uint8_t statNature() const noexcept = 0;

        /**
         * Gets the Pokemon's current level.
         * @return Level (1-100)
         */
        virtual uint8_t level() const noexcept = 0;

        /**
         * Gets the Pokemon's gender.
         * @return 0 = Male, 1 = Female, 2 = Genderless
         */
        virtual uint8_t gender() const noexcept = 0;

        /**
         * Gets a gender symbol string for display.
         * @return "♂" for male, "♀" for female, "" for genderless
         */
        virtual const char* genderSymbol() const noexcept = 0;

        /**
         * Gets the Personality ID (PID).
         * Used for determining gender, shininess, and other properties.
         * @return PID value
         */
        virtual uint32_t pid() const noexcept = 0;

        /**
         * Gets the Encryption Constant.
         * Used as the seed for encrypting/decrypting Pokemon data.
         * @return Encryption Constant value
         */
        virtual uint32_t encryptionConstant() const noexcept = 0;

        // ========================================
        // Stats - Individual Values (IVs)
        // ========================================

        /**
         * Individual Values (IVs) are inherent stat values (0-31) that determine
         * a Pokemon's potential. Higher IVs result in higher final stats.
         */
        virtual uint8_t ivHP() const noexcept = 0;
        virtual uint8_t ivATK() const noexcept = 0;
        virtual uint8_t ivDEF() const noexcept = 0;
        virtual uint8_t ivSPE() const noexcept = 0;
        virtual uint8_t ivSPA() const noexcept = 0;
        virtual uint8_t ivSPD() const noexcept = 0;

        /**
         * Sets an Individual Value for a specific stat.
         * @param statIndex 0=HP, 1=ATK, 2=DEF, 3=SPE, 4=SPA, 5=SPD
         * @param value IV value (0-31)
         */
        virtual void setIV(int statIndex, uint8_t value) noexcept = 0;

        // ========================================
        // Stats - Effort Values (EVs)
        // ========================================

        /**
         * Effort Values (EVs) are earned through battling and training.
         * They provide additional stat points (max 252 per stat, 510 total).
         */
        virtual uint8_t evHP() const noexcept = 0;
        virtual uint8_t evATK() const noexcept = 0;
        virtual uint8_t evDEF() const noexcept = 0;
        virtual uint8_t evSPE() const noexcept = 0;
        virtual uint8_t evSPA() const noexcept = 0;
        virtual uint8_t evSPD() const noexcept = 0;

        /**
         * Sets an Effort Value for a specific stat.
         * @param statIndex 0=HP, 1=ATK, 2=DEF, 3=SPE, 4=SPA, 5=SPD
         * @param value EV value (0-252)
         */
        virtual void setEV(int statIndex, uint8_t value) noexcept = 0;

        // ========================================
        // Base Stats (Species-Dependent)
        // ========================================

        /**
         * Base stats are determined by species and don't change per individual.
         * These are looked up from the species data table.
         */
        virtual uint8_t baseHP() const noexcept = 0;
        virtual uint8_t baseATK() const noexcept = 0;
        virtual uint8_t baseDEF() const noexcept = 0;
        virtual uint8_t baseSPE() const noexcept = 0;
        virtual uint8_t baseSPA() const noexcept = 0;
        virtual uint8_t baseSPD() const noexcept = 0;

        // ========================================
        // Calculated Stats (Battle Stats)
        // ========================================

        /**
         * These are the actual stats used in battle, calculated from:
         * - Base stats (species-dependent)
         * - IVs (individual values)
         * - EVs (effort values)
         * - Nature (stat modifiers)
         * - Level
         */
        virtual uint16_t statHPMax() const noexcept = 0;
        virtual uint16_t statATK() const noexcept = 0;
        virtual uint16_t statDEF() const noexcept = 0;
        virtual uint16_t statSPE() const noexcept = 0;
        virtual uint16_t statSPA() const noexcept = 0;
        virtual uint16_t statSPD() const noexcept = 0;

        // ========================================
        // Status and Conditions
        // ========================================

        /**
         * Gets the Pokemon's friendship/happiness value.
         * @return Friendship value (0-255)
         */
        virtual uint8_t friendship() const noexcept = 0;

        /**
         * Checks if the Pokemon is an egg.
         * @return true if egg, false otherwise
         */
        virtual bool isEgg() const noexcept = 0;

        /**
         * Checks if the Pokemon is shiny (alternate coloration).
         * Shininess is determined by XOR of trainer ID and PID.
         * @param trainerID32 The trainer's ID32 value
         * @param species Species name (for logging/debugging)
         * @return true if shiny, false otherwise
         */
        virtual bool isShiny(uint32_t trainerID32, std::string species) const noexcept = 0;

        /**
         * Checks if the Pokemon is infected with Pokerus.
         * @return true if infected, false otherwise
         */
        virtual bool isPokerusInfected() const noexcept = 0;

        /**
         * Checks if the Pokemon has been cured of Pokerus.
         * @return true if cured, false otherwise
         */
        virtual bool isPokerusCured() const noexcept = 0;

        // ========================================
        // Data Integrity
        // ========================================

        /**
         * Gets the stored checksum value.
         * The checksum validates data integrity.
         * @return Checksum value
         */
        virtual uint16_t checksum() const noexcept = 0;

        /**
         * Calculates the checksum from current data.
         * @return Calculated checksum value
         */
        virtual uint16_t calculateChecksum() const noexcept = 0;

        /**
         * Updates the stored checksum to match current data.
         * This MUST be called after any data modifications.
         */
        virtual void refreshChecksum() noexcept = 0;

        /**
         * Validates that the stored checksum matches calculated checksum.
         * @return true if valid, false if corrupted
         */
        virtual bool checksumValid() const noexcept = 0;

        // ========================================
        // Stat Recalculation
        // ========================================

        /**
         * Recalculates all battle stats based on current IVs, EVs, nature, and level.
         * This should be called after modifying any stat-affecting values.
         */
        virtual void recalculateStats() noexcept = 0;

        // ========================================
        // Advanced Modification
        // ========================================

        /**
         * Regenerates PID while maintaining gender and shininess.
         * Used to fix legality issues when IVs are modified.
         * @param trainerID32 The trainer's ID32 for shiny calculation
         */
        virtual void regeneratePID(uint32_t trainerID32) noexcept = 0;

        /**
         * Sets the shiny status of the Pokemon.
         * Modifies PID while preserving gender.
         * @param makeShiny true to make shiny, false to make non-shiny
         * @param trainerID32 The trainer's ID32 for shiny calculation
         */
        virtual void setShiny(bool makeShiny, uint32_t trainerID32) noexcept = 0;

        // ========================================
        // Data Access
        // ========================================

        /**
         * Gets the size of the Pokemon data.
         * @return Data size in bytes
         */
        size_t getDataSize() const noexcept { return dataSize; }

        /**
         * Gets direct access to the decrypted data buffer.
         * WARNING: Use with caution. Modifying data directly requires
         * calling RefreshChecksum() afterwards.
         * @return Span view of the data buffer
         */
        std::span<std::byte> getData() noexcept { return data; }

        /**
         * Gets read-only access to the decrypted data buffer.
         * @return Const span view of the data buffer
         */
        std::span<const std::byte> getData() const noexcept { return data; }

    protected:
        // Default constructor for derived classes
        Pokemon() = default;
    };
}

#endif
