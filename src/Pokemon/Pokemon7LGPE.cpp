/**
 * Pokemon7LGPE.cpp - Generation 7 Pokemon Let's Go Data Class Implementation
 *
 * Implementation of Pokemon7LGPE class methods including stat calculations,
 * gender determination, and Let's Go-specific features.
 */

#include <cstdio>
#include <algorithm>
#include <cmath>

#include "Pokemon/Pokemon7LGPE.h"
#include "Pokemon/BaseStats.h"

namespace Pokemon {
    // Forward declarations for external helper functions
    extern const char* getSpeciesName(uint16_t speciesId);

    // ========================================
    // Species and Name Lookups
    // ========================================

    const char* Pokemon7LGPE::species() const
    {
        return getSpeciesName(speciesID());
    }

    // ========================================
    // Gender Determination
    // ========================================

    uint8_t Pokemon7LGPE::gender() const
    {
        /**
         * Gender determination in Let's Go uses the encryption constant
         *
         * For simplicity, we use the same logic as PK8 but with EC instead of PID.
         */

        uint16_t species = speciesID();

        // Genderless species (simplified list)
        if (species == 81 || species == 82 ||     // Magnemite, Magneton
            species == 100 || species == 101 ||   // Voltorb, Electrode
            species == 120 || species == 121 ||   // Staryu, Starmie
            species == 132 ||                     // Ditto
            species == 137 ||                     // Porygon
            species == 144 || species == 145 || species == 146 || // Legendary birds
            species == 150 || species == 151) {   // Mewtwo, Mew
            return 2; // Genderless
        }

        // For most species, use lower byte of EC for gender determination
        uint32_t ec = encryptionConstant();
        return ((ec & 0xFF) < 127) ? 0 : 1;
    }

    // ========================================
    // Base Stats (Species-Dependent)
    // ========================================

    uint8_t Pokemon7LGPE::baseHP() const
    {
        return getBaseStats(speciesID())->hp;
    }

    uint8_t Pokemon7LGPE::baseATK() const
    {
        return getBaseStats(speciesID())->atk;
    }

    uint8_t Pokemon7LGPE::baseDEF() const
    {
        return getBaseStats(speciesID())->def;
    }

    uint8_t Pokemon7LGPE::baseSPE() const
    {
        return getBaseStats(speciesID())->spe;
    }

    uint8_t Pokemon7LGPE::baseSPA() const
    {
        return getBaseStats(speciesID())->spa;
    }

    uint8_t Pokemon7LGPE::baseSPD() const
    {
        return getBaseStats(speciesID())->spd;
    }

    // ========================================
    // Level Calculation
    // ========================================

    uint8_t Pokemon7LGPE::level() const
    {
        /**
         * In Let's Go, level can be calculated from experience using the
         * species' growth rate. For simplicity, we'll estimate it.
         *
         * A more accurate implementation would use growth rate tables.
         * For now, we'll use a simplified calculation.
         */

        uint32_t expValue = exp();

        // Simplified level calculation (assumes medium-fast growth rate)
        // This is approximate and should be replaced with proper growth rate tables
        if (expValue == 0) return 1;
        if (expValue >= 1000000) return 100;

        // Cube root approximation for medium-fast
        double level = std::cbrt(expValue);
        return static_cast<uint8_t>(std::min(100.0, std::max(1.0, level)));
    }

    // ========================================
    // Calculated Stats
    // ========================================

    uint16_t Pokemon7LGPE::statHPMax() const
    {
        /**
         * HP calculation in Let's Go includes Awakening Values (AVs):
         * HP = ((2 * Base + IV + EV/4) * Level / 100) + Level + 10 + AV
         *
         * Friendship bonus is NOT applied to HP.
         */

        uint8_t levelValue = level();
        if (levelValue == 0 || levelValue > 100) return 1;

        int hp = ((2 * baseHP() + ivHP() + (evHP() / 4)) * levelValue) / 100 + levelValue + 10;
        hp += avHP(); // Add Awakening Value

        return static_cast<uint16_t>(std::max(1, hp));
    }

    uint16_t Pokemon7LGPE::statATK() const
    {
        uint8_t levelValue = level();
        if (levelValue == 0 || levelValue > 100) return 1;

        // Base calculation with Awakening Value
        int stat = ((2 * baseATK() + ivATK() + (evATK() / 4)) * levelValue) / 100 + 5;
        stat += avATK(); // Add Awakening Value

        // Apply nature modifier (simplified - assumes no nature effect for now)
        // Full implementation would check nature table

        // Apply friendship bonus (simplified - assumes 100% friendship = +10%)
        int friendshipValue = friendship();
        double friendshipBonus = 1.0 + (friendshipValue / 255.0 / 10.0);
        stat = static_cast<int>(stat * friendshipBonus);

        return static_cast<uint16_t>(std::max(1, stat));
    }

    uint16_t Pokemon7LGPE::statDEF() const
    {
        uint8_t levelValue = level();
        if (levelValue == 0 || levelValue > 100) return 1;

        int stat = ((2 * baseDEF() + ivDEF() + (evDEF() / 4)) * levelValue) / 100 + 5;
        stat += avDEF();

        int friendshipValue = friendship();
        double friendshipBonus = 1.0 + (friendshipValue / 255.0 / 10.0);
        stat = static_cast<int>(stat * friendshipBonus);

        return static_cast<uint16_t>(std::max(1, stat));
    }

