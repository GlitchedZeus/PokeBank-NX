// Gen V app adapter (include/Legality/Gen5AppAdapter.h): raw PK5 bytes plus the app's exact
// source game id give a Report, a verdict line and overlay lines. The adapter must never
// render an Incomplete report as "No problems found by complete checks", must accept
// encrypted and decrypted 136/220-byte records, and must survive malformed input.
// The fixtures are PKHeX-built records from tests/test_legality_gen5.cpp.
#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#include "Encryption/Encryption5.h"
#include "Legality/Gen5AppAdapter.h"
#include "legality_gen5_test_helpers.h"

namespace {

using namespace Gen5TestHelpers;
using Legality::Severity;
using Legality::Verdict;
namespace App = Legality::Gen5App;

int failures = 0;
int checked = 0;

void fail(const std::string& what, const App::Result& r) {
    ++failures;
    std::cerr << "FAIL " << what << "\n";
    for (const auto& line : r.lines()) std::cerr << "    " << line << "\n";
}

bool anyLine(const std::vector<std::string>& lines, const std::string& text) {
    return std::any_of(lines.begin(), lines.end(), [&](const std::string& l) { return l.find(text) != std::string::npos; });
}

const char* kComplete = "No problems found by complete checks.";
const char* kIncomplete = "Incomplete / Unknown: required checks or evidence are unresolved.";

// The verdict line and the lines agree with the Report's verdict: "complete" only for
// NoProblemsFound, coverage rows for every Incomplete report, then its actual issues.
void checkRendering(const std::string& what, const App::Result& r) {
    const auto lines = r.lines();
    if (lines.empty() || lines[0] != r.verdictLine) fail(what + ": first line is not the verdict line", r);
    if (r.verdict != r.report.verdict()) fail(what + ": verdict differs from the report's", r);
    if ((r.verdictLine == kComplete) != (r.verdict == Verdict::NoProblemsFound))
        fail(what + ": \"complete\" wording does not follow Verdict::NoProblemsFound", r);
    if (r.verdict == Verdict::NoProblemsFound && !allComplete(r.report)) fail(what + ": complete with a non-Complete area", r);
    if (r.verdict == Verdict::Incomplete && r.issueLines.empty() && !r.emptySlot) {
        if (r.verdictLine != kIncomplete) fail(what + ": incomplete wording", r);
        if (lines.size() != 1 + r.coverageLines.size()) fail(what + ": coverage rows missing", r);
        if (r.coverageLines.size() != 9) fail(what + ": nine coverage rows expected", r);
        if (std::none_of(r.coverageLines.begin(), r.coverageLines.end(),
                         [](const std::string& l) { return l.find("Partial") != std::string::npos || l.find("None") != std::string::npos; }))
            fail(what + ": an Incomplete report has no Partial / None area", r);
    }
    if (r.verdict == Verdict::Incomplete && !r.emptySlot) {
        if (r.verdictLine.rfind("Incomplete / Unknown:", 0) != 0) fail(what + ": missing explicit incomplete headline", r);
        if (anyLine(lines, "No problems found")) fail(what + ": incomplete report rendered as clean", r);
        if (lines.size() != 1 + r.coverageLines.size() + r.issueLines.size()) fail(what + ": incomplete coverage/issues dropped", r);
    }
    if (r.verdict == Verdict::Invalid && !anyLine(r.issueLines, "Invalid: ")) fail(what + ": Invalid without an Invalid line", r);
    for (const auto& line : r.issueLines)
        if (line.rfind("Invalid: ", 0) != 0 && line.rfind("Warning: ", 0) != 0) fail(what + ": issue line without a severity", r);
    const int problems = r.report.problemCount();
    if (static_cast<int>(r.issueLines.size()) != problems) fail(what + ": issue lines do not match problemCount", r);
    ++checked;
}

} // namespace

