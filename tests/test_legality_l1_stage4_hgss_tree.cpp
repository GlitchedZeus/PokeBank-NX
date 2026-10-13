#include "Legality/Gen4HgssOriginalTreeStage.h"
#include <cassert>
#include <iostream>
#include <array>
using namespace Legality::Gen4HgssOriginalTreeStage;
int main(){
    static_assert(groupFor(2,0,1392200658u)==Group::A); // published ID example: 16 -> A
    static_assert(groupFor(3,0,1392200658u)==Group::B); // 24 -> B
    static_assert(groupFor(4,0,1392200658u)==Group::B); // 32 -> A
    static_assert(groupFor(5,0,1392200658u)==Group::B); // 40 -> B
    static_assert(kEncounterGroups[0].size()==20);
    static_assert(kEncounterGroups[1].size()==30);
    static_assert(kEncounterGroups[2].size()==40);
    static_assert(kEncounterGroups[3].size()==50);
    unsigned yes=0, local=0,unknown=0,empty=0;
    for(unsigned x=0;x<10;++x){
        for(unsigned n=2;n<=45;++n){
            for(unsigned t=0;t<n;++t){
                const auto expected=groupFor(static_cast<uint8_t>(n),static_cast<uint8_t>(t),x);
                assert(expected==Group::A||expected==Group::B||expected==Group::Empty);
                Candidate c{};c.exactOriginalGame="heartgold_nds";
                c.originalSaveIndependentlyAttested=true;
                c.fullTrainerIdIndependentlyAttested=true;
                c.originalTreeListIndependentlyAttested=true;
                c.physicalTreeIdIndependentlyAttested=true;
                c.originalGroupIndependentlyAttested=true;
                c.originalFullTrainerId=x;
                c.originalDesignatedTreeCount=static_cast<uint8_t>(n);
                c.originalTreeId=static_cast<uint8_t>(t);
                c.witnessedGroup=expected==Group::Empty?Group::A:expected;
                auto result=evaluate(c);
                if(expected==Group::Empty){assert(result.status==Status::CandidateTreeEmpty);++empty;}
                else {
                    assert(result.status==Status::ConditionalOriginalTreeGroup);++yes;
                    auto bad=c;bad.witnessedGroup=expected==Group::A?Group::B:Group::A;
                    assert(evaluate(bad).status==Status::CandidateTreeGroupContradicted);++local;
                }
                auto missing=c;missing.originalSaveIndependentlyAttested=false;
                assert(evaluate(missing).status==Status::UnknownOriginalTrainer);++unknown;
                missing=c;missing.fullTrainerIdIndependentlyAttested=false;
                assert(evaluate(missing).status==Status::UnknownOriginalTrainer);++unknown;
                missing=c;missing.originalTreeListIndependentlyAttested=false;
                assert(evaluate(missing).status==Status::UnknownTreeList);++unknown;
                missing=c;missing.physicalTreeIdIndependentlyAttested=false;
                assert(evaluate(missing).status==Status::UnknownTreeIdentity);++unknown;
                missing=c;missing.originalGroupIndependentlyAttested=false;
                assert(evaluate(missing).status==Status::UnknownObservedGroup);++unknown;
                missing=c;missing.exactOriginalGame="platinum_nds";
                assert(evaluate(missing).status==Status::UnknownOriginalGame);++unknown;
            }
        }
    }
    Candidate special{};special.exactOriginalGame="soulsilver_nds";
    special.originalSaveIndependentlyAttested=true;
    special.fullTrainerIdIndependentlyAttested=true;
    special.originalTreeListIndependentlyAttested=true;
    special.physicalTreeIdIndependentlyAttested=true;
    special.originalDesignatedTreeCount=5;
    special.originalTreeId=22;
    special.specialTreeC=true;
    special.originalGroupIndependentlyAttested=true;
    special.witnessedGroup=Group::SpecialC;
    assert(evaluate(special).status==Status::UnknownSpecialTreeSource);
    special.specialTreePositionIndependentlyAttested=true;
    assert(evaluate(special).status==Status::ConditionalOriginalTreeGroup);
    special.witnessedGroup=Group::B;
    assert(evaluate(special).status==Status::CandidateTreeGroupContradicted);
    std::cout<<"L1 S4 HGSS tree: "<<yes<<" compatible, "<<local<<" local contradictions, "
        <<empty<<" empty attested trees, "<<unknown<<" conservative Unknown PASS\n";
}
