#include "Legality/Gen4OriginalJkNatureStage.h"
#include <cassert>
#include <cstdint>
#include <iostream>
namespace J=Legality::Gen4OriginalJkNatureStage;
int main(){
    unsigned positives=0,negative=0,unknown=0,sync=0;
    for(unsigned game=1;game<=5;++game){
        for(uint32_t seed=0;seed<1024;++seed){
            J::Candidate c{};c.originalGame=static_cast<J::Game>(game);
            c.historicalLead=J::Lead::NoLead;
            c.originalGameIndependentlyAttested=true;
            c.originalNatureStageIndependentlyAttested=true;
            c.originalLeadIndependentlyAttested=true;
            c.originalPidSnapshotIndependentlyAttested=true;
            c.witnessedPreNatureState=seed*0x1020305u;
            auto state=J::next(c.witnessedPreNatureState);
            c.witnessedTargetNature=J::rollNature(c.originalGame,static_cast<uint16_t>(state>>16));
            bool found=false;
            for(unsigned frame=0;frame<4096;++frame){
                state=J::next(state);auto lo=state>>16;
                state=J::next(state);auto pid=(state&0xFFFF0000u)|lo;
                if(pid%25u==c.witnessedTargetNature){c.originalPid=pid;found=true;break;}
            }
            assert(found);
            c.maxPidPairs=4096;
            assert(J::evaluate(c).status==J::Status::ConditionalOriginalJkNatureStage);++positives;
            auto bad=c;bad.originalPid^=1u;
            assert(J::evaluate(bad).status==J::Status::CandidatePidContradicted);++negative;
            bad=c;bad.witnessedTargetNature=25;
            assert(J::evaluate(bad).status==J::Status::UnknownOriginalNature);++unknown;
            bad=c;bad.originalNatureStageIndependentlyAttested=false;
            assert(J::evaluate(bad).status==J::Status::UnknownOriginalStage);++unknown;
            bad=c;bad.originalPidSnapshotIndependentlyAttested=false;
            assert(J::evaluate(bad).status==J::Status::UnknownOriginalPid);++unknown;
            bad=c;bad.maxPidPairs=0;
            assert(J::evaluate(bad).status==J::Status::UnknownSearchWindow);++unknown;
            bad=c;bad.originalLeadIndependentlyAttested=false;
            assert(J::evaluate(bad).status==J::Status::UnknownOriginalLead);++unknown;
            bad=c;bad.historicalLead=static_cast<J::Lead>(255);
            assert(J::evaluate(bad).status==J::Status::UnknownOriginalLead);++unknown;
            // Independently attested successful Synchronize can overwrite
            // the random nature, yet must still satisfy actual PID nature.
            const uint16_t rand16=static_cast<uint16_t>(J::next(c.witnessedPreNatureState)>>16);
            if(J::synchronizeSuccess(c.originalGame,rand16)){
                bad=c;bad.historicalLead=J::Lead::SynchronizeSuccess;
                assert(J::evaluate(bad).status==J::Status::ConditionalOriginalJkNatureStage);
                ++sync;
            }else{
                bad=c;bad.historicalLead=J::Lead::SynchronizeSuccess;
                assert(J::evaluate(bad).status==J::Status::UnknownOriginalLead);++unknown;
            }
            // A local short replay cap is never proof that all histories fail.
            bad=c;bad.maxPidPairs=1;
            auto limited=J::evaluate(bad);
            assert(limited.status==J::Status::UnknownSearchWindow ||
                   limited.status==J::Status::ConditionalOriginalJkNatureStage);
        }
    }
    assert(sync>100);
    std::cout<<"L1 S4 J/K nature: "<<positives<<" compatible, "<<sync<<" Sync success, "
        <<negative<<" local contradictions, "<<unknown<<" Unknown PASS\n";
}
