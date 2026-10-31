// Generation VIII Sword/Shield (PK8) and Brilliant Diamond/Shining Pearl (PB8) legality
// through Legality::Modern::analyze.
//
// tests/fixtures/gen67/pkhex8_seed2_sample.txt is a stratified sample of the differential
// corpus (tools/pkhex_gen67_diff, `generate8 2 4`): synthetic records PKHeX.Core built from
// its own Sword/Shield encounter templates, records it moved into Sword/Shield through HOME
// (Gen III-VII, Virtual Console, Let's Go, BD/SP, Legends: Arceus, Scarlet/Violet) and grown
// copies, each with PKHeX's verdict and a single-field mutation.
// tests/fixtures/gen67/pkhexb8_seed2_sample.txt is the same for PB8 (`generate8b 2 4`): BD/SP
// encounters, records moved into BD/SP through HOME (Gen III-VII, Virtual Console, Let's Go,
// Sword/Shield, Legends: Arceus, Scarlet/Violet) and grown copies. One line per record:
//   <8|8b> <game> <box|party> <decrypted hex> <PKHeX VALID|INVALID> <NX Invalid|Incomplete> <label>
// Every line must keep its NX verdict class: a PKHeX-valid record is never Invalid, a
// record this analyzer caught stays caught, and nothing is ever "No problems found".
// Then rule edits of fixture records, the interface contract, malformed input, encryption
// round trips, input immutability and a deterministic fuzz pass (run under ASan/UBSan by
// host-sanitize).
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <vector>

#include "Integration/Gen8/Gen8PK8.h"
#include "Legality/Gen8Legality.h"
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
    Format format;
    Game game;
    Slot slot;
    std::vector<uint8_t> plain;
    bool pkhexValid;
    bool nxInvalid;
    std::string label;
};

std::vector<Record> loadFixture(const char* env, const char* fallback, const std::string& want) {
    std::vector<Record> out;
    const char* path = std::getenv(env);
    std::ifstream in(path ? path : fallback);
    std::string line;
    while (std::getline(in, line)) {
        std::istringstream s(line);
        std::string fmt, game, slot, hex, pkhex, nx, label;
        if (!(s >> fmt >> game >> slot >> hex >> pkhex >> nx >> label) || fmt != want) continue;
        out.push_back({fmt == "8b" ? Format::PB8 : Format::PK8, static_cast<Game>(std::stoi(game)),
                       slot == "party" ? Slot::Party : Slot::Box, fromHex(hex), pkhex == "VALID", nx == "Invalid", label});
    }
    return out;
}

Input inputFor(const Record& r, bool encrypted = false) {
    Input in;
    in.format = r.format;
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

void testFixture(const std::vector<Record>& records, const std::string& name) {
    check(records.size() > 1000, name + " fixture loaded (" + std::to_string(records.size()) + " records)");
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
        const auto viaStored = Legality::Modern::analyze(PK8::encrypt(r.plain), inputFor(r, true));
        check(viaStored.verdict() == v && viaStored.issues.size() == report.issues.size(), r.label + ": encrypted path differs");
    }
    check(valid > 300 && caught > 500, name + " fixture covers both PKHeX-valid and caught records");
}

const Record* find(const std::vector<Record>& records, const std::function<bool(const Record&)>& pred) {
    for (const auto& r : records)
        if (pred(r)) return &r;
    return nullptr;
}

bool startsWith(const std::string& s, const char* prefix) { return s.rfind(prefix, 0) == 0; }

