#pragma once
// L2 B12: Diamond/Pearl ordinary egg IV inheritance, independently
// transcribed from pret/pokediamond@b623abef1b4dfe204ca50aab42447ac00c134290
// arm9/overlays/05/asm/ov05_021EC458.s, ov05_021EC9D4 (lines 688-832)
// and ov05_021EC99C (lines 654-685).
// Retail assembly selects 3 indices with LCRandom mod 6/5/4, removes
// LIST POSITION i (not the selected position), then selects 3 physical
// daycare donors with LCRandom & 1. No Power item handling at this stage.
// This is NOT borrowed from Platinum; original DP assembly is the proof.
// The original stage is a hypothetical conditional witness, NOT evidence
// that a detached PK4 has authentic original parents or RNG history.
#include <array>
#include <cstdint>
#include <string_view>

namespace Legality::Gen4DiamondPearlEggIVStage {
enum class Status : uint8_t { UnsupportedGame, MalformedIVs, Replayed };
enum class Check : uint8_t {
    UnsupportedGame, UnknownParentIVs, UnknownDepositOrder,
    UnknownInitialChildIVs, UnknownOriginalChildIVs,
    UnknownFinalSeed, MalformedIVs, Matched, Contradicted
};
constexpr bool supported(std::string_view game) noexcept {
    return game=="diamond_nds" || game=="pearl_nds";
}
constexpr bool valid(const std::array<uint8_t,6>& ivs) noexcept {
    for(auto i:ivs)if(i>31)return false;
    return true;
}
constexpr uint32_t advance(uint32_t seed) noexcept {
    return seed*0x41C64E6Du+0x6073u;
}
constexpr uint32_t rewind(uint32_t seed) noexcept {
    // Always six LCRandom calls for the original DP IV stage.
    for(int i=0;i<6;++i)seed=(seed-0x6073u)*0xEEB9EB65u;
    return seed;
}
struct Replay {
    Status status=Status::UnsupportedGame;
    uint32_t finalSeed=0;
    std::array<uint8_t,6> child{};
    std::array<uint8_t,3> selected{},donor{};
    std::array<uint16_t,6> rolls{};
    uint8_t distinctSelected=0;
};
constexpr Replay replay(std::string_view game,uint32_t initialSeed,
        const std::array<uint8_t,6>& childBeforeInheritance,
        const std::array<uint8_t,6>& daycareSlot0,
        const std::array<uint8_t,6>& daycareSlot1) noexcept {
    Replay out{};
    if(!supported(game))return out;
    if(!valid(childBeforeInheritance)||!valid(daycareSlot0)||
       !valid(daycareSlot1)) {
        out.status=Status::MalformedIVs;return out;
    }
    out.status=Status::Replayed;
    out.child=childBeforeInheritance;
    std::array<uint8_t,6> choices{0,1,2,3,4,5};
    auto next=[&]() constexpr {
        initialSeed=advance(initialSeed);
        return static_cast<uint16_t>(initialSeed>>16);
    };
    // ov05_021EC9D4: indexed-removal bug preserved from original
    // 021EC99C call with r1=loop index i rather than selected index.
    for(uint8_t i=0;i<3;++i) {
        const auto roll=next();
        out.rolls[i]=roll;
        const uint8_t picked=static_cast<uint8_t>(roll%(6u-i));
        out.selected[i]=choices[picked];
        choices[i]=0xFFu;
        uint8_t k=0;
        for(uint8_t j=0;j<6;++j)
            if(choices[j]!=0xFFu)choices[k++]=choices[j];
    }
    for(uint8_t i=0;i<3;++i) {
        const auto roll=next();
        out.rolls[i+3]=roll;
        out.donor[i]=static_cast<uint8_t>(roll&1u);
    }
    std::array<bool,6> observed{};
    for(uint8_t i=0;i<3;++i) {
        const uint8_t stat=out.selected[i];
        if(!observed[stat]){observed[stat]=true;++out.distinctSelected;}
        out.child[stat]=out.donor[i]==0?daycareSlot0[stat]:daycareSlot1[stat];
    }
    out.finalSeed=initialSeed;
    return out;
}
struct OriginalStage {
    std::string_view game{};
    std::array<uint8_t,6> initialChild{},physicalSlot0{},physicalSlot1{},originalEggIVs{};
    uint32_t finalSeed=0;
    bool historicalParentIVsVerified=false;
    bool physicalDepositOrderVerified=false;
    bool preInheritanceChildIVsVerified=false;
    bool originalUnhatchedEggIVsVerified=false;
    bool finalSeedIndependentlyVerified=false;
};
constexpr Check verify(const OriginalStage& in) noexcept {
    if(!supported(in.game))return Check::UnsupportedGame;
    if(!in.historicalParentIVsVerified)return Check::UnknownParentIVs;
    if(!in.physicalDepositOrderVerified)return Check::UnknownDepositOrder;
    if(!in.preInheritanceChildIVsVerified)return Check::UnknownInitialChildIVs;
    if(!in.originalUnhatchedEggIVsVerified)return Check::UnknownOriginalChildIVs;
    if(!in.finalSeedIndependentlyVerified)return Check::UnknownFinalSeed;
    if(!valid(in.initialChild)||!valid(in.physicalSlot0)||
       !valid(in.physicalSlot1)||!valid(in.originalEggIVs))
        return Check::MalformedIVs;
    const auto step=replay(in.game,rewind(in.finalSeed),in.initialChild,
                           in.physicalSlot0,in.physicalSlot1);
    return step.status==Status::Replayed && step.finalSeed==in.finalSeed &&
           step.child==in.originalEggIVs?Check::Matched:Check::Contradicted;
}
static_assert(supported("diamond_nds")&&supported("pearl_nds"));
static_assert(!supported("platinum_nds")&&!supported("heartgold_nds"));
} // namespace Legality::Gen4DiamondPearlEggIVStage
