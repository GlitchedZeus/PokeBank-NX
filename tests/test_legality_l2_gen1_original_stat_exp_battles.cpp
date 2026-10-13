#include "Legality/Gen1OriginalStatExpBattleHistory.h"
#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
namespace L=Legality::Gen1OriginalStatExpBattleHistory;
namespace G=PokeVault::Integration::Gen1;
namespace Ref {
#include "../src/Integration/Gen1/Gen1PersonalData.inc"
}
static constexpr std::uint16_t saturation(unsigned a,unsigned b) noexcept {
    return static_cast<std::uint16_t>(a+b>65535u?65535u:a+b);
}
static void store(G::PokemonRecord& r,const std::array<std::uint16_t,5>& values){
    r.statExperience=values;
    for(unsigned i=0;i<5;++i){
        r.rawBody[0x11+2*i]=static_cast<std::uint8_t>(values[i]>>8u);
        r.rawBody[0x12+2*i]=static_cast<std::uint8_t>(values[i]);
    }
}
static G::PokemonRecord specimen(bool party){
    G::PokemonRecord r{};
    r.partyRecord=party;r.rawBodySize=party?44:33;
    r.species=1;r.rawSpecies=0x99;r.rawBody[0]=0x99;
    return r;
}
static L::History evidence(G::SourceGame game,unsigned enemy,
                            const std::array<std::uint16_t,5>& before){
    L::History h{};
    h.strictPK1IndependentlyVerified=true;
    h.originalInitialSnapshotIndependentlyAttested=true;
    h.allOriginalStatExpEventsIndependentlyEnumerated=true;
    h.sourceGame=game;h.originalBeforeStatExp=before;h.count=1;
    auto& e=h.events[0];
    e.originalEventId=10;e.originalMonotonicEventSequence=100;
    e.originalGame=game;
    e.defeatedNationalSpecies=static_cast<std::uint16_t>(enemy);
    e.originalDefeatIndependentlyAttested=true;
    e.singleEligibleRecipientIndependentlyAttested=true;
    e.expAllAbsentIndependentlyAttested=true;
    e.sourceSpeciesIdentityIndependentlyAttested=true;
    e.originalOrderingIndependentlyAttested=true;
    e.originalAfterStatExpIndependentlyAttested=true;
    for(unsigned i=0;i<5;++i)
        e.originalAfterStatExp[i]=saturation(before[i],Ref::kPersonal[enemy][i]);
    return h;
}
int main(){
    std::uint64_t assertions=0;
    // Independent named golden rows from original PKSM-Core personal1:
    assert((std::array<std::uint8_t,5>{Ref::kPersonal[1][0],Ref::kPersonal[1][1],
        Ref::kPersonal[1][2],Ref::kPersonal[1][3],Ref::kPersonal[1][4]}==
        std::array<std::uint8_t,5>{45,49,49,45,65}));++assertions;
    assert((std::array<std::uint8_t,5>{Ref::kPersonal[25][0],Ref::kPersonal[25][1],
        Ref::kPersonal[25][2],Ref::kPersonal[25][3],Ref::kPersonal[25][4]}==
        std::array<std::uint8_t,5>{35,55,30,90,50}));++assertions;
    assert((std::array<std::uint8_t,5>{Ref::kPersonal[151][0],Ref::kPersonal[151][1],
        Ref::kPersonal[151][2],Ref::kPersonal[151][3],Ref::kPersonal[151][4]}==
        std::array<std::uint8_t,5>{100,100,100,100,100}));++assertions;
    for(unsigned species=1;species<=151;++species)
    for(unsigned game=0;game<=2;++game)
    for(bool party:{false,true})
    for(auto seed:{0u,1u,255u,256u,25599u,65025u,65534u,65535u}){
        auto r=specimen(party);
        const auto before=std::array<std::uint16_t,5>{
            static_cast<std::uint16_t>(seed),1,65025,65534,65535};
        auto h=evidence(static_cast<G::SourceGame>(game),species,before);
        store(r,h.events[0].originalAfterStatExp);
        const auto result=L::evaluate(r,h);
        assert(result.status==L::Status::ConditionalOriginalBattleHistory);
        assert(result.eventsReplayed==1 && result.firstContradictionEvent==0);
        assert(result.replayed==r.statExperience);assertions+=3;
        auto z=h;z.events[0].expAllAbsentIndependentlyAttested=false;
        assert(L::evaluate(r,z).status==L::Status::UnknownOriginalTimeline);++assertions;
        z=h;z.events[0].singleEligibleRecipientIndependentlyAttested=false;
        assert(L::evaluate(r,z).status==L::Status::UnknownOriginalTimeline);++assertions;
        z=h;z.events[0].sourceSpeciesIdentityIndependentlyAttested=false;
        assert(L::evaluate(r,z).status==L::Status::UnknownOriginalTimeline);++assertions;
        z=h;z.allOriginalStatExpEventsIndependentlyEnumerated=false;
        assert(L::evaluate(r,z).status==L::Status::IncompleteEventCoverage);++assertions;
        z=h;z.originalInitialSnapshotIndependentlyAttested=false;
        assert(L::evaluate(r,z).status==L::Status::UnknownOriginalTimeline);++assertions;
        z=h;z.strictPK1IndependentlyVerified=false;
        assert(L::evaluate(r,z).status==L::Status::UnknownSource);++assertions;
        z=h;z.events[0].originalAfterStatExp[0]^=1u;
        assert(L::evaluate(r,z).status==L::Status::ContradictedAttestedEvent);
        assert(L::evaluate(r,z).firstContradictionEvent==1);assertions+=2;
        z=h;z.count=2;
        z.events[1]=z.events[0];
        assert(L::evaluate(r,z).status==L::Status::UnknownOriginalTimeline);++assertions;
        z=h;z.count=17;
        assert(L::evaluate(r,z).status==L::Status::UnknownOriginalTimeline);++assertions;
        z=h;z.events[0].originalGame=static_cast<G::SourceGame>((game+1)%3);
        assert(L::evaluate(r,z).status==L::Status::UnknownOriginalTimeline);++assertions;
        auto corrupt=r;corrupt.statExperience[0]^=1u;
        assert(L::evaluate(corrupt,h).status==L::Status::MalformedAttestedRecord);++assertions;
        corrupt=r;corrupt.rawBodySize=party?33:44;
        assert(L::evaluate(corrupt,h).status==L::Status::MalformedAttestedRecord);++assertions;
        // Independently witnessed COMPLETE two-battle history.
        z=h;z.count=2;
        z.events[1]=z.events[0];
        z.events[1].originalEventId=11;
        z.events[1].originalMonotonicEventSequence=101;
        for(unsigned i=0;i<5;++i)
            z.events[1].originalAfterStatExp[i]=saturation(
                z.events[0].originalAfterStatExp[i],Ref::kPersonal[species][i]);
        store(r,z.events[1].originalAfterStatExp);
        const auto two=L::evaluate(r,z);
        assert(two.status==L::Status::ConditionalOriginalBattleHistory);
        assert(two.eventsReplayed==2);assertions+=2;
        // A correct individual witnessed event must not prove a COMPLETE
        // history when other events are unobserved.
        z.allOriginalStatExpEventsIndependentlyEnumerated=false;
        assert(L::evaluate(r,z).status==L::Status::IncompleteEventCoverage);++assertions;
    }
    std::cout<<"Gen I original source-attested solo-battle StatExp: "
             <<assertions<<" assertions PASS\n";
}
