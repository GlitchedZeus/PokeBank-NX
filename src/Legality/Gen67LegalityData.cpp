// Gen67LegalityData.cpp - lookups over the PKHeX-generated Gen VI/VII rule tables.
// The tables themselves are include/Legality/Gen67LegalityData.inc, written by
// tools/pkhex_gen67_diff/oracle ("dump") and checked against the pinned PKHeX in CI.
#include "Legality/Gen67Legality.h"

#include <algorithm>
#include <array>

namespace Legality::Gen67::Data {
namespace {

struct PersonalRow {
    uint16_t species;
    uint8_t form;
    uint16_t ability1, ability2, abilityHidden;
    uint8_t gender, growth, formCount;
};
struct SpeciesForm {
    uint16_t species;
    uint8_t form;
};
struct ItemForm {
    uint16_t item;
    uint8_t form;
};

#include "Legality/Gen67LegalityData.inc"

template <size_t N>
const PersonalRow* findRow(const std::array<PersonalRow, N>& rows, uint16_t species, uint8_t form) noexcept {
    // Rows are ordered by species, then form.
    const auto it = std::lower_bound(rows.begin(), rows.end(), species,
                                     [](const PersonalRow& r, uint16_t s) { return r.species < s; });
    if (it == rows.end() || it->species != species) return nullptr;
    if (form >= it->formCount) return nullptr;
    const auto* row = &*it + form;
    return row->species == species && row->form == form ? row : nullptr;
}

template <size_t N>
uint8_t lookupForm(const std::array<ItemForm, N>& rows, uint16_t item) noexcept {
    for (const auto& r : rows)
        if (r.item == item) return r.form;
    return 0;
}

template <size_t N>
bool bit(const std::array<uint8_t, N>& bits, uint16_t count, uint16_t index) noexcept {
    return index < count && ((bits[index >> 3] >> (index & 7)) & 1) != 0;
}

} // namespace

std::optional<Personal> personal(uint8_t format, uint16_t species, uint8_t form) noexcept {
    const PersonalRow* row = format == 6 ? findRow(kPersonal6, species, form)
                           : format == 7 ? findRow(kPersonal7, species, form) : nullptr;
    if (!row || species == 0) return std::nullopt;
    return Personal{row->ability1, row->ability2, row->abilityHidden, row->gender, row->growth, row->formCount};
}

uint8_t formCount(uint8_t format, uint16_t species) noexcept {
    const PersonalRow* row = format == 6 ? findRow(kPersonal6, species, 0)
                           : format == 7 ? findRow(kPersonal7, species, 0) : nullptr;
    return row && species != 0 ? row->formCount : 0;
}

uint8_t basePP(uint8_t format, uint16_t move) noexcept {
    if (format == 6) return move < kBasePP6.size() ? kBasePP6[move] : 0;
    if (format == 7) return move < kBasePP7.size() ? kBasePP7[move] : 0;
    return 0;
}

uint16_t maxSpecies(uint8_t format) noexcept { return format == 6 ? kMaxSpecies6 : format == 7 ? kMaxSpecies7 : 0; }
uint16_t maxMove(uint8_t format) noexcept { return format == 6 ? kMaxMove6AO : format == 7 ? kMaxMove7 : 0; }
uint8_t maxBall(uint8_t format) noexcept {
    return static_cast<uint8_t>(format == 6 ? kMaxBall6 : format == 7 ? kMaxBall7 : 0);
}

bool heldItemAllowed(uint8_t format, uint16_t item) noexcept {
    if (item == 0) return true;
    if (format == 6) return bit(kHeldItems6, kHeldItems6Count, item);
    if (format == 7) return bit(kHeldItems7, kHeldItems7Count, item);
    return false;
}

bool battleOnlyForm(uint8_t format, uint16_t species, uint8_t form) noexcept {
    auto has = [&](const auto& rows) {
        return std::any_of(rows.begin(), rows.end(),
                           [&](const SpeciesForm& r) { return r.species == species && r.form == form; });
    };
    return format == 6 ? has(kBattleForms6) : format == 7 ? has(kBattleForms7) : false;
}

uint8_t arceusForm(uint8_t format, uint16_t item) noexcept {
    return format == 6 ? lookupForm(kArceusItems6, item) : lookupForm(kArceusItems7, item);
}
uint8_t genesectForm(uint16_t item) noexcept { return lookupForm(kGenesectItems, item); }
uint8_t silvallyForm(uint16_t item) noexcept { return lookupForm(kSilvallyItems, item); }

uint8_t vivillonMaxWildForm() noexcept { return kVivillonMaxWildForm; }
bool vivillonPatternValid(uint8_t form, uint8_t consoleRegion) noexcept {
    // Vivillon3DS.GetConsoleRegionFlag maps region ids 0-6; the dump records 0-7.
    return form < kVivillonConsoleRegions.size() && consoleRegion < 8 &&
           ((kVivillonConsoleRegions[form] >> consoleRegion) & 1) != 0;
}

bool consoleRegionCountryValid(uint8_t consoleRegion, uint8_t country) noexcept {
    if (consoleRegion >= 7) return false;
    return ((kConsoleRegionCountries[size_t{consoleRegion} * 32 + (country >> 3)] >> (country & 7)) & 1) != 0;
}
bool vcLanguageValid(uint8_t consoleRegion, uint8_t language) noexcept {
    return consoleRegion < kVcRegionLanguages.size() && language < 16 && ((kVcRegionLanguages[consoleRegion] >> language) & 1) != 0;
}
bool vcNatureValidAtMetLevel2(uint8_t growth, uint8_t nature) noexcept {
    return growth >= kVcNatureMetLevel2.size() || nature >= 25 || ((kVcNatureMetLevel2[growth] >> nature) & 1) != 0;
}

uint16_t gameMaxSpecies(Modern::Game game) noexcept {
    using G = Modern::Game;
    switch (game) {
        case G::X: case G::Y: case G::OR: case G::AS: return kMaxSpecies6;
        case G::SN: case G::MN: return kMaxSpecies7SM;
        case G::US: case G::UM: return kMaxSpecies7;
        default: return 0;
    }
}
uint16_t gameMaxMove(Modern::Game game) noexcept {
    using G = Modern::Game;
    switch (game) {
        case G::X: case G::Y: return kMaxMove6XY;
        case G::OR: case G::AS: return kMaxMove6AO;
        case G::SN: case G::MN: return kMaxMove7SM;
        case G::US: case G::UM: return kMaxMove7;
        default: return 0;
    }
}
uint16_t gameMaxItem(Modern::Game game) noexcept {
    using G = Modern::Game;
    switch (game) {
        case G::X: case G::Y: return kMaxItem6XY;
        case G::OR: case G::AS: return kMaxItem6AO;
        case G::SN: case G::MN: return kMaxItem7SM;
        case G::US: case G::UM: return kMaxItem7;
        default: return 0;
    }
}

} // namespace Legality::Gen67::Data
