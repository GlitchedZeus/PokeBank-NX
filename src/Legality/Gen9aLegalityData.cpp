// Gen9aLegalityData.cpp - lookups over the PKHeX-generated Legends: Z-A rule tables.
// The tables themselves are include/Legality/Gen9aLegalityData.inc, written by
// tools/pkhex_gen67_diff/oracle ("dump9a") and checked against the pinned PKHeX in CI.
#include "Legality/Gen9aLegality.h"

#include <algorithm>
#include <array>

namespace Legality::Gen9a::Data {
namespace {

struct PersonalRow9a {
    uint16_t species;
    uint8_t form;
    uint16_t ability1, ability2, abilityHidden;
    uint8_t gender, growth, formCount, present;
};
struct SpeciesForm {
    uint16_t species;
    uint8_t form;
};

#include "Legality/Gen9aLegalityData.inc"

} // namespace

std::optional<Personal> personal(uint16_t species, uint8_t form) noexcept {
    if (species == 0) return std::nullopt;
    const auto it = std::lower_bound(kPersonal9a.begin(), kPersonal9a.end(), species,
                                     [](const PersonalRow9a& r, uint16_t s) { return r.species < s; });
    if (it == kPersonal9a.end() || it->species != species || form >= it->formCount) return std::nullopt;
    const auto& r = *(it + form);
    if (r.species != species || r.form != form) return std::nullopt;
    return Personal{r.ability1, r.ability2, r.abilityHidden, r.gender, r.growth, r.formCount, r.present != 0};
}

uint8_t basePP(uint16_t move) noexcept { return move < kBasePP9a.size() ? kBasePP9a[move] : 0; }
uint16_t maxSpecies() noexcept { return kMaxSpecies9a; }
uint16_t maxMove() noexcept { return kMaxMove9a; }
uint8_t maxBall() noexcept { return static_cast<uint8_t>(kMaxBall9a); }

bool heldItemAllowed(uint16_t item) noexcept {
    if (item == 0) return true;
    return item < kHeldItems9aCount && ((kHeldItems9a[item >> 3] >> (item & 7)) & 1) != 0;
}

bool battleOnlyForm(uint16_t species, uint8_t form) noexcept {
    return std::any_of(kBattleForms9a.begin(), kBattleForms9a.end(),
                       [&](const SpeciesForm& r) { return r.species == species && r.form == form; });
}

bool mintNature(uint8_t nature) noexcept { return nature < 25 && ((kMintNatures9a >> nature) & 1) != 0; }

uint16_t plusCount() noexcept { return kPlusCount9a; }

bool plusLineCanLearn(uint16_t species, uint16_t index) noexcept {
    constexpr size_t stride = (kPlusCount9a + 7) / 8;
    if (species > kMaxSpecies9a || index >= kPlusCount9a) return false;
    return ((kPlusLine9a[species * stride + (index >> 3)] >> (index & 7)) & 1) != 0;
}

} // namespace Legality::Gen9a::Data