void testRuleEdits(const std::vector<Record>& records) {
    // PKHeX-valid records from the fixture, never a personal save: a Sword/Shield native
    // with a non-Zacian species and a 3DS Gen VII record moved through HOME.
    const Record* native = find(records, [](const Record& r) {
        const uint16_t s = PK8::read16(r.plain, 0x08);
        return r.pkhexValid && startsWith(r.label, "native8:base") && s != 888 && s != 889 && s != 890 &&
               ((PK8::read32(r.plain, 0x8C) >> 30) & 1) == 0 && r.plain[0x20] != 0;
    });
    const Record* home7 = find(records, [](const Record& r) { return r.pkhexValid && startsWith(r.label, "home7:base"); });
    check(native && home7, "fixture has native and HOME PK8 bases");
    if (!native || !home7) return;
    auto expectInvalid = [&](const Record& base, const char* name, const std::function<void(std::vector<uint8_t>&)>& f, const char* fragment) {
        const auto r = analyzePlain(base, edit(base.plain, f));
        check(r.verdict() == Verdict::Invalid && hasIssue(r, Severity::Invalid, fragment),
              std::string(name) + ": expected Invalid '" + fragment + "', got " + describe(r));
    };
    auto expectNotInvalid = [&](const Record& base, const char* name, const std::function<void(std::vector<uint8_t>&)>& f) {
        const auto r = analyzePlain(base, edit(base.plain, f));
        check(r.verdict() == Verdict::Incomplete, std::string(name) + ": expected Incomplete, got " + describe(r));
    };
    const Record& n = *native;
    expectInvalid(n, "ev total", [](auto& b) { for (int i = 0; i < 6; ++i) b[0x26 + i] = 100; }, "EV total is above 510");
    expectInvalid(n, "nature", [](auto& b) { b[0x20] = 25; }, "Nature is out of range");
    expectInvalid(n, "duplicate move", [](auto& b) { PK8::write16(b, 0x74, PK8::read16(b, 0x72)); }, "is duplicated");
    expectInvalid(n, "move id", [](auto& b) { PK8::write16(b, 0x72, 827); }, "does not exist");
    expectInvalid(n, "pp", [](auto& b) { b[0x7A] = 99; }, "PP is above its maximum");
    expectInvalid(n, "pid==ec", [](auto& b) { PK8::write32(b, 0x1C, PK8::read32(b, 0x00)); }, "PID equals the encryption constant");
    expectInvalid(n, "ability number", [](auto& b) { b[0x16] = static_cast<uint8_t>((b[0x16] & ~7) | 3); }, "Ability number is not 1, 2 or 4");
    expectInvalid(n, "ball", [](auto& b) { b[0x124] = 27; }, "cannot be held by a PK8");
    expectInvalid(n, "language", [](auto& b) { b[0xE2] = 6; }, "is not a language");
    expectInvalid(n, "origin", [](auto& b) { b[0xDE] = 50; }, "cannot be stored in a PK8");
    expectInvalid(n, "origin z-a", [](auto& b) { b[0xDE] = 52; }, "newer than PK8");
    expectInvalid(n, "species", [](auto& b) { PK8::write16(b, 0x08, 899); }, "does not exist in PK8");
    expectInvalid(n, "not in sword/shield", [](auto& b) { PK8::write16(b, 0x08, 13); }, "not present in Sword/Shield");
    expectInvalid(n, "held item", [](auto& b) { PK8::write16(b, 0x0A, 1608); }, "not obtainable");
    expectInvalid(n, "dynamax 11", [](auto& b) { b[0x90] = 11; }, "Dynamax level 11");
    expectInvalid(n, "stat nature", [](auto& b) { b[0x21] = b[0x20] == 6 ? 0 : 6; }, "neither the nature nor a mint");
    expectInvalid(n, "battle version", [](auto& b) { b[0xDF] = 43; }, "Battle version 43");
    expectInvalid(n, "battle version on a native", [](auto& b) { b[0xDF] = 44; }, "Battle version 44");
    expectInvalid(n, "sociability", [](auto& b) { PK8::write32(b, 0x48, 256); }, "Sociability is above 255");
    expectInvalid(n, "hyper training below 50", [](auto& b) { b[0x126] = 1; PK8::write32(b, 0x10, 0); b[0x125] &= 0x80; }, "Hyper Training needs level 50");
    expectInvalid(n, "level below met", [](auto& b) { PK8::write32(b, 0x10, 0); b[0x125] = static_cast<uint8_t>((b[0x125] & 0x80) | 50); }, "below the met level");
    {
        // A TR the species cannot learn.
        const uint16_t s = PK8::read16(n.plain, 0x08);
        const uint8_t f = n.plain[0x24];
        int tr = -1;
        for (uint8_t i = 0; i < Legality::Gen8::Data::techRecordCount() && tr < 0; ++i)
            if (!Legality::Gen8::Data::canLearnTechRecord(s, f, i)) tr = i;
        check(tr >= 0, "native base has an unlearnable TR");
        if (tr >= 0)
            expectInvalid(n, "tr flag", [tr](auto& b) { b[0x127 + (tr >> 3)] |= static_cast<uint8_t>(1u << (tr & 7)); }, "is flagged but the species cannot learn it");
    }
    // Values a legitimate record can hold stay undecided.
    expectNotInvalid(n, "mint stat nature", [](auto& b) { b[0x21] = b[0x20] == 1 ? 2 : 1; });
    expectNotInvalid(n, "dynamax 10", [](auto& b) { b[0x90] = 10; });
    expectNotInvalid(*home7, "battle version on a Gen VII record", [](auto& b) { b[0xDF] = 45; });
    // Egg rules, on the native record turned into an egg (egg and nickname flags set).
    Record egg = n;
    egg.plain = edit(n.plain, [](auto& b) {
        PK8::write32(b, 0x8C, PK8::read32(b, 0x8C) | 0xC0000000u);
        b[0x90] = 0;
        PK8::write32(b, 0x48, 0);
    });
    expectInvalid(egg, "egg dynamax", [](auto& b) { b[0x90] = 1; }, "Dynamax level 1");
    expectInvalid(egg, "egg nickname flag", [](auto& b) { PK8::write32(b, 0x8C, PK8::read32(b, 0x8C) & 0x7FFFFFFFu); }, "nickname flag");
    expectInvalid(egg, "egg sociability", [](auto& b) { PK8::write32(b, 0x48, 1); }, "egg cannot have sociability");
    expectInvalid(egg, "egg item", [](auto& b) { PK8::write16(b, 0x0A, 1); }, "egg cannot hold an item");
    // Transfer rules on the HOME record.
    expectInvalid(*home7, "pid==ec transfer", [](auto& b) { PK8::write32(b, 0x1C, PK8::read32(b, 0x00)); }, "PID equals the encryption constant");

    // Bad checksum and bad sanity are Invalid structure findings.
    {
        auto bad = n.plain;
        bad[0x08] ^= 1;
        const auto r = analyzePlain(n, bad);
        check(r.verdict() == Verdict::Invalid && hasIssue(r, Severity::Invalid, "checksum"), "checksum mismatch is Invalid");
        const auto rs = analyzePlain(n, edit(n.plain, [](auto& b) { b[0x04] = 1; }));
        check(rs.verdict() == Verdict::Invalid && hasIssue(rs, Severity::Invalid, "sanity"), "sanity flag is Invalid");
    }
    // A PK8 in a non-Sword/Shield game is a source-game Warning, not an Invalid Pokemon.
    {
        Record moved = n;
        moved.game = Game::BD;
        const auto r = analyzePlain(moved, moved.plain);
        check(r.verdict() == Verdict::Incomplete && hasIssue(r, Severity::Warning, "does not store PK8"),
              "PK8 in BD/SP is a source-game Warning");
    }
}

