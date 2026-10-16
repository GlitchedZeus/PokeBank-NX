// Uses the real production event projection and the L5 catalog via stdin.
#include "Legality/Gen4EventTemplate.h"
#include "tools/legality_l6_stage6_qualification.h"
#include <cassert>
#include <iostream>
#include <string_view>
#include <vector>

namespace Q = PokeBank::L6::Qualification;
constexpr std::string_view revision = "6501f0ab46e8f8ca048539dbaf8cae8cb104e722";
constexpr std::string_view tree = "0123456789abcdef0123456789abcdef01234567";
constexpr std::string_view hash = "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef";
unsigned checks = 0;
void check(bool success) { ++checks; assert(success); }
Q::Policy policy() {
    Q::Policy p{revision, tree, {}};
    p.requiredWitnesses.fill(Q::kWitnesses);
    return p;
}
Q::Cell good(unsigned game, unsigned family) {
    Q::Cell c;
    c.game = c.originalGame = game; c.family = family;
    c.evidence = Q::Evidence::OriginalNative;
    c.referenceRevision = revision; c.applicationTree = tree;
    c.specimenSha256 = c.runtimeSpecimenSha256 = c.runtimeLogSha256 = c.historySha256 = hash;
    c.witnessed = Q::kWitnesses;
    c.nativeDecoded = c.originalReceiptVerified = c.runtimeExecuted = c.runtimeCompleted = true;
    c.comparisonAgrees = c.referenceUsePermitted = true; c.exitCode = 0;
    return c;
}
int main(int argc, char**) {
    const auto p = policy();
    if (argc > 1) {
        // Integration mode: consume catalog indices and projection identities.
        // No receipt/provenance assertions are inferred from those identities.
        std::vector<Q::Cell> cells;
        for (unsigned i = 0; i < Q::kCells; ++i) {
            Q::Cell c;
            if (!(std::cin >> c.game >> c.family)) return 2;
            c.evidence = Q::Evidence::SourceModel;
            cells.push_back(c);
        }
        const auto result = Q::evaluate(p, cells);
        check(result.state == Q::State::Unknown && result.unknown == Q::kCells &&
              !result.missing && !result.malformed && !result.duplicates);
        for (unsigned i = 0; i < Q::kEvents; ++i) {
            Q::EventReceipt row;
            if (!(std::cin >> row.index >> row.species >> row.tid >> row.sid >> row.card)) return 2;
            const auto& t = Legality::Gen4EventTemplate::kGen4EventTemplates[i];
            check(row.index == i && row.species == t.species && row.tid == t.tid &&
                  row.sid == t.sid && row.card == t.cardId);
            row.projectionRevision = revision;
            check(Q::qualifyEvent(row, row) == Q::State::Unknown);
        }
        std::string trailing;
        if (std::cin >> trailing) return 2;
        std::cout << "L6 Stage 6 catalog integration: 336 cells Unknown, 247 events Unknown; "
                  << checks << " PASS\n";
        return 0;
    }
    std::vector<Q::Cell> all;
    for (unsigned game = 0; game < Q::kGames; ++game) {
        for (unsigned family = 0; family < Q::kFamilies; ++family) {
            const auto original = good(game, family);
            all.push_back(original);
            check(Q::qualify(p, original) == Q::State::ReceiptConsistent);
            for (const auto kind : {Q::Evidence::Missing, Q::Evidence::Synthetic, Q::Evidence::SourceModel}) {
                auto c = original; c.evidence = kind;
                check(Q::qualify(p, c) == Q::State::Unknown);
            }
            for (unsigned bit = 1; bit <= Q::Transfer; bit <<= 1) {
                auto c = original; c.witnessed &= ~bit;
                check(Q::qualify(p, c) == Q::State::Unknown);
            }
            for (unsigned mutation = 0; mutation < 17; ++mutation) {
                auto c = original;
                switch (mutation) {
                case 0: c.referenceRevision = tree; break;
                case 1: c.applicationTree = revision; break;
                case 2: c.specimenSha256 = ""; break;
                case 3: c.runtimeSpecimenSha256 = ""; break;
                case 4: c.runtimeLogSha256 = ""; break;
                case 5: c.historySha256 = ""; break;
                case 6: c.originalGame = (game + 1) % Q::kGames; break;
                case 7: c.nativeDecoded = false; break;
                case 8: c.originalReceiptVerified = false; break;
                case 9: c.runtimeExecuted = false; break;
                case 10: c.runtimeCompleted = false; break;
                case 11: c.timedOut = true; break;
                case 12: c.exitCode = 1; break;
                case 13: c.comparisonAgrees = false; break;
                case 14: c.referenceUsePermitted = false; break;
                case 15: c.runtimeLogSha256 = std::string_view(hash.data(), 63); break;
                case 16: c.specimenSha256 = c.runtimeSpecimenSha256 =
                    "0000000000000000000000000000000000000000000000000000000000000000"; break;
                }
                check(Q::qualify(p, c) == Q::State::Unknown);
            }
        }
    }
    const auto complete = Q::evaluate(p, all);
    check(complete.state == Q::State::ReceiptConsistent && complete.consistent == Q::kCells);
    check(!complete.authenticatedParity && !complete.acceptanceAuthorized);
    for (unsigned i = 0; i < Q::kCells; ++i) {
        auto broken = all; broken[i] = broken[(i + 1) % Q::kCells];
        check(Q::evaluate(p, broken).state == Q::State::Malformed);
        broken = all; broken.erase(broken.begin() + i);
        check(Q::evaluate(p, broken).state == Q::State::Unknown);
    }
    for (unsigned value = Q::kGames; value < 256; ++value) {
        auto c = good(value, 0);
        check(Q::qualify(p, c) == Q::State::Malformed);
    }
    for (unsigned i = 0; i < Q::kEvents; ++i) {
        const auto& t = Legality::Gen4EventTemplate::kGen4EventTemplates[i];
        Q::EventReceipt e{i, t.species, t.tid, t.sid, t.cardId, Q::Evidence::OriginalNative,
                          revision, hash, hash, true, true, true, false};
        check(Q::qualifyEvent(e, e) == Q::State::ReceiptConsistent);
        for (unsigned mutation = 0; mutation < 12; ++mutation) {
            auto changed = e;
            switch (mutation) {
            case 0: changed.species ^= 1; break;
            case 1: changed.tid ^= 1; break;
            case 2: changed.sid ^= 1; break;
            case 3: changed.card ^= 1; break;
            case 4: changed.index = (i + 1) % Q::kEvents; break;
            case 5: changed.projectionRevision = tree; break;
            case 6: changed.archiveSha256 = ""; break;
            case 7: changed.releaseReceiptSha256 = ""; break;
            case 8: changed.originalReceiptVerified = false; break;
            case 9: changed.archiveDigestVerified = false; break;
            case 10: changed.reviewPermitted = false; break;
            case 11: changed.evidence = Q::Evidence::Synthetic; break;
            }
            check(Q::qualifyEvent(e, changed) == Q::State::Unknown);
        }
    }
    assert(checks == 12525);
    std::cout << "L6 Stage 6 parity qualification: " << checks
              << " PASS (synthetic receipt mutations; no PKHeX execution)\n";
}
