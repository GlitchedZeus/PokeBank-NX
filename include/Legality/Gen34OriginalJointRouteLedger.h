#pragma once
// L2 Stage 5: bounded original-route conjunction, not an OR of separate
// positive PID, IV, move and collection candidates. Negatives are LOCAL.
#include "Legality/Gen34OriginalSameRouteProof.h"
#include "Legality/Gen34OriginalParentLineage.h"
#include <array>
#include <cstdint>
namespace Legality::Gen34OriginalJointRouteLedger {
namespace Identity=Gen34OriginalSameRouteProof;
enum class Verdict : uint8_t { Unknown, Matched, Contradicted };
enum class Status : uint8_t {
    NoCandidates, CapacityUnknown, OneConditionallyMatched,
    UnverifiedOrDuplicateSources, UnresolvedSourceHistory,
    UnknownExhaustiveEnumeration, AllAttestedLocalHistoriesContradicted
};
struct Candidate {
    uint64_t routeId=0;
    Identity::Input originalStages{};
    Identity::Expected originalIdentity{};
    std::array<Verdict,4> stageVerdicts{};
    bool allMethodsForThisRouteIndependentlyCovered=false;
    Gen34OriginalParentLineage::Input originalParentLineage{};
    bool depositedSpeciesIndependentlyAttested=false;
    std::array<uint16_t,2> depositedSpecies{};
};
struct Input {
    std::array<Candidate,8> routes{};
    uint8_t count=0;
    bool allOriginalRoutesIndependentlyEnumerated=false;
};
struct Result {
    Status status=Status::NoCandidates;
    uint8_t considered=0,matched=0,contradicted=0,unknown=0,unverified=0;
    std::array<Identity::Status,8> identityChecks{};
    std::array<Gen34OriginalParentLineage::Status,8> lineageChecks{};
};
constexpr Result evaluate(const Input& in) noexcept {
    Result out{};
    if(!in.count)return out;
    if(in.count>in.routes.size()){out.status=Status::CapacityUnknown;return out;}
    bool duplicated=false;
    for(uint8_t i=0;i<in.count;++i) {
        ++out.considered;
        const auto& c=in.routes[i];
        if(!c.routeId){duplicated=true;++out.unverified;continue;}
        bool repeated=false;
        for(uint8_t j=0;j<i;++j)
            if(c.routeId==in.routes[j].routeId)repeated=true;
        if(repeated) {duplicated=true;++out.unverified;}
        const auto identity=Identity::evaluate(c.originalStages,c.originalIdentity);
        out.identityChecks[i]=identity;
        if(identity!=Identity::Status::ConditionallySameRoute) {
            ++out.unverified;continue;
        }
        if(c.originalParentLineage.supplied){
            namespace Lineage=Gen34OriginalParentLineage;
            if(!c.depositedSpeciesIndependentlyAttested){
                ++out.unverified;continue;
            }
            out.lineageChecks[i]=Lineage::evaluate(c.originalParentLineage,{
                c.originalIdentity.game,
                {c.originalIdentity.slot0Id,c.originalIdentity.slot1Id},
                c.depositedSpecies});
            if(out.lineageChecks[i]!=Lineage::Status::ConditionallyReplayed){
                ++out.unverified;continue;
            }
        }
        bool missing=false,contradicted=false;
        for(const auto v:c.stageVerdicts) {
            missing|=v==Verdict::Unknown;
            contradicted|=v==Verdict::Contradicted;
        }
        if(!missing && !contradicted){++out.matched;continue;}
        // Duplicated archive records and partially attested methods can
        // neither rescue an unrelated stage nor prove exhaustion.
        if(!missing && contradicted &&
           c.allMethodsForThisRouteIndependentlyCovered && !repeated)
            ++out.contradicted;
        else ++out.unknown;
    }
    if(out.matched)out.status=Status::OneConditionallyMatched;
    else if(duplicated||out.unverified)
        out.status=Status::UnverifiedOrDuplicateSources;
    else if(out.unknown)out.status=Status::UnresolvedSourceHistory;
    else if(!in.allOriginalRoutesIndependentlyEnumerated)
        out.status=Status::UnknownExhaustiveEnumeration;
    else if(out.contradicted==out.considered)
        out.status=Status::AllAttestedLocalHistoriesContradicted;
    else out.status=Status::UnresolvedSourceHistory;
    return out;
}
} // namespace Legality::Gen34OriginalJointRouteLedger
