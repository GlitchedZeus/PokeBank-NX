#pragma once

#include "Integration/Encounter/EncounterGuardrails.h"
#include "Names/LocationDisplayNames.h"
#include "Names/SpeciesNames.h"
#include "UI/ExactGameMetLocations.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace PokeBank::UIModel::Gen3EncounterPicker {

namespace Encounter = PokeVault::Integration::EncounterGuardrails;

// R2-BUG-008: the Met Location picker lists the exact-game encounter templates. A record whose
// stored met location is not one of them (traded, hatched, event, or any origin the template
// data does not cover) must not be shown as if a template were its origin: its stored location is
// offered first as "keep stored", and choosing it changes nothing. Template rows stay choosable,
// but one whose level range does not contain the stored met level says so rather than reading as
// this record's acquisition.
inline constexpr uint16_t kKeepStoredRow = 0xFFFF;
namespace MetRows = PokeBank::UIModel::ExactGameMetLocations;
namespace Policy = PokeVault::Integration::MetLocationPolicy;
static_assert(kKeepStoredRow == MetRows::kKeepStoredRowValue);
// Round 12 (R12-004): the default list's last row, "Other location (unverified)...".
inline constexpr uint16_t kUnverifiedListRow = MetRows::kUnverifiedListRowValue;

inline std::vector<uint16_t> rowValues(std::size_t count) {
    std::vector<uint16_t> rows;
    rows.reserve(count);
    for (std::size_t i = 0; i < count; ++i)
        rows.push_back(static_cast<uint16_t>(i));
    return rows;
}

template <typename Choice>
inline const Encounter::EncounterTemplate& encounterOf(const Choice& choice) noexcept {
    if constexpr (requires { choice.encounter; }) return choice.encounter;
    else return choice;
}

template <typename Choice>
inline bool storedLocationListed(const std::vector<Choice>& choices, uint16_t storedLocation) noexcept {
    for (const auto& choice : choices)
        if (encounterOf(choice).location == storedLocation) return true;
    return false;
}

// Picker rows: kKeepStoredRow first when the stored location is set and not a template, then one
// row per template (its index into `choices`).
template <typename Choice>
inline std::vector<uint16_t> rowValuesWithStored(const std::vector<Choice>& choices, uint16_t storedLocation) {
    std::vector<uint16_t> rows;
    rows.reserve(choices.size() + 1);
    if (storedLocation != 0 && !storedLocationListed(choices, storedLocation)) rows.push_back(kKeepStoredRow);
    for (std::size_t i = 0; i < choices.size(); ++i) rows.push_back(static_cast<uint16_t>(i));
    return rows;
}

// A template matches the record's stored met level when that level is inside its range. A stored
// met level of 0 (hatched / not recorded) is not judged.
inline bool matchesStoredMetLevel(const Encounter::EncounterTemplate& encounter, uint8_t storedMetLevel) noexcept {
    return storedMetLevel == 0 || encounter.containsLevel(storedMetLevel);
}

// Initial focus: the keep-stored row when present, otherwise the template the record matches.
template <typename Choice>
inline int selectedRowWithStored(const std::vector<Choice>& choices, const std::vector<uint16_t>& rows,
                                 uint16_t currentLocation, uint8_t currentLevel) noexcept;

template <typename Choice>
inline int selectedRow(const std::vector<Choice>& choices,
                       uint16_t currentLocation, uint8_t currentLevel) noexcept {
    int locationFallback = -1;
    for (int i = 0; i < static_cast<int>(choices.size()); ++i) {
        const auto& encounter = encounterOf(choices[static_cast<std::size_t>(i)]);
        if (encounter.location != currentLocation) continue;
        if (locationFallback < 0) locationFallback = i;
        if (currentLevel == 0 || encounter.containsLevel(currentLevel)) return i;
    }
    return locationFallback >= 0 ? locationFallback : 0;
}

