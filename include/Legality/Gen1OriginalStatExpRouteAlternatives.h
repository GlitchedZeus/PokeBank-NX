#pragma once
// L2 Gen I: independently source-attested ALTERNATIVE original histories.
// A contradicted candidate history is not proof that a PK1 is illegal.
// No ability, nature, held item, gender or breeding exists in Gen I.
#include "Legality/Gen1OriginalStatExpBattleHistory.h"
#include <array>
#include <cstdint>
namespace Legality::Gen1OriginalStatExpRouteAlternatives {
namespace H=Gen1OriginalStatExpBattleHistory;
enum class Status : std::uint8_t {
    NoOriginalRoutes, SourceCapacityUnknown, OneSupportedOriginalHistory,
    UnresolvedOriginalHistories, UnknownOriginalRouteEnumeration,
    AllAttestedOriginalHistoriesContradicted
};
struct Candidate {
    std::uint64_t independentlyIdentifiedRoute=0;
    bool routeIdentityIndependentlyAttested=false;
    H::History history{};
};
struct Input {
    std::array<Candidate,8> routes{};
    std::uint8_t count=0;
    bool allPossibleOriginalRoutesIndependentlyEnumerated=false;
};
struct Result {
    Status status=Status::NoOriginalRoutes;
    std::uint8_t supported=0, contradicted=0, unknown=0;
};
inline Result evaluate(const H::G::PokemonRecord& pokemon,
                       const Input& input) noexcept {
    Result out{};
    if(!input.count)return out;
    if(input.count>input.routes.size()){
        out.status=Status::SourceCapacityUnknown;return out;
    }
    for(std::uint8_t i=0;i<input.count;++i){
        const auto& candidate=input.routes[i];
        bool repeated=false;
        for(std::uint8_t j=0;j<i;++j)
            repeated|=candidate.independentlyIdentifiedRoute==
                      input.routes[j].independentlyIdentifiedRoute;
        if(!candidate.routeIdentityIndependentlyAttested ||
           !candidate.independentlyIdentifiedRoute || repeated){
            ++out.unknown;continue;
        }
        const auto route=H::evaluate(pokemon,candidate.history);
        if(route.status==H::Status::ConditionalOriginalBattleHistory)
            ++out.supported;
        else if(route.status==H::Status::ContradictedAttestedEvent)
            ++out.contradicted;
        else
            ++out.unknown;
    }
    if(out.supported)out.status=Status::OneSupportedOriginalHistory;
    else if(out.unknown)out.status=Status::UnresolvedOriginalHistories;
    else if(!input.allPossibleOriginalRoutesIndependentlyEnumerated)
        out.status=Status::UnknownOriginalRouteEnumeration;
    else if(out.contradicted==input.count)
        out.status=Status::AllAttestedOriginalHistoriesContradicted;
    else out.status=Status::UnresolvedOriginalHistories;
    // Even the exhaustive route-local contradiction is NOT a global
    // Legality::Invalid verdict. Provenance outside this search remains.
    return out;
}
} // namespace Legality::Gen1OriginalStatExpRouteAlternatives
