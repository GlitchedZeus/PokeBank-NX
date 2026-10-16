// Phase 6: Y re-rolls IVs/EVs in a Gen III / IV / V draft through the REAL native record code
// (Pokemon3FRLG, Pokemon4Mutable, StagedPokemon5Record) using the exact sequence the shared
// surfaces run (SharedStatReroll::reroll, then EVs cleared, IVs set, EVs set).
// 120 presses per draft on shiny and non-shiny drafts and gender-ratio edge cases
// (87.5% male 31 = 1:7, 75% male 63, 50:50 127, 75% female 191, genderless 255, male-only 0,
// female-only 254): every press is new and format-valid, and PID / Shiny / Gender / Nature /
// Ability never change. Format-valid generation only; no legality claim.
#include "Encryption/Encryption4.h"
#include "Integration/Gen5/Gen5PidCoordination.h"
#include "Integration/Gen5/Gen5StagedPokemonRecord.h"
#include "Pokemon/PersonalInfo4PT.h"
#include "Pokemon/PersonalInfoTable.h"
#include "Pokemon/Pokemon3FRLG.h"
#include "Pokemon/Pokemon4Mutable.h"
#include "UI/SharedStatReroll.h"

#include <cassert>
#include <iostream>
#include <set>
#include <string>
#include <vector>

namespace R = PokeBank::UIModel::SharedStatReroll;
namespace G5 = PokeVault::Integration::Gen5;
namespace C5 = PokeVault::Integration::Gen5::Crypto;