template <typename Choice>
inline int selectedRowWithStored(const std::vector<Choice>& choices, const std::vector<uint16_t>& rows,
                                 uint16_t currentLocation, uint8_t currentLevel) noexcept {
    if (!rows.empty() && rows.front() == kKeepStoredRow) return 0;
    return selectedRow(choices, currentLocation, currentLevel);
}

template <typename Choice>
inline const Choice* choiceAt(const std::vector<Choice>& choices, uint16_t rowValue) noexcept {
    return rowValue < choices.size() ? &choices[static_cast<std::size_t>(rowValue)] : nullptr;
}

// Round 11 FRLG: the catch templates come from the RECORD's origin game (PK3 origin 1-5), not the
// save being edited: a Ruby-origin Pokémon in a FireRed save is offered Hoenn data under its Ruby
// origin, and a FireRed / LeafGreen record is offered Kanto data. Colosseum / XD and unknown origins
// have no Gen III GBA template list.
// Round 12 (owner H08 "evolved"): the species' own templates and those of every pre-evolution
// (ExactGameMetLocations::templatesWithAncestry), so an evolved Raichu is offered Pikachu's catch.
inline std::vector<Encounter::EncounterProvenanceChoice> templatesForOrigin(uint8_t origin, uint16_t species) {
    return MetRows::templatesWithAncestry(MetRows::gen3GameIdForOrigin(origin), species, 386);
}

// R12-005: a template is a verified encounter for a Pokémon only once its level has reached the
// template's minimum (a Pokémon is never below the level it was met at; evolving keeps the level).
template <typename Choice>
inline bool levelReached(const Choice& choice, uint8_t currentLevel) noexcept {
    return Policy::levelReachesEncounter(encounterOf(choice).minLevel, currentLevel);
}

template <typename Choice>
inline bool reachedTemplateAt(const std::vector<Choice>& choices, uint16_t location, uint8_t currentLevel) noexcept {
    for (const auto& choice : choices)
        if (encounterOf(choice).location == location && levelReached(choice, currentLevel)) return true;
    return false;
}

// The record's stored Met Location / Met Level is one verified template of the species (or its
// ancestry) that its current level reaches; nullptr for any other history (marker, unverified
// place, a level the template does not cover).
template <typename Choice>
inline const Choice* verifiedTemplate(const std::vector<Choice>& choices, uint16_t location, uint8_t metLevel,
                                      uint8_t currentLevel) noexcept {
    for (const auto& choice : choices) {
        const auto& encounter = encounterOf(choice);
        if (encounter.location == location && encounter.containsLevel(metLevel) && levelReached(choice, currentLevel))
            return &choice;
    }
    return nullptr;
}

// Round 12 (R12-004 / R12-005, Integration/Encounter/MetLocationPolicy.h). DEFAULT list: the
// keep-stored row (stored location set and not a verified row here), the templates the current
// level reaches, the gift egg / in-game trade / fateful markers (labelled markers), then the one
// "Other location (unverified)..." row. Never empty.
// Round 13 (owner decision 5): the "Hatched from a Day-Care egg" row (kHatchedEggRow). Gen III eggs
// hatch at Lv. 5; a hatched Pokémon's Met Location is where it hatched and its Met Level is 0
// (PKHeX EncounterEgg3 / EncounterVerifier.VerifyEncounterEgg3). The row writes the origin game's
// Day-Care hatch place (MetLocationPolicy::dayCareHatchLocation) and Met Level 0.
inline constexpr uint16_t kHatchedEggRow = MetRows::kHatchedEggRowValue;
inline PokeVault::Integration::BreedingPolicy::EggVerdict eggVerdict(uint16_t species, uint8_t currentLevel) noexcept {
    return PokeVault::Integration::BreedingPolicy::eggHistory(3, species, currentLevel);
}

