#pragma once
// BREED-001/RNG-001: JOINT exact SAME-PARENT ordinary Gen III egg-history
// witness. One parent pair supplies both inherited moves AND the six-draw
// IV stage. This does NOT recover real parent identities or final RNG state
// from a PK3; those fields must be independently supplied and verified.
// A mismatch excludes THIS history only, never a Pokemon globally.
#include "Legality/Gen34SameParentEggHistory.h"
#include "Legality/Gen3EggIVInheritanceStage.h"
#include <array>
#include <cstdint>

namespace Legality::Gen3JointEggCandidateEvidence {
namespace Move=Gen34SameParentEggHistory;
namespace IV=Gen3EggIVInheritanceStage;
enum class Status : uint8_t {
    UnsupportedGame, UnknownMoveParentHistory, UnknownPairEligibility,
    UnknownLightBall, IncoherentParentWitness,
    UnknownIVParentSnapshot, UnknownIVRNGHistory,
    UnknownSharedParentStage, IncoherentParentStage,
    InvalidIVCandidateInput, MoveCandidateContradicted,
    IVCandidateContradicted, KnownCandidateConsistent
};
struct Parent {
    uint32_t id=0; // Same independently asserted parent as the move witness
    bool originalIVsKnown=false;
    std::array<uint8_t,6> originalIVs{};
};
struct Input {
    Move::Input moveHistory{};
    Parent father{},mother{};
    // A shared father/mother ID is not enough if the two descriptions
    // refer to DIFFERENT historical parent moveset/IV snapshots.
    // Explicit source-attested stage tokens cannot be recovered from PK3.
    bool sharedParentStageIndependentlyVerified=false;
    uint64_t moveParentStageToken=0;
    uint64_t ivParentStageToken=0;
    bool independentlyKnownFinalIVStageState=false;
    uint32_t finalIVStageRNGState=0;
    std::array<uint8_t,6> initialChildIVs{};
    std::array<uint8_t,6> observedUnhatchedIVs{};
};
struct Result {
    Status status=Status::UnknownMoveParentHistory;
    Move::Result moves{};
    IV::CandidateCheck ivs=IV::CandidateCheck::UnknownParentHistory;
};
constexpr Result evaluate(const Input& in) noexcept {
    Result out{};
    const auto game=in.moveHistory.game;
    if(IV::family(game)==IV::Family::Unknown) {
        out.status=Status::UnsupportedGame;
        return out; // Gen IV IV inheritance is NOT identical to Gen III
    }
    out.moves=Move::evaluate(in.moveHistory);
    switch(out.moves.status) {
        case Move::Status::Unsupported:
            out.status=Status::UnsupportedGame;return out;
        case Move::Status::UnknownParents:
            out.status=Status::UnknownMoveParentHistory;return out;
        case Move::Status::UnknownPairEligibility:
            out.status=Status::UnknownPairEligibility;return out;
        case Move::Status::UnknownLightBall:
            out.status=Status::UnknownLightBall;return out;
        case Move::Status::CandidateMismatch:
            out.status=Status::MoveCandidateContradicted;return out;
        case Move::Status::CandidateCompatible: break;
    }
    // Critical anti-union gate: one move parent pair cannot silently be
    // combined with IV evidence from two other imagined parents.
    if(!in.father.id || !in.mother.id ||
       in.father.id!=in.moveHistory.fatherId ||
       in.mother.id!=in.moveHistory.motherId) {
        out.status=Status::IncoherentParentWitness;return out;
    }
    if(!in.father.originalIVsKnown || !in.mother.originalIVsKnown) {
        out.status=Status::UnknownIVParentSnapshot;return out;
    }
    if(!in.sharedParentStageIndependentlyVerified ||
       !in.moveParentStageToken || !in.ivParentStageToken) {
        out.status=Status::UnknownSharedParentStage;return out;
    }
    if(in.moveParentStageToken!=in.ivParentStageToken) {
        out.status=Status::IncoherentParentStage;return out;
    }
    if(!in.independentlyKnownFinalIVStageState) {
        out.status=Status::UnknownIVRNGHistory;return out;
    }
    out.ivs=IV::verifyKnownStage(game,in.finalIVStageRNGState,
                                 in.observedUnhatchedIVs,
                                 in.initialChildIVs,
                                 in.father.originalIVs,
                                 in.mother.originalIVs,true);
    switch(out.ivs) {
        case IV::CandidateCheck::Matched:
            out.status=Status::KnownCandidateConsistent;break;
        case IV::CandidateCheck::CandidateMismatch:
            out.status=Status::IVCandidateContradicted;break;
        case IV::CandidateCheck::MalformedIVDomain:
            out.status=Status::InvalidIVCandidateInput;break;
        case IV::CandidateCheck::UnknownParentHistory:
            out.status=Status::UnknownIVParentSnapshot;break;
        case IV::CandidateCheck::UnsupportedGame:
            out.status=Status::UnsupportedGame;break;
    }
    return out;
}
} // namespace Legality::Gen3JointEggCandidateEvidence
