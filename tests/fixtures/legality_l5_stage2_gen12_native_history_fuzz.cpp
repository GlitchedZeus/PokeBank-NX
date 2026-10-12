// L5 Stage 2 independent adversarial QA against exact production Gen12TimeCapsuleEvidence.h.
// Strictly candidate-scoped; does not run Legality::analyze or prove a retail transfer.
#include "Legality/Gen12TimeCapsuleEvidence.h"
#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
#ifdef NDEBUG
#error "L5 Stage2 checks require enabled assertions"
#endif

namespace T = Legality::Gen12TimeCapsule;
namespace {
constexpr std::array<uint16_t,11> evolutions{
    169,182,186,196,197,199,208,212,230,233,242};
constexpr bool independentlyPossible(uint16_t species) {
    if(species>=1 && species<=151) return true;
    for(auto v:evolutions) if(species==v) return true;
    return false;
}
constexpr bool selfTest() {
    if(T::couldOriginateGen1(196,true,0))return false;
    if(T::couldOriginateGen1(196,false,1))return false;
    if(!T::couldOriginateGen1(196,false,0))return false;
    if(T::couldOriginateGen1(152,false,0))return false;
    if(T::classifyCurrentTrade(151,false,{0,1,165,0})!=T::CurrentTrade::Compatible)return false;
    if(T::classifyCurrentTrade(151,false,{0,166,0,0})!=T::CurrentTrade::MoveNotInGen1)return false;
    if(T::classifyCurrentTrade(151,false,{0,252,0,0})!=T::CurrentTrade::InvalidGen2MoveId)return false;
    if(T::classifyCurrentTrade(151,true,{0,252,0,0})!=T::CurrentTrade::EggNotTradeable)return false;
    if(T::classifyCurrentTrade(196,false,{0,0,0,0})!=T::CurrentTrade::SpeciesNotInGen1)return false;
    return true;
}
static_assert(selfTest());
uint32_t next(uint32_t& seed) {seed=seed*1664525u+1013904223u;return seed;}
}
int main() {
    uint64_t checks=0, potential=0, incompatible=0, malformed=0;
    for(uint32_t raw=0;raw<=65535;++raw) {
        const auto species=static_cast<uint16_t>(raw);
        const bool expected=independentlyPossible(species);
        assert(T::canVisitGen1Species(species)==expected);
        const auto e=T::classifyConditionalStage(species,false,0,{165,166,251,252});
        assert(e.possibleGen1Ancestor==expected);
        assert(e.gen2MoveCount==(expected?2:0));
        if(expected) ++potential;
        for(bool egg : {false,true}) {
            for(uint16_t caught : {0u,1u,0xFFFFu}) {
                assert(T::couldOriginateGen1(species,egg,caught)==(expected&&!egg&&!caught));
                checks+=1;
            }
        }
        checks+=3;
    }
    uint32_t seed=0x7a613b2du;
    for(uint32_t i=0;i<131072;++i) {
        const auto species=static_cast<uint16_t>(next(seed)%257);
        const bool egg=(i%11)==0;
        const uint16_t caught=(i%17)==0?0u:static_cast<uint16_t>(next(seed));
        std::array<uint16_t,4> moves{};
        for(auto& m:moves)m=static_cast<uint16_t>(next(seed)%260);
        const auto original=moves;
        const auto current=T::classifyCurrentTrade(species,egg,moves);
        const auto historical=T::classifyConditionalStage(species,egg,caught,moves);
        const bool gen1Species=species>=1&&species<=151;
        bool malformedMove=false,newMove=false;
        for(auto m:moves){malformedMove|=m>251;newMove|=m>165;}
        const auto expected=egg?T::CurrentTrade::EggNotTradeable:
            !gen1Species?T::CurrentTrade::SpeciesNotInGen1:
            malformedMove?T::CurrentTrade::InvalidGen2MoveId:
            newMove?T::CurrentTrade::MoveNotInGen1:T::CurrentTrade::Compatible;
        assert(current==expected);
        assert(T::canCurrentlyTradeToGen1(species,egg,moves)==(expected==T::CurrentTrade::Compatible));
        const bool historicalPossible=independentlyPossible(species)&&!egg&&caught==0;
        assert(historical.possibleGen1Ancestor==historicalPossible);
        assert(historical.laterEvolutionRequired==
            (historicalPossible && species>151));
        unsigned count=0;for(auto m:moves)if(m>=166&&m<=251)++count;
        assert(historical.gen2MoveCount==(historicalPossible?count:0));
        if(historicalPossible&&newMove&&!malformedMove)++incompatible;
        if(malformedMove)++malformed;
        assert(moves==original); // Helper reads no source fields destructively.
        checks+=7;
    }
    assert(potential==162);
    assert(checks==1507328u);
    assert(incompatible>0 && malformed>0);
    std::cout<<"L5 Stage2 exact production Gen I/II Time Capsule helper: "<<checks
             <<" PASS; source-capable species="<<potential
             <<"; conditional later-move="<<incompatible
             <<"; malformed move inputs="<<malformed
             <<" (not retail histories)\n";
}
