#pragma once
// TRANSFER-001 / MOVE-001: optional independently verified post-Pal
// Park learning history. The source-verified arrival snapshot is fact
// about ONE hypothetical provenance chain; a detached PK4 does not
// preserve what was learned at each later level or game.
#include "Legality/Gen34PalParkSnapshotTransition.h"
#include <array>
#include <cstdint>
#include <string_view>
namespace Legality::Gen34PostPalParkLearning {
namespace P=Gen34PalParkSnapshotTransition;
enum class Method : uint8_t {
    LevelUp, TechnicalMachine, HiddenMachine, Tutor, Reminder
};
enum class Status : uint8_t {
    Unsupported, UnknownTransfer, UnknownTrainingHistory,
    UnknownMethodSource, MalformedHistory, CandidateContradicted,
    CandidateConsistent
};
struct Step {
    uint8_t level=0, stage=0;
    uint16_t move=0;
    Method method=Method::LevelUp;
    uint8_t forgetSlot=0;
    bool independentlyVerifiedMethod=false;
};
struct Input {
    P::Input crossing{};
    bool samePokemonAndGameContinuityVerified=false;
    bool fullPostArrivalSequenceVerified=false;
    uint8_t arrivalLevel=1, observedLevel=1;
    std::array<Step,16> steps{};
    uint8_t stepCount=0;
    std::array<uint16_t,4> observedMoves{};
};
struct Result {
    Status status=Status::UnknownTrainingHistory;
    P::Result originalCrossing{};
    std::array<uint16_t,4> rebuilt{};
    uint8_t appliedMoves=0;
};
constexpr uint8_t firstEmpty(const std::array<uint16_t,4>& moves) noexcept {
    for(uint8_t i=0;i<4;++i)if(!moves[i])return i;
    return 4;
}
constexpr bool contains(const std::array<uint16_t,4>& moves,uint16_t move) noexcept {
    for(uint16_t value:moves)if(value==move)return true;
    return false;
}
constexpr Result evaluate(const Input& in) noexcept {
    Result out{};
    out.originalCrossing=P::evaluate(in.crossing);
    switch(out.originalCrossing.status) {
        case P::Status::UnsupportedGames:out.status=Status::Unsupported;return out;
        case P::Status::UnknownStageWitness:
            out.status=Status::UnknownTransfer;return out;
        case P::Status::MalformedCandidateSnapshot:
            out.status=Status::MalformedHistory;return out;
        case P::Status::CandidateContradicted:
            out.status=Status::CandidateContradicted;return out;
        case P::Status::CandidateConsistent:break;
    }
    out.rebuilt=in.crossing.immediateNdsMoves;
    if(!in.samePokemonAndGameContinuityVerified ||
       !in.fullPostArrivalSequenceVerified)return out;
    if(!in.arrivalLevel || !in.observedLevel ||
       in.observedLevel<in.arrivalLevel || in.observedLevel>100 ||
       in.stepCount>in.steps.size() || !P::structural(in.observedMoves,467)) {
        out.status=Status::MalformedHistory;return out;
    }
    uint8_t lastStage=0, lastLevel=in.arrivalLevel;
    for(uint8_t i=0;i<in.stepCount;++i) {
        const Step step=in.steps[i];
        if(!step.stage || step.stage<=lastStage ||
           step.level<lastLevel || step.level>in.observedLevel ||
           !step.move || step.move>467 || step.forgetSlot>=4) {
            out.status=Status::MalformedHistory;return out;
        }
        lastStage=step.stage;lastLevel=step.level;
        if(!step.independentlyVerifiedMethod) {
            out.status=Status::UnknownMethodSource;return out;
        }
        if(contains(out.rebuilt,step.move)) {
            out.status=Status::CandidateContradicted;return out;
        }
        const uint8_t open=firstEmpty(out.rebuilt);
        if(open<4 && step.forgetSlot!=open) {
            out.status=Status::CandidateContradicted;return out;
        }
        out.rebuilt[step.forgetSlot]=step.move;
        ++out.appliedMoves;
    }
    out.status=out.rebuilt==in.observedMoves?
        Status::CandidateConsistent:Status::CandidateContradicted;
    return out;
}
} // namespace Legality::Gen34PostPalParkLearning
