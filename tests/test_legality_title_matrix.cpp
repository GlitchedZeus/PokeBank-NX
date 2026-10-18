// Per-title acceptance matrix for the Gen I to Gen V legality analyzers (20 titles).
// docs/legality/TITLE_TEST_MATRIX.md lists the same cases with their evidence.
//
// Every case prints one line: title | kind | case | expected | actual | PASS, FAIL or BUG.
// BUG marks a known engine bug: the expected verdict comes from the evidence, and the
// engine still gives the documented wrong verdict. The program exits non-zero when any
// case fails. Pass -v to print every report.
//
// Gen I and Gen II: each case starts from synthetic 32 KiB battery-save bytes and runs
// the app's own path. Gen1::parse / Gen2::parse get the title as the source hint, the
// party record goes through Pokemon1ReadOnly / Pokemon2ReadOnly, and then
// Legality::analyze(pk, pk.getGameGroup(), metadata().sourceGameId) runs, as in
// Gen1PokemonDetailsModal.cpp and Gen2PokemonDetailsModal.cpp.
// Gen III: the app's GBA path. Synthetic 128 KiB saves go through Integration::Gen3::parse
// (the on-device reader, PKSMGen3NativeAdapter.cpp) and FRLGReadOnlyTrainer::create, as in
// UI.cpp; the box slot goes through Legality::analyze(pk, pk.getGameGroup(),
// metadata().sourceGameId), as in PokemonDetailsModal.cpp. Record cases load the stored
// (encrypted) PK3 into Pokemon3FRLG and make the same call with the exact source game id.
// Gen IV: the app's NDS path. Synthetic 512 KiB saves go through Gen4ReadOnlySave::parse
// for every layout with the assigned game id (Gen4AssignedSource.cpp openAssignedSource),
// then Gen4ReadOnlyTrainer::create (UI.cpp handleGen4View); box 1 slot 1 goes through
// Pokemon4ReadOnlyView and Legality::analyze(view, view.getGameGroup(), sourceGameId), as in
// Gen4SharedPokemonSurface.inc drawLegality. Record cases load the stored (encrypted) PK4
// into Pokemon4ReadOnly and make the same call. Legality::analyzePk4 on the decrypted bytes
// must agree: production never drops its Invalid and never certifies more.
// Gen V: Legality::analyzePk5 on the decrypted record and on its stored (encrypted)
// form. This branch has no PK5 save reader (that is PR #307), so the app does not call
// analyzePk5 yet.
//
// Kinds:
//   positive        a legitimate record reads "No problems found"
//   negative        a broken rule reads Invalid, and the rule is named
//   malformed       corrupt bytes: no crash, never "No problems found"; exact verdict
//   unknown         history the analyzer cannot check reads Incomplete
//   original-game   the expected verdict comes from a pret disassembly, not from PKHeX
//   title-specific  the title changes the answer
//
// Nothing here writes a save or changes a record: every case checks its input bytes
// afterwards.
#include "Encryption/Encryption3FRLG.h"
#include "Encryption/Encryption4.h"
#include "Encryption/Encryption5.h"
#include "Integration/Gen1/Gen1ReadOnlySave.h"
#include "Integration/Gen2/Gen2ReadOnlySave.h"
#include "Integration/Gen3/PKSMGen3Adapter.h"
#include "Integration/Gen4/Gen4ReadOnlySave.h"
#include "Legacy/FRLGReadOnlyTrainer.h"
#include "Legacy/Gen4ReadOnlyTrainer.h"
#include "Legality/Gen3Legality.h"
#include "Legality/Gen4Legality.h"
#include "Legality/Gen5Legality.h"
#include "Legality/Legality.h"
#include "Names/ItemNames.h"
#include "Names/SpeciesNames.h"
#include "Pokemon/Pokemon1ReadOnly.h"
#include "Pokemon/Pokemon2ReadOnly.h"
#include "Pokemon/Pokemon3FRLG.h"
#include "Pokemon/Pokemon4ReadOnly.h"
#include "Pokemon/Pokemon4ReadOnlyView.h"
#include "legality_gen5_test_helpers.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace Trainer {
const char* getSpeciesName(uint16_t id) { return Names::getSpeciesName(id); }
const char* getItemName(uint16_t id) { return Names::getItemName(id); }
}