// PB8: MiscVerifierPB8 fields, BD/SP tables and the format boundary, on PKHeX-valid BD/SP
// records from the fixture.
void testBdsp(const std::vector<Record>& records) {
    check(Legality::Modern::profile(Format::PB8)->rulesImplemented, "PB8 has rules");
    const Record* native = find(records, [](const Record& r) {
        const uint16_t s = PK8::read16(r.plain, 0x08);
        return r.pkhexValid && startsWith(r.label, "native8b:base") && s != 327 && s != 290 &&
               ((PK8::read32(r.plain, 0x8C) >> 30) & 1) == 0 && (r.plain[0xDE] == 48 || r.plain[0xDE] == 49);
    });
    const Record* home = find(records, [](const Record& r) {
        return r.pkhexValid && startsWith(r.label, "bdhome7:base") && ((PK8::read32(r.plain, 0x8C) >> 30) & 1) == 0;
    });
    check(native && home, "fixture has native and HOME PB8 bases");
    if (!native || !home) return;
    auto expectInvalid = [&](const Record& base, const char* name, const std::function<void(std::vector<uint8_t>&)>& f, const char* fragment) {
        const auto r = analyzePlain(base, edit(base.plain, f));
        check(r.verdict() == Verdict::Invalid && hasIssue(r, Severity::Invalid, fragment),
              std::string("PB8 ") + name + ": expected Invalid '" + fragment + "', got " + describe(r));
    };
    auto expectNotInvalid = [&](const Record& base, const char* name, const std::function<void(std::vector<uint8_t>&)>& f) {
        const auto r = analyzePlain(base, edit(base.plain, f));
        check(r.verdict() == Verdict::Incomplete, std::string("PB8 ") + name + ": expected Incomplete, got " + describe(r));
    };
    const Record& n = *native;
    expectInvalid(n, "sociability", [](auto& b) { PK8::write32(b, 0x48, 1); }, "has no sociability");
    expectInvalid(n, "dpr illegal flag", [](auto& b) { b[0x52] = 1; }, "illegal-record flag");
    expectInvalid(n, "gigantamax", [](auto& b) { b[0x16] |= 0x10; }, "cannot Gigantamax");
    expectInvalid(n, "dynamax", [](auto& b) { b[0x90] = 1; }, "no Dynamax level");
    expectInvalid(n, "tr flag", [](auto& b) { b[0x127] = 1; }, "cannot hold TR flags");
    expectInvalid(n, "battle version", [](auto& b) { b[0xDF] = 44; }, "Battle version 44");
    expectInvalid(n, "hyper training below 100", [](auto& b) { b[0x126] = 1; }, "needs level 100");
    expectInvalid(n, "species above 493", [](auto& b) { PK8::write16(b, 0x08, 494); }, "does not exist in PB8");
    expectInvalid(n, "ball 28", [](auto& b) { b[0x124] = 38; }, "cannot be held by a PB8");
    expectInvalid(*home, "pid==ec", [](auto& b) { PK8::write32(b, 0x1C, PK8::read32(b, 0x00)); }, "PID equals the encryption constant");
    expectInvalid(n, "spinda tracker", [](auto& b) { PK8::write16(b, 0x08, 327); b[0x24] = 0; PK8::write32(b, 0x135, 1); }, "no HOME tracker");
    expectInvalid(*home, "nincada moved in", [](auto& b) { PK8::write16(b, 0x08, 290); b[0x24] = 0; }, "cannot be moved into BD/SP");
    expectInvalid(*home, "origin 53", [](auto& b) { b[0xDE] = 53; }, "cannot be stored in a PB8");
    // Values a legitimate BD/SP record can hold stay undecided.
    expectNotInvalid(n, "battle version zero", [](auto& b) { b[0xDF] = 0; });
    expectNotInvalid(*home, "battle version on a moved record", [](auto& b) { b[0xDF] = 44; });
    // PID equal to the EC on a BD/SP native may be a fixed-EC WB8 gift; on a Sword/Shield
    // record with EC 0 it may be a HOME gift (WC8.IsHOMEGift). Neither is decided here.
    expectNotInvalid(n, "pid==ec native", [](auto& b) { PK8::write32(b, 0x1C, PK8::read32(b, 0x00)); });
    expectNotInvalid(*home, "ec 0 pid 0 from sword", [](auto& b) { PK8::write32(b, 0x00, 0); PK8::write32(b, 0x1C, 0); b[0xDE] = 44; b[0xDF] = 0; });
    // Egg TR bytes are not checked (PKHeX skips eggs).
    Record egg = n;
    egg.plain = edit(n.plain, [](auto& b) { PK8::write32(b, 0x8C, PK8::read32(b, 0x8C) | 0xC0000000u); });
    {
        const auto r = analyzePlain(egg, edit(egg.plain, [](auto& b) { b[0x127] = 1; }));
        check(!hasIssue(r, Severity::Invalid, "TR flags"), "PB8 egg TR bytes are not flagged: " + describe(r));
    }
    // A PB8 in Sword/Shield is a source-game Warning.
    {
        Record moved = n;
        moved.game = Game::SW;
        const auto r = analyzePlain(moved, moved.plain);
        check(r.verdict() == Verdict::Incomplete && hasIssue(r, Severity::Warning, "does not store PB8"),
              "PB8 in Sword/Shield is a source-game Warning");
    }
    namespace D = Legality::Gen8::Data;
    using Legality::Gen8::Context;
    check(D::maxSpecies(Context::BDSP) == 493 && D::maxBall(Context::BDSP) == 37, "BD/SP limits");
    check(D::personal(387, 0, Context::BDSP) && !D::personal(494, 0, Context::BDSP), "BD/SP personal table ends at Arceus");
    check(D::presentInGame(13, 0, Context::BDSP), "Weedle is present in BD/SP");
}