    uint16_t Pokemon7LGPE::statSPE() const
    {
        uint8_t levelValue = level();
        if (levelValue == 0 || levelValue > 100) return 1;

        int stat = ((2 * baseSPE() + ivSPE() + (evSPE() / 4)) * levelValue) / 100 + 5;
        stat += avSPE();

        int friendshipValue = friendship();
        double friendshipBonus = 1.0 + (friendshipValue / 255.0 / 10.0);
        stat = static_cast<int>(stat * friendshipBonus);

        return static_cast<uint16_t>(std::max(1, stat));
    }

    uint16_t Pokemon7LGPE::statSPA() const
    {
        uint8_t levelValue = level();
        if (levelValue == 0 || levelValue > 100) return 1;

        int stat = ((2 * baseSPA() + ivSPA() + (evSPA() / 4)) * levelValue) / 100 + 5;
        stat += avSPA();

        int friendshipValue = friendship();
        double friendshipBonus = 1.0 + (friendshipValue / 255.0 / 10.0);
        stat = static_cast<int>(stat * friendshipBonus);

        return static_cast<uint16_t>(std::max(1, stat));
    }

    uint16_t Pokemon7LGPE::statSPD() const
    {
        uint8_t levelValue = level();
        if (levelValue == 0 || levelValue > 100) return 1;

        int stat = ((2 * baseSPD() + ivSPD() + (evSPD() / 4)) * levelValue) / 100 + 5;
        stat += avSPD();

        int friendshipValue = friendship();
        double friendshipBonus = 1.0 + (friendshipValue / 255.0 / 10.0);
        stat = static_cast<int>(stat * friendshipBonus);

        return static_cast<uint16_t>(std::max(1, stat));
    }

    // ========================================
    // Stat Recalculation
    // ========================================

    void Pokemon7LGPE::recalculateStats()
    {
        /**
         * In Let's Go, stats are calculated and stored differently than other generations.
         * The stat calculation includes:
         * 1. Base stats (species-dependent)
         * 2. IVs (0-31)
         * 3. EVs (0-252 per stat, max 510 total)
         * 4. Awakening Values (AVs) - unique to Let's Go! (0-200 per stat)
         * 5. Nature modifiers
         * 6. Friendship bonus (up to +10%)
         *
         * For now, we implement a simplified version.
         * A full implementation would match the exact Let's Go formula and
         * store calculated stats in the appropriate locations.
         *
         * Note: Let's Go doesn't have a separate "party stats" section like Gen 8,
         * so recalculation is less critical for save file integrity.
         */

        // Stats are calculated on-the-fly in Let's Go
        // No need to store them in specific offsets like Gen 8
        // The CP (Combat Power) value should be recalculated, but we'll skip that for now
    }

    // ========================================
    // Advanced Modification
    // ========================================

    void Pokemon7LGPE::regeneratePID(uint32_t trainerID32)
    {
        /**
         * Regenerates the Encryption Constant to maintain legality.
         * In Let's Go (Pokemon7LGPE), the EC serves as the PID for shiny/gender calculations.
         *
         * This generates a new EC while preserving:
         * - Current shiny status
         */

        bool wasShiny = isShiny(trainerID32, species());

        // Generate new EC from current values
        uint32_t oldEC = encryptionConstant();
        uint32_t newEC = (oldEC ^ 0x10101010) + speciesID();

        // Write the new EC
        writeUInt32LittleEndian(reinterpret_cast<uint8_t*>(data.data() + 0x00), newEC);

        // Restore shiny status if it was shiny
        if (wasShiny) {
            setShiny(true, trainerID32);
        }

        refreshChecksum();
    }

    void Pokemon7LGPE::setShiny(bool makeShiny, uint32_t trainerID32)
    {
        /**
         * Sets the Pokemon's shiny status by modifying the Encryption Constant.
         * In Let's Go, EC is used for shiny calculations (similar to PID).
         *
         * Algorithm:
         * - For shiny: XOR of (EC ^ trainerID32) must be < 16
         * - For non-shiny: XOR must be >= 16
         */

        if (trainerID32 == 0) {
            return; // Cannot set shiny without trainer ID
        }

        uint32_t ec = encryptionConstant();
        uint16_t tidHigh = (trainerID32 >> 16) & 0xFFFF;
        uint16_t tidLow = trainerID32 & 0xFFFF;

        if (makeShiny) {
            /**
             * Make Pokemon shiny.
             * Set EC so that XOR < 16 (star shiny with XOR = 1)
             */

            // Keep high word of EC, calculate new low word for shiny
            uint32_t ecHigh = (ec >> 16) & 0xFFFF;
            uint32_t H = ecHigh ^ tidHigh;
            uint32_t L = H ^ 1; // XOR = 1 for star shiny
            uint32_t ecLow = L ^ tidLow;

            uint32_t newEC = (ecHigh << 16) | ecLow;
            writeUInt32LittleEndian(reinterpret_cast<uint8_t*>(data.data() + 0x00), newEC);
            refreshChecksum();

        } else {
            /**
             * Make Pokemon non-shiny.
             * Set EC so that XOR >= 16
             */

            uint32_t newEC = ec ^ 0x10001000;

            // Verify it's non-shiny
            uint32_t xorComponent = newEC ^ trainerID32;
            uint32_t xorResult = (xorComponent ^ (xorComponent >> 16)) & 0xFFFF;

            if (xorResult < 16) {
                // Still shiny, flip more bits
                newEC ^= 0x01000100;
            }

            writeUInt32LittleEndian(reinterpret_cast<uint8_t*>(data.data() + 0x00), newEC);
            refreshChecksum();
        }
    }
}