namespace {
constexpr uint16_t kTid = 12345, kSid = 54321;
constexpr int kPresses = 120;

void require(bool ok, const std::string& what) {
    if (!ok) std::cerr << "FAILED: " << what << '\n';
    assert(ok);
}

// First species (1..maxSpecies) whose ratio matches; 0 when the generation has none.
template <class RatioOf>
uint16_t speciesWithRatio(uint8_t ratio, uint16_t maxSpecies, RatioOf ratioOf) {
    for (uint16_t s = 1; s <= maxSpecies; ++s)
        if (ratioOf(s) == ratio) return s;
    return 0;
}
constexpr std::array<uint8_t, 7> kRatios{{31, 63, 127, 191, 255, 0, 254}};

std::array<uint8_t, 6> ivs3(const Pokemon::Pokemon3FRLG& p) {
    return {p.ivHP(), p.ivATK(), p.ivDEF(), p.ivSPE(), p.ivSPA(), p.ivSPD()};
}
std::array<uint8_t, 6> evs3(const Pokemon::Pokemon3FRLG& p) {
    return {p.evHP(), p.evATK(), p.evDEF(), p.evSPE(), p.evSPA(), p.evSPD()};
}

void checkStats(const R::Stats& s, const std::string& gen) {
    require(R::valid(s), gen + " re-rolled stats format-valid");
}

void gen3(uint16_t species, bool shiny) {
    std::array<uint8_t, 80> raw{};
    Pokemon::Pokemon3FRLG p(std::span<const std::byte>(
        reinterpret_cast<const std::byte*>(raw.data()), raw.size()));
    p.setPID(0x12345678u);
    p.setTID16(kTid);
    p.setSID16(kSid);
    p.setLanguage(2);
    p.setSpecies(species);
    p.setLevel(10);
    const uint32_t id32 = kTid | (static_cast<uint32_t>(kSid) << 16);
    p.setShiny(shiny, id32);
    const uint32_t pid = p.pid();
    const bool wasShiny = p.isShiny(id32, "");
    require(wasShiny == shiny, "Gen III fixture shiny state");
    const uint8_t gender = p.gender(), nature = p.nature(), ability = p.abilityNumber();
    R::Rng rng(0xC0FFEEu + species + (shiny ? 7u : 0u));
    R::Stats previous{ivs3(p), evs3(p)};
    std::set<std::array<uint8_t, 12>> seen;
    for (int press = 0; press < kPresses; ++press) {
        const auto next = R::reroll(rng, previous);
        require(!(next == previous), "Gen III every press is new");
        checkStats(next, "Gen III");
        for (int i = 0; i < 6; ++i) p.setEV(i, 0);
        for (int i = 0; i < 6; ++i) p.setIV(i, next.ivs[static_cast<std::size_t>(i)]);
        for (int i = 0; i < 6; ++i) p.setEV(i, next.evs[static_cast<std::size_t>(i)]);
        require(ivs3(p) == next.ivs && evs3(p) == next.evs, "Gen III stats stored");
        require(p.pid() == pid && p.isShiny(id32, "") == wasShiny && p.gender() == gender &&
                p.nature() == nature && p.abilityNumber() == ability, "Gen III PID traits kept");
        std::array<uint8_t, 12> key{};
        for (std::size_t i = 0; i < 6; ++i) { key[i] = next.ivs[i]; key[6 + i] = next.evs[i]; }
        seen.insert(key);
        previous = next;
    }
    require(seen.size() > kPresses * 9 / 10, "Gen III re-rolls are varied");
}

std::vector<std::byte> pk4(uint16_t species, uint32_t pid, uint8_t gender) {
    std::vector<std::byte> d(Encryption::SIZE_STORED4, std::byte{0});
    auto w16 = [&](size_t o, uint16_t v) { d[o] = std::byte(v & 0xFF); d[o + 1] = std::byte(v >> 8); };
    w16(0x00, static_cast<uint16_t>(pid)); w16(0x02, static_cast<uint16_t>(pid >> 16));
    w16(0x08, species); w16(0x0C, kTid); w16(0x0E, kSid);
    w16(0x10, 1000);
    d[0x15] = static_cast<std::byte>(Pokemon::getPersonalInfo4PT(species, 0).ability1);
    d[0x17] = std::byte{2};
    w16(0x28, 33);
    d[0x30] = std::byte{35};
    d[0x40] = static_cast<std::byte>(gender << 1);
    d[0x5F] = std::byte{12}; // Platinum
    d[0x84] = std::byte{5};
    return Encryption::encryptArray4(d);
}

void gen4(uint16_t species, bool shiny) {
    const uint8_t ratio = Pokemon::getPersonalInfo4PT(species, 0).genderRatio;
    const uint32_t pid = 0x2468ACE0u;
    const uint8_t gender = ratio == 255 ? 2 : ratio == 254 ? 1 : ratio == 0 ? 0
        : ((pid & 0xFF) < ratio ? 1 : 0);
    std::string error;
    auto mon = Pokemon::Pokemon4Mutable::fromEncrypted(pk4(species, pid, gender), Enums::GameVersion::PT, &error);
    require(mon.has_value(), "Gen IV fixture parses: " + error);
    if (shiny) require(mon->setShiny(true), "Gen IV fixture made shiny");
    const uint32_t lockedPid = mon->pid();
    const bool wasShiny = mon->shiny();
    require(wasShiny == shiny, "Gen IV fixture shiny state");
    const uint8_t g = mon->gender(), n = mon->nature(), a = mon->abilitySlot();
    R::Rng rng(0xBEEFu + species + (shiny ? 11u : 0u));
    R::Stats previous{mon->ivs(), mon->evs()};
    for (int press = 0; press < kPresses; ++press) {
        const auto next = R::reroll(rng, previous);
        require(!(next == previous), "Gen IV every press is new");
        checkStats(next, "Gen IV");
        for (size_t i = 0; i < 6; ++i) require(mon->setEV(i, 0), "Gen IV EV clear");
        for (size_t i = 0; i < 6; ++i) require(mon->setIV(i, next.ivs[i]), "Gen IV IV set");
        for (size_t i = 0; i < 6; ++i) require(mon->setEV(i, next.evs[i]), "Gen IV EV set");
        require(mon->ivs() == next.ivs && mon->evs() == next.evs, "Gen IV stats stored");
        require(mon->pid() == lockedPid && mon->shiny() == wasShiny && mon->gender() == g &&
                mon->nature() == n && mon->abilitySlot() == a, "Gen IV PID traits kept");
        previous = next;
    }
}

std::vector<uint8_t> pk5(uint16_t species, uint32_t pid) {
    const auto personal = G5::personal5(species, 0, true);
    assert(personal);
    std::vector<uint8_t> pk(C5::StoredSize, 0);
    C5::write16(pk, 0, static_cast<uint16_t>(pid));
    C5::write16(pk, 2, static_cast<uint16_t>(pid >> 16));
    C5::write16(pk, 0x08, species);
    C5::write16(pk, 0x0C, kTid);
    C5::write16(pk, 0x0E, kSid);
    C5::write16(pk, 0x10, 1000);
    pk[0x15] = G5::abilityForSlot5(*personal.info, G5::abilityBit5(pid));
    C5::write16(pk, 0x28, 33);
    pk[0x30] = 35;
    pk[0x40] = static_cast<uint8_t>(G5::genderFromPid5(pid, personal.info->genderRatio) << 1);
    pk[0x41] = 3;
    pk[0x5F] = 21; // Black
    pk[0x84] = 5;
    C5::write16(pk, 6, C5::checksum(pk));
    return C5::encryptCandidate(pk);
}

void gen5(uint16_t species, bool shiny) {
    using Field = G5::StagedPokemon5Record::Field;
    std::string error;
    auto r = G5::StagedPokemon5Record::create(pk5(species, 0x13579BDFu), &error, true);
    require(r.has_value(), "Gen V fixture parses: " + error);
    if (shiny) require(r->stage(Field::Shiny, 0, 1, &error), "Gen V fixture made shiny: " + error);
    const auto before = r->current();
    require(before.shiny() == shiny, "Gen V fixture shiny state");
    R::Rng rng(0xFACEu + species + (shiny ? 13u : 0u));
    R::Stats previous{before.ivs(), before.evs()};
    for (int press = 0; press < kPresses; ++press) {
        const auto next = R::reroll(rng, previous);
        require(!(next == previous), "Gen V every press is new");
        checkStats(next, "Gen V");
        for (size_t i = 0; i < 6; ++i) require(r->stage(Field::EV, i, 0, &error), "Gen V EV clear " + error);
        for (size_t i = 0; i < 6; ++i) require(r->stage(Field::IV, i, next.ivs[i], &error), "Gen V IV set " + error);
        for (size_t i = 0; i < 6; ++i) require(r->stage(Field::EV, i, next.evs[i], &error), "Gen V EV set " + error);
        const auto now = r->current();
        require(now.ivs() == next.ivs && now.evs() == next.evs, "Gen V stats stored");
        require(now.pid() == before.pid() && now.shiny() == before.shiny() &&
                now.gender() == before.gender() && now.nature() == before.nature() &&
                now.ability() == before.ability() && now.hiddenAbility() == before.hiddenAbility(),
                "Gen V PID traits kept");
        previous = next;
    }
}
} // namespace

