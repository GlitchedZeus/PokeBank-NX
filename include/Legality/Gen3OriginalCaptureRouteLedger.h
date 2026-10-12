#pragma once
// L1 Stage 3 original-capture candidate reconciliation core.
// Informational ONLY. Every provided capture route is a bounded subset of
// possible wild, Safari, egg, gift, event, trade and transfer histories;
// even all supplied contradictions can never justify global Invalid.
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
namespace Legality::Gen3OriginalCaptureRouteLedger {
inline constexpr std::size_t kCapacity=32;
enum class Evidence : uint8_t { Missing, CandidateContradicted, SourceStageCompatible };
enum class Outcome : uint8_t {
    UnknownRoot, UnknownCapacity, UnknownDuplicateOrForeignRoute,
    UnknownAlternateAcquisition, OnlyProvidedRoutesContradicted,
    OneConditionalCaptureRoute
};
struct Receipt {
    uint64_t specimenRoot=0, originalSave=0, originalRoute=0;
    Evidence evidence=Evidence::Missing;
    bool originalIdentityIndependentlyAttested=false;
};
struct Result {
    Outcome outcome=Outcome::UnknownRoot;
    uint16_t compatible=0,contradicted=0,unresolved=0;
    bool globalInvalidSupported=false,globallyLegalSupported=false;
};
constexpr Result reconcile(uint64_t root,bool rootIndependentlyAttested,
                           std::span<const Receipt> routes) noexcept {
    Result out{};
    if(!root || !rootIndependentlyAttested)return out;
    if(routes.size()>kCapacity) {
        out.outcome=Outcome::UnknownCapacity;
        return out;
    }
    if(routes.empty()) {
        out.outcome=Outcome::UnknownAlternateAcquisition;
        return out;
    }
    for(std::size_t i=0;i<routes.size();++i) {
        const auto& route=routes[i];
        if(!route.specimenRoot || route.specimenRoot!=root ||
           !route.originalRoute) {
            out.outcome=Outcome::UnknownDuplicateOrForeignRoute;
            return out;
        }
        for(std::size_t j=0;j<i;++j) {
            if(routes[j].originalRoute==route.originalRoute) {
                out.outcome=Outcome::UnknownDuplicateOrForeignRoute;
                return out;
            }
        }
    }
    for(const auto& route:routes) {
        // Missing independently attested original save/capture provenance
        // must never count as a conditional candidate or negative proof.
        if(!route.originalSave || !route.originalIdentityIndependentlyAttested) {
            ++out.unresolved;
            continue;
        }
        switch(route.evidence) {
            case Evidence::SourceStageCompatible: ++out.compatible;break;
            case Evidence::CandidateContradicted: ++out.contradicted;break;
            case Evidence::Missing: ++out.unresolved;break;
            default: ++out.unresolved;break;
        }
    }
    if(out.compatible)out.outcome=Outcome::OneConditionalCaptureRoute;
    else if(out.unresolved)out.outcome=Outcome::UnknownAlternateAcquisition;
    else out.outcome=Outcome::OnlyProvidedRoutesContradicted;
    return out;
}
static_assert([]{
    constexpr std::array<Receipt,2> routes{{
        {7,12,1,Evidence::CandidateContradicted,true},
        {7,12,2,Evidence::SourceStageCompatible,true}
    }};
    const auto out=reconcile(7,true,routes);
    return out.outcome==Outcome::OneConditionalCaptureRoute &&
           out.compatible==1 && out.contradicted==1 &&
           !out.globalInvalidSupported;
}());
} // namespace Legality::Gen3OriginalCaptureRouteLedger
