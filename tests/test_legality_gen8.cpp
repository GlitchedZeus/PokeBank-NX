// Sword/Shield (PK8), Brilliant Diamond/Shining Pearl (PB8) and Scarlet/Violet (PK9) legality
// through Legality::Modern::analyze.
//
// tests/fixtures/gen67/pkhex8_seed2_sample.txt is a stratified sample of the differential
// corpus (tools/pkhex_gen67_diff, `generate8 2 4`): synthetic records PKHeX.Core built from
// its own Sword/Shield encounter templates, records it moved into Sword/Shield through HOME
// (Gen III-VII, Virtual Console, Let's Go, BD/SP, Legends: Arceus, Scarlet/Violet) and grown
// copies, each with PKHeX's verdict and a single-field mutation.
// tests/fixtures/gen67/pkhexb8_seed2_sample.txt is the same for PB8 (`generate8b 2 4`): BD/SP
// encounters, records moved into BD/SP through HOME (Gen III-VII, Virtual Console, Let's Go,
// Sword/Shield, Legends: Arceus, Scarlet/Violet) and grown copies.
// tests/fixtures/gen67/pkhex9_seed2_sample.txt is the same for PK9 (`generate9 2 4`): Scarlet/
// Violet encounters, records moved into Scarlet/Violet through HOME and grown copies (more
// unmutated records per stratum, so the rule-edit tests have bases). One line per record:
//   <8|8b|9> <game> <box|party> <decrypted hex> <PKHeX VALID|INVALID> <NX Invalid|Incomplete> <label>
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
        out.push_back({fmt == "9" ? Format::PK9 : fmt == "8b" ? Format::PB8 : Format::PK8, static_cast<Game>(std::stoi(game)),
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
    // Gigantamax (MiscVerifierPK8.VerifyGigantamax): Charizard can take Gigantamax Soup; Mew has no
    // Gigantamax encounter; a cap Pikachu cannot toggle; an egg never toggles.
    {
        namespace E = Legality::ModernEncounter;
        check(E::gigantamaxPossible(6, 0, true, false) && E::gigantamaxPossible(6, 0, false, false) &&
                  !E::gigantamaxPossible(151, 0, true, false) && E::gigantamaxPossible(151, 0, false, false) &&
                  E::gigantamaxPossible(25, 0, true, false) && !E::gigantamaxPossible(25, 1, true, false) &&
                  !E::gigantamaxPossible(6, 0, true, true) && E::gigantamaxPossible(2000, 0, true, false),
              "Gigantamax table facts");
        const Record* plain = find(records, [](const Record& r) {
            return r.pkhexValid && r.label.rfind("native8:base", 0) == 0 && !(r.plain[0x16] & 0x10) && !(r.plain[0x8F] & 0x40) &&
                   !E::gigantamaxPossible(PK8::read16(r.plain, 0x08), r.plain[0x24], true, false);
        });
        check(plain != nullptr, "fixture has a native without Gigantamax encounters");
        if (plain) expectInvalid(*plain, "gigantamax flag", [](auto& b) { b[0x16] |= 0x10; }, "Gigantamax flag set");
    }
    // A battle version deletes the original moveset (relearn moves included).
    expectNotInvalid(*home7, "battle version on a Gen VII record", [](auto& b) {
        b[0xDF] = 45;
        for (size_t i = 0; i < 8; ++i) b[0x82 + i] = 0;
    });
    if (PK8::read16(home7->plain, 0x82) != 0)
        expectInvalid(*home7, "battle version keeping relearn moves", [](auto& b) { b[0xDF] = 45; }, "original moveset was deleted");
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
    // PID equal to the EC on a BD/SP native is undecided only when the line has a fixed-EC WB8
    // gift; on a Sword/Shield record with EC 0 it may be a HOME gift (WC8.IsHOMEGift).
    {
        auto pidIsEc = [](auto& b) { PK8::write32(b, 0x1C, PK8::read32(b, 0x00)); };
        if (Legality::ModernEncounter::fixedEcGiftPossible(PK8::read16(n.plain, 0x08), n.plain[0xDE]))
            expectNotInvalid(n, "pid==ec native, line with a fixed-EC gift", pidIsEc);
        else
            expectInvalid(n, "pid==ec native", pidIsEc, "PID equals the encryption constant");
        check(Legality::ModernEncounter::fixedEcGiftPossible(5000, 48), "an unknown species may be a fixed-EC gift");
        check(!Legality::ModernEncounter::fixedEcGiftPossible(25, 44), "only Legends: Arceus and BD/SP origins have fixed-EC gifts");
    }
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

// PK9: the moved fields (internal species id, version, language, gender), MiscVerifierPK9
// (Tera types, obedience level, battle version, TM flags), Scarlet/Violet form items, scale
// and egg rules, on PKHeX-valid Scarlet/Violet records from the fixture.
void testScarletViolet(const std::vector<Record>& records) {
    namespace D = Legality::Gen8::Data;
    using Legality::Gen8::Context;
    check(Legality::Modern::profile(Format::PK9)->rulesImplemented, "PK9 has rules");
    auto national = [](const Record& r) { return D::nationalSpecies9(PK8::read16(r.plain, 0x08)); };
    const Record* native = find(records, [&](const Record& r) {
        const uint16_t s = national(r);
        return r.pkhexValid && startsWith(r.label, "native9:base") && s != 1017 && s != 1024 && s != 664 && s != 665 &&
               ((PK8::read32(r.plain, 0x8C) >> 30) & 1) == 0 && (r.plain[0xCE] == 50 || r.plain[0xCE] == 51) &&
               r.plain[0xA8] == 0 && r.plain[0xA9] == 0;
    });
    const Record* home = find(records, [](const Record& r) {
        return r.pkhexValid && startsWith(r.label, "svhome7:base") && ((PK8::read32(r.plain, 0x8C) >> 30) & 1) == 0;
    });
    check(native && home, "fixture has native and HOME PK9 bases");
    if (!native || !home) return;
    auto expectInvalid = [&](const Record& base, const char* name, const std::function<void(std::vector<uint8_t>&)>& f, const char* fragment) {
        const auto r = analyzePlain(base, edit(base.plain, f));
        check(r.verdict() == Verdict::Invalid && hasIssue(r, Severity::Invalid, fragment),
              std::string("PK9 ") + name + ": expected Invalid '" + fragment + "', got " + describe(r));
    };
    auto expectNotInvalid = [&](const Record& base, const char* name, const std::function<void(std::vector<uint8_t>&)>& f) {
        const auto r = analyzePlain(base, edit(base.plain, f));
        check(r.verdict() == Verdict::Incomplete, std::string("PK9 ") + name + ": expected Incomplete, got " + describe(r));
    };
    const Record& n = *native;
    const uint16_t ns = national(n);
    expectInvalid(n, "tera original", [](auto& b) { b[0x94] = 18; }, "Tera type 18 does not exist");
    expectInvalid(n, "obedience above met on an untraded record", [](auto& b) { b[0x11F] = static_cast<uint8_t>((b[0x125] & 0x7F) + 1); }, "Obedience level");
    expectInvalid(n, "battle version", [](auto& b) { b[0xCF] = 50; }, "Battle version 50");
    expectInvalid(n, "battle version on a native", [](auto& b) { b[0xCF] = 44; }, "Battle version 44");
    expectInvalid(n, "origin 53", [](auto& b) { b[0xCE] = 53; }, "cannot be stored in a PK9");
    expectInvalid(n, "species above 1025", [](auto& b) { PK8::write16(b, 0x08, 1100); }, "does not exist in PK9");
    expectInvalid(n, "language", [](auto& b) { b[0xD5] = 6; }, "is not a language");
    expectInvalid(n, "hyper training below 50", [](auto& b) { b[0x126] = 1; PK8::write32(b, 0x10, 0); b[0x125] &= 0x80; b[0x11F] = 0; }, "Hyper Training needs level 50");
    expectInvalid(n, "ball 38", [](auto& b) { b[0x124] = 38; }, "cannot be held by a PK9");
    {
        uint16_t item = 1;
        while (item < 3000 && D::heldItemAllowed(item, Context::ScarletViolet)) ++item;
        expectInvalid(n, "held item", [item](auto& b) { PK8::write16(b, 0x0A, item); }, "not obtainable in Scarlet/Violet");
    }
    {
        int tm = -1;
        for (uint16_t i = 0; i < D::tmCount9() && tm < 0; ++i)
            if (!D::tmLineCanLearn9(ns, i)) tm = i;
        check(tm >= 0, "native base has an unlearnable TM");
        if (tm >= 0)
            expectInvalid(n, "tm flag", [tm](auto& b) {
                const size_t o = tm < 200 ? 0x12F + (tm >> 3) : 0x4B + ((tm - 200) >> 3);
                b[o] |= static_cast<uint8_t>(1u << (tm & 7));
            }, "line cannot learn it");
    }
    // A form item rule only Scarlet/Violet has: Giratina-Origin holds the Griseous Core.
    expectInvalid(n, "giratina origin without core", [](auto& b) { PK8::write16(b, 0x08, 487); b[0x24] = 1; PK8::write16(b, 0x0A, 112); }, "Origin item");
    // Records from other games: the Tera type override is set, and HOME's scale copy holds.
    const Record& h = *home;
    expectInvalid(h, "no tera override on a moved record", [](auto& b) { b[0x95] = 19; }, "must have a Tera type override");
    expectInvalid(h, "scale differs from height after HOME", [](auto& b) {
        if (PK8::read32(b, 0x127) == 0 && PK8::read32(b, 0x12B) == 0) PK8::write32(b, 0x127, 1);
        b[0x4A] = static_cast<uint8_t>(b[0x48] ^ 0x40);
    }, "Height scalar does not match");
    // Values a legitimate record can hold stay undecided.
    expectNotInvalid(n, "mint stat nature", [](auto& b) { b[0x21] = b[0x20] == 1 ? 2 : 1; });
    expectNotInvalid(n, "stellar tera original", [](auto& b) { b[0x94] = 99; });
    expectNotInvalid(h, "battle version on a moved record", [](auto& b) { b[0xCF] = 44; });
    // Eggs: no Tera override, the nickname flag set (except Jacq's egg), no version yet.
    Record egg = n;
    egg.plain = edit(n.plain, [](auto& b) {
        PK8::write32(b, 0x8C, PK8::read32(b, 0x8C) | 0xC0000000u);
        b[0x95] = 19;
    });
    expectInvalid(egg, "egg tera override", [](auto& b) { b[0x95] = 3; }, "egg cannot have a Tera type override");
    expectInvalid(egg, "egg nickname flag", [](auto& b) { PK8::write32(b, 0x8C, PK8::read32(b, 0x8C) & 0x7FFFFFFFu); }, "nickname flag");
    {
        const auto r = analyzePlain(egg, edit(egg.plain, [](auto& b) {
            PK8::write32(b, 0x8C, PK8::read32(b, 0x8C) & 0x7FFFFFFFu);
            PK8::write16(b, 0x120, 60005);
            PK8::write16(b, 0x08, 387);  // a species Jacq's egg can be (the egg table allows the flag clear)
        }));
        check(!hasIssue(r, Severity::Invalid, "nickname flag"), "PK9 Jacq egg needs no nickname flag: " + describe(r));
        const auto r0 = analyzePlain(egg, edit(egg.plain, [](auto& b) { b[0xCE] = 0; }));
        check(!hasIssue(r0, Severity::Invalid, "cannot be stored"), "PK9 unhatched egg with version 0: " + describe(r0));
    }
    // The species id is the internal one: a native whose national and internal ids differ.
    check(D::nationalSpecies9(917) != 917 && D::nationalSpecies9(1100) == 1100 && D::nationalSpecies9(25) == 25, "internal species map");
    check(D::maxSpecies(Context::ScarletViolet) == 1025 && D::maxBall(Context::ScarletViolet) == 37, "Scarlet/Violet limits");
    check(D::ogerponForm(0) == 0 && D::ogerponForm(2407) == 1, "Ogerpon masks");
    {
        Record moved = n;
        moved.game = Game::SW;
        const auto r = analyzePlain(moved, moved.plain);
        check(r.verdict() == Verdict::Incomplete && hasIssue(r, Severity::Warning, "does not store PK9"),
              "PK9 in Sword/Shield is a source-game Warning");
    }
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
        in.format = std::array<Format, 3>{Format::PK8, Format::PB8, Format::PK9}[rng() % 3];
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

// Egg rules (ModernEncounter.h egg facts, hatch lists, egg names) on PK8, PB8 and PK9 records.
void testEggs(const std::vector<Record>& all) {
    namespace B = Legality::ModernEncounter;
    check(B::eggFacts(Format::PK8, 172).exists && !B::eggFacts(Format::PK9, 150).exists && B::eggFacts(Format::PB8, 490).gift,
          "egg facts: Pichu hatches in SW/SH, Mewtwo never in S/V, Manaphy is a BD/SP gift egg");
    check(B::hatchLocation(44, 6) && !B::hatchLocation(44, 0) && !B::hatchLocation(50, 0), "hatch lists");
    for (Format f : {Format::PK8, Format::PB8, Format::PK9}) {
        const size_t versionAt = f == Format::PK9 ? 0xCE : 0xDE;
        const uint8_t own0 = f == Format::PK8 ? 44 : f == Format::PB8 ? 48 : 50;
        const std::string name = f == Format::PK8 ? "PK8" : f == Format::PB8 ? "PB8" : "PK9";
        // A hatched record of the format's own games: met level 1 at a hatching location.
        const Record* hatched = find(all, [&](const Record& r) {
            const uint16_t egg = PK8::read16(r.plain, 0x120);
            return r.pkhexValid && r.format == f && (r.plain[versionAt] == own0 || r.plain[versionAt] == own0 + 1) && egg != 0 &&
                   egg != 65535 && !(r.plain[0x8F] & 0x40) && r.label.rfind("native", 0) == 0;
        });
        check(hatched != nullptr, name + " fixture has a hatched record");
        if (!hatched) continue;
        auto rep = analyzePlain(*hatched, edit(hatched->plain, [](auto& b) { b[0x125] = static_cast<uint8_t>((b[0x125] & 0x80) | 3); }));
        check(rep.verdict() == Verdict::Invalid && hasIssue(rep, Severity::Invalid, "hatched egg's met level"), name + " hatched at met level 3: " + describe(rep));
        rep = analyzePlain(*hatched, edit(hatched->plain, [](auto& b) { b[0x122] = 0xFE; b[0x123] = 0xFF; }));
        check(rep.verdict() == Verdict::Invalid && hasIssue(rep, Severity::Invalid, "hatching location"), name + " hatched at location 65534: " + describe(rep));
        // The same record set back to an egg keeps its hatch location and species name.
        rep = analyzePlain(*hatched, edit(hatched->plain, [](auto& b) { b[0x8F] |= 0x40; }));
        check(rep.verdict() == Verdict::Invalid && (hasIssue(rep, Severity::Invalid, "unhatched egg") || hasIssue(rep, Severity::Invalid, "No egg of the species")),
              name + " hatched record set to egg: " + describe(rep));
        // A record that came through HOME from another game cannot be an egg.
        const Record* moved = find(all, [&](const Record& r) {
            return r.pkhexValid && r.format == f && r.plain[versionAt] != own0 && r.plain[versionAt] != own0 + 1 && r.plain[versionAt] != 0 &&
                   r.label.find("home") != std::string::npos;
        });
        if (moved) {
            rep = analyzePlain(*moved, edit(moved->plain, [](auto& b) { b[0x8F] |= 0x40; }));
            check(rep.verdict() == Verdict::Invalid && hasIssue(rep, Severity::Invalid, "HOME refuses eggs"), name + " HOME record set to egg: " + describe(rep));
        }
    }
}

void testRelearn(const std::vector<Record>& all) {
    auto setRelearn = [](std::vector<uint8_t>& b, size_t slot, uint16_t move) { PK8::write16(b, 0x82 + 2 * slot, move); };
    for (Format f : {Format::PK8, Format::PB8, Format::PK9}) {
        const size_t versionAt = f == Format::PK9 ? 0xCE : 0xDE;
        const uint8_t own0 = f == Format::PK8 ? 44 : f == Format::PB8 ? 48 : 50;
        const std::string name = f == Format::PK8 ? "PK8" : f == Format::PB8 ? "PB8" : "PK9";
        // A record moved in from another game through HOME has its original moveset deleted.
        const Record* moved = find(all, [&](const Record& r) {
            return r.pkhexValid && r.format == f && r.plain[versionAt] != own0 && r.plain[versionAt] != own0 + 1 && r.plain[versionAt] != 0 &&
                   r.label.find("home") != std::string::npos && PK8::read16(r.plain, 0x82) == 0;
        });
        if (moved) {
            const auto rep = analyzePlain(*moved, edit(moved->plain, [&](auto& b) { setRelearn(b, 0, 33); }));
            check(rep.verdict() == Verdict::Invalid && (hasIssue(rep, Severity::Invalid, "original moveset was deleted") ||
                                                        hasIssue(rep, Severity::Invalid, "has no relearn moves")),
                  name + " HOME record with a relearn move: " + describe(rep));
        }
        // A native record repeating a relearn move.
        const Record* native = find(all, [&](const Record& r) {
            return r.pkhexValid && r.format == f && r.label.rfind("native", 0) == 0 && PK8::read16(r.plain, 0x82) != 0 &&
                   PK8::read16(r.plain, 0x84) == 0;
        });
        if (native) {
            const uint16_t first = PK8::read16(native->plain, 0x82);
            const auto rep = analyzePlain(*native, edit(native->plain, [&](auto& b) { setRelearn(b, 1, first); }));
            check(rep.verdict() == Verdict::Invalid && hasIssue(rep, Severity::Invalid, "is repeated"), name + " repeated relearn move: " + describe(rep));
        }
        check(moved != nullptr || native != nullptr, name + " fixture has relearn controls");
    }
}

// Max Raid seeds (ModernEncounter::raidSeed): a PKHeX-valid non-shiny raid record from the shared den
// location keeps a seed; changing a value the seed fixes leaves none.
void testRaidSeeds(const std::vector<Record>& records) {
    namespace E = Legality::ModernEncounter;
    int checked = 0, dens = 0;
    for (const Record& r : records) {
        if (!r.pkhexValid || r.format != Format::PK8 || r.label.rfind("native8:base", 0) != 0 || PK8::read16(r.plain, 0x122) != 162) continue;
        const uint32_t pid = PK8::read32(r.plain, 0x1C), id32 = PK8::read32(r.plain, 0x0C);
        if ((((pid ^ id32) ^ ((pid ^ id32) >> 16)) & 0xFFFF) < 16) continue;
        const auto base = analyzePlain(r, r.plain);
        check(!hasIssue(base, Severity::Invalid, "Max Raid seed"), "valid raid record keeps a seed: " + describe(base));
        auto rep = analyzePlain(r, edit(r.plain, [](auto& b) { b[0x00] ^= 1; }));
        check(hasIssue(rep, Severity::Invalid, "No overworld or Max Raid seed"), "raid record with a changed EC: " + describe(rep));
        rep = analyzePlain(r, edit(r.plain, [](auto& b) { PK8::write32(b, 0x8C, PK8::read32(b, 0x8C) ^ 1u); }));
        check(hasIssue(rep, Severity::Invalid, "No overworld or Max Raid seed"), "raid record with a changed HP IV: " + describe(rep));
        // The same record met at one of its raids' Wild Area dens, when no other encounter of its
        // line can carry that location: still a seed; with another EC, none.
        E::RaidRecord rr;
        rr.species = PK8::read16(r.plain, 0x08);
        rr.form = r.plain[0x24];
        rr.ec = PK8::read32(r.plain, 0x00);
        rr.pid = pid;
        rr.id32 = id32;
        const uint32_t iv32 = PK8::read32(r.plain, 0x8C);
        auto iv = [&](int i) { return static_cast<uint8_t>((iv32 >> (5 * i)) & 31); };
        rr.ivs = {iv(0), iv(1), iv(2), iv(4), iv(5), iv(3)};
        rr.abilityNumber = r.plain[0x16] & 7;
        rr.gender = (r.plain[0x22] >> 2) & 3;
        rr.nature = r.plain[0x20];
        rr.height = r.plain[0x50];
        rr.weight = r.plain[0x51];
        rr.metLocation = 162;
        check(E::raidSeed(rr) != E::RaidResult::NoSeed, "raid record keeps a seed at the Pokemon Den");
        for (uint16_t loc = 122; loc < 235; ++loc) {
            rr.metLocation = loc;
            if (E::raidSeed(rr) != E::RaidResult::SeedFound) continue;
            auto other = rr;
            other.ec ^= 1;
            check(E::raidSeed(other) == E::RaidResult::NoSeed, "raid record at Wild Area den " + std::to_string(loc) + " with another EC");
            ++dens;
            break;
        }
        if (++checked == 20) break;
    }
    check(checked > 0, "fixture has non-shiny raid records");
    check(dens > 0, "some raid record is decidable at a Wild Area den");
}

// Overworld8RNG (ModernEncounter::raidSeed): a PKHeX-valid wild or static Sword/Shield record met
// where every encounter of its line is seed-bound keeps its overworld seed; another EC, IV or size
// leaves none, and a Curry mark makes the record undecided.
void testOverworldSeeds(const std::vector<Record>& records) {
    namespace E = Legality::ModernEncounter;
    int found = 0;
    for (const Record& r : records) {
        if (!r.pkhexValid || r.format != Format::PK8 || r.label.rfind("native8:base:EncounterS", 0) != 0) continue;
        const uint16_t loc = PK8::read16(r.plain, 0x122);
        if (loc == 162 || loc == 244 || PK8::read16(r.plain, 0x120) != 0) continue;
        E::RaidRecord rr;
        rr.species = PK8::read16(r.plain, 0x08);
        rr.form = r.plain[0x24];
        rr.metLocation = loc;
        rr.ec = PK8::read32(r.plain, 0x00);
        rr.pid = PK8::read32(r.plain, 0x1C);
        rr.id32 = PK8::read32(r.plain, 0x0C);
        const uint32_t iv32 = PK8::read32(r.plain, 0x8C);
        auto iv = [&](int i) { return static_cast<uint8_t>((iv32 >> (5 * i)) & 31); };
        rr.ivs = {iv(0), iv(1), iv(2), iv(4), iv(5), iv(3)};
        rr.abilityNumber = r.plain[0x16] & 7;
        rr.gender = (r.plain[0x22] >> 2) & 3;
        rr.nature = r.plain[0x20];
        rr.height = r.plain[0x50];
        rr.weight = r.plain[0x51];
        rr.curryMark = (r.plain[0x40] & 0x08) != 0;
        const auto result = E::raidSeed(rr);
        check(result != E::RaidResult::NoSeed, r.label + ": a PKHeX-valid record keeps a seed");
        if (result != E::RaidResult::SeedFound || rr.curryMark) continue;
        const auto base = analyzePlain(r, r.plain);
        check(!hasIssue(base, Severity::Invalid, "Max Raid seed"), r.label + ": no seed issue: " + describe(base));
        auto other = rr;
        other.ec ^= 0x100;
        check(E::raidSeed(other) == E::RaidResult::NoSeed, r.label + ": another EC has no seed");
        other = rr;
        other.ivs[0] ^= 1;
        check(E::raidSeed(other) == E::RaidResult::NoSeed, r.label + ": another HP IV has no seed");
        other = rr;
        other.height = static_cast<uint8_t>(other.height + 1);
        if (!(other.height == 0 && other.weight == 0))
            check(E::raidSeed(other) == E::RaidResult::NoSeed, r.label + ": another height has no seed");
        other = rr;
        other.ivs[0] ^= 1;
        other.curryMark = true;
        check(E::raidSeed(other) == E::RaidResult::Undecided, r.label + ": a Curry mark is undecided");
        const auto rep = analyzePlain(r, edit(r.plain, [](auto& b) { b[0x01] ^= 1; }));
        check(hasIssue(rep, Severity::Invalid, "No overworld or Max Raid seed"), r.label + ": record with another EC: " + describe(rep));
        ++found;
    }
    check(found > 0, "fixture has overworld-correlated records");
}

// IHyperTrain.GetHyperTrainMinLevel: whether the line can have visited Scarlet/Violet or Z-A.
void testVisitNine() {
    namespace E = Legality::ModernEncounter;
    check(!E::visitNinePossible(19), "Rattata's line is in neither Scarlet/Violet nor Z-A");
    check(E::visitNinePossible(25), "Pikachu is in Scarlet/Violet");
    check(E::visitNinePossible(26), "Raichu's line includes Pikachu");
    check(E::visitNinePossible(5000), "an unknown species is not decided");
}

// MiscVerifierPK9.VerifyEncounter Tera types over the line (ModernEncounter::teraImportPossible /
// teraEggPossible), and the PK9 rule on PKHeX-valid imports.
void testTeraTypes(const std::vector<Record>& records) {
    namespace E = Legality::ModernEncounter;
    check(E::teraImportPossible(1, 11), "Bulbasaur imports as Grass");
    check(!E::teraImportPossible(1, 9), "Bulbasaur never imports as Fire");
    check(E::teraImportPossible(493, 0) && !E::teraImportPossible(493, 9), "Arceus imports only as Normal");
    check(E::teraEggPossible(1, 3) && !E::teraEggPossible(1, 9), "a Bulbasaur egg hatches Grass or Poison");
    int checked = 0;
    for (const Record& r : records) {
        if (!r.pkhexValid || r.format != Format::PK9 || r.plain[0xCE] == 50 || r.plain[0xCE] == 51 || r.plain[0xCE] == 0) continue;
        const uint16_t s = Legality::Gen8::Data::nationalSpecies9(PK8::read16(r.plain, 0x08));
        if (s == 1017 || s == 1024) continue;
        const uint8_t original = r.plain[0x94];
        check(E::teraImportPossible(s, original), r.label + ": a PKHeX-valid import keeps its Tera type");
        uint8_t other = 0;
        while (other <= 17 && E::teraImportPossible(s, other)) ++other;
        if (other > 17) continue;
        const auto rep = analyzePlain(r, edit(r.plain, [other](auto& b) { b[0x94] = other; }));
        check(hasIssue(rep, Severity::Invalid, "is not the HOME import type"), r.label + ": another original Tera type: " + describe(rep));
        if (++checked == 20) break;
    }
    check(checked > 0, "PK9 fixture has imports");
}

// The same seeds for a Sword/Shield record moved to BD/SP or Scarlet/Violet through HOME (PB8, PK9): a PKHeX-valid
// record keeps its seed, and another EC loses it wherever the met location is seed-bound.
void testOverworldSeedsMoved(const std::vector<Record>& records, Format format, size_t versionOffset, const std::string& name) {
    int valid = 0, decided = 0;
    for (const Record& r : records) {
        if (!r.pkhexValid || r.format != format || (r.plain[versionOffset] != 44 && r.plain[versionOffset] != 45)) continue;
        const auto base = analyzePlain(r, r.plain);
        check(!hasIssue(base, Severity::Invalid, "Max Raid seed"), r.label + ": " + name + " from Sword/Shield keeps a seed: " + describe(base));
        ++valid;
        const auto rep = analyzePlain(r, edit(r.plain, [](auto& b) { b[0x01] ^= 1; }));
        if (hasIssue(rep, Severity::Invalid, "No overworld or Max Raid seed")) ++decided;
    }
    check(valid > 0, name + " fixture has Sword/Shield records");
    check(decided > 0, "some " + name + " Sword/Shield record has a seed-bound met location");
}

// LocationsHOME.IsValidMetBDSP/FRLG/SV with GetVersionSWSH: a record moved into Sword/Shield
// from another Switch game keeps the version HOME writes for that game, and its line is in it.
void testSideTransfers(const std::vector<Record>& records) {
    namespace E = Legality::ModernEncounter;
    check(E::lineInGame(25, 47) && E::lineInGame(25, 48) && E::lineInGame(25, 50) && E::lineInGame(25, 56), "Pikachu is everywhere");
    check(!E::lineInGame(387, 56), "Turtwig was never in FR/LG");
    check(E::lineInGame(387, 48), "Turtwig is in BD/SP");
    check(E::lineInGame(5000, 47) && E::lineInGame(19, 44), "unknown species and other versions are not decided");
    int checked = 0;
    for (const Record& r : records) {
        if (!r.pkhexValid) continue;
        const uint8_t side = Legality::Gen8::sideTransferOrigin(PK8::read16(r.plain, 0x122));
        if (side == 0) continue;
        const auto base = analyzePlain(r, r.plain);
        check(!hasIssue(base, Severity::Invalid, "through HOME has version"), r.label + ": PKHeX-valid side transfer: " + describe(base));
        check(!hasIssue(base, Severity::Invalid, "which the HOME met location names"), r.label + ": side transfer line: " + describe(base));
        const auto rep = analyzePlain(r, edit(r.plain, [](auto& b) { b[0xDE] ^= (44 ^ 45); }));
        check(hasIssue(rep, Severity::Invalid, "through HOME has version"), r.label + ": swapped Sword/Shield version: " + describe(rep));
        ++checked;
    }
    check(checked > 0, "PK8 fixture has side transfers");
}

// LocationsHOME.GetRemapState outside Sword/Shield: a BD/SP native keeps its own met location,
// so a HOME side-transfer location on it is Invalid; a Legends: Arceus origin stays undecided.
void testSideMetOutsideSwsh(const std::vector<Record>& bdsp) {
    int checked = 0;
    for (const Record& r : bdsp) {
        if (!r.pkhexValid || (r.plain[0xDE] != 48 && r.plain[0xDE] != 49) || r.plain[0xDE] == 0) continue;
        const uint16_t loc = r.plain[0xDE] == 48 ? 59999 : 59998;
        const auto rep = analyzePlain(r, edit(r.plain, [loc](auto& b) { PK8::write16(b, 0x122, loc); }));
        check(hasIssue(rep, Severity::Invalid, "is a HOME location for Sword/Shield"), r.label + ": BD/SP native with a HOME met: " + describe(rep));
        if (++checked == 20) break;
    }
    check(checked > 0, "PB8 fixture has BD/SP natives");
    for (const Record& r : bdsp) {
        if (!r.pkhexValid || r.plain[0xDE] != 47) continue;
        const auto rep = analyzePlain(r, edit(r.plain, [](auto& b) { PK8::write16(b, 0x122, 59997); }));
        check(!hasIssue(rep, Severity::Invalid, "is a HOME location for Sword/Shield"), r.label + ": Legends: Arceus origin stays undecided");
        break;
    }
}

int main() {
    const auto records = loadFixture("GEN8_FIXTURE", "tests/fixtures/gen67/pkhex8_seed2_sample.txt", "8");
    const auto bdsp = loadFixture("GEN8B_FIXTURE", "tests/fixtures/gen67/pkhexb8_seed2_sample.txt", "8b");
    const auto sv = loadFixture("GEN9_FIXTURE", "tests/fixtures/gen67/pkhex9_seed2_sample.txt", "9");
    testFixture(records, "PK8");
    testFixture(bdsp, "PB8");
    testFixture(sv, "PK9");
    testRuleEdits(records);
    testBdsp(bdsp);
    testScarletViolet(sv);
    testInterface(records);
    testHelpers();
    auto all = records;
    all.insert(all.end(), bdsp.begin(), bdsp.end());
    all.insert(all.end(), sv.begin(), sv.end());
    testEggs(all);
    testRelearn(all);
    testRaidSeeds(records);
    testOverworldSeeds(records);
    testVisitNine();
    testTeraTypes(sv);
    testOverworldSeedsMoved(sv, Format::PK9, 0xCE, "PK9");
    testOverworldSeedsMoved(bdsp, Format::PB8, 0xDE, "PB8");
    testSideTransfers(records);
    testSideMetOutsideSwsh(bdsp);
    testFuzz(all);
    std::cout << "test_legality_gen8: " << checks << " checks, " << failures << " failures\n";
    return failures == 0 ? 0 : 1;
}
