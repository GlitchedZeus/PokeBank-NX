#ifndef POKEBANK_GEN7_STATS_H
#define POKEBANK_GEN7_STATS_H

#include "Integration/Gen7/Gen7PersonalData.h"
#include "Integration/Gen7/Gen7Save.h"
#include "Pokemon/Experience.h"

#include <array>
#include <cstdint>
#include <optional>

// Gen VII level and battle stats (hyper-training aware), used to keep party PK7
// cached stats consistent with staged IV/EV/Nature/Level edits. Sources, pinned:
//   PKHeX 6501f0ab  PKM/PKM.cs LoadStats(IHyperTrain): a hyper-trained stat uses IV 31;
//                   base HP 1 => HP 1; PK7.PersonalInfo = PersonalTable.USUM;
//                   PK7.cs HyperTrainFlags 0xDE bits HP0 Atk1 Def2 SpA3 SpD4 Spe5.
//   PKSM-Core aa22d7a4 source/pkx/PK7.cpp statImpl / updatePartyData (nature x1.1/x0.9,
//                   current HP = max HP; status untouched).
namespace PokeVault::Integration::Gen7::Stats {

inline constexpr uint16_t MaxSpecies = 807;  // USUM (Zeraora); PKSM-Core maxSpecies(US/UM)

[[nodiscard]] constexpr std::optional<Personal7> personal(uint16_t species, uint8_t form) noexcept {
    if (species == 0 || species > MaxSpecies) return std::nullopt;
    const auto& base = PersonalUSUM[species];
    const uint8_t count = base.formCount == 0 ? 1 : base.formCount;
    if (form >= count) return std::nullopt;
    if (form == 0 || base.formStatsIndex == 0) return base;
    const size_t index = size_t{base.formStatsIndex} + form - 1;
    if (index >= PersonalUSUM.size()) return std::nullopt;
    return PersonalUSUM[index];
}

// Storage order HP, Atk, Def, Spe, SpA, SpD -> HyperTrainFlags bit.
inline constexpr std::array<uint8_t, 6> HyperTrainBit = {0, 1, 2, 5, 3, 4};
[[nodiscard]] inline bool hyperTrained(const PK7::Pokemon6& mon, size_t stat) noexcept {
    return stat < 6 && ((mon.plain()[0xDE] >> HyperTrainBit[stat]) & 1) != 0;
}

[[nodiscard]] inline std::optional<uint8_t> level(const PK7::Pokemon6& mon) noexcept {
    const auto p = personal(mon.species(), mon.form());
    if (!p) return std::nullopt;
    return Pokemon::getLevelFromExp(mon.experience(), p->growthRate);
}

[[nodiscard]] inline std::optional<uint32_t> experienceForLevel(const PK7::Pokemon6& mon, uint8_t lvl) noexcept {
    const auto p = personal(mon.species(), mon.form());
    if (!p || lvl < 1 || lvl > 100) return std::nullopt;
    return Pokemon::getExpForLevel(lvl, p->growthRate);
}

[[nodiscard]] inline std::optional<std::array<uint16_t, 6>> compute(const PK7::Pokemon6& mon) noexcept {
    const auto p = personal(mon.species(), mon.form());
    if (!p) return std::nullopt;
    const uint32_t lvl = Pokemon::getLevelFromExp(mon.experience(), p->growthRate);
    const std::array<uint32_t, 6> base = {p->hp, p->atk, p->def, p->spe, p->spa, p->spd};
    std::array<uint16_t, 6> out{};
    for (size_t i = 0; i < 6; ++i) {
        const uint32_t iv = hyperTrained(mon, i) ? 31u : mon.iv(i);
        const uint32_t core = iv + 2 * base[i] + mon.ev(i) / 4;
        if (i == 0) out[0] = static_cast<uint16_t>(base[0] == 1 ? 1 : (core + 100) * lvl / 100 + 10);
        else out[i] = static_cast<uint16_t>(core * lvl / 100 + 5);
    }
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

[[nodiscard]] inline bool refreshParty(PK7::Pokemon6& mon) noexcept {
    if (!mon.partyForm()) return false;
    const auto stats = compute(mon);
    const auto lvl = level(mon);
    if (!stats || !lvl) return false;
    mon.setPartyStats(*lvl, (*stats)[0], *stats);
    return true;
}

} // namespace PokeVault::Integration::Gen7::Stats

#endif
