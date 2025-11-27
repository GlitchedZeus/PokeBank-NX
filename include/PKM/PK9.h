/**
 * PK9.h - Generation 9 Pokemon Data Class
 *
 * This file defines the PK9 class for Generation 8 Pokemon games:
 * - Pokemon Scarlet/Violet
 * - Pokemon Legends: Z-A
 *
 * PK9 Format Details:
 * - Party Size: 344 bytes (0x158)
 * - Stored Size: 328 bytes (0x148)
 * - Encryption: XOR cipher + block shuffling
 * - Data Layout: 4 blocks (Growth, Attacks, EVs, Misc) + party stats
 *
 * Data Structure (Decrypted):
 * 0x00-0x07: Header (Encryption Constant, Checksum)
 * 0x08-0x57: Block A (Growth) - Species, items, EVs
 * 0x58-0xA7: Block B (Attacks) - Moves, IVs, nickname
 * 0xA8-0xF7: Block C (EVs/Contest) - Contest stats, ribbons
 * 0xF8-0x147: Block D (Misc) - OT info, encounter data
 * 0x148-0x157: Party Stats (Level, HP, Attack, Defense, etc.)
 */

#ifndef PK9_H
#define PK9_H

#include <cstdint>
#include <span>
#include <string>

#include "PKM/PKM.h"
#include "Encryption/Gen8Encryption.h"
#include "Utils/Utilities.h"
#include "Utils/StringHelpers.h"

/**
 * PK9 - Generation 8 Pokemon Class
 *
 * Inherits from PKM base class and implements Gen 8-specific data format.
 * Handles automatic decryption on construction and provides accessors for
 * all Pokemon properties.
 *
 * Gen 8 Unique Features:
 * - StatNature (mints can change effective nature without changing actual nature)
 * - Dynamax Level
 * - Home Tracker
 * - Sociability stat
 */
class PK9 final : public PKM
{
public:
    /**
     * Constructs a PK9 object from encrypted Pokemon data.
     *
     * Process:
     * 1. Decrypts the data using Gen8 decryption algorithm
     * 2. Stores decrypted data in internal buffer
     * 3. Creates span view for easy access
     *
     * @param raw Encrypted Pokemon data (SIZE_8PARTY or SIZE_8STORED bytes)
     */
    explicit PK9(std::span<const std::byte> raw)
    {
        // Decrypt the Gen 8 Pokemon data
        buffer = decryptArray8(raw);
        dataSize = raw.size();
        data = std::span<std::byte>(buffer, dataSize);
    }

    /**
     * Destructor - cleans up decrypted data buffer.
     * The base class PKM destructor handles buffer cleanup.
     */
    ~PK9() override = default;

    // Prevent copying (Pokemon data should not be accidentally copied)
    PK9(const PK9&) = delete;
    PK9& operator=(const PK9&) = delete;

    // Allow moving for efficient transfers
    PK9(PK9&&) noexcept = default;
    PK9& operator=(PK9&&) noexcept = default;

    // ========================================
    // Core Data Properties (Block A - Growth)
    // ========================================

    /**
     * Gets the Pokemon's Species ID.
     * Location: 0x08 (2 bytes)
     * @return Species ID (e.g., 1 = Bulbasaur, 25 = Pikachu)
     */
    uint16_t speciesID() const noexcept override
    {
        return readUInt16LittleEndian(reinterpret_cast<const uint8_t*>(data.data() + 0x08));
    }

    /**
     * Gets the Pokemon's species name as a string.
     * Uses lookup table to convert Species ID to name.
     * @return Species name (e.g., "Pikachu", "Charizard")
     */
    const char* species() const noexcept override;

    /**
     * Gets the held item ID.
     * Location: 0x0A (2 bytes)
     * @return Item ID (0 = no item)
     */
    uint16_t heldItem() const noexcept override
    {
        return readUInt16LittleEndian(reinterpret_cast<const uint8_t*>(data.data() + 0x0A));
    }

    /**
     * Gets the original trainer ID (32-bit format).
     * Location: 0x0C (4 bytes)
     * Gen 8 uses ID32 format: SID16 << 16 | TID16
     * @return Trainer ID32 value
     */
    uint32_t id32() const noexcept override
    {
        return readUInt32LittleEndian(reinterpret_cast<const uint8_t*>(data.data() + 0x0C));
    }

    /**
     * Gets the Pokemon's current experience points.
     * Location: 0x10 (4 bytes)
     * @return Experience value (determines level)
     */
    uint32_t exp() const noexcept override
    {
        return readUInt32LittleEndian(reinterpret_cast<const uint8_t*>(data.data() + 0x10));
    }