// Round 13: the default list offers the hatched-egg row only when BreedingPolicy verifies the
// species (or a pre-evolution) hatches from a Day-Care egg in Gen III at this level (`species` 0:
// no hatched-egg row), and only the trade / fateful markers (the gift-egg marker 253 is unverified).
template <typename Choice>
inline std::vector<uint16_t> defaultRows(const std::vector<Choice>& choices, uint16_t storedLocation,
                                         uint8_t origin, uint8_t currentLevel, uint16_t species = 0) {
    std::vector<uint16_t> rows;
    const bool keepRow = storedLocation != 0 && !reachedTemplateAt(choices, storedLocation, currentLevel);
    if (keepRow) rows.push_back(kKeepStoredRow);
    for (std::size_t i = 0; i < choices.size(); ++i)
        if (levelReached(choices[i], currentLevel)) rows.push_back(static_cast<uint16_t>(i));
    if (species != 0 && Policy::dayCareHatchLocation(origin) != 0 && eggVerdict(species, currentLevel).verified())
        rows.push_back(kHatchedEggRow);
    for (const auto id : MetRows::gen3OfferedLocations(origin)) {
        if (!MetRows::gen3DefaultMarker(id) || (keepRow && id == storedLocation)) continue;
        if (reachedTemplateAt(choices, id, currentLevel)) continue;
        rows.push_back(MetRows::otherNativeRow(id));
    }
    rows.push_back(kUnverifiedListRow);
    return rows;
}

// UNVERIFIED list: the keep-stored row again, the templates above the current level (R12-005,
// "level is below this encounter"), then every other native location of the origin game that has
// no template and is not a marker. Empty only for an origin with no native Gen III GBA list.
template <typename Choice>
inline std::vector<uint16_t> unverifiedRows(const std::vector<Choice>& choices, uint16_t storedLocation,
                                            uint8_t origin, uint8_t currentLevel, uint16_t species = 0) {
    std::vector<uint16_t> rows;
    const bool keepRow = storedLocation != 0 && !reachedTemplateAt(choices, storedLocation, currentLevel);
    if (keepRow) rows.push_back(kKeepStoredRow);
    for (std::size_t i = 0; i < choices.size(); ++i)
        if (!levelReached(choices[i], currentLevel)) rows.push_back(static_cast<uint16_t>(i));
    // Round 13: a hatched-egg history BreedingPolicy does not verify (labelled with the reason).
    if (species != 0 && Policy::dayCareHatchLocation(origin) != 0 && !eggVerdict(species, currentLevel).verified())
        rows.push_back(kHatchedEggRow);
    for (const auto row : MetRows::otherNativeRows(
             MetRows::gen3OfferedLocations(origin),
             [&](uint16_t id) { return storedLocationListed(choices, id) || MetRows::gen3DefaultMarker(id); },
             keepRow, storedLocation))
        rows.push_back(row);
    return rows;
}

// Initial focus over a default or unverified list: the keep-stored row, else the template row at
// the stored location whose range holds the stored met level (else its first template row), else
// the stored location's other-native / marker row, else the first row.
template <typename Choice>
inline int selectedRowIn(const std::vector<Choice>& choices, const std::vector<uint16_t>& rows,
                         uint16_t currentLocation, uint8_t currentMetLevel) noexcept {
    if (!rows.empty() && rows.front() == kKeepStoredRow) return 0;
    int fallback = -1;
    for (int i = 0; i < static_cast<int>(rows.size()); ++i) {
        const auto row = rows[static_cast<std::size_t>(i)];
        if (const auto* choice = choiceAt(choices, row)) {
            const auto& encounter = encounterOf(*choice);
            if (encounter.location != currentLocation) continue;
            if (currentMetLevel == 0 || encounter.containsLevel(currentMetLevel)) return i;
            if (fallback < 0) fallback = i;
        } else if (MetRows::isOtherNativeRow(row) && MetRows::otherNativeRowLocation(row) == currentLocation &&
                   fallback < 0) {
            fallback = i;
        }
    }
    return fallback >= 0 ? fallback : 0;
}

