#pragma once

// Round 13 owner decision 9 (Nov 2): "Existing Pokemon retain their current level on species
// change. Reconcile EXP correctly." This is the ONE rule every Gen I-V species-change path for an
// EXISTING Pokemon uses (Gen I / II staged editors, the Gen III session and staged editor, Gen IV
// Pokemon4Mutable::setSpecies, the Gen V staged PK5 Species transaction). A Create draft is not an
// existing Pokemon: it keeps its own default-level rule (UI/SpeciesChangeLevelPolicy.h).
//
// The rule, for the level L the Pokemon shows now (clamped to 1..100) and the NEW species' growth
// rate:
//   - the level stays L;
//   - the current EXP is kept unchanged when it already lies in L's range for the new growth rate
//     (expForLevel(L) <= EXP < expForLevel(L + 1); at L 100 the range is exactly the maximum), so a
//     change between species of the same growth rate never touches EXP;
//   - otherwise EXP becomes the MINIMUM EXP for L under the new growth rate (L 100: the exact
//     maximum). It never exceeds L's range, and the level read back from it is L.
//   - between species of the SAME growth rate nothing changes at all (sameGrowthRate overload): the
//     EXP and the level it reads as are exactly what they were, and a stored level byte that
//     already disagreed with EXP is left as found, never "repaired" by a species change.
// The minimum (rather than a carried within-level fraction) is the value the game itself writes for
// a Pokemon at exactly L, so it is reproducible and never rounds into L + 1.
//
// Header-only and table-agnostic: the caller passes its generation's own EXP-for-level function
// (Pokemon::getExpForLevel for Gen I-IV, Gen5::experienceForLevel5 for Gen V) for the new species.

#include <cstdint>
#include <string>

namespace Pokemon::SpeciesChangeExperience {

struct Result {
    uint8_t level = 1;
    uint32_t experience = 0;
    bool experienceChanged = false;
};

// expForLevel(level) -> minimum total EXP for `level` (1..100) under the NEW species' growth rate.
template <class ExpForLevel>
constexpr Result keepLevel(uint8_t currentLevel, uint32_t currentExperience,
                           ExpForLevel&& expForLevel) noexcept {
    const uint8_t level = currentLevel < 1 ? 1 : currentLevel > 100 ? 100 : currentLevel;
    const uint32_t minimum = static_cast<uint32_t>(expForLevel(level));
    const bool fits = level == 100
        ? currentExperience == minimum
        : currentExperience >= minimum &&
              currentExperience < static_cast<uint32_t>(expForLevel(static_cast<uint8_t>(level + 1)));
    Result result;
    result.level = level;
    result.experience = fits ? currentExperience : minimum;
    result.experienceChanged = result.experience != currentExperience;
    return result;
}

// The entry point every species change uses: `sameGrowthRate` is (old growth rate == new growth
// rate); then EXP is untouched (experienceChanged false). Otherwise keepLevel above.
template <class ExpForLevel>
constexpr Result keepLevel(uint8_t currentLevel, uint32_t currentExperience, bool sameGrowthRate,
                           ExpForLevel&& expForLevel) noexcept {
    if (sameGrowthRate) {
        Result unchanged;
        unchanged.level = currentLevel < 1 ? 1 : currentLevel > 100 ? 100 : currentLevel;
        unchanged.experience = currentExperience;
        return unchanged;
    }
    return keepLevel(currentLevel, currentExperience, expForLevel);
}

// The note an editor status / Review shows when the rule changed EXP ("" when it did not):
// "Kept Lv. 50; EXP 130000 -> 156250 for the new species' growth rate".
inline std::string note(uint32_t beforeExperience, const Result& result) {
    if (!result.experienceChanged) return {};
    return "Kept Lv. " + std::to_string(result.level) + "; EXP " + std::to_string(beforeExperience) +
           " -> " + std::to_string(result.experience) + " for the new species' growth rate";
}

} // namespace Pokemon::SpeciesChangeExperience
