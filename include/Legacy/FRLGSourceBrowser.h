#ifndef POKEBANK_LEGACY_FRLG_SOURCE_BROWSER_H
#define POKEBANK_LEGACY_FRLG_SOURCE_BROWSER_H

#include "Legacy/RetroArchFRLGDiscovery.h"

#include <cstddef>
#include <string>
#include <vector>

namespace PokeVault::Legacy {
    // UI-neutral descriptor for one validated legacy source. The descriptor carries the
    // discovery-result index rather than a raw save pointer, so selection can be resolved back
    // through the session-owned catalog without copying or reparsing the source file.
    struct FRLGSourceCard {
        size_t sourceIndex = 0;
        std::string gameId;
        std::string title;
        std::string platformLabel;
        std::string sourceLabel;
        std::string location;
    };

    // Only strict, ready sources become selectable cards. Invalid, ambiguous, or unreadable files
    // stay in the diagnostic catalog but can never enter the browsing route.
    [[nodiscard]] std::vector<FRLGSourceCard> buildFRLGSourceCards(
        const FRLGDiscoveryResult& discovery);

    // Revalidates the index/identity relationship at activation time. This prevents a stale or
    // malformed UI descriptor from routing to the wrong release or platform.
    [[nodiscard]] const FRLGSource* resolveFRLGSourceCard(
        const FRLGDiscoveryResult& discovery, const FRLGSourceCard& card) noexcept;
}

#endif
