// Phase 10 (owner item E11): shiny and non-shiny re-roll mechanics, Gen I-V, through the real
// record code. At least 100 re-rolls per representative shiny and non-shiny example.
//
// What the format itself says about shininess (cited in the headers under test):
//   Gen I   no shiny flag; Gen II shows a Gen I Pokemon shiny from its DVs after Time Capsule
//           (pokecrystal CheckShininess; Gen1Shiny.h, Gen12DVReroll.h).
//   Gen II  shiny = Attack DV in {2,3,6,7,10,11,14,15}, Defense/Speed/Special DV 10; gender is
//           the Attack DV against the species ratio (pokecrystal GetGender); the Unown letter is
//           bits 1-2 of each DV (pokecrystal GetUnownLetter).
//   Gen III-V shiny = (TID ^ SID ^ PID.high ^ PID.low) < 8 (pokeemerald IsShinyOtIdPersonality,
//           PKHeX 6501f0ab ShinyUtil). IVs and EVs are separate fields, so identical IVs/EVs on a
//           shiny and a non-shiny record are legal and expected; nothing here demands otherwise.
// Prior defect reproduced here (fails on the filtered-walk search this replaces): a shiny
// female of an 87.5%-male dual-ability species (Snorlax) changing nature lost its shininess in
// Gen III about one time in ten and was refused in Gen IV about one time in five.
#include "Conversion/Gen3PidSearch.h"
#include "Encryption/Encryption3FRLG.h"
#include "Encryption/Encryption4.h"
#include "Integration/Gen1/Gen1Shiny.h"
#include "Integration/Gen2/Gen2PersonalData.h"
#include "Integration/Gen5/Gen5PidCoordination.h"
#include "Integration/Gen5/Gen5StagedPokemonRecord.h"
#include "Pokemon/PersonalInfo4PT.h"
#include "Pokemon/PersonalInfoTable.h"
#include "Pokemon/Pokemon3FRLG.h"
#include "Pokemon/Pokemon4Mutable.h"
#include "Pokemon/Pokemon4ReadOnly.h"
#include "UI/Gen12DVReroll.h"
#include "UI/SharedStatReroll.h"

#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <set>
#include <span>
#include <string>
#include <vector>

