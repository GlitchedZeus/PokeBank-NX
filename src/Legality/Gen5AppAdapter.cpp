// Gen5AppAdapter.cpp - see include/Legality/Gen5AppAdapter.h.
#include "Legality/Gen5AppAdapter.h"
#include "Legality/ReportPresentation.h"
#include "Encryption/Encryption5.h"

#include <algorithm>
#include <utility>

namespace Legality::Gen5App {

uint8_t versionFromSourceGameId(std::string_view id) noexcept {
    if (id == "white_nds") return 20;
    if (id == "black_nds") return 21;
    if (id == "white2_nds") return 22;
    if (id == "black2_nds") return 23;
    return 0;
}

bool isEmptySlot(const Report& report) {
    // analyzePk5 returns a default Report for an all-zero or species-0 record; every other
    // path sets the source game coverage or adds an issue.
    const auto& c = report.coverage;
    return report.issues.empty() && c.structure == CoverageLevel::Complete && c.sourceGame == CoverageLevel::None &&
           c.encounter == CoverageLevel::None;
}

std::string verdictLine(const Report& report) {
    if (isEmptySlot(report)) return "Empty slot.";
    return Presentation::headline(report);
}

std::vector<std::string> coverageLines(const Report& report) {
    return Presentation::coverageRows(report);
}

std::vector<std::string> Result::lines() const {
    std::vector<std::string> out;
    out.push_back(verdictLine);
    if (verdict == Verdict::Incomplete && !emptySlot)
        out.insert(out.end(), coverageLines.begin(), coverageLines.end());
    out.insert(out.end(), issueLines.begin(), issueLines.end());
    return out;
}

Result analyze(std::span<const uint8_t> record, std::string_view sourceGameId, const Options& options) {
    Pk5Input input;
    input.encrypted = options.encrypted;
    input.game = versionFromSourceGameId(sourceGameId);
    const bool shape = (options.slot == Pk5Slot::Box && record.size() == 136) ||
                       (options.slot == Pk5Slot::Party && record.size() == 220);
    // Do not manufacture a boxed-PP/form failure by trusting a contradictory
    // Box label on an actual native party record. The context still warns.
    input.slot = shape ? options.slot : Pk5Slot::Unknown;
    input.trainer = options.trainer;
    Result result;
    result.knownGame = input.game != 0;
    result.source = options.source == RecordSource::OriginalSlot || options.source == RecordSource::StagedSlot
        ? options.source : RecordSource::Unknown;
    result.stagedTrainer = options.stagedTrainer;
    result.report = analyzePk5(record, input);
    result.emptySlot = isEmptySlot(result.report);
    if (!result.emptySlot) {
        auto unknown = [&](CoverageLevel& coverage, CheckIdentifier area, std::string text) {
            // A context warning must not hide a stronger existing format failure.
            if (coverage == CoverageLevel::Complete) coverage = CoverageLevel::Partial;
            result.report.issues.push_back({Severity::Warning, std::move(text), area});
        };
        if (!result.knownGame) {
            unknown(result.report.coverage.sourceGame, CheckIdentifier::SourceGame,
                sourceGameId.empty() ? "Source game is missing; the save context is unresolved"
                    : "Source game \"" + std::string(sourceGameId) + "\" is not a Gen V game; the save context was not used");
        }
        if (!shape)
            unknown(result.report.coverage.structure, CheckIdentifier::Structure,
                options.slot == Pk5Slot::Unknown ? "Party or Box source slot is unknown"
                    : "Source slot and native PK5 record size disagree");
        const auto& trainer = options.trainer;
        const auto end = std::find_if(trainer.ot.begin(), trainer.ot.end(),
            [](uint16_t ch) { return ch == 0 || ch == 0xFFFF; });
        const bool trainerKnown = trainer.present && end != trainer.ot.begin() && end != trainer.ot.end() &&
            trainer.language >= 1 && trainer.language <= 7 && trainer.language != 6;
        if (!trainerKnown)
            unknown(result.report.coverage.sourceGame, CheckIdentifier::SourceGame,
                "Save trainer context is missing or malformed; source identity is unresolved");
        if (options.source != RecordSource::OriginalSlot && options.source != RecordSource::StagedSlot)
            unknown(result.report.coverage.sourceGame, CheckIdentifier::SourceGame,
                "Original or staged record provenance is unknown");
        if (record.size() == 136 || record.size() == 220) {
            const auto plain = options.encrypted ? PokeVault::Integration::Gen5::Crypto::decrypt(record)
                                                 : std::vector<uint8_t>(record.begin(), record.end());
            Gen5::Pk5 pk;
            if (Gen5::read(plain, pk) && pk.species > 0 && pk.species <= 649 && pk.sanity == 0 &&
                Gen5::checksum(plain) == pk.checksum)
                result.identity = RecordIdentity{pk.pid, pk.species, pk.tid, pk.sid, pk.checksum,
                    pk.version, pk.language, pk.gender, pk.party};
        }
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
