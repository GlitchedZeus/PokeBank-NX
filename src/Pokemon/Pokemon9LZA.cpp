/**
 * Pokemon9LZA.cpp - Generation 9 Pokemon Data Class Implementation
 *
 * Implementation of Pokemon9LZA class methods including stat calculations,
 * gender determination, shiny manipulation, and PID regeneration.
 */

#include <cstdio>
#include <algorithm>

#include "Pokemon/Pokemon9LZA.h"

// Forward declarations for external helper functions
// These are defined in other files and provide species/item/nature name lookups
extern const char* getSpeciesName(uint16_t speciesId);
extern const char* getItemName(uint16_t itemId);
extern const char* getNatureName(uint8_t natureId);
extern const char* getAbilityName(uint16_t abilityId);

// Base stats structure and lookup function
struct BaseStats {
    uint8_t hp;
    uint8_t atk;
    uint8_t def;
    uint8_t spe;
    uint8_t spa;
    uint8_t spd;
};

extern const BaseStats* getBaseStats(uint16_t speciesId);

// ========================================
// Species and Name Lookups
// ========================================

const char* Pokemon9LZA::species() const noexcept
{
    /**
     * Converts the Pokemon's Species ID to its name string.
     * Uses an external lookup table (defined elsewhere) to get the species name.
     */
    return getSpeciesName(speciesID());
}

// ========================================
// Gender Determination
// ========================================

uint8_t Pokemon9LZA::gender() const noexcept
{
    /**
     * Determines the Pokemon's gender based on species and PID.
     *
     * Gender is determined by:
     * 1. Species gender ratio (some species are genderless, male-only, or female-only)
     * 2. PID value for species with mixed genders
     *
     * Gender Ratios (simplified):
     * - Genderless: Always return 2
     * - Male-only: Always return 0
     * - Female-only: Always return 1
     * - Mixed: Use (PID & 0xFF) compared against gender threshold
     *
     * This is a simplified implementation. A full implementation would include
     * a lookup table for all species' gender ratios.
     */

    uint16_t species = speciesID();

    // Genderless species (simplified list)
    // These Pokemon have no gender
    if (species == 81 || species == 82 ||     // Magnemite, Magneton
        species == 100 || species == 101 ||   // Voltorb, Electrode
        species == 120 || species == 121 ||   // Staryu, Starmie
        species == 132 ||                     // Ditto
        species == 137 || species == 233 ||   // Porygon, Porygon2
        species == 144 || species == 145 || species == 146 || // Legendary birds
        species == 150 || species == 151 ||   // Mewtwo, Mew
        species == 201) {                     // Unown
        return 2; // Genderless
    }

    // For most species, use PID % 256 < 127 for gender determination
    // This is simplified - real implementation would need species gender ratios
    // Typical ratios: 50/50, 87.5/12.5, 75/25, 100/0, 0/100
    return ((pid() & 0xFF) < 127) ? 0 : 1;
}

// ========================================
// Base Stats (Species-Dependent)
// ========================================

uint8_t Pokemon9LZA::baseHP() const noexcept
{
    /**
     * Looks up the base HP stat for this Pokemon's species.
     * Base stats are fixed per species and determine stat growth.
     */
    return getBaseStats(speciesID())->hp;
}

uint8_t Pokemon9LZA::baseATK() const noexcept
{
    return getBaseStats(speciesID())->atk;
}

uint8_t Pokemon9LZA::baseDEF() const noexcept
{
    return getBaseStats(speciesID())->def;
}

uint8_t Pokemon9LZA::baseSPE() const noexcept
{
    return getBaseStats(speciesID())->spe;
}

uint8_t Pokemon9LZA::baseSPA() const noexcept
{
    return getBaseStats(speciesID())->spa;
}

uint8_t Pokemon9LZA::baseSPD() const noexcept
{
    return getBaseStats(speciesID())->spd;
}

// ========================================
// Nature Modifier
// ========================================

