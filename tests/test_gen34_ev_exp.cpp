// SPDX-License-Identifier: AGPL-3.0-or-later
#include "Legality/Gen34EvExpFeasibility.h"
#include <cassert>
#include <iostream>

#ifdef NDEBUG
#error This regression requires assertions
#endif

using namespace Legality::EvExp;
static void replay(const Request& request, const std::vector<Battle>& battles, const Result& proof) {
    assert(proof.verdict == Verdict::Proven);
    Evs actual{};uint32_t experience=0;bool infected=false;
    for (const auto& step:proof.witness) {
        if (step.operation==Operation::Battle) {
            assert(!infected || step.pokerus);
            infected|=step.pokerus;experience+=battles.at(step.battle).experience;
        }
        actual=apply(actual,step,battles,request.generation);
        assert(total(actual)<=510);
    }
    assert(actual==request.target && experience==proof.witnessExperience && experience<=request.availableExperience);
}
int main() {
    // Original HGSS source replay: level2 Hoothoot, six participants/five
    // EXP Shares; the subject holds a Power Bracer. The same one action must
    // supply HP+1 AND Attack+4; they are not separate EXP budgets.
    const std::vector<Battle> hoothoot{{{1,0,0,0,0,0},1,163,2}};
    Request r{{101,104,0,0,0,0},1,4,true,true,false,true,2048};
    auto proof=solve(r,hoothoot);replay(r,hoothoot,proof);assert(proof.witnessExperience==1);
    r.availableExperience=0;assert(solve(r,hoothoot).verdict==Verdict::Impossible);
    // Each individual deficit fits two maximally relaxed actions; their
    // combined29 EVs do not fit two actions of at most14 total EVs each.
    r.target={114,114,101,0,0,0};r.availableExperience=2;
    assert(solve(r,hoothoot).necessaryExperience==3);
    assert(solve(r,hoothoot).verdict==Verdict::Impossible);
    r.generation=3;r.target={112,112,101,0,0,0};
    assert(solve(r,hoothoot).verdict==Verdict::Impossible);
    r.generation=4;r.target={100,100,100,100,100,10};r.availableExperience=0;
    replay(r,hoothoot,solve(r,hoothoot));
    r.target={255,0,0,0,0,0};r.availableExperience=100;
    replay(r,hoothoot,solve(r,hoothoot)); // native cap255, never252
    assert((apply({254,0,0,0,0,0},{Operation::Battle,0,0,1,false},hoothoot,4)==Evs{255,0,0,0,0,0}));
    assert((apply({135,0,0,0,0,0},{Operation::Berry,0,0},hoothoot,3)==Evs{125,0,0,0,0,0}));
    assert((apply({135,0,0,0,0,0},{Operation::Berry,0,0},hoothoot,4)==Evs{100,0,0,0,0,0}));
    assert((apply({99,0,0,0,0,0},{Operation::Vitamin,0,0},hoothoot,4)==Evs{100,0,0,0,0,0}));
    assert((apply({100,100,100,100,100,9},{Operation::Battle,0,0,7,false},hoothoot,4)==Evs{101,100,100,100,100,9}));
    r.target={102,108,0,0,0,0};r.availableExperience=2;r.stateLimit=1;
    proof=solve(r,hoothoot);assert(proof.verdict==Verdict::Incomplete && proof.exhausted);
    r.stateLimit=2048;r.initialEvsKnownZero=false;
    assert(solve(r,hoothoot).verdict==Verdict::Incomplete);
    r.initialEvsKnownZero=true;r.battlesRequireExperience=false;r.availableExperience=0;
    assert(solve(r,hoothoot).verdict==Verdict::Incomplete);
    r.generation=5;assert(solve(r,hoothoot).verdict==Verdict::Incomplete);
    std::cout<<"Joint native EV/EXP: replayable six-stat witnesses,global action-budget negatives,vitamins,berries,255 caps and explicit exhaustion PASS\n";
}
