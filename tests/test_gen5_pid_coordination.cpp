// Gen V PID-linked editing (PKHeX 6501f0ab rules, see Gen5PidCoordination.h):
// - Gender, Shiny and the normal ability slot change only through deterministic
//   PID procedures that keep every other PID-linked trait.
// - Nature is a stored byte in Gen V (0x41) and never touches the PID.
// - Gen III/IV-origin PK5s keep their PID-derived nature / gender / ability.
// - The exact-PID replay (Field::Personality) refuses a PID that breaks a
//   gender / ability link the record already had.
#include "Integration/Gen5/Gen5PidCoordination.h"
#include "Integration/Gen5/Gen5StagedPokemonRecord.h"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

namespace G = PokeVault::Integration::Gen5;
namespace C = PokeVault::Integration::Gen5::Crypto;
using Field = G::StagedPokemon5Record::Field;

namespace {

void write32(std::vector<uint8_t>& dst, size_t off, uint32_t value) {
    C::write16(dst, off, static_cast<uint16_t>(value));
    C::write16(dst, off + 2, static_cast<uint16_t>(value >> 16));
}

// Boxed synthetic PK5 with PID-consistent gender and ability.
std::vector<uint8_t> record(uint16_t species, uint32_t pid, uint8_t origin,
                            uint16_t tid = 12345, uint16_t sid = 54321) {
    const auto personal = G::personal5(species, 0, true);
    assert(personal);
    std::vector<uint8_t> pk(C::StoredSize, 0);
    write32(pk, 0, pid);
    C::write16(pk, 0x08, species);
    C::write16(pk, 0x0C, tid);
    C::write16(pk, 0x0E, sid);
    write32(pk, 0x10, 1000);
    pk[0x15] = G::abilityForSlot5(*personal.info, G::abilityBit5(pid));
    C::write16(pk, 0x28, 33);
    pk[0x30] = 35;
    pk[0x40] = static_cast<uint8_t>(G::genderFromPid5(pid, personal.info->genderRatio) << 1);
    pk[0x41] = 3;
    pk[0x5F] = origin;
    pk[0x84] = 5;
    C::write16(pk, 6, C::checksum(pk));
    return C::encryptCandidate(pk);
}

G::StagedPokemon5Record staged(const std::vector<uint8_t>& bytes) {
    std::string error;
    auto r = G::StagedPokemon5Record::create(bytes, &error, true);
    assert(r && error.empty());
    return *r;
}

bool linked(const G::Pokemon5ReadOnly& p) {
    const auto personal = G::personal5(p.species(), p.form(), true);
    return G::genderFromPid5(p.pid(), personal.info->genderRatio) == p.gender() &&
           (p.hiddenAbility() || G::abilityForSlot5(*personal.info, G::abilityBit5(p.pid())) == p.ability());
}

void shinyToggleKeepsGenderAbilityNature() {
    // Pidove (519): 50/50 gender ratio, two normal abilities.
    auto r = staged(record(519, 0x00012345u, 21));
    const auto before = r.current();
    assert(!before.shiny() && linked(before));
    std::string error;
    assert(r.stage(Field::Shiny, 0, 1, &error));
    const auto shiny = r.current();
    assert(shiny.shiny());
    assert((shiny.pid() & 0xFFFFu) == (before.pid() & 0xFFFFu));
    assert(G::abilityBit5(shiny.pid()) == G::abilityBit5(before.pid()));
    assert(shiny.gender() == before.gender() && shiny.nature() == before.nature() &&
           shiny.ability() == before.ability() && linked(shiny));
    assert(r.stage(Field::Shiny, 0, 0, &error));
    assert(!r.current().shiny() && linked(r.current()));
}

void genderChangeMovesPidLowByteOnly() {
    auto r = staged(record(519, 0x00010080u, 23));
    const auto before = r.current();
    const uint8_t other = before.gender() == 0 ? 1 : 0;
    std::string error;
    assert(r.stage(Field::Gender, 0, other, &error));
    const auto after = r.current();
    assert(after.gender() == other && linked(after));
    assert(after.shiny() == before.shiny());
    assert(G::abilityBit5(after.pid()) == G::abilityBit5(before.pid()));
    assert(after.nature() == before.nature());
    // A shiny Pokemon stays shiny through a gender change.
    assert(r.stage(Field::Shiny, 0, 1, &error));
    assert(r.stage(Field::Gender, 0, before.gender(), &error));
    assert(r.current().shiny() && r.current().gender() == before.gender() && linked(r.current()));
}

void abilitySlotUsesBit16AndKeepsShiny() {
    auto r = staged(record(519, 0x00002222u, 22));
    std::string error;
    assert(r.stage(Field::Shiny, 0, 1, &error));
    assert(r.stage(Field::AbilitySlot, 0, 1, &error));
    const auto after = r.current();
    assert(G::abilityBit5(after.pid()) == 1 && after.shiny() && linked(after));
}

void natureIsStoredNotPidLinkedInGen5() {
    auto r = staged(record(519, 0x00002222u, 20));
    const uint32_t pid = r.current().pid();
    std::string error;
    assert(r.stage(Field::Nature, 0, 17, &error));
    assert(r.current().nature() == 17 && r.current().pid() == pid);
}

void earlierOriginsKeepPidDerivedTraits() {
    // Origin 12 (HeartGold): PID % 25 nature, PID bit 0 ability, PID gender.
    auto r = staged(record(519, 0x00002222u, 12));
    const auto before = r.current();
    std::string error;
    assert(!r.stage(Field::Shiny, 0, 1, &error) && !error.empty());
    assert(!r.stage(Field::Gender, 0, before.gender() == 0 ? 1 : 0, &error));
    assert(!r.stage(Field::Nature, 0, (before.nature() + 1) % 25, &error));
    assert(!r.stage(Field::AbilitySlot, 0, 1, &error));
    assert(!r.stage(Field::Personality, 0, 0x01020304u, &error));
    assert(r.current().pid() == before.pid() && !r.dirty());
    // Unchanged values remain accepted no-ops.
    assert(r.stage(Field::Nature, 0, before.nature(), &error));
    assert(r.stage(Field::Gender, 0, before.gender(), &error));
}

void exactPidReplayKeepsLinks() {
    auto source = record(519, 0x00012345u, 21);
    auto edited = staged(source);
    std::string error;
    assert(edited.stage(Field::Shiny, 0, 1, &error));
    assert(edited.stage(Field::Gender, 0, edited.current().gender() == 0 ? 1 : 0, &error));
    // Replay the draft through the same field order Keep uses.
    auto replay = staged(source);
    assert(replay.stage(Field::Gender, 0, edited.current().gender(), &error));
    assert(replay.stage(Field::Personality, 0, edited.current().pid(), &error));
    assert(std::vector<uint8_t>(replay.stagedBytes().begin(), replay.stagedBytes().end()) ==
           std::vector<uint8_t>(edited.stagedBytes().begin(), edited.stagedBytes().end()));
    // A PID that would make the stored gender inconsistent is refused.
    auto broken = staged(source);
    const auto now = broken.current();
    const auto personal = G::personal5(519, 0, true);
    const uint32_t wrong = now.gender() == 0 ? (now.pid() & ~0xFFu) : (now.pid() | 0xFFu);
    assert(G::genderFromPid5(wrong, personal.info->genderRatio) != now.gender());
    assert(!broken.stage(Field::Personality, 0, wrong, &error) && !broken.dirty());
}

void idChangeCanKeepShinyThroughPid() {
    auto r = staged(record(519, 0x00012345u, 21));
    std::string error;
    assert(r.stage(Field::Shiny, 0, 1, &error));
    const auto before = r.current();
    assert(r.stage(Field::TrainerId, 0, 1, &error));
    const uint32_t kept = G::keepShinyForIds5(before.pid(), before.tid(), before.sid(), 1, before.sid());
    assert(r.stage(Field::Personality, 0, kept, &error));
    assert(r.current().shiny() && linked(r.current()));
}

} // namespace

int main() {
    shinyToggleKeepsGenderAbilityNature();
    genderChangeMovesPidLowByteOnly();
    abilitySlotUsesBit16AndKeepsShiny();
    natureIsStoredNotPidLinkedInGen5();
    earlierOriginsKeepPidDerivedTraits();
    exactPidReplayKeepsLinks();
    idChangeCanKeepShinyThroughPid();
    std::cout << "Gen V PID-coordinated Gender / Shiny / Ability / Nature: PASS\n";
}
