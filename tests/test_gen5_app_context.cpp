// App-boundary invariants, independent of PKHeX encounter generation.
// The unchanged synthetic positive is retained by the older adapter suite.
// Context changes describe the caller's evidence; they never make a Pokemon
// illegal. Definite record failures must survive every missing-context case.
#include "Legality/Gen5AppAdapter.h"
#include "Encryption/Encryption5.h"
#include "legality_gen5_test_helpers.h"
#include <cassert>
#include <iostream>

namespace A = Legality::Gen5App;
namespace C = PokeVault::Integration::Gen5::Crypto;
using namespace Gen5TestHelpers;
using Legality::CoverageLevel;
using Legality::Pk5Slot;
using Legality::Verdict;

int main() {
    const auto positive = fromHex("B4048BB20000CB4D010000001179810B00000000784100020000000000000000000000000000000021000000000000002300000000000000F1FC9F3F00000000000C000000000000420075006C00620061007300610075007200FFFF000000150000000000000000540065007300740065007200FFFF00001A0A0F1A0A0F62EA4000000401000000");
    Legality::Gen5::Pk5 pk;
    assert(Legality::Gen5::read(positive, pk));
    A::Options complete;
    complete.encrypted = false;
    complete.slot = Pk5Slot::Box;
    complete.trainer = {true, pk.tid, pk.sid, pk.ot, pk.otGender, pk.language};
    complete.source = A::RecordSource::OriginalSlot;
    const A::RecordIdentity identity{pk.pid, pk.species, pk.tid, pk.sid, pk.checksum,
        pk.version, pk.language, pk.gender, false};
    size_t cases = 0;
    auto check = [&](const auto& bytes, const char* game, const A::Options& options, Verdict expected) {
        const auto untouched = bytes;
        const auto result = A::analyze(bytes, game, options);
        assert(bytes == untouched && result.verdict == expected && consistent(result.report));
        if (expected != Verdict::NoProblemsFound)
            for (const auto& line : result.lines()) assert(line.find("No problems found") == std::string::npos);
        ++cases;
        return result;
    };
    for (const char* game : {"black_nds", "white_nds", "black2_nds", "white2_nds"}) {
        assert(check(positive, game, complete, Verdict::NoProblemsFound).identity == identity);
        const auto encrypted = C::encryptCandidate(positive);
        auto stored = complete;
        stored.encrypted = true;
        const auto decoded = check(positive, game, complete, Verdict::NoProblemsFound);
        const auto encoded = check(encrypted, game, stored, Verdict::NoProblemsFound);
        assert(sameReport(decoded.report, encoded.report) && encoded.identity == identity);
        // All sixteen combinations of absent source witnesses, including
        // simultaneous omissions. A definitive bad item remains Invalid.
        auto bad = positive;
        bad[0x0A] = 0x7E; bad[0x0B] = 2; // key item638
        refreshChecksum(bad);
        for (unsigned missing = 0; missing < 16; ++missing) {
            auto options = complete;
            if (missing & 2) options.slot = Pk5Slot::Unknown;
            if (missing & 4) options.trainer.present = false;
            if (missing & 8) options.source = A::RecordSource::Unknown;
            const char* title = missing & 1 ? "" : game;
            const auto r = check(positive, title, options, missing ? Verdict::Incomplete : Verdict::NoProblemsFound);
            assert(r.identity == identity && !r.emptySlot);
            const auto invalid = check(bad, title, options, Verdict::Invalid);
            assert(invalid.identity && invalid.identity->checksum != identity.checksum);
        }
        for (const auto slot : {Pk5Slot::Party, static_cast<Pk5Slot>(255)}) {
            auto options = complete; options.slot = slot;
            const auto r = check(positive, game, options, Verdict::Incomplete);
            assert(r.report.coverage.structure == CoverageLevel::Partial);
        }
        auto party = positive; party.resize(220); party[0x8C] = 1;
        auto partyOptions = complete; partyOptions.slot = Pk5Slot::Party;
        assert(check(party, game, partyOptions, Verdict::NoProblemsFound).identity->party);
        check(party, game, complete, Verdict::Incomplete);
        --party[0x30]; refreshChecksum(party);
        check(party, game, partyOptions, Verdict::NoProblemsFound);
        check(party, game, complete, Verdict::Incomplete); // Wrong Box label cannot invent illegal PP.
        for (unsigned fault = 0; fault < 8; ++fault) {
            auto options = complete;
            switch (fault) {
                case 0: options.trainer.ot.fill(0xFFFF); break;
                case 1: options.trainer.ot.fill(u'X'); break;
                case 2: options.trainer.language = 0; break;
                case 3: options.trainer.language = 6; break;
                case 4: options.trainer.language = 255; break;
                case 5: options.trainer.ot[0] = u'X'; break;
                case 6: options.trainer.female = !pk.otGender; break;
                default: options.source = static_cast<A::RecordSource>(255); break;
            }
            check(positive, game, options, Verdict::Incomplete);
        }
        // Different trainers and languages are legitimate trades, including
        // a changed SID with the same TID. No context shortcut rejects them.
        auto trade = complete; trade.trainer.sid ^= 1; trade.trainer.ot[0] = u'X';
        trade.trainer.language = 1;
        check(positive, game, trade, Verdict::NoProblemsFound);
        auto broken = positive; broken[0x40] ^= 1;
        assert(!check(broken, game, complete, Verdict::Invalid).identity);
        broken = positive; broken[4] = 1;
        assert(!check(broken, game, complete, Verdict::Invalid).identity);
        assert(!check(encrypted, game, complete, Verdict::Invalid).identity);
        assert(!check(positive, game, stored, Verdict::Invalid).identity);
        // Species zero cannot hide a well-formed but nonblank payload.
        auto ambiguous = positive; ambiguous[8] = ambiguous[9] = 0;
        refreshChecksum(ambiguous);
        const auto unknown = check(ambiguous, game, complete, Verdict::Incomplete);
        assert(!unknown.emptySlot && !unknown.identity && hasText(unknown.report,
            Legality::Severity::Warning, "not a native blank slot"));
        for (const auto& bytes : {std::vector<uint8_t>(136), C::encryptCandidate(std::vector<uint8_t>(136))}) {
            const auto empty = A::analyze(bytes, game);
            assert(empty.emptySlot && !empty.identity && empty.verdictLine == "Empty slot.");
            ++cases;
        }
        // Correlated hostile fields, with a correct storage checksum.
        bad[0x40] = static_cast<uint8_t>((bad[0x40] & ~6u) | 6u);
        bad[0x41] = 255; bad[0x5F] = 255;
        refreshChecksum(bad);
        check(bad, game, complete, Verdict::Invalid);
    }
    for (size_t size : {0u, 1u, 135u, 137u, 219u, 221u, 4096u}) {
        const auto malformed = check(std::vector<uint8_t>(size, 0xA5), "", {}, Verdict::Incomplete);
        assert(!malformed.emptySlot && !malformed.identity && malformed.report.coverage.structure == CoverageLevel::None);
    }
    std::cout << "Gen V source context: " << cases << " native/encoded/title/identity, missing and hostile witnesses, "
        "cross-field failures, native blanks and immutable input cases PASS\n";
}
