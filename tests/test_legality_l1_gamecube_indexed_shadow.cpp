#include "Legality/Gen3GameCubeIndexedShadowHistory.h"
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
int main(){
    namespace S=Legality::Gen3GameCubeIndexedShadowHistory;
    namespace C=Legality::Gen3ColoShadowEncounter;
    namespace X=Legality::Gen3XdShadowEncounter;
    using Status=S::Status;
    // Pinned PKHeX Colosseum: Makuhita with a real non-empty
    // recursive CPU team-lock history, not only a mathematical XDRNG hit.
    S::Candidate colo{};
    colo.originalGame=S::Game::Colosseum;
    colo.originalRowIndex=0;
    colo.originalSpecies=296;colo.originalLocation=5;colo.originalLevel=30;
    colo.originalOriginVersion=15;
    colo.originalPid=0xC252FEBAu;
    colo.originalIVs={15,9,17,16,24,22};
    colo.originalGameIndependentlyAttested=true;
    colo.originalRowIndependentlyAttested=true;
    colo.originalCaptureStageIndependentlyAttested=true;
    assert(C::kEntries[0].shadowIndex==1);
    assert(S::evaluate(colo).status==Status::ConditionalShadowHistory);
    // First no-lock Colosseum team: its original source is Bayleef
    // (a different exact row and level/location than XD's first shadow).
    auto bayleef=colo;
    bayleef.originalRowIndex=1;bayleef.originalSpecies=153;
    bayleef.originalLocation=3;bayleef.originalLevel=30;
    bayleef.originalPid=0x0985A297u;
    bayleef.originalIVs={6,1,0,7,17,7};
    assert(C::kEntries[1].teamSet==
        static_cast<uint8_t>(Legality::Gen3ColoShadowTeamLock::TeamSet::First));
    assert(S::evaluate(bayleef).status==Status::ConditionalShadowHistory);
    auto bounded=colo;bounded.teamSearchBudget=1;
    assert(S::evaluate(bounded).status==Status::BoundedTeamSearchUnknown);
    auto wrong=colo;wrong.originalFateful=true; // XD field on Colosseum
    assert(S::evaluate(wrong).status==Status::CandidateOriginalIdentityContradicted);
    wrong=colo;wrong.originalIVs[0]=31;
    assert(S::evaluate(wrong).status==Status::CandidatePidIvContradicted);
    // Source table contains two distinct Murkrow shadow candidates with
    // the same persistent PK3 tuple and DIFFERENT team sets; retain both.
    unsigned murkrow=0;bool sawMurkrowLock=false,sawFirstLock=false;
    for(const auto& row:C::kEntries){
        if(row.species==198&&row.location==67&&row.level==43){
            ++murkrow;
            sawMurkrowLock|=row.teamSet==
                static_cast<uint8_t>(Legality::Gen3ColoShadowTeamLock::TeamSet::Murkrow);
            sawFirstLock|=row.teamSet==
                static_cast<uint8_t>(Legality::Gen3ColoShadowTeamLock::TeamSet::First);
        }
    }
    assert(murkrow==2 && sawMurkrowLock && sawFirstLock);
    // XD Teddiursa: pinned anti-shiny skipped-PID policy; its team is First.
    S::Candidate xd{};
    xd.originalGame=S::Game::XD;
    xd.originalRowIndex=0;
    xd.originalSpecies=216;xd.originalLocation=143;xd.originalLevel=11;
    xd.originalOriginVersion=15;xd.originalBall=4;xd.originalFateful=true;
    xd.originalTid=12345;xd.originalSid=35598;
    xd.originalPid=0xB7951831u;xd.originalIVs={9,31,12,31,25,22};
    xd.originalGameIndependentlyAttested=true;
    xd.originalRowIndependentlyAttested=true;
    xd.originalCaptureStageIndependentlyAttested=true;
    xd.originalTrainerIndependentlyAttested=true;
    assert(X::kEntries[0].index==1);
    const auto anti=S::evaluate(xd);
    assert(anti.status==Status::ConditionalShadowHistory && anti.xdAntiShiny);
    wrong=xd;wrong.originalBall=3;
    assert(S::evaluate(wrong).status==Status::CandidateOriginalIdentityContradicted);
    wrong=xd;wrong.originalFateful=false;
    assert(S::evaluate(wrong).status==Status::CandidateOriginalIdentityContradicted);
    // XD shiny anti-check is recomputed from the attested PID and
    // trainer, not trusted from a caller's boolean.
    wrong=xd;wrong.originalTid=static_cast<uint16_t>(
        (wrong.originalPid>>16) ^ (wrong.originalPid&0xFFFFu) ^ wrong.originalSid);
    assert(S::evaluate(wrong).status==Status::CandidateOriginalIdentityContradicted);
    wrong=xd;wrong.originalTid=12346;
    assert(S::evaluate(wrong).status!=Status::ConditionalShadowHistory);
    auto missing=xd;missing.originalTrainerIndependentlyAttested=false;
    assert(S::evaluate(missing).status==Status::UnknownOriginalTrainer);
    missing=xd;missing.originalCaptureStageIndependentlyAttested=false;
    assert(S::evaluate(missing).status==Status::UnknownOriginalStage);
    missing=xd;missing.originalRowIndependentlyAttested=false;
    assert(S::evaluate(missing).status==Status::UnknownPhysicalRow);
    missing=xd;missing.originalGameIndependentlyAttested=false;
    assert(S::evaluate(missing).status==Status::UnknownOriginalGame);
    // XD Mawile, valid standard XDRNG with full original NPC-team path.
    auto mawile=xd;
    std::size_t mawileIndex=X::kEntries.size();
    for(std::size_t i=0;i<X::kEntries.size();++i)
       if(X::kEntries[i].species==303&&X::kEntries[i].location==111){
           mawileIndex=i;break;
       }
    assert(mawileIndex<X::kEntries.size());
    mawile.originalRowIndex=mawileIndex;
    mawile.originalSpecies=303;mawile.originalLocation=111;
    mawile.originalLevel=22;mawile.originalTid=12345;mawile.originalSid=51882;
    mawile.originalPid=0x049F2F05u;
    mawile.originalIVs={31,30,29,31,23,27};
    auto source=S::evaluate(mawile);
    assert(source.status==Status::ConditionalShadowHistory && !source.xdAntiShiny);
    // Miror B rebattle location must be an independent historical witness.
    auto rebattle=mawile;rebattle.originalLocation=90;
    assert(S::evaluate(rebattle).status==Status::UnknownRebattleStage);
    rebattle.originalRebattleIndependentlyAttested=true;
    assert(S::evaluate(rebattle).status==Status::ConditionalShadowHistory);
    auto alien=xd;alien.originalGame=S::Game::Colosseum;
    assert(S::evaluate(alien).status!=Status::ConditionalShadowHistory);
    std::cout<<"L1 E09/E10 pinned Colosseum/XD source + CPU/team histories: PASS\n";
}
