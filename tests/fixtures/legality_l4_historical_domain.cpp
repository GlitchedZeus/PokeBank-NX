#include "Legality/Gen14HistoricalEventPathEvidence.h"
#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
namespace P = Legality::Gen14HistoricalEventPathEvidence;
int main() {
    P::Snapshot original{};
    original.known = P::kAllFields;
    for (size_t i=0;i<P::kFields;++i) original.value[i]=i+301;
    P::Source source{};
    source.originalGeneration=P::Generation::IV;
    source.original=original;
    source.exactOriginalSourceIndependentlyAttested=true;
    source.originalFieldsIndependentlyExtracted=true;
    source.exhaustiveHistoricalChainVerified=true;
    using S=P::Status;
    unsigned cases=0;
    for (unsigned raw=0;raw<256;++raw) {
        const auto generation=static_cast<P::Generation>(raw);
        auto sourceVariant=source;
        sourceVariant.originalGeneration=generation;
        const auto origin=P::evaluate(true,generation,original,sourceVariant);
        if(raw>=1&&raw<=4)
            assert(origin.status==S::ConditionalCandidate);
        else
            assert(origin.status==S::UnknownOriginalSource);
        const auto destination=P::evaluate(true,generation,original,source);
        assert(destination.status == (raw==4 ? S::ConditionalCandidate:S::UnknownRoute));
        ++cases;
        for(unsigned rawAction=0;rawAction<256;++rawAction) {
            const auto action=static_cast<P::Action>(rawAction);
            P::Step step{action,P::Generation::IV,P::Generation::IV,true,0,{}};
            const auto result=P::evaluate(true,P::Generation::IV,original,source,std::array{step});
            if(rawAction>=10)
                assert(result.status==S::UnknownHistoricalTransformation);
            else if(rawAction==3||rawAction==4)
                assert(result.status==S::CandidateRouteContradicted);
            else if(rawAction==1||rawAction==2)
                assert(result.status==S::UnknownHistoricalTransformation);
            else
                assert(result.status==S::ConditionalCandidate);
            assert(!result.authenticated);
            assert(P::routePermitted(P::Generation::IV,P::Generation::IV,action)==
                   (rawAction<10 && rawAction!=3 && rawAction!=4));
            ++cases;
        }
    }
    for(unsigned raw=0;raw<256;++raw) {
        const auto generation=static_cast<P::Generation>(raw);
        P::Step step{P::Action::Trade,P::Generation::IV,generation,true,0,{}};
        const auto result=P::evaluate(true,P::Generation::IV,original,source,std::array{step});
        if(raw==4)assert(result.status==S::ConditionalCandidate);
        else if(raw>=1&&raw<=3)assert(result.status==S::CandidateRouteContradicted);
        else assert(result.status==S::UnknownHistoricalTransformation);
        ++cases;
    }
    // Cross-check all 4x4 supported generation endpoints against every
    // 8-bit action tag. Gen I has neither breeding nor held items; the
    // Time Capsule and Pal Park are the ONLY implemented inter-gen jumps.
    for(unsigned from=1;from<=4;++from)for(unsigned to=1;to<=4;++to)
    for(unsigned rawAction=0;rawAction<256;++rawAction) {
        const auto f=static_cast<P::Generation>(from);
        const auto t=static_cast<P::Generation>(to);
        const auto a=static_cast<P::Action>(rawAction);
        const bool crossing=(rawAction==3 && from==3 && to==4) ||
            (rawAction==4 && ((from==1 && to==2)||(from==2 && to==1)));
        const bool same=from==to && rawAction<10 && rawAction!=3 && rawAction!=4 &&
            !(from==1&&(rawAction==2 || rawAction==8)) &&
            !(from<=2 && rawAction==7);
        assert(P::routePermitted(f,t,a)==(crossing||same));
        ++cases;
    }
    // A malformed middle step must stop a candidate, rather than silently
    // treating the malformed label as a legitimate observed transfer.
    const std::array<P::Step,3> history{{
        {P::Action::Trade,P::Generation::IV,P::Generation::IV,true,0,{}},
        {static_cast<P::Action>(255),P::Generation::IV,P::Generation::IV,true,0,{}},
        {P::Action::Trade,P::Generation::IV,P::Generation::IV,true,0,{}}
    }};
    const auto broken=P::evaluate(true,P::Generation::IV,original,source,history);
    assert(broken.status==S::UnknownHistoricalTransformation);
    assert(broken.inspectedSteps==1);
    assert(broken.contradicted==0);
    assert(broken.unknown==P::kAllFields);
    assert(cases==70144);
    std::cout<<"L4 historical enum / lineage domain: "<<cases<<" classified inputs: PASS\n";
}
