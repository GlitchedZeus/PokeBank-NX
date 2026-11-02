// Let's Go Pikachu/Eevee (PB7) legality through Legality::Modern::analyze.
//
// tests/fixtures/gen67/pkhex7b_seed2_sample.txt is a stratified sample of the differential
// corpus (tools/pkhex_gen67_diff, `generate7b 2 4`): synthetic records PKHeX.Core built from its
// own Let's Go and GO Park encounter templates and grown copies (Awakening Values raised, CP
// recalculated), each with PKHeX's verdict and a single-field mutation. One line per record:
//   7b <game> <box|party> <decrypted hex> <PKHeX VALID|INVALID> <NX Invalid|Incomplete> <label>
// Every line must keep its NX verdict class: a PKHeX-valid record is never Invalid, a record
// this analyzer caught stays caught, and nothing is ever "No problems found". Then rule edits
// of fixture records, the interface contract, malformed input, encryption round trips, input
// immutability and a deterministic fuzz pass (run under ASan/UBSan by host-sanitize).
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

#include "Integration/Gen6/Gen6PK6.h"
#include "Legality/Gen7bLegality.h"
#include "Legality/ModernLegality.h"
#include "Pokemon/Experience.h"

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
    Game game;
    Slot slot;
    std::vector<uint8_t> plain;
    bool pkhexValid;
    bool nxInvalid;
    std::string label;
};

std::vector<Record> loadFixture() {
    std::vector<Record> out;
    const char* path = std::getenv("GEN7B_FIXTURE");
    std::ifstream in(path ? path : "tests/fixtures/gen67/pkhex7b_seed2_sample.txt");
    std::string line;
    while (std::getline(in, line)) {
        std::istringstream s(line);
        std::string fmt, game, slot, hex, pkhex, nx, label;
        if (!(s >> fmt >> game >> slot >> hex >> pkhex >> nx >> label) || fmt != "7b") continue;
        out.push_back({static_cast<Game>(std::stoi(game)), slot == "party" ? Slot::Party : Slot::Box, fromHex(hex),
                       pkhex == "VALID", nx == "Invalid", label});
    }
    return out;
}

Input inputFor(const Record& r, bool encrypted = false) {
    Input in;
    in.format = Format::PB7;
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
    PK6::write16(copy, 0x06, PK6::checksum(copy));
    return copy;
}

const Record* find(const std::vector<Record>& records, const std::function<bool(const Record&)>& pred) {
    for (const auto& r : records)
        if (pred(r)) return &r;
    return nullptr;
}

bool starter(const Record& r) {
    const uint16_t s = PK6::read16(r.plain, 0x08);
    const uint8_t f = r.plain[0x1D] >> 3;
    return (s == 25 && f == 8) || (s == 133 && f == 1);
}

void testFixture(const std::vector<Record>& records) {
    check(records.size() > 300, "fixture loaded (" + std::to_string(records.size()) + " records)");
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
        const auto viaStored = Legality::Modern::analyze(PK6::encrypt(r.plain), inputFor(r, true));
        check(viaStored.verdict() == v && viaStored.issues.size() == report.issues.size(), r.label + ": encrypted path differs");
    }
    check(valid > 100 && caught > 150, "fixture covers both PKHeX-valid and caught records");
}

