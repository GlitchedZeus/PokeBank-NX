// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include "Legality/Legality.h"
#include <initializer_list>
namespace Legality {
/// An authoritative partial analysis must not inherit Complete from an earlier
/// report merely because it emitted no Warning text. This only downgrades;
/// promotion still requires the analyzer's explicit full certification gate.
inline void retainIncompleteCoverage(CoverageSummary& target, const CoverageSummary& source) noexcept {
    for (auto member : {&CoverageSummary::structure, &CoverageSummary::sourceGame,
            &CoverageSummary::internal, &CoverageSummary::moves, &CoverageSummary::encounter,
            &CoverageSummary::eventGift, &CoverageSummary::pidRng,
            &CoverageSummary::eggBreeding, &CoverageSummary::transfer}) {
        if (static_cast<unsigned>(source.*member) < static_cast<unsigned>(target.*member))
            target.*member = source.*member;
    }
}
} // namespace Legality
