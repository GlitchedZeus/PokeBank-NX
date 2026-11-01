// Legends: Z-A (PA9) legality through Legality::Modern::analyze.
//
// tests/fixtures/gen67/pkhex9a_seed2_sample.txt is a stratified sample of the differential
// corpus (tools/pkhex_gen67_diff, `generate9a 2 4`): synthetic records PKHeX.Core built from its
// own Legends: Z-A encounter templates, records moved into Legends: Z-A through HOME and grown
// copies (Plus moves set by level), each with PKHeX's verdict and a single-field mutation. One
// line per record:
//   9a <game> <box|party> <decrypted hex> <PKHeX VALID|INVALID> <NX Invalid|Incomplete> <label>
// Every line must keep its NX verdict class: a PKHeX-valid record is never Invalid, a record
// this analyzer caught stays caught, and nothing is ever "No problems found". Then rule edits
// of fixture records, the interface contract, malformed input, input immutability
// and a deterministic fuzz pass (run under ASan/UBSan by host-sanitize).
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <functional>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <vector>

#include "Integration/Gen8/Gen8PK8.h"
#include "Pokemon/Experience.h"
#include "Legality/Gen8Legality.h"
#include "Legality/Gen9aLegality.h"
#include "Legality/ModernEncounter.h"
#include "Legality/ModernLegality.h"

namespace {

namespace PK8 = PokeVault::Integration::Gen8::PK8;
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
    Game game;
    Slot slot;
    std::vector<uint8_t> plain;
    bool pkhexValid;
    bool nxInvalid;
    std::string label;
};

std::vector<Record> loadFixture() {
    std::vector<Record> out;
    const char* path = std::getenv("GEN9A_FIXTURE");
    std::ifstream in(path ? path : "tests/fixtures/gen67/pkhex9a_seed2_sample.txt");
    std::string line;
    while (std::getline(in, line)) {
        std::istringstream s(line);
        std::string fmt, game, slot, hex, pkhex, nx, label;
        if (!(s >> fmt >> game >> slot >> hex >> pkhex >> nx >> label) || fmt != "9a") continue;
        out.push_back({static_cast<Game>(std::stoi(game)), slot == "party" ? Slot::Party : Slot::Box, fromHex(hex),
                       pkhex == "VALID", nx == "Invalid", label});
    }
    return out;
}

Input inputFor(const Record& r, bool encrypted = false) {
    Input in;
    in.format = Format::PA9;
    in.game = r.game;
    in.slot = r.slot;
    in.encrypted = encrypted;
    return in;
}

Report analyzePlain(const Record& r, const std::vector<uint8_t>& plain) { return Legality::Modern::analyze(plain, inputFor(r)); }

// Edits a decrypted record and refreshes its checksum, like a save editor would.
std::vector<uint8_t> edit(const std::vector<uint8_t>& plain, const std::function<void(std::vector<uint8_t>&)>& f) {
    auto copy = plain;
    f(copy);
    PK8::write16(copy, 0x06, PK8::checksum(copy));
    return copy;
}

const Record* find(const std::vector<Record>& records, const std::function<bool(const Record&)>& pred) {
    for (const auto& r : records)
        if (pred(r)) return &r;
    return nullptr;
}

uint64_t tracker(const std::vector<uint8_t>& b) { return PK8::read32(b, 0x127) | (uint64_t{PK8::read32(b, 0x12B)} << 32); }

void testFixture(const std::vector<Record>& records) {
    check(records.size() > 1500, "fixture loaded (" + std::to_string(records.size()) + " records)");
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
        const auto stored = PK8::encrypt(r.plain);
        check(PK8::decrypt(stored) == r.plain, r.label + ": codec round trip");
        const auto viaStored = Legality::Modern::analyze(stored, inputFor(r, true));
        check(viaStored.verdict() == v && viaStored.issues.size() == report.issues.size(), r.label + ": encrypted path differs");
    }
    check(valid > 300 && caught > 1000, "fixture covers both PKHeX-valid and caught records");
}

