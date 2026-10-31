// Legends: Arceus (PA8) legality through Legality::Modern::analyze.
//
// tests/fixtures/gen67/pkhex8a_seed2_sample.txt is a stratified sample of the differential
// corpus (tools/pkhex_gen67_diff, `generate8a 2 4`): synthetic records PKHeX.Core built from its
// own Legends: Arceus encounter templates, records moved into Legends: Arceus through HOME and
// grown copies, each with PKHeX's verdict and a single-field mutation. One line per record:
//   8a <game> <box|party> <decrypted hex> <PKHeX VALID|INVALID> <NX Invalid|Incomplete> <label>
// Every line must keep its NX verdict class: a PKHeX-valid record is never Invalid, a record
// this analyzer caught stays caught, and nothing is ever "No problems found". Then rule edits
// of fixture records, the codec, the interface contract, malformed input, input immutability
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
#include "Legality/Gen8aLegality.h"
#include "Legality/ModernLegality.h"

namespace {

namespace PK8 = PokeVault::Integration::Gen8::PK8;
namespace Codec = Legality::Gen8a::Codec;
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
    const char* path = std::getenv("GEN8A_FIXTURE");
    std::ifstream in(path ? path : "tests/fixtures/gen67/pkhex8a_seed2_sample.txt");
    std::string line;
    while (std::getline(in, line)) {
        std::istringstream s(line);
        std::string fmt, game, slot, hex, pkhex, nx, label;
        if (!(s >> fmt >> game >> slot >> hex >> pkhex >> nx >> label) || fmt != "8a") continue;
        out.push_back({static_cast<Game>(std::stoi(game)), slot == "party" ? Slot::Party : Slot::Box, fromHex(hex),
                       pkhex == "VALID", nx == "Invalid", label});
    }
    return out;
}

Input inputFor(const Record& r, bool encrypted = false) {
    Input in;
    in.format = Format::PA8;
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
    PK8::write16(copy, 0x06, Codec::checksum(copy));
    return copy;
}

const Record* find(const std::vector<Record>& records, const std::function<bool(const Record&)>& pred) {
    for (const auto& r : records)
        if (pred(r)) return &r;
    return nullptr;
}

uint64_t tracker(const std::vector<uint8_t>& b) { return PK8::read32(b, 0x14D) | (uint64_t{PK8::read32(b, 0x151)} << 32); }

void writeFloat(std::vector<uint8_t>& b, size_t o, float f) {
    uint32_t bits;
    std::memcpy(&bits, &f, 4);
    PK8::write32(b, o, bits);
}

float readFloat(const std::vector<uint8_t>& b, size_t o) {
    const uint32_t bits = PK8::read32(b, o);
    float f;
    std::memcpy(&f, &bits, 4);
    return f;
}

void testFixture(const std::vector<Record>& records) {
    check(records.size() > 2000, "fixture loaded (" + std::to_string(records.size()) + " records)");
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
        const auto stored = Codec::encrypt(r.plain);
        check(Codec::decrypt(stored) == r.plain, r.label + ": codec round trip");
        const auto viaStored = Legality::Modern::analyze(stored, inputFor(r, true));
        check(viaStored.verdict() == v && viaStored.issues.size() == report.issues.size(), r.label + ": encrypted path differs");
    }
    check(valid > 300 && caught > 1000, "fixture covers both PKHeX-valid and caught records");
}