namespace {

using Legality::Report;
using Legality::Severity;
using Gen5TestHelpers::hasText;

// ---------------------------------------------------------------- harness

enum class Outcome { NoProblemsFound, Invalid, Incomplete, Rejected };

const char* outcomeName(Outcome o) {
    switch (o) {
        case Outcome::NoProblemsFound: return "No problems found";
        case Outcome::Invalid: return "Invalid";
        case Outcome::Incomplete: return "Incomplete";
        case Outcome::Rejected: return "Rejected";
    }
    return "?";
}

Outcome fromReport(const Report& r) {
    switch (r.verdict()) {
        case Legality::Verdict::Invalid: return Outcome::Invalid;
        case Legality::Verdict::NoProblemsFound: return Outcome::NoProblemsFound;
        case Legality::Verdict::Incomplete: return Outcome::Incomplete;
    }
    return Outcome::Incomplete;
}

const char* severityName(Severity s) {
    switch (s) {
        case Severity::Info: return "Info";
        case Severity::Warning: return "Warning";
        case Severity::Invalid: return "Invalid";
    }
    return "?";
}

struct Text {
    Severity severity;
    std::string text;
};

Text info(std::string t) { return {Severity::Info, std::move(t)}; }
Text warning(std::string t) { return {Severity::Warning, std::move(t)}; }
Text invalid(std::string t) { return {Severity::Invalid, std::move(t)}; }

struct Expect {
    Outcome outcome = Outcome::Incomplete;
    std::string parserError;    // Rejected: the parser's errorName()
    std::vector<Text> present;  // issues the report must contain (substring match)
    std::vector<Text> absent;   // issues it must not contain
    bool structureNone = false; // the report must say the structure was not read
    // A known engine bug. The expected verdict stays what the evidence says. While the
    // engine still gives this wrong verdict, the row reads BUG and does not fail the run.
    std::optional<Outcome> knownBug;
    std::string bugNote;
};

Expect expect(Outcome o, std::vector<Text> present = {}, std::vector<Text> absent = {}) {
    Expect e;
    e.outcome = o;
    e.present = std::move(present);
    e.absent = std::move(absent);
    return e;
}

Expect rejected(std::string error) {
    Expect e;
    e.outcome = Outcome::Rejected;
    e.parserError = std::move(error);
    return e;
}

struct Actual {
    Outcome outcome = Outcome::Rejected;
    std::string parserError;
    std::optional<Report> report;
    std::vector<std::string> problems;  // failed side checks (input changed, ...)
};

bool verbose = false;
int caseCount = 0, failureCount = 0, bugCount = 0;
std::map<std::string, std::map<std::string, int>> tally;  // title -> kind -> cases

const char* const kTitles[] = {"Red", "Blue", "Yellow", "Gold", "Silver", "Crystal",
                               "Ruby", "Sapphire", "Emerald", "FireRed", "LeafGreen",
                               "Diamond", "Pearl", "Platinum", "HeartGold", "SoulSilver",
                               "Black", "White", "Black 2", "White 2"};
const char* const kKinds[] = {"positive", "negative", "malformed", "unknown", "original-game", "title-specific"};

std::string label(Outcome o, const std::string& error) {
    std::string s = outcomeName(o);
    if (o == Outcome::Rejected)
        s += " (" + error + ")";
    return s;
}

void record(const std::string& title, const std::string& kind, const std::string& name, const Expect& e,
            const Actual& a) {
    std::vector<std::string> why = a.problems;
    // The kinds carry their own promise, whatever the case expects.
    if (kind == "malformed" && (e.outcome == Outcome::NoProblemsFound || a.outcome == Outcome::NoProblemsFound))
        why.push_back("a malformed record must never read \"No problems found\"");
    if (kind == "unknown" && (e.outcome != Outcome::Incomplete || a.outcome != Outcome::Incomplete))
        why.push_back("an undecidable history must read Incomplete");
    if (a.outcome != e.outcome)
        why.push_back("verdict differs");
    else if (e.outcome == Outcome::Rejected && a.parserError != e.parserError)
        why.push_back("parser error differs");
    if (a.report) {
        const Report& r = *a.report;
        if (!Gen5TestHelpers::consistent(r))
            why.push_back("verdict does not follow from the issues and coverage");
        for (const Text& t : e.present)
            if (!hasText(r, t.severity, t.text))
                why.push_back(std::string("missing ") + severityName(t.severity) + ": " + t.text);
        for (const Text& t : e.absent)
            if (hasText(r, t.severity, t.text))
                why.push_back(std::string("unexpected ") + severityName(t.severity) + ": " + t.text);
        if (e.structureNone && r.coverage.structure != Legality::CoverageLevel::None)
            why.push_back("structure coverage is not None");
    }
    ++caseCount;
    ++tally[title][kind];
    const bool pass = why.empty();
    const bool bug = !pass && e.knownBug && a.outcome == *e.knownBug && a.problems.empty();
    if (bug)
        ++bugCount;
    else if (!pass)
        ++failureCount;
    std::printf("%-10s | %-14s | %-66s | expected %-30s | actual %-30s | %s\n", title.c_str(), kind.c_str(),
                name.c_str(), label(e.outcome, e.parserError).c_str(), label(a.outcome, a.parserError).c_str(),
                pass ? "PASS" : bug ? "BUG" : "FAIL");
    if (bug)
        std::printf("    known engine bug: %s\n", e.bugNote.c_str());
    if (pass && e.knownBug)
        std::printf("    note: the known engine bug is fixed; drop the knownBug mark and update the doc\n");
    for (const auto& w : why)
        std::printf("    reason: %s\n", w.c_str());
    if ((!pass || verbose) && a.report)
        for (const auto& i : a.report->issues)
            std::printf("    [%s] %s\n", severityName(i.severity), i.text.c_str());
}

void put16(std::vector<uint8_t>& b, std::size_t o, unsigned v) {
    b[o] = static_cast<uint8_t>(v >> 8);
    b[o + 1] = static_cast<uint8_t>(v);
}

void put24(std::vector<uint8_t>& b, std::size_t o, uint32_t v) {
    b[o] = static_cast<uint8_t>(v >> 16);
    b[o + 1] = static_cast<uint8_t>(v >> 8);
    b[o + 2] = static_cast<uint8_t>(v);
}

// Upper-case Latin name in the Gen I/II character set (A = 0x80), 0x50-terminated.
void putName(std::vector<uint8_t>& b, std::size_t o, std::size_t length, const std::string& s) {
    std::fill_n(b.begin() + static_cast<std::ptrdiff_t>(o), length, uint8_t{0x50});
    for (std::size_t i = 0; i < s.size() && i + 1 < length; ++i)
        b[o + i] = static_cast<uint8_t>(0x80 + (s[i] - 'A'));
}

// The games' stat formula (pokered home/move_mon.asm CalcStat, pokecrystal
// engine/pokemon/move_mon.asm CalcMonStatC): ((base + DV) * 2 + ceil(sqrt(stat exp)) / 4)
// * level / 100 + 5, and + level + 10 instead of + 5 for HP.
unsigned gbStat(uint8_t base, uint8_t dv, uint16_t statExp, uint8_t level, bool hp) {
    unsigned root = 0;
    while (root * root < statExp)
        ++root;
    return ((base + dv) * 2u + root / 4u) * level / 100u + (hp ? level + 10u : 5u);
}

using Tweak = std::function<void(std::vector<uint8_t>&)>;

// Battle stat experience with one stat still zero: only possible when every one of
// those battles added nothing to that stat (pokered engine/battle/experience.asm divides
// the gains among the Pokemon that fought), so the analyzer cannot settle it.
template <typename M>
M splitStatExp(M m) {
    m.statExp = {7, 0, 3, 3, 3};
    return m;
}

// ---------------------------------------------------------------- Gen I

namespace gen1 {

namespace S = PokeVault::Integration::Gen1;

// International Red/Blue/Yellow main-data layout (the parser's kInternational).
constexpr std::size_t kSize = 0x8000, kMain = 0x2598, kMainLength = 0x0F8B, kChecksum = 0x3523;
constexpr std::size_t kMoney = 0x25F3, kTid = 0x2605, kPikachuFriendship = 0x271C, kBoxIndex = 0x284C;
constexpr std::size_t kStarter = 0x29C3, kParty = 0x2F2C, kBox = 0x30C0;
constexpr std::size_t kBody = kParty + 8, kOt = kBody + 6 * 44, kNick = kOt + 6 * 11;

struct Mon {
    uint16_t species = 0;  // national dex number
    uint8_t level = 0;
    uint32_t exp = 0;
    std::array<uint8_t, 5> base{};  // pret base stats: HP Atk Def Spd Spc
    uint8_t type1 = 0, type2 = 0, catchRate = 0;
    std::array<uint8_t, 4> moves{}, pp{};
    std::array<uint16_t, 5> statExp{};
    std::string nickname;
};

uint8_t internalIndex(uint16_t national) {
    for (int i = 1; i < 255; ++i)
        if (S::gen1InternalToNational(static_cast<uint8_t>(i)) == national)
            return static_cast<uint8_t>(i);
    return 0;
}

uint8_t missingNoIndex() {
    for (int i = 1; i < 255; ++i)
        if (S::gen1InternalToNational(static_cast<uint8_t>(i)) == 0)
            return static_cast<uint8_t>(i);
    return 0;
}

void refreshChecksum(std::vector<uint8_t>& b) {
    b[kChecksum] = S::calculateDiff8(std::span<const uint8_t>(b).subspan(kMain, kMainLength));
}

std::string player(const std::string& title) { return title == "Red" ? "RED" : title == "Blue" ? "BLUE" : "YELLOW"; }

// A synthetic international save with one party Pokemon. The starter byte (and, for
// Yellow, the Pikachu friendship byte) is the parser's Red/Blue against Yellow evidence.
std::vector<uint8_t> save(const std::string& title, const Mon& m, const Tweak& tweak = nullptr) {
    std::vector<uint8_t> b(kSize, 0);
    putName(b, kMain, 11, player(title));
    b[kMoney + 1] = 0x30;  // 3000, packed BCD
    put16(b, kTid, 1234);
    b[kBoxIndex] = 0;  // box 1; stored boxes not initialized
    b[kBox] = 0;
    b[kBox + 1] = 0xFF;
    if (title == "Yellow") {
        b[kStarter] = internalIndex(25);
        b[kPikachuFriendship] = 90;
    } else {
        b[kStarter] = internalIndex(title == "Red" ? 1 : 7);
    }
    const uint8_t internal = internalIndex(m.species);
    b[kParty] = 1;
    b[kParty + 1] = internal;
    b[kParty + 2] = 0xFF;
    const std::size_t o = kBody;
    constexpr std::array<uint8_t, 5> dv{0, 8, 8, 8, 8};  // DV word 0x8888: HP DV 0
    std::array<unsigned, 5> stats{};
    for (std::size_t i = 0; i < 5; ++i)
        stats[i] = gbStat(m.base[i], dv[i], m.statExp[i], m.level, i == 0);
    b[o] = internal;
    put16(b, o + 0x01, stats[0]);
    b[o + 0x03] = m.level;
    b[o + 0x05] = m.type1;
    b[o + 0x06] = m.type2;
    b[o + 0x07] = m.catchRate;
    for (std::size_t i = 0; i < 4; ++i) {
        b[o + 0x08 + i] = m.moves[i];
        b[o + 0x1D + i] = m.pp[i];
    }
    put16(b, o + 0x0C, 1234);
    put24(b, o + 0x0E, m.exp);
    for (std::size_t i = 0; i < 5; ++i)
        put16(b, o + 0x11 + 2 * i, m.statExp[i]);
    put16(b, o + 0x1B, 0x8888);
    b[o + 0x21] = m.level;
    for (std::size_t i = 0; i < 5; ++i)
        put16(b, o + 0x22 + 2 * i, stats[i]);
    putName(b, kOt, 11, player(title));
    putName(b, kNick, 11, m.nickname);
    if (tweak)
        tweak(b);
    refreshChecksum(b);
    return b;
}

S::SourceGame hint(const std::string& title) {
    return title == "Red" ? S::SourceGame::Red : title == "Blue" ? S::SourceGame::Blue : S::SourceGame::Yellow;
}

// Parse with the title's hint and analyze the party Pokemon the way the app does.
Actual run(const std::vector<uint8_t>& bytes, S::SourceGame game, const std::string& wantId) {
    Actual a;
    std::vector<uint8_t> input = bytes;
    const auto parsed = S::parse(input, game);
    if (!parsed) {
        a.parserError = S::errorName(parsed.error);
        if (input != bytes)
            a.problems.push_back("save bytes changed");
        return a;
    }
    const auto& sav = *parsed.save;
    if (sav.metadata().sourceGameId != wantId)
        a.problems.push_back("sourceGameId is " + sav.metadata().sourceGameId);
    if (sav.party().size() != 1) {
        a.problems.push_back("party does not hold one Pokemon");
        return a;
    }
    const Pokemon::Pokemon1ReadOnly pk(sav.party().front());
    a.report = Legality::analyze(pk, pk.getGameGroup(), sav.metadata().sourceGameId);
    a.outcome = fromReport(*a.report);
    if (!Gen5TestHelpers::sameReport(*a.report, Legality::analyze(pk, pk.getGameGroup(), sav.metadata().sourceGameId)))
        a.problems.push_back("a second analysis gave a different report");
    const auto src = sav.sourceBytes();
    if (input != bytes || !std::equal(src.begin(), src.end(), bytes.begin(), bytes.end()))
        a.problems.push_back("save bytes changed");
    return a;
}

Actual run(const std::string& title, const std::vector<uint8_t>& bytes) {
    return run(bytes, hint(title), S::sourceGameId(hint(title)));
}

// pret pokered data/pokemon/base_stats/*.asm, the same in pokeyellow unless noted.
Mon pidgey3() {  // Route 1 grass, level 3 (pokered data/wild/maps/Route1.asm:3)
    Mon m;
    m.species = 16;
    m.level = 3;
    m.exp = 57;  // medium slow, level 3
    m.base = {40, 45, 40, 56, 35};
    m.type1 = 0x00;  // NORMAL
    m.type2 = 0x02;  // FLYING
    m.catchRate = 255;
    m.moves = {16, 0, 0, 0};  // Gust (level-1 move; Sand-Attack comes at 5)
    m.pp = {35, 0, 0, 0};
    m.nickname = "PIDGEY";
    return m;
}

Mon ekans6() {  // Route 4 grass, Red only (pokered data/wild/maps/Route4.asm:6-7)
    Mon m;
    m.species = 23;
    m.level = 6;
    m.exp = 216;  // medium fast
    m.base = {35, 60, 44, 55, 40};
    m.type1 = m.type2 = 0x03;  // POISON
    m.catchRate = 255;
    m.moves = {35, 43, 0, 0};  // Wrap, Leer (level-1 moves; Poison Sting at 10)
    m.pp = {20, 30, 0, 0};
    m.nickname = "EKANS";
    return m;
}

Mon rattata2() {  // Route 1 grass, level 2 (pokered data/wild/maps/Route1.asm:6)
    Mon m;
    m.species = 19;
    m.level = 2;
    m.exp = 8;  // medium fast
    m.base = {30, 56, 35, 72, 25};
    m.type1 = m.type2 = 0x00;  // NORMAL
    m.catchRate = 255;
    m.moves = {33, 39, 0, 0};  // Tackle, Tail Whip (level-1 moves; Quick Attack at 7)
    m.pp = {35, 30, 0, 0};
    m.nickname = "RATTATA";
    return m;
}

Mon sandshrew6() {  // Route 4 grass, Blue only (pokered data/wild/maps/Route4.asm:15-16)
    Mon m;
    m.species = 27;
    m.level = 6;
    m.exp = 216;  // medium fast
    m.base = {50, 75, 85, 40, 30};
    m.type1 = m.type2 = 0x04;  // GROUND
    m.catchRate = 255;
    m.moves = {10, 0, 0, 0};  // Scratch (Sand-Attack at 10)
    m.pp = {35, 0, 0, 0};
    m.nickname = "SANDSHREW";
    return m;
}

Mon caterpie3() {  // Viridian Forest grass, level 3 (pokeyellow data/wild/maps/ViridianForest.asm:3)
    Mon m;
    m.species = 10;
    m.level = 3;
    m.exp = 27;  // medium fast
    m.base = {45, 30, 35, 45, 20};
    m.type1 = m.type2 = 0x07;  // BUG
    m.catchRate = 255;
    m.moves = {33, 81, 0, 0};  // Tackle, String Shot (level-1 moves; no level-up moves)
    m.pp = {35, 40, 0, 0};
    m.nickname = "CATERPIE";
    return m;
}

Mon starterPikachu() {  // pokeyellow scripts/OaksLab.asm:1033-1040
    Mon m;
    m.species = 25;
    m.level = 5;
    m.exp = 125;  // medium fast
    m.base = {35, 55, 30, 90, 50};
    m.type1 = m.type2 = 0x17;  // ELECTRIC
    m.catchRate = 0xA3;        // LIGHT_BALL_GSC, written over the catch rate by OaksLab.asm:1039-1040
    m.moves = {84, 45, 0, 0};  // ThunderShock, Growl (level-1 moves; Tail Whip at 6)
    m.pp = {30, 40, 0, 0};
    m.nickname = "PIKACHU";
    return m;
}

Mon dragonair15() {  // Safari Zone Center, Super Rod, level 15 (pokeyellow data/wild/super_rod.asm:25)
    Mon m;
    m.species = 148;
    m.level = 15;
    m.exp = 4218;  // slow: 5 * 15^3 / 4
    m.base = {61, 84, 65, 70, 70};
    m.type1 = m.type2 = 0x1A;  // DRAGON
    m.catchRate = 27;          // Yellow's rate (pokeyellow base_stats/dragonair.asm); Red/Blue say 45
    m.moves = {35, 43, 86, 0};  // Wrap, Leer, Thunder Wave (level-1 moves; Agility at 20)
    m.pp = {20, 30, 20, 0};
    m.nickname = "DRAGONAIR";
    return m;
}

void cases() {
    using O = Outcome;
    const auto originLegit = info("A legitimate origin exists");

    // Red
    record("Red", "positive", "Route 1 Pidgey L3, Gust, EXP 57",
           expect(O::NoProblemsFound, {info("Possible origin: wild Pidgey")}), run("Red", save("Red", pidgey3())));
    record("Red", "original-game", "Route 4 Ekans L6, Wrap/Leer (Red-only slot)",
           expect(O::NoProblemsFound, {info("Possible origin: wild Ekans in Red (level 6+)"), originLegit}),
           run("Red", save("Red", ekans6())));
    {
        Mon m = pidgey3();
        m.moves = {16, 147, 0, 0};  // Gust, Spore
        m.pp = {35, 15, 0, 0};
        record("Red", "negative", "Pidgey L3 knowing Spore", expect(O::Invalid, {invalid("Pidgey cannot learn Spore")}),
               run("Red", save("Red", m)));
    }
    {
        auto bytes = save("Red", pidgey3());
        bytes[kChecksum] ^= 0x01;
        record("Red", "malformed", "main-data checksum byte off by one", rejected("checksum_mismatch"),
               run("Red", bytes));
    }
    {
        Mon m = pidgey3();
        m.moves = {16, 166, 0, 0};
        m.pp = {35, 10, 0, 0};
        record("Red", "malformed", "move id 166 (one past Struggle)",
               expect(O::Invalid, {invalid("Move id 166 cannot exist in a Generation 1 save")}),
               run("Red", save("Red", m)));
    }
    record("Red", "unknown", "Pidgey L3 with battle stat exp {7,0,3,3,3}",
           expect(O::Incomplete, {warning("Stat experience from battles with one stat still at zero")}),
           run("Red", save("Red", splitStatExp(pidgey3()))));
    record("Red", "title-specific", "Red save opened as Yellow", rejected("game_hint_mismatch"),
           run(save("Red", pidgey3()), S::SourceGame::Yellow, "yellow_gb"));

    // Blue
    record("Blue", "positive", "Route 1 Rattata L2, Tackle/Tail Whip, EXP 8",
           expect(O::NoProblemsFound, {info("Possible origin: wild Rattata")}), run("Blue", save("Blue", rattata2())));
    record("Blue", "original-game", "Route 4 Sandshrew L6, Scratch (Blue-only slot)",
           expect(O::NoProblemsFound, {info("Possible origin: wild Sandshrew in Blue (level 6+)"), originLegit}),
           run("Blue", save("Blue", sandshrew6())));
    {
        Mon m = rattata2();
        m.type1 = m.type2 = 0x14;  // FIRE
        record("Blue", "negative", "Rattata with Fire type bytes",
               expect(O::Invalid, {invalid("type byte does not match Rattata")}), run("Blue", save("Blue", m)));
    }
    {
        Mon m = rattata2();
        record("Blue", "malformed", "party level byte 101", rejected("invalid_party"),
               run("Blue", save("Blue", m, [](std::vector<uint8_t>& b) { b[kBody + 0x21] = 101; })));
    }
    {
        Mon m = rattata2();
        m.nickname = "";
        record("Blue", "malformed", "nickname field starts with the terminator",
               expect(O::Invalid, {invalid("Nickname is empty")}), run("Blue", save("Blue", m)));
    }
    record("Blue", "unknown", "Rattata L2 with battle stat exp {7,0,3,3,3}",
           expect(O::Incomplete, {warning("Stat experience from battles with one stat still at zero")}),
           run("Blue", save("Blue", splitStatExp(rattata2()))));
    record("Blue", "title-specific", "Blue save opened as Red: accepted, sourceGameId red_gb",
           expect(O::NoProblemsFound, {info("Possible origin: wild Sandshrew in Blue (level 6+)")}),
           run(save("Blue", sandshrew6()), S::SourceGame::Red, "red_gb"));

    // Yellow
    record("Yellow", "positive", "Viridian Forest Caterpie L3, Tackle/String Shot",
           expect(O::NoProblemsFound, {info("Possible origin: wild Caterpie")}),
           run("Yellow", save("Yellow", caterpie3())));
    record("Yellow", "original-game", "starter Pikachu L5, catch-rate byte 0xA3 (Light Ball)",
           expect(O::NoProblemsFound, {originLegit}), run("Yellow", save("Yellow", starterPikachu())));
    record("Yellow", "title-specific", "Safari Zone Dragonair L15, Yellow catch rate 27",
           expect(O::NoProblemsFound, {info("Possible origin: wild Dragonair in Yellow (level 15+)")}),
           run("Yellow", save("Yellow", dragonair15())));
    {
        Mon m = starterPikachu();
        m.moves = {84, 84, 0, 0};
        m.pp = {30, 30, 0, 0};
        record("Yellow", "negative", "Pikachu knowing ThunderShock twice",
               expect(O::Invalid, {invalid("Duplicate move: ")}), run("Yellow", save("Yellow", m)));
    }
    {
        const uint8_t glitch = missingNoIndex();
        record("Yellow", "malformed",
               "party marker and species = internal index " + std::to_string(glitch) + " (no species)",
               rejected("checksum_mismatch"),
               run("Yellow", save("Yellow", caterpie3(), [glitch](std::vector<uint8_t>& b) {
                       b[kParty + 1] = glitch;
                       b[kBody] = glitch;
                   })));
    }
    record("Yellow", "malformed", "OT name bytes 01 02 03 ... (no glyphs)",
           expect(O::Invalid, {invalid("Original trainer name is empty")}),
           run("Yellow", save("Yellow", caterpie3(), [](std::vector<uint8_t>& b) {
                   for (std::size_t i = 0; i < 11; ++i)
                       b[kOt + i] = static_cast<uint8_t>(i + 1);
               })));
    record("Yellow", "unknown", "Caterpie L3 with battle stat exp {7,0,3,3,3}",
           expect(O::Incomplete, {warning("Stat experience from battles with one stat still at zero")}),
           run("Yellow", save("Yellow", splitStatExp(caterpie3()))));
    record("Yellow", "title-specific", "Yellow save opened as Red", rejected("game_hint_mismatch"),
           run(save("Yellow", caterpie3()), S::SourceGame::Red, "red_gb"));
}

}  // namespace gen1

// ---------------------------------------------------------------- Gen II

namespace gen2 {

namespace S = PokeVault::Integration::Gen2;

// International Gold/Silver and Crystal layouts (the parser's kIntlGS and kIntlC).
struct Layout {
    std::size_t money, currentBox, boxNames, party, currentBoxCopy, checksumEnd, checksum;
    bool crystal;
};
constexpr Layout kGoldSilver{0x23DB, 0x2724, 0x2727, 0x288A, 0x2D6C, 0x2D68, 0x2D69, false};
constexpr Layout kCrystal{0x23DC, 0x2700, 0x2703, 0x2865, 0x2D10, 0x2B82, 0x2D0D, true};
constexpr std::size_t kSize = 0x8000, kTid = 0x2009, kPlayer = 0x200B, kGender = 0x3E3D;
constexpr std::size_t kBoxStride = 0x450;  // 1 + 21 + 20 * 32 + 2 * 20 * 11, plus 2 padding bytes

struct Mon {
    uint8_t species = 0, level = 0, item = 0, friendship = 70, pokerus = 0;
    uint32_t exp = 0;
    std::array<uint8_t, 6> base{};  // pret base stats: HP Atk Def Spd SpA SpD
    std::array<uint8_t, 4> moves{}, pp{};
    std::array<uint16_t, 5> statExp{};
    uint16_t caught = 0;  // Crystal caught data, big-endian bytes 29-30
    std::string nickname;
    bool egg = false;
};

const Layout& layout(const std::string& title) { return title == "Crystal" ? kCrystal : kGoldSilver; }
std::string player(const std::string& title) { return title == "Gold" ? "GOLD" : title == "Silver" ? "SILVER" : "KRIS"; }

std::size_t body(const Layout& l) { return l.party + 8; }
std::size_t otName(const Layout& l) { return body(l) + 6 * 48; }

void refreshChecksum(std::vector<uint8_t>& b, const Layout& l) {
    uint16_t sum = 0;
    for (std::size_t i = kTid; i <= l.checksumEnd; ++i)
        sum = static_cast<uint16_t>(sum + b[i]);
    b[l.checksum] = static_cast<uint8_t>(sum);
    b[l.checksum + 1] = static_cast<uint8_t>(sum >> 8);
}

// A synthetic international save: 14 empty boxes in both banks, an empty current-box
// copy, one party Pokemon, and the primary checksum.
std::vector<uint8_t> save(const std::string& title, const Mon& m, const Tweak& tweak = nullptr) {
    const Layout& l = layout(title);
    std::vector<uint8_t> b(kSize, 0);
    put16(b, kTid, 1234);
    putName(b, kPlayer, 11, player(title));
    put24(b, l.money, 3000);
    b[l.currentBox] = 0;
    if (l.crystal)
        b[kGender] = 0;
    b[l.currentBoxCopy] = 0;
    b[l.currentBoxCopy + 1] = 0xFF;
    for (std::size_t box = 0; box < 14; ++box) {
        const std::size_t at = box < 7 ? 0x4000 + box * kBoxStride : 0x6000 + (box - 7) * kBoxStride;
        b[at] = 0;
        b[at + 1] = 0xFF;
        putName(b, l.boxNames + box * 9, 9, std::string("BOX") + static_cast<char>('A' + box));
    }
    b[l.party] = 1;
    b[l.party + 1] = m.egg ? 0xFD : m.species;
    b[l.party + 2] = 0xFF;
    const std::size_t o = body(l);
    const uint8_t atk = 8, def = 8, spe = 8, spc = 8, hp = 0;  // DV bytes 0x88 0x88
    const std::array<unsigned, 6> stats{
        gbStat(m.base[0], hp, m.statExp[0], m.level, true), gbStat(m.base[1], atk, m.statExp[1], m.level, false),
        gbStat(m.base[2], def, m.statExp[2], m.level, false), gbStat(m.base[3], spe, m.statExp[3], m.level, false),
        gbStat(m.base[4], spc, m.statExp[4], m.level, false), gbStat(m.base[5], spc, m.statExp[4], m.level, false)};
    b[o] = m.species;
    b[o + 1] = m.item;
    for (std::size_t i = 0; i < 4; ++i) {
        b[o + 2 + i] = m.moves[i];
        b[o + 23 + i] = m.pp[i];
    }
    put16(b, o + 6, 1234);
    put24(b, o + 8, m.exp);
    for (std::size_t i = 0; i < 5; ++i)
        put16(b, o + 11 + 2 * i, m.statExp[i]);
    b[o + 21] = 0x88;
    b[o + 22] = 0x88;
    b[o + 27] = m.friendship;
    b[o + 28] = m.pokerus;
    put16(b, o + 29, m.caught);
    b[o + 31] = m.level;
    put16(b, o + 34, stats[0]);
    for (std::size_t i = 0; i < 6; ++i)
        put16(b, o + 36 + 2 * i, stats[i]);
    putName(b, otName(l), 11, player(title));
    putName(b, otName(l) + 6 * 11, 11, m.nickname);
    if (tweak)
        tweak(b);
    refreshChecksum(b, l);
    return b;
}

S::SourceGame hint(const std::string& title) {
    return title == "Gold" ? S::SourceGame::Gold : title == "Silver" ? S::SourceGame::Silver : S::SourceGame::Crystal;
}

Actual run(const std::vector<uint8_t>& bytes, S::SourceGame game, const std::string& wantId) {
    Actual a;
    std::vector<uint8_t> input = bytes;
    const auto parsed = S::parse(input, game);
    if (!parsed) {
        a.parserError = S::errorName(parsed.error);
        if (input != bytes)
            a.problems.push_back("save bytes changed");
        return a;
    }
    const auto& sav = *parsed.save;
    if (sav.metadata().sourceGameId != wantId)
        a.problems.push_back("sourceGameId is " + sav.metadata().sourceGameId);
    if (sav.party().size() != 1) {
        a.problems.push_back("party does not hold one Pokemon");
        return a;
    }
    const Pokemon::Pokemon2ReadOnly pk(sav.party().front());
    a.report = Legality::analyze(pk, pk.getGameGroup(), sav.metadata().sourceGameId);
    a.outcome = fromReport(*a.report);
    if (!Gen5TestHelpers::sameReport(*a.report, Legality::analyze(pk, pk.getGameGroup(), sav.metadata().sourceGameId)))
        a.problems.push_back("a second analysis gave a different report");
    const auto src = sav.sourceBytes();
    if (input != bytes || !std::equal(src.begin(), src.end(), bytes.begin(), bytes.end()))
        a.problems.push_back("save bytes changed");
    return a;
}

Actual run(const std::string& title, const std::vector<uint8_t>& bytes) {
    return run(bytes, hint(title), S::sourceGameId(hint(title)));
}

// pret pokegold / pokecrystal data/pokemon/base_stats/*.asm and evos_attacks.asm.
Mon sentret3() {  // Route 29 morning, level 3 (pokegold data/wild/johto_grass.asm:1577)
    Mon m;
    m.species = 161;
    m.level = 3;
    m.exp = 27;  // medium fast
    m.base = {35, 46, 34, 20, 35, 45};
    m.moves = {33, 0, 0, 0};  // Tackle (Defense Curl at 5)
    m.pp = {35, 0, 0, 0};
    m.nickname = "SENTRET";
    return m;
}

Mon spinarak3() {  // Route 30 night, Gold only (pokegold data/wild/johto_grass.asm:1603,1621)
    Mon m;
    m.species = 167;
    m.level = 3;
    m.exp = 21;  // fast: 4 * 27 / 5
    m.base = {40, 60, 40, 30, 40, 40};
    m.moves = {40, 81, 0, 0};  // Poison Sting, String Shot (level-1 moves; Scary Face at 6)
    m.pp = {35, 40, 0, 0};
    m.nickname = "SPINARAK";
    return m;
}

Mon hoothoot2() {  // Route 29 night, level 2 (pokegold data/wild/johto_grass.asm:1592)
    Mon m;
    m.species = 163;
    m.level = 2;
    m.exp = 8;  // medium fast
    m.base = {60, 30, 30, 50, 36, 56};
    m.moves = {33, 45, 0, 0};  // Tackle, Growl (level-1 moves; Foresight at 6)
    m.pp = {35, 40, 0, 0};
    m.nickname = "HOOTHOOT";
    return m;
}

Mon ledyba3() {  // Route 30 morning, Silver only (pokegold data/wild/johto_grass.asm:1628,1630)
    Mon m;
    m.species = 165;
    m.level = 3;
    m.exp = 21;  // fast
    m.base = {40, 20, 30, 55, 40, 80};
    m.moves = {33, 0, 0, 0};  // Tackle (Supersonic at 8)
    m.pp = {35, 0, 0, 0};
    m.nickname = "LEDYBA";
    return m;
}

// Route 29 morning Pidgey L2 caught in Crystal: caught data = morning (1) << 14 |
// level 2 << 8 | male OT | landmark Route 29 (2) (pokecrystal data/wild/johto_grass.asm:1240,
// engine/pokemon/caught_data.asm:163-199, constants/landmark_constants.asm:7).
constexpr uint16_t kRoute29Morning2 = (1u << 14) | (2u << 8) | 2u;

Mon pidgey2Crystal() {
    Mon m;
    m.species = 16;
    m.level = 2;
    m.exp = 9;  // medium slow, level 2
    m.base = {40, 45, 40, 56, 35, 35};
    m.moves = {33, 0, 0, 0};  // Tackle (Sand-Attack at 5)
    m.pp = {35, 0, 0, 0};
    m.caught = kRoute29Morning2;
    m.nickname = "PIDGEY";
    return m;
}

// The unhatched Odd Egg Pichu as pokecrystal writes it: data/events/odd_eggs.asm:40-64
// copied verbatim into the party by engine/events/odd_egg.asm (_GiveOddEgg) and
// AddMobileMonToParty; party species EGG ($FD), OT name "ODD" (odd_egg.asm:93-94).
// The OT ID line is "dw 02048": little-endian, so the bytes are 00 08.
constexpr std::array<uint8_t, 48> kOddEggPichu{
    172, 0, 84, 204, 146, 0,  // PICHU, NO_ITEM, ThunderShock, Charm, Dizzy Punch
    0x00, 0x08,               // dw 02048
    0x00, 0x00, 125,          // bigdt 125
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // stat exp
    0x00, 0x00,               // dn 0, 0, 0, 0
    30, 20, 10, 0,            // PP
    20,                       // egg cycles
    0, 0, 0,                  // Pokerus, caught data
    5,                        // level
    0, 0,                     // status
    0, 0, 0, 17, 0, 9, 0, 6, 0, 11, 0, 8, 0, 8};  // HP, Max HP, Atk, Def, Spd, SAtk, SDef

void cases() {
    using O = Outcome;
    const auto gsWild = info("Species has pinned Gold/Silver wild-slot provenance");
    const auto crystalWild = info("matches a pinned Crystal wild encounter slot");
    const auto splitWarning = warning("Stat experience from battles with one stat still at zero");

    // Gold
    record("Gold", "positive", "Route 29 Sentret L3, Tackle, EXP 27",
           expect(O::NoProblemsFound, {info("Possible origin: wild Sentret"), gsWild}), run("Gold", save("Gold", sentret3())));
    record("Gold", "original-game", "Route 30 Spinarak L3, Poison Sting/String Shot (Gold-only slot)",
           expect(O::NoProblemsFound, {gsWild}), run("Gold", save("Gold", spinarak3())));
    record("Gold", "title-specific", "the same Spinarak in a Silver save: no Gold/Silver wild note",
           expect(O::NoProblemsFound, {}, {gsWild}), run("Silver", save("Silver", spinarak3())));
    {
        Mon m = sentret3();
        m.pokerus = 0x90;
        record("Gold", "negative", "Sentret with Pokerus byte 0x90 (strain 9)",
               expect(O::Invalid, {invalid("Pokerus strain 9 does not exist in Gen II")}), run("Gold", save("Gold", m)));
    }
    {
        auto bytes = save("Gold", sentret3());
        bytes[kGoldSilver.checksum] ^= 0x01;
        record("Gold", "malformed", "primary checksum off by one", rejected("checksum_mismatch"), run("Gold", bytes));
    }
    {
        Mon m = sentret3();
        m.moves = {33, 252, 0, 0};
        m.pp = {35, 5, 0, 0};
        record("Gold", "malformed", "move id 252 (one past Beat Up)",
               expect(O::Invalid, {invalid("Move id 252 cannot exist in a Generation 2 save")}),
               run("Gold", save("Gold", m)));
    }
    record("Gold", "unknown", "Sentret L3 with battle stat exp {7,0,3,3,3}", expect(O::Incomplete, {splitWarning}),
           run("Gold", save("Gold", splitStatExp(sentret3()))));

    // Silver
    record("Silver", "positive", "Route 29 Hoothoot L2, Tackle/Growl, EXP 8",
           expect(O::NoProblemsFound, {info("Possible origin: wild Hoothoot"), gsWild}),
           run("Silver", save("Silver", hoothoot2())));
    record("Silver", "original-game", "Route 30 Ledyba L3, Tackle (Silver-only slot)",
           expect(O::NoProblemsFound, {gsWild}), run("Silver", save("Silver", ledyba3())));
    record("Silver", "title-specific", "the same Ledyba in a Gold save: no Gold/Silver wild note",
           expect(O::NoProblemsFound, {}, {gsWild}), run("Gold", save("Gold", ledyba3())));
    {
        Mon m = hoothoot2();
        m.item = 7;  // BICYCLE: a CANT_TOSS key item (pokegold data/items/attributes.asm:23-24)
        record("Silver", "negative", "Hoothoot holding item 7 (Bicycle)",
               expect(O::Invalid, {invalid("Held item 7 cannot be held in Gold/Silver/Crystal")}),
               run("Silver", save("Silver", m)));
    }
    record("Silver", "malformed", "party marker Hoothoot, body species Sentret", rejected("invalid_party"),
           run("Silver", save("Silver", hoothoot2(), [](std::vector<uint8_t>& b) { b[body(kGoldSilver)] = 161; })));
    record("Silver", "malformed", "OT name field starts with the terminator",
           expect(O::Invalid, {invalid("Original trainer name is empty")}),
           run("Silver", save("Silver", hoothoot2(), [](std::vector<uint8_t>& b) { b[otName(kGoldSilver)] = 0x50; })));
    record("Silver", "unknown", "Hoothoot L2 with battle stat exp {7,0,3,3,3}", expect(O::Incomplete, {splitWarning}),
           run("Silver", save("Silver", splitStatExp(hoothoot2()))));

    // Crystal
    record("Crystal", "positive", "Route 29 Pidgey L2, morning caught data",
           expect(O::NoProblemsFound, {info("Possible origin: wild Pidgey"), crystalWild}),
           run("Crystal", save("Crystal", pidgey2Crystal())));
    {
        Mon m;
        m.species = 172;
        m.egg = true;
        record("Crystal", "original-game", "unhatched Odd Egg Pichu, bytes from odd_eggs.asm",
               expect(O::NoProblemsFound, {info("A legitimate origin exists")}),
               run("Crystal", save("Crystal", m, [](std::vector<uint8_t>& b) {
                       std::copy(kOddEggPichu.begin(), kOddEggPichu.end(), b.begin() + static_cast<std::ptrdiff_t>(body(kCrystal)));
                       putName(b, otName(kCrystal), 11, "ODD");
                       putName(b, otName(kCrystal) + 6 * 11, 11, "EGG");
                   })));
    }
    {
        Mon m = pidgey2Crystal();
        m.caught = (1u << 14) | (3u << 8) | 2u;  // met at level 3 (a real Route 29 morning slot)
        record("Crystal", "negative", "Pidgey L2 with met level 3",
               expect(O::Invalid, {invalid("Level is below the Crystal met level")}), run("Crystal", save("Crystal", m)));
    }
    {
        auto bytes = save("Crystal", pidgey2Crystal());
        bytes.pop_back();
        record("Crystal", "malformed", "save one byte short (0x7FFF bytes)", rejected("wrong_size"),
               run("Crystal", bytes));
    }
    {
        Mon m = pidgey2Crystal();
        m.caught = 0xFFFF;
        record("Crystal", "malformed", "caught data FFFF (night, met level 63, landmark 127)",
               expect(O::Invalid, {invalid("Level is below the Crystal met level")}), run("Crystal", save("Crystal", m)));
    }
    record("Crystal", "unknown", "Pidgey L2 with battle stat exp {7,0,3,3,3}", expect(O::Incomplete, {splitWarning}),
           run("Crystal", save("Crystal", splitStatExp(pidgey2Crystal()))));
    record("Crystal", "title-specific", "Crystal save opened as Gold", rejected("game_hint_mismatch"),
           run(save("Crystal", pidgey2Crystal()), S::SourceGame::Gold, "gold_gbc"));
    record("Crystal", "title-specific", "Crystal-caught Pidgey traded into a Gold save: no Crystal slot note",
           expect(O::NoProblemsFound, {}, {crystalWild, gsWild}), run("Gold", save("Gold", pidgey2Crystal())));
}

}  // namespace gen2

// ---------------------------------------------------------------- Gen III

namespace gen3 {

namespace G3 = PokeVault::Integration::Gen3;
using Gen5TestHelpers::fromHex;

// Decrypted 80-byte PK3 box records (G/A/E/M order) from the PKHeX-generated corpus of
// tools/pkhex_gen34_diff (`run.sh PKHEX 1 3 3`, PKHeX kwsch/PKHeX@6501f0ab), built by PKHeX
// for OT "Tester". PKHeX calls each one VALID. The corpus id follows each name.
// Ruby: Zangoose L16, Route 114 (met location 29), a Ruby-only grass slot (b26412).
constexpr const char* kRubyZangoose =
    "EC7557C6059AD832D4BBC8C1C9C9CDBFFF000202CED9E7E8D9E6FF00047000007C010000E11A0000004600002B0062000E00D2001E1E1E14000000000000000000000000001D10218D63613800000000";
// Sapphire: Seviper L16, Route 114, a Sapphire-only grass slot (b26472).
constexpr const char* kSapphireSeviper =
    "90502E3264BAAC36CDBFD0C3CABFCCFF00000202CED9E7E8D9E6FF00EC0E00007B010000990900000046000023007A002C005601141E1919000000000000000000000000001D90203A10C23600000000";
// Sapphire: the roaming Latias, L40, met location 16 (b28744).
constexpr const char* kSapphireRoamerLatias =
    "3C4D350AC7503EAFC6BBCEC3BBCDFF0000000202CED9E7E8D9E6FF00D3E600009701000080380100005A00005A011F0128015E000F14050A0000000000000000000000000010A8200000000000000000";
// Sapphire: Seedot L3, Route 102 (met location 17), mass outbreak, moves Bide, Harden and
// Leech Seed (b20200).
constexpr const char* kSapphireOutbreakSeedot =
    "7EADD0DF6ED1B02ACDBFBFBEC9CEFF0000000202CED9E7E8D9E6FF007E6300002A010000390000000046000075006A00490000000A1E0A000000000000000000000000000011832047B6151500000000";
// Emerald: Smeargle L40, Artisan Cave (met location 202), an Emerald-only grass slot (b19084).
constexpr const char* kEmeraldSmeargle =
    "14E4622DC2D1572FCDC7BFBBCCC1C6BFFF000202CED9E7E8D9E6FF0096A70000EB00000000C8000000460000A6000000000000000100000000000000000000000000000000CAA821908ECC1D00000000";
// Emerald: Lugia L70 from Navel Rock, fateful flag set (tests/test_legality_gen3.cpp b19244).
constexpr const char* kEmeraldLugia =
    "A700ECFA4F76F81AC6CFC1C3BBFF000000000202CED9E7E8D9E6FF00B1980000F9000000CE8A06000000000069003800F00081001405051400000000000000000000000000D3C6212070D30C00000080";
// FireRed: Scyther L25, the Celadon Game Corner prize (met location 94) (b13328).
constexpr const char* kFireRedScyther =
    "444F0D1B2C0C571BCDBDD3CEC2BFCCFF00000202CED9E7E8D9E6FF00FD7200007B000000093D0000004600007400E400CE0061001E14281E000000000000000000000000005E19226A09293100000000";
// LeafGreen: Bellsprout L13, Route 5 (met location 105), a LeafGreen-only grass slot (b6228).
constexpr const char* kLeafGreenBellsprout =
    "6529AEEFD0FCDD0DBCBFC6C6CDCACCC9CFCE0202CED9E7E8D9E6FF00EA64000045000000ED0400000046000016004A00230000000A28140000000000000000000000000000698D224140492500000000";

// Colosseum/XD fixtures shared with tests/test_legality_gen3.cpp (PKHeX: VALID). Both are
// Shadow Pokemon whose team lock the Gen3Cxd judge verifies; the e-Reader shadows carry
// fixed zero IVs, which PKHeX only calls Fishy (IVAllEqual_0), so they stay Incomplete.
constexpr const char* kColoShadowBayleef =
    "3F4B16DB8CFE8C44BCBBD3C6BFBFC0FF00000202CED9E7E8D9E6FF00130F0000990000000055000000460000F100EB004B0022000505190F00000000000000000000000000039E27D9249C0C00000001";
constexpr const char* kXdShadowZapdos =
    "22F2467BC84C2639D4BBCABEC9CDFF0000000202CED9E7E8D9E6FF00285F0000910000005A620200002300004601E2003F0155001E28280F000000000000000000000000004AB227BAA5CD0500000081";
constexpr const char* kEReaderTogepi =
    "065CEF6E023F32FA648A9CAEFF00000000000102635D64FF000000006C290000AF00000000190000004600007600CC00BA0019010A140A0A000000000000000000000000008094270000000000000001";
constexpr const char* kEReaderMareep =
    "684E6DDC7B2ED1E07278AE9DFF00000000000102635D64FF0000000049E20000B300000020AB000000460000570054005600B2000A1E14280000000000000000000000000080A5270000000000000001";
// LeafGreen: Pinsir L18, the Celadon Game Corner prize (b13480).
constexpr const char* kLeafGreenPinsir =
    "0C4D378AB71EE760CAC3C8CDC3CCFF0000000202CED9E7E8D9E6FF0092A000007F0000007A1C0000004600000B007400140045001E1E1414000000000000000000000000005E92223286CB0300000000";

constexpr std::size_t kChecksum = 0x1C, kSpecies = 0x20, kItem = 0x22, kExp = 0x24, kMoves = 0x2C, kEvs = 0x38;
constexpr std::size_t kPokerus = 0x44, kOrigins = 0x46, kFateful = 0x4F;
constexpr uint8_t kSapphire = 1, kRuby = 2, kFireRed = 4, kLeafGreen = 5, kColosseumXD = 15;
constexpr uint16_t kEnigmaBerry = 175;

uint16_t le16(const std::vector<uint8_t>& b, std::size_t o) {
    return static_cast<uint16_t>(b[o] | (b[o + 1] << 8));
}

void setLe16(std::vector<uint8_t>& b, std::size_t o, unsigned v) {
    b[o] = static_cast<uint8_t>(v);
    b[o + 1] = static_cast<uint8_t>(v >> 8);
}

void setLe32(std::vector<uint8_t>& b, std::size_t o, uint32_t v) {
    setLe16(b, o, v & 0xFFFF);
    setLe16(b, o + 2, v >> 16);
}

void refreshChecksum(std::vector<uint8_t>& pk) {
    setLe16(pk, kChecksum, Legality::Gen3Pk3::checksum(pk));
}

std::vector<uint8_t> edited(const char* hex, const std::function<void(std::vector<uint8_t>&)>& edit) {
    auto pk = fromHex(hex);
    edit(pk);
    refreshChecksum(pk);
    return pk;
}

// Origins word (0x46): met level bits 0-6, version bits 7-10, ball bits 11-14, OT gender bit 15.
std::vector<uint8_t> withVersion(const char* hex, uint8_t version) {
    return edited(hex, [version](std::vector<uint8_t>& pk) {
        setLe16(pk, kOrigins, (le16(pk, kOrigins) & ~(0xFu << 7)) | (static_cast<unsigned>(version) << 7));
    });
}

std::vector<uint8_t> withMetLevel(const char* hex, uint8_t level) {
    return edited(hex, [level](std::vector<uint8_t>& pk) {
        setLe16(pk, kOrigins, (le16(pk, kOrigins) & ~0x7Fu) | level);
    });
}

// The record as a save stores it: the 48 data bytes encrypted with PID ^ OT ID and
// shuffled by PID % 24 (Encryption3FRLG).
std::vector<uint8_t> stored(const std::vector<uint8_t>& pk) {
    const auto in = std::span<const std::byte>(reinterpret_cast<const std::byte*>(pk.data()), pk.size());
    std::unique_ptr<std::byte[]> enc(Encryption::encryptArray3FRLG(in));
    std::vector<uint8_t> out(pk.size());
    std::memcpy(out.data(), enc.get(), out.size());
    return out;
}

// Record path: the stored bytes go through Pokemon3FRLG and Legality::analyze with the
// save group and the exact source game id, as PokemonDetailsModal.cpp does for a party or
// box slot (a Bank slot passes no source id). analyzePk3 on the same bytes must agree.
Actual run(const std::vector<uint8_t>& pk, const std::string& game) {
    Actual a;
    const auto copy = pk;
    const auto bytes = stored(pk);
    const Pokemon::Pokemon3FRLG mon(std::span<const std::byte>(reinterpret_cast<const std::byte*>(bytes.data()), bytes.size()));
    a.report = Legality::analyze(mon, mon.getGameGroup(), game);
    a.outcome = fromReport(*a.report);
    if (!Gen5TestHelpers::sameReport(*a.report, Legality::analyze(mon, mon.getGameGroup(), game)))
        a.problems.push_back("a second analysis gave a different report");
    const auto pk3 = Legality::analyzePk3(pk, game).verdict();
    if (pk3 == Legality::Verdict::Invalid && a.outcome != Outcome::Invalid)
        a.problems.push_back("Legality::analyze dropped an analyzePk3 Invalid");
    if (a.outcome == Outcome::NoProblemsFound && pk3 != Legality::Verdict::NoProblemsFound)
        a.problems.push_back("Legality::analyze certified what analyzePk3 did not");
    if (pk != copy)
        a.problems.push_back("record bytes changed");
    return a;
}

// Engine entry point for bytes the save reader never hands over (a record that is not
// 80 or 100 bytes long).
Actual runPk3(const std::vector<uint8_t>& pk, const std::string& game) {
    Actual a;
    const auto copy = pk;
    a.report = Legality::analyzePk3(pk, game);
    a.outcome = fromReport(*a.report);
    if (pk != copy)
        a.problems.push_back("record bytes changed");
    return a;
}

// ------------------------------------------------ synthetic 128 KiB GBA saves

enum class Family { RubySapphire, Emerald, FireRedLeafGreen };

constexpr std::size_t kSaveSize = 0x20000, kSectorSize = 0x1000, kSectorData = 0xF80;
// Bytes each section's checksum covers (Gen3SaveValidation.h kChunkLengths).
constexpr std::array<std::size_t, 14> kChunkLengths = {0xF2C, 0xF80, 0xF80, 0xF80, 0xF08, 0xF80, 0xF80,
                                                       0xF80, 0xF80, 0xF80, 0xF80, 0xF80, 0xF80, 0x7D0};
constexpr uint32_t kTesterId = 0x30394B2Du;  // trainer ID word of the save that receives trades
constexpr uint32_t kEmeraldSecurityKey = 0xA1B2C3D4u;

uint16_t sectorChecksum(const uint8_t* data, std::size_t length) {
    uint32_t sum = 0;
    for (std::size_t o = 0; o < length; o += 4)
        sum += static_cast<uint32_t>(data[o]) | static_cast<uint32_t>(data[o + 1]) << 8 |
               static_cast<uint32_t>(data[o + 2]) << 16 | static_cast<uint32_t>(data[o + 3]) << 24;
    return static_cast<uint16_t>((sum & 0xFFFF) + (sum >> 16));
}

uint32_t trainerIdOf(const std::vector<uint8_t>& pk) {
    return static_cast<uint32_t>(le16(pk, 4)) | static_cast<uint32_t>(le16(pk, 6)) << 16;
}

// Save slot A holds sections 0-13 in order (counter 1); slot B is blank. Section 0: trainer
// "Tester" with the given ID word, and the family word at 0xAC: 1 on FireRed/LeafGreen, the
// security key on Emerald, 0 on Ruby/Sapphire. Section 1: money 0 (stored XOR the key on
// Emerald) and an empty party. Section 5: current box 0, then box 1 slot 1 holds `boxed`.
std::vector<uint8_t> save(Family family, const std::vector<uint8_t>& boxed, uint32_t trainerId) {
    std::array<std::vector<uint8_t>, 14> section;
    for (auto& s : section)
        s.assign(kSectorData, 0);
    const uint8_t tester[8] = {0xCE, 0xD9, 0xE7, 0xE8, 0xD9, 0xE6, 0xFF, 0xFF};
    std::copy(std::begin(tester), std::end(tester), section[0].begin());
    setLe32(section[0], 0x0A, trainerId);
    if (family == Family::FireRedLeafGreen)
        setLe32(section[0], 0xAC, 1);
    if (family == Family::Emerald) {
        setLe32(section[0], 0xAC, kEmeraldSecurityKey);
        setLe32(section[1], 0x490, kEmeraldSecurityKey);  // money 0
    }
    const auto record = stored(boxed);
    std::copy(record.begin(), record.end(), section[5].begin() + 4);
    std::vector<uint8_t> b(kSaveSize, 0);
    for (std::size_t id = 0; id < section.size(); ++id) {
        const std::size_t at = id * kSectorSize;
        std::copy(section[id].begin(), section[id].end(), b.begin() + static_cast<std::ptrdiff_t>(at));
        setLe16(b, at + 0xFF4, static_cast<unsigned>(id));
        setLe16(b, at + 0xFF6, sectorChecksum(b.data() + at, kChunkLengths[id]));
        setLe32(b, at + 0xFF8, 0x08012025u);
        setLe32(b, at + 0xFFC, 1);
    }
    return b;
}

std::vector<uint8_t> save(Family family, const std::vector<uint8_t>& boxed) {
    return save(family, boxed, trainerIdOf(boxed));
}

// Save path: G3::parse with the release the file was identified as, then
// FRLGReadOnlyTrainer::create (UI.cpp opens every GBA save this way), then box 1 slot 1
// through Legality::analyze with metadata().sourceGameId.
Actual runSave(const std::vector<uint8_t>& bytes, G3::SourceGame game) {
    Actual a;
    const auto parsed = G3::parse(bytes, game);
    if (!parsed) {
        a.parserError = parsed.detail.empty() ? std::string(G3::errorMessage(parsed.error)) : parsed.detail;
        return a;
    }
    std::string error;
    const auto trainer = PokeVault::Legacy::FRLGReadOnlyTrainer::create(*parsed.save, error);
    if (!trainer) {
        a.parserError = error;
        return a;
    }
    const std::string id(parsed.save->metadata().sourceGameId);
    if (id != G3::sourceGameId(game) || trainer->sourceGameId() != id)
        a.problems.push_back("sourceGameId is " + id);
    if (trainer->boxes.empty() || !trainer->boxes[0][0]) {
        a.problems.push_back("box 1 slot 1 is empty");
        return a;
    }
    const Pokemon::Pokemon& pk = *trainer->boxes[0][0];
    a.report = Legality::analyze(pk, pk.getGameGroup(), id);
    a.outcome = fromReport(*a.report);
    if (!Gen5TestHelpers::sameReport(*a.report, Legality::analyze(pk, pk.getGameGroup(), id)))
        a.problems.push_back("a second analysis gave a different report");
    const auto src = parsed.save->sourceBytes();
    if (!std::equal(src.begin(), src.end(), bytes.begin(), bytes.end()))
        a.problems.push_back("save bytes changed");
    return a;
}

void cases() {
    using O = Outcome;
    using F = Family;
    using SG = G3::SourceGame;
    const auto noEncounter = invalid("Unable to match an encounter from the origin game");
    const auto enigma = warning("Enigma Berry: legality depends on the e-Reader berry stored in the original save");
    const auto eReaderIvs = warning("All IVs are 0");

    // Positive: each title's own record read from that title's save bytes.
    record("Ruby", "positive", "Zangoose L16, Route 114, Ruby-only slot, from Ruby save bytes",
           expect(O::NoProblemsFound), runSave(save(F::RubySapphire, fromHex(kRubyZangoose)), SG::RubyGBA));
    record("Sapphire", "positive", "Seviper L16, Route 114, Sapphire-only slot, from Sapphire save bytes",
           expect(O::NoProblemsFound), runSave(save(F::RubySapphire, fromHex(kSapphireSeviper)), SG::SapphireGBA));
    record("Emerald", "positive", "Smeargle L40, Artisan Cave, Emerald-only Method H slot, Emerald save",
           expect(O::NoProblemsFound), runSave(save(F::Emerald, fromHex(kEmeraldSmeargle)), SG::EmeraldGBA));
    record("FireRed", "positive", "Scyther L25, Celadon Game Corner prize, from FireRed save bytes",
           expect(O::NoProblemsFound, {info("the Gen III analyzer matched static encounter Scyther at met location 94, level 25")}),
           runSave(save(F::FireRedLeafGreen, fromHex(kFireRedScyther)), SG::FireRedGBA));
    record("LeafGreen", "positive", "Bellsprout L13, Route 5, LeafGreen-only slot, LeafGreen save",
           expect(O::NoProblemsFound),
           runSave(save(F::FireRedLeafGreen, fromHex(kLeafGreenBellsprout)), SG::LeafGreenGBA));

    // Negative: one broken rule each, named in the report.
    record("Ruby", "negative", "Zangoose with 255/255/10 EVs (total 520, cap 510)",
           expect(O::Invalid, {invalid("EV total 520 is above 510")}),
           run(edited(kRubyZangoose, [](std::vector<uint8_t>& pk) {
                   pk[kEvs] = 255;
                   pk[kEvs + 1] = 255;
                   pk[kEvs + 2] = 10;
               }),
               "ruby_gba"));
    record("Sapphire", "negative", "Seviper knowing Wrap twice", expect(O::Invalid, {invalid("Move 1 (Wrap) is a duplicate")}),
           run(edited(kSapphireSeviper, [](std::vector<uint8_t>& pk) {
                   pk[kMoves + 2] = pk[kMoves];
                   pk[kMoves + 3] = pk[kMoves + 1];
               }),
               "sapphire_gba"));
    record("Emerald", "negative", "Smeargle with Pokerus strain 0 and 5 days left",
           expect(O::Invalid, {invalid("Pokerus days remaining exceed the strain duration")}),
           run(edited(kEmeraldSmeargle, [](std::vector<uint8_t>& pk) { pk[kPokerus] = 0x05; }), "emerald_gba"));
    record("FireRed", "negative", "prize Scyther with the fateful encounter flag set",
           expect(O::Invalid, {invalid("Fateful encounter flag is not possible for this encounter")}),
           run(edited(kFireRedScyther, [](std::vector<uint8_t>& pk) { pk[kFateful] |= 0x80; }), "firered_gba"));
    record("LeafGreen", "negative", "Bellsprout with EXP 0x00FFFFFF (above the level-100 total)",
           expect(O::Invalid, {invalid("EXP is above the level 100 maximum")}),
           run(edited(kLeafGreenBellsprout, [](std::vector<uint8_t>& pk) { setLe32(pk, kExp, 0x00FFFFFFu); }),
               "leafgreen_gba"));

    // Malformed: never "No problems found".
    {
        // A Bad Egg in box 1: the stored checksum no longer matches the data.
        auto bytes = save(F::RubySapphire, fromHex(kRubyZangoose));
        bytes[5 * kSectorSize + 4 + kChecksum] ^= 0x01;
        setLe16(bytes, 5 * kSectorSize + 0xFF6, sectorChecksum(bytes.data() + 5 * kSectorSize, kChunkLengths[5]));
        record("Ruby", "malformed", "Ruby save whose box record fails its checksum (a Bad Egg)",
               rejected("strict Gen III adapter could not enumerate storage boxes"), runSave(bytes, SG::RubyGBA));
    }
    {
        auto pk = fromHex(kSapphireSeviper);
        pk.pop_back();
        Expect e = expect(O::Incomplete, {warning("Record is not a Gen III PK3 (79 bytes)")});
        e.structureNone = true;
        record("Sapphire", "malformed", "Seviper record cut to 79 bytes (analyzePk3)", e, runPk3(pk, "sapphire_gba"));
    }
    record("Emerald", "malformed", "Smeargle with origin game id 0",
           expect(O::Incomplete, {warning("Origin game id 0 is not a Gen III cartridge")}),
           run(withVersion(kEmeraldSmeargle, 0), "emerald_gba"));
    record("FireRed", "malformed", "prize Scyther whose stored checksum does not match its data",
           expect(O::Incomplete, {warning("PK3 checksum does not match its data")}),
           run([] {
               auto pk = fromHex(kFireRedScyther);
               pk[kChecksum] ^= 0x01;
               return pk;
           }(), "firered_gba"));
    record("LeafGreen", "malformed", "Bellsprout with internal species index 260 (an unused slot)",
           expect(O::Invalid, {invalid("Species index 260 is not a Gen III Pokemon")}),
           run(edited(kLeafGreenBellsprout, [](std::vector<uint8_t>& pk) { setLe16(pk, kSpecies, 260); }),
               "leafgreen_gba"));

    // Unknown: the history depends on data outside the record.
    record("Ruby", "unknown", "Zangoose holding an Enigma Berry (the berry lives in the save)", expect(O::Incomplete, {enigma}),
           run(edited(kRubyZangoose, [](std::vector<uint8_t>& pk) { setLe16(pk, kItem, kEnigmaBerry); }), "ruby_gba"));
    // The Japanese e-Reader shadows (Colosseum Card e Room) have fixed zero IVs; PKHeX calls them
    // VALID with Fishy IVAllEqual_0, and the engine keeps them Incomplete (review item 15-A).
    record("Sapphire", "unknown", "e-Reader shadow Togepi (fixed zero IVs) held in a Sapphire save",
           expect(O::Incomplete, {eReaderIvs}), run(fromHex(kEReaderTogepi), "sapphire_gba"));
    record("Emerald", "unknown", "e-Reader shadow Togepi in box 1 of an Emerald save", expect(O::Incomplete, {eReaderIvs}),
           runSave(save(F::Emerald, fromHex(kEReaderTogepi), kTesterId), SG::EmeraldGBA));
    record("FireRed", "unknown", "prize Scyther holding an Enigma Berry", expect(O::Incomplete, {enigma}),
           run(edited(kFireRedScyther, [](std::vector<uint8_t>& pk) { setLe16(pk, kItem, kEnigmaBerry); }),
               "firered_gba"));
    record("LeafGreen", "unknown", "e-Reader shadow Mareep (fixed zero IVs) held in a LeafGreen save",
           expect(O::Incomplete, {eReaderIvs}), run(fromHex(kEReaderMareep), "leafgreen_gba"));

    // Original game: the expected verdict follows from the pret disassemblies.
    // pokeruby src/data/wild_encounters.json:6292-6349: Route114_Ruby has Zangoose at L15-17 only.
    record("Ruby", "original-game", "Zangoose met at L14 on Route 114 (Ruby: L15-17)", expect(O::Invalid, {noEncounter}),
           run(withMetLevel(kRubyZangoose, 14), "ruby_gba"));
    // pokeemerald src/tv.c:198-203: an Emerald outbreak of Seedot L3 on Route 102 with Bide,
    // Harden, Leech Seed. pokeruby src/tv.c:1070-1078 copies a record-mixed outbreak show into
    // the save and src/wild_encounter.c:358-360 creates it with that level and those moves.
    record("Sapphire", "original-game", "Seedot L3, Route 102, Bide/Harden/Leech Seed (mixed outbreak)",
           expect(O::NoProblemsFound), run(fromHex(kSapphireOutbreakSeedot), "sapphire_gba"));
    // pokeemerald data/maps/NavelRock_Bottom/scripts.inc:54 `seteventmon SPECIES_LUGIA, 70`;
    // asm/macros/event.inc:1990-1995 and src/pokemon.c:2633-2639, 2773-2780: the event mon is
    // created with MON_DATA_MODERN_FATEFUL_ENCOUNTER set.
    record("Emerald", "original-game", "Navel Rock Lugia L70 with the fateful flag", expect(O::NoProblemsFound),
           run(fromHex(kEmeraldLugia), "emerald_gba"));
    record("Emerald", "original-game", "Navel Rock Lugia L70 without the fateful flag",
           expect(O::Invalid, {invalid("Event encounter is missing its fateful encounter flag")}),
           run(edited(kEmeraldLugia, [](std::vector<uint8_t>& pk) { pk[kFateful] &= 0x7F; }), "emerald_gba"));
    // pokefirered data/maps/CeladonCity_GameCorner_PrizeRoom/scripts.inc:155-156
    // `givemon VAR_TEMP_1, 25`: the prize Scyther is L25.
    record("FireRed", "original-game", "prize Scyther with met level 24 (the prize is L25)",
           expect(O::Invalid, {noEncounter}), run(withMetLevel(kFireRedScyther, 24), "firered_gba"));
    // scripts.inc:67-78 (LeafGreen: Pinsir) and 171-172 `givemon VAR_TEMP_1, 18`.
    record("LeafGreen", "original-game", "Pinsir L18 from the Celadon Game Corner prize room",
           expect(O::NoProblemsFound), run(fromHex(kLeafGreenPinsir), "leafgreen_gba"));
    record("LeafGreen", "original-game", "prize Pinsir with met level 17 (the prize is L18)",
           expect(O::Invalid, {noEncounter}), run(withMetLevel(kLeafGreenPinsir, 17), "leafgreen_gba"));

    // Title-specific: the origin version in the record decides which game's encounters apply.
    record("Ruby", "title-specific", "the Ruby Zangoose with version Sapphire (no Sapphire Zangoose)",
           expect(O::Invalid, {noEncounter}), run(withVersion(kRubyZangoose, kSapphire), "sapphire_gba"));
    // pokeruby include/constants/species.h:1282-1286: the roamer is Latias in Sapphire, Latios otherwise.
    record("Ruby", "title-specific", "the Sapphire roaming Latias with version Ruby (Ruby roams Latios)",
           expect(O::Invalid, {noEncounter}), run(withVersion(kSapphireRoamerLatias, kRuby), "ruby_gba"));
    record("Ruby", "title-specific", "the Sapphire Seviper traded into a Ruby save", expect(O::NoProblemsFound),
           runSave(save(F::RubySapphire, fromHex(kSapphireSeviper), kTesterId), SG::RubyGBA));
    record("Sapphire", "title-specific", "the Sapphire Seviper with version Ruby (no Ruby Seviper)",
           expect(O::Invalid, {noEncounter}), run(withVersion(kSapphireSeviper, kRuby), "ruby_gba"));
    record("Sapphire", "title-specific", "the Ruby Zangoose traded into a Sapphire save",
           expect(O::NoProblemsFound),
           runSave(save(F::RubySapphire, fromHex(kRubyZangoose), kTesterId), SG::SapphireGBA));
    // pokeruby has no Artisan Cave map (no ARTISAN anywhere in the tree).
    record("Emerald", "title-specific", "the Emerald Smeargle with version Ruby (no Artisan Cave)",
           expect(O::Invalid, {noEncounter}), run(withVersion(kEmeraldSmeargle, kRuby), "ruby_gba"));
    record("Emerald", "title-specific", "the FireRed prize Scyther traded into an Emerald save",
           expect(O::NoProblemsFound), runSave(save(F::Emerald, fromHex(kFireRedScyther), kTesterId), SG::EmeraldGBA));
    record("Emerald", "title-specific", "Ruby/Sapphire save bytes opened as Emerald",
           rejected("validated Gen III save family does not match the selected source release"),
           runSave(save(F::RubySapphire, fromHex(kEmeraldSmeargle)), SG::EmeraldGBA));
    // scripts.inc:80-90: the FireRed counter that gives Scyther gives Dratini in LeafGreen.
    record("FireRed", "title-specific", "the FireRed prize Scyther with version LeafGreen",
           expect(O::Invalid, {noEncounter}), run(withVersion(kFireRedScyther, kLeafGreen), "leafgreen_gba"));
    record("FireRed", "title-specific", "the LeafGreen Bellsprout traded into a FireRed save",
           expect(O::NoProblemsFound),
           runSave(save(F::FireRedLeafGreen, fromHex(kLeafGreenBellsprout), kTesterId), SG::FireRedGBA));
    // scripts.inc:67-78: FireRed gives Dratini where LeafGreen gives Pinsir.
    record("LeafGreen", "title-specific", "the LeafGreen prize Pinsir with version FireRed",
           expect(O::Invalid, {noEncounter}), run(withVersion(kLeafGreenPinsir, kFireRed), "firered_gba"));
    // pokefirered src/data/wild_encounters.json:8620 (sRoute5_FireRed: no Bellsprout). The
    // nearest FireRed template is an event gift, which the record fails (PKHeX agrees).
    record("LeafGreen", "title-specific", "the LeafGreen Bellsprout with version FireRed",
           expect(O::Invalid, {invalid("Event gift is missing its fateful encounter flag")}),
           run(withVersion(kLeafGreenBellsprout, kFireRed), "firered_gba"));
    record("LeafGreen", "title-specific", "the Emerald Smeargle traded into a LeafGreen save",
           expect(O::NoProblemsFound),
           runSave(save(F::FireRedLeafGreen, fromHex(kEmeraldSmeargle), kTesterId), SG::LeafGreenGBA));

    // Title-specific: version 15 (Colosseum/XD) routes the record to the Gen3Cxd judge. A real
    // Shadow Pokemon certifies in any GBA save; a GBA record relabelled with version 15 matches no
    // Orre encounter and is Invalid (PKHeX: EncInvalid).
    record("Sapphire", "title-specific", "Colosseum Shadow Bayleef (team lock verified) traded into a Sapphire save",
           expect(O::NoProblemsFound),
           runSave(save(F::RubySapphire, fromHex(kColoShadowBayleef), kTesterId), SG::SapphireGBA));
    record("Sapphire", "title-specific", "the Sapphire Seviper with version Colosseum/XD (no Orre Seviper encounter)",
           expect(O::Invalid, {noEncounter}), run(withVersion(kSapphireSeviper, kColosseumXD), "sapphire_gba"));
    record("Emerald", "title-specific", "XD Shadow Zapdos (team lock verified) traded into an Emerald save",
           expect(O::NoProblemsFound), runSave(save(F::Emerald, fromHex(kXdShadowZapdos), kTesterId), SG::EmeraldGBA));
    record("Emerald", "title-specific", "the Emerald Smeargle with version Colosseum/XD, in box 1 of an Emerald save",
           expect(O::Invalid, {noEncounter}),
           runSave(save(F::Emerald, withVersion(kEmeraldSmeargle, kColosseumXD)), SG::EmeraldGBA));
    record("LeafGreen", "title-specific", "Colosseum Shadow Bayleef traded into a LeafGreen save", expect(O::NoProblemsFound),
           runSave(save(F::FireRedLeafGreen, fromHex(kColoShadowBayleef), kTesterId), SG::LeafGreenGBA));
    record("LeafGreen", "title-specific", "the LeafGreen Bellsprout with version Colosseum/XD (no Orre Bellsprout encounter)",
           expect(O::Invalid, {noEncounter}), run(withVersion(kLeafGreenBellsprout, kColosseumXD), "leafgreen_gba"));

    // Title-specific: the same bytes from the Bank carry no source game and are never certified.
    const std::pair<const char*, const char*> bank[] = {{"Ruby", kRubyZangoose},
                                                        {"Sapphire", kSapphireSeviper},
                                                        {"Emerald", kEmeraldSmeargle},
                                                        {"FireRed", kFireRedScyther},
                                                        {"LeafGreen", kLeafGreenBellsprout}};
    for (const auto& [title, hex] : bank)
        record(title, "title-specific", std::string("the ") + title + " positive record from the Bank (no source game)",
               expect(O::Incomplete, {}, {warning(""), invalid("")}), run(fromHex(hex), ""));
}

}  // namespace gen3

// ---------------------------------------------------------------- Gen IV

namespace gen4 {

namespace G4 = PokeVault::Integration::Gen4;
using Gen5TestHelpers::fromHex;

// Decrypted 136-byte PK4 box records. Most come from the PKHeX-generated corpus of
// tools/pkhex_gen34_diff (`run.sh PKHEX 1 3 4`, PKHeX kwsch/PKHeX@6501f0ab), built by PKHeX for
// OT "Tester". PKHeX calls each one VALID with the personal table of the save that holds it.
// The corpus id follows each name (bNNNNN: base corpus, pNN: grown corpus).
// Diamond: Murkrow L11, Eterna Forest (met location 48), a Diamond-only night slot (b29516).
constexpr const char* kDiamondMurkrow =
    "FBEA6C7D0000D985C600000065BBB789E6020000236900020000000000000000000000000000000040003601E4007200230F141E000000008548B10E00000000000000000000000037013F013C0135013C0139014101FFFF000000000000000A00000000000000003E0149015701580149015601FFFF00000000001A0A100000300000040B020000";
// Diamond: Dialga L47, Spear Pillar (met location 51) (b50779).
constexpr const char* kDiamondDialga =
    "652F81620000C944E3010000E194F090F2FA0100002E000200000000000000000000000000000000E800F6005101CB0123050F0500000000F68B9F0B0000000004000000000000002E0133012B01360131012B01FFFF0000000000000000000A00000000000000003E0149015701580149015601FFFF00000000001A0A100000330000042F040000";
// Diamond: an event Arceus L100 from a PCD (OT Michina, fateful) (b50987).
constexpr const char* kDiamondEventArceus =
    "9B12190500001DFDED01D400332BFE74D01213000079000200000000000000000000000000000004C101CB01CC01D3010A0505050000000035503E060000000005000000000000002B013C012D012F013F013D01FFFFFFFFFFFFFFFFFFFF000A000000000000000037014D0147014C014D0152014501FFFF0000001A0A100000F40B001064000000";
// Pearl: Misdreavus L11, Eterna Forest, a Pearl-only night slot (b29636).
constexpr const char* kPearlMisdreavus =
    "EAEAE6D000003A15C8000000BF09CC7F28040000231A0002000000000000000000000000000000002D009500B4003601280F0A0F00000000C6E9B910000000000000000000000000370133013D012E013C012F012B0140013F013D01FFFF000B00000000000000003E0149015701580149015601FFFF00000000001A0A100000300000040B020000";
// Pearl: Dustox L12, Eterna Forest (b35675).
constexpr const char* kPearlDustox =
    "BF8836540000DA340D010000C2541225C006000046130002000000000000000000000000000000005D000000000000001900000000000000FE26032C0000000000000000000000002E013F013D013E0139014201FFFF0000000000000000000B00000000000000003E0149015701580149015601FFFF00000000001A0A100000300000040C020000";
// Pearl: Palkia L47, Spear Pillar (b50795).
constexpr const char* kPearlPalkia =
    "FD77FD000000FC6EE4010000BC9037A2F2FA0100002E0002000000000000000000000000000000006001F6005101CC0114050F0500000000FE872D2B0000000004000000000000003A012B013601350133012B01FFFF0000000000000000000B00000000000000003E0149015701580149015601FFFF00000000001A0A100000330000042F040000";
// Platinum: Giratina-Origin L47 holding the Griseous Orb, Distortion World (met location
// 117; the D/P met-location field holds 3002) (b50859).
constexpr const char* kPlatinumGiratinaOrigin =
    "56F5966D0000F27FE7017000917554A0F2FA0100001A000200000000000000000000000000000000D201F6005101D30105050F05000000005BB08221000000000C00000000007500310133013C012B013E01330138012B01FFFF00000000000C00000000000000003E0149015701580149015601FFFF00000000001A0A100000BA0B00042F170000";
// Platinum: Giratina L47, Turnback Cave (met location 62) (b50855).
constexpr const char* kPlatinumGiratina =
    "AEF3624100007582E7010000917554A0F2FA0100002E000200000000000000000000000000000000D201F6005101D30105050F05000000007CB74D27000000000400000000000000310133013C012B013E01330138012B01FFFF00000000000C00000000000000003E0149015701580149015601FFFF00000000001A0A1000003E0000042F050000";
// Platinum: Rotom L20, Old Chateau (met location 70) (b50731).
constexpr const char* kPlatinumRotom =
    "1CC8E85600002026DF010000F3709434401F0000461A00020000000000000000000000000000000054006D00FD0068001E0A0A0F00000000F69C92390000000004000000000000003C0139013E0139013701FFFF00000000000000000000000C00000000000000003E0149015701580149015601FFFF00000000001A0A1000004600000414090000";
// Platinum: a FireRed Bulbasaur brought in through Pal Park (met location 55, origin game 4) (p24).
constexpr const char* kFireRedBulbasaurPalPark =
    "234D1D850000C10601000000C12727F387000000464100020000000000000000000000000000000021002D00000000002328000000000000A127E6110000000000000000000000002C013F0136012C012B013D012B013F013C01FFFFE0FF000400000000000000003E0149015701580149015601FFFF3F010000001A0A1000003700000405000000";
// HeartGold: Spinarak L2, Route 30 (met location 178), a HeartGold-only night slot (b26992).
constexpr const char* kHeartGoldSpinarak =
    "2B01F5AF00004A4BA7000000885BF9F106000000460F000200000000000000000000000000000000280051000000000023280000000000006E3AD23300000000020000000000B2003D013A01330138012B013C012B013501FFFF00000000000700000000000000003E0149015701580149015601FFFF00000000001A0A100000BA0B000402020400";
// HeartGold: Ho-Oh L45, Bell Tower (met location 205) (b34427).
constexpr const char* kHeartGoldHoOh =
    "41E7E3390000533CFA0000004AD20636F2BC0100002E0002000000000000000000000000000000004601F1007E00DD001E05050500000000ECC1481D00000000040000000000CD0032013901BE0139013201FFFF00000000000000000000000700000000000000003E0149015701580149015601FFFF00000000001A0A100000BA0B00042D090400";
// HeartGold: Pikachu L15 from the Pokewalker Yellow Forest course (met location 233) (b2548).
constexpr const char* kHeartGoldWalkerPikachu =
    "FFFFFFA00000376719000000677690282F0D00004609000200000000000000000000000000000000130057002D0027000F0A281E000000000000BF3400000000000000000000E9003A01330135012B012D0132013F01FFFF000000000000000700000000000000003E0149015701580149015601FFFF00000000001A0A100000BA0B00040F000400";
// SoulSilver: Ledyba L3, Route 30, a SoulSilver-only morning slot (b26800).
constexpr const char* kSoulSilverLedyba =
    "96E6AB5400006746A50000003023CE53150000004644000200000000000000000000000000000000210000000000000023000000000000007716311E00000000000000000000B20036012F012E0143012C012B01FFFF0000000000000000000800000000000000003E0149015701580149015601FFFF00000000001A0A100000BA0B000403020400";
// SoulSilver: Lugia L45, Whirl Islands (met location 218) (b34423).
constexpr const char* kSoulSilverLugia =
    "18954FBA00006C3FF9000000B8788C34F2BC0100002E0002000000000000000000000000000000004601F0003800B1001E050505000000005500FB3E00000000040000000000DA0036013F01310133012B01FFFF00000000000000000000000800000000000000003E0149015701580149015601FFFF00000000001A0A100000BA0B00042D070400";
// SoulSilver: Caterpie L17 in a Sport Ball (HG/SS ball byte 24), Bug-Catching Contest,
// National Park (met location 207) (b192).
constexpr const char* kSoulSilverSportCaterpie =
    "2C2635E300002AB00A00000073960574311300004613000200000000000000000000000000000000210051000000000023280000000000006BFC030200000000020000000000CF002D012B013E012F013C013A0133012F01FFFF00000000000800000000000000003E0149015701580149015601FFFF00000000001A0A100000BA0B000411021800";
// SoulSilver: Pikachu L15 from the Pokewalker (b2596).
constexpr const char* kSoulSilverWalkerPikachu =
    "FEFFFF6600005F6519000000188A07122F0D00004609000200000000000000000000000000000000130057002D0027000F0A281E000000000000BF3400000000000000000000E9003A01330135012B012D0132013F01FFFF000000000000000800000000000000003E0149015701580149015601FFFF00000000001A0A100000BA0B00040F000400";
// Colosseum/XD Pokemon brought in through Pal Park (origin game 15). The PKHeX oracle built
// each from its Colosseum/XD encounter list (EncounterMovesetGenerator, GameVersion.CXD, OT
// Tester) as a CK3/XK3, then converted it CK3/XK3 -> PK3 -> PK4 the way Pal Park does. PKHeX
// calls each one VALID (with a Fishy note on the OT).
// Colosseum: the starter Umbreon L26.
constexpr const char* kColosseumUmbreon =
    "E56A52B30000BDE5C5000000393031D4A8440000461C0002000000000000000000000000000000002C000D01220121011914140A00000000F44B241F0000000000000000000000003F0137012C013C012F0139013801FFFF48A10C02E0FF000F00000000000000003E0149015701580149015601FFFFFFFF0000001A0A100000370000041A000000";
// XD: the starter Eevee L10.
constexpr const char* kXDEevee =
    "E56A52B3000050C485000000393031D4E80300004632000200000000000000000000000000000000210027002C001C00231E190F00000000F44B241F0000000001000000000000002F012F0140012F012F01FFFF0000000048A10C02E0FF000F00000000000000003E0149015701580149015601FFFF00000000001A0A100000370000040A000000";
// Colosseum: a snagged Shadow Quilava L30.
constexpr const char* kColosseumShadowQuilava =
    "B713BB39000086639C000000393031D4005500004642000200000000000000000000000000000000F1006C005B00AC0005140A1900000000B05F39250000002000000000000000003B013F01330136012B0140012B01FFFF48A10C02E0FF000F00000000000000003E0149015701580149015601FFFFFFFF0000001A0A100000370000041E000000";
// XD: a snagged Shadow Ledyba L10.
constexpr const char* kXDShadowLedyba =
    "131320670000F55BA5000000393031D42003000046440002000000000000000000000000000000003C001F014C01300014141414000000009CCB630700000020030000000000000036012F012E0143012C012B01FFFF000048A10C02E0FF000F00000000000000003E0149015701580149015601FFFF00000000001A0A100000370000040A000000";

// PK4 layout (PKHeX PK4.cs; pokeplatinum include/struct_defs/pokemon.h).
constexpr std::size_t kSanity = 0x04, kChecksum = 0x06, kSpecies = 0x08, kTid = 0x0C, kSid = 0x0E, kExp = 0x10;
constexpr std::size_t kLanguage = 0x17, kMoves = 0x28, kFlags = 0x40, kShinyLeaf = 0x41, kVersion = 0x5F;
constexpr std::size_t kOtName = 0x68, kMetLocationDP = 0x80, kBallDPPt = 0x83, kMetLevel = 0x84, kBallHGSS = 0x86;
constexpr uint8_t kHeartGold = 7, kSoulSilver = 8, kDiamond = 10, kPearl = 11, kPlatinum = 12;
constexpr std::size_t kStoredSize = 0x88;

uint16_t le16(const std::vector<uint8_t>& b, std::size_t o) {
    return static_cast<uint16_t>(b[o] | (b[o + 1] << 8));
}

void setLe16(std::vector<uint8_t>& b, std::size_t o, unsigned v) {
    b[o] = static_cast<uint8_t>(v);
    b[o + 1] = static_cast<uint8_t>(v >> 8);
}

void setLe32(std::vector<uint8_t>& b, std::size_t o, uint32_t v) {
    setLe16(b, o, v & 0xFFFF);
    setLe16(b, o + 2, v >> 16);
}

std::span<const std::byte> asBytes(const std::vector<uint8_t>& v) {
    return {reinterpret_cast<const std::byte*>(v.data()), v.size()};
}

std::vector<uint8_t> toVector(std::span<const std::byte> s) {
    std::vector<uint8_t> out(s.size());
    std::transform(s.begin(), s.end(), out.begin(), [](std::byte b) { return static_cast<uint8_t>(b); });
    return out;
}

// The games' checksum: the 16-bit sum of the data words 0x08-0x87.
void refreshChecksum(std::vector<uint8_t>& pk) {
    unsigned sum = 0;
    for (std::size_t o = 0x08; o < kStoredSize; o += 2)
        sum += le16(pk, o);
    setLe16(pk, kChecksum, sum & 0xFFFF);
}

std::vector<uint8_t> edited(const char* hex, const std::function<void(std::vector<uint8_t>&)>& edit) {
    auto pk = fromHex(hex);
    edit(pk);
    refreshChecksum(pk);
    return pk;
}

std::vector<uint8_t> withVersion(const char* hex, uint8_t version) {
    return edited(hex, [version](std::vector<uint8_t>& pk) { pk[kVersion] = version; });
}

// The record as a save stores it (Encryption4: blocks shuffled by PID, data encrypted with the
// checksum as the seed). encryptArray4 writes the checksum of the data it encrypts, so a record
// with a bad checksum has to be damaged after this step.
std::vector<uint8_t> stored(const std::vector<uint8_t>& pk) {
    return toVector(Encryption::encryptArray4(asBytes(pk)));
}

Enums::GameVersion group(const std::string& game) {
    if (game == "diamond_nds" || game == "pearl_nds")
        return Enums::GameVersion::DP;
    if (game == "platinum_nds")
        return Enums::GameVersion::PT;
    return Enums::GameVersion::HGSS;
}

// Legality::analyzePk4 on the bytes the production entity decrypted must agree: production
// never drops its Invalid and never certifies more.
void crossCheck(Actual& a, const Pokemon::Pokemon4ReadOnly& source, const std::string& game) {
    const auto pk4 = Legality::analyzePk4(toVector(source.decryptedBytes()), game).verdict();
    if (pk4 == Legality::Verdict::Invalid && a.outcome != Outcome::Invalid)
        a.problems.push_back("Legality::analyze dropped an analyzePk4 Invalid");
    if (a.outcome == Outcome::NoProblemsFound && pk4 != Legality::Verdict::NoProblemsFound)
        a.problems.push_back("Legality::analyze certified what analyzePk4 did not");
}

// Record path: the stored bytes go through Pokemon4ReadOnly and Pokemon4ReadOnlyView, then
// Legality::analyze(view, view.getGameGroup(), game), as Gen4SharedPokemonSurface.inc
// drawLegality does. `corrupt` says the save reader must quarantine the record (it hides every
// record that fails Pokemon4ReadOnly::valid()); the analysis still runs on it, and
// analyzePk4 on the decrypted bytes must name the Bad Egg. (Legality::analyze itself reads
// such a record as species 0, an empty slot, and reports no issue: see Findings in
// docs/legality/TITLE_TEST_MATRIX.md.)
Actual runStored(const std::vector<uint8_t>& bytes, const std::string& game, bool corrupt = false) {
    Actual a;
    const auto copy = bytes;
    const Pokemon::Pokemon4ReadOnly source(asBytes(bytes), group(game));
    if (source.valid() == corrupt)
        a.problems.push_back(corrupt ? "Pokemon4ReadOnly accepts the corrupt record (the save reader would show it)"
                                     : "Pokemon4ReadOnly rejects the record (the save reader would hide it)");
    if (corrupt && !hasText(Legality::analyzePk4(toVector(source.decryptedBytes()), game), Severity::Warning,
                            "PK4 checksum or sanity word does not match its data (the games show it as a Bad Egg)"))
        a.problems.push_back("analyzePk4 does not report the Bad Egg Warning");
    const Pokemon::Pokemon4ReadOnlyView view(source);
    a.report = Legality::analyze(view, view.getGameGroup(), game);
    a.outcome = fromReport(*a.report);
    if (!Gen5TestHelpers::sameReport(*a.report, Legality::analyze(view, view.getGameGroup(), game)))
        a.problems.push_back("a second analysis gave a different report");
    crossCheck(a, source, game);
    if (bytes != copy)
        a.problems.push_back("record bytes changed");
    return a;
}

Actual run(const std::vector<uint8_t>& pk, const std::string& game, bool corrupt = false) {
    const auto bytes = stored(pk);
    Actual a = runStored(bytes, game, corrupt);
    const Pokemon::Pokemon4ReadOnly source(asBytes(bytes), group(game));
    if (toVector(source.decryptedBytes()) != pk)
        a.problems.push_back("Pokemon4ReadOnly decrypted other bytes than the record");
    return a;
}

// Engine entry point for bytes no save reader hands over (a record that is not 136 or 236
// bytes long).
Actual runPk4(const std::vector<uint8_t>& pk, const std::string& game) {
    Actual a;
    const auto copy = pk;
    a.report = Legality::analyzePk4(pk, game);
    a.outcome = fromReport(*a.report);
    if (pk != copy)
        a.problems.push_back("record bytes changed");
    return a;
}

// ------------------------------------------------ synthetic 512 KiB NDS saves

constexpr std::size_t kSaveSize = 0x80000;

// Block layout per family (PKHeX SAV4DP.cs, SAV4Pt.cs, SAV4HGSS.cs): General and Storage
// block offsets and sizes, footer size, and the Trainer, Party and box data offsets.
struct SaveSpec {
    std::size_t generalSize, storageStart, storageSize, footer, trainer, party, boxData, boxStride;
};

SaveSpec specOf(G4::Layout layout) {
    switch (layout) {
        case G4::Layout::DiamondPearl: return {0xC100, 0xC100, 0x121E0, 0x14, 0x64, 0x98, 4, 30 * kStoredSize};
        case G4::Layout::Platinum: return {0xCF2C, 0xCF2C, 0x121E4, 0x14, 0x68, 0xA0, 4, 30 * kStoredSize};
        case G4::Layout::HeartGoldSoulSilver: return {0xF628, 0xF700, 0x12310, 0x10, 0x64, 0x98, 0, 0x1000};
    }
    return {};
}

// CRC-16/CCITT (polynomial 0x1021, start 0xFFFF), PKHeX Checksums.CRC16_CCITT, written bit by
// bit here.
uint16_t crc16(const uint8_t* data, std::size_t length) {
    uint16_t crc = 0xFFFF;
    for (std::size_t i = 0; i < length; ++i) {
        crc = static_cast<uint16_t>(crc ^ (data[i] << 8));
        for (int bit = 0; bit < 8; ++bit)
            crc = static_cast<uint16_t>((crc & 0x8000) ? (crc << 1) ^ 0x1021 : crc << 1);
    }
    return crc;
}

// Block footer (PKHeX SAV4.cs, SAV4BlockDetection.cs): save counter, block size, magic
// 0x20060623, and the CRC of everything before the footer in the last two bytes.
void stampFooter(std::vector<uint8_t>& b, std::size_t start, std::size_t size, std::size_t footer) {
    const std::size_t end = start + size;
    setLe32(b, end - 0x10, 1);
    setLe32(b, end - 0x0C, static_cast<uint32_t>(size));
    setLe32(b, end - 0x08, 0x20060623u);
    setLe16(b, end - 0x02, crc16(b.data() + start, size - footer));
}

// Partition 0 holds a General and a Storage block; partition 1 is erased flash (0xFF), so it
// holds no valid copy. General: the trainer is the record's OT (name, TID, SID, gender,
// language) with the given ROM code at Trainer + 0x1C (7 HeartGold, 8 SoulSilver, 12 Platinum,
// unused by D/P), and an empty party. Storage: current box 0, every slot a blank record, and box
// 1 slot 1 the stored record (`damage` may change its stored bytes first).
std::vector<uint8_t> save(G4::Layout layout, const std::vector<uint8_t>& pk, uint8_t romCode,
                          const std::function<void(std::vector<uint8_t>&)>& damage = nullptr) {
    const SaveSpec s = specOf(layout);
    std::vector<uint8_t> b(kSaveSize, 0xFF);
    std::fill_n(b.begin(), s.generalSize, uint8_t{0});
    std::fill_n(b.begin() + static_cast<std::ptrdiff_t>(s.storageStart), s.storageSize, uint8_t{0});
    std::copy_n(pk.begin() + kOtName, 16, b.begin() + static_cast<std::ptrdiff_t>(s.trainer));
    setLe16(b, s.trainer + 0x10, le16(pk, kTid));
    setLe16(b, s.trainer + 0x12, le16(pk, kSid));
    b[s.trainer + 0x18] = pk[kMetLevel] >> 7;
    b[s.trainer + 0x19] = pk[kLanguage];
    b[s.trainer + 0x1C] = romCode;
    const auto blank = toVector(Encryption::encryptArray4(asBytes(std::vector<uint8_t>(kStoredSize, 0))));
    for (std::size_t box = 0; box < 18; ++box)
        for (std::size_t slot = 0; slot < 30; ++slot)
            std::copy(blank.begin(), blank.end(),
                      b.begin() + static_cast<std::ptrdiff_t>(s.storageStart + s.boxData + box * s.boxStride +
                                                              slot * kStoredSize));
    auto record = stored(pk);
    if (damage)
        damage(record);
    std::copy(record.begin(), record.end(), b.begin() + static_cast<std::ptrdiff_t>(s.storageStart + s.boxData));
    stampFooter(b, 0, s.generalSize, s.footer);
    stampFooter(b, s.storageStart, s.storageSize, s.footer);
    return b;
}

G4::Layout layoutOf(const std::string& game) {
    if (game == "diamond_nds" || game == "pearl_nds")
        return G4::Layout::DiamondPearl;
    if (game == "platinum_nds")
        return G4::Layout::Platinum;
    return G4::Layout::HeartGoldSoulSilver;
}

// Save path, as UI.cpp handleGen4View opens an assigned Gen IV source: openAssignedSource
// (Gen4AssignedSource.cpp) tries every layout with the assigned game id, needs exactly one to
// validate, and needs the assignment to match; then Gen4ReadOnlyTrainer::create. Box 1 slot 1
// is read as Gen4SharedPokemonSurface.inc boxRecord does (the staged editor's copy when there
// is one) and analyzed as drawLegality does.
Actual runSave(const std::vector<uint8_t>& bytes, const std::string& game) {
    Actual a;
    const auto copy = bytes;
    std::optional<G4::Gen4ReadOnlySave> save;
    int layouts = 0;
    for (G4::Layout layout : {G4::Layout::DiamondPearl, G4::Layout::Platinum, G4::Layout::HeartGoldSoulSilver}) {
        auto candidate = G4::Gen4ReadOnlySave::parse(bytes, layout, game);
        if (!candidate)
            continue;
        ++layouts;
        save = std::move(candidate);
    }
    if (layouts > 1) {
        a.parserError = "Multiple valid raw save layouts; source identity is ambiguous";
        return a;
    }
    if (!save) {
        a.parserError = "No validated Gen IV General/Storage layout";
        return a;
    }
    if (save->assignmentStatus() != G4::AssignmentStatus::Match || save->layout() != layoutOf(game)) {
        a.parserError = "Assigned game/family does not match the validated save; explicit reassignment required";
        return a;
    }
    std::string error;
    const auto trainer = PokeVault::Legacy::Gen4ReadOnlyTrainer::create(*save, game, error);
    if (!trainer) {
        a.parserError = error;
        return a;
    }
    if (trainer->sourceGameId() != game)
        a.problems.push_back("sourceGameId is " + trainer->sourceGameId());
    std::optional<Pokemon::Pokemon4ReadOnly> record;
    if (const auto* staged = trainer->stagedPokemon()) {
        record = staged->boxedPokemon(0, 0, &error);
    } else {
        const auto& source = trainer->sourceSave().box(0, 0);
        if (source.valid() && !source.empty())
            record = source;
        else
            error = source.empty() ? "Selected Generation IV slot is empty"
                                   : "Selected Generation IV PK4 failed strict validation";
    }
    const Pokemon::Pokemon* shown = trainer->boxes.empty() ? nullptr : trainer->boxes[0][0].get();
    if (!record) {
        a.parserError = error;
        if (shown)
            a.problems.push_back("the box shows a record the legality screen refuses");
    } else {
        if (!shown)
            a.problems.push_back("box 1 slot 1 is empty in the box view");
        const Pokemon::Pokemon4ReadOnlyView view(*record);
        a.report = Legality::analyze(view, view.getGameGroup(), game);
        a.outcome = fromReport(*a.report);
        if (shown && !Gen5TestHelpers::sameReport(*a.report, Legality::analyze(*shown, shown->getGameGroup(), game)))
            a.problems.push_back("the box view's Pokemon gives another report");
        crossCheck(a, *record, game);
    }
    const auto src = trainer->sourceSave().sourceBytes();
    if (!std::equal(src.begin(), src.end(), bytes.begin(), bytes.end()) || bytes != copy)
        a.problems.push_back("save bytes changed");
    return a;
}

void cases() {
    using O = Outcome;
    using L = G4::Layout;
    const auto noEncounter = invalid("No matching Gen IV encounter for this species, level, location, ball and PID/IV");
    const auto colosseumXD = warning("the Colosseum/XD encounter is not verified");
    const auto mismatch = "Assigned game/family does not match the validated save; explicit reassignment required";
    // PKHeX does not compare the two met-location fields (PKHEX_DISAGREEMENT_REVIEW.md item 14).
    const auto metLocationField = [](int dp, int expected, int met) {
        return invalid("D/P met location field holds " + std::to_string(dp) + " where Pt/HG/SS write " +
                       std::to_string(expected) + " for met location " + std::to_string(met));
    };

    // Positive: each title's own record read from that title's save bytes.
    // pokediamond arm9/src/map_header.c:20 picks d_enc_data or p_enc_data by GAME_VERSION, and
    // :287 gives Eterna Forest narc_0008. files/fielddata/encountdata/d_enc_data/narc_0008.bin
    // 0x74 and 0x78 (the night slots 2 and 3) hold 198 Murkrow; p_enc_data holds 200 Misdreavus
    // there. Slot 2 (0x14) is L11. pokeplatinum src/overlay006/wild_encounters.c:182-191 swaps
    // the night species into slots 2 and 3.
    record("Diamond", "positive", "Murkrow L11, Eterna Forest, a Diamond-only night slot, from D/P save bytes",
           expect(O::NoProblemsFound), runSave(save(L::DiamondPearl, fromHex(kDiamondMurkrow), 0), "diamond_nds"));
    record("Diamond", "positive", "Dialga L47, Spear Pillar", expect(O::NoProblemsFound),
           run(fromHex(kDiamondDialga), "diamond_nds"));
    record("Pearl", "positive", "Misdreavus L11, Eterna Forest, a Pearl-only night slot, from D/P save bytes",
           expect(O::NoProblemsFound), runSave(save(L::DiamondPearl, fromHex(kPearlMisdreavus), 0), "pearl_nds"));
    record("Pearl", "positive", "Palkia L47, Spear Pillar", expect(O::NoProblemsFound),
           run(fromHex(kPearlPalkia), "pearl_nds"));
    // pokeplatinum res/field/scripts/scripts_distortion_world_giratina_room.s:69
    // `StartGiratinaOriginBattle SPECIES_GIRATINA, 47`.
    record("Platinum", "positive", "Giratina-Origin L47, Distortion World, from Platinum save bytes",
           expect(O::NoProblemsFound),
           runSave(save(L::Platinum, fromHex(kPlatinumGiratinaOrigin), kPlatinum), "platinum_nds"));
    record("Platinum", "positive", "FireRed Bulbasaur brought in through Pal Park (met location 55)",
           expect(O::NoProblemsFound), run(fromHex(kFireRedBulbasaurPalPark), "platinum_nds"));
    // pokeheartgold files/fielddata/encountdata/gs_enc_data.json:512-532: Route 30 slot 0 is L2
    // in HeartGold and L3 in SoulSilver; at night HeartGold has Spinarak there, and in the
    // morning SoulSilver has Ledyba.
    record("HeartGold", "positive", "Spinarak L2, Route 30, a HeartGold-only night slot, from HG/SS save bytes",
           expect(O::NoProblemsFound),
           runSave(save(L::HeartGoldSoulSilver, fromHex(kHeartGoldSpinarak), kHeartGold), "heartgold_nds"));
    record("SoulSilver", "positive", "Ledyba L3, Route 30, a SoulSilver-only morning slot, from HG/SS save bytes",
           expect(O::NoProblemsFound),
           runSave(save(L::HeartGoldSoulSilver, fromHex(kSoulSilverLedyba), kSoulSilver), "soulsilver_nds"));

    // Negative: one broken rule each, named in the report.
    // pokeheartgold src/pokemon.c:1298-1307: only HG/SS write the second ball byte (0x86), and
    // they write the Apricorn balls (17-24) there with a Poke Ball at 0x83.
    record("Diamond", "negative", "Murkrow with a Level Ball in the HG/SS ball byte (0x86 = 18)",
           expect(O::Invalid, {invalid("Ball (#18) is not possible for this encounter"),
                               invalid("HG/SS ball value does not match the D/P/Pt ball value")}),
           run(edited(kDiamondMurkrow, [](std::vector<uint8_t>& pk) { pk[kBallHGSS] = 18; }), "diamond_nds"));
    record("Pearl", "negative", "Palkia with the fateful encounter flag set",
           expect(O::Invalid, {invalid("Fateful encounter flag is set on a non-event encounter")}),
           run(edited(kPearlPalkia, [](std::vector<uint8_t>& pk) { pk[kFlags] |= 0x01; }), "pearl_nds"));
    record("Platinum", "negative", "the Pal Park Bulbasaur with met location 16",
           expect(O::Invalid, {invalid("A Generation III Pokemon must have been met at Pal Park")}),
           run(edited(kFireRedBulbasaurPalPark, [](std::vector<uint8_t>& pk) { setLe16(pk, kMetLocationDP, 16); }),
               "platinum_nds"));
    record("HeartGold", "negative", "Spinarak with a Great Ball at 0x83 and a Poke Ball at 0x86",
           expect(O::Invalid, {invalid("HG/SS ball value does not match the D/P/Pt ball value")}),
           run(edited(kHeartGoldSpinarak, [](std::vector<uint8_t>& pk) { pk[kBallDPPt] = 3; }), "heartgold_nds"));
    record("SoulSilver", "negative", "Lugia with Shiny Leaf byte 32",
           expect(O::Invalid, {invalid("Shiny Leaf value is not obtainable")}),
           run(edited(kSoulSilverLugia, [](std::vector<uint8_t>& pk) { pk[kShinyLeaf] = 32; }), "soulsilver_nds"));

    // Malformed: never "No problems found".
    record("Diamond", "malformed", "D/P save whose box record fails its checksum (a Bad Egg)",
           rejected("selected Gen IV PK4 failed strict validation"),
           runSave(save(L::DiamondPearl, fromHex(kDiamondMurkrow), 0,
                        [](std::vector<uint8_t>& record) { record[kSpecies] ^= 0x01; }),
                   "diamond_nds"));
    {
        auto pk = fromHex(kDiamondMurkrow);
        pk.pop_back();
        Expect e = expect(O::Invalid, {invalid("PK4 record must be 136 or 236 bytes")});
        e.structureNone = true;
        record("Diamond", "malformed", "Murkrow record cut to 135 bytes (analyzePk4)", e, runPk4(pk, "diamond_nds"));
    }
    {
        auto bytes = stored(fromHex(kPearlMisdreavus));
        bytes[kSpecies] ^= 0x01;
        record("Pearl", "malformed", "Misdreavus with one stored data bit flipped (a Bad Egg)",
               expect(O::Incomplete), runStored(bytes, "pearl_nds", true));
    }
    record("Pearl", "malformed", "Misdreavus with species 494 (past the Gen IV National Dex)",
           expect(O::Invalid, {invalid("Species #494 does not exist in Generation IV")}),
           run(edited(kPearlMisdreavus, [](std::vector<uint8_t>& pk) { setLe16(pk, kSpecies, 494); }), "pearl_nds"));
    record("Platinum", "malformed", "Rotom with origin game 0",
           expect(O::Invalid, {invalid("Origin game #0 cannot produce a PK4 (not a Gen III or Gen IV game)")}),
           run(withVersion(kPlatinumRotom, 0), "platinum_nds"));
    {
        auto pk = fromHex(kPlatinumRotom);
        pk.push_back(0);
        Expect e = expect(O::Invalid, {invalid("PK4 record must be 136 or 236 bytes")});
        e.structureNone = true;
        record("Platinum", "malformed", "Rotom record padded to 237 bytes (analyzePk4)", e, runPk4(pk, "platinum_nds"));
    }
    {
        auto bytes = save(L::HeartGoldSoulSilver, fromHex(kHeartGoldSpinarak), kHeartGold);
        bytes[specOf(L::HeartGoldSoulSilver).storageStart + 0x200] ^= 0x01;
        record("HeartGold", "malformed", "HG/SS save whose Storage block fails its CRC",
               rejected("No validated Gen IV General/Storage layout"), runSave(bytes, "heartgold_nds"));
    }
    record("SoulSilver", "malformed", "Ledyba with sanity word 4 (a Bad Egg)", expect(O::Incomplete),
           run(edited(kSoulSilverLedyba, [](std::vector<uint8_t>& pk) { setLe16(pk, kSanity, 4); }), "soulsilver_nds",
               true));

    // Unknown: the history depends on data outside the record.
    record("Diamond", "unknown", "Colosseum Umbreon brought in through Pal Park (origin game 15)",
           expect(O::Incomplete, {colosseumXD}), run(fromHex(kColosseumUmbreon), "diamond_nds"));
    record("Pearl", "unknown", "XD Eevee brought in through Pal Park (origin game 15)", expect(O::Incomplete, {colosseumXD}),
           run(fromHex(kXDEevee), "pearl_nds"));
    // pokeplatinum res/field/scripts/scripts_route_210_grandma_wilma_house.s:81-82 checks the
    // party Pokemon's current type is Dragon (:103-106, :117 teach Draco Meteor). Arceus is a
    // Dragon type only while it holds the Draco Plate, which the record cannot show.
    record("Platinum", "unknown", "event Arceus that knows Draco Meteor (the Pt tutor needs a Dragon type)",
           expect(O::Incomplete, {warning("Arceus can learn it only from the Pt/HG/SS tutor while holding a Draco Plate")}),
           run(edited(kDiamondEventArceus, [](std::vector<uint8_t>& pk) { setLe16(pk, kMoves + 6, 434); }),
               "platinum_nds"));
    record("HeartGold", "unknown", "Colosseum Shadow Quilava brought in through Pal Park",
           expect(O::Incomplete, {colosseumXD}), run(fromHex(kColosseumShadowQuilava), "heartgold_nds"));
    record("SoulSilver", "unknown", "XD Shadow Ledyba brought in through Pal Park", expect(O::Incomplete, {colosseumXD}),
           run(fromHex(kXDShadowLedyba), "soulsilver_nds"));

    // Original game: the expected verdict follows from the pret disassemblies.
    // d_enc_data/narc_0008.bin: Murkrow is in night slots 2 (L11) and 3 (L10) only; the L12
    // slots 10 and 11 hold 267 Beautifly in Diamond and 269 Dustox in Pearl (p_enc_data).
    record("Diamond", "original-game", "Murkrow met at L12 in Eterna Forest (Diamond night slots: L10-11)",
           expect(O::Invalid, {noEncounter}), run(edited(kDiamondMurkrow, [](std::vector<uint8_t>& pk) {
                                                      pk[kMetLevel] = 12;
                                                      setLe32(pk, kExp, 1000);
                                                  }),
                                                  "diamond_nds"));
    record("Pearl", "original-game", "Dustox L12, Eterna Forest (Pearl slots 10-11)", expect(O::NoProblemsFound),
           run(fromHex(kPearlDustox), "pearl_nds"));
    // pokeplatinum res/field/scripts/scripts_old_chateau_back_middle_west_room.s:24
    // `StartWildBattle SPECIES_ROTOM, 20`.
    record("Platinum", "original-game", "Rotom met at L15 in the Old Chateau (the script gives L20)",
           expect(O::Invalid, {noEncounter}),
           run(edited(kPlatinumRotom, [](std::vector<uint8_t>& pk) { pk[kMetLevel] = (pk[kMetLevel] & 0x80) | 15; }),
               "platinum_nds"));
    // pokeplatinum src/pokemon.c:1733-1739: the met-location setter writes the location into the
    // Pt/HG/SS field (0x46) and 3002 (FATEFUL_ENCOUNTER_LOCATION, :59) into the D/P field (0x80)
    // unless the location is one D/P know (src/unk_02017038.c:38-45: 1-111, 2000-2010,
    // 3000-3076). Distortion World is 117 (res/text/location_names.json:473-474).
    record("Platinum", "original-game", "Giratina-Origin whose D/P met-location field says 117 (the game writes 3002)",
           expect(O::Invalid, {metLocationField(117, 3002, 117)}),
           run(edited(kPlatinumGiratinaOrigin, [](std::vector<uint8_t>& pk) { setLe16(pk, kMetLocationDP, 117); }),
               "platinum_nds"));
    // pokeheartgold files/fielddata/script/scr_seq/scr_seq_0021_D17R0110.s:57-72: Ho-Oh is L45
    // when the game version is HeartGold (7) and L70 otherwise; scr_seq_0104_D40R0107.s:69-84:
    // Lugia is L70 in HeartGold and L45 otherwise.
    record("HeartGold", "original-game", "Ho-Oh L45, Bell Tower (HeartGold gives L45, SoulSilver L70)",
           expect(O::NoProblemsFound), run(fromHex(kHeartGoldHoOh), "heartgold_nds"));
    record("HeartGold", "original-game", "Ho-Oh L45 with version SoulSilver, in a HeartGold save (SoulSilver gives L70)",
           expect(O::Invalid, {noEncounter}), run(withVersion(kHeartGoldHoOh, kSoulSilver), "heartgold_nds"));
    record("SoulSilver", "original-game", "Lugia L45, Whirl Islands (SoulSilver gives L45, HeartGold L70)",
           expect(O::NoProblemsFound), run(fromHex(kSoulSilverLugia), "soulsilver_nds"));
    record("SoulSilver", "original-game", "Lugia L45 with version HeartGold, in a SoulSilver save (HeartGold gives L70)",
           expect(O::Invalid, {noEncounter}), run(withVersion(kSoulSilverLugia, kHeartGold), "soulsilver_nds"));
    // pokeheartgold src/pokemon.c:1286-1292 writes METLOC_FARAWAY_PLACE (3002,
    // include/constants/map_sections.h:258) to the D/P field unless
    // LocationIsDiamondPearlCompatible (asm/unk_02017FAC.s:68-97: Twinleaf Town to Battle Park,
    // the Day Care couple to Riley, Lovely Place to Concert Event). Route 30 is 178 (:182).
    record("HeartGold", "original-game", "Spinarak whose D/P met-location field says Route 30 (the game writes 3002)",
           expect(O::Invalid, {metLocationField(178, 3002, 178)}),
           run(edited(kHeartGoldSpinarak, [](std::vector<uint8_t>& pk) { setLe16(pk, kMetLocationDP, 178); }),
               "heartgold_nds"));
    record("SoulSilver", "original-game", "Ledyba whose D/P met-location field says Route 30 (the game writes 3002)",
           expect(O::Invalid, {metLocationField(178, 3002, 178)}),
           run(edited(kSoulSilverLedyba, [](std::vector<uint8_t>& pk) { setLe16(pk, kMetLocationDP, 178); }),
               "soulsilver_nds"));

    // Title-specific: the origin version in the record (0x5F) decides which game's encounters
    // apply, and the save's personal table decides which forms exist.
    record("Diamond", "title-specific", "the Diamond Murkrow with version Pearl (Pearl has Misdreavus there)",
           expect(O::Invalid, {noEncounter}), run(withVersion(kDiamondMurkrow, kPearl), "diamond_nds"));
    record("Diamond", "title-specific", "the Pearl Misdreavus traded into a Diamond save", expect(O::NoProblemsFound),
           runSave(save(L::DiamondPearl, fromHex(kPearlMisdreavus), 0), "diamond_nds"));
    // pokeplatinum include/constants/forms.h:87 ROTOM_FORM_WASH 2. pokediamond
    // arm9/src/pokemon.c:3400-3413 ResolveMonForm: D/P have personal data for Deoxys and
    // Wormadam forms only. pokeplatinum src/overlay005/ov5_021F6454.c:451-509 reverts Rotom and
    // Giratina forms before the Union Room
    // (res/field/scripts/scripts_pokemon_center_2f_common.s:409).
    record("Diamond", "title-specific", "a Platinum Rotom-Wash (form 2) read in a Diamond save",
           expect(O::Invalid, {invalid("Form 2 is out of range")}),
           run(edited(kPlatinumRotom, [](std::vector<uint8_t>& pk) { pk[kFlags] |= 2 << 3; }), "diamond_nds"));
    record("Pearl", "title-specific", "the Pearl Misdreavus with version Diamond (Diamond has Murkrow there)",
           expect(O::Invalid, {noEncounter}), run(withVersion(kPearlMisdreavus, kDiamond), "pearl_nds"));
    record("Pearl", "title-specific", "the Platinum Giratina-Origin read in a Pearl save",
           expect(O::Invalid, {invalid("Form 1 is out of range")}), run(fromHex(kPlatinumGiratinaOrigin), "pearl_nds"));
    record("Pearl", "title-specific", "D/P save bytes assigned to Platinum", rejected(mismatch),
           runSave(save(L::DiamondPearl, fromHex(kPearlMisdreavus), 0), "platinum_nds"));
    record("Platinum", "title-specific", "Rotom-Wash (form 2) from Platinum save bytes", expect(O::NoProblemsFound),
           runSave(save(L::Platinum, edited(kPlatinumRotom, [](std::vector<uint8_t>& pk) { pk[kFlags] |= 2 << 3; }),
                        kPlatinum),
                   "platinum_nds"));
    // pokeplatinum res/field/scripts/scripts_turnback_cave_giratina_room.s:35
    // `StartLegendaryBattle SPECIES_GIRATINA, 47`; D/P give Giratina at L70 there (PKHeX
    // Encounters4DPPt.cs:57 and :90).
    record("Platinum", "title-specific", "the Turnback Cave Giratina L47 with version Diamond (D/P give L70)",
           expect(O::Invalid, {noEncounter}), run(withVersion(kPlatinumGiratina, kDiamond), "platinum_nds"));
    record("Platinum", "title-specific", "the HeartGold Spinarak traded into a Platinum save",
           expect(O::NoProblemsFound),
           runSave(save(L::Platinum, fromHex(kHeartGoldSpinarak), kPlatinum), "platinum_nds"));
    record("HeartGold", "title-specific", "the HeartGold Spinarak L2 with version SoulSilver, in a HeartGold save",
           expect(O::Invalid, {noEncounter}), run(withVersion(kHeartGoldSpinarak, kSoulSilver), "heartgold_nds"));
    // pokeheartgold include/constants/balls.h:25-33: the Apricorn balls are 17-24 (Level Ball 18).
    record("HeartGold", "title-specific", "Spinarak in a Level Ball (an HG/SS Apricorn Ball)",
           expect(O::NoProblemsFound),
           run(edited(kHeartGoldSpinarak, [](std::vector<uint8_t>& pk) { pk[kBallHGSS] = 18; }), "heartgold_nds"));
    // pokeheartgold include/constants/map_sections.h:237: met location 233 is the Pokewalker.
    record("HeartGold", "title-specific", "Pikachu L15 from the Pokewalker (met location 233)",
           expect(O::NoProblemsFound), run(fromHex(kHeartGoldWalkerPikachu), "heartgold_nds"));
    record("HeartGold", "title-specific", "SoulSilver save bytes (ROM code 8) assigned to HeartGold",
           rejected(mismatch), runSave(save(L::HeartGoldSoulSilver, fromHex(kSoulSilverLedyba), kSoulSilver), "heartgold_nds"));
    record("SoulSilver", "title-specific", "the SoulSilver Ledyba L3 with version HeartGold, in a SoulSilver save",
           expect(O::Invalid, {noEncounter}), run(withVersion(kSoulSilverLedyba, kHeartGold), "soulsilver_nds"));
    // pokeheartgold files/data/mushi/mushi_encount.csv:2: Caterpie L7-18 in the Bug-Catching
    // Contest; src/pokemon.c:1298-1307 stores the Sport Ball as 24 at 0x86.
    record("SoulSilver", "title-specific", "Caterpie in a Sport Ball, Bug-Catching Contest, National Park",
           expect(O::NoProblemsFound), run(fromHex(kSoulSilverSportCaterpie), "soulsilver_nds"));
    // pokeplatinum res/text/location_names.json ends at location 125 (:505): Platinum has no
    // location 233.
    record("SoulSilver", "title-specific", "the Pokewalker Pikachu with version Platinum (no Pokewalker in Pt)",
           expect(O::Invalid, {noEncounter}), run(withVersion(kSoulSilverWalkerPikachu, kPlatinum), "soulsilver_nds"));
    record("SoulSilver", "title-specific", "the Platinum Rotom traded into a SoulSilver save",
           expect(O::NoProblemsFound),
           runSave(save(L::HeartGoldSoulSilver, fromHex(kPlatinumRotom), kSoulSilver), "soulsilver_nds"));
}

}  // namespace gen4

// ---------------------------------------------------------------- Gen V

namespace gen5 {

namespace Crypto = PokeVault::Integration::Gen5::Crypto;
using Gen5TestHelpers::fromHex;

// Decrypted 136-byte records from the PKHeX-generated corpus of tools/pkhex_gen5_diff
// (`oracle generate 1 4`, PKHeX kwsch/PKHeX@6501f0ab); PKHeX calls each one VALID.
// Black: Cottonee L14, Pinwheel Forest (met location 33), a Black-only grass slot.
constexpr const char* kBlackCottonee =
    "95CDDC960000097622020000719CB1F0B80A0000469E0002000000000000000000000000000000004A0049004E004800280A1E0F0000000023B3462600000000000000000000000043006F00740074006F006E0065006500FFFF0000000000150000000000000000540065007300740065007200FFFF00000000001A0A0F0000210000040E000000";
// White: Petilil L14, Pinwheel Forest, a White-only grass slot.
constexpr const char* kWhitePetilil =
    "555F262F0000457224020000DFFF68D4B80A000046220002000000000000000000000000000000004A0049004F004800280A0F0F00000000FCFD7C0000000000020D00000000000050006500740069006C0069006C00FFFF00000000000000140000000000000000540065007300740065007200FFFF00000000001A0A0F0000210000048E000000";
// Black 2: Cottonee L15, Castelia City (met location 8), a Black 2-only slot.
constexpr const char* kBlack2Cottonee =
    "681CEE9200002DF6220200003E3E37512F0D0000469E0002000000000000000000000000000000004A0049004E004800280A1E0F00000000D52C840E00000000021700000000000043006F00740074006F006E0065006500FFFF0000000000170000000000000000540065007300740065007200FFFF00000000001A0A0F0000080000048F000000";
// White 2: Petilil L15, Castelia City, a White 2-only slot.
constexpr const char* kWhite2Petilil =
    "FE3334AC000084C42402000097064A2D2F0D000046220002000000000000000000000000000000004A0049004F004800280A0F0F0000000003B03B3A00000000021000000000000050006500740069006C0069006C00FFFF00000000000000160000000000000000540065007300740065007200FFFF00000000001A0A0F0000080000040F000000";
// N's Purrloin (Black 2 / White 2 static gift, OT "N", TID 2), here from Black 2.
constexpr const char* kNsPurrloin =
    "7F0001FF0000B28EFD010000020000005701000046540002000000000000000000000000000000000A002D00120100002328140000000000DE7BEF3D00000000000A02000000000050007500720072006C006F0069006E00FFFF00000000001700000000000000004E00FFFF0000000000000000000000000000001A0A0F00000F00000407000000";
// Transfers (Poke Transfer, met location 30001): the version byte keeps the Gen IV / III game.
constexpr const char* kDiamondCaterpie =
    "8158BAC400000A5A0A00000073A568A1D8000000461300080000000000000000000000000000000021005100000000002328000000000000B709D82C00000000000E00000000000090CE30D13CD5FFFF0000000000000000000000000000000A00000000000000004CD1A4C2B8D2FFFF00000000000000000000001A0A0F00003175000406020000";
constexpr const char* kPlatinumCaterpie =
    "DE8833090000C2300A00000029F737E77D000000461300070000000000000000000000000000000021005100000000002328000000000000D523D92300000000001200000000000043004100540045005200500049004500FFFF00000000000C0000000000000000500072006F006200610064006F00FFFF0000001A0A0F00003175000405020000";
constexpr const char* kHeartGoldCaterpie =
    "A8C0077D0000EAB90A000000E2A44EB7E8030000461300070000000000000000000000000000000021005100000000002328000000000000AF31671E00000000001600000000000043004100540045005200500049004500FFFF0000000000070000000000000000500072006F006200610064006F00FFFF0000001A0A0F0000317500048A020000";
constexpr const char* kSoulSilverSportBallCaterpie =
    "7D01CC04000038D70A000000D66B031D2F0D0000461300080000000000000000000000000000000021005100000000002328000000000000FF525E0000000000020E00000000000090CE30D13CD5FFFF0000000000000000000000000000000800000000000000004CD1A4C2B8D2FFFF00000000000000000000001A0A0F0000317500180F020000";
constexpr const char* kFireRedWeedle =
    "AEDD9D98000003C90D00000083A9E792D800000046130002000000000000000000000000000000002800510000000000232800000000000077736F2600000000000900000000000057004500450044004C004500FFFF000000000000000000040000000000000000540065007300740065007200FFFF00000000001A0A0F00003175000486000000";
constexpr const char* kLeafGreenCaterpie =
    "B7680C5E0000F69D0A00000056498F7B57010000461300040000000000000000000000000000000021005100000000002328000000000000E9B7D72100000000001200000000000043004100540045005200500049004500FFFF0000000000050000000000000000500072006F0076006100FFFF000000000000001A0A0F00003175000487000000";
// Transfers whose origin the analyzer still cannot decide; PKHeX calls each VALID.
// XD Shadow Butterfree and Colosseum Shadow Bayleef (version byte 15, Colosseum / XD).
constexpr const char* kXdShadowButterfree =
    "D644DDD2000090E00C000000B1A78B9B78690000460E0007000000000000000000000000000000005E00EA004F004C010A050F1400000000170F600F00000020010D0000000000004200550054005400450052004600520045004500FFFF000F0000000000000000500072006F006200610064006F00FFFF0000001A0A0F0000317500041E000000";
constexpr const char* kColosseumShadowBayleef =
    "A574741900008F089900000053744475005500004641000300000000000000000000000000000000F100EB004B0022000505190F00000000F375D11400000020000C0000000000004D004100430052004F004E00490055004D00FFFF0000000F00000000000000005400650073007400650075007200FFFF0000001A0A100000317500041E000000";
// A Sapphire egg whose TID/SID PKHeX finds no Ruby/Sapphire seed for (Fishy
// Trainer:TrainerIDNoSeed), and a LeafGreen Caterpie whose Method 1 seed reaches no frame
// that gives this slot and level (Fishy PID:EncConditionBadRNGFrame). PKHeX does not call
// either Invalid, so neither may read Invalid or "No problems found" here.
constexpr const char* kSapphireEggTid =
    "D63257B20000F162010000004159342F87000000464100040000000000000000000000000000000021002D00000000002328000000000000E06FF63F000000000013000000000000420055004C00420041005300410055005200FFFF000000010000000000000000500072006F0076006100FFFF000000000000001A0A1000003175000485000000";
constexpr const char* kLeafGreenRngFrame =
    "FF42ED7E00001A6C0A000000159854287D000000461300010000000000000000000000000000000021005100000000002328000000000000540CD81A000000000018000000000000AD30E330BF30D430FC30FFFF0000000000000000000000050000000000000000C630B930C830FFFF00000000000000000000001A0A1000003175000484000000";

constexpr std::size_t kSpecies = 0x08, kMoves = 0x28, kPp = 0x30, kFlags = 0x40, kFlags2 = 0x42;
constexpr std::size_t kVersion = 0x5F, kBall = 0x83, kMetLevel = 0x84, kNickname = 0x48, kOt = 0x68;
constexpr uint8_t kBlack = 21, kWhite = 20, kBlack2 = 23, kWhite2 = 22;

std::vector<uint8_t> record5(const char* hex) { return fromHex(hex); }

std::vector<uint8_t> edited(const char* hex, const std::function<void(std::vector<uint8_t>&)>& edit) {
    auto pk = fromHex(hex);
    edit(pk);
    Gen5TestHelpers::refreshChecksum(pk);
    return pk;
}

std::vector<uint8_t> withVersion(const char* hex, uint8_t version) {
    return edited(hex, [version](std::vector<uint8_t>& pk) { pk[kVersion] = version; });
}

// The decrypted record, then its stored form; both must give the same report.
Actual run(const std::vector<uint8_t>& decrypted) {
    Actual a;
    const auto copy = decrypted;
    a.report = Legality::analyzePk5(decrypted, {.encrypted = false});
    a.outcome = fromReport(*a.report);
    const auto stored = Crypto::encryptCandidate(decrypted);
    if (!stored.empty()) {
        const auto storedCopy = stored;
        if (!Gen5TestHelpers::sameReport(*a.report, Legality::analyzePk5(stored)))
            a.problems.push_back("the stored (encrypted) form gives a different report");
        if (stored != storedCopy)
            a.problems.push_back("stored bytes changed");
        if (Crypto::checksum(decrypted) == static_cast<uint16_t>(decrypted[6] | (decrypted[7] << 8)) &&
            Crypto::decrypt(stored) != decrypted)
            a.problems.push_back("the stored form does not decrypt back to the record");
    }
    if (decrypted != copy)
        a.problems.push_back("record bytes changed");
    return a;
}

// Stored (encrypted) bytes as a save holds them.
Actual runStored(const std::vector<uint8_t>& stored) {
    Actual a;
    const auto copy = stored;
    a.report = Legality::analyzePk5(stored);
    a.outcome = fromReport(*a.report);
    if (stored != copy)
        a.problems.push_back("stored bytes changed");
    return a;
}

void swapBlocks(std::vector<uint8_t>& pk, std::size_t a, std::size_t b) {
    std::swap_ranges(pk.begin() + static_cast<std::ptrdiff_t>(a), pk.begin() + static_cast<std::ptrdiff_t>(a + 32),
                     pk.begin() + static_cast<std::ptrdiff_t>(b));
}

void cases() {
    using O = Outcome;
    const auto noEncounter = invalid("No Gen V encounter gives");
    const auto cxdWarning = warning("Colosseum / XD origin: the original encounter and moves are not checked");
    const auto checksumInvalid = invalid("PK5 checksum does not match its data");

    // Positive: a version-exclusive wild slot in its own game.
    record("Black", "positive", "Cottonee L14, Pinwheel Forest, Black-only slot", expect(O::NoProblemsFound),
           run(record5(kBlackCottonee)));
    record("White", "positive", "Petilil L14, Pinwheel Forest, White-only slot", expect(O::NoProblemsFound),
           run(record5(kWhitePetilil)));
    record("Black 2", "positive", "Cottonee L15, Castelia City, Black 2-only slot", expect(O::NoProblemsFound),
           run(record5(kBlack2Cottonee)));
    record("White 2", "positive", "Petilil L15, Castelia City, White 2-only slot", expect(O::NoProblemsFound),
           run(record5(kWhite2Petilil)));

    // Title-specific: the version byte is in the record. The same bytes with the paired
    // version byte claim a slot that game does not have.
    record("Black", "title-specific", "the Black Cottonee with version byte White", expect(O::Invalid, {noEncounter}),
           run(withVersion(kBlackCottonee, kWhite)));
    record("White", "title-specific", "the White Petilil with version byte Black", expect(O::Invalid, {noEncounter}),
           run(withVersion(kWhitePetilil, kBlack)));
    record("Black 2", "title-specific", "the Black 2 Cottonee with version byte White 2",
           expect(O::Invalid, {noEncounter}), run(withVersion(kBlack2Cottonee, kWhite2)));
    record("White 2", "title-specific", "the White 2 Petilil with version byte Black 2",
           expect(O::Invalid, {noEncounter}), run(withVersion(kWhite2Petilil, kBlack2)));
    record("Black 2", "title-specific", "N's Purrloin from Black 2", expect(O::NoProblemsFound),
           run(record5(kNsPurrloin)));
    record("White 2", "title-specific", "N's Purrloin with version byte White 2", expect(O::NoProblemsFound),
           run(withVersion(kNsPurrloin, kWhite2)));
    record("Black", "title-specific", "N's Purrloin with version byte Black (not in Black)",
           expect(O::Invalid, {invalid("Closest Gen V encounter (wild encounter, Purrloin) does not match")}),
           run(withVersion(kNsPurrloin, kBlack)));

    // Negative.
    record("Black", "negative", "Black Cottonee with the fateful-encounter flag",
           expect(O::Invalid, {invalid("Fateful encounter flag is set on a non-event encounter")}),
           run(edited(kBlackCottonee, [](std::vector<uint8_t>& pk) { pk[kFlags] |= 1; })));
    record("White", "negative", "White Petilil with met level 13 (the slot is level 14)",
           expect(O::Invalid, {invalid("Closest Gen V encounter")}),
           run(edited(kWhitePetilil,
                      [](std::vector<uint8_t>& pk) { pk[kMetLevel] = static_cast<uint8_t>((pk[kMetLevel] & 0x80) | 13); })));
    record("Black 2", "negative", "Black 2 Cottonee knowing Surf",
           expect(O::Invalid, {invalid("Surf cannot be learned by this Pokemon in Gen V")}),
           run(edited(kBlack2Cottonee, [](std::vector<uint8_t>& pk) {
               pk[kMoves] = 57;  // Surf
               pk[kMoves + 1] = 0;
               pk[kPp] = 15;
           })));
    record("White 2", "negative", "White 2 Petilil with N's sparkle flag",
           expect(O::Invalid, {invalid("N's sparkle flag is set on a Pokemon that is not N's")}),
           run(edited(kWhite2Petilil, [](std::vector<uint8_t>& pk) { pk[kFlags2] |= 2; })));

    // Malformed.
    {
        auto pk = record5(kBlackCottonee);
        pk.pop_back();
        Expect e = expect(O::Incomplete, {warning("Not a 136-byte or 220-byte PK5 record")});
        e.structureNone = true;
        record("Black", "malformed", "Black Cottonee cut to 135 bytes", e, run(pk));
    }
    {
        auto stored = Crypto::encryptCandidate(record5(kWhitePetilil));
        stored[0x10] ^= 0x01;
        record("White", "malformed", "stored White Petilil, one encrypted byte flipped",
               expect(O::Invalid, {checksumInvalid}), runStored(stored));
    }
    {
        auto stored = Crypto::encryptCandidate(record5(kBlack2Cottonee));
        swapBlocks(stored, 0x08, 0x28);
        record("Black 2", "malformed", "stored Black 2 Cottonee, first two encrypted blocks swapped",
               expect(O::Invalid, {checksumInvalid}), runStored(stored));
    }
    {
        // The checksum is a sum of words, so it does not see block order.
        auto pk = record5(kBlack2Cottonee);
        swapBlocks(pk, 0x08, 0x28);
        record("Black 2", "malformed", "decrypted Black 2 Cottonee, blocks A and B swapped",
               expect(O::Invalid, {invalid("is above the Gen V maximum 559")}),
               run(pk));
    }
    record("White 2", "malformed", "White 2 Petilil with species id 0xFFFF",
           expect(O::Invalid, {invalid("Species 65535 is above the Gen V maximum 649")}),
           run(edited(kWhite2Petilil, [](std::vector<uint8_t>& pk) {
               pk[kSpecies] = 0xFF;
               pk[kSpecies + 1] = 0xFF;
           })));
    record("White 2", "malformed", "White 2 Petilil, nickname and OT all U+0001, no terminator",
           expect(O::Invalid, {invalid("Nickname is longer than 10 characters"),
                               invalid("Original Trainer name is longer than 7 characters")}),
           run(edited(kWhite2Petilil, [](std::vector<uint8_t>& pk) {
               for (std::size_t i = 0; i < 11; ++i) {
                   pk[kNickname + 2 * i] = 1;
                   pk[kNickname + 2 * i + 1] = 0;
               }
               for (std::size_t i = 0; i < 8; ++i) {
                   pk[kOt + 2 * i] = 1;
                   pk[kOt + 2 * i + 1] = 0;
               }
           })));

    // Transfers: a transferred Pokemon keeps its Gen IV / III version byte, and nothing in
    // the record says which Gen V game received it, so the verdict does not depend on the
    // title. The Gen IV analyzer's origin analysis (Gen4::analyzeOrigin5) matches the
    // original encounter and the moves, so a clean PKHeX-VALID transfer is certified.
    const auto origin = [](const char* kind, const char* species) {
        return info(std::string("Gen III/IV origin matches a ") + kind + " (" + species + ")");
    };
    record("Black", "positive", "Diamond Caterpie transferred",
           expect(O::NoProblemsFound, {origin("Gen IV wild encounter", "Caterpie")}), run(record5(kDiamondCaterpie)));
    record("White", "positive", "FireRed Weedle transferred",
           expect(O::NoProblemsFound, {origin("Gen III wild encounter", "Weedle")}), run(record5(kFireRedWeedle)));
    record("Black 2", "positive", "HeartGold Caterpie transferred",
           expect(O::NoProblemsFound, {origin("Gen IV wild encounter", "Caterpie")}), run(record5(kHeartGoldCaterpie)));
    record("White 2", "positive", "LeafGreen Caterpie transferred",
           expect(O::NoProblemsFound, {origin("Gen III wild encounter", "Caterpie")}), run(record5(kLeafGreenCaterpie)));

    // Unknown: transfers whose origin is not decided here. Colosseum / XD encounters are
    // not modelled, and PKHeX's "fishy" Gen III results stay Warnings.
    record("Black", "unknown", "XD Shadow Butterfree transferred", expect(O::Incomplete, {cxdWarning}),
           run(record5(kXdShadowButterfree)));
    record("White", "unknown", "Sapphire egg on a TID/SID with no Ruby/Sapphire seed",
           expect(O::Incomplete, {warning("cannot be generated by Ruby/Sapphire")}), run(record5(kSapphireEggTid)));
    record("Black 2", "unknown", "LeafGreen Caterpie on no reachable RNG frame",
           expect(O::Incomplete, {warning("reachable RNG frame for this Generation III")}),
           run(record5(kLeafGreenRngFrame)));
    record("White 2", "unknown", "Colosseum Shadow Bayleef transferred", expect(O::Incomplete, {cxdWarning}),
           run(record5(kColosseumShadowBayleef)));

    // Original-game: Gen V has no disassembly, so these use the Gen IV decompilations
    // (pokeplatinum generated/items.txt:2-17, pokeheartgold include/constants/items.h:270-285
    // and 913-921): no Gen IV game has a Dream Ball, and the Sport Ball is HeartGold /
    // SoulSilver only. The Bug-Catching Contest gives Caterpie at L7-18
    // (pokeheartgold files/data/mushi/mushi_encount.csv:2) and throws only Sport Balls
    // (src/battle/battle_controller_player.c:1865-1873), so the SoulSilver Caterpie
    // transferred at L15 is certified.
    record("Black", "original-game", "Platinum Caterpie in a Dream Ball",
           expect(O::Invalid, {invalid("The Dream Ball did not exist in Gen IV")}),
           run(edited(kPlatinumCaterpie, [](std::vector<uint8_t>& pk) { pk[kBall] = 25; })));
    record("White", "original-game", "HeartGold Caterpie in a Dream Ball",
           expect(O::Invalid, {invalid("The Dream Ball did not exist in Gen IV")}),
           run(edited(kHeartGoldCaterpie, [](std::vector<uint8_t>& pk) { pk[kBall] = 25; })));
    record("Black 2", "original-game", "Platinum Caterpie in a Sport Ball",
           expect(O::Invalid, {invalid("Ball 24 exists only in HeartGold / SoulSilver")}),
           run(edited(kPlatinumCaterpie, [](std::vector<uint8_t>& pk) { pk[kBall] = 24; })));
    record("White 2", "original-game", "SoulSilver Caterpie in a Sport Ball",
           expect(O::NoProblemsFound, {origin("Gen IV wild encounter", "Caterpie")},
                  {invalid("exists only in HeartGold / SoulSilver")}),
           run(record5(kSoulSilverSportBallCaterpie)));
}

}  // namespace gen5

}  // namespace

int main(int argc, char** argv) {
    verbose = argc > 1 && std::strcmp(argv[1], "-v") == 0;
    gen1::cases();
    gen2::cases();
    gen3::cases();
    gen4::cases();
    gen5::cases();

    std::printf("\nCases per title and kind:\n%-10s", "");
    for (const char* kind : kKinds)
        std::printf(" | %-14s", kind);
    std::printf(" | total\n");
    for (const char* title : kTitles) {
        int total = 0;
        std::printf("%-10s", title);
        for (const char* kind : kKinds) {
            const int n = tally[title][kind];
            total += n;
            std::printf(" | %-14d", n);
        }
        std::printf(" | %d\n", total);
    }
    if (failureCount != 0) {
        std::printf("Title matrix: %d of %d cases FAILED\n", failureCount, caseCount);
        return EXIT_FAILURE;
    }
    std::printf("Title matrix: %d cases, %d PASS, %d known engine bug(s) (BUG rows)\n", caseCount,
                caseCount - bugCount, bugCount);
    return EXIT_SUCCESS;
}