void testRuleEdits(const std::vector<Record>& records) {
    // A PKHeX-valid boxed Legends: Z-A native that never left the game and is not an alpha.
    const Record* native = find(records, [](const Record& r) {
        return r.pkhexValid && r.slot == Slot::Box && r.plain[0xCE] == 52 && tracker(r.plain) == 0 && r.plain[0x23] == 0 &&
               r.label.rfind("native9a:base", 0) == 0 && PK8::read16(r.plain, 0x74) != 0;
    });
    check(native != nullptr, "fixture has a boxed Legends: Z-A native base");
    if (!native) return;
    const Record& n = *native;
    auto expectInvalid = [&](const Record& base, const char* name, const std::function<void(std::vector<uint8_t>&)>& f, const char* fragment) {
        const auto r = analyzePlain(base, edit(base.plain, f));
        check(r.verdict() == Verdict::Invalid && hasIssue(r, Severity::Invalid, fragment),
              std::string(name) + ": expected Invalid '" + fragment + "', got " + describe(r));
    };
    auto expectNoIssue = [&](const Record& base, const char* name, const std::function<void(std::vector<uint8_t>&)>& f, const char* fragment) {
        const auto r = analyzePlain(base, edit(base.plain, f));
        check(!hasIssue(r, Severity::Invalid, fragment) && r.verdict() != Verdict::NoProblemsFound,
              std::string(name) + ": expected no Invalid '" + fragment + "', got " + describe(r));
    };
    expectInvalid(n, "ev", [](auto& b) { b[0x26] = 253; }, "An EV is above 252");
    if (PK8::read16(n.plain, 0x82) != 0 && PK8::read16(n.plain, 0x84) == 0)
        expectInvalid(n, "relearn repeated", [](auto& b) { PK8::write16(b, 0x84, PK8::read16(b, 0x82)); }, "is repeated");
    if (const Record* moved = find(records, [](const Record& r) {
            return r.pkhexValid && r.plain[0xCE] != 52 && PK8::read16(r.plain, 0x82) == 0;
        }))
        expectInvalid(*moved, "relearn after HOME", [](auto& b) { PK8::write16(b, 0x82, 33); }, "original moveset was deleted");
    expectInvalid(n, "ev total", [](auto& b) { b[0x26] = 252; b[0x27] = 252; b[0x28] = 7; }, "EV total is above 510");
    expectInvalid(n, "pp ups", [](auto& b) { b[0x7E] = 1; }, "has PP Ups");
    expectInvalid(n, "pp above", [](auto& b) { b[0x7A] = 99; }, "PP is above its maximum");
    expectInvalid(n, "pp used", [](auto& b) { b[0x7A] = static_cast<uint8_t>(b[0x7A] - 1); }, "neither full nor zero");
    expectNoIssue(n, "pp zero (HOME)", [](auto& b) { b[0x7A] = 0; }, "PP");
    expectInvalid(n, "level boost", [](auto& b) { b[0x4B] = 1; }, "Level boost");
    expectInvalid(n, "plus flag past the end", [](auto& b) { b[0x94 + 10] = 1; }, "Plus move flag past");
    expectInvalid(n, "plus flag high nibble", [](auto& b) { b[0x94 + 9] = 0x10; }, "Plus move flag past");
    expectNoIssue(n, "plus flag in range", [](auto& b) { b[0x94 + 9] = 0x08; }, "Plus move flag past");
    expectInvalid(n, "battle version", [](auto& b) { b[0xCF] = 43; }, "Battle version");
    expectNoIssue(n, "Sword/Shield battle version after a HOME visit", [](auto& b) { b[0xCF] = 44; }, "Battle version");
    expectInvalid(n, "obedience untraded", [](auto& b) { b[0x11F] = static_cast<uint8_t>(b[0x125] + 1); }, "Obedience level");
    expectInvalid(n, "alpha through HOME", [](auto& b) { b[0x23] = 1; b[0x127] = 1; b[0x48] = 200; }, "255 height and weight");
    expectNoIssue(n, "alpha native scalars", [](auto& b) { b[0x23] = 1; b[0x48] = 200; }, "255 height and weight");
    expectInvalid(n, "stat nature", [](auto& b) { b[0x21] = 25; }, "Stat nature");
    expectInvalid(n, "nature", [](auto& b) { b[0x20] = 25; }, "Nature is out of range");
    expectInvalid(n, "ability number", [](auto& b) { b[0x16] = static_cast<uint8_t>((b[0x16] & ~7) | 3); }, "Ability number");
    expectNoIssue(n, "ability of another species", [](auto& b) { PK8::write16(b, 0x14, 26); }, "Ability");
    expectInvalid(n, "duplicate move", [](auto& b) { PK8::write16(b, 0x74, PK8::read16(b, 0x72)); }, "is duplicated");
    {
        // A Plus flag the species' evolution line cannot learn, then one it can.
        const uint16_t species = Legality::Gen8::Data::nationalSpecies9(PK8::read16(n.plain, 0x08));
        auto flagEdit = [](uint16_t i) {
            return [i](std::vector<uint8_t>& b) {
                if (i < 264) b[0xD6 + (i >> 3)] |= static_cast<uint8_t>(1u << (i & 7));
                else b[0x94 + ((i - 264) >> 3)] |= static_cast<uint8_t>(1u << ((i - 264) & 7));
            };
        };
        uint16_t bad = 0, good = 0;
        bool foundBad = false, foundGood = false;
        for (uint16_t i = 0; i < Legality::Gen9a::Data::plusCount(); ++i) {
            const bool can = Legality::Gen9a::Data::plusLineCanLearn(species, i);
            if (!can && !foundBad) { bad = i; foundBad = true; }
            if (can && !foundGood) { good = i; foundGood = true; }
        }
        check(foundBad && foundGood, "fixture species has learnable and unlearnable Plus moves");
        check(!Legality::Gen9a::Data::plusLineCanLearn(species, Legality::Gen9a::Data::plusCount()), "Plus index past the end is never learnable");
        check(!Legality::Gen9a::Data::plusLineCanLearn(1001, 0), "species past 1000 learns no Plus move");
        expectInvalid(n, "plus move the line cannot learn", flagEdit(bad), "line cannot learn");
        expectNoIssue(n, "plus move the line learns", flagEdit(good), "line cannot learn");
    }
    expectNoIssue(n, "empty first move", [](auto& b) { PK8::write16(b, 0x72, 0); b[0x7A] = 0; }, "move slot");
    {
        // The species field holds the Scarlet/Violet internal id; find one whose national id is above 1000.
        uint16_t internal = 1;
        while (internal < 2000 && Legality::Gen8::Data::nationalSpecies9(internal) <= 1000) ++internal;
        expectInvalid(n, "species above 1000", [internal](auto& b) { PK8::write16(b, 0x08, internal); }, "does not exist in PA9");
    }
    expectInvalid(n, "ball", [](auto& b) { b[0x124] = 38; }, "cannot be held by a PA9");
    expectInvalid(n, "language 6", [](auto& b) { b[0xD5] = 6; }, "is not a language");
    expectNoIssue(n, "Spanish (LATAM) on a Z-A origin", [](auto& b) { b[0xD5] = 11; }, "Language");
    expectInvalid(n, "Spanish (LATAM) on a Scarlet origin", [](auto& b) { b[0xD5] = 11; b[0xCE] = 50; }, "does not exist in the origin game");
    {
        // HyperTrainingVerifier: level 50 or more.
        const auto personal = Legality::Gen9a::Data::personal(Legality::Gen8::Data::nationalSpecies9(PK8::read16(n.plain, 0x08)), n.plain[0x24]);
        check(personal.has_value(), "hyper training fixture species has personal data");
        const uint8_t growth = personal ? personal->growth : 0;
            expectInvalid(n, "PID equals EC", [](auto& b) { b[0xCE] = 50; PK8::write32(b, 0x1C, PK8::read32(b, 0x00)); },
                      "PID equals the encryption constant");
        expectNoIssue(n, "PID equals EC on a BD/SP origin", [](auto& b) { b[0xCE] = 48; PK8::write32(b, 0x1C, PK8::read32(b, 0x00)); },
                      "PID equals the encryption constant");
        expectInvalid(n, "Generation V PID", [](auto& b) { b[0xCE] = 21; PK8::write32(b, 0x1C, PK8::read32(b, 0x00) ^ 0x10000u); },
                      "Poke Transporter PID");
        {
            // The alpha flag must be some encounter's: a species of the format with no alpha encounter exists.
            int noAlpha = 0;
            for (uint16_t sp = 1; sp < 1100; ++sp)
                if (!Legality::ModernEncounter::alphaPossible(Legality::Modern::Format::PA9, sp, true)) ++noAlpha;
            check(noAlpha > 0, "some PA9 species line has no alpha encounter");
            check(Legality::ModernEncounter::alphaPossible(Legality::Modern::Format::PK8, 25, true), "only PA8/PA9 decide the alpha flag");
        }
    expectInvalid(n, "hyper training", [growth](auto& b) {
            PK8::write32(b, 0x10, Pokemon::getExpForLevel(100, growth));
            PK8::write32(b, 0x8C, (PK8::read32(b, 0x8C) & ~0x1Fu) | 31u);
            b[0x126] = 1;
        }, "Hyper Training on a perfect IV");
        expectInvalid(n, "hyper training below level 50", [growth](auto& b) {
            PK8::write32(b, 0x10, Pokemon::getExpForLevel(49, growth));
            b[0x126] = 2;
        }, "Hyper Training needs level 50 or more");
    }
    {
        uint16_t item = 1;
        while (item < 3000 && Legality::Gen9a::Data::heldItemAllowed(item)) ++item;
        expectInvalid(n, "held item", [item](auto& b) { PK8::write16(b, 0x0A, item); }, "not obtainable");
    }
    // Bad checksum and sanity.
    {
        auto bad = n.plain;
        bad[0x0A] ^= 1;
        const auto r = analyzePlain(n, bad);
        check(r.verdict() == Verdict::Invalid && hasIssue(r, Severity::Invalid, "checksum"), "checksum mismatch is Invalid");
        const auto s = analyzePlain(n, edit(n.plain, [](auto& b) { PK8::write16(b, 0x04, 1); }));
        check(s.verdict() == Verdict::Invalid && hasIssue(s, Severity::Invalid, "sanity"), "sanity field is Invalid");
    }
    // A PA9 in a non-Z-A game is a source-game Warning, not an Invalid Pokemon.
    {
        Record moved = n;
        moved.game = Game::SL;
        const auto r = analyzePlain(moved, moved.plain);
        check(r.verdict() == Verdict::Incomplete && hasIssue(r, Severity::Warning, "does not store PA9"),
              "PA9 in Scarlet is a source-game Warning");
    }
}

