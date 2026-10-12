#include "Legality/Gen14CandidateMoveMachineEvidence.h"
#include <cassert>
#include <iostream>
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
static_assert([]{auto s=given(G::Ruby,M::TM,9,92);
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
int main(){
    for(G g:{G::Ruby,G::Sapphire,G::Emerald,G::FireRed,G::LeafGreen}) {
        assert(E::classify(given(g,M::TM,6,92))==S::CandidateCompatible);
        assert(E::classify(given(g,M::TM,10,237))==S::CandidateCompatible);
        assert(E::classify(given(g,M::HM,1,15))==S::CandidateCompatible);
    }
    std::cout<<"L3 pinned GBA machine-item method evidence: PASS\n";
}
