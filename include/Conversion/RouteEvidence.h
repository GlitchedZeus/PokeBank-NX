#ifndef CONVERSION_ROUTE_EVIDENCE_H
#define CONVERSION_ROUTE_EVIDENCE_H

#include <cstdint>

#include "Conversion/Fidelity.h"
#include "Enums/GameVersion.h"
#include "Utils/MoveTransaction.h"

namespace Conversion {
    // Evidence contract for a future source-retiring cross-game Move. This is intentionally data-only:
    // creating one does not enable a route or mutate either store. Historical origin, native source/
    // destination formats and current storage ownership are separate concepts by construction.
    struct RouteEvidence {
        Enums::GameVersion sourceFormat = Enums::GameVersion::Invalid;
        Enums::GameVersion destinationFormat = Enums::GameVersion::Invalid;
        uint8_t historicalOriginVersion = 0;
        Report fidelity{};
        PokeBank::Storage::MoveTx::StoreDescriptor sourceStore{};
        PokeBank::Storage::MoveTx::StoreDescriptor destinationStore{};
        PokeBank::Storage::MoveTx::Digest sourcePayload{};
        PokeBank::Storage::MoveTx::Digest destinationPayload{};
        bool lossesAcknowledged = false;

        bool requiresLossAcknowledgement() const noexcept { return fidelity.hasLosses(); }
        bool lossPolicySatisfied() const noexcept {
            return !requiresLossAcknowledgement() || lossesAcknowledged;
        }
    };
}

#endif
