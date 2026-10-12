#pragma once
// RNG-003: conditional ordinary daycare IV stage, not whole egg legality.
// Parents are PHYSICAL daycare slots, never inferred father/mother roles.
// Platinum and HGSS retail source pins/fixtures: docs/legality/GEN4_EGG_IV_STAGE.md.
#include <array>
#include <cstdint>
#include <string_view>

namespace Legality::Gen4EggIVInheritanceStage {
using IVs=std::array<uint8_t,6>;
enum class Family : uint8_t { Unsupported, Platinum, HGSS };
enum class Status : uint8_t {
    UnsupportedGame, UnknownParentSlots, UnknownInitialIVs,
    UnknownHeldItems, UnknownRNGState, MalformedIVDomain, ReplayedStage
};
enum class CandidateCheck : uint8_t {
    UnsupportedGame, UnknownParentSlots, UnknownInitialIVs,
    UnknownHeldItems, UnknownRNGState, MalformedIVDomain, Matched, CandidateMismatch
};
struct Input {
    std::string_view game{};
    bool parentSlotsKnown=false;
    std::array<IVs,2> parents{};
    bool initialIVsKnown=false;
    IVs initialIVs{};
    bool heldItemsKnown=false;
    std::array<uint16_t,2> heldItems{};
    bool rngStateKnown=false;
    uint32_t rngState=0; // Entry state for replay; final state for verifyFinalState.
};
struct Result {
    Status status=Status::UnsupportedGame;
    uint32_t finalSeed=0;
    std::array<uint16_t,6> draws{};
    uint8_t drawCount=0;
    std::array<uint8_t,3> selectedStats{},donorSlots{};
    IVs childIVs{};
};
constexpr Family family(std::string_view game) noexcept {
    if(game=="platinum_nds")return Family::Platinum;
    if(game=="heartgold_nds"||game=="soulsilver_nds")return Family::HGSS;
    return Family::Unsupported; // DP has not been independently verified here.
}
constexpr int powerStat(uint16_t item) noexcept {
    switch(item) {
        case 294:return 0; // Weight: HP
        case 289:return 1; // Bracer: Attack
        case 290:return 2; // Belt: Defense
        case 293:return 3; // Anklet: Speed
        case 291:return 4; // Lens: Special Attack
        case 292:return 5; // Band: Special Defense
        default:return -1;
    }
}
constexpr bool validIVs(const IVs& values) noexcept {
    for(auto v:values)if(v>31)return false;
    return true;
}
constexpr Status validate(const Input& in) noexcept {
    const auto f=family(in.game);
    if(f==Family::Unsupported)return Status::UnsupportedGame;
    if(!in.parentSlotsKnown)return Status::UnknownParentSlots;
    if(!in.initialIVsKnown)return Status::UnknownInitialIVs;
    // Platinum does not consult held items in this stage.
    if(f==Family::HGSS&&!in.heldItemsKnown)return Status::UnknownHeldItems;
    if(!in.rngStateKnown)return Status::UnknownRNGState;
    if(!validIVs(in.initialIVs)||!validIVs(in.parents[0])||!validIVs(in.parents[1]))
        return Status::MalformedIVDomain;
    return Status::ReplayedStage;
}
constexpr uint8_t stageDrawCount(const Input& in) noexcept {
    if(family(in.game)==Family::Platinum)return 6;
    if(family(in.game)!=Family::HGSS||!in.heldItemsKnown)return 0;
    const unsigned count=(powerStat(in.heldItems[0])>=0)+(powerStat(in.heldItems[1])>=0);
    return count==0?6:count==1?4:5;
}
constexpr uint16_t draw(uint32_t& state,Result& out) noexcept {
    state=state*0x41C64E6Du+0x6073u;
    const auto value=static_cast<uint16_t>(state>>16);
    out.draws[out.drawCount++]=value;
    return value;
}
// Fixed-array compaction preserves the retail stale tail. Platinum later
// removes loop position i, whereas HGSS removes the selected list index.
constexpr void removeIndex(std::array<uint8_t,6>& available,uint8_t index) noexcept {
    available[index]=255;
    uint8_t written=0;
    for(uint8_t i=0;i<6;++i)
        if(available[i]!=255)available[written++]=available[i];
}
constexpr Result replay(const Input& in) noexcept {
    Result out{};out.status=validate(in);
    if(out.status!=Status::ReplayedStage)return out;
    out.childIVs=in.initialIVs;
    auto state=in.rngState;
    std::array<uint8_t,6> available{0,1,2,3,4,5};
    uint8_t first=0;
    if(family(in.game)==Family::HGSS) {
        const int stat0=powerStat(in.heldItems[0]),stat1=powerStat(in.heldItems[1]);
        if(stat0>=0||stat1>=0) {
            const uint8_t donor=(stat0>=0&&stat1>=0)?
                (draw(state,out)%2?0:1):(stat0>=0?0:1);
            out.donorSlots[0]=donor;
            out.selectedStats[0]=static_cast<uint8_t>(donor?stat1:stat0);
            removeIndex(available,out.selectedStats[0]);
            first=1;
        }
    }
    for(uint8_t i=first;i<3;++i) {
        const auto index=static_cast<uint8_t>(draw(state,out)%(6u-i));
        out.selectedStats[i]=available[index];
        removeIndex(available,family(in.game)==Family::Platinum?i:index);
    }
    for(uint8_t i=first;i<3;++i)
        out.donorSlots[i]=static_cast<uint8_t>(draw(state,out)%2);
    for(uint8_t i=0;i<3;++i) {
        const auto stat=out.selectedStats[i];
        out.childIVs[stat]=in.parents[out.donorSlots[i]][stat];
    }
    out.finalSeed=state;
    return out;
}
// Rewind only a known stage endpoint and known game/item branch. This
// does not recover a seed from child IVs and never yields global Invalid.
constexpr CandidateCheck verifyFinalState(Input in,const IVs& observed) noexcept {
    switch(validate(in)) {
        case Status::UnsupportedGame:return CandidateCheck::UnsupportedGame;
        case Status::UnknownParentSlots:return CandidateCheck::UnknownParentSlots;
        case Status::UnknownInitialIVs:return CandidateCheck::UnknownInitialIVs;
        case Status::UnknownHeldItems:return CandidateCheck::UnknownHeldItems;
        case Status::UnknownRNGState:return CandidateCheck::UnknownRNGState;
        case Status::MalformedIVDomain:return CandidateCheck::MalformedIVDomain;
        case Status::ReplayedStage:break;
    }
    if(!validIVs(observed))return CandidateCheck::MalformedIVDomain;
    const auto finalSeed=in.rngState;
    for(uint8_t i=0;i<stageDrawCount(in);++i)
        in.rngState=(in.rngState-0x6073u)*0xEEB9EB65u;
    const auto out=replay(in);
    return out.finalSeed==finalSeed&&out.childIVs==observed?
        CandidateCheck::Matched:CandidateCheck::CandidateMismatch;
}
} // namespace Legality::Gen4EggIVInheritanceStage
