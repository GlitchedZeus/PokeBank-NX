// SPDX-License-Identifier: GPL-3.0-or-later
// Native PK7 envelope/field evidence is not historical legality certification.
#include "Legality/Legality.h"
#include "Legality/Gen7NativePK7Evidence.h"
#include <cstdint>
#include <span>
namespace Legality {
Report analyzePk7Record(std::span<const std::uint8_t> raw,bool encrypted) {
    const auto native=Gen7NativePK7::inspect(raw,encrypted);
    Report report{};
    report.coverage.structure=CoverageLevel::Partial;
    report.coverage.sourceGame=CoverageLevel::None;
    report.coverage.internal=CoverageLevel::Partial;
    report.coverage.moves=CoverageLevel::None;
    report.coverage.encounter=CoverageLevel::None;
    report.coverage.eventGift=CoverageLevel::None;
    report.coverage.pidRng=CoverageLevel::None;
    report.coverage.eggBreeding=CoverageLevel::None;
    report.coverage.transfer=CoverageLevel::None;
    auto note=[&](Severity severity,CheckIdentifier id,const char* detail) {
        report.issues.push_back({severity,detail,id});
    };
    using S=Gen7NativePK7::Status;
    switch(native.status) {
    case S::UnknownLength:
        note(Severity::Warning,CheckIdentifier::Structure,
             "Unsupported PK7 record length; format is unverified");break;
    case S::InvalidSanity:
        note(Severity::Warning,CheckIdentifier::Structure,
             "PK7 sanity field mismatch; source encoding is unverified");break;
    case S::InvalidChecksum:
        note(Severity::Warning,CheckIdentifier::Checksum,
             "PK7 checksum mismatch; source encryption is unverified");break;
    case S::EmptySlot:break;
    case S::InvalidSpecies:
        note(Severity::Invalid,CheckIdentifier::Species,
             "Checksum-verified native PK7 species exceeds Ultra Sun/Moon range");break;
    case S::InvalidNature:
        note(Severity::Invalid,CheckIdentifier::Stats,
             "Checksum-verified native PK7 nature exceeds 24");break;
    case S::InvalidEVPerStat:
        note(Severity::Invalid,CheckIdentifier::Stats,
             "Native PK7 EV field exceeds 252");break;
    case S::InvalidEVTotal:
        note(Severity::Invalid,CheckIdentifier::Stats,
             "Native PK7 EV total exceeds 510");break;
    case S::Parsed:
        report.coverage.structure=CoverageLevel::Complete;
        note(Severity::Info,CheckIdentifier::Structure,
             "Native PK7 checksum and basic fields verified; Gen VII historical legality incomplete");break;
    }
    return report;
}
} // namespace Legality
