#pragma once
// Platinum/HGSS original daycare source + same-route PID/IV/four moves.
// No historical data inferred from a PK4; exclusions are candidate-local.
#include "Legality/Gen4PlatinumHGSSFullEggHistories.h"
#include "Legality/Gen4PlatinumHGSSOriginalDaycareSource.h"
#include "Legality/Gen34OriginalDaycareRoles.h"
namespace Legality::Gen4PlatinumHGSSSourceBoundEggHistories {
namespace F=Gen4PlatinumHGSSFullEggHistories;
namespace S=Gen4PlatinumHGSSOriginalDaycareSource;
enum class Status : unsigned char { Unsupported, NoCandidates, Overflow,
    OneMatched, UnknownSources, Unresolved, UnknownEnumeration, AllLocalContradicted };
struct Input {
    F::Input full{};
    std::array<S::Input,8> archives{};
    bool allArchivesIndependentlyEnumerated=false;
};
struct Result {
    Status status=Status::NoCandidates;
    unsigned char matched=0,contradicted=0,unknown=0,unverified=0;
    std::array<S::Status,8> sourceChecks{};
    std::array<F::Status,8> routeChecks{};
};
constexpr Result evaluate(const Input& in) noexcept {
    Result out{};
    const auto& h=in.full.original;
    if(!F::Moves::supported(h.game)){out.status=Status::Unsupported;return out;}
    if(!h.count)return out;
    if(h.count>8){out.status=Status::Overflow;return out;}
    bool duplicate=false;
    for(unsigned char i=0;i<h.count;++i){
        const auto& route=h.alternatives[i];
        const auto& p=route.pid;
        if(!route.routeIdentity){++out.unverified;duplicate=true;continue;}
        for(unsigned char j=0;j<i;++j)
            if(route.routeIdentity==h.alternatives[j].routeIdentity)duplicate=true;
        const auto roles=Gen34OriginalDaycareRoles::evaluate(
            {p.parents[0].species,p.parents[0].gender},
            {p.parents[1].species,p.parents[1].gender},493);
        if(roles==Gen34OriginalDaycareRoles::Status::UnknownOriginalMetadata){
            ++out.unverified;continue;
        }
        const S::Expected expected{h.game,h.originalEggEventIdentity,
            route.originalDaycarePairIdentity,
            {{{p.parents[0].snapshotId,p.parents[0].species,p.parents[0].heldItem,
               p.parents[0].gender,p.originalParentLanguages[0],p.parents[0].originalPID},
              {p.parents[1].snapshotId,p.parents[1].species,p.parents[1].heldItem,
               p.parents[1].gender,p.originalParentLanguages[1],p.parents[1].originalPID}}},
            p.parentLanguagesAtCollectionIndependentlyVerified,
            p.parents[0].originalNaturePIDIndependentlyVerified &&
            p.parents[1].originalNaturePIDIndependentlyVerified};
        out.sourceChecks[i]=S::evaluate(in.archives[i],expected);
        if(out.sourceChecks[i]!=S::Status::ConditionallyAttested ||
           !route.originalSourceHistoryIndependentlyAttested){
            ++out.unverified;continue;
        }
        auto single=in.full;
        single.original.count=1;
        single.original.alternatives[0]=route;
        single.moveStages[0]=in.full.moveStages[i];
        single.original.allOrdinaryDaycareAlternativesIndependentlyEnumerated=true;
        single.allJointOrdinaryDaycareRoutesIndependentlyEnumerated=true;
        out.routeChecks[i]=F::evaluate(single).status;
        if(out.routeChecks[i]==F::Status::OneConditionallyConsistentFullHistory){
            ++out.matched;continue;
        }
        if((roles==Gen34OriginalDaycareRoles::Status::IncompatibleOrdinaryPair ||
            out.routeChecks[i]==F::Status::AllAttestedOrdinaryDaycareRoutesContradicted) &&
           route.fullyAttestedForCandidateLocalExclusion && !duplicate)
            ++out.contradicted;
        else ++out.unknown;
    }
    if(out.matched)out.status=Status::OneMatched;
    else if(duplicate||out.unverified)out.status=Status::UnknownSources;
    else if(out.unknown)out.status=Status::Unresolved;
    else if(!in.allArchivesIndependentlyEnumerated ||
            !in.full.allJointOrdinaryDaycareRoutesIndependentlyEnumerated ||
            !h.allOrdinaryDaycareAlternativesIndependentlyEnumerated)
        out.status=Status::UnknownEnumeration;
    else if(out.contradicted==h.count)out.status=Status::AllLocalContradicted;
    else out.status=Status::Unresolved;
    return out;
}
} // namespace Legality::Gen4PlatinumHGSSSourceBoundEggHistories
