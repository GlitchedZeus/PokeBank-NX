// Gen7bLegalityData.cpp - lookups over the PKHeX-generated Let's Go Pikachu/Eevee rule tables.
// The tables themselves are include/Legality/Gen7bLegalityData.inc, written by
// tools/pkhex_gen67_diff/oracle ("dump7b") and checked against the pinned PKHeX in CI.
#include "Legality/Gen7bLegality.h"

#include <algorithm>
#include <array>

namespace Legality::Gen7b::Data {
namespace {

struct PersonalRow7b {
    uint16_t species;
    uint8_t form;
    uint16_t ability1, ability2, abilityHidden;
    uint8_t gender, growth, formCount, present;
    uint8_t hp, atk, def, spe, spa, spd;
    uint16_t height, weight;
};
struct SpeciesForm {
    uint16_t species;
    uint8_t form;
};

#include "Legality/Gen7bLegalityData.inc"

} // namespace

std::optional<Personal> personal(uint16_t species, uint8_t form) noexcept {
    if (species == 0) return std::nullopt;
    const auto it = std::lower_bound(kPersonal7b.begin(), kPersonal7b.end(), species,
                                     [](const PersonalRow7b& r, uint16_t s) { return r.species < s; });
    if (it == kPersonal7b.end() || it->species != species || form >= it->formCount) return std::nullopt;
    const auto& r = *(it + form);
    if (r.species != species || r.form != form) return std::nullopt;
    return Personal{r.ability1, r.ability2, r.abilityHidden, r.gender, r.growth, r.formCount, r.present != 0,
                    {r.hp, r.atk, r.def, r.spe, r.spa, r.spd}, r.height, r.weight};
}

uint8_t basePP(uint16_t move) noexcept { return move < kBasePP7b.size() ? kBasePP7b[move] : 0; }
uint16_t maxSpecies() noexcept { return kMaxSpecies7b; }
uint16_t maxMove() noexcept { return kMaxMove7b; }
uint8_t maxBall() noexcept { return static_cast<uint8_t>(kMaxBall7b); }

bool heldItemAllowed(uint16_t item) noexcept {
    if (item == 0) return true;
    return item < kHeldItems7bCount && ((kHeldItems7b[item >> 3] >> (item & 7)) & 1) != 0;
}

bool battleOnlyForm(uint16_t species, uint8_t form) noexcept {
    return std::any_of(kBattleForms7b.begin(), kBattleForms7b.end(),
                       [&](const SpeciesForm& r) { return r.species == species && r.form == form; });
}

} // namespace Legality::Gen7b::Data
