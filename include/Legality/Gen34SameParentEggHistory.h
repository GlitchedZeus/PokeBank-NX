#pragma once
// BREED-001: one proposed four-slot father across all ordinary egg
// acquisition passes. A contradiction excludes only this parent hypothesis.
#include "Legality/Gen34OrdinaryEggMoveConstruction.h"
#include "Legality/Gen34EggMoveEvidence.h"
#include <array>
#include <cstdint>
#include <string_view>
namespace Legality::Gen34SameParentEggHistory {
enum class Status : uint8_t {
    Unsupported, UnknownParents, UnknownPairEligibility,
    UnknownLightBall, CandidateCompatible, CandidateMismatch
};
enum class PairProof : uint8_t {
    Unknown, IndependentlyCompatible, IndependentlyIncompatible
};
struct Input {
    std::string_view game{};
    uint16_t species=0;
    uint32_t fatherId=0,motherId=0;
    bool snapshotsKnown=false;
    std::array<uint16_t,4> base{},father{},mother{};
    std::array<bool,4> verifiedMachine{},verifiedSharedLevel{};
    bool lightBallKnown=false,lightBall=false;
    std::array<uint16_t,4> observed{};
    uint16_t fatherSpecies=0,motherSpecies=0;
    PairProof pairProof=PairProof::Unknown;
};
struct Result {
    Status status=Status::UnknownParents;
    std::array<uint16_t,4> generated{};
    uint8_t eggSources=0,machineSources=0,sharedSources=0;
};
constexpr bool has(const std::array<uint16_t,4>& moves,uint16_t move) noexcept {
    if(!move)return false;
    for(uint16_t own:moves)if(own==move)return true;
    return false;
}
constexpr Result evaluate(const Input& in) noexcept {
    using namespace Gen34OrdinaryEggMoveConstruction;
    Result out{};
    if(!supportedGame(in.game) || in.species==0 || in.species>493)
        {out.status=Status::Unsupported;return out;}
    const bool gen3=in.game=="ruby_gba" || in.game=="sapphire_gba" ||
        in.game=="emerald_gba" || in.game=="firered_gba" ||
        in.game=="leafgreen_gba";
    if(gen3 && in.species>386)
        {out.status=Status::Unsupported;return out;}
    if(!in.snapshotsKnown || !in.fatherId || !in.motherId)return out;
    const uint16_t maxSpecies=gen3?386:493;
    if(!in.fatherSpecies || !in.motherSpecies ||
       in.pairProof==PairProof::Unknown) {
        out.status=Status::UnknownPairEligibility;return out;
    }
    // A known hypothetical parent's species outside the source game's
    // Pokédex proves this candidate pair is impossible; it does NOT make
    // the child's original parentage or overall legality impossible.
    if(in.fatherSpecies>maxSpecies || in.motherSpecies>maxSpecies) {
        out.status=Status::CandidateMismatch;return out;
    }
    if(in.pairProof==PairProof::IndependentlyIncompatible)
        {out.status=Status::CandidateMismatch;return out;}
    Gen34OrdinaryEggMoveConstruction::Input source{};
    source.gameId=in.game;source.species=in.species;
    source.baseMoves=in.base;source.lightBallStateKnown=in.lightBallKnown;
    source.eligibleParentHeldLightBall=in.lightBall;
    // Every method stage uses the same father's actual four move slots.
    for(uint8_t i=0;i<4;++i) {
        const uint16_t move=in.father[i];
        if(!move)continue;
        if(Gen34EggMove::isEggMove(in.game,in.species,move))
            source.fatherEggMoves[out.eggSources++]=move;
        if(in.verifiedMachine[i])
            source.fatherMachineMoves[out.machineSources++]=move;
        if(in.verifiedSharedLevel[i] && has(in.mother,move))
            source.sharedLevelMoves[out.sharedSources++]=move;
    }
    const auto candidate=construct(source);
    if(candidate.status==Gen34OrdinaryEggMoveConstruction::Status::UnknownLightBallState)
        {out.status=Status::UnknownLightBall;return out;}
    if(!candidate.constructed())
        {out.status=Status::Unsupported;return out;}
    out.generated=candidate.moves;
    out.status=candidate.moves==in.observed ?
        Status::CandidateCompatible:Status::CandidateMismatch;
    return out;
}
}
