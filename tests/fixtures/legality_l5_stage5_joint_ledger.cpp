// L5 independently authored, source-agnostic adversarial route ledger.
// Not a retail-original or live production Legality::analyze verification.
#include "tools/audit_legality_l5_stage5_candidate_ledger.h"
#include <array>
#include <cassert>
#include <cstdlib>
#include <utility>
#include <cstdint>
#include <iostream>
#ifdef NDEBUG
#error "L5 QA requires live assertions"
#endif
namespace QA=Legality::Audit::L5CandidateLedger;
static unsigned long long checks=0;
#define CHECK(x) do { ++checks; if(!(x)) {std::cerr<<"FAIL line "<<__LINE__<<" "<<#x<<'\n';std::abort();} }while(false)
constexpr QA::Packet route(std::uint64_t p,std::uint64_t r,std::uint64_t s){
    QA::Packet q{};q.pokemon=p;q.originalRoute=r;q.originalSource=s;
    q.originalReceiptAttested=q.samePokemonHistoryAttested=q.completeStageHistory=true;
    q.required=QA::Encounter|QA::Breeding|QA::Moves|QA::Event;
    q.lane.fill(QA::Claim::Compatible);return q;
}
constexpr QA::Input baseline(){
    QA::Input x{};x.pokemon=33;x.count=2;
    x.packets[0]=route(33,11,100);
    x.packets[1]=route(33,12,100);
    x.packets[0].lane[0]=QA::Claim::Contradicted;
    x.packets[1].lane[2]=QA::Claim::Contradicted;
    x.allOriginalRoutesAttested=x.allAlternativeAcquisitionRoutesExcluded=
        x.boundedEnumerationProvenComplete=true;
    return x;
}
void directed(){
    auto b=baseline();
    CHECK(QA::evaluate(b).status==QA::Status::AllEnumeratedCandidateRoutesContradicted);
    for(int gate=0;gate<3;++gate){
        auto q=b;
        if(gate==0)q.allOriginalRoutesAttested=false;
        if(gate==1)q.allAlternativeAcquisitionRoutesExcluded=false;
        if(gate==2)q.boundedEnumerationProvenComplete=false;
        CHECK(QA::evaluate(q).status==QA::Status::Unresolved);
    }
    for(int idx=0;idx<2;++idx){
        auto q=b;q.packets[idx].lane[1]=QA::Claim::Unsupported;
        CHECK(QA::evaluate(q).unresolved>0);
        CHECK(QA::evaluate(q).status==QA::Status::Unresolved);
    }
    {auto q=b;q.packets[1].lane[2]=QA::Claim::Compatible;
     CHECK(QA::evaluate(q).status==QA::Status::AuthenticatedCandidateConsistent);}
    {auto q=b;q.count=3;q.packets[2]=route(33,13,100);
     q.packets[2].originalReceiptAttested=false;
     CHECK(QA::evaluate(q).status==QA::Status::Unresolved);}
    {auto q=b;q.count=3;q.packets[2]=q.packets[0];
     CHECK(QA::evaluate(q).status==QA::Status::AllEnumeratedCandidateRoutesContradicted);}
    {auto q=b;q.count=3;q.packets[2]=q.packets[0];
     q.packets[2].lane[0]=QA::Claim::Compatible;
     CHECK(QA::evaluate(q).duplicateConflicts==1);
     CHECK(QA::evaluate(q).status==QA::Status::Unresolved);}
    {auto q=b;q.packets[1].originalSource=101;
     CHECK(QA::evaluate(q).status==QA::Status::AllEnumeratedCandidateRoutesContradicted);}
    for(int lane=0;lane<4;++lane){
        auto q=b;q.packets[0].required&=std::uint8_t(~(1u<<lane));
        CHECK(QA::evaluate(q).status==QA::Status::Unresolved);
    }
    {auto q=b;q.packets[0].pokemon=34;
     CHECK(QA::evaluate(q).status==QA::Status::Unresolved);}
    {auto q=b;q.packets[0].originalRoute=0;
     CHECK(QA::evaluate(q).status==QA::Status::Unresolved);}
    {auto q=b;q.packets[0].originalSource=0;
     CHECK(QA::evaluate(q).status==QA::Status::Unresolved);}
    {auto q=b;q.count=17;
     CHECK(QA::evaluate(q).status==QA::Status::Unresolved);}
    {auto q=b;q.count=0;
     CHECK(QA::evaluate(q).status==QA::Status::Unresolved);}
    {auto q=b;q.pokemon=0;
     CHECK(QA::evaluate(q).status==QA::Status::Unresolved);}
}
uint32_t next(uint32_t& seed){return seed=seed*1664525u+1013904223u;}
void deterministic(){
    uint32_t seed=0xc0ffee45;
    for(int i=0;i<100000;++i){
        auto x=baseline();
        const uint32_t n=next(seed);
        const int mode=int(n%12);
        if(mode==0)x.packets[0].lane[(n>>8)%4]=QA::Claim::Absent;
        if(mode==1)x.packets[1].lane[(n>>8)%4]=QA::Claim::Unsupported;
        if(mode==2)x.packets[1].lane[(n>>8)%4]=QA::Claim::Compatible;
        if(mode==3)x.packets[0].originalReceiptAttested=false;
        if(mode==4)x.allOriginalRoutesAttested=false;
        if(mode==5)x.allAlternativeAcquisitionRoutesExcluded=false;
        if(mode==6)x.boundedEnumerationProvenComplete=false;
        if(mode==7){x.count=3;x.packets[2]=route(33,13,100);x.packets[2].originalReceiptAttested=false;}
        if(mode==8){x.count=3;x.packets[2]=x.packets[0];x.packets[2].lane[3]=QA::Claim::Contradicted;}
        if(mode==9){x.count=3;x.packets[2]=x.packets[0];}
        if(mode==10){x.packets[0].originalSource=0;}
        if(mode==11){x.packets[0].required=0;}
        const auto a=QA::evaluate(x);
        CHECK(!a.globalInvalid);
        CHECK(a.authenticatedDistinctRoutes<=x.count);
        CHECK(a.compatible+a.contradicted+a.unresolved<=x.count);
        // Packet order cannot change a candidate-set verdict or route counts.
        auto y=x;std::swap(y.packets[0],y.packets[x.count-1]);
        const auto b=QA::evaluate(y);
        CHECK(a.status==b.status);
        CHECK(a.compatible==b.compatible);
        CHECK(a.contradicted==b.contradicted);
        CHECK(a.unresolved==b.unresolved);
        CHECK(a.duplicateConflicts==b.duplicateConflicts);
        // Strengthening an alternative by adding a genuinely consistent route
        // makes global candidate-set rejection impossible.
        if(x.count<16){
            auto z=x;z.packets[z.count++]=route(33,1000u+std::uint64_t(i),100);
            CHECK(QA::evaluate(z).status==QA::Status::AuthenticatedCandidateConsistent);
        }
    }
}
int main(){directed();deterministic();std::cout<<"L5 Stage5 candidate evidence ledger PASS "<<checks<<" assertions (not global legality)\n";}
