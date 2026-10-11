#pragma once
// MOVE-001: fully specified CONDITIONAL Emerald same-species level-up
// timeline. All offers and player decisions are from a source-pinned
// table and explicit hypothesis, never reconstructed from stored PK3.
#include "Legality/Gen3EmeraldLevelUpEvidence.h"
#include <array>
#include <cstdint>
#include <string_view>
namespace Legality::Gen3EmeraldLevelUpTimeline {
enum class Decision : uint8_t { AutoLearn, ReplaceSlot, Decline, AlreadyKnown };
enum class Status : uint8_t { Unsupported, UnknownSnapshot, UnknownSpeciesContinuity,
    UnknownGameContinuity, IncompleteTrainingHistory,
    CandidateContradicted, CandidateConsistent };
struct Step {
    uint8_t level=0;
    uint16_t offeredMove=0;
    Decision decision=Decision::Decline;
    uint8_t replacementSlot=0;
};
struct Input {
    std::string_view game{};
    uint16_t species=0;
    uint8_t encounterLevel=0, observedLevel=0;
    bool initialSnapshotVerified=false;
    bool sameSpeciesVerified=false;
    // The same Emerald game and absence of TM, tutor, trade, daycare,
    // Move Deleter, event or other move mutation throughout the claimed
    // interval must be independently established for an EXCLUSIVE
    // ordinary level-up-only timeline, not inferred from current PK3.
    bool sameEmeraldGameThroughoutVerified=false;
    bool noOtherMoveChangesVerified=false;
    bool allOffersAndDecisionsVerified=false;
    std::array<uint16_t,4> initialMoves{}, observedMoves{};
    std::array<Step,24> steps{};
    uint8_t stepCount=0;
};
struct Result {
    Status status=Status::UnknownSnapshot;
    std::array<uint16_t,4> reconstructed{};
    uint8_t offers=0,replacements=0,declines=0;
};
constexpr bool contains(const std::array<uint16_t,4>& state,uint16_t move) noexcept {
    for(uint16_t value:state)if(value==move)return true;
    return false;
}
constexpr bool structural(const std::array<uint16_t,4>& state) noexcept {
    bool empty=false;
    for(uint8_t i=0;i<4;++i){
        if(!state[i]){empty=true;continue;}
        if(state[i]>354 || empty)return false;
        for(uint8_t j=0;j<i;++j)if(state[i]==state[j])return false;
    }
    return true;
}
constexpr uint8_t emptySlot(const std::array<uint16_t,4>& state) noexcept {
    for(uint8_t i=0;i<4;++i)if(!state[i])return i;
    return 4;
}
constexpr Result evaluate(const Input& in) noexcept {
    Result out{};
    if(in.game!="emerald_gba" || !in.species || in.species>386 ||
       !in.encounterLevel || in.observedLevel<in.encounterLevel ||
       in.observedLevel>100){out.status=Status::Unsupported;return out;}
    if(!in.initialSnapshotVerified)return out;
    if(!in.sameSpeciesVerified){
        out.status=Status::UnknownSpeciesContinuity;return out;
    }
    if(!in.sameEmeraldGameThroughoutVerified) {
        out.status=Status::UnknownGameContinuity;return out;
    }
    out.reconstructed=in.initialMoves;
    if(!structural(in.initialMoves)||!structural(in.observedMoves)){
        out.status=Status::CandidateContradicted;return out;
    }
    if(!in.allOffersAndDecisionsVerified ||
       !in.noOtherMoveChangesVerified ||
       in.stepCount>in.steps.size()){
        out.status=Status::IncompleteTrainingHistory;return out;
    }
    const uint16_t begin=Gen3EmeraldLevelUp::kEmeraldLevelOffsets[in.species];
    const uint16_t end=Gen3EmeraldLevelUp::kEmeraldLevelOffsets[in.species+1];
    for(uint16_t i=begin;i<end;++i) {
        const uint16_t packed=Gen3EmeraldLevelUp::kEmeraldPackedLevelMoves[i];
        const uint8_t level=static_cast<uint8_t>(packed>>9);
        const uint16_t move=packed&511u;
        if(level<=in.encounterLevel||level>in.observedLevel)continue;
        if(out.offers>=in.stepCount || out.offers>=in.steps.size()){
            out.status=Status::IncompleteTrainingHistory;return out;
        }
        const Step step=in.steps[out.offers++];
        if(step.level!=level||step.offeredMove!=move){
            out.status=Status::CandidateContradicted;return out;
        }
        const bool already=contains(out.reconstructed,move);
        const uint8_t open=emptySlot(out.reconstructed);
        switch(step.decision){
        case Decision::AlreadyKnown:
            if(!already){out.status=Status::CandidateContradicted;return out;}
            break;
        case Decision::AutoLearn:
            if(already || open==4 || step.replacementSlot!=open){
                out.status=Status::CandidateContradicted;return out;
            }
            out.reconstructed[open]=move;break;
        case Decision::ReplaceSlot:
            if(already||open!=4||step.replacementSlot>=4){
                out.status=Status::CandidateContradicted;return out;
            }
            out.reconstructed[step.replacementSlot]=move;
            ++out.replacements;break;
        case Decision::Decline:
            if(already||open!=4){
                out.status=Status::CandidateContradicted;return out;
            }
            ++out.declines;break;
        }
    }
    if(out.offers!=in.stepCount){
        out.status=Status::CandidateContradicted;return out;
    }
    out.status=out.reconstructed==in.observedMoves?
        Status::CandidateConsistent:Status::CandidateContradicted;
    return out;
}
}
