#pragma once
// L1 Stage 3: bounded same-capture R/S/E/FRLG original wild-source + LCRNG
// witness reconciliation. Both original witnesses must be independently
// attested to ONE physical capture: matching present species, PID, current
// save, or a mathematically reversible seed cannot establish that identity.
// No Pokémon/save writes and no global Invalid or Legal result.
#include "Legality/Gen3OriginalWildSourceStage.h"
#include "Legality/Gen3OriginalWildRngWitness.h"
#include "Legality/Gen3OriginalCaptureRouteLedger.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
namespace Legality::Gen3OriginalWildJointHistory {
namespace W=Gen3OriginalWildSourceStage;
namespace R=Gen3OriginalWildRngWitness;

enum class Status : uint8_t {
    UnknownOriginalRoot, UnknownOriginalSource, UnknownOriginalStage,
    UnknownSameCaptureLink, UnknownConflictingWitness,
    CandidateOriginalSourceContradicted, CandidateOriginalRngContradicted,
    ConditionalSourceAndStageOnly
};
struct Candidate {
    uint64_t physicalCaptureIdentity=0,originalSaveIdentity=0;
    bool originalCaptureIdentityIndependentlyAttested=false;
    bool sourceAndRngBelongToSameCaptureIndependentlyAttested=false;
    W::Candidate source{};
    R::Stage stage{};
};
struct Result {
    Status status=Status::UnknownOriginalRoot;
    W::Status source=W::Status::UnknownOriginalSource;
    R::Status rng=R::Status::UnknownOriginalStage;
    uint8_t physicalSlot=255,originalLevel=0,nature=255;
    bool globalInvalidSupported=false,globallyLegalSupported=false;
};
constexpr bool sourceContradiction(W::Status s) noexcept {
    switch(s) {
        case W::Status::CandidateGameContradicted:
        case W::Status::CandidateMethodContradicted:
        case W::Status::CandidateSpeciesContradicted:
        case W::Status::CandidateLevelContradicted:
        case W::Status::CandidateRodContradicted:return true;
        default:return false;
    }
}
constexpr bool rngContradiction(R::Status s) noexcept {
    return s==R::Status::CandidateSlotContradicted ||
           s==R::Status::CandidateLevelContradicted ||
           s==R::Status::CandidateNatureContradicted;
}
constexpr bool physicallyLinked(const Candidate& c) noexcept {
    return c.source.originalGame==c.stage.exactOriginalGame &&
           c.source.exactGroupIndex==c.stage.exactGroupIndex &&
           c.source.physicalSlot==c.stage.attestedSlot &&
           c.source.originalLevel==c.stage.attestedLevel &&
           (c.source.originalMethod!=W::Method::Fishing ||
            c.source.originalRod==c.stage.rod);
}
inline Result evaluate(const Candidate& c) noexcept {
    if(!c.physicalCaptureIdentity || !c.originalSaveIdentity ||
       !c.originalCaptureIdentityIndependentlyAttested)
        return {};
    if(!c.sourceAndRngBelongToSameCaptureIndependentlyAttested)
        return {Status::UnknownSameCaptureLink};
    // Distinct physical slot or game/rod/source group testimony must not
    // be spliced into a fabricated single-capture Method-H history.
    if(!physicallyLinked(c))return {Status::UnknownConflictingWitness};
    const auto source=W::evaluate(c.source);
    const auto rng=R::evaluate(c.stage);
    Result out{};
    out.source=source.status;
    out.rng=rng.status;
    if(sourceContradiction(source.status)) {
        out.status=Status::CandidateOriginalSourceContradicted;
        return out;
    }
    if(source.status!=W::Status::SourceTemplateCompatibleHistoryUnverified) {
        out.status=Status::UnknownOriginalSource;
        return out;
    }
    if(rngContradiction(rng.status)) {
        out.status=Status::CandidateOriginalRngContradicted;
        return out;
    }
    if(rng.status!=R::Status::StageOnlyCompatibleHistoryUnverified) {
        out.status=Status::UnknownOriginalStage;
        return out;
    }
    out.status=Status::ConditionalSourceAndStageOnly;
    out.physicalSlot=rng.calculatedSlot;
    out.originalLevel=rng.calculatedLevel;
    out.nature=rng.calculatedNature;
    return out;
}

inline constexpr std::size_t kMaxCandidates=Gen3OriginalCaptureRouteLedger::kCapacity;
enum class UniverseStatus : uint8_t {
    UnknownOriginalRoot, UnknownCapacity, UnknownDuplicateCaptureWitness,
    UnknownUnresolvedHistories, ProvidedRoutesLocallyContradictedOnly,
    OneConditionalSameCaptureRoute
};
struct Hypothesis {
    uint64_t independentlyIdentifiedRoute=0;
    Candidate candidate{};
};
struct UniverseResult {
    UniverseStatus status=UniverseStatus::UnknownOriginalRoot;
    uint16_t compatible=0,locallyContradicted=0,unresolved=0;
    bool globalInvalidSupported=false,globallyLegalSupported=false;
};
inline UniverseResult evaluateUniverse(
    uint64_t root,bool rootIndependentlyAttested,
    std::span<const Hypothesis> hypotheses) noexcept {
    namespace L=Gen3OriginalCaptureRouteLedger;
    if(hypotheses.size()>kMaxCandidates)
        return {UniverseStatus::UnknownCapacity};
    std::array<L::Receipt,kMaxCandidates> receipts{};
    for(std::size_t i=0;i<hypotheses.size();++i) {
        const auto& h=hypotheses[i];
        const auto checked=evaluate(h.candidate);
        L::Evidence evidence=L::Evidence::Missing;
        if(checked.status==Status::ConditionalSourceAndStageOnly)
            evidence=L::Evidence::SourceStageCompatible;
        else if(checked.status==Status::CandidateOriginalSourceContradicted ||
                checked.status==Status::CandidateOriginalRngContradicted)
            evidence=L::Evidence::CandidateContradicted;
        receipts[i]={h.candidate.physicalCaptureIdentity,
                     h.candidate.originalSaveIdentity,
                     h.independentlyIdentifiedRoute,evidence,
                     h.candidate.originalCaptureIdentityIndependentlyAttested &&
                     h.candidate.sourceAndRngBelongToSameCaptureIndependentlyAttested};
    }
    const auto audit=L::reconcile(root,rootIndependentlyAttested,
        std::span<const L::Receipt>(receipts.data(),hypotheses.size()));
    UniverseResult out{};
    out.compatible=audit.compatible;
    out.locallyContradicted=audit.contradicted;
    out.unresolved=audit.unresolved;
    switch(audit.outcome) {
        case L::Outcome::UnknownRoot:
            out.status=UniverseStatus::UnknownOriginalRoot;break;
        case L::Outcome::UnknownCapacity:
            out.status=UniverseStatus::UnknownCapacity;break;
        case L::Outcome::UnknownDuplicateOrForeignRoute:
            out.status=UniverseStatus::UnknownDuplicateCaptureWitness;break;
        case L::Outcome::UnknownAlternateAcquisition:
            out.status=UniverseStatus::UnknownUnresolvedHistories;break;
        case L::Outcome::OnlyProvidedRoutesContradicted:
            out.status=UniverseStatus::ProvidedRoutesLocallyContradictedOnly;break;
        case L::Outcome::OneConditionalCaptureRoute:
            out.status=UniverseStatus::OneConditionalSameCaptureRoute;break;
    }
    return out;
}
} // namespace Legality::Gen3OriginalWildJointHistory