void testInterface(const std::vector<Record>& records) {
    check(Legality::Modern::profile(Format::PK8)->rulesImplemented, "PK8 has rules");
    Input in;
    in.format = Format::PK8;
    check(Legality::Modern::analyze(std::vector<uint8_t>(0x148, 0), in).issues.empty(), "all-zero record is an empty slot");
    for (size_t size : {size_t{0}, size_t{0xE8}, size_t{0x147}, size_t{0x168}}) {
        const auto r = Legality::Modern::analyze(std::vector<uint8_t>(size, 1), in);
        check(r.coverage.structure == Legality::CoverageLevel::None && r.verdict() == Verdict::Incomplete,
              "wrong size " + std::to_string(size));
    }
    if (records.empty()) return;
    // Stored (0x148) and party (0x158) forms of one record agree on the verdict.
    const Record* party = find(records, [](const Record& r) { return r.plain.size() == PK8::PartySize && !r.pkhexValid && r.nxInvalid; });
    if (party) {
        std::vector<uint8_t> stored(party->plain.begin(), party->plain.begin() + PK8::StoredSize);
        Record boxed = *party;
        boxed.slot = Slot::Box;
        check(analyzePlain(boxed, stored).verdict() == Verdict::Invalid, "stored copy of a caught party record is Invalid");
    }
    // Species zero with a payload is ambiguous, not a blank slot.
    {
        const auto r = analyzePlain(records.front(), edit(records.front().plain, [](auto& b) { PK8::write16(b, 0x08, 0); }));
        check(r.verdict() == Verdict::Incomplete && hasIssue(r, Severity::Warning, "Species is zero"), "species zero payload");
    }
    if (const Record* base = find(records, [](const Record& r) { return r.pkhexValid; })) {
        Input t = inputFor(*base);
        t.trainer.present = true;
        t.trainer.tid = PK8::read16(base->plain, 0x0C);
        t.trainer.sid = PK8::read16(base->plain, 0x0E);
        t.trainer.ot = {'Z', 'z', 0};
        check(hasIssue(Legality::Modern::analyze(base->plain, t), Severity::Warning, "OT name or gender"), "same ID, other OT");
        t.trainer.present = false;
        t.integrity = Legality::Modern::Integrity::Verified;
        t.game = Game::SW;
        const auto verified = Legality::Modern::analyze(base->plain, t);
        check(verified.coverage.sourceGame == Legality::CoverageLevel::Complete && verified.verdict() == Verdict::Incomplete,
              "verified container: source game Complete, verdict still Incomplete");
    }
}