void testRuleEdits(const std::vector<Record>& records) {
    // A PKHeX-valid boxed Legends: Arceus native that never left the game: not an alpha, no
    // HOME tracker, not Enamorus.
    const Record* native = find(records, [](const Record& r) {
        return r.pkhexValid && r.slot == Slot::Box && r.plain[0xEE] == 47 && tracker(r.plain) == 0 &&
               (r.plain[0x16] & 0x20) == 0 && PK8::read16(r.plain, 0x08) != 905 && r.label.rfind("native8a:base", 0) == 0;
    });
    check(native != nullptr, "fixture has a boxed Legends: Arceus native base");
    if (!native) return;
    const Record& n = *native;
    const uint16_t species = PK8::read16(n.plain, 0x08);
    const auto personal = Legality::Gen8a::Data::personal(species, n.plain[0x24]);
    check(personal.has_value(), "native personal entry");
    if (!personal) return;
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
    expectInvalid(n, "ev total", [](auto& b) { b[0x26] = 252; b[0x27] = 252; b[0x28] = 7; }, "EV total is above 510");
    expectInvalid(n, "pp ups", [](auto& b) { b[0x86] = 1; }, "has PP Ups");
    expectInvalid(n, "pp", [](auto& b) { b[0x5C] = 99; }, "PP is above its maximum");
    expectInvalid(n, "noble", [](auto& b) { b[0x16] |= 0x40; }, "Noble flag");
    expectInvalid(n, "scale", [](auto& b) { b[0x52] = static_cast<uint8_t>(b[0x50] ^ 1); }, "Scale is not the height scalar");
    expectInvalid(n, "effort level", [](auto& b) { b[0xA6] = 11; }, "Effort Level is above the cap");
    expectInvalid(n, "alpha move", [](auto& b) { PK8::write16(b, 0x3E, Legality::Gen8a::Data::moveShop()[0]); }, "not an alpha");
    expectInvalid(n, "sociability", [](auto& b) { PK8::write32(b, 0x48, 5); }, "Sociability is not zero");
    expectInvalid(n, "gigantamax", [](auto& b) { b[0x16] |= 0x10; }, "Gigantamax");
    expectInvalid(n, "dynamax", [](auto& b) { b[0x98] = 3; }, "Dynamax level");
    expectInvalid(n, "tr flag", [](auto& b) { b[0x140] = 4; }, "TR flags are set");
    expectInvalid(n, "fullness", [](auto& b) { b[0xEC] = 10; }, "Fullness is not zero");
    expectInvalid(n, "enjoyment", [](auto& b) { b[0xED] = 1; }, "Enjoyment is not zero");
    expectInvalid(n, "battle version", [](auto& b) { b[0xEF] = 44; }, "Battle version");
    expectInvalid(n, "stat nature", [](auto& b) { b[0x21] = 25; }, "Stat nature");
    expectInvalid(n, "nature", [](auto& b) { b[0x20] = 25; }, "Nature is out of range");
    expectInvalid(n, "height", [](auto& b) { writeFloat(b, 0xAC, readFloat(b, 0xAC) + 0.5f); }, "Stored height");
    expectInvalid(n, "weight", [](auto& b) { writeFloat(b, 0xB0, readFloat(b, 0xB0) * 1.01f); }, "Stored weight");
    expectInvalid(n, "species above 905", [](auto& b) { PK8::write16(b, 0x08, 906); }, "does not exist in PA8");
    expectInvalid(n, "not in hisui", [](auto& b) { PK8::write16(b, 0x08, 1); b[0x24] = 0; }, "not present in Legends: Arceus");
    expectInvalid(n, "ball", [](auto& b) { b[0x137] = 38; }, "cannot be held by a PA8");
    expectInvalid(n, "language", [](auto& b) { b[0xF2] = 6; }, "is not a language");
    expectInvalid(n, "hyper training", [](auto& b) {
        PK8::write32(b, 0x94, (PK8::read32(b, 0x94) & ~0x1Fu) | 31u);
        b[0x13E] = 1;
    }, "Hyper Training on a perfect IV");
    expectInvalid(n, "first move empty", [](auto& b) { PK8::write16(b, 0x54, 0); }, "first move slot is empty");
    {
        uint16_t item = 1;
        while (item < 2000 && Legality::Gen8a::Data::heldItemAllowed(item)) ++item;
        expectInvalid(n, "held item", [item](auto& b) { PK8::write16(b, 0x0A, item); }, "not obtainable");
    }
    // Move shop: a purchase the species cannot make is Invalid, one it can make is not.
    {
        int forbidden = -1, permitted = -1;
        for (int i = 0; i < static_cast<int>(Legality::Gen8a::Data::moveShop().size()); ++i) {
            if (((personal->moveShop >> i) & 1) == 0 && forbidden < 0) forbidden = i;
            if (((personal->moveShop >> i) & 1) != 0 && permitted < 0) permitted = i;
        }
        if (forbidden >= 0)
            expectInvalid(n, "purchase", [forbidden](auto& b) { b[0x155 + forbidden / 8] |= static_cast<uint8_t>(1 << (forbidden % 8)); },
                          "purchased but the species cannot buy it");
        if (permitted >= 0) {
            expectNoIssue(n, "permitted purchase", [permitted](auto& b) { b[0x155 + permitted / 8] |= static_cast<uint8_t>(1 << (permitted % 8)); },
                          "Move shop");
            // An alpha with a masterable, mastered alpha move passes the alpha-move rules.
            const uint16_t move = Legality::Gen8a::Data::moveShop()[static_cast<size_t>(permitted)];
            expectNoIssue(n, "alpha move mastered", [&](auto& b) {
                b[0x16] |= 0x20;
                PK8::write16(b, 0x3E, move);
                b[0x15D + permitted / 8] |= static_cast<uint8_t>(1 << (permitted % 8));
            }, "Alpha move");
            expectInvalid(n, "alpha move not mastered", [&](auto& b) {
                b[0x16] |= 0x20;
                PK8::write16(b, 0x3E, move);
            }, "not marked as mastered");
        }
        if (forbidden >= 0) {
            const uint16_t move = Legality::Gen8a::Data::moveShop()[static_cast<size_t>(forbidden)];
            expectInvalid(n, "alpha move forbidden", [&](auto& b) { b[0x16] |= 0x20; PK8::write16(b, 0x3E, move); },
                          "species can master");
        }
    }
    // Sizes are only decided for a never-transferred, non-alpha native.
    expectNoIssue(n, "height with a HOME tracker", [](auto& b) { writeFloat(b, 0xAC, readFloat(b, 0xAC) + 0.5f); PK8::write32(b, 0x14D, 1); }, "Stored height");
    expectNoIssue(n, "height on an alpha", [](auto& b) { writeFloat(b, 0xAC, readFloat(b, 0xAC) + 0.5f); b[0x16] |= 0x20; }, "Stored height");
    expectNoIssue(n, "height on a transfer", [](auto& b) { writeFloat(b, 0xAC, readFloat(b, 0xAC) + 0.5f); b[0xEE] = 44; }, "Stored height");
    // Bad checksum and sanity.
    {
        auto bad = n.plain;
        bad[0x08] ^= 1;
        const auto r = analyzePlain(n, bad);
        check(r.verdict() == Verdict::Invalid && hasIssue(r, Severity::Invalid, "checksum"), "checksum mismatch is Invalid");
        const auto s = analyzePlain(n, edit(n.plain, [](auto& b) { PK8::write16(b, 0x04, 1); }));
        check(s.verdict() == Verdict::Invalid && hasIssue(s, Severity::Invalid, "sanity"), "sanity field is Invalid");
    }
    // A PA8 in a non-Legends: Arceus game is a source-game Warning, not an Invalid Pokemon.
    {
        Record moved = n;
        moved.game = Game::SW;
        const auto r = analyzePlain(moved, moved.plain);
        check(r.verdict() == Verdict::Incomplete && hasIssue(r, Severity::Warning, "does not store PA8"),
              "PA8 in Sword/Shield is a source-game Warning");
    }
    // The size helpers reproduce the stored values of the untouched native.
    const uint8_t hs = n.plain[0x50], ws = n.plain[0x51];
    check(Legality::Gen8a::heightAbsolute(personal->height, hs) == readFloat(n.plain, 0xAC), "height helper matches the native");
    check(Legality::Gen8a::weightAbsolute(personal->weight, hs, ws) == readFloat(n.plain, 0xB0), "weight helper matches the native");
}

