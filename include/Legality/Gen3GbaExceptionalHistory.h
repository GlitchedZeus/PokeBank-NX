#pragma once
// L3 Stage 4 production-data binding for unusual Gen III acquisition paths.
// The source catalogue is independently fingerprinted in the accepted L3
// Stage 3 adapter. This file does NOT grant original player-history receipts.
#include "Legality/Gen3GbaExceptionalHistoryCore.h"
#include "Legality/Gen3GbaJointMethodHistory.h"
#include <array>
#include <span>

namespace Legality::Gen3GbaExceptionalHistory {
namespace X=Gen3GbaExceptionalHistoryCore;
namespace H=Gen3GbaJointMethodHistoryCore;
namespace Original=Gen3GbaJointMethodHistory;
struct OriginalWitness {
    std::array<H::History,5> phases{};
    std::array<X::Edge,4> edges{};
    std::uint8_t phaseCount=0;
    bool entireClaimedHistoryAttested=false;
};
// Source catalogues are local to this call and their spans cannot escape.
// Reject if any catalog fails independent original-game/species FNV pin.
[[nodiscard]] constexpr X::Result evaluate(const OriginalWitness& input) noexcept {
    X::Result result{};
    if(!input.phaseCount||input.phaseCount>input.phases.size())return result;
    std::array<Original::Evidence,5> pinned{};
    X::Witness route{};
    route.phaseCount=input.phaseCount;
    route.entireClaimedHistoryAttested=input.entireClaimedHistoryAttested;
    route.edges=input.edges;
    for(std::uint8_t i=0;i<input.phaseCount;++i) {
        const auto& h=input.phases[i];
        pinned[i]=Original::originalEvidence(h.game,h.species);
        if(!pinned[i].pinned)return result;
        route.phases[i].history=h;
        route.phases[i].catalog={
            h.game,h.species,
            std::span<const H::Offer>(pinned[i].offers.data(),pinned[i].count),
            std::span<const std::uint16_t>(
                Legality::Gen3GbaOriginalMachineData::kMachineMove.data(),
                Legality::Gen3GbaOriginalMachineData::kMachineMove.size()),
            pinned[i].machines,true,true,true,true
        };
    }
    return X::evaluate(route);
}
// All six lines of evidence (original game, stage species, level-up array,
// numbered item table, current move slots, and historical teaching choice)
// must agree. Source methods alone can never imply a real retail encounter.
} // namespace Legality::Gen3GbaExceptionalHistory
