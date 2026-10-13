#pragma once
// L4 Gen I source-bound record preflight. No encounter table, modern traits
// or save writes. This is a candidate witness, not a global legality verdict.
#include <array>
#include <cstdint>
namespace Legality::Gen1OriginalGiftNativePreflight {
enum class RateStatus:uint8_t { Unknown, OriginalRateMatches,
    OriginalRateContradicted, TradebackItemMatches, TradebackItemContradicted };
// Original Stadium media attestation is not a Red/Blue/Yellow cartridge
// script pin. Mixing them would make real Stadium candidates unprovable or
// let a pinned game script falsely attest a separate distribution channel.
constexpr bool verifiedOriginalMechanism(bool stadiumGift,
    bool cartridgeScriptIndependentlyPinned,
    bool stadiumMediaMechanismIndependentlyPinned) noexcept {
    return stadiumGift ? stadiumMediaMechanismIndependentlyPinned
                       : cartridgeScriptIndependentlyPinned;
}

struct RateHistory {
    bool originalAcquisitionByteIndependentlyAttested=false;
    uint8_t originalAcquisitionByte=0;
    bool exhaustiveUnchangedByteHistoryIndependentlyProven=false;
    bool independentTimeCapsuleReturnWitness=false;
    bool independentReturnItemWitness=false;
    bool actualCrossingSnapshotIndependentlyVerified=false;
    bool returnByteVerifiedAsGen2HeldItem=false;
    uint8_t returnItemByte=0;
};
constexpr RateStatus compareRate(uint8_t nativeByte,const RateHistory& h) noexcept {
    if(!h.originalAcquisitionByteIndependentlyAttested)return RateStatus::Unknown;
    if(h.exhaustiveUnchangedByteHistoryIndependentlyProven &&
       !h.independentTimeCapsuleReturnWitness)
        return nativeByte==h.originalAcquisitionByte?
            RateStatus::OriginalRateMatches:RateStatus::OriginalRateContradicted;
    if(h.independentTimeCapsuleReturnWitness &&
       h.independentReturnItemWitness &&
       h.actualCrossingSnapshotIndependentlyVerified &&
       h.returnByteVerifiedAsGen2HeldItem)
        return nativeByte==h.returnItemByte?
            RateStatus::TradebackItemMatches:RateStatus::TradebackItemContradicted;
    return RateStatus::Unknown;
}
// Native PokemonRecord stores exact 8-bit moves, 4-bit DVs and PP Ups; it
// does not store original cartridge, gender, ribbon, ability, nature or egg.
constexpr bool bounded(uint16_t species,uint8_t level,
    const std::array<uint8_t,4>& moves,const std::array<uint8_t,5>& dvs,
    const std::array<uint8_t,4>& ppUps) noexcept {
    if(species<1 || species>151 || level<1 || level>100)return false;
    for(const auto m:moves)if(m>165)return false;
    for(const auto dv:dvs)if(dv>15)return false;
    for(const auto pp:ppUps)if(pp>3)return false;
    return true;
}
static_assert(bounded(25,10,{1,2,165,0},{0,1,2,3,15},{0,1,2,3}));
static_assert(!bounded(25,10,{1,2,166,0},{0,1,2,3,15},{0,1,2,3}));
static_assert(compareRate(190,RateHistory{})==RateStatus::Unknown);
} // namespace Legality::Gen1OriginalGiftNativePreflight
