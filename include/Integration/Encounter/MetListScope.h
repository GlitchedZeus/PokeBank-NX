#pragma once
// Round 14 (owner Round 13 R06 Gen V: "all pokemon seem the have the same met location : mystery zone,
// link trade, PKMN Breedeer, other location"; H05 "pikachu has werid met locations"; shared-UX audit
// P1-9: one met-location source and ordering for Crystal and Gen III-V).
//
// Measured on the Round 13 base (tests/test_met_list_scope.cpp): the Gen V encounter data is wired and
// matches PKHeX (Black 231, White 264, Black 2 / White 2 286 wild species; PKHeX 6042efa2
// encounter_*.pkl), but a NEW Pokemon is created at Lv. 5 and almost no Unova encounter is that low:
// at Lv. 5 only 23 of 649 species have a verified Black row (Lv. 30: 193; Lv. 50: 310), so for ~96%
// of species the DEFAULT list held nothing but the stored row, the markers and "Other location
// (unverified)...", and its caption only said "No verified encounter here". The level rule is right
// (R12-005: a Pokemon is never below the level it was met at), so the rows stay unverified; what was
// wrong is that the list did not say WHY, and the unverified list buried those rows among ~270 ids.
//
// This header is the one caption and order every Met Location picker uses:
//   * DEFAULT caption: verified rows -> the policy's scope; none, but rows exist at a higher level ->
//     "No catch at Lv. 5; 6 need Lv. 12+ (unverified list)"; none at any level -> "Not in this
//     game's wild / gift data; markers only" (Black Pikachu: PKHeX has no Black / White / Black 2 /
//     White 2 wild, gift, static or trade Pikachu or Pichu).
//   * UNVERIFIED list (Gen IV / V EncounterContext45::Catalogue): row 1 stays the stored location,
//     then the "level too low" places, lowest level first, then every other native place -- the order
//     the Gen III (Gen3EncounterPickerModel::unverifiedRows) and Crystal (Gen2PokemonPickerModel
//     levelBlocked) lists already use.
// Nothing here verifies or changes a Pokemon; it only words and orders the rows the policy builds.
#include "Integration/Encounter/MetLocationPolicy.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace PokeVault::Integration::MetListScope {

inline constexpr const char* kNotInGameData = "Not in this game's wild / gift data; markers only";

// The places whose encounter the Pokemon's level has not reached (R12-005), and the lowest such level.
struct LevelGap {
    uint8_t level = 0;                 // the Pokemon's current level (0 = not judged)
    uint8_t lowest = 0;                // lowest minimum level among those places
    std::vector<uint16_t> places;      // distinct locations
    void add(uint16_t location, uint8_t minLevel) {
        if (std::find(places.begin(), places.end(), location) == places.end()) places.push_back(location);
        if (lowest == 0 || minLevel < lowest) lowest = minLevel;
    }
    [[nodiscard]] std::size_t count() const noexcept { return places.size(); }
};

// The DEFAULT list's caption. `verified` = the list's verified encounter rows (markers and the stored
// row excluded); `verifiedScope` the caption a list with verified rows keeps.
[[nodiscard]] inline std::string defaultScope(std::size_t verified, const LevelGap& gap,
                                              const char* verifiedScope = MetLocationPolicy::kDefaultListScope) {
    // Short enough for the 425 px caption room at its widest ("12 ... Lv. 100+", tests/test_special_met_location_notes.cpp).
    const std::string need = gap.count() == 1 ? " needs Lv. " : " need Lv. ";
    if (verified > 0)
        return gap.count() == 0 ? std::string(verifiedScope)
                                : "Verified + markers; " + std::to_string(gap.count()) + " more" + need + std::to_string(gap.lowest) + "+";
    if (gap.count() > 0)
        return "No catch at Lv. " + std::to_string(gap.level) + "; " + std::to_string(gap.count()) + need +
               std::to_string(gap.lowest) + "+ (unverified list)";
    return kNotInGameData;
}

// ---- Gen IV / V (EncounterContext45::Catalogue; a template so this header stays dependency-free) --
template <class Catalogue>
[[nodiscard]] inline LevelGap levelGap(const Catalogue& context) {
    LevelGap gap;
    gap.level = context.currentLevel();
    for (const auto id : context.choices(true))
        if (const auto* row = context.wildRow(id); row && !row->levelReached) gap.add(id, row->encounter.minLevel);
    return gap;
}
template <class Catalogue>
[[nodiscard]] inline std::string defaultScope(const Catalogue& context) {
    return defaultScope(context.verifiedCount(), levelGap(context));
}
// The UNVERIFIED list in the shared order: row 1 (the stored location), the "level too low" places
// (lowest level first), then every other place in the catalogue's native order.
template <class Catalogue>
[[nodiscard]] inline std::vector<uint16_t> unverifiedChoices(const Catalogue& context) {
    const auto& all = context.choices(true);
    if (all.empty()) return all;
    std::vector<uint16_t> blocked, rest;
    for (std::size_t i = 1; i < all.size(); ++i) {
        const auto* row = context.wildRow(all[i]);
        (row && !row->levelReached ? blocked : rest).push_back(all[i]);
    }
    std::stable_sort(blocked.begin(), blocked.end(), [&](uint16_t a, uint16_t b) {
        return context.wildRow(a)->encounter.minLevel < context.wildRow(b)->encounter.minLevel;
    });
    std::vector<uint16_t> out{all.front()};
    out.insert(out.end(), blocked.begin(), blocked.end());
    out.insert(out.end(), rest.begin(), rest.end());
    return out;
}

} // namespace PokeVault::Integration::MetListScope
