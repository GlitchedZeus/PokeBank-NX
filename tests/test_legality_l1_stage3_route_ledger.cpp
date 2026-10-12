#include "Legality/Gen3OriginalCaptureRouteLedger.h"
#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
namespace L=Legality::Gen3OriginalCaptureRouteLedger;
using L::Evidence;
using L::Outcome;

int main() {
    std::uint64_t checks=0;
    // 3^8 exhausts the complete 8-route evidence alphabet, with every
    // alternate acquisition treated as an independent physical candidate.
    for(unsigned encoded=0;encoded<6561;++encoded) {
        std::array<L::Receipt,8> routes{};
        unsigned n=encoded;
        bool match=false,missing=false;
        for(unsigned i=0;i<8;++i) {
            const auto ev=static_cast<Evidence>(n%3u);
            n/=3;
            routes[i]={1234,5678,i+1,ev,true};
            match|=ev==Evidence::SourceStageCompatible;
            missing|=ev==Evidence::Missing;
        }
        const auto expected=match?Outcome::OneConditionalCaptureRoute:
           missing?Outcome::UnknownAlternateAcquisition:
                   Outcome::OnlyProvidedRoutesContradicted;
        const auto a=L::reconcile(1234,true,routes);
        assert(a.outcome==expected);
        assert(a.compatible+a.contradicted+a.unresolved==8);
        assert(!a.globalInvalidSupported && !a.globallyLegalSupported);
        checks+=3;
        std::reverse(routes.begin(),routes.end());
        const auto b=L::reconcile(1234,true,routes);
        assert(a.outcome==b.outcome && a.compatible==b.compatible &&
               a.contradicted==b.contradicted && a.unresolved==b.unresolved);
        ++checks;
        // Unwitnessed source/save identity contaminates ONLY that candidate.
        routes[0].originalIdentityIndependentlyAttested=false;
        const auto c=L::reconcile(1234,true,routes);
        assert(c.unresolved>0 && !c.globalInvalidSupported);
        ++checks;
        routes[0].originalIdentityIndependentlyAttested=true;
        routes[0].originalSave=0;
        const auto d=L::reconcile(1234,true,routes);
        assert(d.unresolved>0 && !d.globallyLegalSupported);
        ++checks;
        // Invalid evidence enum is never a false positive or contradiction.
        routes[0].originalSave=5678;
        routes[0].evidence=static_cast<Evidence>(255);
        const auto e=L::reconcile(1234,true,routes);
        assert(e.unresolved>0 && !e.globalInvalidSupported);
        ++checks;
        routes[0].originalRoute=routes[1].originalRoute;
        assert(L::reconcile(1234,true,routes).outcome==
               Outcome::UnknownDuplicateOrForeignRoute);
        ++checks;
    }
    // Complete capacity boundary: 32 accepted, 33 refused.
    std::array<L::Receipt,33> capacity{};
    for(unsigned i=0;i<capacity.size();++i)
        capacity[i]={1234,5678,i+1,Evidence::CandidateContradicted,true};
    assert(L::reconcile(1234,true,
        std::span<const L::Receipt>(capacity.data(),32)).outcome==
        Outcome::OnlyProvidedRoutesContradicted);
    assert(L::reconcile(1234,true,capacity).outcome==Outcome::UnknownCapacity);
    checks+=2;
    assert(L::reconcile(0,true,capacity).outcome==Outcome::UnknownRoot);
    assert(L::reconcile(1234,false,capacity).outcome==Outcome::UnknownRoot);
    assert(L::reconcile(1234,true,std::span<const L::Receipt>()).outcome==
           Outcome::UnknownAlternateAcquisition);
    checks+=3;
    for(unsigned i=0;i<32;++i) {
        auto subset=capacity;
        subset[i].specimenRoot=987;
        assert(L::reconcile(1234,true,
            std::span<const L::Receipt>(subset.data(),32)).outcome==
            Outcome::UnknownDuplicateOrForeignRoute);
        ++checks;
    }
    assert(checks==6561*8ull+37ull);
    std::cout<<"L1 Stage3 GBA captured-route provenance: "<<checks
             <<" PASS (never global Invalid)\n";
}
