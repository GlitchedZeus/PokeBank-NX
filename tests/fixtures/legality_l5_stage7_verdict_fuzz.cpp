// L5 independent model of evidence constraints; not Legality::analyze execution.
#include "tools/audit_legality_l5_stage7_verdict_model.h"
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#ifdef NDEBUG
#error "L5 QA assertions must be enabled"
#endif
namespace G=Legality::Audit::L5FinalGate;
static std::uint64_t assertions=0;
#define CHECK(x) do { ++assertions; if(!(x)){std::cerr<<"L5 Stage7 failure "<<__LINE__<<" "<<#x<<'\n';std::abort();} }while(false)
static std::uint32_t next(std::uint32_t& s){return s=s*1664525u+1013904223u;}
static G::Cell complete(){ G::Cell c{};
    c.nativeAnalyzed=c.originalSaveAuthenticated=c.fullOriginalHistoryAuthenticated=true;
    c.executablePKHeXCompared=c.sameNativeFormat=c.sameGameAndMethod=true;
    c.fullCandidateUniverseEnumerated=c.allAlternateSourcesExcluded=true;
    c.allRuntimeComparisonsAgree=true;return c;
}
static void directed(){
    CHECK(G::grade(G::Cell{})==G::Clearance::Research);
    auto c=complete();CHECK(G::grade(c)==G::Clearance::CompleteRetailParity);
    for(unsigned b=0;b<9;++b){
        c=complete();switch(b){
        case 0:c.nativeAnalyzed=false;break;
        case 1:c.originalSaveAuthenticated=false;break;
        case 2:c.fullOriginalHistoryAuthenticated=false;break;
        case 3:c.executablePKHeXCompared=false;break;
        case 4:c.sameNativeFormat=false;break;
        case 5:c.sameGameAndMethod=false;break;
        case 6:c.fullCandidateUniverseEnumerated=false;break;
        case 7:c.allAlternateSourcesExcluded=false;break;
        case 8:c.allRuntimeComparisonsAgree=false;break;
        }
        CHECK(G::grade(c)!=G::Clearance::CompleteRetailParity);
    }
    G::Site s{};s.domain=G::Domain::Encounter;
    s.exactNativeFormatConfirmed=true;s.candidateContradiction=true;
    CHECK(G::scope(s)==G::Scope::CandidateOnly);
    s.originalSourceAuthenticated=s.allOriginalRoutesAuthenticated=
        s.allAlternateAcquisitionsExcluded=true;
    s.unexaminedRouteExists=false;
    CHECK(G::scope(s)==G::Scope::ExhaustivelyContradictedOriginal);
    s.consistentAlternativeFound=true;
    CHECK(G::scope(s)==G::Scope::CandidateOnly);
    s=G::Site{};s.domain=G::Domain::NativeStored;s.exactNativeFormatConfirmed=true;
    s.candidateContradiction=true;s.directStoredInvariantProven=true;
    CHECK(G::scope(s)==G::Scope::StoredInvariantNeedsRuntime);
    G::Dashboard d{};
    for(auto& cell:d.cells)cell=complete();
    d.anyOriginalPairedRetailHistory=d.runtimeFullAnalyzerExecuted=true;
    CHECK(G::summarize(d).releaseApproved);
    for(unsigned k=0;k<336;++k){
        d.cells[k].originalSaveAuthenticated=false;
        CHECK(!G::summarize(d).releaseApproved);
        d.cells[k].originalSaveAuthenticated=true;
    }
    d.emitted[0]=s;
    CHECK(!G::summarize(d).releaseApproved);
    d.emitted[0]=G::Site{};
    d.anyOriginalPairedRetailHistory=false;
    CHECK(!G::summarize(d).releaseApproved);
}
static void sweep(){
    std::uint32_t state=0x7e57a3e1u;
    for(std::uint32_t i=0;i<180000u;++i){
        G::Cell c=complete();const auto b=next(state);
        switch(b%12u){
        case 0:c.nativeApplicable=false;break;
        case 1:c.nativeAnalyzed=false;break;
        case 2:c.originalSaveAuthenticated=false;break;
        case 3:c.fullOriginalHistoryAuthenticated=false;break;
        case 4:c.executablePKHeXCompared=false;break;
        case 5:c.sameNativeFormat=false;break;
        case 6:c.sameGameAndMethod=false;break;
        case 7:c.fullCandidateUniverseEnumerated=false;break;
        case 8:c.allAlternateSourcesExcluded=false;break;
        case 9:c.allRuntimeComparisonsAgree=false;break;
        case 10:c.nativeApplicable=false;c.nativeAnalyzed=true;break;
        case 11:break;
        }
        const auto grade=G::grade(c);
        CHECK(grade==G::Clearance::CompleteRetailParity || !c.nativeApplicable ||
              !c.nativeAnalyzed || !c.originalSaveAuthenticated ||
              !c.fullOriginalHistoryAuthenticated || !c.executablePKHeXCompared ||
              !c.sameNativeFormat || !c.sameGameAndMethod ||
              !c.fullCandidateUniverseEnumerated || !c.allAlternateSourcesExcluded ||
              !c.allRuntimeComparisonsAgree);
        CHECK(c.nativeApplicable || grade==G::Clearance::NotApplicable);
        CHECK(!c.nativeAnalyzed || grade!=G::Clearance::Research || !c.nativeApplicable);
        G::Site x{};x.domain=(b&1u)?G::Domain::Encounter:G::Domain::Event;
        x.exactNativeFormatConfirmed=true;x.candidateContradiction=true;
        x.originalSourceAuthenticated=(b&2u)!=0;
        x.allOriginalRoutesAuthenticated=(b&4u)!=0;
        x.allAlternateAcquisitionsExcluded=(b&8u)!=0;
        x.unexaminedRouteExists=(b&16u)!=0;
        x.consistentAlternativeFound=(b&32u)!=0;
        CHECK(G::scope(x)!=G::Scope::StoredInvariantNeedsRuntime);
        auto alternative=x;alternative.consistentAlternativeFound=true;
        CHECK(G::scope(alternative)!=G::Scope::ExhaustivelyContradictedOriginal);
        alternative=x;alternative.unexaminedRouteExists=true;
        CHECK(G::scope(alternative)!=G::Scope::ExhaustivelyContradictedOriginal);
        alternative=x;alternative.originalSourceAuthenticated=false;
        CHECK(G::scope(alternative)!=G::Scope::ExhaustivelyContradictedOriginal);
        G::Dashboard d{};
        for(std::size_t j=0;j<d.cells.size();++j)d.cells[j]=complete();
        d.cells[i%336]=c;
        d.anyOriginalPairedRetailHistory=d.runtimeFullAnalyzerExecuted=true;
        auto out=G::summarize(d);
        CHECK(out.nativeApplicable+out.notApplicable==336);
        CHECK(out.fullyCertified<=out.nativeApplicable);
        CHECK(!d.cells[i%336].nativeApplicable || grade!=G::Clearance::Research ||
              out.fullyCertified<out.nativeApplicable);
        d.anyOriginalPairedRetailHistory=false;
        CHECK(!G::summarize(d).releaseApproved);
    }
}
int main(){directed();sweep();
  std::cout<<"L5 Stage7 evidence lattice PASS "<<assertions
           <<" assertions (independent QA only, release blocked)\n";
}