// The default list's verified template rows (keep-stored, markers and the last row excluded).
template <typename Choice>
inline std::size_t verifiedRowCount(const std::vector<Choice>& choices, const std::vector<uint16_t>& rows) noexcept {
    std::size_t count = 0;
    for (const auto row : rows)
        if (row < choices.size()) ++count;
    return count;
}

// The full picker (ExactGameMetLocations.h): keep-stored row, catch templates, then every other
// location native to the record's origin game. It is empty only when there is nothing to keep and
// the origin has no native Gen III GBA list; it never fails closed because a species has no
// template in this game.
template <typename Choice>
inline std::vector<uint16_t> rowValuesWithNative(const std::vector<Choice>& choices, uint16_t storedLocation,
                                                 uint8_t origin) {
    auto rows = rowValuesWithStored(choices, storedLocation);
    const bool keepRow = !rows.empty() && rows.front() == kKeepStoredRow;
    const auto others = MetRows::otherNativeRows(
        MetRows::gen3OfferedLocations(origin),
        [&](uint16_t id) { return storedLocationListed(choices, id); }, keepRow, storedLocation);
    rows.insert(rows.end(), others.begin(), others.end());
    return rows;
}

// Initial focus over the full picker: keep-stored, else the matching template, else the stored
// location's other-native row, else the first row.
template <typename Choice>
inline int selectedRowWithNative(const std::vector<Choice>& choices, const std::vector<uint16_t>& rows,
                                 uint16_t currentLocation, uint8_t currentLevel) noexcept {
    if (!rows.empty() && rows.front() == kKeepStoredRow) return 0;
    if (storedLocationListed(choices, currentLocation)) return selectedRow(choices, currentLocation, currentLevel);
    for (int i = 0; i < static_cast<int>(rows.size()); ++i) {
        const auto row = rows[static_cast<std::size_t>(i)];
        if (MetRows::isOtherNativeRow(row) && MetRows::otherNativeRowLocation(row) == currentLocation) return i;
    }
    return 0;
}

