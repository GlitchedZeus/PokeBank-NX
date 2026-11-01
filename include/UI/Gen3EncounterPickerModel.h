#pragma once

#include "Integration/Encounter/EncounterGuardrails.h"
#include "UI/ExactGameMetLocations.h"

#include <cstddef>
#include <cstdint>
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
static_assert(kKeepStoredRow == MetRows::kKeepStoredRowValue);

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
inline std::vector<Encounter::EncounterProvenanceChoice> templatesForOrigin(uint8_t origin, uint16_t species) {
    const auto game = MetRows::gen3GameIdForOrigin(origin);
    if (game.empty()) return {};
    return Encounter::forGameSpeciesWithGen3Provenance(game, species);
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

} // namespace PokeBank::UIModel::Gen3EncounterPicker