int main() {
    // A PKHeX-valid Black 2 egg (tests/test_legality_gen5.cpp "b0") and a Gen IV card transfer
    // whose Gen V report is Incomplete at its Warning ("transfer-cxd" is a Colosseum origin).
    const auto bulbasaur = fromHex("B4048BB20000CB4D010000001179810B00000000784100020000000000000000000000000000000021000000000000002300000000000000F1FC9F3F00000000000C000000000000420075006C00620061007300610075007200FFFF000000150000000000000000540065007300740065007200FFFF00001A0A0F1A0A0F62EA4000000401000000");
    const uint8_t version = bulbasaur[0x5F];
    const char* exact = version == 20 ? "white_nds" : version == 21 ? "black_nds" : version == 22 ? "white2_nds" : "black2_nds";
    const char* other = version == 23 ? "white_nds" : "black2_nds";

    // Game id mapping.
    if (App::versionFromSourceGameId("white_nds") != 20 || App::versionFromSourceGameId("black_nds") != 21 ||
        App::versionFromSourceGameId("white2_nds") != 22 || App::versionFromSourceGameId("black2_nds") != 23 ||
        App::versionFromSourceGameId("black2") != 0 || App::versionFromSourceGameId("diamond_nds") != 0 ||
        App::versionFromSourceGameId("") != 0)
        fail("source game id mapping", {});
    ++checked;

    // Decrypted record, exact game: complete.
    {
        auto r = App::analyze(bulbasaur, exact, {.encrypted = false});
        checkRendering("decrypted exact", r);
        if (r.verdict != Verdict::NoProblemsFound || !r.knownGame || r.lines().size() != 1) fail("decrypted exact: expected one complete line", r);
        // The record was only read.
        auto copy = bulbasaur;
        r = App::analyze(copy, exact, {.encrypted = false});
        if (copy != bulbasaur) fail("input was modified", r);
        // Stored (encrypted) form: the same report.
        const auto stored = PokeVault::Integration::Gen5::Crypto::encryptCandidate(bulbasaur);
        const auto e = App::analyze(stored, exact);
        checkRendering("encrypted exact", e);
        if (!sameReport(e.report, r.report)) fail("encrypted form differs", e);
        // Party-sized record (220 bytes) with the level byte: still complete.
        auto party = bulbasaur;
        party.resize(220, 0);
        party[0x8C] = 1;
        const auto p = App::analyze(party, exact, {.encrypted = false, .slot = Legality::Pk5Slot::Party});
        checkRendering("party record", p);
        if (p.verdict != Verdict::NoProblemsFound) fail("party record: expected complete", p);
        checked += 3;
    }
    // Another Gen V game: traded in, information only, still complete.
    {
        const auto r = App::analyze(bulbasaur, other, {.encrypted = false});
        checkRendering("other Gen V game", r);
        if (r.verdict != Verdict::NoProblemsFound || !r.knownGame) fail("other Gen V game: expected complete", r);
        if (!std::any_of(r.report.issues.begin(), r.report.issues.end(),
                         [](const Legality::Issue& i) { return i.severity == Severity::Info && i.text.find("traded") != std::string::npos; }))
            fail("other Gen V game: expected a traded Info line", r);
        ++checked;
    }
    // Unknown id: never complete.
    for (const char* id : {"diamond_nds", "red_gb", "Black2_nds", "black2"}) {
        const auto r = App::analyze(bulbasaur, id, {.encrypted = false});
        checkRendering(std::string("unknown id ") + id, r);
        if (r.verdict != Verdict::Incomplete || r.knownGame || r.verdictLine == kComplete) fail(std::string("unknown id ") + id, r);
        if (!anyLine(r.issueLines, "is not a Gen V game")) fail(std::string("unknown id ") + id + ": expected the game Warning", r);
        if (r.report.coverage.sourceGame != Legality::CoverageLevel::Partial) fail("unknown id: source game coverage", r);
        ++checked;
    }
    // Empty id: unknown game, nothing added.
    {
        const auto r = App::analyze(bulbasaur, "", {.encrypted = false});
        checkRendering("empty id", r);
        if (r.verdict != Verdict::NoProblemsFound || r.knownGame) fail("empty id: expected complete without game context", r);
        ++checked;
    }
    // Box slot: PP must be full; party slot: any PP.
    {
        auto used = bulbasaur;
        used[0x30] = static_cast<uint8_t>(used[0x30] - 1);  // first move's PP
        refreshChecksum(used);
        const auto box = App::analyze(used, exact, {.encrypted = false, .slot = Legality::Pk5Slot::Box});
        checkRendering("box slot used PP", box);
        if (box.verdict != Verdict::Invalid || !anyLine(box.issueLines, "boxed Pokemon")) fail("box slot: expected Invalid PP", box);
        const auto party = App::analyze(used, exact, {.encrypted = false, .slot = Legality::Pk5Slot::Party});
        checkRendering("party slot used PP", party);
        if (party.verdict != Verdict::NoProblemsFound) fail("party slot: used PP is allowed", party);
        const auto unknown = App::analyze(used, exact, {.encrypted = false});
        if (unknown.verdict != Verdict::NoProblemsFound) fail("unknown slot: used PP is allowed", unknown);
        checked += 3;
    }
    // Save trainer context: the save's own trainer is information; its ID with another name is a Warning.
    {
        Legality::Pk5Trainer tr;
        tr.present = true;
        tr.tid = static_cast<uint16_t>(bulbasaur[0x0C] | (bulbasaur[0x0D] << 8));
        tr.sid = static_cast<uint16_t>(bulbasaur[0x0E] | (bulbasaur[0x0F] << 8));
        for (std::size_t i = 0; i < 8; ++i) tr.ot[i] = static_cast<uint16_t>(bulbasaur[0x68 + 2 * i] | (bulbasaur[0x69 + 2 * i] << 8));
        tr.female = (bulbasaur[0x84] & 0x80) != 0;
        tr.language = bulbasaur[0x17];
        auto own = App::analyze(bulbasaur, exact, {.encrypted = false, .trainer = tr});
        checkRendering("own trainer", own);
        if (own.verdict != Verdict::NoProblemsFound) fail("own trainer: expected complete", own);
        tr.ot[0] = u'X';
        const auto renamed = App::analyze(bulbasaur, exact, {.encrypted = false, .trainer = tr});
        checkRendering("same ID other OT", renamed);
        if (renamed.verdict != Verdict::Incomplete || !anyLine(renamed.issueLines, "suspicious provenance"))
            fail("same ID other OT: expected a Warning", renamed);
        tr.tid ^= 1;
        const auto traded = App::analyze(bulbasaur, exact, {.encrypted = false, .trainer = tr});
        checkRendering("other trainer", traded);
        if (traded.verdict != Verdict::NoProblemsFound) fail("other trainer: trades are normal", traded);
        checked += 3;
    }
    // Malformed input: sizes, checksum, bad egg, empty slot.
    for (std::size_t size : {0u, 1u, 8u, 135u, 137u, 219u, 221u, 4096u}) {
        std::vector<uint8_t> junk(size, 0xA5);
        for (const char* id : {exact, "diamond_nds", ""}) {
            const auto r = App::analyze(junk, id);
            checkRendering("malformed size " + std::to_string(size), r);
            if (r.verdict != Verdict::Incomplete || r.verdictLine == kComplete) fail("malformed size " + std::to_string(size), r);
            if (!anyLine(r.issueLines, "Not a 136-byte or 220-byte PK5 record")) fail("malformed size: expected the structure Warning", r);
        }
        ++checked;
    }
    {
        auto broken = bulbasaur;
        broken[0x40] ^= 1;  // changed without a checksum refresh
        const auto r = App::analyze(broken, exact, {.encrypted = false});
        checkRendering("broken checksum", r);
        if (r.verdict != Verdict::Invalid || !anyLine(r.issueLines, "checksum")) fail("broken checksum: expected Invalid", r);
        const std::vector<uint8_t> zero(136, 0);
        const auto empty = App::analyze(zero, exact);
        checkRendering("empty slot", empty);
        if (!empty.emptySlot || empty.verdictLine != "Empty slot." || empty.lines().size() != 1) fail("empty slot: expected the empty-slot line", empty);
        const auto emptyUnknown = App::analyze(zero, "diamond_nds");
        if (!emptyUnknown.emptySlot || !emptyUnknown.issueLines.empty()) fail("empty slot under an unknown game: nothing to report", emptyUnknown);
        checked += 3;
    }
    // An Incomplete report with no problems renders the coverage rows, never "complete":
    // a Gen III-origin record (Poke Transfer) keeps the egg/breeding area Partial by design.
    {
        const auto g3 = fromHex("3F6F8F0A0000D8E5B6010000FF82D3E9000000000A2200030000000000000000000000000000000021000000000000000800000000000000D3B8AE5700000000000F000000000000530065006500640006FFFF000000000000000000000400000000000000005400650073007400650075007200FFFF00000000001A0A0F00000000030A0101");
        auto r = App::analyze(g3, exact, {.encrypted = false});
        checkRendering("Gen III origin", r);
        if (r.verdictLine == kComplete) fail("Gen III origin must not read as complete", r);
        ++checked;
    }
    // Fuzz: random records of every size under every game id; the rendering invariants hold.
    uint64_t seed = 0x9E3779B97F4A7C15ull;
    auto next = [&] { seed ^= seed << 13; seed ^= seed >> 7; seed ^= seed << 17; return static_cast<uint32_t>(seed); };
    const int rounds = std::getenv("GEN5_FUZZ_ROUNDS") ? std::atoi(std::getenv("GEN5_FUZZ_ROUNDS")) : 5000;
    const char* ids[] = {"black_nds", "white_nds", "black2_nds", "white2_nds", "diamond_nds", ""};
    for (int i = 0; i < rounds; ++i) {
        const std::size_t sizes[] = {136, 220, 136, 220, static_cast<std::size_t>(next() % 300)};
        std::vector<uint8_t> pk(sizes[next() % 5]);
        for (auto& b : pk) b = static_cast<uint8_t>(next());
        if (next() & 1) {
            // a mutated valid record with a good checksum reaches the analyzer
            pk = bulbasaur;
            for (uint32_t n = 1 + next() % 4; n > 0; --n) pk[8 + next() % 128] = static_cast<uint8_t>(next());
            refreshChecksum(pk);
        }
        App::Options o;
        o.encrypted = (next() & 1) != 0 && pk != bulbasaur;
        o.slot = static_cast<Legality::Pk5Slot>(next() % 3);
        const auto copy = pk;
        const auto r = App::analyze(pk, ids[next() % 6], o);
        if (pk != copy) fail("fuzz: input was modified", r);
        checkRendering("fuzz", r);
    }
    if (failures) {
        std::cerr << failures << " Gen V adapter failure(s)\n";
        return 1;
    }
    std::cout << "Gen V app adapter: " << checked << " cases PASS\n";
    return 0;
}