void testRuleEdits(const std::vector<Record>& records) {
    // A PKHeX-valid boxed Let's Go native, not the partner Pikachu/Eevee and not Meltan's line.
    const Record* native = find(records, [](const Record& r) {
        const uint16_t s = PK6::read16(r.plain, 0x08);
        return r.pkhexValid && r.slot == Slot::Box && !starter(r) && s != 808 && s != 809 &&
               (r.plain[0xDF] == 42 || r.plain[0xDF] == 43) && r.plain[0x78] == 0 && r.plain[0x79] == 0;
    });
    check(native != nullptr, "fixture has a boxed Let's Go native base");
    if (!native) return;
    const Record& n = *native;
    auto expectInvalid = [&](const Record& base, const char* name, const std::function<void(std::vector<uint8_t>&)>& f, const char* fragment) {
        const auto r = analyzePlain(base, edit(base.plain, f));
        check(r.verdict() == Verdict::Invalid && hasIssue(r, Severity::Invalid, fragment),
              std::string(name) + ": expected Invalid '" + fragment + "', got " + describe(r));
    };
    auto expectNotInvalid = [&](const Record& base, const char* name, const std::function<void(std::vector<uint8_t>&)>& f) {
        const auto r = analyzePlain(base, edit(base.plain, f));
        check(r.verdict() == Verdict::Incomplete, std::string(name) + ": expected Incomplete, got " + describe(r));
    };
    expectInvalid(n, "ev", [](auto& b) { b[0x1E] = 4; }, "have no EVs");
    expectInvalid(n, "awakening above 200", [](auto& b) { b[0x24] = 201; }, "Awakening Value is above 200");
    expectInvalid(n, "cp", [](auto& b) { PK6::write16(b, 0xFE, static_cast<uint16_t>(PK6::read16(b, 0xFE) + 1)); }, "Stored CP");
    expectInvalid(n, "awakening without cp", [](auto& b) { b[0x25] = static_cast<uint8_t>(b[0x25] < 200 ? b[0x25] + 1 : 199); }, "Stored CP");
    expectInvalid(n, "spirit", [](auto& b) { b[0xAC] = 99; }, "Spirit is not 100");
    expectInvalid(n, "mood", [](auto& b) { b[0xAD] = 101; }, "Mood is not 100");
    expectInvalid(n, "received time", [](auto& b) { b[0xCE] = 24; }, "Received time");
    expectInvalid(n, "received date", [](auto& b) { b[0xCD] = static_cast<uint8_t>(b[0xCD] == 1 ? 2 : 1); }, "differs from the met date");
    expectInvalid(n, "received year", [](auto& b) { b[0xCB] = 61; b[0xD4] = 61; }, "not a valid date");
    expectInvalid(n, "height", [](auto& b) {
        float f;
        const uint32_t bits = PK6::read32(b, 0x2C);
        std::memcpy(&f, &bits, 4);
        f += 0.5f;
        uint32_t out;
        std::memcpy(&out, &f, 4);
        PK6::write32(b, 0x2C, out);
    }, "Stored height");
    expectInvalid(n, "height scalar", [](auto& b) { b[0x3A] = static_cast<uint8_t>(b[0x3A] ^ 0x80); }, "Stored height");
    expectInvalid(n, "origin", [](auto& b) { b[0xDF] = 30; }, "cannot be stored in a PB7");
    expectInvalid(n, "go without go park", [](auto& b) { b[0xDF] = 34; PK6::write16(b, 0xDA, 3); }, "cannot be stored in a PB7");
    expectInvalid(n, "not in let's go", [](auto& b) { PK6::write16(b, 0x08, 152); }, "not present in Let's Go");
    expectInvalid(n, "species", [](auto& b) { PK6::write16(b, 0x08, 810); }, "does not exist in PB7");
    expectInvalid(n, "egg", [](auto& b) { PK6::write32(b, 0x74, PK6::read32(b, 0x74) | 0x40000000u); }, "has no eggs");
    expectInvalid(n, "pid==ec", [](auto& b) { PK6::write32(b, 0x18, PK6::read32(b, 0x00)); }, "PID equals the encryption constant");
    expectInvalid(n, "ball", [](auto& b) { b[0xDC] = 27; }, "cannot be held by a PB7");
    // No Let's Go encounter gives a Dream Ball (25), whatever the species.
    expectInvalid(n, "dream ball", [](auto& b) { b[0xDC] = 25; }, "not possible for any encounter");
    expectInvalid(n, "hyper training", [](auto& b) { b[0xDE] = 1; }, "needs level 100");
    {
        // At level 100 a gold bottle cap (all six flags) may cover a perfect IV; one silver cap may not.
        const auto growth = Legality::Gen7b::Data::personal(PK6::read16(n.plain, 0x08), n.plain[0x1D] >> 3)->growth;
        auto at100 = [&](uint8_t flags) {
            return analyzePlain(n, edit(n.plain, [&](auto& b) {
                PK6::write32(b, 0x10, Pokemon::getExpForLevel(100, growth));
                PK6::write32(b, 0x74, (PK6::read32(b, 0x74) & ~0x1Fu) | 31u);  // HP IV 31
                b[0xDE] = flags;
            }));
        };
        check(!hasIssue(at100(0x3F), Severity::Invalid, "perfect IV"), "gold bottle cap over a perfect IV is not Invalid");
        check(hasIssue(at100(0x01), Severity::Invalid, "perfect IV"), "silver bottle cap on a perfect IV is Invalid");
    }
    {
        uint16_t item = 1;
        while (item < 2000 && Legality::Gen7b::Data::heldItemAllowed(item)) ++item;
        expectInvalid(n, "held item", [item](auto& b) { PK6::write16(b, 0x0A, item); }, "not obtainable");
    }
    // Values a legitimate record can hold stay undecided.
    {
        Record party = n;
        party.slot = Slot::Party;
        const auto r = analyzePlain(party, edit(n.plain, [](auto& b) { b[0xAC] = 70; b[0xAD] = 130; }));
        check(r.verdict() == Verdict::Incomplete, "spirit/mood in the party are not decided: " + describe(r));
        Record unknown = n;
        unknown.slot = Slot::Unknown;
        const auto u = analyzePlain(unknown, edit(n.plain, [](auto& b) { b[0xAC] = 70; }));
        check(!hasIssue(u, Severity::Invalid, "Spirit"), "spirit with an unknown slot is not decided");
    }
    expectNotInvalid(n, "traded record keeps another received date", [](auto& b) {
        b[0x78] = 'A';
        b[0xCD] = static_cast<uint8_t>(b[0xCD] == 1 ? 2 : 1);
    });
    if (const Record* partner = find(records, [](const Record& r) { return r.pkhexValid && starter(r); })) {
        const auto r = analyzePlain(*partner, edit(partner->plain, [](auto& b) {
            PK6::write16(b, 0xFE, static_cast<uint16_t>(PK6::read16(b, 0xFE) + 7));
            b[0xAC] = 50;
        }));
        check(!hasIssue(r, Severity::Invalid, "CP") && !hasIssue(r, Severity::Invalid, "Spirit"),
              "partner Pikachu/Eevee keeps its own CP and spirit: " + describe(r));
    }
    // Bad checksum is an Invalid structure finding.
    {
        auto bad = n.plain;
        bad[0x08] ^= 1;
        const auto r = analyzePlain(n, bad);
        check(r.verdict() == Verdict::Invalid && hasIssue(r, Severity::Invalid, "checksum"), "checksum mismatch is Invalid");
    }
    // A PB7 in a non-Let's Go game is a source-game Warning, not an Invalid Pokemon.
    {
        Record moved = n;
        moved.game = Game::SW;
        const auto r = analyzePlain(moved, moved.plain);
        check(r.verdict() == Verdict::Incomplete && hasIssue(r, Severity::Warning, "does not store PB7"),
              "PB7 in Sword/Shield is a source-game Warning");
    }
}

