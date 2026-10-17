// ONE Home party-icon rule for Gen I-V (campaign directive; QA-F5 follow-up):
// every generation fills species / level / form / shiny / gender from its own
// strictly parsed record, only where the format defines the field. Gen IV and
// Gen V go through the same fields in test_home_card_gen4_snapshot.cpp and
// test_home_card_gen5_snapshot.cpp; this test covers the Gen I-III adapters
// that SaveSelectScreen uses for the RetroArch / forwarder previews.
// Synthetic records only.
#include "Integration/Common/HomePartyPreview.h"

#include <cassert>
#include <cstdint>
#include <iostream>

namespace P = PokeVault::Integration::HomePartyPreview;
namespace S = PokeVault::Source;
using PokeVault::Integration::Gen1::PokemonRecord;

namespace {

constexpr uint8_t kUnknown = S::HomeCardPartySlot::GenderUnknown;

void gen1HasNoShinyGenderOrForm() {
    PokemonRecord p;
    p.species = 25;
    p.level = 33;
    p.dvs = {15, 15, 10, 10, 10}; // would be "shiny" in Gen II: Gen I has no shiny
    const auto slot = P::fromGen1(p);
    assert(slot.species == 25 && slot.level == 33 && slot.form == 0);
    assert(!slot.shiny && slot.gender == kUnknown);
}

void gen2CarriesShinyUnownLetterAndDvGender() {
    PokeVault::Integration::Gen2::PokemonRecord p;
    p.species = 25;              // Pikachu, Gen II ratio 127: female iff Atk DV <= 7
    p.level = 41;
    p.dvs = {0, 10, 10, 10, 10}; // HP / Atk / Def / Spe / Special; Atk 10 has bit 2
    p.shiny = true;              // as Gen2ReadOnlySave derives it from these DVs
    auto slot = P::fromGen2(p);
    assert(slot.species == 25 && slot.level == 41 && slot.shiny && slot.gender == 0);
    p.dvs[1] = 3;
    p.shiny = false;
    slot = P::fromGen2(p);
    assert(!slot.shiny && slot.gender == 1);

    PokeVault::Integration::Gen2::PokemonRecord unown;
    unown.species = 201;
    unown.level = 5;
    unown.form = 7;              // Unown H, from Gen2ReadOnlySave's DV letter
    slot = P::fromGen2(unown);
    assert(slot.species == 201 && slot.form == 7 && slot.gender == 2); // genderless
}

PokeVault::Integration::Gen3::PokemonRecord gen3(uint16_t species, uint32_t pid,
                                                 uint8_t level, bool party = true) {
    PokeVault::Integration::Gen3::PokemonRecord p;
    p.species = species;
    p.pid = pid;
    p.tid = 12345;
    p.sid = 54321;
    p.originalBytes.assign(party ? 100 : 80, 0);
    if (party) p.originalBytes[0x54] = level; // PK3 Stat_Level (PKHeX 6501f0ab)
    return p;
}

void gen3UsesStoredLevelPidShinyAndUnownLetter() {
    // Level: the stored party level, never level 0 and never a growth guess.
    auto slot = P::fromGen3(gen3(6, 0xABCD0123u, 36));
    assert(slot.species == 6 && slot.level == 36 && slot.form == 0 && !slot.shiny);
    // Boxed 80-byte record: no stored level, shown without one.
    assert(P::fromGen3(gen3(6, 0xABCD0123u, 0, false)).level == 0);
    // Shiny: (TID ^ SID ^ PIDhi ^ PIDlo) < 8.
    const uint16_t lo = 0x1111;
    const uint32_t shiny = (static_cast<uint32_t>(lo ^ 12345u ^ 54321u) << 16) | lo;
    assert(P::fromGen3(gen3(6, shiny, 50)).shiny);
    assert(P::fromGen3(gen3(6, shiny ^ 0x00080000u, 50)).shiny == false);
    // Unown letter from the PID (EntityPID.GetUnownForm3): 2 bits per byte.
    assert(P::fromGen3(gen3(201, 0x00000001u, 10)).form == 1);    // B
    assert(P::fromGen3(gen3(201, 0x03030303u, 10)).form == 255 % 28);
    assert(P::fromGen3(gen3(201, 0x01000000u, 10)).form == 64 % 28);
    // Only Unown uses the PID letter.
    assert(P::fromGen3(gen3(25, 0x03030303u, 10)).form == 0);
    // Gender from the PID against the Gen III ratio (Gen3Gender.h, PKSM-Core personal3).
    // Pikachu 127: PID low byte 0x03 < 127 -> female; 0x90 -> male. Unown genderless.
    assert(P::fromGen3(gen3(25, 0x03030303u, 10)).gender == 1);
    assert(P::fromGen3(gen3(25, 0x00000090u, 10)).gender == 0);
    assert(P::fromGen3(gen3(201, 0x00000001u, 10)).gender == 2);
    assert(P::fromGen3(gen3(29, 0x000000FFu, 10)).gender == 1); // Nidoran F: always female
    // A species with no Gen III data has no gender, never a guess.
    assert(P::fromGen3(gen3(387, 0x000000FFu, 10)).gender == kUnknown);
}

} // namespace

int main() {
    gen1HasNoShinyGenderOrForm();
    gen2CarriesShinyUnownLetterAndDvGender();
    gen3UsesStoredLevelPidShinyAndUnownLetter();
    std::cout << "Home party preview Gen I-III: species/level/form/shiny/gender from native records PASS\n";
}