void testInterface(const std::vector<Record>& records) {
    check(Legality::Modern::profile(Format::PA9)->rulesImplemented, "PA9 has rules");
    Input in;
    in.format = Format::PA9;
    check(Legality::Modern::analyze(std::vector<uint8_t>(0x148, 0), in).issues.empty(), "all-zero record is an empty slot");
    for (size_t size : {size_t{0}, size_t{0x104}, size_t{0x147}, size_t{0x168}}) {
        const auto r = Legality::Modern::analyze(std::vector<uint8_t>(size, 1), in);
        check(r.coverage.structure == Legality::CoverageLevel::None && r.verdict() == Verdict::Incomplete,
              "wrong size " + std::to_string(size));
    }
    if (records.empty()) return;
    const auto r = analyzePlain(records.front(), edit(records.front().plain, [](auto& b) { PK8::write16(b, 0x08, 0); }));
    check(r.verdict() == Verdict::Incomplete && hasIssue(r, Severity::Warning, "Species is zero"), "species zero payload");
}

void testFuzz(const std::vector<Record>& records) {
    const char* env = std::getenv("GEN9A_FUZZ_ROUNDS");
    const size_t rounds = env ? std::strtoul(env, nullptr, 10) : 20000;
    std::mt19937 rng(9);
    for (size_t i = 0; i < rounds; ++i) {
        Input in;
        in.format = Format::PA9;
        in.game = static_cast<Game>(std::array<uint8_t, 4>{0, 52, 44, 50}[rng() % 4]);
        in.slot = static_cast<Slot>(rng() % 4);
        in.encrypted = (rng() & 1) != 0;
        std::vector<uint8_t> bytes;
        if (!records.empty() && (rng() % 3) != 0) {
            const auto& base = records[rng() % records.size()];
            bytes = base.plain;
            for (int k = 0, n = 1 + static_cast<int>(rng() % 6); k < n; ++k) bytes[8 + rng() % (bytes.size() - 8)] ^= static_cast<uint8_t>(1u << (rng() % 8));
            PK8::write16(bytes, 0x06, PK8::checksum(bytes));
            if (in.encrypted) bytes = PK8::encrypt(bytes);
        } else {
            bytes.resize(std::array<size_t, 4>{0x148, 0x158, 0x147, 0}[rng() % 4]);
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
    testFuzz(records);
    std::cout << "test_legality_gen9a: " << checks << " checks, " << failures << " failures\n";
    return failures == 0 ? 0 : 1;
}
