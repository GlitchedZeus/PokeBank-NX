#ifndef POKEBANK_GEN2_BATTLE_STATS_H
#define POKEBANK_GEN2_BATTLE_STATS_H

#include "Integration/Gen2/Gen2PersonalData.h"

#include <array>
#include <cstdint>

namespace PokeVault::Integration::Gen2 {

struct BattleStats {
    uint16_t hp = 0;
    uint16_t attack = 0;
    uint16_t defense = 0;
    uint16_t speed = 0;
    uint16_t specialAttack = 0;
    uint16_t specialDefense = 0;

    constexpr std::array<uint16_t,6> asArray() const noexcept {
        return {hp, attack, defense, speed, specialAttack, specialDefense};
    }
};

inline uint8_t statExperienceContribution(uint16_t value) noexcept {
    // G/S/C use the same Stat Experience square-root table behavior as R/B/Y:
    // smallest integer root whose square is >= the stored value, clamped to 255,
    // then integer-divided by four.
    uint16_t root = 0;
    while (root < 255 && static_cast<uint32_t>(root) * root < value) ++root;
    return static_cast<uint8_t>(root / 4);
}

inline uint8_t derivedHPDV(const std::array<uint8_t,4>& dvs) noexcept {
    return static_cast<uint8_t>(((dvs[0] & 1U) << 3U) | ((dvs[1] & 1U) << 2U) |
                                ((dvs[2] & 1U) << 1U) | (dvs[3] & 1U));
}

inline uint16_t calculatedStat(uint8_t base, uint8_t dv, uint16_t statExp,
                               uint8_t level, bool hp) noexcept {
    const uint32_t effort = statExperienceContribution(statExp);
    const uint32_t scaled = ((static_cast<uint32_t>(base + dv) * 2U + effort) * level) / 100U;
    return static_cast<uint16_t>(scaled + (hp ? static_cast<uint32_t>(level) + 10U : 5U));
}

inline BattleStats calculateBattleStats(
    uint16_t species, uint8_t level, const std::array<uint8_t,4>& dvs,
    const std::array<uint16_t,5>& statExperience) noexcept {
    const auto* personal = personalRecord(species);
    if (!personal || level < 1 || level > 100) return {};
    for (const auto dv : dvs)
        if (dv > 15) return {};

    const uint8_t hpDV = derivedHPDV(dvs);
    BattleStats out;
    out.hp = calculatedStat(personal->hp, hpDV, statExperience[0], level, true);
    out.attack = calculatedStat(personal->attack, dvs[0], statExperience[1], level, false);
    out.defense = calculatedStat(personal->defense, dvs[1], statExperience[2], level, false);
    out.speed = calculatedStat(personal->speed, dvs[2], statExperience[3], level, false);
    // Generation II stores one Special DV and one Special Stat Experience value, but its
    // personal table has separate Special Attack and Special Defense base stats.
    out.specialAttack = calculatedStat(personal->specialAttack, dvs[3], statExperience[4], level, false);
    out.specialDefense = calculatedStat(personal->specialDefense, dvs[3], statExperience[4], level, false);
    return out;
}

} // namespace PokeVault::Integration::Gen2

#endif