namespace {

namespace DV = PokeBank::UIModel::Gen12DVReroll;
namespace Stat = PokeBank::UIModel::SharedStatReroll;
namespace Search = Conversion::Gen3PidSearch;
namespace Gen1 = PokeVault::Integration::Gen1;
namespace Gen2 = PokeVault::Integration::Gen2;
namespace G5 = PokeVault::Integration::Gen5;
namespace C5 = PokeVault::Integration::Gen5::Crypto;

constexpr int kRerolls = 100;
constexpr uint16_t kTid = 12345, kSid = 54321;
constexpr uint32_t kId32 = kTid | (static_cast<uint32_t>(kSid) << 16);

int failures = 0;
void require(bool ok, const std::string& what) {
    if (!ok) { std::cerr << "FAILED: " << what << '\n'; ++failures; }
    assert(ok);
}

// ---------------------------------------------------------------- Gen I: Time Capsule DVs --
// Gen I has no gender and no flag; the Gen II rule on the four stored DVs decides the shown state.
void gen1(DV::DVs start, uint32_t seed) {
    const bool shiny = DV::isShiny(start);
    require(Gen1::ShinyDVs::isShiny(start) == shiny, "Gen I: Gen1Shiny and Gen12DVReroll agree on the rule");
    DV::DVs dvs = start;
    std::set<uint32_t> seen;
    for (int i = 0; i < kRerolls; ++i) {
        const auto r = DV::reroll(dvs, seed);
        require(r.changed && r.dvs != dvs, "Gen I: every press is a new roll");
        for (const auto v : r.dvs) require(v <= 15, "Gen I: DVs stay 0..15");
        require(DV::isShiny(r.dvs) == shiny, "Gen I: Time Capsule shiny state kept");
        require(Gen1::ShinyDVs::isShiny(r.dvs) == shiny, "Gen I: Gen1Shiny agrees after the roll");
        if (shiny) require(r.dvs[1] == 10 && r.dvs[2] == 10 && r.dvs[3] == 10, "Gen I shiny: Def/Spe/Spc DV 10");
        dvs = r.dvs;
        seen.insert(static_cast<uint32_t>(dvs[0]) << 12 | dvs[1] << 8 | dvs[2] << 4 | dvs[3]);
    }
    require(seen.size() >= (shiny ? 7u : 60u), "Gen I: rolls are varied");
}

// ------------------------------------------------------------------- Gen II: DVs + gender --
void gen2(uint16_t species, DV::DVs start, uint32_t seed, std::size_t minDistinct) {
    const auto genderOf = [species](uint8_t attack) {
        return static_cast<int>(Gen2::genderFromAttackDV(species, attack));
    };
    const bool shiny = DV::isShiny(start);
    const int gender = genderOf(start[0]);
    const int letter = DV::unownLetter(start);
    DV::DVs dvs = start;
    std::set<uint32_t> seen;
    for (int i = 0; i < kRerolls; ++i) {
        const auto r = DV::reroll(dvs, seed, genderOf, species == 201);
        require(r.changed && r.dvs != dvs, "Gen II: every press is a new roll");
        require(DV::isShiny(r.dvs) == shiny, "Gen II: shiny DV pattern kept");
        require(genderOf(r.dvs[0]) == gender, "Gen II: gender (Attack DV vs. ratio) kept");
        if (species == 201) require(DV::unownLetter(r.dvs) == letter, "Gen II: Unown letter kept");
        dvs = r.dvs;
        seen.insert(static_cast<uint32_t>(dvs[0]) << 12 | dvs[1] << 8 | dvs[2] << 4 | dvs[3]);
    }
    require(seen.size() >= minDistinct, "Gen II: rolls are varied");
}

// ------------------------------------------------------------------------------ Gen III --
std::array<uint8_t, 6> ivs3(const Pokemon::Pokemon3FRLG& p) {
    return {p.ivHP(), p.ivATK(), p.ivDEF(), p.ivSPE(), p.ivSPA(), p.ivSPD()};
}
std::array<uint8_t, 6> evs3(const Pokemon::Pokemon3FRLG& p) {
    return {p.evHP(), p.evATK(), p.evDEF(), p.evSPE(), p.evSPA(), p.evSPD()};
}
std::array<uint8_t, 4> pp3(const Pokemon::Pokemon3FRLG& p) {
    return {p.movePP(0), p.movePP(1), p.movePP(2), p.movePP(3)};
}

// pokeemerald CalculateMonStats: HP = (2B + IV + EV/4) * L / 100 + L + 10; other stats
// ((2B + IV + EV/4) * L / 100 + 5) * nature, where nature n raises stat n / 5 and lowers n % 5
// over Atk, Def, Spe, SpA, SpD (gNatureStatTable). Gen IV uses the same formula.
uint16_t expectedStat(int base, int iv, int ev, int level, int statIndex, uint8_t nature) {
    const int common = (2 * base + iv + ev / 4) * level / 100;
    if (statIndex == 0) return static_cast<uint16_t>(common + level + 10);
    const int up = nature / 5, down = nature % 5, which = statIndex - 1;
    int percent = 100;
    if (up != down) {
        if (which == up) percent = 110;
        else if (which == down) percent = 90;
    }
    return static_cast<uint16_t>((common + 5) * percent / 100);
}

struct Pk3Fixture {
    std::array<uint8_t, 100> raw{};
    Pokemon::Pokemon3FRLG mon;
    explicit Pk3Fixture(uint16_t species, uint32_t pid, bool shiny)
        : mon(std::span<const std::byte>(reinterpret_cast<const std::byte*>(raw.data()), raw.size())) {
        mon.setPID(pid);
        mon.setTID16(kTid);
        mon.setSID16(kSid);
        mon.setLanguage(2);
        mon.setSpecies(species);
        mon.setLevel(50);
        mon.setMove(0, 33); // Tackle, 35 PP
        mon.setMovePP(0, 35);
        mon.setShiny(shiny, kId32);
    }
};

void checkStats3(const Pokemon::Pokemon3FRLG& p, const std::string& what) {
    const auto& bs = Pokemon::getPersonalInfoG3(p.speciesID());
    const int base[6] = {bs.hp, bs.atk, bs.def, bs.spe, bs.spa, bs.spd};
    const auto iv = ivs3(p), ev = evs3(p);
    const uint16_t stored[6] = {p.statHPMax(), p.statATK(), p.statDEF(), p.statSPE(), p.statSPA(), p.statSPD()};
    for (int i = 0; i < 6; ++i)
        require(stored[i] == expectedStat(base[i], iv[static_cast<std::size_t>(i)],
                                          ev[static_cast<std::size_t>(i)], p.level(), i, p.nature()),
                what + ": Gen III stat formula");
    require(p.statHPCurrent() <= p.statHPMax(), what + ": current HP within max");
    require(p.checksumValid(), what + ": checksum valid");
}

// The record's PID-linked traits, read back the way the game would.
struct Traits3 {
    bool shiny; uint8_t gender, nature, ability; uint8_t letter; uint8_t wurmple;
    bool operator==(const Traits3&) const = default;
};
Traits3 traits3(const Pokemon::Pokemon3FRLG& p) {
    return {p.isShiny(kId32, ""), p.gender(), p.nature(), p.abilityNumber(),
            p.speciesID() == 201 ? p.form() : uint8_t{0},
            p.speciesID() >= 265 && p.speciesID() <= 269 ? Search::wurmpleEvo(p.pid()) : uint8_t{0}};
}

// 100 trait edits that each change exactly one PID-linked trait and keep the rest.
void gen3Traits(uint16_t species, bool shiny, uint32_t pid) {
    const auto& g3 = Pokemon::getPersonalInfoG3(species);
    const bool dualAbility = g3.ability2 != g3.ability1;
    const uint8_t ratio = Pokemon::getPersonalInfo(species, 0).genderRatio;
    const bool variableGender = ratio != 0 && ratio != 254 && ratio != 255;
    Pk3Fixture f(species, pid, shiny);
    auto& p = f.mon;
    require(p.isShiny(kId32, "") == shiny, "Gen III fixture shiny state (" + std::to_string(species) + ")");
    const auto ivs = ivs3(p), evs = evs3(p);
    const auto pp = pp3(p);
    std::set<uint32_t> pids;
    for (int i = 0; i < kRerolls; ++i) {
        const auto before = traits3(p);
        auto want = before;
        const int what = i % 3;
        if (what == 0) { want.nature = static_cast<uint8_t>((before.nature + 1 + i) % 25); p.setNature(want.nature); }
        else if (what == 1 && variableGender) { want.gender = before.gender == 0 ? 1 : 0; p.setGender(want.gender); }
        else if (what == 1) { p.setGender(before.gender); } // fixed ratio: a no-op request
        else if (dualAbility) { want.ability = before.ability == 1 ? 2 : 1; p.setAbilityNumber(want.ability); }
        else { p.setAbilityNumber(1); }
        const auto after = traits3(p);
        require(after == want, "Gen III edit " + std::to_string(i) + " changed exactly the asked trait (species " +
                                   std::to_string(species) + (shiny ? ", shiny)" : ")"));
        require(after.shiny == shiny, "Gen III: shiny state never yields to another edit");
        require(ivs3(p) == ivs && evs3(p) == evs, "Gen III: IVs/EVs untouched by a PID edit");
        require(pp3(p) == pp && p.movePP(0) == 35, "Gen III: PP untouched by a PID edit");
        require(static_cast<int>((p.iv32() >> 31) & 1) == (dualAbility ? static_cast<int>(p.pid() & 1u) : 0),
                "Gen III: ability bit and PID bit 0 agree (no contradictory record)");
        checkStats3(p, "Gen III after trait edit");
        pids.insert(p.pid());
    }
    require(pids.size() >= 30, "Gen III: trait edits produce varied PIDs");
}

// Already holding the trait: nothing else moves (setShiny on a shiny, same nature).
void gen3NoOp() {
    Pk3Fixture f(143, 0x1234560Fu, true);
    const uint32_t pid = f.mon.pid();
    f.mon.setShiny(true, kId32);
    f.mon.setNature(f.mon.nature());
    f.mon.setGender(f.mon.gender());
    f.mon.regeneratePID(kId32);
    require(f.mon.pid() == pid, "Gen III: a satisfied request leaves the PID alone");
    f.mon.setShiny(false, kId32);
    require(!f.mon.isShiny(kId32, "") && f.mon.pid() != pid && f.mon.gender() == 1, "Gen III: shiny off, gender kept");
}

// The prior defect: a shiny female Snorlax (87.5% male, Immunity / Thick Fat) changing nature.
void gen3PriorDefect() {
    int lostShiny = 0, couldNotBecomeShiny = 0;
    for (uint32_t seed = 1; seed <= kRerolls; ++seed) {
        Pk3Fixture f(143, 0x1000u * seed + 0x0Fu, false); // low byte 0x0F < 31: female
        auto& p = f.mon;
        p.setShiny(true, kId32);
        if (!p.isShiny(kId32, "")) { ++couldNotBecomeShiny; continue; }
        const uint8_t want = static_cast<uint8_t>((p.nature() + 1 + seed) % 25);
        p.setNature(want);
        if (!p.isShiny(kId32, "")) ++lostShiny;
        require(p.nature() == want && p.gender() == 1, "Gen III Snorlax: nature landed, gender kept");
    }
    require(couldNotBecomeShiny == 0, "Gen III: every female Snorlax can be made shiny");
    require(lostShiny == 0, "Gen III: no shiny female Snorlax loses shininess on a nature change");
}

// Gen III Unown: the letter is PID-derived. Every letter is reachable non-shiny; shiny, only the
// letters this TID/SID can show are, and asking for another leaves the record unchanged.
void gen3Unown() {
    for (const bool shiny : {false, true}) {
        Pk3Fixture f(201, 0x0BADF00Du, shiny);
        auto& p = f.mon;
        require(p.isShiny(kId32, "") == shiny, "Unown fixture shiny");
        int reachable = 0;
        for (int round = 0; round < 4; ++round)
            for (uint8_t letter = 0; letter < 28; ++letter) {
                const uint32_t before = p.pid();
                const uint8_t nature = p.nature();
                const bool ok = p.setUnownLetter(letter, kId32);
                Search::Wanted w;
                w.shiny = shiny ? 1 : 0;
                w.unownForm = letter;
                w.tid32 = kId32;
                w.genderRatio = 255;
                const bool possible = Search::construct(before, w).has_value();
                require(ok == possible, "Unown letter " + std::to_string(letter) + ": reachable iff the format allows");
                if (ok) {
                    ++reachable;
                    require(p.form() == letter && p.nature() == nature && p.isShiny(kId32, "") == shiny,
                            "Unown: letter set, nature and shiny kept");
                } else {
                    require(p.pid() == before, "Unown: impossible letter leaves the PID unchanged");
                }
                checkStats3(p, "Unown");
            }
        if (!shiny) require(reachable == 4 * 28, "Unown non-shiny: all 28 letters, 4 rounds");
        else {
            // Independent enumeration: every shiny PID for this TSV is low | ((low ^ TSV ^ k) << 16)
            // with k < 8; the letters those PIDs show are exactly the reachable ones.
            std::set<uint8_t> letters;
            const uint16_t tsv = Search::trainerShinyValue(kId32);
            for (uint32_t low = 0; low < 0x10000u; ++low)
                for (uint32_t k = 0; k < 8; ++k)
                    letters.insert(Search::unownForm(low | (((low ^ tsv ^ k) & 0xFFFFu) << 16)));
            require(reachable == 4 * static_cast<int>(letters.size()), "Unown shiny: exactly the letters this TSV can show");
            require(!letters.empty(), "Unown shiny: at least one letter");
        }
        // 100 nature changes on the Unown keep the letter and the shiny state.
        const uint8_t letter = p.form();
        for (int i = 0; i < kRerolls; ++i) {
            const uint8_t want = static_cast<uint8_t>((p.nature() + 7) % 25);
            p.setNature(want);
            require(p.nature() == want && p.form() == letter && p.isShiny(kId32, "") == shiny,
                    "Unown: nature change keeps letter and shiny");
        }
    }
}

// Wurmple line: the cocoon a Wurmple grows into is (PID >> 16) % 10 < 5; it is kept across
// every trait edit so a Silcoon / Cascoon record never contradicts its own PID.
void gen3Wurmple() {
    for (const uint16_t species : {265, 266, 268}) {
        for (const bool shiny : {false, true}) {
            Pk3Fixture f(species, 0x0F0F0F0Fu, shiny);
            const uint8_t evo = Search::wurmpleEvo(f.mon.pid());
            for (int i = 0; i < kRerolls; ++i) {
                f.mon.setNature(static_cast<uint8_t>((f.mon.nature() + 3) % 25));
                require(Search::wurmpleEvo(f.mon.pid()) == evo && f.mon.isShiny(kId32, "") == shiny,
                        "Gen III Wurmple line: evolution value kept");
            }
        }
    }
}

// Y re-roll of IVs/EVs on shiny and non-shiny Gen III records: identical IV/EV sets are applied to
// both, stats follow the Gen III formula, PID / PP untouched.
void gen3StatReroll(uint16_t species) {
    Pk3Fixture plain(species, 0x2468ACE1u, false), shiny(species, 0x2468ACE1u, true);
    require(!plain.mon.isShiny(kId32, "") && shiny.mon.isShiny(kId32, ""), "Gen III stat fixtures");
    const uint32_t plainPid = plain.mon.pid(), shinyPid = shiny.mon.pid();
    Stat::Rng rng(0xA11CEu + species);
    Stat::Stats previous{ivs3(plain.mon), evs3(plain.mon)};
    for (int i = 0; i < kRerolls; ++i) {
        const auto next = Stat::reroll(rng, previous);
        require(Stat::valid(next) && !(next == previous), "Gen III stat reroll valid and new");
        for (auto* m : {&plain.mon, &shiny.mon}) {
            for (int s = 0; s < 6; ++s) m->setEV(s, 0);
            for (int s = 0; s < 6; ++s) m->setIV(s, next.ivs[static_cast<std::size_t>(s)]);
            for (int s = 0; s < 6; ++s) m->setEV(s, next.evs[static_cast<std::size_t>(s)]);
            require(ivs3(*m) == next.ivs && evs3(*m) == next.evs, "Gen III: identical IVs/EVs stored on both");
            require(m->movePP(0) == 35, "Gen III: PP untouched by IV/EV reroll");
            checkStats3(*m, "Gen III stat reroll");
        }
        require(plain.mon.pid() == plainPid && !plain.mon.isShiny(kId32, ""), "Gen III non-shiny stays non-shiny");
        require(shiny.mon.pid() == shinyPid && shiny.mon.isShiny(kId32, ""), "Gen III shiny stays shiny");
        previous = next;
    }
}

// ------------------------------------------------------------------------------- Gen IV --
std::vector<std::byte> pk4(uint16_t species, uint32_t pid, uint8_t gender) {
    std::vector<std::byte> d(Encryption::SIZE_PARTY4, std::byte{0});
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
    d[0x8C] = std::byte{10};
    return Encryption::encryptArray4(d);
}

uint8_t gender4(uint16_t species, uint32_t pid) {
    const uint8_t ratio = Pokemon::getPersonalInfo4PT(species, 0).genderRatio;
    return ratio == 255 ? 2 : ratio == 254 ? 1 : ratio == 0 ? 0 : ((pid & 0xFF) < ratio ? 1 : 0);
}

Pokemon::Pokemon4Mutable fixture4(uint16_t species, uint32_t pid, bool shiny) {
    std::string error;
    auto mon = Pokemon::Pokemon4Mutable::fromEncrypted(pk4(species, pid, gender4(species, pid)),
                                                       Enums::GameVersion::PT, &error);
    require(mon.has_value(), "Gen IV fixture parses: " + error);
    require(mon->setShiny(shiny), "Gen IV fixture shiny set");
    require(mon->shiny() == shiny, "Gen IV fixture shiny state");
    return *mon;
}

void checkStats4(const Pokemon::Pokemon4Mutable& m, const std::string& what) {
    Pokemon::Pokemon4ReadOnly ro(m.encryptedBytes(), Enums::GameVersion::PT);
    require(ro.valid() && ro.isParty(), what + ": Gen IV record parses as a party record");
    const auto& bs = Pokemon::getPersonalInfo4PT(m.species(), 0);
    const int base[6] = {bs.hp, bs.atk, bs.def, bs.spe, bs.spa, bs.spd};
    const auto iv = m.ivs(), ev = m.evs();
    const auto battle = ro.battleStats();
    const uint16_t stored[6] = {ro.maxHP(), battle[0], battle[1], battle[2], battle[3], battle[4]};
    for (int i = 0; i < 6; ++i)
        require(stored[i] == expectedStat(base[i], iv[static_cast<std::size_t>(i)],
                                          ev[static_cast<std::size_t>(i)], m.level(), i, m.nature()),
                what + ": Gen IV stat formula");
    require(ro.currentHP() <= ro.maxHP(), what + ": Gen IV current HP within max");
    require(ro.gender() == gender4(m.species(), m.pid()), what + ": stored gender bits agree with the PID");
}

struct Traits4 {
    bool shiny; uint8_t gender, nature, ability; uint8_t wurmple;
    bool operator==(const Traits4&) const = default;
};
Traits4 traits4(const Pokemon::Pokemon4Mutable& m) {
    return {m.shiny(), m.gender(), m.nature(), m.abilitySlot(),
            m.species() >= 265 && m.species() <= 269 ? Search::wurmpleEvo(m.pid()) : uint8_t{0}};
}

void gen4Traits(uint16_t species, bool shiny, uint32_t pid) {
    const auto& personal = Pokemon::getPersonalInfo4PT(species, 0);
    const bool dualAbility = personal.ability2 != 0 && personal.ability2 != personal.ability1;
    const bool variableGender = personal.genderRatio != 0 && personal.genderRatio != 254 && personal.genderRatio != 255;
    auto m = fixture4(species, pid, shiny);
    const auto ivs = m.ivs(), evs = m.evs();
    const auto pp = m.pp();
    std::set<uint32_t> pids;
    for (int i = 0; i < kRerolls; ++i) {
        const auto before = traits4(m);
        auto want = before;
        const int what = i % 3;
        bool ok = true;
        if (what == 0) { want.nature = static_cast<uint8_t>((before.nature + 1 + i) % 25); ok = m.setNature(want.nature); }
        else if (what == 1 && variableGender) { want.gender = before.gender == 0 ? 1 : 0; ok = m.setGender(want.gender); }
        else if (what == 1) { ok = m.setGender(before.gender); }
        else if (dualAbility) { want.ability = before.ability == 0 ? 1 : 0; ok = m.setAbilitySlot(want.ability); }
        else { ok = m.setAbilitySlot(0); }
        require(ok, "Gen IV edit " + std::to_string(i) + " accepted (species " + std::to_string(species) + ")");
        require(traits4(m) == want, "Gen IV edit changed exactly the asked trait");
        require(m.shiny() == shiny, "Gen IV: shiny state never yields");
        require(m.ivs() == ivs && m.evs() == evs && m.pp() == pp, "Gen IV: IVs/EVs/PP untouched by a PID edit");
        checkStats4(m, "Gen IV after trait edit");
        pids.insert(m.pid());
    }
    require(pids.size() >= 30, "Gen IV: trait edits produce varied PIDs");
}

void gen4PriorDefect() {
    int refused = 0;
    for (uint32_t seed = 1; seed <= kRerolls; ++seed) {
        std::string error;
        auto m = Pokemon::Pokemon4Mutable::fromEncrypted(pk4(143, 0x1000u * seed + 0x0Fu, 1),
                                                         Enums::GameVersion::PT, &error);
        require(m.has_value(), "Gen IV Snorlax parses");
        if (!m->setShiny(true)) { ++refused; continue; }
        const uint8_t want = static_cast<uint8_t>((m->nature() + 1 + seed) % 25);
        if (!m->setNature(want)) { ++refused; continue; }
        require(m->shiny() && m->gender() == 1 && m->nature() == want, "Gen IV Snorlax traits");
    }
    require(refused == 0, "Gen IV: no shiny female Snorlax edit is refused");
}

void gen4NoOp() {
    auto m = fixture4(143, 0x1234560Fu, true);
    const uint32_t pid = m.pid();
    require(m.setShiny(true) && m.setNature(m.nature()) && m.setGender(m.gender()) && m.pid() == pid,
            "Gen IV: a satisfied request leaves the PID alone");
    require(m.setShiny(false) && !m.shiny() && m.gender() == 1 && m.pid() != pid, "Gen IV: shiny off keeps gender");
}

void gen4StatReroll(uint16_t species) {
    auto plain = fixture4(species, 0x2468ACE1u, false), shiny = fixture4(species, 0x2468ACE1u, true);
    const uint32_t plainPid = plain.pid(), shinyPid = shiny.pid();
    Stat::Rng rng(0xB0B0u + species);
    Stat::Stats previous{plain.ivs(), plain.evs()};
    for (int i = 0; i < kRerolls; ++i) {
        const auto next = Stat::reroll(rng, previous);
        require(Stat::valid(next) && !(next == previous), "Gen IV stat reroll valid and new");
        for (auto* m : {&plain, &shiny}) {
            for (size_t s = 0; s < 6; ++s) require(m->setEV(s, 0), "Gen IV EV clear");
            for (size_t s = 0; s < 6; ++s) require(m->setIV(s, next.ivs[s]), "Gen IV IV set");
            for (size_t s = 0; s < 6; ++s) require(m->setEV(s, next.evs[s]), "Gen IV EV set");
            require(m->ivs() == next.ivs && m->evs() == next.evs, "Gen IV: identical IVs/EVs stored on both");
            require(m->pp()[0] == 35, "Gen IV: PP untouched by IV/EV reroll");
            checkStats4(*m, "Gen IV stat reroll");
        }
        require(plain.pid() == plainPid && !plain.shiny() && shiny.pid() == shinyPid && shiny.shiny(),
                "Gen IV: shiny state is the PID's, not the stats'");
        previous = next;
    }
}

// -------------------------------------------------------------------------------- Gen V --
std::vector<uint8_t> pk5(uint16_t species, uint32_t pid, uint8_t origin) {
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
    pk[0x5F] = origin;
    pk[0x84] = 5;
    C5::write16(pk, 6, C5::checksum(pk));
    return C5::encryptCandidate(pk);
}

// Pure PID rule: 100 PIDs per TID/SID pair; the shiny toggle keeps the low half (gender), bit 16
// (Gen V ability slot) and, being outside the PID, the nature byte; off again is one bit away.
void gen5PidRule() {
    uint32_t x = 0x9E3779B9u;
    const auto next = [&x]() { x ^= x << 13; x ^= x >> 17; x ^= x << 5; return x; };
    for (int i = 0; i < kRerolls; ++i) {
        const uint32_t pid = next();
        const uint16_t tid = static_cast<uint16_t>(next()), sid = static_cast<uint16_t>(next());
        const uint32_t on = G5::withShiny5(pid, tid, sid, true);
        require(G5::shinyPid5(on, tid, sid), "Gen V: shiny on");
        require((on & 0xFFFFu) == (pid & 0xFFFFu), "Gen V: low half (gender byte) kept");
        require(G5::abilityBit5(on) == G5::abilityBit5(pid), "Gen V: ability bit 16 kept");
        const uint32_t off = G5::withShiny5(on, tid, sid, false);
        require(!G5::shinyPid5(off, tid, sid) && (off & 0xFFFFu) == (pid & 0xFFFFu) &&
                G5::abilityBit5(off) == G5::abilityBit5(pid), "Gen V: shiny off keeps gender and ability");
        require(G5::withShiny5(on, tid, sid, true) == on && G5::withShiny5(off, tid, sid, false) == off,
                "Gen V: a satisfied request is the identity");
        const uint16_t tid2 = static_cast<uint16_t>(next());
        const uint32_t kept = G5::keepShinyForIds5(on, tid, sid, tid2, sid);
        require(G5::shinyPid5(kept, tid2, sid) && (kept & 0xFFFFu) == (pid & 0xFFFFu), "Gen V: TID change keeps shiny");
    }
}

// Through the staged record: 100 shiny toggles, then 100 gender and ability edits on shiny and
// non-shiny Gen V-origin records; an earlier-origin record refuses PID edits instead of changing.
void gen5Record(uint16_t species, bool shiny) {
    using Field = G5::StagedPokemon5Record::Field;
    std::string error;
    auto r = G5::StagedPokemon5Record::create(pk5(species, 0x13579BDFu, 21), &error, true);
    require(r.has_value(), "Gen V fixture parses: " + error);
    if (shiny) require(r->stage(Field::Shiny, 0, 1, &error), "Gen V fixture shiny: " + error);
    const auto start = r->current();
    require(start.shiny() == shiny, "Gen V fixture shiny state");
    const auto ivs = start.ivs(), evs = start.evs();
    const auto pp = start.pp();
    for (int i = 0; i < kRerolls; ++i) {
        const auto before = r->current();
        const bool toggled = !before.shiny();
        require(r->stage(Field::Shiny, 0, toggled ? 1u : 0u, &error), "Gen V shiny toggle: " + error);
        const auto after = r->current();
        require(after.shiny() == toggled, "Gen V: shiny toggled");
        require(after.gender() == before.gender() && after.nature() == before.nature() &&
                after.ability() == before.ability() && after.hiddenAbility() == before.hiddenAbility() &&
                after.ivs() == ivs && after.evs() == evs && after.pp() == pp && after.form() == before.form(),
                "Gen V: shiny toggle keeps gender, nature, ability, IVs/EVs, PP, form");
        require(after.checksumValid(), "Gen V: checksum valid after the toggle");
    }
    require(r->current().shiny() == shiny, "Gen V: back at the fixture's state after 100 toggles");
    const auto personal = G5::personal5(species, 0, true);
    const bool variableGender = G5::variableGenderRatio5(personal.info->genderRatio);
    const bool dualAbility = personal.info->ability2 != 0 && personal.info->ability2 != personal.info->ability1;
    for (int i = 0; i < kRerolls; ++i) {
        const auto before = r->current();
        if (i % 2 == 0 && variableGender) {
            const uint8_t g = before.gender() == 0 ? 1 : 0;
            require(r->stage(Field::Gender, 0, g, &error), "Gen V gender edit: " + error);
            const auto after = r->current();
            require(after.gender() == g && G5::genderFromPid5(after.pid(), personal.info->genderRatio) == g,
                    "Gen V: stored gender and PID gender agree");
            require(after.shiny() == shiny && after.ability() == before.ability() && after.nature() == before.nature(),
                    "Gen V: gender edit keeps shiny, ability, nature");
        } else if (dualAbility) {
            const uint8_t slot = static_cast<uint8_t>(G5::abilityBit5(before.pid()) ^ 1u);
            require(r->stage(Field::AbilitySlot, 0, slot, &error), "Gen V ability edit: " + error);
            const auto after = r->current();
            require(G5::abilityBit5(after.pid()) == slot && after.shiny() == shiny && after.gender() == before.gender(),
                    "Gen V: ability edit keeps shiny and gender");
        }
        require(r->current().ivs() == ivs && r->current().evs() == evs && r->current().pp() == pp,
                "Gen V: IVs/EVs/PP untouched by PID edits");
    }
    // Y stat reroll: identical IV/EV sets on this record regardless of its shiny state.
    Stat::Rng rng(0x5EEDu + species + (shiny ? 1u : 0u));
    Stat::Stats previous{ivs, evs};
    const uint32_t pid = r->current().pid();
    for (int i = 0; i < kRerolls; ++i) {
        const auto next = Stat::reroll(rng, previous);
        for (size_t s = 0; s < 6; ++s) require(r->stage(Field::EV, s, 0, &error), "Gen V EV clear " + error);
        for (size_t s = 0; s < 6; ++s) require(r->stage(Field::IV, s, next.ivs[s], &error), "Gen V IV set " + error);
        for (size_t s = 0; s < 6; ++s) require(r->stage(Field::EV, s, next.evs[s], &error), "Gen V EV set " + error);
        const auto now = r->current();
        require(now.ivs() == next.ivs && now.evs() == next.evs && now.pid() == pid && now.shiny() == shiny &&
                now.pp() == pp, "Gen V: stat reroll keeps PID, shiny, PP");
        previous = next;
    }
}

void gen5EarlierOrigin() {
    using Field = G5::StagedPokemon5Record::Field;
    std::string error;
    auto r = G5::StagedPokemon5Record::create(pk5(143, 0x13579BDFu, 12), &error, true); // Platinum origin
    require(r.has_value(), "Gen V earlier-origin fixture: " + error);
    const auto before = r->current();
    require(!r->stage(Field::Shiny, 0, before.shiny() ? 0u : 1u, &error), "Gen V: earlier-origin shiny edit refused");
    require(r->current().pid() == before.pid(), "Gen V: refusal leaves the PID untouched");
}

} // namespace