void testHelpers() {
    using Legality::Gen8::originGeneration;
    using Legality::Gen8::sideTransferOrigin;
    check(originGeneration(44, 0) == 8 && originGeneration(45, 0) == 8 && originGeneration(48, 0) == 8, "gen8 ids");
    check(originGeneration(42, 0) == 7 && originGeneration(33, 0) == 7 && originGeneration(24, 0) == 6, "gen6/7 ids");
    check(originGeneration(34, 30012) == 8 && originGeneration(34, 50) == 7 && originGeneration(34, 0) == 0, "GO by location");
    check(originGeneration(56, 0) == 3 && originGeneration(35, 0) == 1 && originGeneration(41, 0) == 2, "VC ids");
    check(originGeneration(52, 0) == 9, "Z-A is generation 9");
    for (uint8_t v : {0, 6, 46, 50, 51, 53, 55, 58, 255}) check(originGeneration(v, 0) == 0, "not a PK8 origin " + std::to_string(v));
    check(sideTransferOrigin(60000) == 47 && sideTransferOrigin(59999) == 48 && sideTransferOrigin(59996) == 51 &&
          sideTransferOrigin(59994) == 57 && sideTransferOrigin(30001) == 0, "HOME side-transfer locations");
    namespace D = Legality::Gen8::Data;
    const auto grookey = D::personal(810, 0);
    check(grookey && grookey->present && grookey->ability1 == 65 && grookey->baseStats[0] == 50, "Grookey personal");
    check(!D::presentInGame(13, 0) && D::presentInGame(25, 0), "Weedle absent, Pikachu present");
    check(D::maxSpecies() == 898 && D::maxMove() == 826 && D::maxBall() == 26 && D::techRecordCount() == 100, "limits");
    check(D::basePP(1) == 35 && D::basePP(827) == 0, "base PP");
    check(D::mintNature(1) && D::mintNature(12) && !D::mintNature(0) && !D::mintNature(25), "mint natures");
    check(D::battleOnlyForm(555, 1) && !D::battleOnlyForm(555, 0), "Zen Mode Darmanitan is battle-only");
    check(D::arceusForm(0) == 0 && D::silvallyForm(904) == 1, "form items");
}

