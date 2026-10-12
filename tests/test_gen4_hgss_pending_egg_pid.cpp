#include "Legality/Gen4HGSSPendingEggPid.h"
#include <cassert>
#include <iostream>
namespace H=Legality::Gen4HGSSPendingEggPid;
H::Input input(std::span<const uint32_t> values) {
    H::Input in{};in.game="heartgold_nds";
    in.parentItemsAndOrderVerified=in.creationStageVerified=true;
    in.parentStageToken=in.mtStageToken=42;
    in.parentPidKnown={true,true};in.parentPid={0,7};
    in.lcEntryStateKnown=true;
    in.consecutiveMTOutputsVerified=in.exactConsumedMTCountVerified=true;
    in.mtOutputs=values;return in;
}
int main(){
    const std::array<uint32_t,1> zero{0};
    auto in=input(zero);
    auto r=H::replay(in);assert(r.status==H::Status::ReplayedPendingPid);
    assert(r.pendingPid==0&&r.mtConsumed==1&&r.lcConsumed==0);
    in.lcEntryStateKnown=false;in.parentPidKnown={false,false};
    assert(H::replay(in).status==H::Status::ReplayedPendingPid); // irrelevant without proc
    const std::array<uint32_t,3> natureZero{0,1,25};
    in=input(natureZero);in.heldItems={229,0};
    r=H::replay(in);assert(r.status==H::Status::ReplayedPendingPid);
    assert(r.everstoneSlot==0&&r.pendingPid==25&&r.mtConsumed==3&&r.lcConsumed==1);
    const std::array<uint32_t,2> natureSeven{0,7};
    in=input(natureSeven);in.heldItems={0,229};
    r=H::replay(in);assert(r.status==H::Status::ReplayedPendingPid&&r.everstoneSlot==1&&r.pendingPid==7);
    in.heldItems={229,229};in.lcEntryState=2;
    r=H::replay(in);assert(r.status==H::Status::ReplayedPendingPid&&r.everstoneSlot==1&&r.lcConsumed==2);
    in.lcEntryState=3;in.mtOutputs=natureZero;
    r=H::replay(in);assert(r.status==H::Status::ReplayedPendingPid&&r.everstoneSlot==0&&r.lcConsumed==2);
    // Full 16-bit proc threshold: 32766 succeeds, 32767 and 32768 fail.
    in=input(natureZero);in.heldItems={229,0};in.lcEntryState=0xb36b61a1u;
    assert(H::replay(in).everstoneSlot==0);
    in.mtOutputs=zero;in.lcEntryState=0x9ed061a1u;
    r=H::replay(in);assert(r.status==H::Status::ReplayedPendingPid&&r.everstoneSlot==-1&&r.pendingPid==0);
    in.lcEntryState=0x8a3561a1u;assert(H::replay(in).everstoneSlot==-1);
    // The game's bound is 2401 draws, not 2400; zero cannot satisfy nature.
    std::array<uint32_t,2401> exhausted{};
    in=input(exhausted);in.heldItems={229,0};
    r=H::replay(in);assert(r.status==H::Status::ReplayedPendingPid);
    assert(r.mtConsumed==2401&&r.pendingPid==0&&r.retailRetryLimitReached);
    in.mtOutputs=std::span<const uint32_t>(exhausted.data(),2400);
    assert(H::replay(in).status==H::Status::IncompleteMTTranscript);
    exhausted.back()=25;in.mtOutputs=exhausted;
    r=H::replay(in);assert(r.status==H::Status::ReplayedPendingPid&&!r.retailRetryLimitReached&&r.pendingPid==25);
    const std::array<uint32_t,2> extra{25,50};in.mtOutputs=extra;
    assert(H::replay(in).status==H::Status::TranscriptCountContradicted);
    std::array<uint32_t,2402> oversized{};in.mtOutputs=oversized;
    assert(H::replay(in).status==H::Status::MalformedMTTranscript);
    const auto baseline=input(natureZero);
    auto check=[&](auto change,H::Status status){auto x=baseline;x.heldItems={229,0};change(x);assert(H::replay(x).status==status);};
    check([](auto& x){x.game="platinum_nds";},H::Status::UnsupportedGame);
    check([](auto& x){x.parentItemsAndOrderVerified=false;},H::Status::UnknownParentItems);
    check([](auto& x){x.creationStageVerified=false;},H::Status::UnknownCreationStage);
    check([](auto& x){x.mtStageToken=0;},H::Status::UnknownCreationStage);
    check([](auto& x){x.mtStageToken=99;},H::Status::IncoherentCreationStage);
    check([](auto& x){x.lcEntryStateKnown=false;},H::Status::UnknownLCRNGState);
    check([](auto& x){x.parentPidKnown[0]=false;},H::Status::UnknownParentNature);
    check([](auto& x){x.consecutiveMTOutputsVerified=false;},H::Status::UnknownMTTranscript);
    check([](auto& x){x.exactConsumedMTCountVerified=false;},H::Status::UnknownMTTranscript);
    check([](auto& x){x.mtOutputs={};},H::Status::UnknownMTTranscript);
    in=input(natureZero);in.heldItems={229,0};in.game="soulsilver_nds";
    assert(H::replay(in).pendingPid==25);
    std::cout<<"HGSS pending egg PID: donor, threshold, transcript and 2401-draw boundary controls PASS\n";
}
