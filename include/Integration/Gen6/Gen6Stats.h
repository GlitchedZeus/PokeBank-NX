#ifndef POKEBANK_GEN6_STATS_H
#define POKEBANK_GEN6_STATS_H

#include "Integration/Gen6/Gen6PK6.h"
#include "Integration/Gen6/Gen6PersonalData.h"
#include "Pokemon/Experience.h"

#include <array>
#include <cstdint>
#include <optional>

// Gen VI level and battle-stat derivation, used to keep party records' cached
// stats consistent with staged IV/EV/Nature/Level edits. Sources, pinned:
//   PKHeX 6501f0ab  PKM/PKM.cs LoadStats (HP: base HP 1 => 1, else
//                   (IV+2B+EV/4+100)*L/100+10; others (IV+2B+EV/4)*L/100+5),
//                   CurrentLevel = Experience.GetLevel(EXP, EXPGrowth), setter
//                   EXP = Experience.GetEXP(level, growth); PK6.PersonalInfo uses
//                   PersonalTable.AO; PersonalInfo.FormIndex/HasForm.
//   PKSM-Core aa22d7a4 source/pkx/PK6.cpp statImpl (nature/5 raises, nature%5 lowers
//                   Atk/Def/Spe/SpA/SpD by 10%) and updatePartyData (stats, level,
//                   current HP = max HP; status untouched).
// EXP tables: shared Pokemon::getLevelFromExp/getExpForLevel (verbatim PKHeX Experience.cs).
namespace PokeVault::Integration::Gen6::Stats {

inline constexpr uint16_t MaxSpecies = 721;

// Base-stat row for species/form, or nullopt when the species is outside Gen VI
// or the form is outside the species' form count (never silently the base row).
[[nodiscard]] constexpr std::optional<Personal6> personal(uint16_t species, uint8_t form) noexcept {
    if (species == 0 || species > MaxSpecies) return std::nullopt;
    const auto& base = PersonalAO[species];
    const uint8_t count = base.formCount == 0 ? 1 : base.formCount;
    if (form >= count) return std::nullopt;
    if (form == 0 || base.formStatsIndex == 0) return base;
    const size_t index = size_t{base.formStatsIndex} + form - 1;
    if (index >= PersonalAO.size()) return std::nullopt;
    return PersonalAO[index];
}

[[nodiscard]] inline std::optional<uint8_t> level(const PK6::Pokemon6& mon) noexcept {
    const auto p = personal(mon.species(), mon.form());
    if (!p) return std::nullopt;
    return Pokemon::getLevelFromExp(mon.experience(), p->growthRate);
}

[[nodiscard]] inline std::optional<uint32_t> experienceForLevel(const PK6::Pokemon6& mon, uint8_t lvl) noexcept {
    const auto p = personal(mon.species(), mon.form());
    if (!p || lvl < 1 || lvl > 100) return std::nullopt;
    return Pokemon::getExpForLevel(lvl, p->growthRate);
}

// Battle stats in PK6 storage order: HP, Atk, Def, Spe, SpA, SpD.
[[nodiscard]] inline std::optional<std::array<uint16_t, 6>> compute(const PK6::Pokemon6& mon) noexcept {
    const auto p = personal(mon.species(), mon.form());
    if (!p) return std::nullopt;
    const uint32_t lvl = Pokemon::getLevelFromExp(mon.experience(), p->growthRate);
    const std::array<uint32_t, 6> base = {p->hp, p->atk, p->def, p->spe, p->spa, p->spd};
    std::array<uint16_t, 6> out{};
    for (size_t i = 0; i < 6; ++i) {
        const uint32_t core = mon.iv(i) + 2 * base[i] + mon.ev(i) / 4;
        if (i == 0) out[0] = static_cast<uint16_t>(base[0] == 1 ? 1 : (core + 100) * lvl / 100 + 10);
        else out[i] = static_cast<uint16_t>(core * lvl / 100 + 5);
    }
    // Nature amplification over Atk, Def, Spe, SpA, SpD (stat indexes 1..5).
    const uint8_t nature = mon.nature();
    if (nature < 25) {
        const size_t up = nature / 5 + 1, down = nature % 5 + 1;
        if (up != down) {
            out[up] = static_cast<uint16_t>(out[up] * 11 / 10);
            out[down] = static_cast<uint16_t>(out[down] * 9 / 10);
        }
    }
    return out;
}

// Rewrites a party record's cached level and stats; current HP becomes max HP.
[[nodiscard]] inline bool refreshParty(PK6::Pokemon6& mon) noexcept {
    if (!mon.partyForm()) return false;
    const auto stats = compute(mon);
    const auto lvl = level(mon);
    if (!stats || !lvl) return false;
    mon.setPartyStats(*lvl, (*stats)[0], *stats);
    return true;
}

} // namespace PokeVault::Integration::Gen6::Stats

#endif
