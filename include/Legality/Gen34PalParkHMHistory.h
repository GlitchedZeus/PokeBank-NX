#pragma once
// TRANSFER-001: candidate-local HM chronology across the Gen III -> IV
// Pal Park boundary. Gen III HMs cannot be assumed to survive migration:
// a present HM on native PK4 needs a later learning history. Broad Gen IV
// compatibility is an opportunity, NOT evidence that teaching occurred.
// Never conclude globally Invalid from an unsupported reacquisition route.
#include "Integration/Gen4/Gen4MoveCompatibility.h"
#include "Legality/Gen34PalParkHMIds.h"
#include <array>
#include <cstdint>

namespace Legality::Gen34PalParkHMHistory {
constexpr bool isGen3HM(uint16_t move) noexcept {
    return Gen34PalParkHMIds::isGen3HM(move);
}
struct Result {
    uint8_t storedGen3HM=0;
    uint8_t gen4PoolCandidates=0;
    uint8_t unmodeledReacquisitions=0;
};
// Caller must independently prove a native Gen III-handheld-origin PK4
// with valid split Pal Park marker and Ball fields. This function does not
// infer an encounter, a chosen post-transfer game, or the learned move order.
inline Result classify(uint16_t species,uint8_t form,
                       const std::array<uint16_t,4>& moves) noexcept {
    Result result{};
    for(const uint16_t move:moves) {
        if(!isGen3HM(move))continue;
        ++result.storedGen3HM;
        if(PokeVault::Integration::Gen4MoveCompatibility::anywhereInGen4(
               species,form,move))
            ++result.gen4PoolCandidates;
        else
            ++result.unmodeledReacquisitions;
    }
    return result;
}
} // namespace Legality::Gen34PalParkHMHistory
