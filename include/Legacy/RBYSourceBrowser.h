#ifndef POKEBANK_LEGACY_RBY_SOURCE_BROWSER_H
#define POKEBANK_LEGACY_RBY_SOURCE_BROWSER_H

#include "Legacy/RetroArchRBYDiscovery.h"
#include "Legacy/FRLGSourceBrowser.h"
#include "Legacy/LegacySourceBindings.h"

#include <string_view>
#include <vector>

namespace PokeVault::Legacy {
    // The existing FRLGSourceCard/FRLGSaveInstance structures are presentation-only legacy source
    // descriptors despite their historical names. Reuse them here so the Game Sources UI keeps one
    // stable physical-save child model while parsing remains strictly generation-specific.
    [[nodiscard]] std::vector<FRLGSourceCard> buildRBYSourceCards(
        const RBYDiscoveryResult& discovery);

    [[nodiscard]] std::vector<FRLGSourceCard> buildRBYSourceCardsForProfile(
        const RBYDiscoveryResult& discovery, const LegacySourceBindings& bindings,
        std::string_view profileIdentity);

    [[nodiscard]] const RBYSource* resolveRBYSaveInstance(
        const RBYDiscoveryResult& discovery, const FRLGSourceCard& card,
        size_t instanceIndex) noexcept;
}

#endif
