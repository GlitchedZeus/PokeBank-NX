#pragma once
// L4 Stage 4: original encrypted PK3 source-side facts for a hypothesized
// Gen III -> Pal Park -> Gen IV route. This bridge checks original bytes;
// a separately independently verified Gen IV final snapshot is REQUIRED.
// Neither a source Pokemon nor a speculative Pal Park route proves receipt.
#include "Legality/Gen3DistributionNativePK3Evidence.h"
#include "Legality/Gen14HistoricalEventPathEvidence.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace Legality::Gen3EventPalParkMetadataBridge {
namespace N=Gen3DistributionNativePK3Evidence;
namespace P=Gen14HistoricalEventPathEvidence;
struct Source {
    bool independentOriginalEventIdentity=false;
    bool historicalChainExhaustivelyProven=false;
    bool sourceToFinalRecordIdentityIndependentlyProven=false;
    bool palParkTransitionIndependentlyVerified=false;
    bool originalSpeciesNationalIndependentlyVerified=false;
    uint16_t originalSpeciesNational=0;
    // Raw PK3 language bytes are NOT PK4 UTF-16 original trainer/name
    // bytes. Independent after-transfer raw-text evidence is required.
    P::Snapshot afterPalPark{};
    std::span<const P::Step> laterVerifiedSteps{};
};
struct Result {
    bool originalPK3Available=false;
    N::W::Native nativePK3{};
    P::Result lineage{};
    bool authenticated=false;
};
constexpr P::Snapshot project(const N::W::Native& n) noexcept {
    P::Snapshot s{};
    if(!n.available)return s;
    s.known=P::TrainerIDs|P::Language|P::OriginalBall|
            P::OriginVersion|P::EggState|P::HeldItem|P::Fateful;
    s.value[0]=n.speciesInternal; // diagnostic only; internal != National Dex
    s.value[1]=(uint64_t(n.ids.sid)<<16)|n.ids.tid;
    s.value[2]=n.language;
    s.value[3]=n.ball;
    s.value[4]=n.originGame;
    s.value[5]=n.egg;
    s.value[6]=n.heldItem;
    s.value[12]=n.fateful;
    return s;
}
inline Result evaluate(std::span<const std::byte> encryptedPK3,
                       const P::Snapshot& independentlyObservedPK4,
                       const Source& source={}) noexcept {
    Result out{};
    out.nativePK3=N::inspectEncrypted(encryptedPK3);
    if(!out.nativePK3.available)return out;
    out.originalPK3Available=true;
    if(!source.independentOriginalEventIdentity ||
       !source.sourceToFinalRecordIdentityIndependentlyProven ||
       !source.palParkTransitionIndependentlyVerified ||
       !source.historicalChainExhaustivelyProven) {
        out.lineage.status=P::Status::UnknownOriginalSource;
        return out;
    }
    // Original Ball is immutable as a logical ball ID; the split DP/HGSS
    // fields and Pal Park locations have to be validated separately by
    // Gen4EventBallHistoryEvidence. Never use max(ballDP,ballHGSS).
    P::Source origin{};
    origin.exactOriginalSourceIndependentlyAttested=true;
    origin.originalFieldsIndependentlyExtracted=true;
    origin.originalGeneration=P::Generation::III;
    origin.original=project(out.nativePK3);
    if(source.originalSpeciesNationalIndependentlyVerified) {
        origin.original.known|=P::Species;
        origin.original.value[0]=source.originalSpeciesNational;
    }
    origin.exhaustiveHistoricalChainVerified=true;
    std::array<P::Step,P::kMaxSteps> steps{};
    if(source.laterVerifiedSteps.size()>steps.size()-1) {
        out.lineage.status=P::Status::UnknownHistoricalTransformation;
        return out;
    }
    steps[0].action=P::Action::PalPark;
    steps[0].from=P::Generation::III;
    steps[0].to=P::Generation::IV;
    steps[0].independentlyVerified=true;
    // Pal Park replaces the byte-level text representation, not the
    // original Ball or OT IDs. Only independently attested post-Pal Park
    // values may be installed for changed fields.
    steps[0].mutated=source.afterPalPark.known&(
        P::MoveSet|P::NicknameRaw|P::OriginalTrainerRaw|P::MetDate|P::Language);
    steps[0].verifiedAfter=source.afterPalPark;
    for(size_t i=0;i<source.laterVerifiedSteps.size();++i)
        steps[i+1]=source.laterVerifiedSteps[i];
    out.lineage=P::evaluate(true,P::Generation::IV,independentlyObservedPK4,
        origin,std::span<const P::Step>(steps.data(),source.laterVerifiedSteps.size()+1));
    return out;
}
} // namespace Legality::Gen3EventPalParkMetadataBridge
