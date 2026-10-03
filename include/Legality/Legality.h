/**
 * Legality.h - PKSE legality checker (Layers 1+2, informational).
 *
 * analyze() runs a set of structural (Layer 1) and internal-consistency (Layer 2)
 * checks against a decrypted Pokemon and returns human-readable issues. It is
 * INFORMATIONAL only (no auto-fix) and deliberately conservative: checks that need
 * data PKSE does not have (per-species legal abilities, learnsets, form counts,
 * gender ratios, encounter tables) are NOT performed. A clean report therefore means
 * "no problems found", never a guarantee of full legality.
 *
 * No RTTI / no exceptions: every check calls base Pokemon virtuals; per-gen
 * differences branch on the game group / capability flags, never on concrete type.
 */

#ifndef LEGALITY_LEGALITY_H
#define LEGALITY_LEGALITY_H

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "Enums/GameVersion.h"
#include "Legality/LegalityContext.h"

namespace Pokemon { class Pokemon; }  // fwd decl — no heavy include

namespace Legality {

    enum class Severity : uint8_t { Info, Warning, Invalid };

    enum class CheckIdentifier : uint8_t {
        Structure,
        SourceGame,
        Species,
        Stats,
        Ability,
        Moves,
        Items,
        Origin,
        Encounter,
        EventGift,
        PidRng,
        Egg,
        Transfer,
        Trainer,
        Checksum,
        Misc,
    };

    enum class Verdict : uint8_t {
        Invalid,
        NoProblemsFound,
        Incomplete,
    };

    struct Issue {
        Severity severity;
        std::string text;
        CheckIdentifier identifier = CheckIdentifier::Misc;
    };

    struct CoverageSummary {
        CoverageLevel structure = CoverageLevel::Complete;
        CoverageLevel sourceGame = CoverageLevel::None;
        CoverageLevel internal = CoverageLevel::Partial;
        CoverageLevel moves = CoverageLevel::Partial;
        CoverageLevel encounter = CoverageLevel::None;
        CoverageLevel eventGift = CoverageLevel::None;
        CoverageLevel pidRng = CoverageLevel::None;
        CoverageLevel eggBreeding = CoverageLevel::None;
        CoverageLevel transfer = CoverageLevel::None;
    };

    struct Report {
        std::vector<Issue> issues;
        CoverageSummary coverage{};

        /// Number of Warning+Invalid issues (Info notes are not counted as problems).
        int problemCount() const noexcept {
            int n = 0;
            for (const auto& i : issues) if (i.severity != Severity::Info) ++n;
            return n;
        }
        bool ok() const noexcept { return problemCount() == 0; }

        bool hasInvalid() const noexcept {
            for (const auto& i : issues)
                if (i.severity == Severity::Invalid) return true;
            return false;
        }

        Verdict verdict() const noexcept {
            if (hasInvalid()) return Verdict::Invalid;
            const bool complete =
                coverage.structure == CoverageLevel::Complete &&
                coverage.sourceGame == CoverageLevel::Complete &&
                coverage.internal == CoverageLevel::Complete &&
                coverage.moves == CoverageLevel::Complete &&
                coverage.encounter == CoverageLevel::Complete &&
                coverage.eventGift == CoverageLevel::Complete &&
                coverage.pidRng == CoverageLevel::Complete &&
                coverage.eggBreeding == CoverageLevel::Complete &&
                coverage.transfer == CoverageLevel::Complete;
            return complete && problemCount() == 0
                ? Verdict::NoProblemsFound
                : Verdict::Incomplete;
        }
    };

    struct Context {
        Enums::GameVersion originGroup;
        std::string_view exactSourceGameId{};
    };

    /// Preferred context-aware API for the Gen I-IV legality engine.
    Report analyze(const Pokemon::Pokemon& pk, const Context& context);

    /// Analyze a decrypted Pokemon and return its legality issues (informational).
    /// originGroup = the save's format group (Trainer::getGameGroup()). Returns an
    /// empty report for an empty slot (species 0).
    /// exactSourceGameId is optional container/save context (for example "ruby_gba"). It is
    /// deliberately separate from the Pokemon format group: all PK3 records share one entity
    /// implementation, while Ruby/Sapphire/Emerald/FRLG have different native move pools.
    /// Pass an empty id for Bank records or callers that do not own exact source context.
    Report analyze(const Pokemon::Pokemon& pk, Enums::GameVersion originGroup,
                   std::string_view exactSourceGameId = {});
}

#endif
