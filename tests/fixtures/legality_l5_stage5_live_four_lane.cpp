// L5 Stage 5: compile ACTUAL, separately pinned L1/L2/L3/L4 headers.
// Candidate-local evidence only. No original retail records or global Invalid.
#include "Legality/Gen4HgssOriginalTreeStage.h"
#include "Legality/Gen4EggIVHistoricalCandidateSet.h"
#include "Legality/Gen3GbaJointMethodHistoryCore.h"
#include "Legality/Gen4EventClockReceiptEvidence.h"
#include "tools/audit_legality_l5_stage5_candidate_ledger.h"
#include <array>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#ifdef NDEBUG
#error "L5 crosslane tests require assertions"
#endif
namespace L1=Legality::Gen4HgssOriginalTreeStage;
namespace L2=Legality::Gen4EggIVHistoricalCandidateSet;
namespace IV=Legality::Gen4EggIVInheritanceStage;
namespace L3=Legality::Gen3GbaJointMethodHistoryCore;
namespace L4=Legality::Gen4EventClockReceiptEvidence;
namespace Join=Legality::Audit::L5CandidateLedger;
static unsigned long long checks=0;
#define CHECK(x) do { ++checks; if(!(x)){std::cerr<<"Stage5 native line "<<__LINE__<<": "<<#x<<'\n';std::abort();}}while(false)
void treeHistory(){
    for(unsigned n=1;n<=45;++n)for(unsigned tid=0;tid<100;++tid)
    for(unsigned id=0;id<n;++id){
        L1::Candidate c{};
        c.exactOriginalGame="heartgold_nds";c.originalFullTrainerId=tid;
        c.originalDesignatedTreeCount=static_cast<uint8_t>(n);
        c.originalTreeId=static_cast<uint8_t>(id);
        CHECK(L1::evaluate(c).status==L1::Status::UnknownOriginalTrainer);
        c.originalSaveIndependentlyAttested=c.fullTrainerIdIndependentlyAttested=true;
        c.originalTreeListIndependentlyAttested=c.physicalTreeIdIndependentlyAttested=true;
        CHECK(L1::evaluate(c).status==L1::Status::UnknownObservedGroup);
        c.originalGroupIndependentlyAttested=true;c.witnessedGroup=L1::Group::A;
        const auto r=L1::evaluate(c);
        CHECK(r.status==L1::Status::ConditionalOriginalTreeGroup ||
              r.status==L1::Status::CandidateTreeGroupContradicted ||
              r.status==L1::Status::CandidateTreeEmpty);
        c.originalSaveIndependentlyAttested=false;
        CHECK(L1::evaluate(c).status==L1::Status::UnknownOriginalTrainer);
    }
}
L2::Input egg(){
    L2::Input in{};in.game="platinum_nds";
    in.initialChild={1,2,3,4,5,6};
    in.physicalSlot0={10,11,12,13,14,15};
    in.physicalSlot1={20,21,22,23,24,25};
    const auto replay=IV::replay(in.game,0x8ac76123u,in.initialChild,
                                in.physicalSlot0,in.physicalSlot1,0,0);
    in.observedChild=replay.childIVs;
    in.originalStageIdentity=17;
    in.originalPhysicalParentSnapshotsVerified=true;
    in.originalPhysicalSlotOrderVerified=true;
    in.originalInitialChildIVsVerified=true;
    in.originalUnhatchedChildIVsVerified=true;
    in.originalStageIdentityIndependentlyVerified=true;
    in.count=1;auto& h=in.histories[0];
    h.routeIdentity=77;h.stageIdentity=17;
    h.independentSourceAttestation=true;
    h.historicalHeldItemsKnown=true;
    h.finalRNGStateKnown=true;
    h.finalRNGState=replay.finalSeed;
    in.allIVStageHypothesesIndependentlyEnumerated=true;
    return in;
}
void eggHistory(){
    auto in=egg();
    CHECK(L2::evaluate(in).status==L2::Status::ConditionalHistoryMatched);
    in.originalPhysicalSlotOrderVerified=false;
    CHECK(L2::evaluate(in).status==L2::Status::UnknownOriginalParentStage);
    in=egg();in.histories[0].independentSourceAttestation=false;
    CHECK(L2::evaluate(in).status==L2::Status::UnresolvedCandidateHistory);
    in=egg();in.count=17;
    CHECK(L2::evaluate(in).status==L2::Status::CapacityExceeded);
    in=egg();in.histories[0].finalRNGState^=0x1111u;
    CHECK(L2::evaluate(in).status==L2::Status::AllEnumeratedStageHypothesesContradicted);
    in.count=2;in.histories[1]=in.histories[0];
    CHECK(L2::evaluate(in).status==L2::Status::MissingOrDuplicateRouteIdentity);
    in=egg();in.count=2;in.histories[1]=in.histories[0];
    in.histories[1].routeIdentity=78;in.histories[1].finalRNGState^=0x1111u;
    CHECK(L2::evaluate(in).status==L2::Status::ConditionalHistoryMatched);
    for(uint32_t seed=1;seed<=20000;++seed){
        in=egg();in.histories[0].finalRNGState^=seed;
        const auto r=L2::evaluate(in);
        CHECK(r.status==L2::Status::AllEnumeratedStageHypothesesContradicted ||
              r.status==L2::Status::ConditionalHistoryMatched);
        in.allIVStageHypothesesIndependentlyEnumerated=false;
        CHECK(L2::evaluate(in).status!=L2::Status::AllEnumeratedStageHypothesesContradicted);
    }
}
L3::History path(){
    L3::History h{};h.candidateId=117;h.game=L3::Game::Ruby;h.species=1;
    h.initialLevel=5;h.observedLevel=6;
    h.initialMoves={33,0,0,0};h.observedMoves=h.initialMoves;
    h.initialSnapshotVerified=true;
    h.samePokemonAndGameVerified=true;
    h.allPlayerActionsVerified=true;
    return h;
}
void moveHistory(){
    std::array<L3::Offer,2> offers{{{5,33},{10,45}}};
    std::array<uint16_t,58> machines{};machines.fill(99);machines[0]=46;
    L3::Catalog c{};c.game=L3::Game::Ruby;c.species=1;
    c.offers=offers;c.machineMoves=machines;c.machineMask=1;
    c.originalSourceVerified=c.fullLevelUpSourceVerified=true;
    c.fullMachineSourceVerified=c.originalRelearnerFunctionVerified=true;
    auto h=path();
    CHECK(L3::evaluate(c,h).status==L3::Status::CandidateConsistent);
    h.observedMoves={34,0,0,0};
    CHECK(L3::evaluate(c,h).status==L3::Status::CandidateContradicted);
    L3::Alternatives a{};a.count=2;
    a.paths[0]=h;a.paths[1]=path();
    a.allActualHistoriesAuthenticatedAndEnumerated=true;
    CHECK(L3::evaluate(c,a)==L3::Status::CandidateConsistent);
    a.paths[1].initialSnapshotVerified=false;
    CHECK(L3::evaluate(c,a)==L3::Status::Unknown);
    a.count=1;a.allActualHistoriesAuthenticatedAndEnumerated=false;
    CHECK(L3::evaluate(c,a)==L3::Status::Unknown);
    a.allActualHistoriesAuthenticatedAndEnumerated=true;
    CHECK(L3::evaluate(c,a)==L3::Status::CandidateContradicted);
    for(unsigned n=0;n<20000;++n){
        h=path();h.observedMoves={static_cast<uint16_t>(1+(n%354)),0,0,0};
        const auto s=L3::evaluate(c,h).status;
        CHECK(s==L3::Status::CandidateConsistent||s==L3::Status::CandidateContradicted);
        h.allPlayerActionsVerified=false;
        CHECK(L3::evaluate(c,h).status==L3::Status::Unknown);
    }
}
L4::Evidence clockSource(){
    L4::Evidence e{};e.releaseWindowIndependentlyVerified=true;
    e.first={2009,1,1};e.last={2009,12,31};
    e.originalReceiptField=L4::Field::Met;
    e.originalFieldUnchangedIndependentlyVerified=true;
    e.clockAccurateAtOriginalReceiptIndependentlyVerified=true;
    e.noLaterDateRewriteIndependentlyVerified=true;
    e.eggOrHatchHistoryComplete=true;return e;
}
void eventHistory(){
    const L4::Date eggDate{2008,1,1},metDate{2010,1,1};
    auto e=clockSource();
    CHECK(L4::evaluate(true,eggDate,metDate,e).status==L4::Status::CandidateOutsideWindow);
    e.clockAccurateAtOriginalReceiptIndependentlyVerified=false;
    CHECK(L4::evaluate(true,eggDate,metDate,e).status==L4::Status::UnknownClockOrTransform);
    e=clockSource();e.dateFieldHasEggOrHatchAmbiguity=true;
    e.eggOrHatchHistoryComplete=false;
    CHECK(L4::evaluate(true,eggDate,metDate,e).status==L4::Status::UnknownClockOrTransform);
    for(unsigned year=2000;year<2200;++year)for(unsigned month=1;month<=12;++month){
        auto q=clockSource();
        const L4::Date observed{static_cast<uint16_t>(year),static_cast<uint8_t>(month),1};
        const auto s=L4::evaluate(true,eggDate,observed,q).status;
        CHECK(s==L4::Status::CandidateOutsideWindow||s==L4::Status::CandidateWithinWindow);
        q.noLaterDateRewriteIndependentlyVerified=false;
        CHECK(L4::evaluate(true,eggDate,observed,q).status==L4::Status::UnknownClockOrTransform);
    }
}
void combined(){
    Join::Input i{};i.pokemon=99;i.count=4;
    for(unsigned n=0;n<4;++n){
        auto& p=i.packets[n];p.pokemon=99;p.originalRoute=n+1;
        p.originalSource=n+100;
        p.originalReceiptAttested=p.samePokemonHistoryAttested=p.completeStageHistory=true;
        p.required=Join::Encounter|Join::Breeding|Join::Moves|Join::Event;
        p.lane.fill(Join::Claim::Compatible);p.lane[n]=Join::Claim::Contradicted;
    }
    i.allOriginalRoutesAttested=i.allAlternativeAcquisitionRoutesExcluded=
        i.boundedEnumerationProvenComplete=true;
    CHECK(Join::evaluate(i).status==Join::Status::AllEnumeratedCandidateRoutesContradicted);
    for(unsigned lane=0;lane<4;++lane){
        auto x=i;x.packets[lane].lane[lane]=Join::Claim::Unsupported;
        CHECK(Join::evaluate(x).status==Join::Status::Unresolved);
        x=i;x.packets[lane].lane[lane]=Join::Claim::Compatible;
        CHECK(Join::evaluate(x).status==Join::Status::AuthenticatedCandidateConsistent);
    }
    auto x=i;x.count=5;x.packets[4]=x.packets[0];
    x.packets[4].lane[0]=Join::Claim::Compatible;
    CHECK(Join::evaluate(x).duplicateConflicts==1);
    CHECK(Join::evaluate(x).status==Join::Status::Unresolved);
    x=i;x.count=5;x.packets[4]=x.packets[0];
    x.packets[4].originalReceiptAttested=false;
    CHECK(Join::evaluate(x).status==Join::Status::AllEnumeratedCandidateRoutesContradicted);
    x=i;x.count=5;x.packets[4]=x.packets[0];
    x.packets[4].originalRoute=8;x.packets[4].originalReceiptAttested=false;
    CHECK(Join::evaluate(x).status==Join::Status::Unresolved);
}
int main(){
    treeHistory();eggHistory();moveHistory();eventHistory();combined();
    std::cout<<"L5 Stage5 four ORIGINAL lane helper integration PASS "<<checks
             <<" assertions (candidate-scoped only)\n";
}
