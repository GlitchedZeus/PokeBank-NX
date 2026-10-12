#include "Legality/Gen14CandidateMoveGbaRelearner.h"
#include <array>
#include <cassert>
#include <iostream>
namespace R=Legality::Gen14CandidateMoveGbaRelearner;
using G=R::Game; using S=R::Status;
constexpr R::Input base(G game,uint16_t species,uint8_t level,
                        std::array<uint16_t,4> before,uint16_t learned,
                        uint8_t slot,std::array<uint16_t,4> after) {
    R::Input a{};a.candidateId=1001;a.game=game;
    a.nationalSpecies=species;a.level=level;
    a.beforeMoves=before;a.afterMoves=after;
    a.selectedMove=learned;a.replacementSlot=slot;
    a.originalPokemonAndSpeciesVerified=true;
    a.originalGameAndServiceAccessVerified=true;
    a.exactLevelAndFourSlotsVerified=true;
    a.exactPlayerChoiceVerified=true;
    return a;
}
constexpr auto emeraldBulba=base(G::Emerald,1,15,{33,45,73,22},
                                 77,0,{77,45,73,22});
static_assert(R::evaluate(emeraldBulba).status==S::CandidateConsistent);
static_assert(R::evaluate(emeraldBulba).offerCount==2); // 77,79 at lv15
static_assert(R::evaluate(emeraldBulba).offers[0]==77);
static_assert(R::evaluate(emeraldBulba).offers[1]==79);
static_assert([]{auto a=emeraldBulba;a.selectedMove=76;
    return R::evaluate(a).status==S::CandidateContradicted;}()); // lv46
static_assert([]{auto a=emeraldBulba;a.selectedMove=73;
    return R::evaluate(a).status==S::CandidateContradicted;}()); // already known
static_assert([]{auto a=emeraldBulba;a.exactPlayerChoiceVerified=false;
    return R::evaluate(a).status==S::Unresolved;}());
static_assert([]{auto a=emeraldBulba;a.originalGameAndServiceAccessVerified=false;
    return R::evaluate(a).status==S::Unresolved;}());
static_assert([]{auto a=emeraldBulba;a.beforeMoves={33,0,73,22};
    return R::evaluate(a).status==S::Unresolved;}());
static_assert([]{auto a=emeraldBulba;a.replacementSlot=4;
    return R::evaluate(a).status==S::CandidateContradicted;}());
static_assert([]{auto a=emeraldBulba;a.afterMoves={33,45,73,22};
    return R::evaluate(a).status==S::CandidateContradicted;}());
static_assert([]{auto a=emeraldBulba;a.game=G::Ruby;
    return R::evaluate(a).status==S::CandidateConsistent;}());
static_assert([]{auto a=emeraldBulba;a.game=G::Sapphire;
    return R::evaluate(a).status==S::CandidateConsistent;}());
constexpr auto frlgChar=base(G::FireRed,4,13,{10,45,52,0},232,3,
                             {10,45,52,232});
static_assert(R::evaluate(frlgChar).status==S::CandidateConsistent);
static_assert(R::evaluate(frlgChar).offerCount==1);
static_assert(R::evaluate(frlgChar).offers[0]==232); // FRLG Metal Claw
static_assert([]{auto a=frlgChar;a.game=G::Emerald;
    return R::evaluate(a).status==S::CandidateContradicted;}());
static_assert([]{auto a=frlgChar;a.game=G::Ruby;
    return R::evaluate(a).status==S::CandidateContradicted;}());
static_assert([]{auto a=frlgChar;a.game=G::LeafGreen;
    return R::evaluate(a).status==S::CandidateConsistent;}());
static_assert([]{auto a=frlgChar;a.replacementSlot=2;
    return R::evaluate(a).status==S::CandidateContradicted;}());
constexpr auto repeatedIvysaur=base(G::Emerald,2,4,{33,0,0,0},45,1,
                                     {33,45,0,0});
static_assert(R::evaluate(repeatedIvysaur).status==S::CandidateConsistent);
static_assert(R::evaluate(repeatedIvysaur).skippedDuplicate==1);
static_assert(R::evaluate(repeatedIvysaur).offers[0]==45);
static_assert(R::evaluate(repeatedIvysaur).offers[1]==73);
constexpr auto blastoise=base(G::FireRed,9,70,{33,39,145,110},56,0,
                             {56,39,145,110});
static_assert(R::evaluate(blastoise).status==S::CandidateConsistent);
static_assert(R::evaluate(blastoise).offers[R::evaluate(blastoise).offerCount-1]==56);
constexpr auto charizardHeat=base(G::FireRed,6,36,{10,45,52,232},257,3,
                                  {10,45,52,257});
static_assert(R::evaluate(charizardHeat).status==S::CandidateConsistent);
static_assert([]{auto a=charizardHeat;a.game=G::Emerald;
    return R::evaluate(a).status==S::CandidateContradicted;}());
int main(){
    unsigned tested=0;
    for(G g:{G::Ruby,G::Sapphire,G::Emerald,G::FireRed,G::LeafGreen}) {
        const auto b=base(g,1,15,{33,45,73,22},77,0,{77,45,73,22});
        assert(R::evaluate(b).status==S::CandidateConsistent);++tested;
    }
    for(G g:{G::FireRed,G::LeafGreen}) {
        auto c=frlgChar;c.game=g;
        assert(R::evaluate(c).status==S::CandidateConsistent);++tested;
        c.selectedMove=108;
        assert(R::evaluate(c).status==S::CandidateContradicted);++tested;
    }
    assert(tested==9);
    std::cout<<"L3 pinned RS/E/FRLG Move Relearner exact-game menu+four-slot decisions: PASS\n";
}