int main() {
    // Gen I: non-shiny and Time-Capsule-shiny DV sets.
    uint32_t seed = 0x5EED1u;
    gen1({9, 4, 12, 1}, seed);
    gen1({3, 10, 10, 10}, seed);
    // A Gen I record's "make shiny" is the same rule; the fallback to normal changes one DV.
    require(Gen1::ShinyDVs::isShiny(Gen1::ShinyDVs::makeShiny({9, 4, 12, 1})) &&
            !Gen1::ShinyDVs::isShiny(Gen1::ShinyDVs::makeNormalFallback({3, 10, 10, 10})), "Gen I make shiny / normal");

    // Gen II: 1:7 Eevee (31), 3:1 Growlithe (63), 1:1 Pikachu (127), 7:1-female Clefairy (191),
    // male-only Tauros (0), female-only Chansey (254), genderless Magnemite (255), Unown.
    struct Case2 { uint16_t species; DV::DVs start; std::size_t distinct; };
    const Case2 cases2[] = {
        {133, {12, 7, 4, 9}, 50}, {133, {1, 7, 4, 9}, 50}, {133, {2, 10, 10, 10}, 7},
        {58, {4, 1, 1, 1}, 50},   {58, {3, 1, 1, 1}, 50},  {58, {6, 10, 10, 10}, 5},  {58, {3, 10, 10, 10}, 1},
        {25, {8, 3, 3, 3}, 50},   {25, {7, 3, 3, 3}, 50},  {25, {10, 10, 10, 10}, 3}, {25, {2, 10, 10, 10}, 3},
        {35, {12, 5, 5, 5}, 50},  {35, {11, 5, 5, 5}, 50}, {35, {14, 10, 10, 10}, 1}, {35, {3, 10, 10, 10}, 5},
        {128, {5, 5, 5, 5}, 50},  {113, {5, 5, 5, 5}, 50}, {81, {5, 5, 5, 5}, 50},    {81, {7, 10, 10, 10}, 7},
        {201, {4, 9, 1, 12}, 20}, {201, {2, 10, 10, 10}, 3}, {201, {6, 10, 10, 10}, 3},
    };
    for (const auto& c : cases2) gen2(c.species, c.start, seed, c.distinct);
    // Shiny Gen II Unown can only be I (Attack 2,3,10,11) or V (6,7,14,15): bits 1-2 decide.
    require(DV::unownLetter({2, 10, 10, 10}) == 8 && DV::unownLetter({6, 10, 10, 10}) == 21, "Gen II shiny Unown is I or V");
    // 1:7 female cannot be shiny: every shiny Attack DV is >= 2 and female needs 0 or 1.
    require(Gen2::genderFromAttackDV(133, 2) == Gen2::PokemonGender::Male, "Gen II 1:7 shiny is always male");

    // Gen III: Snorlax 143 (31, Immunity/Thick Fat), Pikachu 25 (127, Static only), Clefairy 35
    // (191, Cute Charm only), Chansey 113 (254, Natural Cure/Serene Grace), Tauros 128 (0),
    // Magnemite 81 (255, Magnet Pull/Sturdy), Unown 201; ability/gender facts come from the tables.
    for (const bool shiny : {false, true}) {
        gen3Traits(143, shiny, 0x1234560Fu);
        gen3Traits(25, shiny, 0x1234567Fu);
        gen3Traits(35, shiny, 0x1234567Fu);
        gen3Traits(113, shiny, 0x1234567Fu);
        gen3Traits(128, shiny, 0x1234567Fu);
        gen3Traits(81, shiny, 0x1234567Fu);
        gen3StatReroll(143);
        gen3StatReroll(25);
    }
    gen3NoOp();
    gen3PriorDefect();
    gen3Unown();
    gen3Wurmple();

    // Gen IV (Platinum personal): Snorlax 143 (31, Immunity/Thick Fat), Pikachu 25 (127, Static
    // only), Chansey 113, Tauros 128, Magnemite 81.
    for (const bool shiny : {false, true}) {
        gen4Traits(143, shiny, 0x1234560Fu);
        gen4Traits(25, shiny, 0x1234567Fu);
        gen4Traits(113, shiny, 0x1234567Fu);
        gen4Traits(128, shiny, 0x1234567Fu);
        gen4Traits(81, shiny, 0x1234567Fu);
        gen4StatReroll(143);
    }
    gen4NoOp();
    gen4PriorDefect();

    // Gen V: Snivy 495 (31, Overgrow only), Pikachu 25 (127), Audino 531 (127, Healer/Regenerator),
    // Chansey 113, Tauros 128, Magnemite 81.
    gen5PidRule();
    for (const bool shiny : {false, true}) {
        gen5Record(495, shiny);
        gen5Record(25, shiny);
        gen5Record(531, shiny);
        gen5Record(113, shiny);
        gen5Record(128, shiny);
        gen5Record(81, shiny);
    }
    gen5EarlierOrigin();

    require(failures == 0, "all re-roll mechanics checks");
    std::cout << "Reroll mechanics Gen I-V (" << kRerolls << "+ re-rolls per shiny / non-shiny example; "
                 "DV links, PID links, stats, PP, no contradictory records): PASS\n";
    return 0;
}