    /**
     * Gets the Pokemon's ability ID.
     * Location: 0x14 (2 bytes)
     * @return Ability ID
     */
    uint16_t ability() const noexcept override
    {
        return readUInt16LittleEndian(reinterpret_cast<const uint8_t*>(data.data() + 0x14));
    }

    /**
     * Gets the Pokemon's nature.
     * Location: 0x20 (1 byte)
     * Nature affects stat growth (e.g., Adamant boosts Attack, lowers Sp. Attack)
     * @return Nature ID (0-24)
     */
    uint8_t nature() const noexcept override
    {
        return static_cast<uint8_t>(data[0x20]);
    }

    /**
     * Gets the Pokemon's stat nature (affected by mints).
     * Location: 0x21 (1 byte)
     * Gen 8 introduced mints that can change effective nature without changing
     * the original nature. This value determines which nature affects stats.
     * @return Stat Nature ID (0-24)
     */
    uint8_t statNature() const noexcept
    {
        return static_cast<uint8_t>(data[0x21]);
    }

    // ========================================
    // Encryption and Identification
    // ========================================

    /**
     * Gets the Encryption Constant.
     * Location: 0x00 (4 bytes)
     * Used as the seed for encrypting/decrypting Pokemon data.
     * @return Encryption Constant value
     */
    uint32_t encryptionConstant() const noexcept override
    {
        return readUInt32LittleEndian(reinterpret_cast<const uint8_t*>(data.data() + 0x00));
    }

    /**
     * Gets the Personality ID (PID).
     * Location: 0x1C (4 bytes)
     * Determines gender, shininess, and other properties.
     * @return PID value
     */
    uint32_t pid() const noexcept override
    {
        return readUInt32LittleEndian(reinterpret_cast<const uint8_t*>(data.data() + 0x1C));
    }

    // ========================================
    // Block B (Attacks/Nickname)
    // ========================================

    /**
     * Gets the Pokemon's nickname (custom name set by trainer).
     * Location: 0x58 (26 bytes, UTF-16LE)
     * Maximum 13 characters including null terminator.
     * @return Nickname as UTF-16 string
     */
    std::u16string nickname() const override
    {
        const uint8_t* nicknameStart = reinterpret_cast<const uint8_t*>(data.data() + 0x58);
        return getString(nicknameStart, 26);
    }

    // ========================================
    // Block D (Misc)
    // ========================================

    /**
     * Gets the Pokemon's friendship/happiness value.
     * Location: 0xCA (1 byte)
     * Affects evolution for some Pokemon and move power for Return/Frustration.
     * @return Friendship value (0-255)
     */
    uint8_t friendship() const noexcept override
    {
        return static_cast<uint8_t>(data[0xCA]);
    }

    /**
     * Checks if the Pokemon is an egg.
     * Egg status is stored in bit 30 of the IV32 value.
     * @return true if egg, false otherwise
     */
    bool isEgg() const noexcept override
    {
        return (iv32() & 0x40000000) != 0;
    }

    /**
     * Gets the Pokerus status byte.
     * Location: 0xCB (1 byte)
     * Lower nibble: Days remaining (0 = cured)
     * Upper nibble: Strain
     * @return Pokerus status byte
     */
    uint8_t pokerus() const noexcept
    {
        return static_cast<uint8_t>(data[0xCB]);
    }

    /**
     * Checks if the Pokemon is currently infected with Pokerus.
     * @return true if infected, false otherwise
     */
    bool isPokerusInfected() const noexcept
    {
        return (pokerus() & 0xF) > 0;
    }

    /**
     * Checks if the Pokemon has been cured of Pokerus.
     * Cured Pokemon retain the EV gain bonus but can't spread the virus.
     * @return true if cured, false otherwise
     */
    bool isPokerusCured() const noexcept
    {
        return (pokerus() & 0xF0) > 0 && (pokerus() & 0xF) == 0;
    }

    // ========================================
    // Shiny and Gender
    // ========================================

    /**
     * Checks if the Pokemon is shiny (alternate coloration).
     *
     * Gen 8 shiny calculation:
     * XOR = (PID_High ^ PID_Low ^ TID16 ^ SID16)
     * Pokemon is shiny if XOR < 16
     * - XOR = 0: Square shiny
     * - XOR = 1-15: Star shiny
     *
     * @param trainerID32 The trainer's ID32 value
     * @param species Species name (for logging/debugging)
     * @return true if shiny, false otherwise
     */
    bool isShiny(uint32_t trainerID32, std::string species) const noexcept override
    {
        if (trainerID32 == 0) {
            return false;
        }
        uint32_t xorComponent = (pid() ^ trainerID32);
        uint32_t xorResult = (xorComponent ^ (xorComponent >> 16)) & 0xFFFF;
        return xorResult < 16;
    }