int Pokemon9LZA::getNatureModifier(int statIndex) const noexcept
{
    /**
     * Returns the nature modifier for a given stat.
     *
     * Nature affects 5 stats (all except HP):
     * - Increased stat: 110 (multiply by 110, divide by 100 = 1.1x)
     * - Decreased stat: 90 (multiply by 90, divide by 100 = 0.9x)
     * - Neutral: 100 (multiply by 100, divide by 100 = 1.0x)
     *
     * Gen 8 uses StatNature (affected by mints) instead of Nature for calculations.
     * Mints allow changing effective nature without changing the Pokemon's actual nature.
     *
     * Nature Table Format:
     * Each nature has 5 modifiers for [ATK, DEF, SPE, SPA, SPD]:
     * - 0 = decreased (0.9x)
     * - 1 = neutral (1.0x)
     * - 2 = increased (1.1x)
     */

    uint8_t nature = statNature();

    // Nature modifier table [25 natures][5 stats]
    // Stats: 0=ATK, 1=DEF, 2=SPE, 3=SPA, 4=SPD
    static const int8_t natureTable[25][5] = {
        {1, 1, 1, 1, 1}, // 0: Hardy (neutral)
        {2, 0, 1, 1, 1}, // 1: Lonely (+Atk, -Def)
        {2, 1, 0, 1, 1}, // 2: Brave (+Atk, -Spe)
        {2, 1, 1, 0, 1}, // 3: Adamant (+Atk, -SpA)
        {2, 1, 1, 1, 0}, // 4: Naughty (+Atk, -SpD)
        {0, 2, 1, 1, 1}, // 5: Bold (-Atk, +Def)
        {1, 1, 1, 1, 1}, // 6: Docile (neutral)
        {1, 2, 0, 1, 1}, // 7: Relaxed (+Def, -Spe)
        {1, 2, 1, 0, 1}, // 8: Impish (+Def, -SpA)
        {1, 2, 1, 1, 0}, // 9: Lax (+Def, -SpD)
        {0, 1, 2, 1, 1}, // 10: Timid (-Atk, +Spe)
        {1, 0, 2, 1, 1}, // 11: Hasty (-Def, +Spe)
        {1, 1, 1, 1, 1}, // 12: Serious (neutral)
        {1, 1, 2, 0, 1}, // 13: Jolly (+Spe, -SpA)
        {1, 1, 2, 1, 0}, // 14: Naive (+Spe, -SpD)
        {0, 1, 1, 2, 1}, // 15: Modest (-Atk, +SpA)
        {1, 0, 1, 2, 1}, // 16: Mild (-Def, +SpA)
        {1, 1, 0, 2, 1}, // 17: Quiet (+SpA, -Spe)
        {1, 1, 1, 1, 1}, // 18: Bashful (neutral)
        {1, 1, 1, 2, 0}, // 19: Rash (+SpA, -SpD)
        {0, 1, 1, 1, 2}, // 20: Calm (-Atk, +SpD)
        {1, 0, 1, 1, 2}, // 21: Gentle (-Def, +SpD)
        {1, 1, 0, 1, 2}, // 22: Sassy (+SpD, -Spe)
        {1, 1, 1, 0, 2}, // 23: Careful (+SpD, -SpA)
        {1, 1, 1, 1, 1}, // 24: Quirky (neutral)
    };

    // Validate inputs
    if (nature >= 25 || statIndex < 0 || statIndex >= 5) {
        return 100; // Neutral if invalid
    }

    int modifier = natureTable[nature][statIndex];
    return modifier == 0 ? 90 : (modifier == 2 ? 110 : 100);
}

// ========================================
// Stat Recalculation
// ========================================

void Pokemon9LZA::recalculateStats() noexcept
{
    /**
     * Recalculates all battle stats based on current IVs, EVs, base stats, level, and nature.
     *
     * Stat Formulas (Gen 3+):
     * HP = ((2 * Base + IV + EV/4) * Level / 100) + Level + 10
     * Other Stats = (((2 * Base + IV + EV/4) * Level / 100) + 5) * NatureMod / 100
     *
     * Where:
     * - Base = Base stat for the species
     * - IV = Individual Value (0-31)
     * - EV = Effort Value (0-252)
     * - Level = Pokemon's current level (1-100)
     * - NatureMod = Nature modifier (90, 100, or 110)
     */

    uint8_t levelValue = level();
    if (levelValue == 0 || levelValue > 100) {
        return; // Invalid level, don't recalculate
    }

    // Calculate HP (different formula from other stats)
    int hp = ((2 * baseHP() + ivHP() + (evHP() / 4)) * levelValue) / 100 + levelValue + 10;
    writeUInt16LittleEndian(reinterpret_cast<uint8_t*>(data.data() + 0x14A), static_cast<uint16_t>(hp));

    // Calculate other stats (ATK, DEF, SPE, SPA, SPD)
    int stats[5];
    int baseStats[5] = {baseATK(), baseDEF(), baseSPE(), baseSPA(), baseSPD()};
    int ivs[5] = {ivATK(), ivDEF(), ivSPE(), ivSPA(), ivSPD()};
    int evs[5] = {evATK(), evDEF(), evSPE(), evSPA(), evSPD()};

    for (int i = 0; i < 5; i++) {
        // Base stat calculation
        int baseStat = ((2 * baseStats[i] + ivs[i] + (evs[i] / 4)) * levelValue) / 100 + 5;

        // Apply nature modifier
        int modifier = getNatureModifier(i);
        stats[i] = (baseStat * modifier) / 100;
    }

    // Write calculated stats to party stat section
    writeUInt16LittleEndian(reinterpret_cast<uint8_t*>(data.data() + 0x14C), static_cast<uint16_t>(stats[0])); // ATK
    writeUInt16LittleEndian(reinterpret_cast<uint8_t*>(data.data() + 0x14E), static_cast<uint16_t>(stats[1])); // DEF
    writeUInt16LittleEndian(reinterpret_cast<uint8_t*>(data.data() + 0x150), static_cast<uint16_t>(stats[2])); // SPE
    writeUInt16LittleEndian(reinterpret_cast<uint8_t*>(data.data() + 0x152), static_cast<uint16_t>(stats[3])); // SPA
    writeUInt16LittleEndian(reinterpret_cast<uint8_t*>(data.data() + 0x154), static_cast<uint16_t>(stats[4])); // SPD
}

