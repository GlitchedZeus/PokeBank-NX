#include "Legality/Gen4PlatinumHGSSOriginalEggPIDHistory.h"
#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
namespace H=Legality::Gen4PlatinumHGSSOriginalEggPIDHistory;
constexpr std::array<uint32_t,3> natureTrace{1u,0u,32u};
H::Input fixture(const char* game,std::span<const uint32_t> mt=natureTrace){
    H::Input x{};
    x.game=game;
    x.originalEggEventId=0xBA55;
    x.originalEggIdentityIndependentlyVerified=true;
    x.originalUnhatchedEggPID=32;
    x.ordinaryPairEligibilityIndependentlyVerified=true;
    x.eligibleOrdinaryPair=true;
    x.physicalDepositOrderIndependentlyVerified=true;
    x.parents[0]={101,25,229,1,7,true,true,true};
    x.parents[1]={202,25,0,0,11,true,true,true};
    x.parentStageToken=0xBA55;
    x.pendingStageToken=0xBA55;
    x.collectionStageToken=0xBA55;
    x.creationAndCollectionStagesIndependentlyVerified=true;
    x.lcrngEntryIndependentlyVerified=true;
    x.lcrngEntryState=0;
    x.consecutiveMTTranscriptIndependentlyVerified=true;
    x.exactMTConsumptionIndependentlyVerified=true;
    x.consecutiveMT=mt;
    x.parentLanguagesAtCollectionIndependentlyVerified=true;
    x.originalParentLanguages={2,2};
    x.collectingTrainerFullOTIndependentlyVerified=true;
    x.collectingTrainerFullOT=0;
    return x;
}
int main(){
    uint32_t seeded=0,negative=0;
    for(const char* game:{"platinum_nds","heartgold_nds","soulsilver_nds"}){
        auto x=fixture(game);
        const auto original=H::evaluate(x);
        assert(original.status==H::Status::ConditionalOriginalEggPIDMatched);
        assert(original.originalEverstoneDonor==0);
        assert(original.everstoneActivated && original.mtDrawCount==3);
        assert(original.lcDrawCount==1 && original.finalLCState==0x00006073u);
        // Pt: only female/Ditto Everstone influences pending PID.
        // HGSS: either physical holder may influence pending PID.
        auto reversed=x;
        reversed.parents[0]=x.parents[1];
        reversed.parents[1]=x.parents[0];
        assert(H::evaluate(reversed).status==H::Status::ConditionalOriginalEggPIDMatched);
        assert(H::evaluate(reversed).originalEverstoneDonor==1);
        auto maleStone=x;
        maleStone.parents[0].heldItem=0;
        maleStone.parents[1].heldItem=229;
        if(H::family(game)==H::Family::Platinum){
            constexpr std::array<uint32_t,1> one{1};
            maleStone.consecutiveMT=one;
            maleStone.originalUnhatchedEggPID=1;
            const auto noInherit=H::evaluate(maleStone);
            assert(noInherit.status==H::Status::ConditionalOriginalEggPIDMatched);
            assert(noInherit.originalEverstoneDonor==-1 && noInherit.lcDrawCount==0);
        } else {
            constexpr std::array<uint32_t,2> matching{1,11};
            maleStone.consecutiveMT=matching;
            maleStone.originalUnhatchedEggPID=11;
            const auto inherits=H::evaluate(maleStone);
            assert(inherits.status==H::Status::ConditionalOriginalEggPIDMatched);
            assert(inherits.originalEverstoneDonor==1 && inherits.lcDrawCount==1);
        }
        // HGSS: dual Everstones use a donor-selection draw BEFORE coin.
        auto both=x;both.parents[1].heldItem=229;
        const auto dual=H::evaluate(both);
        if(H::family(game)==H::Family::HGSS){
            assert(dual.lcDrawCount==2);
            assert(!dual.everstoneActivated && dual.originalEverstoneDonor==-1); // coin fails at seed 0
        } else {
            assert(dual.lcDrawCount==1 && dual.originalEverstoneDonor==0);
        }
        auto ditto=x;
        ditto.parents[1].species=132;ditto.parents[1].gender=2;
        ditto.parents[1].heldItem=229;
        // Platinum: Ditto over female; HGSS: slot-selection random draw.
        if(H::family(game)==H::Family::HGSS)ditto.lcrngEntryState=2;
        const auto dittoCheck=H::evaluate(ditto);
        assert(dittoCheck.originalEverstoneDonor==1);
        auto unknownItem=x;unknownItem.parents[0].creationItemIndependentlyVerified=false;
        assert(H::evaluate(unknownItem).status==H::Status::UnknownCreationItemHistory);
        auto unknownParent=x;unknownParent.parents[0].originalSnapshotIndependentlyVerified=false;
        assert(H::evaluate(unknownParent).status==H::Status::UnknownOriginalParentSnapshots);
        auto missingNature=x;missingNature.parents[0].originalNaturePIDIndependentlyVerified=false;
        assert(H::evaluate(missingNature).status==H::Status::UnknownEverstoneParentNature);
        auto missingLC=x;missingLC.lcrngEntryIndependentlyVerified=false;
        assert(H::evaluate(missingLC).status==H::Status::UnknownEverstoneLCRNG);
        auto mismatchedStage=x;mismatchedStage.pendingStageToken++;
        assert(H::evaluate(mismatchedStage).status==H::Status::IncoherentSourceChronology);
        auto unknownChronology=x;unknownChronology.creationAndCollectionStagesIndependentlyVerified=false;
        assert(H::evaluate(unknownChronology).status==H::Status::UnknownSourceChronology);
        auto unverifiedMT=x;unverifiedMT.consecutiveMTTranscriptIndependentlyVerified=false;
        assert(H::evaluate(unverifiedMT).status==H::Status::UnknownMTTranscript);
        auto unverifiedCount=x;unverifiedCount.exactMTConsumptionIndependentlyVerified=false;
        assert(H::evaluate(unverifiedCount).status==H::Status::UnknownMTTranscript);
        auto incomplete=x;incomplete.consecutiveMT=std::span(natureTrace).first(2);
        assert(H::evaluate(incomplete).status==H::Status::IncompleteMTTranscript);
        constexpr std::array<uint32_t,4> extra{1,0,32,57};
        auto overconsumed=x;overconsumed.consecutiveMT=extra;
        assert(H::evaluate(overconsumed).status==H::Status::ContradictoryMTConsumption);
        auto unknownLanguage=x;unknownLanguage.parentLanguagesAtCollectionIndependentlyVerified=false;
        assert(H::evaluate(unknownLanguage).status==H::Status::UnknownCollectionLanguages);
        auto unknownOT=x;unknownOT.collectingTrainerFullOTIndependentlyVerified=false;
        assert(H::evaluate(unknownOT).status==H::Status::UnknownCollectionTrainer);
        auto crossedParent=x;crossedParent.parents[1].snapshotId=101;
        assert(H::evaluate(crossedParent).status==H::Status::UnknownOriginalParentSnapshots);
        auto incompatible=x;incompatible.eligibleOrdinaryPair=false;
        assert(H::evaluate(incompatible).status==H::Status::IncompatibleOrdinaryPair);
        auto twoDitto=x;twoDitto.parents[0].species=132;twoDitto.parents[1].species=132;
        assert(H::evaluate(twoDitto).status==H::Status::IncompatibleOrdinaryPair);
        auto twoMales=x;twoMales.parents[0].gender=0;
        assert(H::evaluate(twoMales).status==H::Status::IncompatibleOrdinaryPair);
        auto wrongPID=x;wrongPID.originalUnhatchedEggPID++;
        assert(H::evaluate(wrongPID).status==H::Status::OriginalEggPIDContradicted);
        // At most 2401 MT candidates, with last candidate retained by
        // retail even if its PID nature does not match the donor.
        std::array<uint32_t,2401> rejects{};rejects.fill(1);
        auto fallback=x;fallback.consecutiveMT=rejects;
        fallback.originalUnhatchedEggPID=1;
        assert(H::evaluate(fallback).status==H::Status::ConditionalOriginalEggPIDMatched);
        assert(H::evaluate(fallback).retailNatureAttemptLimitReached);
        auto truncated=fallback;truncated.consecutiveMT=std::span(rejects).first(2400);
        assert(H::evaluate(truncated).status==H::Status::IncompleteMTTranscript);
        constexpr std::array<uint32_t,2402> tooLong{};
        auto overflow=x;overflow.consecutiveMT=tooLong;
        assert(H::evaluate(overflow).status==H::Status::OversizedMTTranscript);
        // Verify generic game-family seed fanout and source evidence gates.
        for(uint32_t seed=0;seed<2048;++seed){
            auto item=x;
            item.lcrngEntryState=seed;
            constexpr std::array<uint32_t,1> single{32u};
            item.consecutiveMT=single;
            const auto r=H::evaluate(item);
            assert(r.status==H::Status::ConditionalOriginalEggPIDMatched ||
                   r.status==H::Status::OriginalEggPIDContradicted ||
                   r.status==H::Status::IncompleteMTTranscript);
            // A positive may only be claimed when the observed original
            // PID equals the candidate's reconstructed final PID.
            if(r.status!=H::Status::IncompleteMTTranscript){
                item.originalUnhatchedEggPID=r.originalFinalPID;
                assert(H::evaluate(item).status==H::Status::ConditionalOriginalEggPIDMatched);
                item.originalUnhatchedEggPID^=1u;
                assert(H::evaluate(item).status==H::Status::OriginalEggPIDContradicted);
            }
            ++seeded;
        }
        negative+=26;
    }
    auto unsupported=fixture("diamond_nds");
    assert(H::evaluate(unsupported).status==H::Status::UnsupportedGame);
    std::cout<<"L2 original Platinum/HGSS PID "<<seeded
             <<" seeds + "<<negative<<" fixed controls PASS (synthetic source stages)\n";
}
