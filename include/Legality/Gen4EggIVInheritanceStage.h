#pragma once
// RNG-003: source-pinned retail Platinum/HGSS ordinary egg-IV inheritance
// STAGE ONLY. Inputs describe physical deposited daycare slots, not the
// move-father/mother roles. Matching a known stage is not global legality.
// Neither the parent-held items nor pre-inheritance IVs nor RNG stage state
// can be recovered from a detached PK4 child.
#include <array>
#include <cstdint>
#include <string_view>

namespace Legality::Gen4EggIVInheritanceStage {
enum class Family : uint8_t { Unknown, Platinum, HeartGoldSoulSilver };
enum class Status : uint8_t {
    UnsupportedGame, MalformedIVDomain, ReplayedStage
};
enum class CandidateCheck : uint8_t {
    UnsupportedGame, UnknownParents, UnknownPhysicalSlots,
    UnknownInitialChildIVs, UnknownHeldItemHistory,
    UnknownRNGStageBoundary, MalformedIVDomain, Matched, CandidateMismatch
};
constexpr Family family(std::string_view game) noexcept {
    if(game=="platinum_nds")return Family::Platinum;
    if(game=="heartgold_nds" || game=="soulsilver_nds")
        return Family::HeartGoldSoulSilver;
    return Family::Unknown; // DP has not been independently executed.
}
constexpr int8_t powerStat(uint16_t item) noexcept {
    switch(item) {
        case 294:return 0; // Power Weight -> HP
        case 289:return 1; // Power Bracer -> Attack
        case 290:return 2; // Power Belt -> Defense
        case 293:return 3; // Power Anklet -> Speed
        case 291:return 4; // Power Lens -> Special Attack
        case 292:return 5; // Power Band -> Special Defense
        default:return -1;
    }
}
constexpr bool validIVs(const std::array<uint8_t,6>& ivs) noexcept {
    for(uint8_t iv:ivs)if(iv>31)return false;
    return true;
}
constexpr uint32_t next(uint32_t state) noexcept {
    return static_cast<uint32_t>(state*0x41C64E6Du+0x6073u);
}
constexpr uint32_t rewind(uint32_t finalSeed,uint8_t draws) noexcept {
    for(uint8_t i=0;i<draws;++i)
        finalSeed=static_cast<uint32_t>(
            (finalSeed-0x6073u)*0xEEB9EB65u);
    return finalSeed;
}
constexpr uint8_t drawsFor(Family f,uint16_t held0,uint16_t held1) noexcept {
    if(f==Family::Platinum)return 6;
    if(f!=Family::HeartGoldSoulSilver)return 0;
    const bool p0=powerStat(held0)>=0,p1=powerStat(held1)>=0;
    return p0&&p1?5:(p0||p1?4:6);
}
struct Result {
    Status status=Status::UnsupportedGame;
    uint32_t finalSeed=0;
    uint8_t drawCount=0;
    std::array<uint16_t,6> draws{};
    std::array<uint8_t,3> selectedStats{};
    std::array<uint8_t,3> donorSlots{};
    std::array<uint8_t,6> childIVs{};
    uint8_t distinctInheritedStats=0;
};
constexpr Result replay(std::string_view game,uint32_t entrySeed,
                        const std::array<uint8_t,6>& initialChild,
                        const std::array<uint8_t,6>& physicalSlot0,
                        const std::array<uint8_t,6>& physicalSlot1,
                        uint16_t heldItem0,uint16_t heldItem1) noexcept {
    const auto kind=family(game);
    if(kind==Family::Unknown)return {};
    if(!validIVs(initialChild) || !validIVs(physicalSlot0) ||
       !validIVs(physicalSlot1))
        return {Status::MalformedIVDomain};
    Result out{};
    out.status=Status::ReplayedStage;
    out.childIVs=initialChild;
    auto draw=[&entrySeed,&out]() constexpr -> uint16_t {
        entrySeed=next(entrySeed);
        const auto value=static_cast<uint16_t>(entrySeed>>16);
        out.draws[out.drawCount++]=value;
        return value;
    };
    std::array<uint8_t,6> available{0,1,2,3,4,5};
    uint8_t start=0;
    if(kind==Family::HeartGoldSoulSilver) {
        const bool p0=powerStat(heldItem0)>=0;
        const bool p1=powerStat(heldItem1)>=0;
        if(p0||p1) {
            // Two holders spend one draw even when they hold IDENTICAL
            // Power items. Odd picks physical slot 0, even slot 1.
            const uint8_t forcedSlot=p0&&p1?
                static_cast<uint8_t>((draw()&1u)?0u:1u):
                static_cast<uint8_t>(p0?0u:1u);
            const uint8_t forcedStat=static_cast<uint8_t>(
                powerStat(forcedSlot==0?heldItem0:heldItem1));
            out.selectedStats[0]=forcedStat;
            out.donorSlots[0]=forcedSlot;
            start=1;
            // Remove the forced STAT VALUE, preserving five possibilities.
            uint8_t at=0;
            while(at<6 && available[at]!=forcedStat)++at;
            for(uint8_t j=at;j<5;++j)available[j]=available[j+1];
        }
    }
    for(uint8_t i=start;i<3;++i) {
        const uint16_t roll=draw();
        const uint8_t index=static_cast<uint8_t>(roll%(6u-i));
        out.selectedStats[i]=available[index];
        if(kind==Family::Platinum) {
            // Exact indexed-removal/stale-tail retail behavior: remove
            // array POSITION i instead of the selected index. Later
            // selections may repeat an already inherited stat.
            available[i]=0xFFu;
            uint8_t compacted=0;
            for(uint8_t j=0;j<6;++j)
                if(available[j]!=0xFFu)
                    available[compacted++]=available[j];
        } else {
            const uint8_t remaining=static_cast<uint8_t>(6u-i);
            for(uint8_t j=index;j+1<remaining;++j)
                available[j]=available[j+1];
        }
    }
    for(uint8_t i=start;i<3;++i)
        out.donorSlots[i]=static_cast<uint8_t>(draw()%2u);
    std::array<bool,6> distinct{};
    for(uint8_t i=0;i<3;++i) {
        const uint8_t stat=out.selectedStats[i];
        if(!distinct[stat]) {
            distinct[stat]=true;++out.distinctInheritedStats;
        }
        out.childIVs[stat]=out.donorSlots[i]==0?
            physicalSlot0[stat]:physicalSlot1[stat];
    }
    out.finalSeed=entrySeed;
    return out;
}
struct KnownFinalStage {
    std::string_view game{};
    std::array<uint8_t,6> initialChild{},physicalSlot0{},physicalSlot1{};
    std::array<uint8_t,6> observedChild{};
    uint16_t heldItem0=0,heldItem1=0;
    uint32_t finalSeed=0;
    bool parentSnapshotsIndependentlyVerified=false;
    bool depositOrderIndependentlyVerified=false;
    bool initialChildIVsIndependentlyVerified=false;
    bool heldItemHistoryIndependentlyVerified=false;
    bool finalRNGStateIndependentlyVerified=false;
};
constexpr CandidateCheck verifyKnownFinalStage(const KnownFinalStage& in) noexcept {
    const auto kind=family(in.game);
    if(kind==Family::Unknown)return CandidateCheck::UnsupportedGame;
    if(!in.parentSnapshotsIndependentlyVerified)
        return CandidateCheck::UnknownParents;
    if(!in.depositOrderIndependentlyVerified)
        return CandidateCheck::UnknownPhysicalSlots;
    if(!in.initialChildIVsIndependentlyVerified)
        return CandidateCheck::UnknownInitialChildIVs;
    // Platinum's retail stage never inspects held items, but HGSS does.
    if(kind==Family::HeartGoldSoulSilver &&
       !in.heldItemHistoryIndependentlyVerified)
        return CandidateCheck::UnknownHeldItemHistory;
    if(!in.finalRNGStateIndependentlyVerified)
        return CandidateCheck::UnknownRNGStageBoundary;
    if(!validIVs(in.initialChild)||!validIVs(in.physicalSlot0)||
       !validIVs(in.physicalSlot1)||!validIVs(in.observedChild))
        return CandidateCheck::MalformedIVDomain;
    const uint8_t draws=drawsFor(kind,in.heldItem0,in.heldItem1);
    const auto reconstructed=rewind(in.finalSeed,draws);
    const auto output=replay(in.game,reconstructed,in.initialChild,
                             in.physicalSlot0,in.physicalSlot1,
                             in.heldItem0,in.heldItem1);
    return output.status==Status::ReplayedStage &&
           output.drawCount==draws &&
           output.finalSeed==in.finalSeed &&
           output.childIVs==in.observedChild?
        CandidateCheck::Matched:CandidateCheck::CandidateMismatch;
}
} // namespace Legality::Gen4EggIVInheritanceStage