void testInterface(const std::vector<Record>& records) {
    check(Legality::Modern::profile(Format::PA8)->rulesImplemented, "PA8 has rules");
    Input in;
    in.format = Format::PA8;
    check(Legality::Modern::analyze(std::vector<uint8_t>(0x168, 0), in).issues.empty(), "all-zero record is an empty slot");
    for (size_t size : {size_t{0}, size_t{0x148}, size_t{0x167}, size_t{0x179}}) {
        const auto r = Legality::Modern::analyze(std::vector<uint8_t>(size, 1), in);
        check(r.coverage.structure == Legality::CoverageLevel::None && r.verdict() == Verdict::Incomplete,
              "wrong size " + std::to_string(size));
    }
    check(Codec::decrypt(std::vector<uint8_t>(0x148, 0)).empty() && Codec::encrypt(std::vector<uint8_t>(5, 0)).empty(),
          "codec refuses other sizes");
    if (records.empty()) return;
    const auto r = analyzePlain(records.front(), edit(records.front().plain, [](auto& b) { PK8::write16(b, 0x08, 0); }));
    check(r.verdict() == Verdict::Incomplete && hasIssue(r, Severity::Warning, "Species is zero"), "species zero payload");
    // Party bytes: the stored part is the same record, the party tail is encrypted separately.
    auto party = records.front().plain;
    party.resize(Codec::PartySize, 0x11);
    check(Codec::decrypt(Codec::encrypt(party)) == party, "party codec round trip");
}

void testFuzz(const std::vector<Record>& records) {
    const char* env = std::getenv("GEN8A_FUZZ_ROUNDS");
    const size_t rounds = env ? std::strtoul(env, nullptr, 10) : 20000;
    std::mt19937 rng(8);
    for (size_t i = 0; i < rounds; ++i) {
        Input in;
        in.format = Format::PA8;
        in.game = static_cast<Game>(std::array<uint8_t, 4>{0, 47, 44, 50}[rng() % 4]);
        in.slot = static_cast<Slot>(rng() % 4);
        in.encrypted = (rng() & 1) != 0;
        std::vector<uint8_t> bytes;
        if (!records.empty() && (rng() % 3) != 0) {
            const auto& base = records[rng() % records.size()];
            bytes = base.plain;
            for (int k = 0, n = 1 + static_cast<int>(rng() % 6); k < n; ++k) bytes[8 + rng() % (bytes.size() - 8)] ^= static_cast<uint8_t>(1u << (rng() % 8));
            PK8::write16(bytes, 0x06, Codec::checksum(bytes));
            if (in.encrypted) bytes = Codec::encrypt(bytes);
        } else {
            bytes.resize(std::array<size_t, 4>{0x168, 0x178, 0x167, 0}[rng() % 4]);
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
    std::cout << "test_legality_gen8a: " << checks << " checks, " << failures << " failures\n";
    return failures == 0 ? 0 : 1;
}
