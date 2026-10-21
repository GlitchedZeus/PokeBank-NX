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
#include "Integration/Gen1/Gen1ReadOnlySave.h"
#include "Integration/Gen2/Gen2ReadOnlySave.h"
#include "Legality/GBLegality.h"
#include "Legality/Legality.h"
#include "Names/ItemNames.h"
#include "Names/SpeciesNames.h"
#include "Pokemon/Experience.h"
#include "Pokemon/Pokemon1ReadOnly.h"
#include "Pokemon/Pokemon2ReadOnly.h"

#include <array>
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

std::vector<uint8_t> redSave(uint32_t exp) {
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
    put16(b, o + 1, 231);           // current HP
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
    for (unsigned i = 0; i < 5; ++i)
        put16(b, o + 0x22 + i * 2, 231 - i);
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
        check(bytes == original, name + ": save bytes unchanged");
        check(std::equal(parsed.save->sourceBytes().begin(), parsed.save->sourceBytes().end(), original.begin()),
              name + ": parsed source bytes unchanged");
    }
}

// ---------------------------------------------------------------- 3. native PK2

Legality::Report nativePk2(uint8_t level, uint32_t exp, std::array<uint8_t, 4> moves) {
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
    return Legality::analyze(pk, Enums::GameVersion::GSC, "gold_gbc");
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
    check(hasIssue(nativePk2(5, gameExp(MediumSlow, 5), {33, 45, 33, 0}), Legality::Severity::Invalid,
                   "Duplicate move: Tackle"),
          "native PK2 duplicate Tackle");
}

} // namespace

int main() {
    duplicateMoves();
    levelExpGrid();
    realRedSave();
    productionPk2();
    if (failures != 0) {
        std::fprintf(stderr, "Gen I/II structural legality: %d of %d checks FAILED\n", failures, checks);
        return EXIT_FAILURE;
    }
    std::printf("Gen I/II structural legality: %d checks PASS\n", checks);
    return EXIT_SUCCESS;
}
