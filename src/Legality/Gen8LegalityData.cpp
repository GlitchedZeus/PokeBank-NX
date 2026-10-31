// Gen8LegalityData.cpp - lookups over the PKHeX-generated Sword/Shield and BD/SP rule tables.
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
template <size_t N>
ptrdiff_t findRow(const std::array<PersonalRow8, N>& rows, uint16_t species, uint8_t form) noexcept {
    if (species == 0) return -1;
    const auto it = std::lower_bound(rows.begin(), rows.end(), species,
                                     [](const PersonalRow8& r, uint16_t s) { return r.species < s; });
    if (it == rows.end() || it->species != species || form >= it->formCount) return -1;
    const auto index = (it - rows.begin()) + form;
    const auto& row = rows[static_cast<size_t>(index)];
    return row.species == species && row.form == form ? index : -1;
}

const PersonalRow8* findPersonal(Context ctx, uint16_t species, uint8_t form) noexcept {
    if (ctx == Context::BDSP) {
        const auto i = findRow(kPersonal8b, species, form);
        return i < 0 ? nullptr : &kPersonal8b[static_cast<size_t>(i)];
    }
    const auto i = findRow(kPersonal8, species, form);
    return i < 0 ? nullptr : &kPersonal8[static_cast<size_t>(i)];
}

template <size_t N>
uint8_t lookupForm(const std::array<ItemForm, N>& rows, uint16_t item) noexcept {
    for (const auto& r : rows)
        if (r.item == item) return r.form;
    return 0;
}

} // namespace

std::optional<Personal> personal(uint16_t species, uint8_t form, Context ctx) noexcept {
    const PersonalRow8* r = findPersonal(ctx, species, form);
    if (!r) return std::nullopt;
    return Personal{r->ability1, r->ability2, r->abilityHidden, r->gender, r->growth, r->formCount, r->present != 0,
                    {r->hp, r->atk, r->def, r->spe, r->spa, r->spd}};
}

uint8_t formCount(uint16_t species, Context ctx) noexcept {
    const PersonalRow8* r = findPersonal(ctx, species, 0);
    return r ? r->formCount : 0;
}

bool presentInGame(uint16_t species, uint8_t form, Context ctx) noexcept {
    const PersonalRow8* r = findPersonal(ctx, species, form);
    return r && r->present != 0;
}

bool canLearnTechRecord(uint16_t species, uint8_t form, uint8_t index) noexcept {
    const auto row = findRow(kPersonal8, species, form);
    if (row < 0 || index >= kTechRecordCount8) return false;
    return ((kTechRecords8[static_cast<size_t>(row) * kTechRecordBytes + (index >> 3)] >> (index & 7)) & 1) != 0;
}
uint8_t techRecordCount() noexcept { return kTechRecordCount8; }

uint8_t basePP(uint16_t move, Context ctx) noexcept {
    const auto& pp = ctx == Context::BDSP ? kBasePP8b : kBasePP8;
    return move < pp.size() ? pp[move] : 0;
}
uint16_t maxSpecies(Context ctx) noexcept { return ctx == Context::BDSP ? kMaxSpecies8b : kMaxSpecies8; }
uint16_t maxMove(Context ctx) noexcept { return ctx == Context::BDSP ? kMaxMove8b : kMaxMove8; }
uint16_t maxItem(Context ctx) noexcept { return ctx == Context::BDSP ? kMaxItem8b : kMaxItem8; }
uint8_t maxBall(Context ctx) noexcept { return static_cast<uint8_t>(ctx == Context::BDSP ? kMaxBall8b : kMaxBall8); }

bool heldItemAllowed(uint16_t item, Context ctx) noexcept {
    if (item == 0) return true;
    if (ctx == Context::BDSP) return item < kHeldItems8bCount && ((kHeldItems8b[item >> 3] >> (item & 7)) & 1) != 0;
    return item < kHeldItems8Count && ((kHeldItems8[item >> 3] >> (item & 7)) & 1) != 0;
}

bool battleOnlyForm(uint16_t species, uint8_t form, Context ctx) noexcept {
    auto has = [&](const auto& rows) {
        return std::any_of(rows.begin(), rows.end(), [&](const SpeciesForm& r) { return r.species == species && r.form == form; });
    };
    return ctx == Context::BDSP ? has(kBattleForms8b) : has(kBattleForms8);
}

uint8_t arceusForm(uint16_t item, Context ctx) noexcept {
    return ctx == Context::BDSP ? lookupForm(kArceusItems8b, item) : lookupForm(kArceusItems8, item);
}
uint8_t genesectForm(uint16_t item) noexcept { return lookupForm(kGenesectItems8, item); }
uint8_t silvallyForm(uint16_t item) noexcept { return lookupForm(kSilvallyItems8, item); }

bool mintNature(uint8_t nature) noexcept { return nature < 25 && ((kMintNatures >> nature) & 1) != 0; }

} // namespace Legality::Gen8::Data
