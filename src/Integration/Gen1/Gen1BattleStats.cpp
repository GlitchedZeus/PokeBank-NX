#include "Integration/Gen1/Gen1StagedPokemonEditor.h"

#include <array>
#include <cstdint>

namespace PokeVault::Integration::Gen1 {
namespace {
#include "Gen1PersonalData.inc"

uint8_t statExperienceContribution(uint16_t value) noexcept {
    // R/B/Y look up the smallest integer whose square is >= Stat Exp, clamp that
    // root to the game's byte table range, then divide by four using integer math.
    uint16_t root = 0;
    while (root < 255 && static_cast<uint32_t>(root) * root < value) ++root;
    return static_cast<uint8_t>(root / 4);
}

uint16_t calculatedStat(uint8_t base, uint8_t dv, uint16_t statExp,
                        uint8_t level, bool hp) noexcept {
    const uint32_t effort = statExperienceContribution(statExp);
    const uint32_t scaled = ((static_cast<uint32_t>(base + dv) * 2U + effort) * level) / 100U;
    return static_cast<uint16_t>(scaled + (hp ? static_cast<uint32_t>(level) + 10U : 5U));
}
} // namespace

std::array<uint8_t,2> StagedPokemonEditor::personalTypes(uint16_t species) noexcept {
    if (species < 1 || species > 151) return {0, 0};
    return {kPersonal[species][5], kPersonal[species][6]};
}

BattleStats StagedPokemonEditor::calculateBattleStats(
    uint16_t species, uint8_t level, const std::array<uint8_t,4>& dvs,
    const std::array<uint16_t,5>& statExperience) noexcept {
    if (species < 1 || species > 151 || level < 1 || level > 100)
        return {};
    for (const auto dv : dvs)
        if (dv > 15) return {};

    const std::array<uint8_t,5> effectiveDVs{
        derivedHPDV(dvs), dvs[0], dvs[1], dvs[2], dvs[3],
    };

    BattleStats out;
    out.hp      = calculatedStat(kPersonal[species][0], effectiveDVs[0], statExperience[0], level, true);
    out.attack  = calculatedStat(kPersonal[species][1], effectiveDVs[1], statExperience[1], level, false);
    out.defense = calculatedStat(kPersonal[species][2], effectiveDVs[2], statExperience[2], level, false);
    out.speed   = calculatedStat(kPersonal[species][3], effectiveDVs[3], statExperience[3], level, false);
    out.special = calculatedStat(kPersonal[species][4], effectiveDVs[4], statExperience[4], level, false);
    return out;
}

} // namespace PokeVault::Integration::Gen1