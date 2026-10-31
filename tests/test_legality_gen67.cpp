// Gen VI (PK6) and Gen VII (PK7) legality through Legality::Modern::analyze.
//
// tests/fixtures/gen67/pkhex_seed2_sample.txt is a stratified sample of the differential
// corpus (tools/pkhex_gen67_diff, `generate 2 8`): synthetic records PKHeX.Core built from
// its own Gen VI/VII encounter templates, Gen V encounters moved with Poke Transporter,
// Virtual Console Gen I/II encounters and grown copies, each with PKHeX's verdict and a
// single-field mutation. One line per record:
//   <6|7> <game> <box|party> <decrypted hex> <PKHeX VALID|INVALID> <NX Invalid|Incomplete> <label>
// Every line must keep its NX verdict class: a PKHeX-valid record is never Invalid, a
// record this analyzer caught stays caught, and nothing is ever "No problems found"
// (no modern area has Complete coverage yet).
// Then rule edits of fixture records, the Modern interface contract, malformed sizes,
// encryption round trips, input immutability and a deterministic fuzz pass (run under
// ASan/UBSan by host-sanitize).
#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <vector>

#include "Integration/Gen6/Gen6PK6.h"
#include "Legality/Gen67Legality.h"
#include "Legality/ModernLegality.h"