// One picker row as {place, detail}. Round 11 follow-up: a verified catch template says its method
// and level ("Wild Lv. 3–5"); every other native location says "Not a listed catch; unverified" (or
// names its trade / egg / event marker, also "unverified"), and the keep-stored row is the save's
// own value, never judged. A template row is never labelled unverified, and no row says "legal".
struct RowText {
    std::string place;
    std::string detail;
    bool verifiedCatch = false;   // a catch template of the species / its ancestry in the origin game
};
template <typename Choice>
inline RowText rowText(const std::vector<Choice>& choices, uint16_t value, uint8_t origin,
                       uint16_t storedLocation, uint8_t storedMetLevel, uint8_t currentLevel = 0,
                       uint16_t species = 0) {
    if (value == kUnverifiedListRow)   // Round 12: opens the unverified list; never a location
        return {Policy::kUnverifiedListRowPlace, Policy::kUnverifiedListRowDetail, false};
    if (value == kHatchedEggRow) {
        // Round 13 (owner decision 5): the hatch place it writes; verified only by BreedingPolicy.
        const auto egg = eggVerdict(species, currentLevel);
        RowText out{Names::getMetLocationDisplayName(origin, Policy::dayCareHatchLocation(origin)), {}, egg.verified()};
        if (egg.verified()) {
            out.detail = std::string(Policy::kHatchedEggVerified) + " Lv. 5+";
            if (egg.hatchedAs != species) out.detail += " via " + std::string(Names::getSpeciesName(egg.hatchedAs));
        } else {
            out.detail = std::string(Policy::kHatchedEggUnverified) + "; " +
                         PokeVault::Integration::BreedingPolicy::reasonText(egg.reason) + "; unverified";
        }
        return out;
    }
    if (value == kKeepStoredRow) {
        // R2-BUG-008: the save's own met location, not judged (traded / hatched / event / other).
        return {"Keep stored: " + Names::getMetLocationDisplayName(origin, storedLocation),
                "Met Lv. " + std::to_string(storedMetLevel), false};
    }
    if (MetRows::isOtherNativeRow(value)) {
        const auto location = MetRows::otherNativeRowLocation(value);
        return {Names::getMetLocationDisplayName(origin, location), MetRows::gen3OtherDetail(location), false};
    }
    const auto* selected = choiceAt(choices, value);
    if (!selected) return {"Unknown location", {}, false};
    const auto& encounter = encounterOf(*selected);
    RowText out{Names::getMetLocationDisplayName(origin, encounter.location), {}, levelReached(*selected, currentLevel)};
    // Round 5 NEW-10: short enough that every row fits the 620 px window on one line
    // (tests/test_met_location_fit.cpp).
    out.detail = std::string(Encounter::methodName(encounter.method)) + " Lv. " + std::to_string(encounter.minLevel);
    if (encounter.maxLevel != encounter.minLevel) out.detail += "–" + std::to_string(encounter.maxLevel);
    if constexpr (requires { selected->evolved; }) {
        // Round 12: every pre-evolution's rows are offered now (not only when the species has none
        // of its own), so the row names the ancestor it was met as the way Gen II / IV / V rows do
        // ("via Slowpoke"), without the evolution gate, to stay one line.
        if (selected->evolved)
            out.detail += " via " + std::string(Names::getSpeciesName(selected->originalEncounterSpecies));
    }
    // R12-005: a template above the Pokémon's level is listed only in the unverified list, and says why.
    // (short -- no method or evolution gate -- so the reason fits beside the place name)
    if (!out.verifiedCatch) {
        out.detail = "Lv. " + std::to_string(encounter.minLevel) + "+";   // its minimum is what the level misses
        if constexpr (requires { selected->evolved; })
            if (selected->evolved) out.detail += " via " + std::string(Names::getSpeciesName(selected->originalEncounterSpecies));
        out.detail += std::string("; ") + Policy::kLevelBelowEncounter;
    }
    return out;
}

// The editor's "Encounter origin" line and the Review note for a stored Met Location: the species it
// was met as when the location / met level is a verified template the current level reaches,
// otherwise an honest unverified label (a marker names itself). Never "legal".
template <typename Choice>
inline std::string encounterOriginText(const std::vector<Choice>& choices, uint16_t species, uint16_t location,
                                       uint8_t metLevel, uint8_t currentLevel) {
    if (location == 0) return "Unknown";
    if (const auto* verified = verifiedTemplate(choices, location, metLevel, currentLevel)) {
        if constexpr (requires { verified->evolved; }) {
            if (verified->evolved)
                return std::string(Names::getSpeciesName(verified->originalEncounterSpecies)) + " → " +
                       Names::getSpeciesName(species);
        }
        return std::string(Names::getSpeciesName(species));
    }
    // Round 13 (owner decision 5): Met Level 0 is a hatched egg (any hatch place of the game).
    if (metLevel == 0 && !MetRows::gen3MarkerLocation(location)) {
        const auto egg = eggVerdict(species, currentLevel);
        if (egg.verified())
            return egg.hatchedAs == species
                ? "Hatched: " + std::string(Names::getSpeciesName(species))
                : "Hatched: " + std::string(Names::getSpeciesName(egg.hatchedAs)) + " → " + Names::getSpeciesName(species);
        return std::string("Hatched-egg history; ") + PokeVault::Integration::BreedingPolicy::reasonText(egg.reason) +
               "; unverified";
    }
    if (MetRows::gen3MarkerLocation(location)) return MetRows::gen3OtherDetail(location);
    if (storedLocationListed(choices, location)) return "Not a listed catch at this level; unverified";
    return MetRows::kOtherPlaceDetail;
}

} // namespace PokeBank::UIModel::Gen3EncounterPicker
