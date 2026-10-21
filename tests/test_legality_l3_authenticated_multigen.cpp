// Stage 7 SYNTHETIC PK1-PK4 chronology fixtures, NOT parsed native save bytes.
#include "Legality/Gen14AuthenticatedMoveHistory.h"
#include <cassert>
#include <cstdint>
#include <iostream>
namespace H=Legality::Gen14AuthenticatedMoveHistory;
using G=H::Game;using A=H::Action;using M=H::Method;using S=H::Status;
constexpr H::History start(G game,std::uint16_t species=1){
    H::History h{};h.candidate=0x88;h.individual=0x1234;
    h.originalGame=game;h.exhaustiveIntermediateChronologyVerified=h.sourceSpecificChronologyPassed=true;
    h.initial.id=42;h.initial.individual=h.individual;h.initial.candidate=h.candidate;
    h.initial.game=game;h.initial.species=species;h.initial.level=5;
    h.initial.moves={33,45,0,0};h.initial.nativeRecordVerified=true;
    h.initial.sourceSaveVerified=true;h.observed=h.initial;return h;
}
constexpr H::Step& add(H::History& h,A action,M method=M::None,
                       std::uint16_t move=0,std::uint8_t slot=0){
    auto& s=h.steps[h.count];s.before=h.observed;s.after=s.before;
    s.after.id=s.before.id+1;s.action=action;s.method=method;
    s.move=move;s.slot=slot;
    s.originalActionVerified=s.methodSourceVerified=s.sameIndividualContinuityVerified=true;
    return s;
}
constexpr void done(H::History& h){h.observed=h.steps[h.count].after;++h.count;}
constexpr H::History timeCapsule(){
    auto h=start(G::Gold);auto& s=add(h,A::TimeCapsule);s.after.game=G::Red;
    s.visitOrTransferVerified=true;done(h);
    auto& t=add(h,A::TimeCapsule);t.after.game=G::Silver;
    t.visitOrTransferVerified=true;done(h);return h;
}
constexpr H::History palPark(){
    auto h=start(G::Ruby);
    {auto& s=add(h,A::Learn,M::TM,92,2);s.machineNumber=6;
       s.itemReceiptId=9001;s.after.moves[2]=92;done(h);}
    {auto& s=add(h,A::Learn,M::LevelUp,73,3);s.after.level=7;
       s.after.moves[3]=73;done(h);}
    {auto& s=add(h,A::Evolve);s.after.species=2;
       s.speciesFormTransitionSourceVerified=true;done(h);}
    {auto& s=add(h,A::Visit);s.after.game=G::Emerald;
       s.visitOrTransferVerified=true;done(h);}
    {auto& s=add(h,A::DeleteMove,M::None,0,1);
       s.after.moves={33,92,73,0};done(h);}
    {auto& s=add(h,A::PalPark);s.after.game=G::Platinum;
       s.visitOrTransferVerified=true;done(h);}
    {auto& s=add(h,A::Learn,M::Tutor,189,3);s.after.moves[3]=189;done(h);}
    return h;
}
static_assert(H::evaluate(timeCapsule()).status==S::CandidateConsistent);
static_assert(H::evaluate(palPark()).status==S::CandidateConsistent);
static_assert([] {auto p=palPark();p.steps[5].after.moves[0]=45;
   return H::evaluate(p).status==S::Unknown;}()); // broken chain is untrusted
