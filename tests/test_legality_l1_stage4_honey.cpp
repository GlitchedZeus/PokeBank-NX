#include "Legality/Gen4SinnohOriginalHoneyHistory.h"
#include <cassert>
#include <iostream>
using namespace Legality::Gen4SinnohOriginalHoneyHistory;
int main(){
    static_assert(kOriginalMetLocations.size()==21);
    constexpr auto fixed=munchlaxTreeIndices(0u);
    static_assert(fixed[0]!=fixed[1]);
    unsigned compatible=0,contradiction=0,unknown=0;
    for(uint32_t trainer=0;trainer<256;++trainer){
        for(uint8_t tree=0;tree<21;++tree){
            Candidate c{};
            c.exactGame="platinum_nds";
            c.originalSaveTrainerId32=trainer*0x1020305u;
            c.originalTreeIndex=tree;
            c.originalMetLocation=kOriginalMetLocations[tree];
            c.originalMinutesRemaining=1080;
            c.savedEncounterRow=5;
            c.savedEncounterGroup=permitsMunchlax(c.originalSaveTrainerId32,tree)?Group::Munchlax:Group::Common;
            c.originalSpecies=c.savedEncounterGroup==Group::Munchlax?446:415;
            c.originalSaveIndependentlyAttested=true;
            c.originalTreeIndependentlyAttested=true;
            c.originalSlatherEventIndependentlyAttested=true;
            c.originalTimerIndependentlyAttested=true;
            c.originalSavedSelectionIndependentlyAttested=true;
            c.originalSpeciesIndependentlyAttested=true;
            c.originalTrainerIdIndependentlyAttested=true;
            assert(evaluate(c).status==Status::ConditionalOriginalHoneyHistory);++compatible;
            auto bad=c;bad.originalMinutesRemaining=1081;
            assert(evaluate(bad).status==Status::CandidateTimerBeforeAppearance);++contradiction;
            bad=c;bad.originalMinutesRemaining=0;
            assert(evaluate(bad).status==Status::CandidateTimerExpired);++contradiction;
            bad=c;bad.originalTreeIndex=21;
            assert(evaluate(bad).status==Status::UnknownTree);++unknown;
            bad=c;bad.originalMetLocation=999;
            assert(evaluate(bad).status==Status::UnknownTree);++unknown;
            bad=c;bad.originalSlatherEventIndependentlyAttested=false;
            assert(evaluate(bad).status==Status::UnknownSlather);++unknown;
            bad=c;bad.originalTimerIndependentlyAttested=false;
            assert(evaluate(bad).status==Status::UnknownHistoricalTimer);++unknown;
            bad=c;bad.originalMinutesRemaining=1441;
            assert(evaluate(bad).status==Status::UnknownHistoricalTimer);++unknown;
            bad=c;bad.originalSavedSelectionIndependentlyAttested=false;
            assert(evaluate(bad).status==Status::UnknownSavedSelection);++unknown;
            bad=c;bad.originalSpeciesIndependentlyAttested=false;
            assert(evaluate(bad).status==Status::UnknownSavedSelection);++unknown;
            bad=c;bad.exactGame="heartgold_nds";
            assert(evaluate(bad).status==Status::UnknownOriginalGame);++unknown;
            if(c.savedEncounterGroup==Group::Munchlax){
                bad=c;bad.originalTrainerIdIndependentlyAttested=false;
                assert(evaluate(bad).status==Status::UnknownTrainer);++unknown;
                bad=c;bad.savedEncounterGroup=Group::Common;
                assert(evaluate(bad).status==Status::CandidateGroupContradicted);++contradiction;
                bad=c;bad.originalSpecies=415;
                assert(evaluate(bad).status==Status::CandidateSpeciesContradicted);++contradiction;
                uint8_t other=0;
                while(other<21 && permitsMunchlax(c.originalSaveTrainerId32,other))++other;
                assert(other<21);
                bad=c;bad.originalTreeIndex=other;bad.originalMetLocation=kOriginalMetLocations[other];
                assert(evaluate(bad).status==Status::CandidateMunchlaxTreeContradicted);++contradiction;
            }
        }
    }
    std::cout<<"L1 S4 Sinnoh honey: "<<compatible<<" compatible, "
        <<contradiction<<" candidate-local, "<<unknown<<" Unknown PASS\n";
}