    /**
     * Gets the Pokemon's gender.
     * Gender is determined by PID and species gender ratio.
     * @return 0 = Male, 1 = Female, 2 = Genderless
     */
    uint8_t gender() const noexcept override;

    /**
     * Gets a gender symbol string for display.
     * @return "♂" for male, "♀" for female, "" for genderless
     */
    const char* genderSymbol() const noexcept override
    {
        uint8_t genderValue = gender();
        if (genderValue == 0) return "♂"; // Male
        if (genderValue == 1) return "♀"; // Female
        return ""; // Genderless
    }

    // ========================================
    // Stats - Effort Values (EVs)
    // ========================================

    /**
     * Effort Values (EVs) earned through battling.
     * Location: 0x26-0x2B (1 byte each)
     * Max 252 per stat, 510 total across all stats.
     */
    uint8_t evHP() const noexcept override  { return static_cast<uint8_t>(data[0x26]); }
    uint8_t evATK() const noexcept override { return static_cast<uint8_t>(data[0x27]); }
    uint8_t evDEF() const noexcept override { return static_cast<uint8_t>(data[0x28]); }
    uint8_t evSPE() const noexcept override { return static_cast<uint8_t>(data[0x29]); }
    uint8_t evSPA() const noexcept override { return static_cast<uint8_t>(data[0x2A]); }
    uint8_t evSPD() const noexcept override { return static_cast<uint8_t>(data[0x2B]); }

    /**
     * Sets an Effort Value for a specific stat.
     * Automatically recalculates battle stats and refreshes checksum.
     * @param statIndex 0=HP, 1=ATK, 2=DEF, 3=SPE, 4=SPA, 5=SPD
     * @param value EV value (0-252)
     */
    void setEV(int statIndex, uint8_t value) noexcept override {
        if (statIndex >= 0 && statIndex < 6) {
            data[0x26 + statIndex] = static_cast<std::byte>(value);
            recalculateStats();
            refreshChecksum();
        }
    }

    // ========================================
    // Stats - Individual Values (IVs)
    // ========================================

    /**
     * Gets the packed IV32 value.
     * Location: 0x8C (4 bytes)
     * Contains all 6 IVs plus special flags (IsEgg, IsNicknamed).
     */
    uint32_t iv32() const noexcept { return readUInt32LittleEndian(reinterpret_cast<const uint8_t*>(data.data() + 0x8C)); }

    /**
     * Individual Values (IVs) - inherent stat potential (0-31).
     * Extracted from IV32 using bit shifts:
     * - HP:  bits 0-4
     * - ATK: bits 5-9
     * - DEF: bits 10-14
     * - SPE: bits 15-19
     * - SPA: bits 20-24
     * - SPD: bits 25-29
     * - Bit 30: IsEgg flag
     * - Bit 31: IsNicknamed flag
     */
    uint8_t ivHP() const noexcept override  { return (iv32() >> 0) & 0x1F; }
    uint8_t ivATK() const noexcept override { return (iv32() >> 5) & 0x1F; }
    uint8_t ivDEF() const noexcept override { return (iv32() >> 10) & 0x1F; }
    uint8_t ivSPE() const noexcept override { return (iv32() >> 15) & 0x1F; }
    uint8_t ivSPA() const noexcept override { return (iv32() >> 20) & 0x1F; }
    uint8_t ivSPD() const noexcept override { return (iv32() >> 25) & 0x1F; }

    /**
     * Sets an Individual Value for a specific stat.
     * IVs are packed into a single 32-bit value, so we must:
     * 1. Read the current IV32
     * 2. Clear the 5 bits for this stat
     * 3. Set the new value
     * 4. Write back IV32
     * 5. Recalculate stats and checksum
     *
     * @param statIndex 0=HP, 1=ATK, 2=DEF, 3=SPE, 4=SPA, 5=SPD
     * @param value IV value (0-31)
     */
    void setIV(int statIndex, uint8_t value) noexcept override {
        if (statIndex >= 0 && statIndex < 6 && value <= 31) {
            uint32_t iv = iv32();
            int shift = statIndex * 5;
            uint32_t mask = ~(0x1F << shift);  // Clear the 5 bits for this stat
            iv = (iv & mask) | ((value & 0x1F) << shift);  // Set new value
            writeUInt32LittleEndian(reinterpret_cast<uint8_t*>(data.data() + 0x8C), iv);
            recalculateStats();
            refreshChecksum();
        }
    }

    // ========================================
    // Checksum Validation
    // ========================================

    /**
     * Gets the stored checksum value.
     * Location: 0x06 (2 bytes)
     * @return Checksum value
     */
    uint16_t checksum() const noexcept override {
        return readUInt16LittleEndian(reinterpret_cast<const uint8_t*>(data.data() + 0x06));
    }

