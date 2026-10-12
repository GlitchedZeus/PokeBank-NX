#include "Legality/Gen3PidIvFamilyRouter.h"
#include <cassert>
#include <iostream>
int main() {
    namespace F=Legality::Gen3PidIvFamilyRouter;
    using Status=F::Status;
    F::Candidate c{};
    c.originalGame=F::Game::Emerald;
    c.family=F::Family::HandheldWild;
    c.familyIndependentlyAttested=true;
    c.pid=0xE97E0000u;
    c.ivs={17,19,20,16,13,12};
    const auto normal=F::evaluate(c);
    assert(normal.matched());
    assert(normal.handheldMethod==Legality::Gen3PidIv::Method::Method1);
    assert(normal.seed==0u);
    // Methods 2/3/4 remain separate LCRNG frame histories.
    c.pid=0x5271E97Eu;c.ivs={2,18,3,12,22,24};
    assert(F::evaluate(c).handheldMethod==Legality::Gen3PidIv::Method::Method2);
    c.pid=0x3DD1BB49u;c.ivs={23,12,31,9,3,3};
    assert(F::evaluate(c).handheldMethod==Legality::Gen3PidIv::Method::Method3);
    c.pid=0x31B05271u;c.ivs={2,18,3,5,30,11};
    assert(F::evaluate(c).handheldMethod==Legality::Gen3PidIv::Method::Method4);
    c.originalGame=F::Game::FireRed;
    c.family=F::Family::TruncatedRoamer;
    c.pid=0x84EA0B71u;c.ivs={10,4,0,0,0,0};
    assert(F::evaluate(c).handheldMethod==Legality::Gen3PidIv::Method::Method1Roamer);
    assert(F::evaluate(c).seed==0x12345678u);
    c.originalGame=F::Game::Emerald; // Emerald roamer is not the truncated family
    assert(F::evaluate(c).status==Status::UnsupportedOriginalMethod);
    c.originalGame=F::Game::FireRed;
    c.ivs[2]=1;
    assert(F::evaluate(c).status==Status::CandidateUnresolved);
    c.originalGame=F::Game::Colosseum;
    c.family=F::Family::GameCubeShadow;
    c.pid=0x0985A297u;c.ivs={6,1,0,7,17,7};
    c.originalTrainerIndependentlyAttested=false;
    assert(F::evaluate(c).status==Status::UnknownOriginalTrainer);
    c.originalTrainerIndependentlyAttested=true;
    assert(F::evaluate(c).matched());
    c.originalGame=F::Game::XD;
    c.pid=0xB7951831u;c.ivs={9,31,12,31,25,22};
    c.tid=12345;c.sid=35598;
    assert(F::evaluate(c).matched()); // pinned XD anti-shiny rejection
    c.originalGame=F::Game::Ruby;
    c.family=F::Family::ChannelGift;
    c.pid=0x264750D9u;c.ivs={6,31,14,27,5,27};
    c.sid=45819;c.channelOriginGame=2;c.channelOtGender=1;
    c.channelStageIndependentlyAttested=true;
    assert(F::evaluate(c).matched());
    c.channelOtGender=0;
    assert(F::evaluate(c).status==Status::CandidateUnresolved);
    c.channelOtGender=1;
    c.family=F::Family::BacdEvent;
    c.pid=0x67DBFC33u;c.ivs={12,25,27,30,2,31};
    c.tid=40122;c.sid=45819;
    assert(F::evaluate(c).matched());
    auto wrong=c;wrong.family=F::Family::Unknown;
    assert(F::evaluate(wrong).status==Status::UnknownOriginalRngFamily);
    wrong=c;wrong.familyIndependentlyAttested=false;
    assert(F::evaluate(wrong).status==Status::UnknownOriginalRngFamily);
    wrong=c;wrong.ivs[0]=32;
    assert(F::evaluate(wrong).status==Status::MalformedStoredIvs);
    wrong=c;wrong.originalGame=F::Game::Unknown;
    assert(F::evaluate(wrong).status==Status::UnknownOriginalGame);
    wrong=c;wrong.originalGame=F::Game::Colosseum;
    assert(F::evaluate(wrong).status==Status::UnsupportedOriginalMethod);
    std::cout<<"L1 E11 five-family exact-game dispatch: PASS\n";
}