// ========================================
// PID Regeneration
// ========================================

void Pokemon9LZA::regeneratePID(uint32_t trainerID32) noexcept
{
    /**
     * Regenerates PID while maintaining gender and shininess.
     *
     * This is useful when IVs are modified
     * that may flag the Pokemon as invalid if PID doesn't match certain criteria.
     *
     * The algorithm:
     * 1. Save current gender and shiny status
     * 2. Generate a new PID based on Encryption Constant
     * 3. Adjust PID to restore shininess if needed
     * 4. Adjust PID to restore gender if needed
     */

    // Save current properties
    bool wasShiny = isShiny(trainerID32, species());
    uint8_t currentGender = gender();

    // Generate a base random PID using EC as seed
    uint32_t ec = encryptionConstant();
    uint32_t newPID = ec ^ 0x13371337; // Simple transform of EC

    // Adjust PID for shininess if needed
    if (wasShiny) {
        // Force shiny: adjust PID so (PID ^ TID32) results in XOR < 16
        uint32_t desiredXor = (newPID ^ trainerID32);
        uint32_t xorValue = (desiredXor ^ (desiredXor >> 16)) & 0xFFFF;

        if (xorValue >= 16) {
            // Adjust lower 16 bits to make it shiny
            uint32_t highXor = (newPID >> 16) ^ (trainerID32 >> 16);

            // Make XOR result < 16 (square shiny: XOR = 0, star shiny: XOR = 1-15)
            uint32_t targetXor = 1; // Star shiny
            uint32_t newLow = (highXor ^ targetXor) ^ (trainerID32 & 0xFFFF);
            newPID = (newPID & 0xFFFF0000) | (newLow & 0xFFFF);
        }
    } else {
        // Force non-shiny: ensure XOR >= 16
        uint32_t desiredXor = (newPID ^ trainerID32);
        uint32_t xorValue = (desiredXor ^ (desiredXor >> 16)) & 0xFFFF;

        if (xorValue < 16) {
            // Adjust to make non-shiny
            newPID ^= 0x10000000; // Flip a high bit to ensure XOR >= 16
        }
    }

    // Adjust for gender if needed (only for gendered species)
    if (currentGender < 2) {
        // Preserve gender by adjusting lower byte
        // Gender is typically determined by (PID & 0xFF) vs gender ratio
        // For 50/50 ratio: (PID & 0xFF) < 127 = male, >= 127 = female
        uint8_t pidGenderByte = newPID & 0xFF;
        bool pidWouldBeMale = pidGenderByte < 127;
        bool shouldBeMale = (currentGender == 0);

        if (pidWouldBeMale != shouldBeMale) {
            // Flip the gender by adjusting lower byte
            if (shouldBeMale) {
                newPID = (newPID & 0xFFFFFF00) | ((pidGenderByte - 64) & 0xFF);
            } else {
                newPID = (newPID & 0xFFFFFF00) | ((pidGenderByte + 64) & 0xFF);
            }
        }
    }

    // Write new PID
    writeUInt32LittleEndian(reinterpret_cast<uint8_t*>(data.data() + 0x1C), newPID);
}

// ========================================
// Shiny Manipulation
// ========================================

