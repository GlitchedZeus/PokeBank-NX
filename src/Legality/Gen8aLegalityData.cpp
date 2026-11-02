// Gen8aLegalityData.cpp - lookups over the PKHeX-generated Legends: Arceus rule tables.
// The tables themselves are include/Legality/Gen8aLegalityData.inc, written by
// tools/pkhex_gen67_diff/oracle ("dump8a") and checked against the pinned PKHeX in CI.
#include "Legality/Gen8aLegality.h"

#include <algorithm>
#include <array>

namespace Legality::Gen8a::Data {
namespace {

struct PersonalRow8a {
    uint16_t species;
    uint8_t form;
    uint16_t ability1, ability2, abilityHidden;
    uint8_t gender, growth, formCount, present;
    uint16_t height, weight;
    uint64_t moveShop;
};
struct SpeciesForm {
    uint16_t species;
    uint8_t form;
};

#include "Legality/Gen8aLegalityData.inc"

} // namespace

std::optional<Personal> personal(uint16_t species, uint8_t form) noexcept {
    if (species == 0) return std::nullopt;
    const auto it = std::lower_bound(kPersonal8a.begin(), kPersonal8a.end(), species,
                                     [](const PersonalRow8a& r, uint16_t s) { return r.species < s; });
    if (it == kPersonal8a.end() || it->species != species || form >= it->formCount) return std::nullopt;
    const auto& r = *(it + form);
    if (r.species != species || r.form != form) return std::nullopt;
    return Personal{r.ability1, r.ability2, r.abilityHidden, r.gender, r.growth, r.formCount, r.present != 0,
                    r.height, r.weight, r.moveShop};
}

std::span<const uint16_t> moveShop() noexcept { return kMoveShop8a; }
uint8_t basePP(uint16_t move) noexcept { return move < kBasePP8a.size() ? kBasePP8a[move] : 0; }
uint16_t maxSpecies() noexcept { return kMaxSpecies8a; }
uint16_t maxMove() noexcept { return kMaxMove8a; }
uint8_t maxBall() noexcept { return static_cast<uint8_t>(kMaxBall8a); }

bool heldItemAllowed(uint16_t item) noexcept {
    if (item == 0) return true;
    return item < kHeldItems8aCount && ((kHeldItems8a[item >> 3] >> (item & 7)) & 1) != 0;
}

bool battleOnlyForm(uint16_t species, uint8_t form) noexcept {
    return std::any_of(kBattleForms8a.begin(), kBattleForms8a.end(),
                       [&](const SpeciesForm& r) { return r.species == species && r.form == form; });
}

bool mintNature(uint8_t nature) noexcept { return nature < 25 && ((kMintNatures8a >> nature) & 1) != 0; }

} // namespace Legality::Gen8a::Data
