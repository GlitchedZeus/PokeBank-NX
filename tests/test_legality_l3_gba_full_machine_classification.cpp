#include "Legality/Gen14CandidateMoveMachineEvidence.h"
#include <array>
#include <cassert>
#include <iostream>
namespace E=Legality::Gen14CandidateMoveMachineEvidence;
namespace D=Legality::Gen3GbaOriginalMachineData;
namespace C=Legality::Gen14CandidateMoveChronology;
constexpr std::array<C::Game,5> kGames{C::Game::Ruby,C::Game::Sapphire,C::Game::Emerald,C::Game::FireRed,C::Game::LeafGreen};
int main(){
    uint64_t compat=0,contradicted=0,unresolved=0;
    for(const auto game:kGames) for(uint16_t species=1;species<=386;++species)
       for(uint8_t number=1;number<=58;++number) {
        const E::Machine type=number<=50?E::Machine::TM:E::Machine::HM;
        const uint8_t machineNumber=number<=50?number:number-50;
        const uint16_t move=D::kMachineMove[number-1];
        const bool supported=(D::kRubySapphireMasks[species]&(uint64_t{1}<<(number-1)))!=0;
        const E::Stage s{game,species,type,machineNumber,move,true,true};
        const auto result=E::classify(s);
        assert(result==(supported?E::Status::CandidateCompatible:E::Status::CandidateContradicted));
        if(supported)++compat;else ++contradicted;
        auto unverified=s;unverified.itemIdentityVerified=false;
        assert(E::classify(unverified)==E::Status::Unresolved);++unresolved;
        auto wrongMove=s;wrongMove.move=(move==1?2:1);
        assert(E::classify(wrongMove)==E::Status::CandidateContradicted);
        C::Step correctStep{game,42,C::Action::Learn,type==E::Machine::TM?C::Method::TM:C::Method::HM,move,2,true,game};
        assert(E::classifyChronologyStep(s,correctStep)==result);
        correctStep.method=C::Method::Tutor;
        assert(E::classifyChronologyStep(s,correctStep)==E::Status::Unresolved);
       }
    assert(compat==8927ULL*5 && compat+contradicted==386ULL*58*5);
    assert(unresolved==386ULL*58*5);
    assert(E::classify({C::Game::Platinum,1,E::Machine::TM,6,92,true,true})==E::Status::Unresolved);
    std::cout << "L3 original source-bound machine classification: 5 games x 386 species x 58 items, " << compat << " positives, " << contradicted << " candidate negatives: PASS\n";
}
