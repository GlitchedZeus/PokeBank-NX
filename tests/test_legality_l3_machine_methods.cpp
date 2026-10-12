#include "Legality/Gen14CandidateMoveMachineEvidence.h"
#include <cassert>
#include <iostream>
#include <array>
namespace E=Legality::Gen14CandidateMoveMachineEvidence;
namespace C=Legality::Gen14CandidateMoveChronology;
using G=C::Game;using M=E::Machine;using S=E::Status;
constexpr E::Stage given(G g,M kind,uint8_t number,uint16_t move){
    return {g,1,kind,number,move,true,true};
}
static_assert(E::classify(given(G::Ruby,M::TM,6,92))==S::CandidateCompatible);
static_assert(E::classify(given(G::Sapphire,M::TM,10,237))==S::CandidateCompatible);
static_assert(E::classify(given(G::Emerald,M::HM,1,15))==S::CandidateCompatible);
static_assert(E::classify(given(G::FireRed,M::TM,6,92))==S::CandidateCompatible);
static_assert(E::classify(given(G::LeafGreen,M::HM,1,15))==S::CandidateCompatible);
static_assert(E::classify(given(G::Emerald,M::TM,6,237))==S::CandidateContradicted);
static_assert(E::classify(given(G::Ruby,M::TM,10,92))==S::CandidateContradicted);
static_assert(E::classify(given(G::FireRed,M::HM,1,57))==S::CandidateContradicted);
static_assert(E::classify(given(G::Platinum,M::HM,1,15))==S::Unresolved);
static_assert([]{auto s=given(G::Ruby,M::TM,6,92);s.nationalSpecies=4;
    return E::classify(s)==S::Unresolved;}());
static_assert([]{auto s=given(G::Ruby,M::TM,50,92);
    return E::classify(s)==S::Unresolved;}());
static_assert([]{auto s=given(G::Ruby,M::TM,6,92);s.itemIdentityVerified=false;
    return E::classify(s)==S::Unresolved;}());
static_assert([]{auto s=given(G::Ruby,M::TM,6,92);s.samePokemonAndGameVerified=false;
    return E::classify(s)==S::Unresolved;}());
static_assert([]{auto s=given(G::Ruby,M::TM,6,92);s.machine=static_cast<M>(255);
    return E::classify(s)==S::Unresolved;}());
static_assert([]{auto s=given(G::Ruby,M::TM,6,92);
    C::Step w{G::Ruby,41,C::Action::Learn,C::Method::TM,92,2,true,G::Ruby};
    return E::classifyChronologyStep(s,w)==S::CandidateCompatible;}());
static_assert([]{auto s=given(G::Ruby,M::TM,6,92);
    C::Step w{G::Emerald,41,C::Action::Learn,C::Method::TM,92,2,true,G::Emerald};
    return E::classifyChronologyStep(s,w)==S::Unresolved;}());
static_assert([]{auto s=given(G::Ruby,M::TM,6,92);
    C::Step w{G::Ruby,41,C::Action::Learn,C::Method::Tutor,92,2,true,G::Ruby};
    return E::classifyChronologyStep(s,w)==S::Unresolved;}());
static_assert([]{auto s=given(G::Ruby,M::TM,6,92);
    C::Step w{G::Ruby,41,C::Action::Learn,C::Method::TM,92,2,false,G::Ruby};
    return E::classifyChronologyStep(s,w)==S::Unresolved;}());
constexpr std::array<std::pair<uint8_t,uint16_t>,15> tmMoves{{
    {6,92},{9,331},{10,237},{11,241},{17,182},{19,202},
    {21,218},{22,76},{27,216},{32,104},{36,188},{42,263},
    {43,290},{44,156},{45,213}
}};
constexpr std::array<std::pair<uint8_t,uint16_t>,4> hmMoves{{
    {1,15},{4,70},{5,148},{6,249}
}};
static_assert([]{
    for(auto [tm,move]:tmMoves) {
        if(E::classify(given(G::Ruby,M::TM,tm,move))!=S::CandidateCompatible)
            return false;
    }
    return true;
}());
static_assert([]{
    for(auto [hm,move]:hmMoves) {
        if(E::classify(given(G::LeafGreen,M::HM,hm,move))!=S::CandidateCompatible)
            return false;
    }
    return true;
}());
int main(){
    for(G g:{G::Ruby,G::Sapphire,G::Emerald,G::FireRed,G::LeafGreen}) {
        assert(E::classify(given(g,M::TM,6,92))==S::CandidateCompatible);
        assert(E::classify(given(g,M::TM,10,237))==S::CandidateCompatible);
        assert(E::classify(given(g,M::HM,1,15))==S::CandidateCompatible);
        for(auto [machine,move]:tmMoves)
            assert(E::classify(given(g,M::TM,machine,move))==S::CandidateCompatible);
        for(auto [machine,move]:hmMoves)
            assert(E::classify(given(g,M::HM,machine,move))==S::CandidateCompatible);
    }
    std::cout<<"L3 pinned GBA machine-item method evidence: PASS\n";
}
