// SPDX-License-Identifier: GPL-3.0-or-later
// Read-only PK6 transport/field structural reporting; no historical verdicts.
#include "Legality/Legality.h"
#include "Legality/Gen6NativePK6Evidence.h"
#include <cstdint>
#include <span>

namespace Legality {
Report analyzePk6Record(std::span<const std::uint8_t> raw,bool encrypted) {
    const auto native=Gen6NativePK6::inspect(raw,encrypted);
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
    using S=Gen6NativePK6::Status;
    switch(native.status) {
    case S::UnknownLength:
        note(Severity::Warning,CheckIdentifier::Structure,
            "Unsupported PK6 record length; transport is unverified");
        break;
    case S::InvalidSanity:
        note(Severity::Warning,CheckIdentifier::Structure,
            "PK6 sanity field mismatch; record framing or decryption may be wrong");
        break;
    case S::InvalidChecksum:
        note(Severity::Warning,CheckIdentifier::Checksum,
            "PK6 decrypted checksum mismatch; encoding or ciphertext may be wrong");
        break;
    case S::EmptySlot:
        break;
    case S::InvalidSpecies:
        note(Severity::Invalid,CheckIdentifier::Species,
            "Checksum-verified native PK6 species exceeds Generation VI National range");
        break;
    case S::InvalidNature:
        note(Severity::Invalid,CheckIdentifier::Stats,
            "Checksum-verified native PK6 nature exceeds 24");
        break;
    case S::InvalidEVPerStat:
        note(Severity::Invalid,CheckIdentifier::Stats,
            "Native Generation VI PK6 EV field exceeds 252");
        break;
    case S::InvalidEVTotal:
        note(Severity::Invalid,CheckIdentifier::Stats,
            "Native Generation VI PK6 EV total exceeds 510");
        break;
    case S::Parsed:
        report.coverage.structure=CoverageLevel::Complete;
        note(Severity::Info,CheckIdentifier::Structure,
             "Native PK6 checksum and basic fields verified; historical legality incomplete");
        break;
    }
    return report;
}
} // namespace Legality
