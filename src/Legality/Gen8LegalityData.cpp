// Gen8LegalityData.cpp - lookups over the PKHeX-generated Sword/Shield rule tables.
// The tables themselves are include/Legality/Gen8LegalityData.inc, written by
// tools/pkhex_gen67_diff/oracle ("dump8") and checked against the pinned PKHeX in CI.
#include "Legality/Gen8Legality.h"

#include <algorithm>
#include <array>

namespace Legality::Gen8::Data {
namespace {

struct PersonalRow8 {
    uint16_t species;
    uint8_t form;
    uint16_t ability1, ability2, abilityHidden;
    uint8_t gender, growth, formCount, present;
    uint8_t hp, atk, def, spe, spa, spd;
};
struct SpeciesForm {
    uint16_t species;
    uint8_t form;
};
struct ItemForm {
    uint16_t item;
    uint8_t form;
};

#include "Legality/Gen8LegalityData.inc"

constexpr size_t kTechRecordBytes = (kTechRecordCount8 + 7) / 8;
static_assert(kTechRecords8.size() == kPersonal8.size() * kTechRecordBytes);

// Index of the species/form row (rows are ordered by species, then form); -1 when outside.
ptrdiff_t findRow(uint16_t species, uint8_t form) noexcept {
    if (species == 0) return -1;
    const auto it = std::lower_bound(kPersonal8.begin(), kPersonal8.end(), species,
                                     [](const PersonalRow8& r, uint16_t s) { return r.species < s; });
    if (it == kPersonal8.end() || it->species != species || form >= it->formCount) return -1;
    const auto index = (it - kPersonal8.begin()) + form;
    const auto& row = kPersonal8[static_cast<size_t>(index)];
    return row.species == species && row.form == form ? index : -1;
}

template <size_t N>
uint8_t lookupForm(const std::array<ItemForm, N>& rows, uint16_t item) noexcept {
    for (const auto& r : rows)
        if (r.item == item) return r.form;
    return 0;
}

} // namespace

std::optional<Personal> personal(uint16_t species, uint8_t form) noexcept {
    const auto index = findRow(species, form);
    if (index < 0) return std::nullopt;
    const auto& r = kPersonal8[static_cast<size_t>(index)];
    return Personal{r.ability1, r.ability2, r.abilityHidden, r.gender, r.growth, r.formCount, r.present != 0,
                    {r.hp, r.atk, r.def, r.spe, r.spa, r.spd}};
}

uint8_t formCount(uint16_t species) noexcept {
    const auto index = findRow(species, 0);
    return index < 0 ? 0 : kPersonal8[static_cast<size_t>(index)].formCount;
}

bool presentInGame(uint16_t species, uint8_t form) noexcept {
    const auto index = findRow(species, form);
    return index >= 0 && kPersonal8[static_cast<size_t>(index)].present != 0;
}

bool canLearnTechRecord(uint16_t species, uint8_t form, uint8_t index) noexcept {
    const auto row = findRow(species, form);
    if (row < 0 || index >= kTechRecordCount8) return false;
    return ((kTechRecords8[static_cast<size_t>(row) * kTechRecordBytes + (index >> 3)] >> (index & 7)) & 1) != 0;
}
uint8_t techRecordCount() noexcept { return kTechRecordCount8; }

uint8_t basePP(uint16_t move) noexcept { return move < kBasePP8.size() ? kBasePP8[move] : 0; }
uint16_t maxSpecies() noexcept { return kMaxSpecies8; }
uint16_t maxMove() noexcept { return kMaxMove8; }
uint16_t maxItem() noexcept { return kMaxItem8; }
uint8_t maxBall() noexcept { return static_cast<uint8_t>(kMaxBall8); }

bool heldItemAllowed(uint16_t item) noexcept {
    if (item == 0) return true;
    return item < kHeldItems8Count && ((kHeldItems8[item >> 3] >> (item & 7)) & 1) != 0;
}

bool battleOnlyForm(uint16_t species, uint8_t form) noexcept {
    return std::any_of(kBattleForms8.begin(), kBattleForms8.end(),
                       [&](const SpeciesForm& r) { return r.species == species && r.form == form; });
}

uint8_t arceusForm(uint16_t item) noexcept { return lookupForm(kArceusItems8, item); }
uint8_t genesectForm(uint16_t item) noexcept { return lookupForm(kGenesectItems8, item); }
uint8_t silvallyForm(uint16_t item) noexcept { return lookupForm(kSilvallyItems8, item); }

bool mintNature(uint8_t nature) noexcept { return nature < 25 && ((kMintNatures >> nature) & 1) != 0; }

} // namespace Legality::Gen8::Data
