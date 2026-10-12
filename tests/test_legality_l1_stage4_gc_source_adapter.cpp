// Stage 4 original Colosseum/XD indexed shadow/team-lock, fixed/trade
// templates and same-capture native PK3 adapters. Never global Invalid.
#include "Legality/Gen3GameCubeOriginalSourceAdapter.h"
#include <cassert>
#include <cstdint>
#include <iostream>
namespace A=Legality::Gen3GameCubeOriginalSourceAdapter;
namespace H=Legality::Gen3GameCubeOriginalTransferHistory;
namespace S=Legality::Gen3GameCubeIndexedShadowHistory;
namespace F=Legality::Gen3GameCubeOriginalFixedHistory;
namespace N=Legality::Gen3GameCubeOriginalNativeStage;
using Status=H::Status;

H::Candidate native(uint16_t species,uint32_t pid,
                    const std::array<uint8_t,6>& ivs,
                    N::Game game,N::Route route,
                    uint32_t ot32=0) {
    H::Candidate a{};
    a.specimenIdentity=10;a.originalGameCubeSaveIdentity=20;
    a.originalPhysicalSourceRoute=30;a.recipientSaveIdentity=40;
    a.originalRootIndependentlyAttested=true;
    a.originalSceneAndRouteIndependentlyAttested=true;
    a.originalIvsIndependentlyAttested=true;
    a.originalCapturedIvs=ivs;
    a.originalPurificationIndependentlyAttested=true;
    a.originalShadowCpuTeamHistoryIndependentlyAttested=true;
    a.originalTrainerIndependentlyAttested=true;
    a.originalRecipientTransferIndependentlyAttested=true;
    a.native.originalGame=game;
    a.native.originalRoute=route;
    a.native.originalGameIndependentlyAttested=true;
    a.native.originalSceneIndependentlyAttested=true;
    a.native.originalCapturedPidSpeciesOtIndependentlyAttested=true;
    a.native.originalPurificationIndependentlyAttested=true;
    a.native.originalRecipientSaveIndependentlyAttested=true;
    a.native.originalSpecies=species;
    a.native.originalPersonality=pid;
    a.native.originalTrainerId=ot32;
    a.native.native.species=species;
    a.native.native.personality=pid;
    a.native.native.originalTrainerId=ot32;
    a.native.native.originGame=route==N::Route::BonusDiscReward?1:15;
    a.native.native.ivs=ivs;
    a.native.native.checksumAndDecryptionVerified=true;
    a.attestedBonusRecipientNativeGame=1;
    return a;
}
int main() {
    unsigned passed=0;
    static_assert(A::mapShadow(S::Status::ConditionalShadowHistory)==
                  H::SourceCheck::SourceMatched);
    static_assert(A::mapShadow(S::Status::BoundedPidIvUnknown)==
                  H::SourceCheck::BoundedSearchExhausted);
    static_assert(A::mapFixed(F::Status::ConditionalOriginalFixedHistory)==
                  H::SourceCheck::SourceMatched);
    static_assert(A::mapFixed(F::Status::BoundedOriginalRngUnresolved)==
                  H::SourceCheck::BoundedSearchExhausted);
    // Original Colosseum Makuhita CPU team-lock, indexed original row 0.
    S::Candidate colo{};
    colo.originalGame=S::Game::Colosseum;
    colo.originalRowIndex=0;
    colo.originalSpecies=296;colo.originalLocation=5;colo.originalLevel=30;
    colo.originalOriginVersion=15;
    colo.originalPid=0xC252FEBA;
    colo.originalIVs={15,9,17,16,24,22};
    colo.originalGameIndependentlyAttested=true;
    colo.originalRowIndependentlyAttested=true;
    colo.originalCaptureStageIndependentlyAttested=true;
    auto c=native(colo.originalSpecies,colo.originalPid,
                  colo.originalIVs,N::Game::Colosseum,N::Route::ShadowPurified);
    assert(A::sameNativeShadowCapture(c,colo));
    assert(A::evaluateShadow(c,colo).status==Status::ConditionalGameCubeTransferOnly);
    passed+=2;
    auto bounded=colo;bounded.teamSearchBudget=1;
    assert(A::evaluateShadow(c,bounded).status==Status::UnknownBoundedTeamSearch);
    ++passed;
    auto wrong=colo;wrong.originalPid^=1;
    assert(A::evaluateShadow(c,wrong).status==Status::UnknownOriginalSource);
    ++passed;
    wrong=colo;wrong.originalFateful=true;
    assert(A::evaluateShadow(c,wrong).status==Status::CandidateOriginalSourceContradicted);
    ++passed;
    // Exact original XD Teddiursa anti-shiny trainer and full team source.
    S::Candidate xd{};
    xd.originalGame=S::Game::XD;xd.originalRowIndex=0;
    xd.originalSpecies=216;xd.originalLocation=143;xd.originalLevel=11;
    xd.originalBall=4;xd.originalOriginVersion=15;xd.originalFateful=true;
    xd.originalPid=0xB7951831;
    xd.originalIVs={9,31,12,31,25,22};
    xd.originalTid=12345;xd.originalSid=35598;
    xd.originalGameIndependentlyAttested=true;
    xd.originalRowIndependentlyAttested=true;
    xd.originalCaptureStageIndependentlyAttested=true;
    xd.originalTrainerIndependentlyAttested=true;
    c=native(xd.originalSpecies,xd.originalPid,xd.originalIVs,
             N::Game::XD,N::Route::ShadowPurified);
    c.originalTrainerTid=xd.originalTid;c.originalTrainerSid=xd.originalSid;
    assert(A::evaluateShadow(c,xd).status==Status::ConditionalGameCubeTransferOnly);
    ++passed;
    c.originalTrainerTid^=1;
    assert(A::evaluateShadow(c,xd).status==Status::UnknownOriginalSource);
    ++passed;
    // Fixed Colosseum Duking Plusle uses independently witnessed OT text.
    F::Candidate plusle{};
    plusle.originalGame=F::Game::Colosseum;
    plusle.originalRowIndex=2;
    auto& f=plusle.originalNative;
    f.species=311;f.originGame=15;f.language=2;
    f.otGender=0;f.tid=37149;f.sid=0;
    f.metLevel=13;f.metLocation=254;f.ball=4;
    f.pid=0x0985A297;f.ivs={6,1,0,7,17,7};
    f.otName=u"DUKING";
    plusle.originalGameIndependentlyAttested=true;
    plusle.originalPhysicalRowIndependentlyAttested=true;
    plusle.originalGiftOrTradeSceneIndependentlyAttested=true;
    plusle.originalNativeSnapshotIndependentlyAttested=true;
    plusle.originalTrainerAndTextIndependentlyAttested=true;
    c=native(f.species,f.pid,f.ivs,N::Game::Colosseum,
             N::Route::FixedGiftOrTrade,
             (static_cast<uint32_t>(f.sid)<<16)|f.tid);
    assert(A::sameNativeFixedCapture(c,plusle));
    assert(A::evaluateFixed(c,plusle).status==Status::ConditionalGameCubeTransferOnly);
    passed+=2;
    c.originalPhysicalSourceRoute=0;
    assert(A::evaluateFixed(c,plusle).status==Status::UnknownOriginalRoot);
    ++passed;
    c.originalPhysicalSourceRoute=30;c.originalCapturedIvs[0]^=1;
    assert(A::evaluateFixed(c,plusle).status==Status::UnknownOriginalSource);
    ++passed;
    // All unexpected SourceCheck enumerators are Unknown, not positive.
    for(unsigned n=128;n<256;++n) {
        assert(A::mapShadow(static_cast<S::Status>(n))==H::SourceCheck::Unknown);
        assert(A::mapFixed(static_cast<F::Status>(n))==H::SourceCheck::Unknown);
        passed+=2;
    }
    assert(passed>250);
    std::cout<<"L1 Stage4 original source adapters: "<<passed<<" PASS\n";
}