    /**
     * Calculates the checksum from offset 0x08 to SIZE_8STORED.
     * Checksum is the sum of all 16-bit values in the encrypted blocks.
     * Party stats (0x148+) are NOT included in checksum.
     * @return Calculated checksum value
     */
    uint16_t calculateChecksum() const noexcept override {
        uint16_t checksum = 0;

        // Sum all 16-bit values from offset 0x08 to SIZE_8STORED
        const size_t checksumEnd = std::min(dataSize, SIZE_8STORED);
        for (size_t i = 0x08; i < checksumEnd; i += 2) {
            checksum += readUInt16LittleEndian(reinterpret_cast<const uint8_t*>(data.data() + i));
        }

        return checksum;
    }

    /**
     * Recalculates and updates the stored checksum.
     * MUST be called after any modification to Pokemon data.
     */
    void refreshChecksum() noexcept override {
        uint16_t newChecksum = calculateChecksum();
        writeUInt16LittleEndian(reinterpret_cast<uint8_t*>(data.data() + 0x06), newChecksum);
    }

    /**
     * Validates that stored checksum matches calculated checksum.
     * @return true if valid, false if data is corrupted
     */
    bool checksumValid() const noexcept override {
        return checksum() == calculateChecksum();
    }

    // ========================================
    // Base Stats (Species-Dependent)
    // ========================================

    /**
     * Base stats are determined by species and don't change per individual.
     * These are looked up from the PersonalInfo table.
     */
    uint8_t baseHP() const noexcept override;
    uint8_t baseATK() const noexcept override;
    uint8_t baseDEF() const noexcept override;
    uint8_t baseSPE() const noexcept override;
    uint8_t baseSPA() const noexcept override;
    uint8_t baseSPD() const noexcept override;

    // ========================================
    // Party Stats (Calculated Battle Stats)
    // ========================================

    /**
     * Party stats are the actual values used in battle.
     * Location: 0x148-0x157 (unencrypted section)
     * These are calculated from base stats, IVs, EVs, nature, and level.
     */
    uint8_t level() const noexcept override { return static_cast<uint8_t>(data[0x148]); }
    uint16_t statHPMax() const noexcept override { return readUInt16LittleEndian(reinterpret_cast<const uint8_t*>(data.data() + 0x14A)); }
    uint16_t statATK() const noexcept override { return readUInt16LittleEndian(reinterpret_cast<const uint8_t*>(data.data() + 0x14C)); }
    uint16_t statDEF() const noexcept override { return readUInt16LittleEndian(reinterpret_cast<const uint8_t*>(data.data() + 0x14E)); }
    uint16_t statSPE() const noexcept override { return readUInt16LittleEndian(reinterpret_cast<const uint8_t*>(data.data() + 0x150)); }
    uint16_t statSPA() const noexcept override { return readUInt16LittleEndian(reinterpret_cast<const uint8_t*>(data.data() + 0x152)); }
    uint16_t statSPD() const noexcept override { return readUInt16LittleEndian(reinterpret_cast<const uint8_t*>(data.data() + 0x154)); }

    // ========================================
    // Stat Calculation
    // ========================================

    /**
     * Gets the nature modifier for a given stat.
     * Nature affects 5 stats (all except HP):
     * - Increased stat: 1.1x (110%)
     * - Decreased stat: 0.9x (90%)
     * - Neutral: 1.0x (100%)
     *
     * @param statIndex 0=ATK, 1=DEF, 2=SPE, 3=SPA, 4=SPD
     * @return Modifier percentage (90, 100, or 110)
     */
    int getNatureModifier(int statIndex) const noexcept;

    /**
     * Recalculates all battle stats based on current values.
     * Formula (HP): ((2 * Base + IV + EV/4) * Level / 100) + Level + 10
     * Formula (Other): (((2 * Base + IV + EV/4) * Level / 100) + 5) * NatureMod
     *
     * This should be called after modifying IVs, EVs, or nature.
     */
    void recalculateStats() noexcept override;

    // ========================================
    // Advanced Modification
    // ========================================

    /**
     * Regenerates PID while maintaining gender and shininess.
     * Used to fix legality issues when IVs are modified.
     * @param trainerID32 The trainer's ID32 for shiny calculation
     */
    void regeneratePID(uint32_t trainerID32) noexcept override;

    /**
     * Sets the shiny status of the Pokemon.
     * Modifies PID while preserving gender.
     * @param makeShiny true to make shiny, false to make non-shiny
     * @param trainerID32 The trainer's ID32 for shiny calculation
     */
    void setShiny(bool makeShiny, uint32_t trainerID32) noexcept override;

    /**
     * Helper to convert uint32_t to hex string.
     * @param value Value to convert
     * @return Hex string representation
     */
    static std::string toHex(uint32_t value);
};

#endif // PK9_H
