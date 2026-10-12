#include "Legality/Gen4HGSSEggPidHistory.h"
#include <array>
#include <cassert>
#include <iostream>
namespace H=Legality::Gen4HGSSEggPidHistory;
H::Input base() {
    static constexpr std::array<uint32_t,1> pid{0x01234567u};
    H::Input in{};
    in.creation.game="heartgold_nds";
    in.creation.parentItemsAndOrderVerified=in.creation.creationStageVerified=true;
    in.creation.parentStageToken=in.creation.mtStageToken=0x4321;
    in.creation.consecutiveMTOutputsVerified=in.creation.exactConsumedMTCountVerified=true;
    in.creation.mtOutputs=pid;
    in.collection.game="heartgold_nds";
    in.collection.parentLanguagesAtCollectionVerified=true;
    in.collection.physicalSlot0Language=in.collection.physicalSlot1Language=2;
    in.collection.trainerIdAtCollectionVerified=true;
    in.collection.trainerId32=0x12345678u;
    in.collection.pendingPid=pid[0];in.collection.observedFinalPid=pid[0];
    in.creationEggEventToken=in.collectionEggEventToken=0xBEEF;
    return in;
}
int main() {
    auto in=base();auto r=H::replay(in);
    assert(r.status==H::Status::ReplayedFinalPid);
    assert(r.creation.pendingPid==in.collection.pendingPid);
    in.collection.physicalSlot1Language=1;
    uint32_t final=in.collection.pendingPid;
    for(int i=0;i<4&&!H::Collection::shiny(final,in.collection.trainerId32);++i)
        final=H::Collection::next(final);
    in.collection.observedFinalPid=final;
    r=H::replay(in);assert(r.status==H::Status::ReplayedFinalPid);
    assert(r.collection.finalPid==final);
    auto check=[&](auto mutate,H::Status wanted) {auto x=base();mutate(x);assert(H::replay(x).status==wanted);};
    check([](auto& x){x.creation.creationStageVerified=false;},H::Status::CreationStageNotReplayed);
    check([](auto& x){x.collection.parentLanguagesAtCollectionVerified=false;},H::Status::UnknownCollectionStage);
    check([](auto& x){x.collection.trainerIdAtCollectionVerified=false;},H::Status::UnknownCollectionStage);
    check([](auto& x){x.creationEggEventToken=0;},H::Status::UnknownEggEventIdentity);
    check([](auto& x){x.collectionEggEventToken=0xCAFE;},H::Status::IncoherentEggEvent);
    check([](auto& x){++x.collection.pendingPid;},H::Status::IncoherentPendingPid);
    check([](auto& x){++x.collection.observedFinalPid;},H::Status::CandidatePidContradicted);
    auto ss=base();ss.creation.game=ss.collection.game="soulsilver_nds";
    assert(H::replay(ss).status==H::Status::ReplayedFinalPid);
    std::cout<<"HGSS egg PID history: one-event creation/collection chronology controls PASS\n";
}
