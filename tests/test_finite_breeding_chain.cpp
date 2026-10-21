#include "Legality/FiniteBreedingChain.h"
#include <cassert>
#include <iostream>
using namespace Legality::Breeding;
int main() {
    // A chain deeper than PKHeX's depth=10 cap must still complete.
    std::vector<Species> deep(20);deep[1].gifts={3};
    for(size_t i=2;i<deep.size();++i){deep[i].inherited=3;deep[i].fathers={static_cast<uint16_t>(i-1)};}
    FiniteChain longProof(deep);longProof.solve();assert(longProof.canKnow(19,3));
    const auto trace=longProof.witness(19,3);assert(trace.size()==19);
    // A cycle with two individually obtainable event moves must never merge
    // those two different individuals into a fictional father.
    std::vector<Species> cycle(4);cycle[1].gifts={1,2};cycle[1].inherited=3;cycle[1].fathers={2};
    cycle[2].inherited=3;cycle[2].fathers={1};cycle[3].inherited=3;cycle[3].fathers={1,2};
    FiniteChain noUnion(cycle);noUnion.solve();assert(noUnion.canKnow(3,1)&&noUnion.canKnow(3,2));assert(!noUnion.canKnow(3,3));
    // One actual carrier acquires an old-era TM first and a new-era level move
    // afterwards. Reversing that history would purge the new-era inherited move.
    std::vector<Species> eras(4);eras[1].previousEra=1;eras[1].ordinary=2;
    eras[2].gifts={2};eras[2].previousEra=1;
    eras[3].evolvesTo={2};eras[3].gifts={2};
    FiniteChain transfer(eras,1);transfer.solve();assert(transfer.canKnow(1,3));assert(!transfer.canKnow(2,3));
    // A mixed gift/native ancestry is allowed, but two different fathers aren't.
    cycle[1].ordinary=4;cycle[3].inherited=7;
    FiniteChain mix(cycle);mix.solve();assert(mix.canKnow(3,5)&&mix.canKnow(3,6));assert(!mix.canKnow(3,7));
    // Deterministic adversarial finite models: adding an origin cannot remove
    // an existing proof, and every returned witness references an attained state.
    uint32_t random=8173939;size_t checks=0;
    for(size_t trial=0;trial<2000;++trial) {
        const auto next=[&](){random^=random<<13;random^=random>>17;random^=random<<5;return random;};
        std::vector<Species> graph(7);
        for(size_t s=1;s<graph.size();++s){auto& r=graph[s];r.ordinary=next()&15;r.previousEra=next()&15;r.inherited=next()&15;r.gifts={static_cast<uint8_t>(next()&15)};r.fathers={static_cast<uint16_t>(1+next()%6)};r.evolvesTo={static_cast<uint16_t>(1+next()%6)};}
        FiniteChain before(graph,7);before.solve();
        auto extra=graph;extra[1].gifts.push_back(15);FiniteChain after(extra,7);after.solve();
        for(size_t s=1;s<graph.size();++s)for(uint8_t mask=0;mask<16;++mask) {
            if(before.canKnow(s,mask))assert(after.canKnow(s,mask));
            for(const auto& step:before.witness(s,mask))if(step.parentSpecies)assert(before.canKnow(step.parentSpecies,step.parentMask));
            ++checks;
        }
    }
    std::cout<<"Finite joint breeding proof: deep chain, non-merging cycles/events, era chronology, mixed origins and "<<checks<<" adversarial state checks PASS\n";
}
