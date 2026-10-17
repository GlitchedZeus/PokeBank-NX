// Gen5AppAdapter.cpp - see include/Legality/Gen5AppAdapter.h.
#include "Legality/Gen5AppAdapter.h"

#include <utility>

namespace Legality::Gen5App {

uint8_t versionFromSourceGameId(std::string_view id) noexcept {
    if (id == "white_nds") return 20;
    if (id == "black_nds") return 21;
    if (id == "white2_nds") return 22;
    if (id == "black2_nds") return 23;
    return 0;
}

namespace {

const char* levelName(CoverageLevel level) {
    switch (level) {
        case CoverageLevel::Complete: return "Complete";
        case CoverageLevel::Partial: return "Partial";
        case CoverageLevel::None: return "None";
    }
    return "None";
}

int countSeverity(const Report& report, Severity severity) {
    int n = 0;
    for (const auto& issue : report.issues)
        if (issue.severity == severity) ++n;
    return n;
}

} // namespace

bool isEmptySlot(const Report& report) {
    // analyzePk5 returns a default Report for an all-zero or species-0 record; every other
    // path sets the source game coverage or adds an issue.
    const auto& c = report.coverage;
    return report.issues.empty() && c.structure == CoverageLevel::Complete && c.sourceGame == CoverageLevel::None &&
           c.encounter == CoverageLevel::None;
}

std::string verdictLine(const Report& report) {
    if (isEmptySlot(report)) return "Empty slot.";
    switch (report.verdict()) {
        case Verdict::NoProblemsFound:
            return "No problems found by complete checks.";
        case Verdict::Invalid:
            return "Invalid: " + std::to_string(countSeverity(report, Severity::Invalid)) + " issue(s).";
        case Verdict::Incomplete:
            break;
    }
    const int warnings = countSeverity(report, Severity::Warning);
    if (warnings == 0) return "No problems found by the checks that ran. Coverage is incomplete.";
    return "Checks incomplete: " + std::to_string(warnings) + " warning(s).";
}

std::vector<std::string> coverageLines(const Report& report) {
    const auto& c = report.coverage;
    const std::pair<const char*, CoverageLevel> areas[] = {
        {"structure", c.structure}, {"source game", c.sourceGame}, {"internal", c.internal},
        {"moves", c.moves},         {"encounter", c.encounter},    {"event/gift", c.eventGift},
        {"PID/RNG", c.pidRng},      {"egg/breeding", c.eggBreeding}, {"transfer", c.transfer},
    };
    std::vector<std::string> out;
    out.reserve(sizeof(areas) / sizeof(areas[0]));
    for (const auto& [name, level] : areas) out.push_back(std::string(name) + ": " + levelName(level));
    return out;
}

std::vector<std::string> Result::lines() const {
    std::vector<std::string> out;
    out.push_back(verdictLine);
    if (verdict == Verdict::Incomplete && issueLines.empty() && !emptySlot)
        out.insert(out.end(), coverageLines.begin(), coverageLines.end());
    out.insert(out.end(), issueLines.begin(), issueLines.end());
    return out;
}

Result analyze(std::span<const uint8_t> record, std::string_view sourceGameId, const Options& options) {
    Pk5Input input;
    input.encrypted = options.encrypted;
    input.game = versionFromSourceGameId(sourceGameId);
    input.slot = options.slot;
    input.trainer = options.trainer;
    Result result;
    result.knownGame = input.game != 0;
    result.report = analyzePk5(record, input);
    result.emptySlot = isEmptySlot(result.report);
    if (!sourceGameId.empty() && !result.knownGame && !result.emptySlot) {
        // Not a Gen V save: the record was still checked as a PK5, but the save context is
        // missing, so the report must not read as complete. An empty report is an empty
        // slot and stays empty.
        result.report.coverage.sourceGame = CoverageLevel::Partial;
        result.report.issues.push_back({Severity::Warning,
                                        "Source game \"" + std::string(sourceGameId) + "\" is not a Gen V game; the save context was not used",
                                        CheckIdentifier::SourceGame});
    }
    result.verdict = result.report.verdict();
    result.verdictLine = Gen5App::verdictLine(result.report);
    result.coverageLines = Gen5App::coverageLines(result.report);
    for (const auto& issue : result.report.issues) {
        if (issue.severity == Severity::Invalid) result.issueLines.push_back("Invalid: " + issue.text);
        else if (issue.severity == Severity::Warning) result.issueLines.push_back("Warning: " + issue.text);
    }
    return result;
}

} // namespace Legality::Gen5App
