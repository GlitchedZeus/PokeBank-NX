// Stage 7 SYNTHETIC authenticated witness fixtures. They are NOT authentic
// breeder/event distribution records, source TM mappings or physical PKs.
#include "Legality/Gen14AuthenticatedMoveHistory.h"
#include <cassert>
#include <cstdint>
#include <iostream>
namespace H=Legality::Gen14AuthenticatedMoveHistory;
using G=H::Game;using A=H::Action;using M=H::Method;using S=H::Status;
constexpr H::History base(G game=G::Ruby,std::uint16_t species=1){
    H::History h{};h.candidate=7001;h.individual=0x1234567;
    h.originalGame=game;h.exhaustiveIntermediateChronologyVerified=h.sourceSpecificChronologyPassed=true;
    h.initial.id=100;h.initial.candidate=h.candidate;h.initial.individual=h.individual;
    h.initial.game=game;h.initial.species=species;h.initial.level=5;
    h.initial.moves={33,45,0,0};
    h.initial.nativeRecordVerified=h.initial.sourceSaveVerified=true;
    h.observed=h.initial;return h;
}
constexpr void append(H::History& h,A action,M method,std::uint16_t move=0,
                      std::uint8_t slot=0,std::uint8_t level=5){
    auto& s=h.steps[h.count];s.before=h.observed;s.after=s.before;
    s.after.id=s.before.id+1;s.after.level=level;
    s.action=action;s.method=method;s.move=move;s.slot=slot;
    s.originalActionVerified=s.methodSourceVerified=s.sameIndividualContinuityVerified=true;
    if(action==A::Learn)s.after.moves[slot]=move;
    h.observed=s.after;++h.count;
}
constexpr H::History validEgg(){
    auto h=base();append(h,A::Learn,M::InheritedEgg,73,2);
    auto& s=h.steps[0];s.crossLaneEvidenceId=778;
    h.breedingCount=1;auto& e=h.breeding[0];
    e.evidenceId=778;e.eggSnapshotId=h.initial.id;e.motherSnapshotId=901;
    e.fatherSnapshotId=902;e.candidate=h.candidate;e.individual=h.individual;
    e.originalGame=h.originalGame;e.species=h.initial.species;e.form=h.initial.form;
    e.move=73;e.acquisitionLevel=5;
    e.originalEggVerified=e.parentsIndependentlyVerified=
    e.inheritanceMethodSourceVerified=e.sameEggIndividualVerified=true;
    return h;
}
constexpr H::History validEvent(){
    auto h=base(G::Emerald);append(h,A::Learn,M::DistributedEvent,273,2);
    h.steps[0].crossLaneEvidenceId=779;
    h.eventCount=1;auto& e=h.events[0];
    e.evidenceId=779;e.distributionId=5002;e.templateId=6003;
    e.originalSnapshotId=h.initial.id;e.candidate=h.candidate;
    e.individual=h.individual;e.originalGame=h.originalGame;
    e.species=h.initial.species;e.form=h.initial.form;e.move=273;
    e.earliestLevel=5;e.latestLevel=5;
    e.distributionIndependentlyVerified=e.templateIndependentlyVerified=
    e.templateMethodMoveVerified=e.originalEncounterVerified=true;
    return h;
}
static_assert(H::evaluate(validEgg()).status==S::CandidateConsistent);
static_assert(H::evaluate(validEvent()).status==S::CandidateConsistent);
static_assert([]{auto h=validEgg();h.breeding[0].parentsIndependentlyVerified=false;
    return H::evaluate(h).status==S::Unknown;}());
static_assert([]{auto h=validEvent();h.events[0].templateId=0;
    return H::evaluate(h).status==S::Unknown;}());
int main(){
    std::uint64_t checks=0;
    for(unsigned i=0;i<12000;++i){
        auto egg=validEgg();assert(H::evaluate(egg).status==S::CandidateConsistent);++checks;
        auto event=validEvent();assert(H::evaluate(event).status==S::CandidateConsistent);++checks;
        auto x=egg;x.breeding[0].species=4;
        assert(H::evaluate(x).status==S::Unknown);++checks;
        x=egg;x.breeding[0].originalGame=G::LeafGreen;
        assert(H::evaluate(x).status==S::Unknown);++checks;
        x=egg;x.breeding[0].motherSnapshotId=x.breeding[0].fatherSnapshotId;
        assert(H::evaluate(x).status==S::Unknown);++checks;
        x=egg;x.breeding[0].eggSnapshotId+=4;
        assert(H::evaluate(x).status==S::Unknown);++checks;
        x=egg;x.breeding[0].evidenceId+=1;
        assert(H::evaluate(x).status==S::Unknown);++checks;
        x=egg;x.steps[0].after.moves[2]=22;x.observed=x.steps[0].after;
        assert(H::evaluate(x).status==S::CandidateContradicted);++checks;
        auto e=event;e.events[0].originalSnapshotId+=1;
        assert(H::evaluate(e).status==S::Unknown);++checks;
        e=event;e.events[0].templateMethodMoveVerified=false;
        assert(H::evaluate(e).status==S::Unknown);++checks;
        e=event;e.events[0].latestLevel=4;
        assert(H::evaluate(e).status==S::Unknown);++checks;
        e=event;e.events[0].originalGame=G::FireRed;
        assert(H::evaluate(e).status==S::Unknown);++checks;
        e=event;e.events[0].individual++;
        assert(H::evaluate(e).status==S::Unknown);++checks;
        e=event;e.events[0].templateId=0;
        assert(H::evaluate(e).status==S::Unknown);++checks;
        e=event;e.events[0].form=1;
        assert(H::evaluate(e).status==S::Unknown);++checks;
        e=event;e.events[0].move=99;
        assert(H::evaluate(e).status==S::Unknown);++checks;
        e=event;e.events[0].distributionIndependentlyVerified=false;
        assert(H::evaluate(e).status==S::Unknown);++checks;
        e=event;e.eventCount=0;
        assert(H::evaluate(e).status==S::Unknown);++checks;
        e=event;e.eventCount=2;e.events[1]=e.events[0];
        assert(H::evaluate(e).status==S::Unknown);++checks;
        x=egg;x.breedingCount=2;x.breeding[1]=x.breeding[0];
        assert(H::evaluate(x).status==S::Unknown);++checks;
        x=egg;x.steps[0].methodSourceVerified=false;
        assert(H::evaluate(x).status==S::Unknown);++checks;
        x=egg;x.initial.sourceSaveVerified=false;
        assert(H::evaluate(x).status==S::Unknown);++checks;
        x=egg;x.steps[0].before.candidate=7020;
        assert(H::evaluate(x).status==S::Unknown);++checks;
        x=egg;x.steps[0].after.individual++;
        x.observed=x.steps[0].after;
        assert(H::evaluate(x).status==S::Unknown);++checks;
        assert(!H::evaluate(egg).globalInvalidAuthorized);++checks;
        assert(!H::evaluate(event).globalInvalidAuthorized);++checks;
    }
    std::cout<<"L3 authenticated L2/L4 source-boundary synthetic assertions: "<<checks<<" PASS\n";
}
