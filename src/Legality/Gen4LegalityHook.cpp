/**
 * Gen4LegalityHook.cpp - folds the Gen IV PK4 analyzer into Legality::analyze().
 *
 * Kept apart from Gen4Legality.cpp so the analyzer itself does not depend on the shared
 * Pokemon model: the hook only reads the decrypted bytes of a native PK4 record.
 */
#include "Legality/Gen4Legality.h"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>

#include "Names/MoveNames.h"
#include "Pokemon/Pokemon.h"
#include "Pokemon/Pokemon4ReadOnly.h"

namespace Legality {

namespace {

// analyze()'s audited Gen IV move-pool Warning (Legality.cpp, exact Gen IV save source).
constexpr std::string_view kAuditedPoolWarning =
    "Move is outside the audited direct/transfer pool for this exact Gen IV game: ";

// The audited pools hold the level-up, egg, tutor and TM/HM moves of the current species
// and its pre-evolutions in D/P, Pt and HG/SS; they cannot hold the moves an encounter
// itself supplies (a Mystery Gift's or Pokewalker Pokemon's moves, a Pal Park transfer's
// Generation III moves). When the analyzer matched an encounter for this very record,
// found nothing Invalid and verified every move against that encounter and the evolution
// chain, the Warning is superseded; it stays as an Info note saying how the move was
// verified. Anything else (no matched encounter, an unverified move, an Invalid finding)
// keeps the Warning as it is.
void yieldVerifiedMoves(Report& report, const Report& analyzer, const Gen4::MoveEvidence& evidence) {
    if (analyzer.hasInvalid() || !evidence.allMovesVerified()) return;
    for (Issue& issue : report.issues) {
        if (issue.severity != Severity::Warning || issue.identifier != CheckIdentifier::Moves) continue;
        const std::string_view text = issue.text;
        if (!text.starts_with(kAuditedPoolWarning)) continue;
        const std::string_view name = text.substr(kAuditedPoolWarning.size());
        for (std::size_t i = 0; i < 4; ++i) {
            const uint16_t move = evidence.moves[i];
            if (move == 0 || name != Names::getMoveName(move)) continue;
            issue.severity = Severity::Info;
            issue.text += std::string("; the Gen IV encounter and move search verified it for this record (") +
                          evidence.source[i] + ")";
            break;
        }
    }
}

} // namespace

void applyGen4Analyzer(Report& report, const Pokemon::Pokemon& pk, std::string_view exactSourceGameId) {
    const Pokemon::Pokemon4ReadOnly* native = pk.nativePk4ReadOnly();
    if (native == nullptr) return;
    const auto bytes = native->decryptedBytes();
    if (bytes.size() != 136 && bytes.size() != 236) return; // structure problems are analyze()'s
    const std::span<const uint8_t> data(reinterpret_cast<const uint8_t*>(bytes.data()), bytes.size());
    Gen4::MoveEvidence evidence;
    const Report g = analyzePk4(data, exactSourceGameId, &evidence);
    yieldVerifiedMoves(report, g, evidence);
    const bool existingClean = report.problemCount() == 0;
    for (const Issue& issue : g.issues) {
        if (issue.severity == Severity::Info) continue;
        bool dup = false;
        for (const Issue& i : report.issues) if (i.severity == issue.severity && i.text == issue.text) dup = true;
        if (!dup) report.issues.push_back(issue);
    }
    if (existingClean && g.verdict() == Verdict::NoProblemsFound) {
        CoverageSummary& c = report.coverage;
        c.structure = c.sourceGame = c.internal = c.moves = c.encounter = CoverageLevel::Complete;
        c.eventGift = c.pidRng = c.eggBreeding = c.transfer = CoverageLevel::Complete;
    }
}

} // namespace Legality