void testFuzz(const std::vector<Record>& records) {
    // records holds both PK8 and PB8 fixture lines; each mutated record keeps its format.
    const char* env = std::getenv("GEN8_FUZZ_ROUNDS");
    const size_t rounds = env ? std::strtoul(env, nullptr, 10) : 20000;
    std::mt19937 rng(8);
    for (size_t i = 0; i < rounds; ++i) {
        Input in;
        in.format = (rng() & 1) != 0 ? Format::PK8 : Format::PB8;
        in.game = static_cast<Game>(std::array<uint8_t, 5>{0, 44, 45, 48, 30}[rng() % 5]);
        in.slot = static_cast<Slot>(rng() % 4);
        in.encrypted = (rng() & 1) != 0;
        std::vector<uint8_t> bytes;
        if (!records.empty() && (rng() % 3) != 0) {
            // A fixture record with random byte flips and a refreshed checksum.
            const auto& base = records[rng() % records.size()];
            in.format = base.format;
            bytes = base.plain;
            for (int k = 0, n = 1 + static_cast<int>(rng() % 6); k < n; ++k) bytes[8 + rng() % (bytes.size() - 8)] ^= static_cast<uint8_t>(1u << (rng() % 8));
            PK8::write16(bytes, 0x06, PK8::checksum(bytes));
            if (in.encrypted) bytes = PK8::encrypt(bytes);
        } else {
            bytes.resize(std::array<size_t, 5>{0x148, 0x158, 0x147, 0, 0xE8}[rng() % 5]);
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
    const auto records = loadFixture("GEN8_FIXTURE", "tests/fixtures/gen67/pkhex8_seed2_sample.txt", "8");
    const auto bdsp = loadFixture("GEN8B_FIXTURE", "tests/fixtures/gen67/pkhexb8_seed2_sample.txt", "8b");
    testFixture(records, "PK8");
    testFixture(bdsp, "PB8");
    testRuleEdits(records);
    testBdsp(bdsp);
    testInterface(records);
    testHelpers();
    auto all = records;
    all.insert(all.end(), bdsp.begin(), bdsp.end());
    testFuzz(all);
    std::cout << "test_legality_gen8: " << checks << " checks, " << failures << " failures\n";
    return failures == 0 ? 0 : 1;
}
