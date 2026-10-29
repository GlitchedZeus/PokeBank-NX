// Stage 4 L1 real original native source-stage and original-transfer
// reconciliation; synthetic attestations only, never global Legal/Invalid.
#include "Legality/Gen3GameCubeOriginalTransferHistory.h"
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
namespace H=Legality::Gen3GameCubeOriginalTransferHistory;
namespace N=Legality::Gen3GameCubeOriginalNativeStage;
using S=H::Status;

H::Candidate original(N::Game game,N::Route route) {
    H::Candidate c{};
    c.specimenIdentity=0x13579BDF;
    c.originalGameCubeSaveIdentity=0xAABBCCDD;
    c.originalPhysicalSourceRoute=91;
    c.recipientSaveIdentity=0xDDCCBBAA;
    c.originalRootIndependentlyAttested=true;
    c.originalSceneAndRouteIndependentlyAttested=true;
    c.originalSourceEvaluatorIndependentlyReplayed=true;
    c.sourceCheck=H::SourceCheck::SourceMatched;
    c.originalCapturedIvs={15,7,31,3,25,1};
    c.originalIvsIndependentlyAttested=true;
    c.originalTrainerTid=12345;c.originalTrainerSid=35598;
    c.originalTrainerIndependentlyAttested=true;
    c.originalShadowCpuTeamHistoryIndependentlyAttested=true;
    c.originalPurificationIndependentlyAttested=true;
    c.originalRecipientTransferIndependentlyAttested=true;
    c.attestedBonusRecipientNativeGame=1;
    c.native.originalGame=game;
    c.native.originalRoute=route;
    c.native.originalGameIndependentlyAttested=true;
    c.native.originalSceneIndependentlyAttested=true;
    c.native.originalCapturedPidSpeciesOtIndependentlyAttested=true;
    c.native.originalPurificationIndependentlyAttested=true;
    c.native.originalRecipientSaveIndependentlyAttested=true;
    c.native.originalPersonality=0xB7951831;
    c.native.originalTrainerId=0x87654321;
    c.native.originalSpecies=216;
    c.native.native.personality=c.native.originalPersonality;
    c.native.native.originalTrainerId=c.native.originalTrainerId;
    c.native.native.species=c.native.originalSpecies;
    c.native.native.ivs=c.originalCapturedIvs;
    c.native.native.checksumAndDecryptionVerified=true;
    c.native.native.originGame=route==N::Route::BonusDiscReward?1:15;
    return c;
}
int main() {
    std::uint64_t count=0;
    for(const auto game:{N::Game::Colosseum,N::Game::XD}) {
        for(const auto route:{N::Route::ShadowPurified,
                             N::Route::FixedGiftOrTrade,
                             N::Route::BonusDiscReward}) {
            const auto c=original(game,route);
            const auto status=H::evaluate(c).status;
            const bool possible=(game!=N::Game::XD ||
                                 route!=N::Route::BonusDiscReward);
            assert(status==(possible?S::ConditionalGameCubeTransferOnly:
                                     S::UnknownNativeSnapshot));
            ++count;
            if(!possible)continue;
            auto bad=c;bad.originalIvsIndependentlyAttested=false;
            assert(H::evaluate(bad).status==S::UnknownIndependentNativeIvs);
            bad=c;bad.originalCapturedIvs[0]=33;
            assert(H::evaluate(bad).status==S::UnknownMalformedNativeIvs);
            bad=c;bad.native.native.ivs[2]=255;
            assert(H::evaluate(bad).status==S::UnknownMalformedNativeIvs);
            bad=c;bad.native.native.checksumAndDecryptionVerified=false;
            assert(H::evaluate(bad).status==S::UnknownNativeSnapshot);
            bad=c;bad.native.native.personality^=1;
            assert(H::evaluate(bad).status==S::CandidateNativeIdentityContradicted);
            bad=c;bad.native.native.ivs[0]^=1;
            assert(H::evaluate(bad).status==S::CandidateNativeIvsContradicted);
            bad=c;bad.originalSourceEvaluatorIndependentlyReplayed=false;
            assert(H::evaluate(bad).status==S::UnknownOriginalSource);
            bad=c;bad.sourceCheck=H::SourceCheck::CandidateContradicted;
            assert(H::evaluate(bad).status==S::CandidateOriginalSourceContradicted);
            bad=c;bad.sourceCheck=H::SourceCheck::BoundedSearchExhausted;
            assert(H::evaluate(bad).status==S::UnknownBoundedTeamSearch);
            bad=c;bad.sourceCheck=static_cast<H::SourceCheck>(255);
            assert(H::evaluate(bad).status==S::UnknownOriginalSource);
            bad=c;bad.specimenIdentity=0;
            assert(H::evaluate(bad).status==S::UnknownOriginalRoot);
            bad=c;bad.originalGameCubeSaveIdentity=0;
            assert(H::evaluate(bad).status==S::UnknownOriginalRoot);
            bad=c;bad.originalPhysicalSourceRoute=0;
            assert(H::evaluate(bad).status==S::UnknownOriginalRoot);
            bad=c;bad.originalRecipientTransferIndependentlyAttested=false;
            assert(H::evaluate(bad).status==S::UnknownOriginalRecipient);
            bad=c;bad.recipientSaveIdentity=0;
            assert(H::evaluate(bad).status==S::UnknownOriginalRecipient);
            bad=c;bad.originalSceneAndRouteIndependentlyAttested=false;
            assert(H::evaluate(bad).status==S::UnknownScene);
            bad=c;bad.native.originalCapturedPidSpeciesOtIndependentlyAttested=false;
            assert(H::evaluate(bad).status==S::UnknownNativeSnapshot);
            count+=17;
            if(route==N::Route::ShadowPurified) {
                bad=c;bad.originalTrainerIndependentlyAttested=false;
                assert(H::evaluate(bad).status==S::UnknownOriginalTrainer);
                bad=c;bad.originalShadowCpuTeamHistoryIndependentlyAttested=false;
                assert(H::evaluate(bad).status==S::UnknownOriginalTrainer);
                bad=c;bad.originalPurificationIndependentlyAttested=false;
                assert(H::evaluate(bad).status==S::UnknownPurification);
                count+=3;
                if(game==N::Game::XD) {
                    bad=c;
                    bad.originalTrainerTid=static_cast<uint16_t>(
                         (bad.native.originalPersonality>>16) ^
                         (bad.native.originalPersonality&0xFFFFu) ^
                         bad.originalTrainerSid);
                    assert(H::evaluate(bad).status==
                           S::CandidateOriginalTrainerShinyContradicted);
                    ++count;
                }
            }
            if(route==N::Route::BonusDiscReward) {
                bad=c;bad.attestedBonusRecipientNativeGame=2;
                assert(H::evaluate(bad).status==S::UnknownOriginalRecipient);
                bad=c;bad.attestedBonusRecipientNativeGame=4;
                assert(H::evaluate(bad).status==S::UnknownOriginalRecipient);
                count+=2;
            }
        }
    }
    // All 256 corrupt enum values, including a claimed XD bonus.
    for(unsigned game=0;game<256;++game) {
        for(unsigned route=0;route<256;++route) {
            auto c=original(static_cast<N::Game>(game),
                            static_cast<N::Route>(route));
            const auto result=H::evaluate(c);
            const bool validGame=(game==1 || game==2);
            const bool validRoute=(route>=1 && route<=3);
            const bool known=validGame && validRoute &&
                             !(game==2 && route==3);
            if(known)assert(result.status==S::ConditionalGameCubeTransferOnly);
            else assert(result.status==S::UnknownScene ||
                        result.status==S::UnknownNativeSnapshot);
            assert(!result.globalInvalidSupported && !result.globallyLegalSupported);
            count+=3;
        }
    }
    // Exhaust every 3-state 8-route universe: independent contradictory
    // sources can never make a global-invalid determination.
    for(unsigned encoding=0;encoding<6561;++encoding) {
        std::array<H::Alternative,8> a{};
        unsigned value=encoding;
        bool seenMatch=false,seenUnknown=false;
        for(unsigned i=0;i<a.size();++i) {
            a[i].routeIdentity=i+1;
            a[i].original=original(N::Game::Colosseum,N::Route::ShadowPurified);
            const unsigned symbol=value%3;value/=3;
            a[i].original.sourceCheck=
                symbol==0?H::SourceCheck::SourceMatched:
                symbol==1?H::SourceCheck::CandidateContradicted:
                          H::SourceCheck::BoundedSearchExhausted;
            seenMatch|=symbol==0;
            seenUnknown|=symbol==2;
        }
        const auto result=H::evaluateUniverse(0x13579BDF,true,a);
        assert(result.status==
            (seenMatch?H::UniverseStatus::OneConditionalTransferHistory:
             seenUnknown?H::UniverseStatus::UnknownAlternativeHistory:
                         H::UniverseStatus::SuppliedRoutesLocallyContradictedOnly));
        assert(result.conditional+result.contradicted+result.unknown==8);
        assert(!result.globalInvalidSupported && !result.globallyLegalSupported);
        count+=3;
        a[0].routeIdentity=a[1].routeIdentity;
        assert(H::evaluateUniverse(0x13579BDF,true,a).status==
               H::UniverseStatus::UnknownDuplicateRoute);
        ++count;
    }
    std::array<H::Alternative,17> tooMany{};
    for(unsigned i=0;i<tooMany.size();++i) {
        tooMany[i].routeIdentity=i+1;
        tooMany[i].original=original(N::Game::Colosseum,N::Route::FixedGiftOrTrade);
    }
    assert(H::evaluateUniverse(0x13579BDF,true,tooMany).status==
           H::UniverseStatus::UnknownCapacity);
    assert(H::evaluateUniverse(0,true,std::span<const H::Alternative>()).status==
           H::UniverseStatus::UnknownRoot);
    count+=2;
    assert(count>220000);
    std::cout<<"L1 Stage4 GC native source/lock/purification/transfer "<<count
             <<" independent conditions PASS\n";
}
