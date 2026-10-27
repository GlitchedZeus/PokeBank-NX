// Gen V species change keeps the PID (no silent rerolls), so the target species' gender ratio
// must read that PID as the stored gender. Upstream PKSE 620c5f6 established the invariant
// "never a PID that contradicts the stored gender"; before this test a female Pikachu
// (PID low byte 0x50) could become an Eevee whose PID reads male (ratio 31).
#include "Integration/Gen5/Gen5StagedPokemonRecord.h"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

namespace {
namespace G = PokeVault::Integration::Gen5;
namespace C = G::Crypto;
using F = G::StagedPokemon5Record::Field;

constexpr uint16_t kTid = 12345;
constexpr uint16_t kSid = 54321;

// A Gen V-origin (Black, 0x15) Pikachu PK5 with the given PID and stored gender.
std::vector<uint8_t> pikachu(uint32_t pid, uint8_t gender) {
    std::vector<uint8_t> pk(C::StoredSize, 0);
    C::write16(pk, 0, static_cast<uint16_t>(pid));
    C::write16(pk, 2, static_cast<uint16_t>(pid >> 16));
    C::write16(pk, 0x08, 25);
    C::write16(pk, 0x0C, kTid);
    C::write16(pk, 0x0E, kSid);
    C::write16(pk, 0x10, 1000);
    pk[0x14] = 70;
    const auto personal = G::personal5(25, 0, false);
    assert(personal);
    pk[0x15] = G::abilityForSlot5(*personal.info, (pid >> 16) & 1);
    pk[0x17] = 2;
    pk[0x40] = static_cast<uint8_t>(gender << 1);
    pk[0x41] = 3;
    pk[0x5F] = 21;
    pk[0x83] = 4;
    pk[0x84] = 0x85;
    return C::encryptCandidate(pk);
}

struct Outcome {
    bool ok = false;
    std::string error;
    uint16_t species = 0;
    uint32_t pid = 0;
    uint8_t gender = 0;
    std::vector<uint8_t> stagedBefore;
    std::vector<uint8_t> stagedAfter;
};

Outcome toEevee(uint32_t pid, uint8_t gender) {
    std::string error;
    auto record = G::StagedPokemon5Record::create(pikachu(pid, gender), &error, false);
    assert(record);
    assert(record->current().gender() == gender);
    assert(record->stageName(true, "PIKA", &error));
    Outcome out;
    {
        const auto before = record->current();
        const auto bytes = before.decodedBytes();
        out.stagedBefore.assign(bytes.begin(), bytes.end());
    }
    out.ok = record->stage(F::Species, 0, 133, &error);
    out.error = error;
    const auto now = record->current();
    out.species = now.species();
    out.pid = now.pid();
    out.gender = now.gender();
    const auto afterBytes = now.decodedBytes();
    out.stagedAfter.assign(afterBytes.begin(), afterBytes.end());
    return out;
}
}  // namespace

int main() {
    const auto eevee = G::personal5(133, 0, false);
    assert(eevee && eevee.info->genderRatio == 31);

    // Female Pikachu whose PID low byte (0x50) reads male under Eevee's ratio: refused, unchanged.
    {
        const uint32_t pid = 0x12340050u;
        assert(G::genderFromPid5(pid, 127) == 1);
        assert(G::genderFromPid5(pid, 31) == 0);
        const auto r = toEevee(pid, 1);
        assert(!r.ok);
        assert(r.error.find("other gender") != std::string::npos);
        assert(r.species == 25 && r.pid == pid && r.gender == 1);
        assert(r.stagedAfter == r.stagedBefore);
    }

    // Female Pikachu whose low byte (0x10) is female under both ratios: allowed, PID kept.
    {
        const uint32_t pid = 0x12340010u;
        const auto r = toEevee(pid, 1);
        assert(r.ok);
        assert(r.species == 133 && r.pid == pid && r.gender == 1);
        assert(G::genderFromPid5(r.pid, 31) == r.gender);
    }

    // Male Pikachu (0xF0) stays male under Eevee's ratio: allowed.
    {
        const uint32_t pid = 0x123400F0u;
        const auto r = toEevee(pid, 0);
        assert(r.ok);
        assert(r.species == 133 && r.pid == pid && r.gender == 0);
    }

    // Shiny female with an incompatible low byte: still refused, still shiny, PID and bit 16 kept.
    {
        const uint32_t low = 0x0050u;
        const uint32_t high = ((static_cast<uint32_t>(kTid ^ kSid) ^ low) & 0xFFFEu) | 1u;
        const uint32_t pid = (high << 16) | low;
        assert(G::shinyPid5(pid, kTid, kSid));
        const auto r = toEevee(pid, 1);
        assert(!r.ok);
        assert(r.pid == pid && G::shinyPid5(r.pid, kTid, kSid) && G::abilityBit5(r.pid) == 1);
    }

    // A record that already disagrees (low byte 0xDF reads male as Pikachu, stored female) is
    // left as found: the species change neither refuses nor "repairs" the PID.
    {
        const uint32_t pid = 0x13579BDFu;
        assert(G::genderFromPid5(pid, 127) == 0);
        std::string error;
        auto record = G::StagedPokemon5Record::create(pikachu(pid, 1), &error, false);
        assert(record && record->stageName(true, "SPARK", &error));
        assert(record->stage(F::Species, 0, 26, &error));
        assert(record->current().pid() == pid && record->current().gender() == 1);
    }

    std::cout << "Gen V species change keeps PID gender consistent (PKSE 620c5f6 invariant): PASS\n";
}
