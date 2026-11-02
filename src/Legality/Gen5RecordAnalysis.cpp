// Native Gen V structural analyzer. This is separate from the main app.
#include "Legality/Legality.h"
#include "Legality/Gen5ProductNativeEvidence.h"
#include <cstdint>
#include <span>

namespace Legality {
Report analyzePk5Record(std::span<const uint8_t> encrypted) {
    using namespace Gen5ProductNativeEvidence;
    const auto evidence=inspect(encrypted);
    Report report{};
    report.coverage.sourceGame=CoverageLevel::None;
    report.coverage.internal=CoverageLevel::Partial;
    report.coverage.moves=CoverageLevel::None;
    report.coverage.encounter=CoverageLevel::None;
    report.coverage.eventGift=CoverageLevel::None;
    report.coverage.pidRng=CoverageLevel::None;
    report.coverage.eggBreeding=CoverageLevel::None;
    report.coverage.transfer=CoverageLevel::None;
    report.coverage.structure=CoverageLevel::Partial;
    auto issue=[&](Severity severity, CheckIdentifier kind, const char* description) {
        report.issues.push_back({severity,description,kind});
    };
    switch(evidence.status) {
    case Status::UnknownLength:
        issue(Severity::Warning,CheckIdentifier::Structure,
              "Not a supported 136/220-byte PK5 record; format unverified");
        break;
    case Status::InvalidSanity:
        issue(Severity::Invalid,CheckIdentifier::Structure,
              "PK5 native sanity field is not zero");
        break;
    case Status::InvalidChecksum:
        issue(Severity::Invalid,CheckIdentifier::Checksum,
              "PK5 decrypted storage payload does not match its checksum");
        break;
    case Status::InvalidSpecies:
        issue(Severity::Invalid,CheckIdentifier::Species,
              "Checksum-verified PK5 species exceeds Gen V native 649 ceiling");
        break;
    case Status::EmptySlot:
        // An empty slot is not an authenticated Pokemon.
        break;
    case Status::InvalidNature:
        issue(Severity::Invalid,CheckIdentifier::Stats,
              "Native PK5 stored nature exceeds the 25-nature domain");
        break;
    case Status::InvalidEVTotal:
        issue(Severity::Invalid,CheckIdentifier::Stats,
              "Native PK5 stored EV total exceeds 510");
        break;
    case Status::Parsed:
        report.coverage.structure=CoverageLevel::Complete;
        issue(Severity::Info,CheckIdentifier::Structure,
              "PK5 checksum and intrinsic fields verified; historical legality is incomplete");
        break;
    }
    return report;
}
} // namespace Legality
