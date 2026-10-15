// L6 Stage 6: executable, audit-owned release-evidence protocol regression.
// This verifies receipt accounting only. It cannot attest original provenance,
// decide whole-Pokémon legality, or authorize a merge.
#include "tools/legality_l6_stage6_evidence.h"

#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <string_view>
#include <vector>

namespace Evidence = PokeBank::L6::Evidence;

namespace {

constexpr std::string_view kHead = "0123456789abcdef0123456789abcdef01234567";
constexpr std::string_view kTree = "89abcdef0123456789abcdef0123456789abcdef";
constexpr std::string_view kInvocation = "python3 tools/audit_l6_stage6_evidence.py";

Evidence::Receipt receipt(Evidence::Lane lane, Evidence::Compiler compiler,
                          Evidence::Mode mode, std::uint32_t programs) {
    return {lane, compiler, mode, kHead, kTree, kInvocation, true, true, false,
            false, true, true, 0, programs, programs};
}

std::vector<Evidence::Receipt> complete(std::uint32_t programs) {
    std::vector<Evidence::Receipt> receipts;
    receipts.reserve(Evidence::kRequired);
    for (std::size_t lane = 0; lane < Evidence::kLanes; ++lane) {
        for (std::size_t compiler = 0; compiler < Evidence::kCompilers; ++compiler) {
            for (std::size_t mode = 0; mode < Evidence::kModes; ++mode) {
                receipts.push_back(receipt(static_cast<Evidence::Lane>(lane),
                                           static_cast<Evidence::Compiler>(compiler),
                                           static_cast<Evidence::Mode>(mode), programs));
            }
        }
    }
    return receipts;
}

void expectBlocked(const Evidence::Result& result, unsigned& checks) {
    assert(result.outcome == Evidence::Outcome::Blocked);
    ++checks;
    assert(!result.acceptanceAuthorized && !result.wholePokemonLegal &&
           !result.historicalOriginAuthenticated);
    ++checks;
}

} // namespace

int main() {
    unsigned checks = 0;
    // Exercise every lane/compiler/mode tuple across varied nonzero program
    // counts. Each mutation changes just one receipt, proving the protocol is
    // fail-closed without relying on a test order or a synthetic happy path.
    for (unsigned round = 1; round <= 128; ++round) {
        const auto programs = 1u + (round % 31u);
        const auto good = complete(programs);
        const auto verified = Evidence::evaluate(kHead, kTree, good);
        assert(verified.outcome == Evidence::Outcome::NativeMatrixVerified);
        ++checks;
        assert(verified.passed == Evidence::kRequired && verified.failed == 0 &&
               verified.missing == 0 && verified.stale == 0 && verified.incomplete == 0 &&
               verified.missingSourcePins == 0 && verified.duplicates == 0 &&
               verified.malformed == 0);
        ++checks;
        assert(!verified.acceptanceAuthorized && !verified.wholePokemonLegal &&
               !verified.wholePokemonInvalid && !verified.historicalOriginAuthenticated);
        ++checks;

        for (std::size_t index = 0; index < good.size(); ++index) {
            auto changed = good;
            changed.erase(changed.begin() + static_cast<std::ptrdiff_t>(index));
            const auto missing = Evidence::evaluate(kHead, kTree, changed);
            expectBlocked(missing, checks);
            assert(missing.missing == 1);
            ++checks;

            changed = good;
            changed[index].head = "fedcba9876543210fedcba9876543210fedcba98";
            const auto stale = Evidence::evaluate(kHead, kTree, changed);
            expectBlocked(stale, checks);
            assert(stale.stale == 1);
            ++checks;

            changed = good;
            changed[index].sourcePinsVerified = false;
            const auto unpinned = Evidence::evaluate(kHead, kTree, changed);
            expectBlocked(unpinned, checks);
            assert(unpinned.missingSourcePins == 1);
            ++checks;

            changed = good;
            changed[index].dryRun = true;
            const auto dryRun = Evidence::evaluate(kHead, kTree, changed);
            expectBlocked(dryRun, checks);
            assert(dryRun.failed == 1);
            ++checks;

            changed = good;
            changed[index].timedOut = true;
            const auto timedOut = Evidence::evaluate(kHead, kTree, changed);
            expectBlocked(timedOut, checks);
            assert(timedOut.failed == 1);
            ++checks;

            changed = good;
            changed[index].exitCode = 19;
            const auto failed = Evidence::evaluate(kHead, kTree, changed);
            expectBlocked(failed, checks);
            assert(failed.failed == 1);
            ++checks;

            changed = good;
            --changed[index].executedPrograms;
            const auto incomplete = Evidence::evaluate(kHead, kTree, changed);
            expectBlocked(incomplete, checks);
            assert(incomplete.incomplete == 1);
            ++checks;

            changed = good;
            changed.push_back(changed[index]);
            const auto duplicate = Evidence::evaluate(kHead, kTree, changed);
            assert(duplicate.outcome == Evidence::Outcome::Malformed && duplicate.duplicates == 1);
            ++checks;
        }
    }

    auto malformed = complete(1);
    malformed.front().lane = Evidence::Lane::Count;
    assert(Evidence::evaluate(kHead, kTree, malformed).outcome == Evidence::Outcome::Malformed);
    ++checks;
    malformed = complete(1);
    malformed.front().invocation = "";
    assert(Evidence::evaluate(kHead, kTree, malformed).outcome == Evidence::Outcome::Malformed);
    ++checks;
    malformed = complete(1);
    malformed.front().requiredPrograms = 0;
    assert(Evidence::evaluate(kHead, kTree, malformed).outcome == Evidence::Outcome::Malformed);
    ++checks;
    malformed = complete(1);
    malformed.front().executedPrograms = 2;
    assert(Evidence::evaluate(kHead, kTree, malformed).outcome == Evidence::Outcome::Malformed);
    ++checks;
    malformed = complete(1);
    malformed.front().tree = "89ABCDEF0123456789abcdef0123456789abcdef";
    assert(Evidence::evaluate(kHead, kTree, malformed).outcome == Evidence::Outcome::Malformed);
    ++checks;
    malformed.assign(Evidence::kLimit + 1, receipt(Evidence::Lane::Encounter,
                                                     Evidence::Compiler::GCC,
                                                     Evidence::Mode::Normal, 1));
    assert(Evidence::evaluate(kHead, kTree, malformed).outcome == Evidence::Outcome::Malformed);
    ++checks;

    assert(checks == 67974);
    std::cout << "L6 Stage 6 release evidence protocol: " << checks
              << " PASS (synthetic receipts, no acceptance authorization)\n";
}
