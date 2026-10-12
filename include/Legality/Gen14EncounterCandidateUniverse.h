#pragma once
// L1 E17/E18: coordinator-wiring-ready, read-only aggregation of source-
// specific ORIGINAL encounter candidate evidence. This module deliberately
// has NO "Invalid" output: contradiction of any number of attested paths
// is not proof that the entire historical candidate universe is complete.
#include "Legality/Gen1IndexedOriginalSourceHistory.h"
#include "Legality/Gen2IndexedOriginalSourceHistory.h"
#include "Legality/Gen3SafariIndexedOriginalHistory.h"
#include "Legality/Gen3GameCubeIndexedShadowHistory.h"
#include "Legality/Gen4SinnohSpecialOriginalStage.h"
#include "Legality/Gen4StaticIndexedOriginalHistory.h"
#include "Legality/Gen4WildSpecialStageEvidence.h"
#include "Legality/Gen3MethodHOriginalStage.h"
#include "Legality/Gen3PidIvFamilyRouter.h"
#include "Legality/Gen4NatureReversalReplay.h"
#include "Legality/Gen4WildSourceLinkedHistory.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
namespace Legality::Gen14EncounterCandidateUniverse {
enum class Family : uint8_t {
    Gen1StaticTrade, Gen2Wild, Gen2StaticGift,
    Gen3Safari, Gen3GameCubeShadow, Gen4SinnohWild,
    Gen4StaticGiftRoamer, Gen4HgssSpecial,
    Gen3EmeraldMethodH, Gen3PidIvOnly, Gen4WildSourceLinked,
    Gen4NatureStageOnly, Count
};
enum class EvidenceKind : uint8_t {
    UnknownOriginalHistory, CandidateLocalContradiction,
    OriginalTemplateCompatible, ConditionalOriginalHistory
};
struct Candidate {
    Family family=Family::Gen1StaticTrade;
    EvidenceKind evidence=EvidenceKind::UnknownOriginalHistory;
    // Candidate's source-game, original save and relevant capture-stage
    // have independently recorded provenance; never infer this from a
    // matching row, present-day trainer, or mathematical correlation.
    bool originalSourceIndependentlyAttested=false;
};
enum class Outcome : uint8_t {
    UnknownOrMissingOriginalHistories,
    CandidateSetLocallyContradictedOnly,
    SourceTemplateCompatibleHistoryUnproven,
    SourceAttestedConditionalHistory
};
struct Summary {
    Outcome outcome=Outcome::UnknownOrMissingOriginalHistories;
    std::size_t supplied=0,verifiedConditional=0,sourceTemplateOnly=0;
    std::size_t locallyContradicted=0,unknown=0;
    // A caller's chosen subset of candidates is NEVER an exhaustive
    // original-retail encounter proof; do not expose global Invalid.
    bool globalInvalidSupported=false;
};
constexpr EvidenceKind map(Gen1IndexedOriginalSourceHistory::Status s) noexcept {
    namespace H=Gen1IndexedOriginalSourceHistory;
    if(s==H::Status::ConditionalOriginalSource)
        return EvidenceKind::OriginalTemplateCompatible;
    switch(s) {
        case H::Status::CandidateOriginalSpeciesContradicted:
        case H::Status::CandidateOriginalLevelContradicted:
        case H::Status::CandidateEvolutionContradicted:
        case H::Status::CandidateOriginalCatchRateContradicted:
            return EvidenceKind::CandidateLocalContradiction;
        default: return EvidenceKind::UnknownOriginalHistory;
    }
}
constexpr EvidenceKind map(Gen2IndexedOriginalSourceHistory::Status s) noexcept {
    namespace H=Gen2IndexedOriginalSourceHistory;
    if(s==H::Status::ConditionalOriginalSource)
        return EvidenceKind::OriginalTemplateCompatible;
    switch(s) {
        case H::Status::CandidateSpeciesContradicted:
        case H::Status::CandidateLocationContradicted:
        case H::Status::CandidateLevelContradicted:
        case H::Status::CandidateMethodContradicted:
        case H::Status::CandidateTimeContradicted:
        case H::Status::CandidateEggContradicted:
        case H::Status::CandidateShinyContradicted:
            return EvidenceKind::CandidateLocalContradiction;
        default: return EvidenceKind::UnknownOriginalHistory;
    }
}
constexpr EvidenceKind map(Gen3SafariIndexedOriginalHistory::Status s) noexcept {
    namespace H=Gen3SafariIndexedOriginalHistory;
    if(s==H::Status::ConditionalOriginalSafariHistory)
        return EvidenceKind::ConditionalOriginalHistory;
    switch(s) {
        case H::Status::CandidateRowContradicted:
        case H::Status::CandidateBallContradicted:
        case H::Status::CandidatePidIvContradicted:
            return EvidenceKind::CandidateLocalContradiction;
        default: return EvidenceKind::UnknownOriginalHistory;
    }
}
constexpr EvidenceKind map(Gen3GameCubeIndexedShadowHistory::Status s) noexcept {
    namespace H=Gen3GameCubeIndexedShadowHistory;
    if(s==H::Status::ConditionalShadowHistory)
        return EvidenceKind::ConditionalOriginalHistory;
    switch(s) {
        case H::Status::CandidateOriginalIdentityContradicted:
        case H::Status::CandidatePidIvContradicted:
        case H::Status::CandidateTeamHistoryContradicted:
            return EvidenceKind::CandidateLocalContradiction;
        default: return EvidenceKind::UnknownOriginalHistory;
    }
}
constexpr EvidenceKind map(Gen4SinnohSpecialOriginalStage::Status s) noexcept {
    namespace H=Gen4SinnohSpecialOriginalStage;
    if(s==H::Status::ConditionalOriginalWildSource ||
       s==H::Status::ConditionalOriginalHoneySource ||
       s==H::Status::ConditionalRadarSourceAwaitingPidReplay)
        return EvidenceKind::OriginalTemplateCompatible;
    switch(s) {
        case H::Status::CandidateOriginalSpeciesOrFormContradicted:
        case H::Status::CandidateOriginalLocationOrLevelContradicted:
        case H::Status::CandidateOriginalMethodContradicted:
        case H::Status::CandidateTreeSelectionContradicted:
        case H::Status::CandidateRadarSourceContradicted:
            return EvidenceKind::CandidateLocalContradiction;
        default: return EvidenceKind::UnknownOriginalHistory;
    }
}
constexpr EvidenceKind map(Gen4StaticIndexedOriginalHistory::Status s) noexcept {
    namespace H=Gen4StaticIndexedOriginalHistory;
    if(s==H::Status::ConditionalOriginalGiftEggSourceAwaitingBreedingVerification ||
       s==H::Status::ConditionalOriginalStaticHistory)
        return EvidenceKind::OriginalTemplateCompatible;
    switch(s) {
        case H::Status::CandidateIdentityContradicted:
        case H::Status::CandidateCaptureStageContradicted:
        case H::Status::CandidateEggStageContradicted:
        case H::Status::CandidateBallContradicted:
        case H::Status::CandidateTemplateConstraintsContradicted:
        case H::Status::CandidatePidFamilyContradicted:
            return EvidenceKind::CandidateLocalContradiction;
        default: return EvidenceKind::UnknownOriginalHistory;
    }
}
constexpr EvidenceKind map(Gen4WildSpecialStageEvidence::Status s) noexcept {
    namespace H=Gen4WildSpecialStageEvidence;
    if(s==H::Status::KnownConditionalSpecialEncounter)
        return EvidenceKind::ConditionalOriginalHistory;
    switch(s) {
        case H::Status::CandidateRowContradicted:
        case H::Status::CandidatePidIvContradicted:
        case H::Status::IncoherentNativeRecord:
            return EvidenceKind::CandidateLocalContradiction;
        default: return EvidenceKind::UnknownOriginalHistory;
    }
}
// RNG correlation or a nature-reversal frame is one stage, NOT an
// original encounter row. Promote it only to template/stage compatibility.
constexpr EvidenceKind map(Gen3PidIvFamilyRouter::Status s) noexcept {
    return s==Gen3PidIvFamilyRouter::Status::ConditionalRngCorrelation ?
        EvidenceKind::OriginalTemplateCompatible :
        EvidenceKind::UnknownOriginalHistory;
}
constexpr EvidenceKind map(Gen4NatureReversalReplay::Status s) noexcept {
    using S=Gen4NatureReversalReplay::Status;
    if(s==S::AttestedStageMatched)
        return EvidenceKind::OriginalTemplateCompatible;
    if(s==S::AttestedStagePidContradicted)
        return EvidenceKind::CandidateLocalContradiction;
    return EvidenceKind::UnknownOriginalHistory;
}
constexpr EvidenceKind map(Gen3MethodHOriginalStage::Status s) noexcept {
    using S=Gen3MethodHOriginalStage::Status;
    if(s==S::KnownConditionalEmeraldSafariHistory)
        return EvidenceKind::ConditionalOriginalHistory;
    if(s==S::CandidateRowContradicted ||
       s==S::CandidatePidIvContradicted)
        return EvidenceKind::CandidateLocalContradiction;
    return EvidenceKind::UnknownOriginalHistory;
}
constexpr EvidenceKind map(Gen4WildSourceLinkedHistory::Status s) noexcept {
    using S=Gen4WildSourceLinkedHistory::Status;
    if(s==S::KnownConditionalWildHistory)
        return EvidenceKind::ConditionalOriginalHistory;
    if(s==S::CandidateEncounterContradicted ||
       s==S::CandidatePidIvContradicted ||
       s==S::CandidateRngContradicted ||
       s==S::IncoherentCandidateRow ||
       s==S::IncoherentNativeRecord)
        return EvidenceKind::CandidateLocalContradiction;
    return EvidenceKind::UnknownOriginalHistory;
}
constexpr Summary summarize(std::span<const Candidate> candidates) noexcept {
    Summary out{};
    out.supplied=candidates.size();
    if(candidates.empty())return out;
    for(const auto& candidate:candidates){
        // A hypothetical field match lacking an original provenance witness
        // has no authority to refute or affirm a retail origin.
        if(!candidate.originalSourceIndependentlyAttested){
            ++out.unknown;
            continue;
        }
        switch(candidate.evidence) {
            case EvidenceKind::ConditionalOriginalHistory:
                ++out.verifiedConditional;break;
            case EvidenceKind::OriginalTemplateCompatible:
                ++out.sourceTemplateOnly;break;
            case EvidenceKind::CandidateLocalContradiction:
                ++out.locallyContradicted;break;
            case EvidenceKind::UnknownOriginalHistory:
                ++out.unknown;break;
        }
    }
    if(out.verifiedConditional>0)
        out.outcome=Outcome::SourceAttestedConditionalHistory;
    else if(out.sourceTemplateOnly>0)
        out.outcome=Outcome::SourceTemplateCompatibleHistoryUnproven;
    else if(out.unknown>0)
        out.outcome=Outcome::UnknownOrMissingOriginalHistories;
    else
        out.outcome=Outcome::CandidateSetLocallyContradictedOnly;
    return out;
}
} // namespace Legality::Gen14EncounterCandidateUniverse
