// Per-title acceptance matrix for the Gen I, Gen II, Gen III and Gen V legality analyzers.
// docs/legality/TITLE_TEST_MATRIX.md lists the same cases with their evidence.
//
// Every case prints one line: title | kind | case | expected | actual | PASS or FAIL.
// The program exits non-zero when any case fails. Pass -v to print every report.
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
#include "Encryption/Encryption5.h"
#include "Integration/Gen1/Gen1ReadOnlySave.h"
#include "Integration/Gen2/Gen2ReadOnlySave.h"
#include "Integration/Gen3/PKSMGen3Adapter.h"
#include "Legacy/FRLGReadOnlyTrainer.h"
#include "Legality/Gen3Legality.h"
#include "Legality/Gen5Legality.h"
#include "Legality/Legality.h"
#include "Names/ItemNames.h"
#include "Names/SpeciesNames.h"
#include "Pokemon/Pokemon1ReadOnly.h"
#include "Pokemon/Pokemon2ReadOnly.h"
#include "Pokemon/Pokemon3FRLG.h"
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
int caseCount = 0, failureCount = 0;
std::map<std::string, std::map<std::string, int>> tally;  // title -> kind -> cases

const char* const kTitles[] = {"Red", "Blue", "Yellow", "Gold", "Silver", "Crystal",
                               "Ruby", "Sapphire", "Emerald", "FireRed", "LeafGreen",
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
    if (!pass)
        ++failureCount;
    std::printf("%-9s | %-14s | %-66s | expected %-30s | actual %-30s | %s\n", title.c_str(), kind.c_str(),
                name.c_str(), label(e.outcome, e.parserError).c_str(), label(a.outcome, a.parserError).c_str(),
                pass ? "PASS" : "FAIL");
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
    const auto colosseumXD = warning("Colosseum/XD origin; Gen III cartridge rules cannot verify this Pokemon");

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
    record("Sapphire", "unknown", "Seviper with origin Colosseum/XD (version 15)", expect(O::Incomplete, {colosseumXD}),
           run(withVersion(kSapphireSeviper, kColosseumXD), "sapphire_gba"));
    record("Emerald", "unknown", "Colosseum/XD-origin record in box 1 of an Emerald save",
           expect(O::Incomplete, {colosseumXD}),
           runSave(save(F::Emerald, withVersion(kEmeraldSmeargle, kColosseumXD)), SG::EmeraldGBA));
    record("FireRed", "unknown", "prize Scyther holding an Enigma Berry", expect(O::Incomplete, {enigma}),
           run(edited(kFireRedScyther, [](std::vector<uint8_t>& pk) { setLe16(pk, kItem, kEnigmaBerry); }),
               "firered_gba"));
    record("LeafGreen", "unknown", "Bellsprout with origin Colosseum/XD (version 15)", expect(O::Incomplete, {colosseumXD}),
           run(withVersion(kLeafGreenBellsprout, kColosseumXD), "leafgreen_gba"));

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
    const auto transferWarning = warning("Gen III/IV origin: the original encounter and moves are not checked");
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

    // Unknown: a transfer keeps its Gen IV / III version byte, and nothing in the record
    // says which Gen V game received it. The verdict does not depend on the title.
    record("Black", "unknown", "Diamond Caterpie transferred", expect(O::Incomplete, {transferWarning}),
           run(record5(kDiamondCaterpie)));
    record("White", "unknown", "FireRed Weedle transferred", expect(O::Incomplete, {transferWarning}),
           run(record5(kFireRedWeedle)));
    record("Black 2", "unknown", "HeartGold Caterpie transferred", expect(O::Incomplete, {transferWarning}),
           run(record5(kHeartGoldCaterpie)));
    record("White 2", "unknown", "LeafGreen Caterpie transferred", expect(O::Incomplete, {transferWarning}),
           run(record5(kLeafGreenCaterpie)));

    // Original-game: Gen V has no disassembly, so these use the Gen IV decompilations
    // (pokeplatinum generated/items.txt:2-17, pokeheartgold include/constants/items.h:270-285
    // and 913-921): no Gen IV game has a Dream Ball, and the Sport Ball is HeartGold /
    // SoulSilver only.
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
           expect(O::Incomplete, {transferWarning}, {invalid("exists only in HeartGold / SoulSilver")}),
           run(record5(kSoulSilverSportBallCaterpie)));
}

}  // namespace gen5

}  // namespace

int main(int argc, char** argv) {
    verbose = argc > 1 && std::strcmp(argv[1], "-v") == 0;
    gen1::cases();
    gen2::cases();
    gen3::cases();
    gen5::cases();

    std::printf("\nCases per title and kind:\n%-9s", "");
    for (const char* kind : kKinds)
        std::printf(" | %-14s", kind);
    std::printf(" | total\n");
    for (const char* title : kTitles) {
        int total = 0;
        std::printf("%-9s", title);
        for (const char* kind : kKinds) {
            const int n = tally[title][kind];
            total += n;
            std::printf(" | %-14d", n);
        }
        std::printf(" | %d\n", total);
    }
    std::printf("Gen IV titles (Diamond, Pearl, Platinum, HeartGold, SoulSilver): pending the Gen IV analyzer merge.\n");
    if (failureCount != 0) {
        std::printf("Title matrix: %d of %d cases FAILED\n", failureCount, caseCount);
        return EXIT_FAILURE;
    }
    std::printf("Title matrix: %d cases PASS\n", caseCount);
    return EXIT_SUCCESS;
}