int main(){
    std::uint64_t checks=0;
    for(unsigned i=0;i<12000;++i){
        auto p=palPark();assert(H::evaluate(p).status==S::CandidateConsistent);++checks;
        auto t=timeCapsule();assert(H::evaluate(t).status==S::CandidateConsistent);++checks;
        auto x=p;x.steps[2].after.species=4;
        // A changed ancestor without a matching next snapshot cannot be joined.
        assert(H::evaluate(x).status==S::Unknown);++checks;
        x=p;x.steps[3].before.id++;
        assert(H::evaluate(x).status==S::Unknown);++checks;
        x=p;x.steps[5].after.moves[0]=45;
        assert(H::evaluate(x).status==S::Unknown);++checks;
        x=p;x.steps[5].after.id=x.steps[0].before.id;
        assert(H::evaluate(x).status==S::Unknown);++checks;
        x=p;x.steps[0].machineNumber=0;
        assert(H::evaluate(x).status==S::Unknown);++checks;
        x=p;x.steps[0].itemReceiptId=0;
        assert(H::evaluate(x).status==S::Unknown);++checks;
        x=p;x.steps[0].after.moves[2]=237;
        assert(H::evaluate(x).status==S::Unknown);++checks;
        x=p;x.steps[1].after.level=4;
        assert(H::evaluate(x).status==S::Unknown);++checks;
        x=p;x.steps[1].before.nativeRecordVerified=false;
        assert(H::evaluate(x).status==S::Unknown);++checks;
        x=p;x.steps[5].after.game=G::Diamond;
        assert(H::evaluate(x).status==S::Unknown);++checks;
        x=p;x.steps[4].after.moves={33,92,0,0};
        assert(H::evaluate(x).status==S::Unknown);++checks;
        x=p;x.steps[5].visitOrTransferVerified=false;
        assert(H::evaluate(x).status==S::Unknown);++checks;
        x=p;x.steps[6].methodSourceVerified=false;
        assert(H::evaluate(x).status==S::Unknown);++checks;
        x=p;x.steps[4].slot=0;
        assert(H::evaluate(x).status==S::CandidateContradicted);++checks;
        x=p;x.steps[0].slot=3;
        assert(H::evaluate(x).status==S::CandidateContradicted);++checks;
        x=p;x.steps[0].after.moves[2]=93;
        x.steps[1].before.moves[2]=93;x.steps[1].after.moves[2]=93;
        // Still cannot splice into later unmodified immutable snapshots.
        assert(H::evaluate(x).status==S::Unknown);++checks;
        x=p;x.steps[5].after.level=99;
        assert(H::evaluate(x).status==S::Unknown);++checks;
        x=p;x.steps[0].originalActionVerified=false;
        assert(H::evaluate(x).status==S::Unknown);++checks;
        x=p;x.exhaustiveIntermediateChronologyVerified=false;
        assert(H::evaluate(x).status==S::Unknown);++checks;
        t=timeCapsule();t.steps[0].after.species=152;
        assert(H::evaluate(t).status==S::Unknown);++checks;
        t=timeCapsule();t.steps[0].after.game=G::Emerald;
        assert(H::evaluate(t).status==S::Unknown);++checks;
        auto hm=start(G::Ruby);auto& hs=add(hm,A::Learn,M::HM,15,2);
        hs.itemReceiptId=4;hs.machineNumber=1;hs.after.moves[2]=15;done(hm);
        auto& tp=add(hm,A::PalPark);tp.after.game=G::Pearl;
        tp.visitOrTransferVerified=true;done(hm);
        assert(H::evaluate(hm).status==S::CandidateContradicted);++checks;
        auto own=p;own.steps[6].after.moves[3]=188;own.observed=own.steps[6].after;
        assert(H::evaluate(own).status==S::CandidateContradicted);++checks;
        H::Alternatives alternatives{};alternatives.count=2;
        alternatives.paths[0]=hm;alternatives.paths[1]=p;
        assert(H::evaluate(alternatives).status==S::CandidateConsistent);++checks;
        alternatives.paths[1].steps[0].methodSourceVerified=false;
        assert(H::evaluate(alternatives).status==S::Unknown);++checks;
        alternatives.paths[1]=hm;alternatives.exhaustiveUniverseIndependentlyVerified=true;
        assert(H::evaluate(alternatives).status==S::CandidateContradicted);++checks;
        alternatives.paths[1].candidate++;
        assert(H::evaluate(alternatives).status==S::Unknown);++checks;
        assert(!H::evaluate(p).globalInvalidAuthorized);++checks;
    }
    std::cout<<"L3 authenticated PK1-PK4 multigen synthetic assertions: "<<checks<<" PASS\n";
}
