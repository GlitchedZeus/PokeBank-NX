#ifndef POKEBANK_LEGACY_GSC_SOURCE_BROWSER_H
#define POKEBANK_LEGACY_GSC_SOURCE_BROWSER_H

#include "Legacy/RetroArchGSCDiscovery.h"
#include "Legacy/FRLGSourceBrowser.h"
#include "Legacy/LegacySourceBindings.h"

#include <string_view>
#include <vector>

namespace PokeVault::Legacy {
    [[nodiscard]] std::vector<FRLGSourceCard> buildGSCSourceCards(
        const GSCDiscoveryResult& discovery);

    [[nodiscard]] std::vector<FRLGSourceCard> buildGSCSourceCardsForProfile(
        const GSCDiscoveryResult& discovery, const LegacySourceBindings& bindings,
        std::string_view profileIdentity);

    [[nodiscard]] const GSCSource* resolveGSCSaveInstance(
        const GSCDiscoveryResult& discovery, const FRLGSourceCard& card,
        size_t instanceIndex) noexcept;
}

#endif
