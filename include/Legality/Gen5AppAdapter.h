/**
 * Gen5AppAdapter.h - the app-facing entry for Gen V (PK5) legality.
 *
 * Takes the raw PK5 bytes a Gen V save slot holds and the app's exact source game id
 * ("black_nds", "white_nds", "black2_nds", "white2_nds") and returns the engine's
 * Legality::Report together with the lines a verdict overlay shows. It depends only on
 * Legality::analyzePk5 (Gen5Legality.h), not on any save or Pokemon type, so it can be
 * called from a passive PK5 panel with the slot's 136 (box) or 220 (party) bytes.
 *
 * Read-only contract: the record is only read, nothing is written back, no save is
 * touched, and the same input gives the same Result.
 *
 * Rendering rule (the false-completion hazard): Report::ok() is true for an Incomplete
 * report with no Warning, so a surface must never print "No problems found" from ok().
 * verdictLine() comes from Report::verdict():
 *   Verdict::NoProblemsFound  "No problems found by complete checks."
 *   Verdict::Incomplete, no problems
 *                             "Incomplete / Unknown: required checks or evidence are unresolved."
 *                             followed by one coverage row per area (coverageLines)
 *   Verdict::Incomplete, Warnings
 *                             "Incomplete / Unknown: N warning(s)." then the issues
 *   Verdict::Invalid          "Invalid: N issue(s)." then the issues
 *   native blank slot         "Empty slot." (no Pokemon to judge)
 * Info issues are never rendered as problems.
 */
#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "Legality/Gen5Legality.h"

namespace Legality::Gen5App {

/// Caller-owned source of the selected snapshot. This describes workspace
/// provenance, not authentication of a historical encounter or distribution.
enum class RecordSource : uint8_t { Unknown, OriginalSlot, StagedSlot };

struct RecordIdentity {
    uint32_t pid = 0;
    uint16_t species = 0, tid = 0, sid = 0, checksum = 0;
    uint8_t originGame = 0, language = 0, gender = 0;
    bool party = false;
    bool operator==(const RecordIdentity&) const = default;
};

/// PKHeX GameVersion id for the app's source game id: "white_nds" 20, "black_nds" 21,
/// "white2_nds" 22, "black2_nds" 23. Any other id gives 0 (not a Gen V game).
uint8_t versionFromSourceGameId(std::string_view sourceGameId) noexcept;

/// Save-side context. Missing context leaves a nonempty app report Incomplete.
/// Detached PK5 analysis without a save remains available through analyzePk5.
struct Options {
    /// True for the slot's stored bytes (encrypted and shuffled); false for a record the
    /// caller already decrypted.
    bool encrypted = true;
    /// Party or box slot when the caller knows it; Unknown otherwise.
    Pk5Slot slot = Pk5Slot::Unknown;
    /// The save's trainer, when the caller has it (set `present`).
    Pk5Trainer trainer{};
    RecordSource source = RecordSource::Unknown;
    bool stagedTrainer = false;
};

struct Result {
    Report report;
    Verdict verdict = Verdict::Incomplete;
    bool knownGame = false;                   // the source game id named a Gen V game
    bool emptySlot = false;                   // native blank record: nothing to judge
    RecordSource source = RecordSource::Unknown;
    bool stagedTrainer = false;
    /// Identity of the checked snapshot, only after its size/checksum/sanity
    /// pass. It never substitutes for the original save or an event receipt.
    std::optional<RecordIdentity> identity;
    std::string verdictLine;                  // first overlay line, see the header comment
    std::vector<std::string> coverageLines;   // "<area>: Complete|Partial|None", every area
    std::vector<std::string> issueLines;      // "Invalid: ..." / "Warning: ...", in report order

    /// The overlay lines in order: verdictLine, then the coverage rows whenever the report is
    /// Incomplete, then the issue lines.
    std::vector<std::string> lines() const;
};

/// Analyze one PK5 record from a save whose game is `sourceGameId`. A source game id that
/// is missing or not a Gen V game adds a Warning (source game coverage Partial).
/// Unknown slot, missing/malformed trainer or unknown record source likewise
/// leave the report Incomplete. Context failures never make a Pokemon Invalid.
Result analyze(std::span<const uint8_t> record, std::string_view sourceGameId, const Options& options = {});

/// True for the empty Report analyzePk5 returns for a native blank record.
bool isEmptySlot(const Report& report);

/// The verdict line for any Report (see the header comment); "Empty slot." for an empty slot.
std::string verdictLine(const Report& report);

/// One "<area>: <level>" row per coverage area, in the order the Report defines them.
std::vector<std::string> coverageLines(const Report& report);

} // namespace Legality::Gen5App