void Pokemon9LZA::setShiny(bool makeShiny, uint32_t trainerID32) noexcept
{
    /**
     * Sets the Pokemon's shiny status while maintaining gender.
     *
     * Shiny Pokemon have alternate coloration and are determined by the XOR
     * of their PID and the trainer's ID:
     * XOR = (PID_High ^ PID_Low ^ TID16 ^ SID16)
     * - Shiny if XOR < 16
     * - Square shiny if XOR = 0
     * - Star shiny if XOR = 1-15
     *
     * This function modifies the PID to achieve the desired shiny status
     * while preserving the Pokemon's gender.
     */

    bool currentlyShiny = isShiny(trainerID32, species());

    // If already in desired state, do nothing
    if (currentlyShiny == makeShiny) {
        return;
    }

    // Get current PID and preserve gender byte
    uint32_t currentPID = pid();
    uint8_t genderByte = currentPID & 0xFF;

    if (makeShiny) {
        /**
         * Make Pokemon shiny while preserving gender byte.
         *
         * Algorithm:
         * 1. Try different high word values
         * 2. For each high word, try different XOR results (0-15, all shiny)
         * 3. Find a combination where the low byte matches the gender byte
         * 4. Use that PID
         */

        uint32_t ec = encryptionConstant();
        uint32_t baseHighWord = (ec ^ 0x13371337) >> 16;
        uint32_t tidHigh = trainerID32 >> 16;
        uint32_t tidLow = trainerID32 & 0xFFFF;

        // Try different high words (start with base, then try variations)
        for (int attempt = 0; attempt < 256; attempt++) {
            uint32_t highWord = (baseHighWord + attempt) & 0xFFFF;
            uint32_t H = highWord ^ tidHigh;

            // Try xorResult values 0-15 (all shiny values, prefer 1 for star shiny)
            for (int targetXor = 1; targetXor < 16; targetXor++) {
                uint32_t L = H ^ targetXor;
                uint32_t lowWord = L ^ tidLow;

                // Check if this preserves gender byte
                if ((lowWord & 0xFF) == genderByte) {
                    // Found a match! Construct new PID
                    uint32_t newPID = (highWord << 16) | lowWord;

                    // Write new PID and refresh checksum
                    writeUInt32LittleEndian(reinterpret_cast<uint8_t*>(data.data() + 0x1C), newPID);
                    refreshChecksum();
                    return;
                }
            }

            // Also try targetXor = 0 (square shiny)
            uint32_t L = H ^ 0;
            uint32_t lowWord = L ^ tidLow;
            if ((lowWord & 0xFF) == genderByte) {
                uint32_t newPID = (highWord << 16) | lowWord;
                writeUInt32LittleEndian(reinterpret_cast<uint8_t*>(data.data() + 0x1C), newPID);
                refreshChecksum();
                return;
            }
        }

        // If we get here, couldn't find a match (extremely unlikely)
        // Fallback: just make shiny without preserving gender
        uint32_t highWord = baseHighWord & 0xFFFF;
        uint32_t H = highWord ^ tidHigh;
        uint32_t L = H ^ 1; // Star shiny
        uint32_t lowWord = L ^ tidLow;
        uint32_t newPID = (highWord << 16) | lowWord;
        writeUInt32LittleEndian(reinterpret_cast<uint8_t*>(data.data() + 0x1C), newPID);
        refreshChecksum();

    } else {
        /**
         * Make Pokemon non-shiny while preserving gender byte.
         *
         * Algorithm:
         * 1. Generate a new PID from EC
         * 2. Preserve the gender byte
         * 3. If still shiny, flip bits until XOR >= 16
         */

        uint32_t ec = encryptionConstant();
        uint32_t newPID = (ec ^ 0x13371337);

        // Preserve gender byte
        newPID = (newPID & 0xFFFFFF00) | genderByte;

        // Check if it's shiny
        uint32_t xorComponent = newPID ^ trainerID32;
        uint32_t xorResult = (xorComponent ^ (xorComponent >> 16)) & 0xFFFF;

        if (xorResult < 16) {
            // Need to make non-shiny - flip bit 8 to change XOR
            newPID ^= 0x00000100;

            // Verify it's now non-shiny
            xorComponent = newPID ^ trainerID32;
            xorResult = (xorComponent ^ (xorComponent >> 16)) & 0xFFFF;

            if (xorResult < 16) {
                // Still shiny, flip a high bit instead
                newPID ^= 0x00010000;
            }
        }

        // Write new PID and refresh checksum
        writeUInt32LittleEndian(reinterpret_cast<uint8_t*>(data.data() + 0x1C), newPID);
        refreshChecksum();
    }
}

// ========================================
// Utility Functions
// ========================================

std::string Pokemon9LZA::toHex(uint32_t value)
{
    /**
     * Converts a uint32_t value to a hexadecimal string.
     * Format: 0xXXXXXXXX (8 hex digits with 0x prefix)
     */
    char buf[16];
    snprintf(buf, sizeof(buf), "0x%08X", value);
    return std::string(buf);
}
