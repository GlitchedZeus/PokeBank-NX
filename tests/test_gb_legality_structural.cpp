// Structural Gen I/II rules that a corpus of game-built records never exercises: duplicate
// moves, the stored level against EXP, and EXP above the level-100 total.
//
// Each was missed by the PKHeX differential because its corpora only produced records the
// games could make (moves were de-duplicated, EXP always matched the level). The records
// below were rebuilt from the defect descriptions of an independent audit; the PKHeX
// verdicts quoted are from tools/pkhex_gb_diff with kwsch/PKHeX@6501f0ab.
//
//  1. GB engine (GB::checkGen1/checkGen2, GB::formatProblems): duplicate moves in every
//     slot pattern, malformed packing, a legal one-move record; a level/EXP grid over the
//     four Gen I/II growth rates whose expected answers come from the games' own EXP
//     formulas, not from PokeBank NX's tables.
//  2. Production analyzer on a real, checksum-valid Red save: a level-100 Bulbasaur with
//     exactly the level-100 EXP total, one more, one less. The save bytes must not change.
//  3. Production analyzer on native PK2 records (Pokemon2ReadOnly).
//  4. The party max-HP rule: the engine against the games' CalcStat run over every stat
//     experience value up to the stored one (base HP from the PKSM port, independent of
//     the pret-generated GBBaseStats.inc), and the production analyzer on party records.
#include "Integration/Gen1/Gen1ReadOnlySave.h"
#include "Integration/Gen2/Gen2PersonalData.h"
#include "Integration/Gen2/Gen2ReadOnlySave.h"
#include "Legality/GBLegality.h"
#include "Legality/Legality.h"
#include "Names/ItemNames.h"
#include "Names/SpeciesNames.h"
#include "Pokemon/Experience.h"
#include "Pokemon/Pokemon1ReadOnly.h"
#include "Pokemon/Pokemon2ReadOnly.h"

#include <array>
#include <bitset>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <span>
#include <string>
#include <vector>

namespace Trainer {
const char* getSpeciesName(uint16_t id) { return Names::getSpeciesName(id); }
const char* getItemName(uint16_t id) { return Names::getItemName(id); }
}