namespace {

namespace PK6 = PokeVault::Integration::Gen6::PK6;
using Legality::Report;
using Legality::Severity;
using Legality::Verdict;
using Legality::Modern::Format;
using Legality::Modern::Game;
using Legality::Modern::Input;
using Legality::Modern::Slot;

int failures = 0;
int checks = 0;

void check(bool ok, const std::string& what) {
    ++checks;
    if (!ok) {
        ++failures;
        std::cerr << "FAIL: " << what << "\n";
    }
}

std::vector<uint8_t> fromHex(const std::string& hex) {
    std::vector<uint8_t> out(hex.size() / 2);
    for (size_t i = 0; i < out.size(); ++i) out[i] = static_cast<uint8_t>(std::stoi(hex.substr(2 * i, 2), nullptr, 16));
    return out;
}

bool hasIssue(const Report& r, Severity s, const std::string& fragment) {
    return std::any_of(r.issues.begin(), r.issues.end(),
                       [&](const auto& i) { return i.severity == s && i.text.find(fragment) != std::string::npos; });
}

std::string describe(const Report& r) {
    std::string out;
    for (const auto& i : r.issues)
        out += std::string(i.severity == Severity::Invalid ? "[Invalid] " : i.severity == Severity::Warning ? "[Warning] " : "[Info] ") + i.text + "; ";
    return out;
}

struct Record {
    Format format;
    Game game;
    Slot slot;
    std::vector<uint8_t> plain;
    bool pkhexValid;
    bool nxInvalid;
    std::string label;
};

std::vector<Record> loadFixture() {
    std::vector<Record> out;
    const char* path = std::getenv("GEN67_FIXTURE");
    std::ifstream in(path ? path : "tests/fixtures/gen67/pkhex_seed2_sample.txt");
    std::string line;
    while (std::getline(in, line)) {
        std::istringstream s(line);
        std::string fmt, game, slot, hex, pkhex, nx, label;
        if (!(s >> fmt >> game >> slot >> hex >> pkhex >> nx >> label)) continue;
        out.push_back({fmt == "6" ? Format::PK6 : Format::PK7, static_cast<Game>(std::stoi(game)),
                       slot == "party" ? Slot::Party : Slot::Box, fromHex(hex), pkhex == "VALID", nx == "Invalid", label});
    }
    return out;
}

Report analyzePlain(const Record& r, const std::vector<uint8_t>& plain) {
    Input in;
    in.format = r.format;
    in.game = r.game;
    in.slot = r.slot;
    in.encrypted = false;
    return Legality::Modern::analyze(plain, in);
}

// Edits a decrypted record and refreshes its checksum, like a save editor would.
std::vector<uint8_t> edit(const std::vector<uint8_t>& plain, const std::function<void(std::vector<uint8_t>&)>& f) {
    auto copy = plain;
    f(copy);
    PK6::write16(copy, 0x06, PK6::checksum(copy));
    return copy;
}

void testFixture(const std::vector<Record>& records) {
    check(records.size() > 1000, "fixture loaded (" + std::to_string(records.size()) + " records)");
    size_t valid = 0, caught = 0;
    for (const auto& r : records) {
        const auto report = analyzePlain(r, r.plain);
        const auto v = report.verdict();
        check(v != Verdict::NoProblemsFound, r.label + ": never No problems found");
        if (r.pkhexValid) {
            ++valid;
            check(v != Verdict::Invalid, r.label + ": PKHeX-valid record reported Invalid: " + describe(report));
        }
        if (r.nxInvalid) {
            ++caught;
            check(v == Verdict::Invalid, r.label + ": previously caught record is no longer Invalid");
        } else {
            check(v == Verdict::Incomplete, r.label + ": expected Incomplete, got " + describe(report));
        }
        // Encrypted and decrypted input give the same report.
        Input enc;
        enc.format = r.format;
        enc.game = r.game;
        enc.slot = r.slot;
        const auto stored = PK6::encrypt(r.plain);
        const auto viaStored = Legality::Modern::analyze(stored, enc);
        check(viaStored.verdict() == v && viaStored.issues.size() == report.issues.size(), r.label + ": encrypted path differs");
    }
    check(valid > 500 && caught > 500, "fixture covers both PKHeX-valid and caught records");
}

const Record* find(const std::vector<Record>& records, const std::function<bool(const Record&)>& pred) {
    for (const auto& r : records)
        if (pred(r)) return &r;
    return nullptr;
}

void testRuleEdits(const std::vector<Record>& records) {
    // A PKHeX-valid native PK7 and PK6 record from the fixture, never a personal save.
    const Record* pk7 = find(records, [](const Record& r) {
        return r.pkhexValid && r.format == Format::PK7 && r.label.rfind("native7:base", 0) == 0;
    });
    const Record* pk6 = find(records, [](const Record& r) {
        return r.pkhexValid && r.format == Format::PK6 && r.label.rfind("native6:base", 0) == 0;
    });
    check(pk7 && pk6, "fixture has native PK6 and PK7 bases");
    if (!pk7 || !pk6) return;
    auto expectInvalid = [&](const Record& base, const char* name, const std::function<void(std::vector<uint8_t>&)>& f, const char* fragment) {
        const auto r = analyzePlain(base, edit(base.plain, f));
        check(r.verdict() == Verdict::Invalid && hasIssue(r, Severity::Invalid, fragment),
              std::string(name) + ": expected Invalid '" + fragment + "', got " + describe(r));
    };
    expectInvalid(*pk7, "ev total", [](auto& b) { for (int i = 0; i < 6; ++i) b[0x1E + i] = 100; }, "EV total is above 510");
    expectInvalid(*pk7, "ev 253", [](auto& b) { b[0x1E] = 253; for (int i = 1; i < 6; ++i) b[0x1E + i] = 0; }, "An EV is above 252");
    expectInvalid(*pk7, "nature", [](auto& b) { b[0x1C] = 25; }, "Nature is out of range");
    expectInvalid(*pk7, "duplicate move", [](auto& b) { PK6::write16(b, 0x5C, PK6::read16(b, 0x5A)); }, "is duplicated");
    expectInvalid(*pk7, "empty first move", [](auto& b) { PK6::write16(b, 0x5A, 0); b[0x62] = 0; b[0x66] = 0; }, "first move slot is empty");
    expectInvalid(*pk7, "move id", [](auto& b) { PK6::write16(b, 0x5A, 729); }, "does not exist");
    expectInvalid(*pk7, "pp", [](auto& b) { b[0x62] = 99; }, "PP is above its maximum");
    expectInvalid(*pk7, "pid==ec", [](auto& b) { PK6::write32(b, 0x18, PK6::read32(b, 0x00)); }, "PID equals the encryption constant");
    expectInvalid(*pk7, "ability number", [](auto& b) { b[0x15] = 3; }, "Ability number is not 1, 2 or 4");
    expectInvalid(*pk7, "ability", [](auto& b) { b[0x14] = 0; }, "not one of the species' abilities");
    expectInvalid(*pk7, "ball", [](auto& b) { b[0xDC] = 27; }, "does not exist");
    expectInvalid(*pk7, "language", [](auto& b) { b[0xE3] = 6; }, "is not a language");
    expectInvalid(*pk7, "origin", [](auto& b) { b[0xDF] = 34; }, "is not a game");
    expectInvalid(*pk7, "console region", [](auto& b) { b[0xE2] = 7; }, "Console region 7 does not exist");
    expectInvalid(*pk7, "hyper training below 100", [](auto& b) { b[0xDE] = 1; PK6::write32(b, 0x10, 0); }, "Hyper Training needs level 100");
    expectInvalid(*pk7, "species", [](auto& b) { PK6::write16(b, 0x08, 808); }, "does not exist in PK7");
    expectInvalid(*pk6, "pk6 species", [](auto& b) { PK6::write16(b, 0x08, 722); }, "does not exist in PK6");
    expectInvalid(*pk6, "pk6 origin newer", [](auto& b) { b[0xDF] = 30; }, "newer than PK6");
    expectInvalid(*pk6, "pk6 vc origin", [](auto& b) { b[0xDF] = 35; }, "Virtual Console origin");
    expectInvalid(*pk6, "pk6 held item", [](auto& b) { PK6::write16(b, 0x0A, 776); }, "not obtainable");
    expectInvalid(*pk6, "level below met", [](auto& b) { PK6::write32(b, 0x10, 0); b[0xDD] = static_cast<uint8_t>((b[0xDD] & 0x80) | 50); }, "below the met level");

    // Bad checksum and bad sanity are Invalid structure findings.
    {
        auto bad = pk7->plain;
        bad[0x08] ^= 1;  // species changes without a checksum refresh
        const auto r = analyzePlain(*pk7, bad);
        check(r.verdict() == Verdict::Invalid && hasIssue(r, Severity::Invalid, "checksum"), "checksum mismatch is Invalid");
        auto sanity = edit(pk7->plain, [](auto& b) { b[0x04] = 1; });
        const auto rs = analyzePlain(*pk7, sanity);
        check(rs.verdict() == Verdict::Invalid && hasIssue(rs, Severity::Invalid, "sanity"), "sanity flag is Invalid");
    }
    // A record from the wrong family: a source-game Warning, not an Invalid Pokemon.
    {
        Record moved = *pk7;
        moved.game = Game::X;
        const auto r = analyzePlain(moved, moved.plain);
        check(r.verdict() == Verdict::Incomplete && hasIssue(r, Severity::Warning, "does not store PK7"),
              "PK7 in a Gen VI game is a source-game Warning");
    }
}

void testInterface(const std::vector<Record>& records) {
    using Legality::Modern::profile;
    check(profile(Format::PK6)->storedSize == 0xE8 && profile(Format::PK6)->partySize == 0x104, "PK6 sizes");
    check(profile(Format::PK8)->storedSize == 0x148 && profile(Format::PK8)->partySize == 0x158, "PK8 sizes");
    check(profile(Format::PA8)->storedSize == 0x168 && profile(Format::PA8)->partySize == 0x178, "PA8 sizes");
    check(profile(Format::PB7)->storedSize == 0x104, "PB7 size");
    check(profile(Format::Unknown) == nullptr, "Unknown has no profile");
    check(Legality::Modern::nativeFormat(Game::UM) == Format::PK7 && Legality::Modern::nativeFormat(Game::OR) == Format::PK6 &&
          Legality::Modern::nativeFormat(Game::ZA) == Format::PA9 && Legality::Modern::nativeFormat(Game::Unknown) == Format::Unknown,
          "native formats");
    check(Legality::Modern::generationOf(Game::SW) == 8 && Legality::Modern::generationOf(Game::SL) == 9, "generations");

    // Every format now has an analyzer (PK8, PB8 and PK9: tests/test_legality_gen8.cpp; PB7:
    // tests/test_legality_gen7b.cpp; PA8: tests/test_legality_gen8a.cpp; PA9:
    // tests/test_legality_gen9a.cpp). Garbage bytes of the right size still never pass.
    for (Format f : {Format::PK6, Format::PK7, Format::PB7, Format::PK8, Format::PB8, Format::PA8, Format::PK9, Format::PA9}) {
        check(profile(f)->rulesImplemented, std::string(profile(f)->name) + " has rules");
        std::vector<uint8_t> bytes(profile(f)->partySize, 0x5A);
        Input in;
        in.format = f;
        const auto r = Legality::Modern::analyze(bytes, in);
        check(r.verdict() != Verdict::NoProblemsFound, std::string(profile(f)->name) + " garbage is never clean");
    }
    // Unknown format.
    {
        std::vector<uint8_t> bytes(0xE8, 1);
        const auto r = Legality::Modern::analyze(bytes, Input{});
        check(r.verdict() == Verdict::Incomplete && hasIssue(r, Severity::Warning, "format is unknown"), "unknown format");
    }
    // Empty slots and malformed sizes.
    for (Format f : {Format::PK6, Format::PK7}) {
        Input in;
        in.format = f;
        std::vector<uint8_t> zero(0xE8, 0);
        check(Legality::Modern::analyze(zero, in).issues.empty(), "all-zero record is an empty slot");
        std::vector<uint8_t> shortRec(0x88, 1);
        const auto r = Legality::Modern::analyze(shortRec, in);
        check(r.coverage.structure == Legality::CoverageLevel::None && r.verdict() == Verdict::Incomplete, "wrong size");
        check(Legality::Modern::analyze(std::span<const uint8_t>{}, in).issues.size() == 1, "empty span is a wrong size");
    }
    // Species zero with a payload is ambiguous, not a blank slot.
    if (!records.empty()) {
        auto zeroSpecies = edit(records.front().plain, [](auto& b) { PK6::write16(b, 0x08, 0); });
        const auto r = analyzePlain(records.front(), zeroSpecies);
        check(r.verdict() == Verdict::Incomplete && hasIssue(r, Severity::Warning, "Species is zero"), "species zero payload");
    }
    // Trainer context: same IDs, different OT is a Warning; a traded Pokemon is not.
    if (const Record* base = find(records, [](const Record& r) { return r.pkhexValid; })) {
        Input in;
        in.format = base->format;
        in.game = base->game;
        in.slot = base->slot;
        in.encrypted = false;
        in.trainer.present = true;
        in.trainer.tid = PK6::read16(base->plain, 0x0C);
        in.trainer.sid = PK6::read16(base->plain, 0x0E);
        in.trainer.ot = {'Z', 'z', 0};
        check(hasIssue(Legality::Modern::analyze(base->plain, in), Severity::Warning, "OT name or gender"), "same ID, other OT");
        in.trainer.tid ^= 1;
        check(!hasIssue(Legality::Modern::analyze(base->plain, in), Severity::Warning, "OT name or gender"), "traded Pokemon");
        // Container integrity feeds source-game coverage only.
        in.integrity = Legality::Modern::Integrity::Verified;
        in.trainer.present = false;
        const auto verified = Legality::Modern::analyze(base->plain, in);
        check(verified.coverage.sourceGame == Legality::CoverageLevel::Complete && verified.verdict() == Verdict::Incomplete,
              "verified container: source game Complete, verdict still Incomplete");
        in.integrity = Legality::Modern::Integrity::Failed;
        check(hasIssue(Legality::Modern::analyze(base->plain, in), Severity::Warning, "integrity"), "failed container warning");
    }
}

void testHelpers() {
    using Legality::Gen67::originGeneration;
    check(originGeneration(3) == 3 && originGeneration(15) == 3 && originGeneration(7) == 4 && originGeneration(12) == 4, "gen3/4 ids");
    check(originGeneration(23) == 5 && originGeneration(27) == 6 && originGeneration(33) == 7, "gen5-7 ids");
    check(originGeneration(35) == 1 && originGeneration(41) == 2, "VC ids");
    for (uint8_t v : {0, 6, 9, 13, 14, 16, 19, 28, 29, 34, 42, 47, 52, 255}) check(originGeneration(v) == 0, "not an origin " + std::to_string(v));
    // PK5.GetTransferPID: shiny under Gen VI odds but not Gen V flips bit 31.
    check(Legality::Gen67::transferPid(0x12345678u, 0x12345678u ^ 0x00000008u) == (0x12345678u ^ 0x80000000u), "transfer pid flip");
    check(Legality::Gen67::transferPid(0x12345678u, 0) == 0x12345678u, "transfer pid keep");
    check(Legality::Gen67::unownForm3(0) == 0 && Legality::Gen67::unownForm3(0x03030303u) == (0xFFu % 28), "unown letters");
    // Generated tables agree with the existing personal data and spot values from PKHeX.
    const auto bulbasaur = Legality::Gen67::Data::personal(7, 1, 0);
    check(bulbasaur && bulbasaur->ability1 == 65 && bulbasaur->abilityHidden == 34 && bulbasaur->gender == 31, "Bulbasaur personal");
    check(Legality::Gen67::Data::formCount(6, 666) == 20 && Legality::Gen67::Data::formCount(7, 25) > 1, "form counts");
    check(Legality::Gen67::Data::basePP(6, 1) == 35 && Legality::Gen67::Data::basePP(7, 0) == 0, "base PP");
    check(Legality::Gen67::Data::maxSpecies(6) == 721 && Legality::Gen67::Data::maxSpecies(7) == 807, "species limits");
    check(Legality::Gen67::Data::gameMaxMove(Game::X) == 617 && Legality::Gen67::Data::gameMaxMove(Game::SN) == 719, "game move limits");
    check(Legality::Gen67::Data::battleOnlyForm(6, 6, 1) && !Legality::Gen67::Data::battleOnlyForm(6, 6, 0), "Mega Charizard is battle-only");
    check(Legality::Gen67::Data::consoleRegionCountryValid(0, 1) && !Legality::Gen67::Data::consoleRegionCountryValid(0, 49), "Japan region");
}

void testFuzz(const std::vector<Record>& records) {
    const char* env = std::getenv("GEN67_FUZZ_ROUNDS");
    const size_t rounds = env ? std::strtoul(env, nullptr, 10) : 20000;
    std::mt19937 rng(67);
    for (size_t i = 0; i < rounds; ++i) {
        Input in;
        in.format = (i & 1) ? Format::PK7 : Format::PK6;
        in.game = static_cast<Game>(std::array<uint8_t, 6>{0, 24, 27, 30, 33, 44}[rng() % 6]);
        in.slot = static_cast<Slot>(rng() % 4);
        in.encrypted = (rng() & 1) != 0;
        std::vector<uint8_t> bytes;
        if (!records.empty() && (rng() % 3) != 0) {
            // A fixture record with random byte flips and a refreshed checksum.
            const auto& base = records[rng() % records.size()];
            bytes = base.plain;
            for (int k = 0, n = 1 + static_cast<int>(rng() % 6); k < n; ++k) bytes[8 + rng() % (bytes.size() - 8)] ^= static_cast<uint8_t>(1u << (rng() % 8));
            PK6::write16(bytes, 0x06, PK6::checksum(bytes));
            if (in.encrypted) bytes = PK6::encrypt(bytes);
        } else {
            bytes.resize(std::array<size_t, 5>{0xE8, 0x104, 0xE7, 0, 0x148}[rng() % 5]);
            for (auto& b : bytes) b = static_cast<uint8_t>(rng());
        }
        const auto copy = bytes;
        const auto a = Legality::Modern::analyze(bytes, in);
        const auto b = Legality::Modern::analyze(bytes, in);
        if (bytes != copy) check(false, "input was modified");
        if (a.verdict() != b.verdict() || a.issues.size() != b.issues.size()) check(false, "nondeterministic report");
        if (a.verdict() == Verdict::NoProblemsFound) check(false, "fuzz record reported No problems found");
    }
    check(true, "fuzz rounds " + std::to_string(rounds));
}

} // namespace

int main() {
    const auto records = loadFixture();
    testFixture(records);
    testRuleEdits(records);
    testInterface(records);
    testHelpers();
    testFuzz(records);
    std::cout << "test_legality_gen67: " << checks << " checks, " << failures << " failures\n";
    return failures == 0 ? 0 : 1;
}
