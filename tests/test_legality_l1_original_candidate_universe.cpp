#include "Legality/Gen14EncounterCandidateUniverse.h"
#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
int main(){
    namespace U=Legality::Gen14EncounterCandidateUniverse;
    using Kind=U::EvidenceKind;using Outcome=U::Outcome;using Family=U::Family;
    // Compile-time source-helper status contracts. In particular, bounded
    // search exhaustion, alternate exact game and unsupported stages cannot
    // be smuggled into a globally Invalid outcome.
    static_assert(U::map(
        Legality::Gen1IndexedOriginalSourceHistory::Status::ConditionalOriginalSource)
        ==Kind::OriginalTemplateCompatible);
    static_assert(U::map(
        Legality::Gen2IndexedOriginalSourceHistory::Status::CandidateMethodContradicted)
        ==Kind::CandidateLocalContradiction);
    static_assert(U::map(
        Legality::Gen2IndexedOriginalSourceHistory::Status::UnknownOriginalTime)
        ==Kind::UnknownOriginalHistory);
    static_assert(U::map(
        Legality::Gen1IndexedOriginalSourceHistory::Status::UnknownLaterGenerationEvolution)
        ==Kind::UnknownOriginalHistory);
    static_assert(U::map(
        Legality::Gen1IndexedOriginalSourceHistory::Status::UnknownCurrentSpecies)
        ==Kind::UnknownOriginalHistory);
    static_assert(U::map(
        Legality::Gen1IndexedOriginalSourceHistory::Status::UnknownEncounterKind)
        ==Kind::UnknownOriginalHistory);
    static_assert(U::map(
        Legality::Gen3SafariIndexedOriginalHistory::Status::ConditionalOriginalSafariHistory)
        ==Kind::ConditionalOriginalHistory);
    static_assert(U::map(
        Legality::Gen3SafariIndexedOriginalHistory::Status::ForeignRowNotEvidence)
        ==Kind::UnknownOriginalHistory);
    static_assert(U::map(
        Legality::Gen3GameCubeIndexedShadowHistory::Status::BoundedTeamSearchUnknown)
        ==Kind::UnknownOriginalHistory);
    static_assert(U::map(
        Legality::Gen4SinnohSpecialOriginalStage::Status::ConditionalRadarSourceAwaitingPidReplay)
        ==Kind::OriginalTemplateCompatible);
    static_assert(U::map(
        Legality::Gen4StaticIndexedOriginalHistory::Status::ConditionalOriginalGiftEggSourceAwaitingBreedingVerification)
        ==Kind::OriginalTemplateCompatible);
    static_assert(U::map(
        Legality::Gen4WildSpecialStageEvidence::Status::BoundedCandidateUnresolved)
        ==Kind::UnknownOriginalHistory);
    static_assert(U::map(
        Legality::Gen4WildSpecialStageEvidence::Status::KnownConditionalSpecialEncounter)
        ==Kind::ConditionalOriginalHistory);
    static_assert(U::map(
        Legality::Gen3MethodHOriginalStage::Status::KnownConditionalEmeraldSafariHistory)
        ==Kind::ConditionalOriginalHistory);
    static_assert(U::map(
        Legality::Gen3MethodHOriginalStage::Status::UnsupportedLeadHistory)
        ==Kind::UnknownOriginalHistory);
    static_assert(U::map(
        Legality::Gen3GameCubeOriginalFixedHistory::Status::ConditionalOriginalFixedHistory)
        ==Kind::ConditionalOriginalHistory);
    static_assert(U::map(
        Legality::Gen3GameCubeOriginalFixedHistory::Status::CandidateOriginalPidIvContradicted)
        ==Kind::CandidateLocalContradiction);
    static_assert(U::map(
        Legality::Gen3GameCubeOriginalFixedHistory::Status::UnknownRecipientSave)
        ==Kind::UnknownOriginalHistory);
    static_assert(U::map(
        Legality::Gen3GameCubeOriginalFixedHistory::Status::UnknownMalformedOriginalNativeIvs)
        ==Kind::UnknownOriginalHistory);
    static_assert(U::map(
        Legality::Gen3GameCubeOriginalFixedHistory::Status::BoundedOriginalRngUnresolved)
        ==Kind::UnknownOriginalHistory);
    static_assert(U::map(
        Legality::Gen3OriginalRoamerStage::Status::ConditionalOriginalRoamerHistory)
        ==Kind::ConditionalOriginalHistory);
    static_assert(U::map(
        Legality::Gen3OriginalRoamerStage::Status::CandidateOriginalPidIvContradicted)
        ==Kind::CandidateLocalContradiction);
    static_assert(U::map(
        Legality::Gen3OriginalRoamerStage::Status::UnknownOriginalRoamerSelection)
        ==Kind::UnknownOriginalHistory);
    static_assert(U::map(
        Legality::Gen3PidIvFamilyRouter::Status::ConditionalRngCorrelation)
        ==Kind::OriginalTemplateCompatible);
    static_assert(U::map(
        Legality::Gen3PidIvFamilyRouter::Status::SearchBoundExhausted)
        ==Kind::UnknownOriginalHistory);
    static_assert(U::map(
        Legality::Gen4NatureReversalReplay::Status::AttestedStageMatched)
        ==Kind::OriginalTemplateCompatible);
    static_assert(U::map(
        Legality::Gen4NatureReversalReplay::Status::SearchExhausted)
        ==Kind::UnknownOriginalHistory);
    static_assert(U::map(
        Legality::Gen4WildSourceLinkedHistory::Status::KnownConditionalWildHistory)
        ==Kind::ConditionalOriginalHistory);
    static_assert(U::map(
        Legality::Gen4WildSourceLinkedHistory::Status::UnknownLeadHistory)
        ==Kind::UnknownOriginalHistory);
    constexpr std::array<U::Candidate,4> mixed{{
        {Family::Gen1StaticTrade,Kind::CandidateLocalContradiction,true},
        {Family::Gen2Wild,Kind::UnknownOriginalHistory,true},
        {Family::Gen3Safari,Kind::OriginalTemplateCompatible,true},
        {Family::Gen4HgssSpecial,Kind::ConditionalOriginalHistory,true}
    }};
    constexpr auto mix=U::summarize(mixed);
    static_assert(mix.outcome==Outcome::SourceAttestedConditionalHistory);
    static_assert(mix.verifiedConditional==1u && mix.sourceTemplateOnly==1u);
    static_assert(!mix.globalInvalidSupported);
    constexpr std::array<U::Candidate,2> noAlternative{{
        {Family::Gen3GameCubeShadow,Kind::CandidateLocalContradiction,true},
        {Family::Gen4StaticGiftRoamer,Kind::CandidateLocalContradiction,true}
    }};
    constexpr auto local=U::summarize(noAlternative);
    static_assert(local.outcome==Outcome::CandidateSetLocallyContradictedOnly);
    static_assert(local.locallyContradicted==2u);
    static_assert(!local.globalInvalidSupported);
    constexpr std::array<U::Candidate,2> exhausted{{
        {Family::Gen4HgssSpecial,Kind::CandidateLocalContradiction,true},
        {Family::Gen3GameCubeShadow,Kind::UnknownOriginalHistory,true}
    }};
    static_assert(U::summarize(exhausted).outcome==
                  Outcome::UnknownOrMissingOriginalHistories);
    constexpr std::array<U::Candidate,1> unverified{{
        {Family::Gen4SinnohWild,Kind::ConditionalOriginalHistory,false}
    }};
    static_assert(U::summarize(unverified).outcome==
                  Outcome::UnknownOrMissingOriginalHistories);
    // A source-only attested wild Honey/Radar or gifted egg path must not
    // be presented as proof of full RNG legality.
    constexpr std::array<U::Candidate,2> sourceOnly{{
        {Family::Gen4SinnohWild,Kind::OriginalTemplateCompatible,true},
        {Family::Gen4StaticGiftRoamer,Kind::OriginalTemplateCompatible,true}
    }};
    static_assert(U::summarize(sourceOnly).outcome==
                  Outcome::SourceTemplateCompatibleHistoryUnproven);
    // Adversarial alternatives: failed Safari/roamer checks do not rule
    // out later transfers, an event or an egg route without original
    // evidence. The family set here is intentionally non-exhaustive.
    constexpr std::array<U::Candidate,5> alternativeAcquisition{{
        {Family::Gen3Safari,Kind::CandidateLocalContradiction,true},
        {Family::Gen3OriginalRoamer,Kind::CandidateLocalContradiction,true},
        {Family::Gen4StaticGiftRoamer,Kind::OriginalTemplateCompatible,false},
        {Family::Gen2StaticGift,Kind::UnknownOriginalHistory,true},
        {Family::Gen4SinnohWild,Kind::CandidateLocalContradiction,true}
    }};
    constexpr auto alternative=U::summarize(alternativeAcquisition);
    static_assert(alternative.outcome==Outcome::UnknownOrMissingOriginalHistories);
    static_assert(alternative.locallyContradicted==3u);
    static_assert(alternative.unknown==2u);
    static_assert(!alternative.globalInvalidSupported);
    // An unrecognized family or evidence byte may originate in corrupted
    // external observations. It is never proof of a capture, even if its
    // boolean witness has accidentally been set true.
    constexpr std::array<U::Candidate,3> malformed{{
        {Family::Count,Kind::ConditionalOriginalHistory,true},
        {static_cast<Family>(255),Kind::CandidateLocalContradiction,true},
        {Family::Gen1StaticTrade,static_cast<Kind>(255),true}
    }};
    constexpr auto malformedResult=U::summarize(malformed);
    static_assert(malformedResult.outcome==Outcome::UnknownOrMissingOriginalHistories);
    static_assert(malformedResult.unknown==3u);
    static_assert(malformedResult.verifiedConditional==0u);
    static_assert(!malformedResult.globalInvalidSupported);
    constexpr auto allCandidateFamiliesContradicted=[]{
        std::array<U::Candidate,static_cast<std::size_t>(Family::Count)> a{};
        for(std::size_t i=0;i<a.size();++i)
            a[i]={static_cast<Family>(i),Kind::CandidateLocalContradiction,true};
        return a;
    }();
    constexpr auto allFailed=U::summarize(allCandidateFamiliesContradicted);
    static_assert(allFailed.locallyContradicted==
                  static_cast<std::size_t>(Family::Count));
    static_assert(allFailed.outcome==Outcome::CandidateSetLocallyContradictedOnly);
    static_assert(!allFailed.globalInvalidSupported);
    constexpr std::array<U::Candidate,0> empty{};
    static_assert(U::summarize(empty).outcome==
                  Outcome::UnknownOrMissingOriginalHistories);
    // Same extracted source family can have two competing physical rows:
    // one locally contradictory and another unknown => NOT global Invalid.
    std::array<U::Candidate,2> duplicate{{
        {Family::Gen3Safari,Kind::CandidateLocalContradiction,true},
        {Family::Gen3Safari,Kind::UnknownOriginalHistory,true}
    }};
    assert(U::summarize(duplicate).outcome==
           Outcome::UnknownOrMissingOriginalHistories);
    duplicate[1].evidence=Kind::ConditionalOriginalHistory;
    assert(U::summarize(duplicate).outcome==
           Outcome::SourceAttestedConditionalHistory);
    for(auto evidence:{
        Kind::UnknownOriginalHistory,Kind::CandidateLocalContradiction,
        Kind::OriginalTemplateCompatible,Kind::ConditionalOriginalHistory
    }){
        const std::array<U::Candidate,1> one{{
            {Family::Gen4HgssSpecial,evidence,true}
        }};
        assert(!U::summarize(one).globalInvalidSupported);
    }
    std::cout<<"L1 E17/E18 original encounter candidate universe never global Invalid: PASS\n";
}