namespace {

namespace GB = Legality::GB;

int checks = 0, failures = 0;

void check(bool ok, const std::string& what) {
    ++checks;
    if (!ok) {
        ++failures;
        std::fprintf(stderr, "FAIL %s\n", what.c_str());
    }
}

bool hasText(const std::vector<GB::Problem>& problems, const std::string& needle) {
    for (const auto& p : problems)
        if (p.text.find(needle) != std::string::npos && p.definitive)
            return true;
    return false;
}

// A non-definitive problem (a Warning on the production path).
bool hasWarning(const std::vector<GB::Problem>& problems, const std::string& needle) {
    for (const auto& p : problems)
        if (p.text.find(needle) != std::string::npos && !p.definitive)
            return true;
    return false;
}

bool hasIssue(const Legality::Report& r, Legality::Severity s, const std::string& needle) {
    for (const auto& issue : r.issues)
        if (issue.severity == s && issue.text.find(needle) != std::string::npos)
            return true;
    return false;
}

// ---------------------------------------------------------------- the games' EXP formulas

// pokered engine/battle/experience.asm / pokecrystal engine/pokemon/experience.asm
// CalcExpAtLevel, with the Gen II growth-rate codes (0 medium fast, 3 medium slow, 4 fast,
// 5 slow). Level 1 is 0 for every rate.
enum Growth : uint8_t { MediumFast = 0, MediumSlow = 3, Fast = 4, Slow = 5 };

uint32_t gameExp(Growth g, int n) {
    if (n <= 1)
        return 0;
    const int64_t c = static_cast<int64_t>(n) * n * n;
    switch (g) {
        case MediumSlow: return static_cast<uint32_t>(6 * c / 5 - 15 * n * n + 100 * n - 140);
        case Fast: return static_cast<uint32_t>(4 * c / 5);
        case Slow: return static_cast<uint32_t>(5 * c / 4);
        default: return static_cast<uint32_t>(c);
    }
}

int gameLevel(Growth g, uint32_t exp) {
    int level = 1;
    while (level < 100 && exp >= gameExp(g, level + 1))
        ++level;
    return level;
}

struct Species {
    uint8_t national;
    Growth growth;  // from the pret base stats
    uint8_t nxGrowth;  // the same, in Pokemon::getGrowthRate's numbering
};
// Rattata medium fast, Bulbasaur medium slow, Clefairy fast, Magikarp slow.
constexpr std::array<Species, 4> kSpecies{{{19, MediumFast, 0}, {1, MediumSlow, 3}, {35, Fast, 4}, {129, Slow, 5}}};

// ---------------------------------------------------------------- 1. GB engine

GB::Entity2 bulbasaur2(std::array<uint8_t, 4> moves) {
    GB::Entity2 e;
    e.species = 1;
    e.level = 5;
    e.exp = gameExp(MediumSlow, 5);
    e.moves = moves;
    for (int i = 0; i < 4; ++i)  // full PP for each move present (Tackle 35, Growl 40)
        e.pp[i] = moves[i] == 33 ? 35 : moves[i] == 45 ? 40 : moves[i] == 0 ? 0 : 20;
    e.dvs = {8, 8, 8, 8};
    e.tid = 1234;
    e.friendship = 120;
    e.otName = "GOLD";
    e.nickname = "BULBA";
    e.language = GB::Language::International;
    return e;
}

GB::Entity1 bulbasaur1(std::array<uint8_t, 4> moves) {
    GB::Entity1 e;
    e.species = 1;
    e.level = 10;
    e.exp = gameExp(MediumSlow, 10);
    e.moves = moves;
    e.pp = {35, 40, 0, 0};
    e.catchRate = 45;
    e.type1 = 0x16;
    e.type2 = 0x03;
    e.dvs = {8, 8, 8, 8};
    e.tid = 1234;
    e.otName = "ASH";
    e.nickname = "BULBA";
    e.language = GB::Language::International;
    return e;
}

void duplicateMoves() {
    struct Case {
        std::array<uint8_t, 4> moves;
        bool legal;          // PKHeX: VALID only for the first two
        const char* fragment;
    };
    const Case cases[] = {
        {{33, 45, 0, 0}, true, ""},
        {{33, 0, 0, 0}, true, ""},  // the Move Deleter can leave one move
        {{33, 33, 0, 0}, false, "Duplicate move: Tackle"},
        {{33, 45, 33, 0}, false, "Duplicate move: Tackle"},
        {{45, 33, 33, 0}, false, "Duplicate move: Tackle"},
        {{33, 45, 45, 0}, false, "Duplicate move: Growl"},
        {{33, 45, 22, 33}, false, "Duplicate move: Tackle"},
        {{45, 45, 45, 45}, false, "Duplicate move: Growl"},
        {{33, 0, 45, 0}, false, "Moves are not packed"},
        {{0, 33, 0, 0}, false, "at least one move"},  // reported before packing
        {{0, 0, 0, 0}, false, "at least one move"},
    };
    for (const Case& c : cases) {
        const std::string name = "PK2 moves " + std::to_string(c.moves[0]) + "," + std::to_string(c.moves[1]) + "," +
                                 std::to_string(c.moves[2]) + "," + std::to_string(c.moves[3]);
        const GB::Result r = GB::checkGen2(bulbasaur2(c.moves));
        check((r.status == GB::Status::Legal) == c.legal, name + ": status");
        if (!c.legal)
            check(hasText(r.problems, c.fragment), name + ": " + c.fragment);
    }
    // Gen I: the same rule (PKHeX Move1:Duplicate on PK1).
    for (const auto& moves : {std::array<uint8_t, 4>{33, 33, 0, 0}, std::array<uint8_t, 4>{33, 45, 33, 0},
                              std::array<uint8_t, 4>{33, 45, 45, 33}}) {
        const GB::Result r = GB::checkGen1(bulbasaur1(moves));
        check(r.status == GB::Status::Illegal && hasText(r.problems, "Duplicate move"), "PK1 duplicate moves");
    }
    check(GB::checkGen1(bulbasaur1({33, 45, 0, 0})).status == GB::Status::Legal, "PK1 Tackle/Growl level 10");
}

void levelExpGrid() {
    const int levels[] = {2, 5, 6, 49, 50, 51, 98, 99, 100};
    for (const Species& s : kSpecies) {
        check(Pokemon::getGrowthRate(s.national) == s.nxGrowth, "growth rate of species " + std::to_string(s.national));
        const uint32_t max = gameExp(s.growth, 100);
        check(Pokemon::getExpForLevel(100, s.nxGrowth) == max, "level-100 total of species " + std::to_string(s.national));
        for (int level : levels) {
            const uint32_t at = gameExp(s.growth, level), next = gameExp(s.growth, std::min(100, level + 1));
            // kDayCareReach: the Day Care holds the top EXP byte at $50 (pokered
            // IncrementDayCareMonExp, pokecrystal DayCareStep), so $50FFFF is the most a
            // Pokemon still in the Day Care can carry.
            constexpr uint32_t kDayCareReach = 0x50FFFF;
            const uint32_t exps[] = {0, at > 0 ? at - 1 : 0, at, at + 1, next > 0 ? next - 1 : 0, next, max - 1, max,
                                     max + 1, max + 1000000, kDayCareReach, kDayCareReach + 1, 0xFFFFFF};
            for (uint32_t exp : exps) {
                const bool above = exp > max;
                const int expLevel = gameLevel(s.growth, exp);
                // The stored level ahead of its EXP: impossible in both games.
                const bool ahead = !above && expLevel < level;
                // EXP ahead of the stored level: a Pokemon still in the Day Care.
                const bool stale = !above && expLevel > level;
                // EXP above the level-100 total within the Day Care's reach: Gen I adds it at any
                // stored level, Gen II only while the stored level is below 100.
                const bool dayCare1 = above && exp <= kDayCareReach;
                const bool dayCare2 = dayCare1 && level < 100;
                const std::string name = "species " + std::to_string(s.national) + " level " + std::to_string(level) +
                                         " EXP " + std::to_string(exp);
                GB::Entity2 e2;
                e2.species = s.national;
                e2.level = static_cast<uint8_t>(level);
                e2.exp = exp;
                e2.nickname = "MON";
                const auto p2 = GB::formatProblems(e2);
                check(hasText(p2, "above the level-100 total") == (above && !dayCare2), name + " PK2 cap");
                check(hasWarning(p2, "above the level-100 total") == dayCare2, name + " PK2 cap (Day Care)");
                check(hasText(p2, "does not match EXP") == ahead, name + " PK2 level ahead");
                check(hasWarning(p2, "is below the level the EXP gives") == stale, name + " PK2 stale level");
                GB::Entity1 e1;
                e1.species = s.national;
                e1.level = static_cast<uint8_t>(level);
                e1.exp = exp;
                e1.nickname = "MON";
                const auto p1 = GB::formatProblems(e1);
                check(hasText(p1, "above the level-100 total") == (above && !dayCare1), name + " PK1 cap");
                check(hasWarning(p1, "above the level-100 total") == dayCare1, name + " PK1 cap (Day Care)");
                check(hasText(p1, "does not match EXP") == ahead, name + " PK1 level ahead");
                check(hasWarning(p1, "is below the level the EXP gives") == stale, name + " PK1 stale level");
                // Exactly one level/EXP problem, and never both a definitive and a tentative one.
                int count1 = 0, count2 = 0;
                for (const auto& p : p1)
                    count1 += p.area == GB::Area::Stats;
                for (const auto& p : p2)
                    count2 += p.area == GB::Area::Stats;
                check(count1 == (above || ahead || stale ? 1 : 0), name + " PK1 one level/EXP problem");
                check(count2 == (above || ahead || stale ? 1 : 0), name + " PK2 one level/EXP problem");
                // A party or PC box slot of the source save is never out of step (pokered
                // _MoveMon and pokecrystal SendGetMonIntoFromBox recompute the level from EXP
                // on every return to the party): the Day Care exceptions close and turn
                // definitive. The Day Care's own slot carries them, so nothing is reported.
                // The level ahead of its EXP and EXP past the Day Care's reach stay Invalid
                // in every slot.
                for (const GB::Storage storage : {GB::Storage::PartyOrBox, GB::Storage::DayCare}) {
                    const bool box = storage == GB::Storage::PartyOrBox;
                    const std::string where = box ? " (party/box)" : " (Day Care slot)";
                    GB::Entity2 s2 = e2;
                    s2.storage = storage;
                    const auto q2 = GB::formatProblems(s2);
                    check(hasText(q2, "above the level-100 total") == (above && (!dayCare2 || box)), name + " PK2 cap" + where);
                    check(!hasWarning(q2, "above the level-100 total"), name + " PK2 cap not tentative" + where);
                    check(hasText(q2, "does not match EXP") == ahead, name + " PK2 level ahead" + where);
                    check(hasText(q2, "is below the level the EXP gives") == (stale && box), name + " PK2 stale level" + where);
                    check(!hasWarning(q2, "is below the level the EXP gives"), name + " PK2 stale not tentative" + where);
                    GB::Entity1 s1 = e1;
                    s1.storage = storage;
                    const auto q1 = GB::formatProblems(s1);
                    check(hasText(q1, "above the level-100 total") == (above && (!dayCare1 || box)), name + " PK1 cap" + where);
                    check(!hasWarning(q1, "above the level-100 total"), name + " PK1 cap not tentative" + where);
                    check(hasText(q1, "does not match EXP") == ahead, name + " PK1 level ahead" + where);
                    check(hasText(q1, "is below the level the EXP gives") == (stale && box), name + " PK1 stale level" + where);
                    check(!hasWarning(q1, "is below the level the EXP gives"), name + " PK1 stale not tentative" + where);
                }
            }
        }
    }
    // An unhatched egg is judged by its encounter instead (checkGen2): a daycare egg carries
    // its level-5 total, a Crystal Odd Egg always 125.
    GB::Entity2 egg;
    egg.species = 172;  // Pichu, an Odd Egg species
    egg.level = 5;
    egg.exp = 125;
    egg.isEgg = true;
    egg.nickname = "EGG";
    check(!hasText(GB::formatProblems(egg), "EXP"), "unhatched egg: no level/EXP problem from the format rules");
    // checkGen2 (the PKHeX-verdict corpus) on the reported records.
    GB::Entity2 zero = bulbasaur2({33, 45, 0, 0});
    zero.level = 50;
    zero.exp = 0;
    check(GB::checkGen2(zero).status == GB::Status::Illegal, "level-50 Bulbasaur with 0 EXP (PKHeX INVALID)");
    GB::Entity2 capped = bulbasaur2({33, 45, 0, 0});
    capped.level = 100;
    capped.exp = gameExp(MediumSlow, 100);
    check(!hasText(GB::checkGen2(capped).problems, "EXP"), "level-100 Bulbasaur at the cap");
    capped.exp += 1;
    check(GB::checkGen2(capped).status == GB::Status::Illegal &&
              hasText(GB::checkGen2(capped).problems, "above the level-100 total"),
          "level-100 Bulbasaur one EXP above the cap (PKHeX LevelEXPTooHigh; DayCareStep skips level 100)");
    // The same EXP on a level-99 deposit is what DayCareStep produces: Uncertain, not Illegal.
    GB::Entity2 deposited = capped;
    deposited.level = 99;
    const GB::Result dep = GB::checkGen2(deposited);
    check(dep.status == GB::Status::Uncertain && hasWarning(dep.problems, "above the level-100 total"),
          "level-99 Bulbasaur one EXP above the cap (Day Care slot)");
    deposited.exp = 0x510000;
    check(GB::checkGen2(deposited).status == GB::Status::Illegal, "level-99 Bulbasaur past the Day Care's reach");
    // EXP ahead of the stored level, below the cap: a stale Day Care level (Uncertain).
    GB::Entity2 stale = bulbasaur2({33, 45, 0, 0});
    stale.level = 5;
    stale.exp = gameExp(MediumSlow, 30);
    const GB::Result st = GB::checkGen2(stale);
    check(st.status == GB::Status::Uncertain && hasWarning(st.problems, "is below the level the EXP gives"),
          "level-5 Bulbasaur with the level-30 total (Day Care slot)");
    // Gen I: a level-100 deposit keeps gaining EXP (no level check), so the same record is
    // Uncertain as a PK1 up to $50FFFF and Illegal past it.
    GB::Entity1 dep1 = bulbasaur1({33, 45, 0, 0});
    dep1.level = 100;
    dep1.exp = gameExp(MediumSlow, 100) + 1;
    check(GB::checkGen1(dep1).status == GB::Status::Uncertain, "PK1 level 100 one EXP above the cap (Day Care slot)");
    dep1.exp = 0x50FFFF;
    check(GB::checkGen1(dep1).status == GB::Status::Uncertain, "PK1 level 100 at the Day Care's reach");
    dep1.exp = 0x510000;
    check(GB::checkGen1(dep1).status == GB::Status::Illegal, "PK1 level 100 past the Day Care's reach");
}

// ---------------------------------------------------------------- 2. a real Red save

// The international Red/Blue main-data layout (PKHeX SAV1Offsets, as in the L5 real-save
// fixture): party list at 0x2F2C, checksum at 0x3523 over 0x2598..0x2598+0xF8B.
constexpr unsigned kSaveSize = 0x8000, kMain = 0x2598, kMainLength = 0x0F8B, kChecksum = 0x3523;
constexpr unsigned kParty = 0x2F2C, kTid = 0x2605, kMoney = 0x25F3, kBoxIndex = 0x284C, kBox = 0x30C0;

void put16(std::vector<uint8_t>& b, unsigned o, unsigned v) {
    b[o] = static_cast<uint8_t>(v >> 8);
    b[o + 1] = static_cast<uint8_t>(v);
}

// Max HP 200 is what level 100, HP DV 0 (DVs 8/8/8/8) and no HP stat experience give
// Bulbasaur (base 45): (45 * 2) * 100 / 100 + 100 + 10. Its other stats are (base + 8) * 2 + 5:
// Attack and Defense 119 (base 49), Speed 111 (45), Special 151 (65).
std::vector<uint8_t> redSave(uint32_t exp, uint16_t maxHP = 200,
                             std::array<uint16_t, 4> stats = {119, 119, 111, 151}, int currentHP = -1) {
    std::vector<uint8_t> b(kSaveSize, 0);
    b[kMain] = 0x80;  // player name "A"
    b[kMain + 1] = 0x50;
    b[kMoney] = 0x00;
    b[kMoney + 1] = 0x30;
    b[kMoney + 2] = 0x00;
    put16(b, kTid, 1234);
    b[kBoxIndex] = 0;
    b[kBox] = 0;
    b[kBox + 1] = 0xFF;
    b[kParty] = 1;
    b[kParty + 1] = 0x99;  // Bulbasaur's Gen I internal index
    b[kParty + 2] = 0xFF;
    const unsigned o = kParty + 8;
    b[o] = 0x99;
    put16(b, o + 1, currentHP < 0 ? maxHP : static_cast<unsigned>(currentHP));  // current HP
    b[o + 3] = 100;                 // box level
    b[o + 5] = 0x16;                // Grass
    b[o + 6] = 0x03;                // Poison
    b[o + 7] = 45;                  // Bulbasaur's catch rate
    b[o + 8] = 33;                  // Tackle
    b[o + 9] = 45;                  // Growl
    put16(b, o + 0x0C, 1234);
    b[o + 0x0E] = static_cast<uint8_t>(exp >> 16);
    b[o + 0x0F] = static_cast<uint8_t>(exp >> 8);
    b[o + 0x10] = static_cast<uint8_t>(exp);
    put16(b, o + 0x1B, 0x8888);     // DVs
    b[o + 0x1D] = 35;
    b[o + 0x1E] = 40;
    b[o + 0x21] = 100;              // party level
    put16(b, o + 0x22, maxHP);
    for (unsigned i = 0; i < 4; ++i)
        put16(b, o + 0x24 + i * 2, stats[i]);
    const unsigned ot = o + 6 * 44, nick = ot + 6 * 11;
    b[ot] = 0x80;
    b[ot + 1] = 0x50;
    b[nick] = 0x81;
    b[nick + 1] = 0x50;
    unsigned sum = 0xFF;
    for (unsigned i = kMain; i < kMain + kMainLength; ++i)
        sum = (sum - b[i]) & 0xFF;
    b[kChecksum] = static_cast<uint8_t>(sum);
    return b;
}

void realRedSave() {
    using namespace PokeVault::Integration::Gen1;
    const uint32_t max = gameExp(MediumSlow, 100);
    struct Case {
        uint32_t exp;
        Legality::Verdict expect;
        Legality::Severity severity;  // of the issue expected
        const char* text;             // the issue expected, or nullptr
    };
    const Case cases[] = {
        {max, Legality::Verdict::NoProblemsFound, Legality::Severity::Invalid, nullptr},
        // Above the cap within the Day Care's reach: Incomplete with a Warning (pokered
        // IncrementDayCareMonExp adds EXP at level 100 too); PKHeX: LevelEXPTooHigh.
        {max + 1, Legality::Verdict::Incomplete, Legality::Severity::Warning, "above the level-100 total"},
        {max + 500000, Legality::Verdict::Incomplete, Legality::Severity::Warning, "above the level-100 total"},
        {0x50FFFF, Legality::Verdict::Incomplete, Legality::Severity::Warning, "above the level-100 total"},
        // Past the reach: Invalid.
        {0x510000, Legality::Verdict::Invalid, Legality::Severity::Invalid, "above the level-100 total"},
        {0xFFFFFF, Legality::Verdict::Invalid, Legality::Severity::Invalid, "above the level-100 total"},
        // The stored level ahead of its EXP: Invalid.
        {max - 1, Legality::Verdict::Invalid, Legality::Severity::Invalid, "does not match EXP"},
    };
    for (const Case& c : cases) {
        const std::string name = "Red save, level-100 Bulbasaur, EXP " + std::to_string(c.exp);
        const std::vector<uint8_t> original = redSave(c.exp);
        std::vector<uint8_t> bytes = original;
        const auto parsed = parse(bytes, SourceGame::Red);
        check(parsed && parsed.save && parsed.save->party().size() == 1, name + ": parses");
        if (!parsed || !parsed.save || parsed.save->party().size() != 1)
            continue;
        const auto& record = parsed.save->party().front();
        check(record.species == 1 && record.level == 100 && record.experience == c.exp, name + ": record fields");
        const Pokemon::Pokemon1ReadOnly pk(record);
        const auto report = Legality::analyze(pk, pk.getGameGroup(), "red_gb");
        check(report.verdict() == c.expect, name + ": verdict");
        if (c.text)
            check(hasIssue(report, c.severity, c.text), name + ": " + c.text);
        else
            check(!hasIssue(report, Legality::Severity::Invalid, "EXP") && !hasIssue(report, Legality::Severity::Warning, "EXP"),
                  name + ": no EXP problem");
        if (c.expect == Legality::Verdict::Incomplete)
            check(!report.hasInvalid() && report.coverage.encounter == Legality::CoverageLevel::Partial,
                  name + ": Day Care slot is unfinished, not Invalid");
        // The same party record with the slot named by its save: the Day Care exception
        // closes (Invalid); named as the Day Care's own slot it is that slot's behaviour.
        Legality::Context party{pk.getGameGroup(), "red_gb"};
        party.storage = Legality::SlotStorage::PartyOrBox;
        const auto inParty = Legality::analyze(pk, party);
        const bool reach = c.exp <= 0x50FFFF;
        if (c.expect == Legality::Verdict::Incomplete) {
            check(inParty.verdict() == Legality::Verdict::Invalid &&
                      hasIssue(inParty, Legality::Severity::Invalid, "this one is in the party or a PC box"),
                  name + ": party slot closes the Day Care exception");
        } else {
            check(inParty.verdict() == c.expect, name + ": party slot verdict unchanged");
        }
        Legality::Context dayCare{pk.getGameGroup(), "red_gb"};
        dayCare.storage = Legality::SlotStorage::DayCare;
        const auto inDayCare = Legality::analyze(pk, dayCare);
        if (c.expect == Legality::Verdict::Incomplete && reach)
            check(inDayCare.verdict() == Legality::Verdict::NoProblemsFound, name + ": Day Care slot explains it");
        else
            check(inDayCare.verdict() == c.expect, name + ": Day Care slot verdict unchanged");
        check(bytes == original, name + ": save bytes unchanged");
        check(std::equal(parsed.save->sourceBytes().begin(), parsed.save->sourceBytes().end(), original.begin()),
              name + ": parsed source bytes unchanged");
    }
}

// ---------------------------------------------------------------- 3. native PK2

Legality::Report nativePk2(uint8_t level, uint32_t exp, std::array<uint8_t, 4> moves,
                           Legality::SlotStorage storage = Legality::SlotStorage::Unknown) {
    PokeVault::Integration::Gen2::PokemonRecord rec{};
    rec.species = 1;
    rec.level = level;
    rec.experience = exp;
    rec.moves = moves;
    rec.pp = {35, 40, 0, 0};
    rec.trainerId = 1234;
    rec.dvs = {0, 8, 8, 8, 8};
    rec.friendship = 70;
    rec.originalTrainer = "GOLD";
    rec.nickname = "BULBA";
    const Pokemon::Pokemon2ReadOnly pk(rec);
    Legality::Context context{Enums::GameVersion::GSC, "gold_gbc"};
    context.storage = storage;
    return Legality::analyze(pk, context);
}

// A Gold/Silver record the way the save parser stores it, run through Legality::analyze.
Legality::Report pk2Verdict(uint16_t species, uint8_t level, uint32_t exp, std::array<uint8_t, 4> moves,
                            std::array<uint8_t, 4> pp, const char* ot, const char* nick,
                            const char* source = "gold_gbc") {
    PokeVault::Integration::Gen2::PokemonRecord rec{};
    rec.species = species;
    rec.level = level;
    rec.experience = exp;
    rec.moves = moves;
    rec.pp = pp;
    rec.trainerId = 1234;
    rec.dvs = {0, 8, 8, 8, 8};
    rec.friendship = 70;
    rec.originalTrainer = ot;
    rec.nickname = nick;
    const Pokemon::Pokemon2ReadOnly pk(rec);
    return Legality::analyze(pk, Enums::GameVersion::GSC, source);
}

void productionPk2() {
    // GB::checkGen2 decides the verdict on the production path, as GB::checkGen1 does for PK1.
    const auto wild = pk2Verdict(16, 5, gameExp(MediumSlow, 5), {33, 28, 0, 0}, {35, 15, 0, 0}, "GOLD", "PIDGEY");
    check(wild.verdict() == Legality::Verdict::NoProblemsFound, "native PK2 wild Pidgey: No problems found");
    check(hasIssue(wild, Legality::Severity::Info, "Possible origin"), "native PK2 wild Pidgey: origin named");
    const auto sourceFree = pk2Verdict(16, 5, gameExp(MediumSlow, 5), {33, 28, 0, 0}, {35, 15, 0, 0}, "GOLD",
                                       "PIDGEY", "");
    check(sourceFree.verdict() == Legality::Verdict::NoProblemsFound, "source-free native PK2: No problems found");
    // Spore is not in Pidgey's Gen II learnset, any tutor or any event: no origin exists.
    const auto badMove = pk2Verdict(16, 5, gameExp(MediumSlow, 5), {147, 0, 0, 0}, {15, 0, 0, 0}, "GOLD", "PIDGEY");
    check(badMove.verdict() == Legality::Verdict::Invalid, "native PK2 Pidgey with Spore: Invalid");
    // Too much PP for the move is a definitive format rule.
    const auto badPp = pk2Verdict(16, 5, gameExp(MediumSlow, 5), {33, 28, 0, 0}, {63, 15, 0, 0}, "GOLD", "PIDGEY");
    check(badPp.verdict() == Legality::Verdict::Invalid, "native PK2 Tackle with 63 PP: Invalid");
    // Battle stat experience with one stat at zero is only unconfirmed (gains are divided).
    PokeVault::Integration::Gen2::PokemonRecord split{};
    split.species = 16;
    split.level = 5;
    split.experience = gameExp(MediumSlow, 5);
    split.moves = {33, 28, 0, 0};
    split.pp = {35, 15, 0, 0};
    split.trainerId = 1234;
    split.dvs = {0, 8, 8, 8, 8};
    split.statExperience = {7, 0, 3, 3, 3};
    split.originalTrainer = "GOLD";
    split.nickname = "PIDGEY";
    const Pokemon::Pokemon2ReadOnly splitPk(split);
    const auto splitReport = Legality::analyze(splitPk, Enums::GameVersion::GSC, "gold_gbc");
    check(splitReport.verdict() == Legality::Verdict::Incomplete, "native PK2 split stat experience: Incomplete");
    check(!splitReport.hasInvalid(), "native PK2 split stat experience: not Invalid");

    const uint32_t max = gameExp(MediumSlow, 100);
    check(!hasIssue(nativePk2(100, max, {33, 45, 0, 0}), Legality::Severity::Invalid, "EXP"),
          "native PK2 level 100 at the cap");
    check(hasIssue(nativePk2(100, max + 1, {33, 45, 0, 0}), Legality::Severity::Invalid, "above the level-100 total"),
          "native PK2 level 100, one EXP above the cap");
    check(hasIssue(nativePk2(50, 0, {33, 45, 0, 0}), Legality::Severity::Invalid, "does not match EXP"),
          "native PK2 level 50 with 0 EXP");
    // A Day Care slot's stale level (pokecrystal DayCareStep): Incomplete with a Warning.
    const auto stale = nativePk2(50, gameExp(MediumSlow, 70), {33, 45, 0, 0});
    check(stale.verdict() == Legality::Verdict::Incomplete && !stale.hasInvalid() &&
              hasIssue(stale, Legality::Severity::Warning, "is below the level the EXP gives"),
          "native PK2 level 50 with the level-70 total: Incomplete");
    const auto deposited99 = nativePk2(99, max + 1, {33, 45, 0, 0});
    check(deposited99.verdict() == Legality::Verdict::Incomplete && !deposited99.hasInvalid() &&
              hasIssue(deposited99, Legality::Severity::Warning, "above the level-100 total"),
          "native PK2 level 99 one EXP above the cap: Incomplete");
    check(hasIssue(nativePk2(99, 0x510000, {33, 45, 0, 0}), Legality::Severity::Invalid, "above the level-100 total"),
          "native PK2 level 99 past the Day Care's reach: Invalid");
    // The same records with the slot named by their save.
    const auto staleBox = nativePk2(50, gameExp(MediumSlow, 70), {33, 45, 0, 0}, Legality::SlotStorage::PartyOrBox);
    check(staleBox.verdict() == Legality::Verdict::Invalid &&
              hasIssue(staleBox, Legality::Severity::Invalid, "is below the level the EXP gives"),
          "native PK2 stale level in a party/box slot: Invalid");
    const auto staleDayCare = nativePk2(50, gameExp(MediumSlow, 70), {33, 45, 0, 0}, Legality::SlotStorage::DayCare);
    check(staleDayCare.verdict() == Legality::Verdict::NoProblemsFound, "native PK2 stale level in the Day Care slot");
    const auto capBox = nativePk2(99, max + 1, {33, 45, 0, 0}, Legality::SlotStorage::PartyOrBox);
    check(capBox.verdict() == Legality::Verdict::Invalid &&
              hasIssue(capBox, Legality::Severity::Invalid, "above the level-100 total"),
          "native PK2 level 99 above the cap in a party/box slot: Invalid");
    check(nativePk2(99, max + 1, {33, 45, 0, 0}, Legality::SlotStorage::DayCare).verdict() ==
              Legality::Verdict::NoProblemsFound,
          "native PK2 level 99 above the cap in the Day Care slot");
    // Gen II's DayCareStep adds nothing to a level-100 deposit, and nothing passes $50FFFF.
    check(hasIssue(nativePk2(100, max + 1, {33, 45, 0, 0}, Legality::SlotStorage::DayCare), Legality::Severity::Invalid,
                   "above the level-100 total"),
          "native PK2 level 100 above the cap in the Day Care slot: Invalid");
    check(hasIssue(nativePk2(99, 0x510000, {33, 45, 0, 0}, Legality::SlotStorage::DayCare), Legality::Severity::Invalid,
                   "above the level-100 total"),
          "native PK2 past the Day Care's reach in the Day Care slot: Invalid");
    check(hasIssue(nativePk2(50, 0, {33, 45, 0, 0}, Legality::SlotStorage::DayCare), Legality::Severity::Invalid,
                   "does not match EXP"),
          "native PK2 level ahead of EXP in the Day Care slot: Invalid");
    check(nativePk2(50, gameExp(MediumSlow, 50), {33, 45, 0, 0}, Legality::SlotStorage::PartyOrBox).verdict() ==
              nativePk2(50, gameExp(MediumSlow, 50), {33, 45, 0, 0}).verdict(),
          "native PK2 in-step record: the slot changes nothing");
    check(hasIssue(nativePk2(5, gameExp(MediumSlow, 5), {33, 45, 33, 0}), Legality::Severity::Invalid,
                   "Duplicate move: Tackle"),
          "native PK2 duplicate Tackle");
}

// ---------------------------------------------------------------- 4. party max HP

// pokered home/move_mon.asm CalcStat / pokecrystal CalcMonStatC for HP: b counts up from 1
// until b * b reaches the stat experience or b is 255; the bonus is b / 4.
unsigned gameRoot(unsigned statExp) {
    unsigned b = 0;
    do {
        ++b;
    } while (b != 255 && b * b < statExp);
    return b;
}

unsigned gameMaxHp(unsigned base, unsigned dv, unsigned level, unsigned statExp) {
    return std::min(999u, ((base + dv) * 2 + gameRoot(statExp) / 4) * level / 100 + level + 10);
}

void partyMaxHpGrid() {
    // Every max HP a party struct can hold at this level: the formula at any stat experience
    // the record has had, 0 up to the stored one (stat experience only grows).
    const unsigned levels[] = {1, 2, 5, 37, 99, 100};
    const uint16_t statExps[] = {0, 1, 5, 6468, 65535};
    const std::array<std::array<uint8_t, 4>, 4> dvSets{{{0, 0, 0, 0}, {15, 15, 15, 15}, {9, 2, 7, 12}, {8, 9, 14, 1}}};
    // The bonuses (b / 4) the games can have used at any stat experience up to each value.
    std::array<std::bitset<64>, std::size(statExps)> bonuses{};
    for (std::size_t i = 0; i < std::size(statExps); ++i)
        for (unsigned s = 0; s <= statExps[i]; ++s)
            bonuses[i].set(gameRoot(s) / 4);
    int judged = 0;
    for (uint16_t species = 1; species <= 251; ++species) {
        const auto* personal = PokeVault::Integration::Gen2::personalRecord(species);
        check(personal && personal->hp != 0, "PKSM base HP of species " + std::to_string(species));
        if (!personal)
            continue;
        // The PKSM port's two base-HP errors: Granbull is 90 and Magby 45 in pret pokecrystal
        // data/pokemon/base_stats and in PKHeX personal_c; the port has 99 and 74.
        const unsigned baseHp = species == 210 ? 90u : species == 240 ? 45u : personal->hp;
        check((species == 210 || species == 240) == (baseHp != personal->hp),
              "PKSM base HP erratum of species " + std::to_string(species));
        for (unsigned level : levels)
            for (const auto& dvs : dvSets)
                for (std::size_t si = 0; si < std::size(statExps); ++si) {
                    const uint16_t statExp = statExps[si];
                    const unsigned hpDv = ((dvs[0] & 1u) << 3) | ((dvs[1] & 1u) << 2) | ((dvs[2] & 1u) << 1) | (dvs[3] & 1u);
                    std::bitset<1001> reachable;
                    for (unsigned bonus = 0; bonus < 64; ++bonus)
                        if (bonuses[si].test(bonus))
                            reachable.set(std::min(999u, ((baseHp + hpDv) * 2 + bonus) * level / 100 + level + 10));
                    const unsigned lo = gameMaxHp(baseHp, hpDv, level, 0);
                    const unsigned hi = gameMaxHp(baseHp, hpDv, level, statExp);
                    for (unsigned maxHP : {0u, 1u, lo - 1, lo, lo + 1, (lo + hi) / 2, hi - 1, hi, hi + 1, 999u, 1000u}) {
                        const std::string name = "species " + std::to_string(species) + " level " + std::to_string(level) +
                                                 " HP DV " + std::to_string(hpDv) + " stat exp " + std::to_string(statExp) +
                                                 " max HP " + std::to_string(maxHP);
                        // 0 is no party snapshot (a box struct in party layout): not judged.
                        const bool bad = maxHP != 0 && !(maxHP <= 1000 && reachable.test(std::min(maxHP, 1000u)));
                        GB::Entity2 e2;
                        e2.species = static_cast<uint8_t>(species);
                        e2.level = static_cast<uint8_t>(level);
                        e2.exp = Pokemon::getExpForLevel(static_cast<uint8_t>(level), Pokemon::getGrowthRate(species));
                        e2.dvs = dvs;
                        e2.statExp = {statExp, 0, 0, 0, 0};
                        e2.nickname = "MON";
                        e2.partyStats = true;
                        e2.maxHP = static_cast<uint16_t>(maxHP);
                        check(hasText(GB::formatProblems(e2), "Max HP") == bad, name + " (PK2)");
                        e2.partyStats = false;
                        check(!hasText(GB::formatProblems(e2), "Max HP"), name + " (PK2, no party stats)");
                        if (species <= 151) {
                            GB::Entity1 e1;
                            e1.species = static_cast<uint8_t>(species);
                            e1.level = e2.level;
                            e1.exp = e2.exp;
                            e1.dvs = dvs;
                            e1.statExp = e2.statExp;
                            e1.nickname = "MON";
                            e1.partyStats = true;
                            e1.maxHP = e2.maxHP;
                            check(hasText(GB::formatProblems(e1), "Max HP") == bad, name + " (PK1)");
                        }
                        ++judged;
                    }
                }
    }
    check(judged > 200000, "party max-HP grid size");
    // An unhatched egg is not judged by the party rule.
    GB::Entity2 egg;
    egg.species = 172;
    egg.level = 5;
    egg.exp = 125;
    egg.isEgg = true;
    egg.nickname = "EGG";
    egg.partyStats = true;
    egg.maxHP = 1;
    check(!hasText(GB::formatProblems(egg), "Max HP"), "unhatched egg: no party max-HP problem");
}

// Pidgey level 5, DVs 8/8/8/8, no stat experience: Attack 10 (base 45), Defense 9 (40),
// Speed 11 (56), Special Attack and Defense 9 (35): ((base + 8) * 2) * 5 / 100 + 5.
Legality::Report pk2Party(uint16_t maxHP, uint16_t recordMaxHP, uint16_t hpStatExp = 0,
                          std::array<uint16_t, 5> stats = {10, 9, 11, 9, 9}) {
    PokeVault::Integration::Gen2::PokemonRecord rec{};
    rec.species = 16;  // Pidgey, base HP 40
    rec.level = 5;
    rec.experience = gameExp(MediumSlow, 5);
    rec.moves = {33, 28, 0, 0};
    rec.pp = {35, 15, 0, 0};
    rec.trainerId = 1234;
    rec.dvs = {0, 8, 8, 8, 8};
    rec.statExperience = {hpStatExp, 0, 0, 0, 0};
    rec.friendship = 70;
    rec.originalTrainer = "GOLD";
    rec.nickname = "PIDGEY";
    rec.maxHP = recordMaxHP;
    rec.partyRecord = true;
    // The stored party struct the record was parsed from (pokecrystal party_struct).
    std::array<uint8_t, 48> body{};
    body[0] = 16;
    body[2] = 33;
    body[3] = 28;
    body[6] = 1234 >> 8;
    body[7] = 1234 & 0xFF;
    body[21] = 0x88;
    body[22] = 0x88;
    body[31] = 5;
    body[36] = static_cast<uint8_t>(maxHP >> 8);
    body[37] = static_cast<uint8_t>(maxHP);
    for (std::size_t i = 0; i < 5; ++i) {
        body[38 + i * 2] = static_cast<uint8_t>(stats[i] >> 8);
        body[39 + i * 2] = static_cast<uint8_t>(stats[i]);
    }
    std::copy(body.begin(), body.end(), rec.rawBody.begin());
    rec.rawBodySize = body.size();
    const Pokemon::Pokemon2ReadOnly pk(rec);
    return Legality::analyze(pk, Enums::GameVersion::GSC, "gold_gbc");
}

void partyMaxHpProduction() {
    using namespace PokeVault::Integration::Gen1;
    // Red save, level-100 Bulbasaur (HP DV 0, no HP stat experience): 200 is the only max HP.
    for (const uint16_t maxHP : {199, 201, 231, 999}) {
        const std::string name = "Red save, level-100 Bulbasaur, max HP " + std::to_string(maxHP);
        const std::vector<uint8_t> original = redSave(gameExp(MediumSlow, 100), maxHP);
        std::vector<uint8_t> bytes = original;
        const auto parsed = parse(bytes, SourceGame::Red);
        check(parsed && parsed.save && parsed.save->party().size() == 1, name + ": parses");
        if (!parsed || !parsed.save || parsed.save->party().size() != 1)
            continue;
        const Pokemon::Pokemon1ReadOnly pk(parsed.save->party().front());
        const auto report = Legality::analyze(pk, pk.getGameGroup(), "red_gb");
        check(report.verdict() == Legality::Verdict::Invalid && hasIssue(report, Legality::Severity::Invalid, "Max HP"),
              name + ": Invalid");
        check(bytes == original, name + ": save bytes unchanged");
    }
    {
        std::vector<uint8_t> bytes = redSave(gameExp(MediumSlow, 100), 200);
        const auto parsed = parse(bytes, SourceGame::Red);
        check(parsed && parsed.save && parsed.save->party().size() == 1, "Red save, max HP 200: parses");
        if (parsed && parsed.save && parsed.save->party().size() == 1) {
            const Pokemon::Pokemon1ReadOnly pk(parsed.save->party().front());
            check(Legality::analyze(pk, pk.getGameGroup(), "red_gb").verdict() == Legality::Verdict::NoProblemsFound,
                  "Red save, level-100 Bulbasaur, max HP 200: No problems found");
        }
    }
    // Native PK2 party records: Pidgey level 5, HP DV 0: (40 * 2) * 5 / 100 + 15 = 19; with
    // 6468 HP stat experience (b = 81, bonus 20) anything from 19 to 20 ((80 + 20) * 5 / 100 + 15).
    check(pk2Party(19, 19).verdict() == Legality::Verdict::NoProblemsFound, "PK2 party Pidgey max HP 19");
    check(pk2Party(20, 20).verdict() == Legality::Verdict::Invalid, "PK2 party Pidgey max HP 20 without stat experience");
    check(pk2Party(20, 20, 6468).verdict() != Legality::Verdict::Invalid, "PK2 party Pidgey max HP 20 with stat experience");
    check(!hasIssue(pk2Party(19, 19, 6468), Legality::Severity::Invalid, "Max HP"),
          "PK2 party Pidgey max HP 19 recalculated before its stat experience");
    check(hasIssue(pk2Party(21, 21, 6468), Legality::Severity::Invalid, "Max HP"), "PK2 party Pidgey max HP 21");
    // The rule reads only stored bytes the record's own fields were parsed from.
    check(!hasIssue(pk2Party(20, 19), Legality::Severity::Invalid, "Max HP"), "PK2 record and stored bytes disagree");
    check(!hasIssue(pk2Party(0, 0), Legality::Severity::Invalid, "Max HP"), "PK2 box struct in party layout (max HP 0)");
}

// True when a definitive problem names this stat ("Attack 12 is not ..."), so that Attack
// does not match Special Attack.
bool hasStat(const std::vector<GB::Problem>& problems, const std::string& stat) {
    for (const auto& p : problems)
        if (p.definitive && p.text.rfind(stat + " ", 0) == 0 && p.text.find(" is not what level ") != std::string::npos)
            return true;
    return false;
}

unsigned gameStat(unsigned base, unsigned dv, unsigned level, unsigned statExp) {
    return std::min(999u, ((base + dv) * 2 + gameRoot(statExp) / 4) * level / 100 + 5);
}

void partyStatGrid() {
    // Attack, Defense, Speed, Special Attack and Special Defense, one at a time, with every
    // other stat at a value the formula gives: the stored stat must be the formula at some
    // stat experience between 0 and the stored one. Bases from the PKSM port (independent
    // of the pret-generated table the engine reads).
    const unsigned levels[] = {1, 5, 37, 100};
    const uint16_t statExps[] = {0, 6468, 65535};
    const std::array<std::array<uint8_t, 4>, 3> dvSets{{{0, 0, 0, 0}, {15, 15, 15, 15}, {9, 2, 7, 12}}};
    std::array<std::bitset<64>, std::size(statExps)> bonuses{};
    for (std::size_t i = 0; i < std::size(statExps); ++i)
        for (unsigned s = 0; s <= statExps[i]; ++s)
            bonuses[i].set(gameRoot(s) / 4);
    static const char* const kNames[] = {"Attack", "Defense", "Speed", "Special Attack", "Special Defense"};
    int judged = 0;
    for (uint16_t species = 1; species <= 251; ++species) {
        const auto* personal = PokeVault::Integration::Gen2::personalRecord(species);
        if (!personal)
            continue;
        const unsigned baseHp = species == 210 ? 90u : species == 240 ? 45u : personal->hp;  // see partyMaxHpGrid
        const unsigned bases[] = {personal->attack, personal->defense, personal->speed, personal->specialAttack,
                                  personal->specialDefense};
        for (unsigned level : levels)
            for (const auto& dvs : dvSets) {
                const unsigned hpDv = ((dvs[0] & 1u) << 3) | ((dvs[1] & 1u) << 2) | ((dvs[2] & 1u) << 1) | (dvs[3] & 1u);
                const unsigned statDv[] = {dvs[0], dvs[1], dvs[2], dvs[3], dvs[3]};
                std::array<uint16_t, 5> fresh{};
                for (std::size_t k = 0; k < 5; ++k)
                    fresh[k] = static_cast<uint16_t>(gameStat(bases[k], statDv[k], level, 0));
                for (std::size_t k = 0; k < 5; ++k)
                    for (std::size_t si = 0; si < std::size(statExps); ++si) {
                        const uint16_t statExp = statExps[si];
                        std::bitset<1001> reachable;
                        for (unsigned bonus = 0; bonus < 64; ++bonus)
                            if (bonuses[si].test(bonus))
                                reachable.set(std::min(999u, ((bases[k] + statDv[k]) * 2 + bonus) * level / 100 + 5));
                        const unsigned lo = gameStat(bases[k], statDv[k], level, 0);
                        const unsigned hi = gameStat(bases[k], statDv[k], level, statExp);
                        for (unsigned value : {0u, lo - 1, lo, lo + 1, (lo + hi) / 2, hi - 1, hi, hi + 1, 1000u}) {
                            const std::string name = "species " + std::to_string(species) + " level " +
                                                     std::to_string(level) + " " + kNames[k] + " DV " +
                                                     std::to_string(statDv[k]) + " stat exp " + std::to_string(statExp) +
                                                     " value " + std::to_string(value);
                            const bool bad = !(value <= 1000 && reachable.test(std::min(value, 1000u)));
                            GB::Entity2 e2;
                            e2.species = static_cast<uint8_t>(species);
                            e2.level = static_cast<uint8_t>(level);
                            e2.exp = Pokemon::getExpForLevel(static_cast<uint8_t>(level), Pokemon::getGrowthRate(species));
                            e2.dvs = dvs;
                            // Special Attack and Special Defense share the Special stat experience.
                            e2.statExp[k == 4 ? 4 : k + 1] = statExp;
                            e2.nickname = "MON";
                            e2.partyStats = true;
                            e2.maxHP = static_cast<uint16_t>(gameMaxHp(baseHp, hpDv, level, 0));
                            e2.stats = fresh;
                            e2.stats[k] = static_cast<uint16_t>(value);
                            const auto problems = GB::formatProblems(e2);
                            check(hasStat(problems, kNames[k]) == bad, name + " (PK2)");
                            for (std::size_t other = 0; other < 5; ++other)
                                if (other != k)
                                    check(!hasStat(problems, kNames[other]), name + " (PK2): only that stat");
                            check(!hasStat(problems, "Max HP"), name + " (PK2): max HP");
                            e2.partyStats = false;
                            check(!hasStat(GB::formatProblems(e2), kNames[k]), name + " (PK2, no party stats)");
                            // Gen I has the same Attack, Defense and Speed.
                            if (species <= 151 && k < 3) {
                                GB::Entity1 e1;
                                e1.species = e2.species;
                                e1.level = e2.level;
                                e1.exp = e2.exp;
                                e1.dvs = dvs;
                                e1.statExp = e2.statExp;
                                e1.nickname = "MON";
                                e1.partyStats = true;
                                e1.maxHP = e2.maxHP;
                                e1.stats = {e2.stats[0], e2.stats[1], e2.stats[2], 0, 0};
                                // The PKSM table has no Gen I Special, so it is left at 0 (which
                                // the engine rejects as Special); only the stat under test is read.
                                const auto p1 = GB::formatProblems(e1);
                                check(hasStat(p1, kNames[k]) == bad, name + " (PK1)");
                                ++judged;
                            }
                            ++judged;
                        }
                    }
            }
    }
    check(judged > 100000, "party stat grid size");
}

// Gen I keeps one Special (pokered base_stats), Gen II splits it, and the Time Capsule
// recomputes it from the receiving generation's bases. Psyduck: Gen I Special 50, Gen II
// Special Attack 65 and Special Defense 50.
void partySpecialSplit() {
    const std::array<uint8_t, 4> dvs{8, 8, 8, 8};
    // Level 50, no stat experience: Special Attack (65 + 8) * 2 / 2 + 5 = 78; Special
    // Defense and the Gen I Special (50 + 8) * 2 / 2 + 5 = 63. Max HP (50 * 2) / 2 + 60 = 110,
    // Attack (52 + 8) + 5 = 65, Defense (48 + 8) + 5 = 61, Speed (55 + 8) + 5 = 68.
    GB::Entity1 e1;
    e1.species = 54;
    e1.level = 50;
    e1.exp = Pokemon::getExpForLevel(50, Pokemon::getGrowthRate(54));
    e1.dvs = dvs;
    e1.nickname = "PSYDUCK";
    e1.partyStats = true;
    e1.maxHP = 110;
    e1.stats = {65, 61, 68, 63, 0};
    check(GB::formatProblems(e1).empty(), "PK1 Psyduck with its Gen I Special 63");
    e1.stats[3] = 78;
    check(hasStat(GB::formatProblems(e1), "Special"), "PK1 Psyduck with the Gen II Special Attack 78");
    GB::Entity2 e2;
    e2.species = 54;
    e2.level = 50;
    e2.exp = e1.exp;
    e2.dvs = dvs;
    e2.nickname = "PSYDUCK";
    e2.partyStats = true;
    e2.maxHP = 110;
    e2.stats = {65, 61, 68, 78, 63};
    check(GB::formatProblems(e2).empty(), "PK2 Psyduck with Special Attack 78 and Special Defense 63");
    e2.stats[3] = 63;
    check(hasStat(GB::formatProblems(e2), "Special Attack"), "PK2 Psyduck carrying the Gen I Special as Special Attack");
    // A battle stage or a burn halving Attack never reaches the party struct.
    e2.stats = {32, 61, 68, 78, 63};
    check(hasStat(GB::formatProblems(e2), "Attack"), "PK2 Psyduck with a halved Attack");
    // A box struct exported in party layout (max HP 0) is not judged.
    e2.maxHP = 0;
    check(GB::formatProblems(e2).empty(), "PK2 box struct in party layout: stats not judged");
}

// The same Bulbasaur (level 100, DVs 8/8/8/8, EXP at the level-100 total) alone in the Red
// save's current box: a box struct keeps the current HP it was deposited with.
std::vector<uint8_t> redBoxSave(uint16_t currentHP, uint16_t hpStatExp) {
    std::vector<uint8_t> b = redSave(gameExp(MediumSlow, 100));
    b[kBox] = 1;
    b[kBox + 1] = 0x99;
    b[kBox + 2] = 0xFF;
    const unsigned o = kBox + 20 + 2;  // 20 slots, the count and the list terminator
    b[o] = 0x99;
    put16(b, o + 1, currentHP);
    b[o + 3] = 100;
    b[o + 5] = 0x16;
    b[o + 6] = 0x03;
    b[o + 7] = 45;
    b[o + 8] = 33;
    b[o + 9] = 45;
    put16(b, o + 0x0C, 1234);
    const uint32_t exp = gameExp(MediumSlow, 100);
    b[o + 0x0E] = static_cast<uint8_t>(exp >> 16);
    b[o + 0x0F] = static_cast<uint8_t>(exp >> 8);
    b[o + 0x10] = static_cast<uint8_t>(exp);
    put16(b, o + 0x11, hpStatExp);
    put16(b, o + 0x1B, 0x8888);
    b[o + 0x1D] = 35;
    b[o + 0x1E] = 40;
    const unsigned ot = o + 20 * 33, nick = ot + 20 * 11;
    b[ot] = 0x80;
    b[ot + 1] = 0x50;
    b[nick] = 0x81;
    b[nick + 1] = 0x50;
    unsigned sum = 0xFF;
    for (unsigned i = kMain; i < kMain + kMainLength; ++i)
        sum = (sum - b[i]) & 0xFF;
    b[kChecksum] = static_cast<uint8_t>(sum);
    return b;
}

void currentHpRule() {
    // Engine: a party struct's current HP is at most its stored max HP.
    for (const uint16_t hp : {0, 1, 199, 200, 201, 999}) {
        GB::Entity2 e = bulbasaur2({33, 45, 0, 0});
        e.partyStats = true;
        e.maxHP = 200;
        e.hpKnown = true;
        e.currentHP = hp;
        check(hasText(GB::formatProblems(e), "Current HP") == (hp > 200),
              "PK2 current HP " + std::to_string(hp) + " of max 200");
        e.hpKnown = false;
        check(!hasText(GB::formatProblems(e), "Current HP"), "PK2 current HP not known");
    }
    // A Gen I box struct (no stored max HP): at most the formula's max HP at its level with
    // its own HP stat experience, checked over a grid with the PKSM bases (independent of
    // the engine's table) and the games' square root.
    int judged = 0;
    const std::array<std::array<uint8_t, 4>, 3> dvSets{{{0, 0, 0, 0}, {15, 15, 15, 15}, {9, 2, 7, 12}}};
    for (uint16_t species = 1; species <= 151; ++species) {
        const auto* personal = PokeVault::Integration::Gen2::personalRecord(species);
        if (!personal)
            continue;
        for (unsigned level : {1u, 5u, 50u, 100u})
            for (const auto& dvs : dvSets)
                for (uint16_t statExp : {0, 6468, 65535}) {
                    const unsigned hpDv = ((dvs[0] & 1u) << 3) | ((dvs[1] & 1u) << 2) | ((dvs[2] & 1u) << 1) | (dvs[3] & 1u);
                    const unsigned top = gameMaxHp(personal->hp, hpDv, level, statExp);
                    for (unsigned hp : {0u, top - 1, top, top + 1, 65535u}) {
                        GB::Entity1 e;
                        e.species = static_cast<uint8_t>(species);
                        e.level = static_cast<uint8_t>(level);
                        e.exp = Pokemon::getExpForLevel(static_cast<uint8_t>(level), Pokemon::getGrowthRate(species));
                        e.dvs = dvs;
                        e.statExp = {statExp, 0, 0, 0, 0};
                        e.nickname = "MON";
                        e.hpKnown = true;
                        e.currentHP = static_cast<uint16_t>(hp);
                        check(hasText(GB::formatProblems(e), "Current HP") == (hp > top),
                              "PK1 box species " + std::to_string(species) + " level " + std::to_string(level) +
                                  " stat exp " + std::to_string(statExp) + " current HP " + std::to_string(hp));
                        ++judged;
                    }
                }
    }
    check(judged > 8000, "box current-HP grid size");

    using namespace PokeVault::Integration::Gen1;
    const auto analyzeParty = [](int currentHP) {
        std::vector<uint8_t> bytes = redSave(gameExp(MediumSlow, 100), 200, {119, 119, 111, 151}, currentHP);
        const auto parsed = parse(bytes, SourceGame::Red);
        check(parsed && parsed.save && parsed.save->party().size() == 1, "Red save party current HP: parses");
        const Pokemon::Pokemon1ReadOnly pk(parsed.save->party().front());
        return Legality::analyze(pk, pk.getGameGroup(), "red_gb");
    };
    check(analyzeParty(200).verdict() == Legality::Verdict::NoProblemsFound, "Red save party current HP 200 of 200");
    check(analyzeParty(0).verdict() == Legality::Verdict::NoProblemsFound, "Red save party fainted (current HP 0)");
    const auto over = analyzeParty(201);
    check(over.verdict() == Legality::Verdict::Invalid && hasIssue(over, Legality::Severity::Invalid, "Current HP 201"),
          "Red save party current HP 201 of 200: Invalid");
    const auto analyzeBox = [](uint16_t currentHP, uint16_t hpStatExp) {
        const std::vector<uint8_t> original = redBoxSave(currentHP, hpStatExp);
        std::vector<uint8_t> bytes = original;
        const auto parsed = parse(bytes, SourceGame::Red);
        const bool ok = parsed && parsed.save && !parsed.save->boxes().empty() &&
                        parsed.save->boxes().front().slots.size() > 0 && parsed.save->boxes().front().slots[0];
        check(ok, "Red save box record: parses");
        check(bytes == original, "Red save box record: bytes unchanged");
        if (!ok)
            return Legality::Report{};
        const Pokemon::Pokemon1ReadOnly pk(*parsed.save->boxes().front().slots[0]);
        Legality::Context context{pk.getGameGroup(), "red_gb"};
        context.storage = Legality::SlotStorage::PartyOrBox;
        return Legality::analyze(pk, context);
    };
    // Level 100, HP DV 0: 200 with no HP stat experience; with 65535 (b = 255, bonus 63)
    // ((45 * 2) + 63) + 110 = 263.
    check(analyzeBox(200, 0).verdict() == Legality::Verdict::NoProblemsFound, "Red save box current HP 200");
    check(hasIssue(analyzeBox(201, 0), Legality::Severity::Invalid, "Current HP 201"), "Red save box current HP 201");
    // (HP stat experience alone, the others at zero, is a stat-experience Warning of its own.)
    const auto full = analyzeBox(263, 65535);
    check(full.verdict() != Legality::Verdict::Invalid && !hasIssue(full, Legality::Severity::Invalid, "Current HP"),
          "Red save box current HP 263 with full HP stat experience");
    check(hasIssue(analyzeBox(264, 65535), Legality::Severity::Invalid, "Current HP 264"),
          "Red save box current HP 264 with full HP stat experience");
}

void resum(std::vector<uint8_t>& b) {
    unsigned sum = 0xFF;
    for (unsigned i = kMain; i < kMain + kMainLength; ++i)
        sum = (sum - b[i]) & 0xFF;
    b[kChecksum] = static_cast<uint8_t>(sum);
}

void statusAndBoxLevelRules() {
    // Engine: every status byte value, PK1 and PK2 (party), and a PK2 egg.
    for (unsigned s = 0; s < 256; ++s) {
        const bool ok = s <= 7 || s == 0x08 || s == 0x10 || s == 0x20 || s == 0x40;
        GB::Entity1 e1 = bulbasaur1({33, 45, 0, 0});
        e1.statusKnown = true;
        e1.status = static_cast<uint8_t>(s);
        check(hasText(GB::formatProblems(e1), "Status byte") == !ok, "PK1 status " + std::to_string(s));
        // A fainted Pokemon may keep its status (recoil or Destiny Bond knocking both out).
        e1.hpKnown = true;
        e1.currentHP = 0;
        check(hasText(GB::formatProblems(e1), "Status byte") == !ok, "PK1 fainted, status " + std::to_string(s));
        e1.statusKnown = false;
        check(!hasText(GB::formatProblems(e1), "Status byte"), "PK1 status not known " + std::to_string(s));
        GB::Entity2 e2 = bulbasaur2({33, 45, 0, 0});
        e2.statusKnown = true;
        e2.status = static_cast<uint8_t>(s);
        check(hasText(GB::formatProblems(e2), "Status byte") == !ok, "PK2 status " + std::to_string(s));
        GB::Entity2 egg = e2;
        egg.isEgg = true;
        egg.nickname = "EGG";
        const auto eggProblems = GB::formatProblems(egg);
        check(hasText(eggProblems, "Status byte") == !ok, "PK2 egg status pattern " + std::to_string(s));
        check(hasText(eggProblems, "An unhatched egg has no status condition") == (ok && s != 0),
              "PK2 egg status " + std::to_string(s));
    }
    // Engine: a party struct's box-level byte is 0 or at most the party level.
    for (unsigned box = 0; box < 256; ++box) {
        GB::Entity1 e = bulbasaur1({33, 45, 0, 0});
        e.boxLevelKnown = true;
        e.boxLevel = static_cast<uint8_t>(box);
        check(hasText(GB::formatProblems(e), "Box-level byte") == (box > e.level), "PK1 box level " + std::to_string(box));
        e.boxLevelKnown = false;
        check(!hasText(GB::formatProblems(e), "Box-level byte"), "PK1 box level not known " + std::to_string(box));
    }

    // Production: the Red save's party Bulbasaur (level 100) and a box Bulbasaur.
    using namespace PokeVault::Integration::Gen1;
    const unsigned party = kParty + 8, boxed = kBox + 20 + 2;
    const auto analyze = [](std::vector<uint8_t> bytes, bool fromBox) {
        const std::vector<uint8_t> original = bytes;
        const auto parsed = parse(bytes, SourceGame::Red);
        const bool ok = parsed && parsed.save &&
                        (fromBox ? !parsed.save->boxes().empty() && parsed.save->boxes().front().slots.size() > 0 &&
                                       parsed.save->boxes().front().slots[0].has_value()
                                 : parsed.save->party().size() == 1);
        check(ok, "Red save status/box level: parses");
        check(bytes == original, "Red save status/box level: bytes unchanged");
        if (!ok)
            return Legality::Report{};
        const Pokemon::Pokemon1ReadOnly pk(fromBox ? *parsed.save->boxes().front().slots[0]
                                                   : parsed.save->party().front());
        Legality::Context context{pk.getGameGroup(), "red_gb"};
        context.storage = Legality::SlotStorage::PartyOrBox;
        return Legality::analyze(pk, context);
    };
    for (const uint8_t status : {0x00, 0x03, 0x08, 0x40, 0x09, 0x18, 0x80}) {
        const bool ok = status <= 7 || status == 0x08 || status == 0x40;
        std::vector<uint8_t> b = redSave(gameExp(MediumSlow, 100));
        b[party + 4] = status;
        resum(b);
        const auto r = analyze(b, false);
        check((r.verdict() == Legality::Verdict::Invalid) == !ok && hasIssue(r, Legality::Severity::Invalid, "Status byte") == !ok,
              "Red save party status " + std::to_string(status));
        std::vector<uint8_t> bx = redBoxSave(200, 0);
        bx[boxed + 4] = status;
        resum(bx);
        const auto rb = analyze(bx, true);
        check(hasIssue(rb, Legality::Severity::Invalid, "Status byte") == !ok, "Red save box status " + std::to_string(status));
    }
    for (const uint8_t box : {0, 5, 100, 101, 255}) {
        std::vector<uint8_t> b = redSave(gameExp(MediumSlow, 100));
        b[party + 3] = box;
        resum(b);
        const auto r = analyze(b, false);
        check(hasIssue(r, Legality::Severity::Invalid, "Box-level byte") == (box > 100),
              "Red save party box level " + std::to_string(box));
        check((r.verdict() == Legality::Verdict::NoProblemsFound) == (box <= 100),
              "Red save party box level verdict " + std::to_string(box));
    }
}

void partyStatProduction() {
    using namespace PokeVault::Integration::Gen1;
    // Red save, level-100 Bulbasaur, DVs 8: Attack 119, Defense 119, Speed 111, Special 151.
    const std::array<std::array<uint16_t, 4>, 4> wrong{{{120, 119, 111, 151}, {119, 0, 111, 151},
                                                        {119, 119, 222, 151}, {119, 119, 111, 150}}};
    static const char* const kNames[] = {"Attack", "Defense", "Speed", "Special"};
    for (std::size_t k = 0; k < wrong.size(); ++k) {
        const std::string name = std::string("Red save, level-100 Bulbasaur, wrong ") + kNames[k];
        const std::vector<uint8_t> original = redSave(gameExp(MediumSlow, 100), 200, wrong[k]);
        std::vector<uint8_t> bytes = original;
        const auto parsed = parse(bytes, SourceGame::Red);
        check(parsed && parsed.save && parsed.save->party().size() == 1, name + ": parses");
        if (!parsed || !parsed.save || parsed.save->party().size() != 1)
            continue;
        const Pokemon::Pokemon1ReadOnly pk(parsed.save->party().front());
        const auto report = Legality::analyze(pk, pk.getGameGroup(), "red_gb");
        check(report.verdict() == Legality::Verdict::Invalid &&
                  hasIssue(report, Legality::Severity::Invalid, std::string(kNames[k]) + " "),
              name + ": Invalid");
        check(bytes == original, name + ": save bytes unchanged");
    }
    // Native PK2 party Pidgey (see pk2Party).
    check(pk2Party(19, 19).verdict() == Legality::Verdict::NoProblemsFound, "PK2 party Pidgey, formula stats");
    check(hasIssue(pk2Party(19, 19, 0, {10, 9, 11, 10, 9}), Legality::Severity::Invalid, "Special Attack 10"),
          "PK2 party Pidgey, Special Attack 10 without stat experience");
    check(hasIssue(pk2Party(19, 19, 0, {10, 9, 11, 9, 0}), Legality::Severity::Invalid, "Special Defense 0"),
          "PK2 party Pidgey, Special Defense 0");
    check(!hasIssue(pk2Party(0, 0, 0, {1, 1, 1, 1, 1}), Legality::Severity::Invalid, "is not what level"),
          "PK2 box struct in party layout: stats not judged");
}

} // namespace

int main() {
    duplicateMoves();
    levelExpGrid();
    realRedSave();
    productionPk2();
    partyMaxHpGrid();
    partyMaxHpProduction();
    partyStatGrid();
    partySpecialSplit();
    partyStatProduction();
    currentHpRule();
    statusAndBoxLevelRules();
    if (failures != 0) {
        std::fprintf(stderr, "Gen I/II structural legality: %d of %d checks FAILED\n", failures, checks);
        return EXIT_FAILURE;
    }
    std::printf("Gen I/II structural legality: %d checks PASS\n", checks);
    return EXIT_SUCCESS;
}