int main() {
    // Pure model: valid, varied, never repeats the previous press.
    R::Rng rng(1);
    R::Stats previous{};
    for (int i = 0; i < 1000; ++i) {
        const auto next = R::reroll(rng, previous);
        require(R::valid(next) && !(next == previous), "model reroll valid and new");
        previous = next;
    }
    int cases = 0;
    for (const uint8_t ratio : kRatios) {
        const uint16_t s3 = speciesWithRatio(ratio, 386, [](uint16_t s) { return Pokemon::getPersonalInfo(s, 0).genderRatio; });
        const uint16_t s4 = speciesWithRatio(ratio, 493, [](uint16_t s) { return Pokemon::getPersonalInfo4PT(s, 0).genderRatio; });
        const uint16_t s5 = speciesWithRatio(ratio, 649, [](uint16_t s) {
            const auto p = G5::personal5(s, 0, true); return p ? p.info->genderRatio : uint8_t{1}; });
        for (const bool shiny : {false, true}) {
            if (s3) { gen3(s3, shiny); ++cases; }
            if (s4) { gen4(s4, shiny); ++cases; }
            if (s5) { gen5(s5, shiny); ++cases; }
        }
    }
    require(cases == 42, "every generation x ratio x shiny case ran");
    std::cout << "Shared Y re-roll (Gen III/IV/V, " << cases << " drafts x " << kPresses
              << " presses, shiny/non-shiny, gender-ratio edges): PASS\n";
}
