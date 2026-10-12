#pragma once
// L4 V16: source-qualified retained original Poké Ball evidence.
// Gen IV PK4 stores DP/Pt ball at 0x83 and HGSS mirrored field at 0x86.
// A native ball is a final-stage observation, not an original-event
// distribution source, nor proof of specific visit/trade history.
// Pinned accepted references: Gen34BallDomainEvidence,
// Gen34EggBallEvidence and Gen4TransferEvidence.
#include "Legality/Gen34BallDomainEvidence.h"
#include "Legality/Gen34EggBallEvidence.h"
#include "Legality/Gen4TransferEvidence.h"
#include "Pokemon/Pokemon4ReadOnly.h"
#include <cstdint>

namespace Legality::Gen4EventBallHistoryEvidence {
enum class Route : uint8_t { Unknown, EventArchive, PalParkGen3, OrdinaryEgg };
enum class Status : uint8_t {
    UnavailableNative, UnknownOriginalRoute, UnknownOriginalBall,
    UnknownHistoricalRetention, UnknownSplitFieldOrigin,
    CandidateBallMatch, CandidateBallMismatch,
    CandidateSplitFieldsMismatch, CandidateOrdinaryEggBall,
    UnknownOrdinaryEggHistory
};
struct SourceEvidence {
    Route route=Route::Unknown;
    // Trust in the specific original distribution, not just WC4 existence.
    bool exactOriginIndependentlyVerified=false;
    bool originalBallIndependentlyVerified=false;
    uint8_t originalBall=0;
    bool ballPreservationHistoryKnown=false;
};
struct Native {
    uint8_t originGeneration=0;
    uint8_t ballDPPt=0;
    uint8_t ballHGSS=0;
    uint16_t metLocationDP=0;
    uint16_t metLocationExtended=0;
    uint16_t eggLocation=0;
    uint8_t metLevel=0;
    bool egg=false;
};
struct Result {
    Status status=Status::UnavailableNative;
    uint8_t observedBall=0;
    Gen4Transfer::Evidence palPark=Gen4Transfer::Evidence::NotApplicable;
};
constexpr Result evaluate(bool available,const Native& native,
                          const SourceEvidence& source={}) noexcept {
    Result out{};
    if(!available)return out;
    // The accepted Gen IV viewer uses max for display. Both raw fields
    // remain accessible for transfer-provenance analysis below.
    out.observedBall=native.ballDPPt>native.ballHGSS
        ? native.ballDPPt:native.ballHGSS;
    if(source.route==Route::Unknown ||
       !source.exactOriginIndependentlyVerified) {
        out.status=Status::UnknownOriginalRoute;return out;
    }
    if(source.route==Route::OrdinaryEgg) {
        // Retail Gen III/IV eggs do not inherit parents' ball. A Poke Ball
        // witness never excludes alternative Ranger/event/traded egg routes.
        const auto candidate=Gen34EggBallEvidence::analyze(
            4,native.egg,native.eggLocation,native.metLevel,out.observedBall);
        out.status=(candidate==Gen34EggBallEvidence::Kind::NativeEggPokeBall)
            ? Status::CandidateOrdinaryEggBall : Status::UnknownOrdinaryEggHistory;
        return out;
    }
    if(source.route==Route::PalParkGen3) {
        if(native.originGeneration!=3) {
            out.status=Status::UnknownSplitFieldOrigin;return out;
        }
        out.palPark=Gen4Transfer::classifyStoredFields(
            native.originGeneration,native.metLocationDP,
            native.metLocationExtended,native.egg);
        if(out.palPark!=Gen4Transfer::Evidence::PalParkDiamondPearlFields &&
           out.palPark!=Gen4Transfer::Evidence::PalParkPtHgssFields) {
            out.status=Status::UnknownSplitFieldOrigin;return out;
        }
        if(!Gen4Transfer::validPalParkBallFields(
            out.palPark,native.ballDPPt,native.ballHGSS)) {
            out.status=Status::CandidateSplitFieldsMismatch;return out;
        }
        if(!source.originalBallIndependentlyVerified ||
           !Gen4Transfer::isGen3Ball(source.originalBall)) {
            out.status=Status::UnknownOriginalBall;return out;
        }
        if(!source.ballPreservationHistoryKnown) {
            out.status=Status::UnknownHistoricalRetention;return out;
        }
        out.status=native.ballDPPt==source.originalBall
            ? Status::CandidateBallMatch : Status::CandidateBallMismatch;
        return out;
    }
    if(!source.originalBallIndependentlyVerified ||
       Gen34BallDomain::classify(4,source.originalBall)!=
           Gen34BallDomain::Result::WithinGenerationDomain) {
        out.status=Status::UnknownOriginalBall;return out;
    }
    if(!source.ballPreservationHistoryKnown) {
        out.status=Status::UnknownHistoricalRetention;return out;
    }
    if(!out.observedBall) {
        out.status=Status::UnknownOriginalBall;return out;
    }
    out.status=out.observedBall==source.originalBall
        ? Status::CandidateBallMatch:Status::CandidateBallMismatch;
    return out;
}
inline Result evaluateNative(const Pokemon::Pokemon4ReadOnly& pokemon,
                             const SourceEvidence& source={}) noexcept {
    if(!pokemon.valid() || pokemon.empty())return {};
    const uint8_t origin=pokemon.originVersion();
    // Stored PK4 origin IDs 1-5 cover Ruby/Sapphire/Emerald/FR/LG;
    // 15 is GameCube (Colosseum/XD), which uses the Gen III domain.
    // Do NOT drop Ruby/Sapphire/Emerald (1-3), the main Pal Park games.
    const uint8_t generation=((origin>=1 && origin<=5) || origin==15) ? 3 :
                             ((origin==7 || origin==8 || origin==10 ||
                               origin==11 || origin==12) ? 4 : 0);
    return evaluate(true,{
        generation,pokemon.ballDPPt(),pokemon.ballHGSS(),
        pokemon.metLocationDP(),pokemon.metLocationExtended(),
        pokemon.eggLocationExtended()!=0
            ? pokemon.eggLocationExtended():pokemon.eggLocationDP(),
        pokemon.metLevel(),pokemon.isEgg()
    },source);
}
} // namespace Legality::Gen4EventBallHistoryEvidence
