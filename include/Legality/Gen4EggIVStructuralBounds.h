#pragma once
// RNG-003 / BREED-004: source-pinned NECESSARY conditions for ordinary
// Platinum/HGSS egg-IV inheritance when original RNG seed is unknown.
// Passing means only "not excluded by this deliberately over-approximated
// inheritance domain", NEVER a reconstructed RNG or global Valid child.
// Exclusion is strictly about independently attested historical parent
// snapshots, physical slots, and one observed original child.
#include "Legality/Gen4EggIVInheritanceStage.h"
#include <array>
#include <cstdint>
#include <string_view>

namespace Legality::Gen4EggIVStructuralBounds {
namespace RNG=Gen4EggIVInheritanceStage;
enum class Status : uint8_t {
    UnsupportedGame, UnknownHistoricalParents, UnknownPhysicalSlots,
    UnknownOriginalChildIVs, UnknownHeldItemHistory, MalformedIVDomain,
    CandidateContradiction, NecessaryPatternNotExcluded
};
struct Input {
    std::string_view game{};
    std::array<uint8_t,6> physicalSlot0{},physicalSlot1{},observedChild{};
    uint16_t heldItem0=0,heldItem1=0;
    bool historicalParentIVsIndependentlyVerified=false;
    bool historicalPhysicalOrderIndependentlyVerified=false;
    bool originalChildIVsIndependentlyVerified=false;
    // Required to make a Power-item force inference in HGSS. Not
    // reconstructed from present-day held-item fields.
    bool heldItemsAtIVStageIndependentlyVerified=false;
};
struct Result {
    Status status=Status::UnknownHistoricalParents;
    uint8_t matchingStatCount=0;
    // Platinum may overwrite the same inherited stat several times.
    // HGSS always inherits three distinct stat indices.
    uint8_t guaranteedMinimumDistinctStats=0;
    bool powerForceRequired=false;
    bool anyAttestedForcedDonorFits=false;
};
constexpr bool mayMatch(uint8_t child,uint8_t p0,uint8_t p1) noexcept {
    return child==p0 || child==p1;
}
constexpr Result evaluate(const Input& in) noexcept {
    Result out{};
    const auto family=RNG::family(in.game);
    if(family==RNG::Family::Unknown) {
        out.status=Status::UnsupportedGame;return out;
    }
    if(!in.historicalParentIVsIndependentlyVerified) return out;
    if(!in.historicalPhysicalOrderIndependentlyVerified) {
        out.status=Status::UnknownPhysicalSlots;return out;
    }
    if(!in.originalChildIVsIndependentlyVerified) {
        out.status=Status::UnknownOriginalChildIVs;return out;
    }
    if(!RNG::validIVs(in.physicalSlot0) ||
       !RNG::validIVs(in.physicalSlot1) ||
       !RNG::validIVs(in.observedChild)) {
        out.status=Status::MalformedIVDomain;return out;
    }
    std::array<bool,6> matches{};
    for(uint8_t stat=0;stat<6;++stat) {
        matches[stat]=mayMatch(in.observedChild[stat],
            in.physicalSlot0[stat],in.physicalSlot1[stat]);
        if(matches[stat])++out.matchingStatCount;
    }
    const bool hgss=family==RNG::Family::HeartGoldSoulSilver;
    out.guaranteedMinimumDistinctStats=hgss?3u:1u;
    // Platinum's indexed-removal bug allows two or three writes to
    // select the same stat; at least ONE final inherited value still
    // originates from one of the two same-stage parents.
    if(!hgss) {
        out.status=out.matchingStatCount>=1?
            Status::NecessaryPatternNotExcluded:
            Status::CandidateContradiction;
        return out;
    }
    // HGSS always selects THREE distinct inherited stats regardless
    // of whether zero, one or two Power items are held. This bound
    // needs no known historical item state or RNG final seed.
    if(out.matchingStatCount<3) {
        out.status=Status::CandidateContradiction;return out;
    }
    // A tighter forced-donor bound *does* require independently
    // verified historical items, including the absence of Power items.
    if(!in.heldItemsAtIVStageIndependentlyVerified) {
        out.status=Status::UnknownHeldItemHistory;return out;
    }
    const int8_t p0=RNG::powerStat(in.heldItem0);
    const int8_t p1=RNG::powerStat(in.heldItem1);
    out.powerForceRequired=p0>=0||p1>=0;
    if(!out.powerForceRequired) {
        out.status=Status::NecessaryPatternNotExcluded;return out;
    }
    // With a single holder, that physical slot's stat is forced.
    // With two holders, RNG chooses ONE physical holder first, even
    // if both hold the identical Power item. Allow either without a
    // known RNG parity. Two other DISTINCT stats must still match.
    const auto fits=[&](int8_t stat,uint8_t slot) constexpr noexcept {
        if(stat<0)return false;
        const uint8_t index=static_cast<uint8_t>(stat);
        const bool donorMatches=in.observedChild[index]==
            (slot==0?in.physicalSlot0[index]:in.physicalSlot1[index]);
        const uint8_t others=static_cast<uint8_t>(
            out.matchingStatCount-(matches[index]?1u:0u));
        return donorMatches && others>=2;
    };
    out.anyAttestedForcedDonorFits=fits(p0,0)||fits(p1,1);
    out.status=out.anyAttestedForcedDonorFits?
        Status::NecessaryPatternNotExcluded:
        Status::CandidateContradiction;
    return out;
}
} // namespace Legality::Gen4EggIVStructuralBounds
