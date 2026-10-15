/**
 * Gen4LegalityHook.cpp - folds the Gen IV PK4 analyzer into Legality::analyze().
 *
 * Kept apart from Gen4Legality.cpp so the analyzer itself does not depend on the shared
 * Pokemon model: the hook only reads the decrypted bytes of a native PK4 record.
 */
#include "Legality/Gen4Legality.h"

#include <cstdint>
#include <span>

#include "Pokemon/Pokemon.h"
#include "Pokemon/Pokemon4ReadOnly.h"

namespace Legality {

void applyGen4Analyzer(Report& report, const Pokemon::Pokemon& pk, std::string_view exactSourceGameId) {
    const Pokemon::Pokemon4ReadOnly* native = pk.nativePk4ReadOnly();
    if (native == nullptr || !native->sizeValid()) return;
    const auto bytes = native->decryptedBytes();
    const std::span<const uint8_t> data(reinterpret_cast<const uint8_t*>(bytes.data()), bytes.size());
    const Report g = analyzePk4(data, exactSourceGameId);
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
