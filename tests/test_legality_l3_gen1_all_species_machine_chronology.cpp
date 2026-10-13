// RBY machine *conditional* four-slot chronology, every original species.
// These constructed witnesses are NOT original individual/trainer history.
#include "Legality/Gen1RbyAllSpeciesJointHistory.h"
#include "Legality/Gen1OriginalMachineTables.h"
#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
#ifdef NDEBUG
#error Native legality evidence tests require assertions
#endif
namespace C=Legality::Gen1RbyAllSpeciesJointHistory;
namespace M=Legality::Gen1OriginalMachineTables;
using G=C::Game;
using A=C::Action;
using O=C::Outcome;
static C::Path witnessed(G game,std::uint16_t species,bool hm,
                         std::uint8_t number,std::uint16_t move) {
    C::Path p{};
    p.candidate=98765;
    p.individual=123456;
    p.source=p.observed=game;
    p.firstSpecies=p.lastSpecies=species;
    p.firstLevel=p.lastLevel=100;
    p.firstMoves={1,2,3,4};
    p.lastMoves={move,2,3,4};
    p.originalPhysicalSnapshotAttested=true;
    p.observedPhysicalSnapshotAttested=true;
    p.sameIndividualAttested=true;
    p.allTrainerDecisionsAttested=true;
    p.allInterveningMethodsCovered=true;
    p.count=1;
    auto& step=p.steps[0];
    step.action=A::TeachMachine;
    step.game=game;
    step.level=100;
    step.move=move;
    step.slot=0;
    step.machineMethod=hm?C::B::Method::HM:C::B::Method::TM;
    step.machineNumber=number;
    step.receipt=species*1000U+(hm?500U:0U)+number;
    step.independentlyAttested=true;
    step.sourceMethodAttested=true;
    return p;
}
int main() {
    std::uint64_t positives=0,unsupported=0,negatives=0;
    for(const G game:{G::Red,G::Blue,G::Yellow})
      for(std::uint16_t species=1;species<=151;++species)
        for(unsigned index=1;index<=55;++index) {
            const bool hm=index>50;
            const auto machine=static_cast<std::uint8_t>(hm?index-50:index);
            const auto move=M::moveFor(static_cast<std::uint8_t>(game),species,hm,machine);
            auto candidate=witnessed(game,species,hm,machine,move?move:85);
            const auto result=C::evaluate(candidate);
            assert(!result.globalInvalid);
            if(!move) {
                // Never turn an unsupported original method into whole-Pokémon
                // Invalid merely because an artificial candidate failed.
                assert(result.outcome==O::Unknown);
                ++unsupported;
                continue;
            }
            assert(result.outcome==O::CandidateConsistent);
            assert(result.teaches==1 && result.species==species);
            assert(result.finalMoves==candidate.lastMoves);
            ++positives;
            auto wrong=candidate;
            wrong.steps[0].move=move==165?164:165;
            wrong.lastMoves[0]=wrong.steps[0].move;
            assert(C::evaluate(wrong).outcome==O::CandidateContradicted);
            ++negatives;
            wrong=candidate;
            wrong.steps[0].machineMethod=static_cast<C::B::Method>(255);
            assert(C::evaluate(wrong).outcome==O::Unknown);
            ++negatives;
            wrong=candidate;wrong.steps[0].receipt=0;
            assert(C::evaluate(wrong).outcome==O::Unknown);
            ++negatives;
            wrong=candidate;wrong.steps[0].sourceMethodAttested=false;
            assert(C::evaluate(wrong).outcome==O::Unknown);
            ++negatives;
            wrong=candidate;wrong.allInterveningMethodsCovered=false;
            assert(C::evaluate(wrong).outcome==O::Unknown);
            ++negatives;
            wrong=candidate;wrong.originalPhysicalSnapshotAttested=false;
            assert(C::evaluate(wrong).outcome==O::Unknown);
            ++negatives;
        }
    assert(positives==3037U*2U+3043U);
    assert(positives+unsupported==151U*55U*3U);
    assert(negatives==positives*6U);
    // Charizard learns Fly only via the original Yellow machine table.
    auto yellow=witnessed(G::Yellow,6,true,2,19);
    assert(C::evaluate(yellow).outcome==O::CandidateConsistent);
    auto red=witnessed(G::Red,6,true,2,19);
    assert(C::evaluate(red).outcome==O::Unknown);
    // A generic Yellow Pikachu cannot Surf by its normal HM03 table.
    auto surfing=witnessed(G::Yellow,25,true,3,57);
    assert(C::evaluate(surfing).outcome==O::Unknown);
    // Mew's 55 original flags never authenticate the historical event.
    auto mew=witnessed(G::Red,151,false,50,164);
    assert(C::evaluate(mew).outcome==O::CandidateConsistent);
    mew.allTrainerDecisionsAttested=false;
    assert(C::evaluate(mew).outcome==O::Unknown);
    // Mutation of original source-game identity invalidates the witness:
    // explicitly separate last observed labels from claimed original route.
    auto split=witnessed(G::Yellow,6,true,2,19);
    split.observed=G::Blue;
    assert(C::evaluate(split).outcome==O::Unknown);
    std::cout<<"RBY all-151 four-slot machine witnesses: "<<positives
             <<" synthetic positive, "<<unsupported<<" unsupported, "
             <<negatives<<" candidate negative/Unknown checks PASS\n";
}
