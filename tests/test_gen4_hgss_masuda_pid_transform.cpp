#include "Legality/Gen4HGSSMasudaPidTransform.h"
#include <cassert>
#include <iostream>
namespace H=Legality::Gen4HGSSMasudaPidTransform;
H::Input base() {
    H::Input in{};in.game="heartgold_nds";
    in.parentLanguagesAtCollectionVerified=in.trainerIdAtCollectionVerified=true;
    in.trainerId32=0x12345678u;return in;
}
int main() {
    auto in=base();in.physicalSlot0Language=in.physicalSlot1Language=2;
    in.pendingPid=0x01234567u;in.observedFinalPid=in.pendingPid;
    auto r=H::replay(in);assert(r.status==H::Status::ReplayedFinalPid);
    assert(!r.masudaActive&&!r.prandomSteps);
    in.physicalSlot1Language=1;
    uint32_t expected=in.pendingPid;uint8_t steps=0;
    if(!H::shiny(expected,in.trainerId32)) {
        for(;steps<4;++steps) {expected=H::next(expected);if(H::shiny(expected,in.trainerId32)){++steps;break;}}
        if(steps==4&&!H::shiny(expected,in.trainerId32))steps=4;
    }
    in.observedFinalPid=expected;r=H::replay(in);
    assert(r.status==H::Status::ReplayedFinalPid&&r.masudaActive&&r.prandomSteps==steps);
    in.observedFinalPid^=1;assert(H::replay(in).status==H::Status::CandidatePidContradicted);
    // A pending shiny PID is retained without ARNG calls even when Masuda is active.
    in.pendingPid=0;in.trainerId32=0;in.observedFinalPid=0;
    r=H::replay(in);assert(r.status==H::Status::ReplayedFinalPid&&r.masudaActive&&!r.prandomSteps);
    // Find one deterministic non-shiny PID that remains non-shiny after all four retries.
    in.trainerId32=0xCAFEBABEu;in.pendingPid=1;
    while(H::shiny(in.pendingPid,in.trainerId32)||H::shiny(H::next(H::next(H::next(H::next(in.pendingPid)))),in.trainerId32))++in.pendingPid;
    uint32_t after=in.pendingPid;for(int i=0;i<4;++i)after=H::next(after);
    in.observedFinalPid=after;r=H::replay(in);
    assert(r.status==H::Status::ReplayedFinalPid&&r.prandomSteps==4&&!H::shiny(r.finalPid,in.trainerId32));
    const auto valid=base();
    auto check=[&](auto mutate,H::Status status){auto x=valid;x.pendingPid=1;x.observedFinalPid=1;mutate(x);assert(H::replay(x).status==status);};
    check([](auto& x){x.game="platinum_nds";},H::Status::UnsupportedGame);
    check([](auto& x){x.parentLanguagesAtCollectionVerified=false;},H::Status::UnknownParentLanguages);
    check([](auto& x){x.trainerIdAtCollectionVerified=false;},H::Status::UnknownTrainerId);
    auto ss=base();ss.game="soulsilver_nds";ss.physicalSlot0Language=1;ss.physicalSlot1Language=2;ss.trainerId32=0;ss.pendingPid=0;ss.observedFinalPid=0;
    assert(H::replay(ss).status==H::Status::ReplayedFinalPid);
    std::cout<<"HGSS Masuda final PID transform: language, shiny and four-retry controls PASS\n";
}
