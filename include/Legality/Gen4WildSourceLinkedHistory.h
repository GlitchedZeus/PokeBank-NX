#pragma once
// ENCOUNTER-004 / RNG-004: only source-attested Gen IV wild encounter
// candidate histories. Joins one original encounter row (including original
// slot/lead attributes) to the SAME Method-1 PID/IV seed and a source-verified
// encounter stage. Each failure is LOCAL to an independently supplied
// hypothesis; it never proves a Pokemon globally Invalid.
#include "Legality/Gen4WildEncounter.h"
#include "Legality/Gen4WildRngCorrelation.h"
#include "Legality/Gen4PidIvCorrelation.h"
#include "Pokemon/Pokemon4ReadOnly.h"
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace Legality::Gen4WildSourceLinkedHistory {
namespace Wild=Gen4Wild;
namespace Rng=Gen4WildRng;
enum class Lead : uint8_t { Unknown, NoLead, Synchronize };
enum class Status : uint8_t {
    UnsupportedGame, UnknownOriginalSource, UnknownOriginalSpecies,
    UnknownOriginalEncounterStage, UnknownLeadHistory,
    UnknownOriginalRngFamily, MalformedIVDomain,
    IncoherentNativeRecord, UnknownNativeSource,
    IncoherentCandidateRow, CandidateEncounterContradicted,
    CandidatePidIvContradicted, CandidateRngContradicted,
    KnownConditionalWildHistory
};
struct Observation {
    std::string_view exactOriginalGame{};
    uint16_t originalSpecies=0,originalLocation=0;
    uint8_t originalLevel=0,originalForm=0;
    uint32_t originalTrainerId32=0,pid=0;
    std::array<uint8_t,6> ivs{};
    // Fields that can be changed or lost after capture must come
    // from separately verified archival source evidence, not current
    // party level, a detached origin-game label or current species.
    bool originalWildSourceIndependentlyVerified=false;
    bool originalSpeciesAndFormIndependentlyVerified=false;
    bool originalLocationAndLevelIndependentlyVerified=false;
    bool originalTrainerIdIndependentlyVerified=false;
    bool originalPidIvSnapshotIndependentlyVerified=false;
    // This states independently that the candidate family was normal
    // Method 1; this is NOT inferred from a matching mathematical seed.
    bool originalMethodOneFamilyIndependentlyVerified=false;
};
struct Candidate {
    uint64_t routeIdentity=0;
    std::size_t originalRowIndex=0;
    bool rowIdentityIndependentlyAttested=false;
    Lead historicalLead=Lead::Unknown;
    bool originalLeadIndependentlyAttested=false;
};
struct Result {
    Status status=Status::UnknownOriginalSource;
    Rng::Result rng{};
    uint32_t reconstructedPrePidSeed=0;
    bool methodOneSeedMatched=false;
    uint8_t matchedSlot=0xFFu;
};
constexpr Enums::GameVersion requiredSourceGroup(std::string_view id) noexcept {
    switch(Wild::gameForId(id)) {
        case Wild::Game::Diamond:
        case Wild::Game::Pearl:return Enums::GameVersion::DP;
        case Wild::Game::Platinum:return Enums::GameVersion::PT;
        case Wild::Game::HeartGold:
        case Wild::Game::SoulSilver:return Enums::GameVersion::HGSS;
        case Wild::Game::Invalid:return Enums::GameVersion::Invalid;
    }
    return Enums::GameVersion::Invalid;
}
inline Result evaluate(const Observation& o,const Candidate& c) noexcept {
    Result out{};
    const auto g=Wild::gameForId(o.exactOriginalGame);
    if(g==Wild::Game::Invalid)return {Status::UnsupportedGame};
    if(!o.originalWildSourceIndependentlyVerified)return out;
    if(!o.originalSpeciesAndFormIndependentlyVerified ||
       !o.originalSpecies || o.originalSpecies>493) {
        out.status=Status::UnknownOriginalSpecies;return out;
    }
    if(!o.originalLocationAndLevelIndependentlyVerified ||
       !o.originalTrainerIdIndependentlyVerified ||
       !o.originalPidIvSnapshotIndependentlyVerified ||
       !o.originalLevel || o.originalLevel>100) {
        out.status=Status::UnknownOriginalEncounterStage;return out;
    }
    if(!c.rowIdentityIndependentlyAttested) {
        out.status=Status::UnknownOriginalEncounterStage;return out;
    }
    if(!c.originalLeadIndependentlyAttested || c.historicalLead==Lead::Unknown) {
        out.status=Status::UnknownLeadHistory;return out;
    }
    if(!o.originalMethodOneFamilyIndependentlyVerified) {
        out.status=Status::UnknownOriginalRngFamily;return out;
    }
    if(!Gen3PidIv::validStoredIVs(o.ivs)) {
        out.status=Status::MalformedIVDomain;return out;
    }
    if(c.originalRowIndex>=std::size(Wild::kPackedGen4WildEncounters)) {
        out.status=Status::IncoherentCandidateRow;return out;
    }
    const auto row=Wild::kPackedGen4WildEncounters[c.originalRowIndex];
    if(Wild::game(row)!=g) {
        out.status=Status::IncoherentCandidateRow;return out;
    }
    // Verify the exact row, not "some encounter of this species".
    if(Wild::species(row)!=o.originalSpecies ||
       Wild::location(row)!=o.originalLocation ||
       !Wild::levelMatches(row,o.originalLevel) ||
       !Wild::formMatches(Wild::form(row),o.originalForm)) {
        out.status=Status::CandidateEncounterContradicted;return out;
    }
    if(o.originalSpecies==446 && Wild::method(row)==9 &&
       !Wild::isMunchlaxTreeLocation(o.originalTrainerId32,o.originalLocation)) {
        out.status=Status::CandidateEncounterContradicted;return out;
    }
    const auto methodOne=Gen4PidIv::analyze(o.pid,o.ivs);
    if(!methodOne.matched()) {
        // The specific alleged NORMAL Method-1 source route was
        // contradicted; Cute Charm, radar, events etc remain separate.
        out.status=Status::CandidatePidIvContradicted;return out;
    }
    out.reconstructedPrePidSeed=methodOne.originSeed;
    out.methodOneSeedMatched=true;
    const bool hgss=g==Wild::Game::HeartGold||g==Wild::Game::SoulSilver;
    if(c.historicalLead==Lead::NoLead) {
        out.rng=Rng::matchNoLeadRow(
            hgss,row,methodOne.originSeed,o.pid,o.originalLevel);
        if(!out.rng.matched())out.rng=Rng::matchAcceptedNoLeadRerollRow(
            hgss,row,methodOne.originSeed,o.pid,o.originalLevel);
    } else if(c.historicalLead==Lead::Synchronize) {
        out.rng=Rng::matchSynchronizeRow(
            hgss,row,methodOne.originSeed,o.pid,o.originalLevel);
        // HGSS Bug Contest offers a separately verified Sync reroll.
        if(!out.rng.matched() && hgss && Wild::method(row)==8) {
            const auto proof=Gen4BugContestSynchronize::match(
                row,methodOne.originSeed,o.pid,o.originalLevel);
            if(proof.matched())out.rng={Rng::Method::MethodKSynchronize,
                                      methodOne.originSeed,proof.slot};
        }
    }
    if(!out.rng.matched()) {
        out.status=Status::CandidateRngContradicted;return out;
    }
    out.matchedSlot=out.rng.slot;
    out.status=Status::KnownConditionalWildHistory;
    return out;
}

// Stronger optional physical bridge for an UNCHANGED original captured
// species/PID/IV snapshot preserved in encrypted PK4. Neither a current
// party record nor a matching TID alone attests to historical wild origin.
struct NativeWitness {
    const Pokemon::Pokemon4ReadOnly* record=nullptr;
    bool originalCapturedSpeciesUnchangedIndependentlyVerified=false;
    bool nativeSaveSourceIndependentlyVerified=false;
    bool sameCreatureAndPidIvStageIndependentlyVerified=false;
};
inline Result evaluateNative(const Observation& o,const Candidate& c,
                             const NativeWitness& w) noexcept {
    if(!w.record || !w.originalCapturedSpeciesUnchangedIndependentlyVerified ||
       !w.sameCreatureAndPidIvStageIndependentlyVerified)
        return {Status::UnknownOriginalEncounterStage};
    const auto& n=*w.record;
    if(!n.valid() || n.empty() || n.isEgg())return {Status::IncoherentNativeRecord};
    if(!w.nativeSaveSourceIndependentlyVerified ||
       n.sourceGroup()!=requiredSourceGroup(o.exactOriginalGame))
        return {Status::UnknownNativeSource};
    if(n.species()!=o.originalSpecies || n.form()!=o.originalForm ||
       n.pid()!=o.pid || n.ivs()!=o.ivs)
        return {Status::IncoherentNativeRecord};
    return evaluate(o,c);
}

enum class UniverseStatus : uint8_t {
    NoCandidates, CapacityExceededUnknown, UnknownRoot,
    UnknownRouteIdentity, OneConditionalWildRoute,
    UnknownAlternativeProvenance, UnknownNegativeMethodCoverage,
    UnknownCompleteEnumeration, AllAttestedWildRoutesContradicted
};
struct Alternative {
    Candidate candidate{};
    // The route must be independently sourced to this original encounter,
    // rather than invented from a current matching move/species/slot.
    bool alternativeHistoricallyGrounded=false;
    // This covers ONLY the exact Normal Method-1 lead/slot hypothesis.
    bool completeNegativeMethodCoverageIndependentlyVerified=false;
};
struct UniverseInput {
    Observation observation{};
    NativeWitness native{};
    bool originalWildUniverseIndependentlyExhaustivelyEnumerated=false;
    std::array<Alternative,8> alternatives{};
    uint8_t count=0;
};
struct UniverseResult {
    UniverseStatus status=UniverseStatus::NoCandidates;
    uint8_t considered=0,compatible=0,contradicted=0,unresolved=0,unverified=0;
    std::array<Status,8> individual{};
};
constexpr bool localContradiction(Status s) noexcept {
    return s==Status::CandidateEncounterContradicted ||
           s==Status::CandidatePidIvContradicted ||
           s==Status::CandidateRngContradicted;
}
inline UniverseResult evaluateUniverse(const UniverseInput& u) noexcept {
    UniverseResult out{};
    if(!u.count)return out;
    if(u.count>u.alternatives.size()) {
        out.status=UniverseStatus::CapacityExceededUnknown;return out;
    }
    if(!u.native.record || !u.native.nativeSaveSourceIndependentlyVerified ||
       !u.native.record->valid() || u.native.record->empty() ||
       u.native.record->isEgg()) {
        out.status=UniverseStatus::UnknownRoot;return out;
    }
    for(uint8_t i=0;i<u.count;++i) {
        const uint64_t id=u.alternatives[i].candidate.routeIdentity;
        if(!id) {
            out.status=UniverseStatus::UnknownRouteIdentity;return out;
        }
        for(uint8_t j=0;j<i;++j)
            if(u.alternatives[j].candidate.routeIdentity==id) {
                out.status=UniverseStatus::UnknownRouteIdentity;return out;
            }
    }
    for(uint8_t i=0;i<u.count;++i) {
        ++out.considered;
        const auto& alt=u.alternatives[i];
        if(!alt.alternativeHistoricallyGrounded) {
            ++out.unverified;continue;
        }
        const auto result=evaluateNative(u.observation,alt.candidate,u.native);
        out.individual[i]=result.status;
        // A borrowed/out-of-range/cross-game encounter row is not a
        // source-grounded negative history, even with a forged method
        // coverage flag. Never count missing/alien rows as exhausted.
        if(result.status==Status::IncoherentCandidateRow ||
           result.status==Status::IncoherentNativeRecord ||
           result.status==Status::UnknownNativeSource ||
           result.status==Status::UnknownOriginalSource) {
            ++out.unverified;continue;
        }
        if(result.status==Status::KnownConditionalWildHistory) {
            ++out.compatible;continue;
        }
        if(localContradiction(result.status) &&
           alt.completeNegativeMethodCoverageIndependentlyVerified) {
            ++out.contradicted;continue;
        }
        ++out.unresolved;
    }
    if(out.compatible)out.status=UniverseStatus::OneConditionalWildRoute;
    else if(out.unverified)out.status=UniverseStatus::UnknownAlternativeProvenance;
    else if(out.unresolved)out.status=UniverseStatus::UnknownNegativeMethodCoverage;
    else if(!u.originalWildUniverseIndependentlyExhaustivelyEnumerated)
        out.status=UniverseStatus::UnknownCompleteEnumeration;
    else if(out.contradicted==out.considered)
        out.status=UniverseStatus::AllAttestedWildRoutesContradicted;
    else out.status=UniverseStatus::UnknownNegativeMethodCoverage;
    return out;
}
} // namespace Legality::Gen4WildSourceLinkedHistory