void testInterface(const std::vector<Record>& records) {
    check(Legality::Modern::profile(Format::PB7)->rulesImplemented, "PB7 has rules");
    Input in;
    in.format = Format::PB7;
    check(Legality::Modern::analyze(std::vector<uint8_t>(0x104, 0), in).issues.empty(), "all-zero record is an empty slot");
    for (size_t size : {size_t{0}, size_t{0xE8}, size_t{0x103}, size_t{0x148}}) {
        const auto r = Legality::Modern::analyze(std::vector<uint8_t>(size, 1), in);
        check(r.coverage.structure == Legality::CoverageLevel::None && r.verdict() == Verdict::Incomplete,
              "wrong size " + std::to_string(size));
    }
    if (records.empty()) return;
    const auto r = analyzePlain(records.front(), edit(records.front().plain, [](auto& b) { PK6::write16(b, 0x08, 0); }));
    check(r.verdict() == Verdict::Incomplete && hasIssue(r, Severity::Warning, "Species is zero"), "species zero payload");
}

void testHelpers() {
    using namespace Legality::Gen7b;
    const auto pikachu = Data::personal(25, 0);
    check(pikachu && pikachu->present && pikachu->baseStats[0] == 35, "Pikachu personal");
    check(Data::personal(152, 0) && !Data::personal(152, 0)->present, "Chikorita absent from Let's Go");
    check(Data::maxSpecies() == 809 && Data::maxMove() == 742 && Data::maxBall() == 26, "limits");
    // PB7.GetHeightRatio at the scalar extremes: 0.6 and 1.4 (single precision).
    check(heightAbsolute(100, 0) == 0.6f * 100.0f, "height ratio at scalar 0");
    check(weightAbsolute(100, 0, 0) > 0.0f, "weight ratio");
}

void testFuzz(const std::vector<Record>& records) {
    const char* env = std::getenv("GEN7B_FUZZ_ROUNDS");
    const size_t rounds = env ? std::strtoul(env, nullptr, 10) : 20000;
    std::mt19937 rng(7);
    for (size_t i = 0; i < rounds; ++i) {
        Input in;
        in.format = Format::PB7;
        in.game = static_cast<Game>(std::array<uint8_t, 4>{0, 42, 43, 44}[rng() % 4]);
        in.slot = static_cast<Slot>(rng() % 4);
        in.encrypted = (rng() & 1) != 0;
        std::vector<uint8_t> bytes;
        if (!records.empty() && (rng() % 3) != 0) {
            const auto& base = records[rng() % records.size()];
            bytes = base.plain;
            for (int k = 0, n = 1 + static_cast<int>(rng() % 6); k < n; ++k) bytes[8 + rng() % (bytes.size() - 8)] ^= static_cast<uint8_t>(1u << (rng() % 8));
            PK6::write16(bytes, 0x06, PK6::checksum(bytes));
            if (in.encrypted) bytes = PK6::encrypt(bytes);
        } else {
            bytes.resize(std::array<size_t, 4>{0x104, 0xE8, 0x103, 0}[rng() % 4]);
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
    std::cout << "test_legality_gen7b: " << checks << " checks, " << failures << " failures\n";
    return failures == 0 ? 0 : 1;
}
