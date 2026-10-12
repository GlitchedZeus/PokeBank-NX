#include "Legality/Gen3OriginalRoamerStage.h"
#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
int main(){
    namespace R=Legality::Gen3OriginalRoamerStage;
    using S=R::Status;using G=R::Game;using T=R::Starter;
    // Independently replayed 32-bit GBA RNG test fixture pre-PID seeds.
    // These represent ORIGINAL capture-side witnesses, not retail archives.
    struct Fixture {
        G game;T starter;uint8_t tvChoice;uint16_t species;
        uint8_t level,mapGroup,mapNum,route;uint32_t seed,pid;
        std::array<uint8_t,6> ivs;bool truncated;
    };
    constexpr std::array<Fixture,8> fixture{{
        {G::Ruby,T::Unknown,255,381,40,0,0x19,0,
            0x12345678u,0x84EA0B71u,{10,4,0,0,0,0},true},
        {G::Sapphire,T::Unknown,255,380,40,0,0x31,0,
            0x0BADCAFEu,0x309F6B4Du,{10,3,0,0,0,0},true},
        {G::Emerald,T::Unknown,1,381,40,0,0x20,0,
            0x00224466u,0x293D4C52u,{20,21,30,10,18,8},false},
        {G::Emerald,T::Unknown,0,380,40,0,0x21,0,
            0x1234AABBu,0xD0235414u,{30,2,15,4,18,24},false},
        {G::FireRed,T::Bulbasaur,255,244,50,3,0,1,
            0xCAFED00Du,0xF76348F0u,{28,5,0,0,0,0},true},
        {G::LeafGreen,T::Charmander,255,245,50,3,0,15,
            0x31415926u,0xA2F87B60u,{11,5,0,0,0,0},true},
        {G::FireRed,T::Squirtle,255,243,50,3,0,25,
            0x98765432u,0xCC51C8B5u,{25,2,0,0,0,0},true},
        {G::LeafGreen,T::Squirtle,255,243,50,3,0,20,
            0x07070707u,0xC608588Cu,{29,7,0,0,0,0},true}
    }};
    static_assert(R::speciesForOriginalSource(G::FireRed,T::Bulbasaur,0)==244u);
    static_assert(R::speciesForOriginalSource(G::FireRed,T::Charmander,0)==245u);
    static_assert(R::speciesForOriginalSource(G::FireRed,T::Squirtle,0)==243u);
    static_assert(R::speciesForOriginalSource(G::Ruby,T::Unknown,255)==381u);
    static_assert(R::speciesForOriginalSource(G::Sapphire,T::Unknown,255)==380u);
    static_assert(R::speciesForOriginalSource(G::Emerald,T::Unknown,0)==380u);
    static_assert(R::speciesForOriginalSource(G::Emerald,T::Unknown,1)==381u);
    static_assert(R::validRubySapphireOriginalMap(0x19u));
    static_assert(R::validRubySapphireOriginalMap(0x31u));
    static_assert(!R::validRubySapphireOriginalMap(0x1Bu));
    unsigned passed=0;
    for(const auto& f:fixture){
        R::Candidate c{};
        c.originalGame=f.game;c.originalStarter=f.starter;
        c.originalEmeraldTvChoice=f.tvChoice;c.originalSpecies=f.species;
        c.originalLevel=f.level;c.originalMapGroup=f.mapGroup;
        c.originalMapNum=f.mapNum;c.originalKantoRouteNumber=f.route;
        c.originalPrePidSeed=f.seed;c.originalPid=f.pid;c.originalIVs=f.ivs;
        c.originalGameIndependentlyAttested=true;
        c.originalRoamerActivationIndependentlyAttested=true;
        c.originalRoamerChoiceIndependentlyAttested=true;
        c.originalMapContextIndependentlyAttested=true;
        c.originalPidIvAndSeedIndependentlyAttested=true;
        auto positive=R::evaluate(c);
        assert(positive.status==S::ConditionalOriginalRoamerHistory);
        assert(positive.usesTruncatedRoamerIvStorage==f.truncated);
        assert(positive.sourceLevel==f.level);
        assert(positive.sourceSpecies==f.species);
        assert(positive.originalSeed==f.seed);
        auto missing=c;missing.originalGameIndependentlyAttested=false;
        assert(R::evaluate(missing).status==S::UnknownOriginalGame);
        missing=c;missing.originalRoamerActivationIndependentlyAttested=false;
        assert(R::evaluate(missing).status==S::UnknownOriginalRoamerActivation);
        missing=c;missing.originalMapContextIndependentlyAttested=false;
        assert(R::evaluate(missing).status==S::UnknownOriginalRoute);
        missing=c;missing.originalPidIvAndSeedIndependentlyAttested=false;
        assert(R::evaluate(missing).status==S::UnknownOriginalPidIvStage);
        if(f.game==G::Emerald||R::isFrlg(f.game)){
            missing=c;missing.originalRoamerChoiceIndependentlyAttested=false;
            assert(R::evaluate(missing).status==S::UnknownOriginalRoamerSelection);
        }
        auto wrong=c;wrong.originalSpecies^=1u;
        assert(R::evaluate(wrong).status==S::CandidateOriginalSpeciesContradicted);
        wrong=c;wrong.originalLevel++;
        assert(R::evaluate(wrong).status==S::CandidateOriginalLevelContradicted);
        wrong=c;wrong.originalMapGroup=8;
        assert(R::evaluate(wrong).status==S::CandidateOriginalRouteContradicted);
        wrong=c;wrong.originalPid^=1u;
        assert(R::evaluate(wrong).status==S::CandidateOriginalPidIvContradicted);
        wrong=c;wrong.originalIVs[0]^=1u;
        assert(R::evaluate(wrong).status==S::CandidateOriginalPidIvContradicted);
        wrong=c;wrong.originalIVs[0]=32u;
        assert(R::evaluate(wrong).status==S::MalformedOriginalIvs);
        if(R::isFrlg(f.game)){
            wrong=c;wrong.originalKantoRouteNumber=0u;
            assert(R::evaluate(wrong).status==S::CandidateOriginalRouteContradicted);
            wrong=c;wrong.originalStarter=T::Unknown;
            assert(R::evaluate(wrong).status==S::UnknownOriginalRoamerSelection);
        }
        if(f.game==G::Emerald){
            wrong=c;wrong.originalEmeraldTvChoice=255u;
            assert(R::evaluate(wrong).status==S::UnknownOriginalRoamerSelection);
            wrong=c;wrong.originalIVs[3]^=1u;
            assert(R::evaluate(wrong).status==S::CandidateOriginalPidIvContradicted);
        }else{
            wrong=c;wrong.originalIVs[3]=1u;
            assert(R::evaluate(wrong).status==S::CandidateOriginalPidIvContradicted);
        }
        ++passed;
    }
    assert(passed==8u);
    std::cout<<"L1 E06/E08 8 GBA source-attested roamer PID-IV original routes: PASS\n";
}
