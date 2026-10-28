#include "Legality/FiniteBreedingChain.h"
#include <algorithm>
#include <cassert>
#include <iostream>
using namespace Legality::Breeding;
// Independent small-state interpreter. It teaches/forgets one move at a time
// and explicitly constructs offspring from two concrete individuals. It does
// not use the optimized solver's subset/combined-mask transition expressions.
static std::vector<std::array<bool,16>> interpret(const std::vector<Species>& graph) {
    std::vector<std::array<bool,16>> state(graph.size());
    for(auto& individual:state)individual[0]=true;
    for(size_t s=1;s<graph.size();++s)for(auto gift:graph[s].gifts)state[s][gift]=true;
    while(true) {
        auto next=state;
        for(size_t s=1;s<graph.size();++s) {
            const auto& rule=graph[s];
            for(uint8_t moves=0;moves<16;++moves)if(state[s][moves]) {
                for(uint8_t bit=0;bit<4;++bit) {
                    if(moves&(1u<<bit))next[s][moves&~(1u<<bit)]=true;
                    if(rule.ordinary&(1u<<bit))next[s][moves|(1u<<bit)]=true;
                }
                for(auto to:rule.evolvesTo)if(to<graph.size())next[to][moves]=true;
            }
            auto mothers=rule.mothers;
            if(mothers.empty())mothers.push_back(0); // no shared-level contributor
            for(auto father:rule.fathers)for(auto mother:mothers)
                for(uint8_t dad=0;dad<16;++dad)if(state[father][dad])
                    for(uint8_t mom=0;mom<16;++mom)if(state[mother][mom]) {
                        uint8_t baby=0;
                        for(uint8_t move=0;move<4;++move) {
                            if(!(dad&(1u<<move))||!(rule.inherited&(1u<<move)))continue;
                            if((rule.requiresMother&(1u<<move))&&!(mom&(1u<<move)))continue;
                            baby|=1u<<move;
                        }
                        next[s][baby]=true;
                    }
        }
        if(next==state)return state;
        state=std::move(next);
    }
}
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
    // Each mother can know either event move, but no single mother can know
    // both. Even a father that knows both cannot produce the joint inheritance.
    std::vector<Species> pairs(4);
    pairs[1].gifts={1,2};pairs[2].ordinary=3;
    pairs[3].inherited=3;pairs[3].requiresMother=3;
    pairs[3].fathers={2};pairs[3].mothers={1};
    FiniteChain noMaternalUnion(pairs);noMaternalUnion.solve();
    assert(noMaternalUnion.canKnow(3,1)&&noMaternalUnion.canKnow(3,2));
    assert(!noMaternalUnion.canKnow(3,3));
    assert(!noMaternalUnion.hasFather(3,3));
    pairs[1].gifts.push_back(3);
    FiniteChain actualPair(pairs);actualPair.solve();assert(actualPair.canKnow(3,3));
    assert(actualPair.hasFather(3,3));
    const auto pairedTrace=actualPair.witness(3,3);
    assert(pairedTrace.front().motherSpecies==1&&pairedTrace.front().motherMask==3);
    assert(std::any_of(pairedTrace.begin(),pairedTrace.end(),[](const Step& s){return s.species==1&&s.kind==StepKind::Gift&&s.mask==3;}));
    // A Ditto route cannot invent shared level-up knowledge. Paternal-only
    // sources still work; existing models' empty-mother behavior is preserved.
    pairs[3].mothers.clear();pairs[3].requiresMother=2;
    FiniteChain ditto(pairs);ditto.solve();assert(ditto.canKnow(3,1)&&!ditto.canKnow(3,2));
    // Deterministic adversarial finite models: adding an origin cannot remove
    // an existing proof, and every returned witness references an attained state.
    uint32_t random=8173939;size_t checks=0;
    for(size_t trial=0;trial<300;++trial) {
        const auto next=[&](){random^=random<<13;random^=random>>17;random^=random<<5;return random;};
        std::vector<Species> graph(5);
        for(size_t s=1;s<graph.size();++s) {
            auto& r=graph[s];r.ordinary=next()&15;r.inherited=next()&15;
            r.requiresMother=next()&15;r.gifts={static_cast<uint8_t>(next()&15)};
            r.fathers={static_cast<uint16_t>(1+next()%4)};
            if(next()&1)r.mothers={static_cast<uint16_t>(1+next()%4)};
            r.evolvesTo={static_cast<uint16_t>(1+next()%4)};
        }
        const auto expected=interpret(graph);FiniteChain actual(graph);actual.solve();
        for(size_t s=1;s<graph.size();++s)for(uint8_t mask=0;mask<16;++mask) {
            assert(actual.canKnow(s,mask)==expected[s][mask]);
            for(const auto& step:actual.witness(s,mask)) {
                if(step.parentSpecies)assert(expected[step.parentSpecies][step.parentMask]);
                if(step.motherSpecies)assert(expected[step.motherSpecies][step.motherMask]);
            }
        }
    }
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
