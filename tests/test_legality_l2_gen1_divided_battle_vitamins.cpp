#include "Legality/Gen1OriginalStatExpBattleHistory.h"
#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
namespace L=Legality::Gen1OriginalStatExpBattleHistory;
namespace G=PokeVault::Integration::Gen1;
namespace Original {
#include "../src/Integration/Gen1/Gen1PersonalData.inc"
}
static unsigned checks=0;
static void store(G::PokemonRecord& p,const std::array<std::uint16_t,5>& stats){
    p.statExperience=stats;
    for(unsigned i=0;i<5;++i){
        p.rawBody[0x11+2*i]=static_cast<std::uint8_t>(stats[i]>>8u);
        p.rawBody[0x12+2*i]=static_cast<std::uint8_t>(stats[i]);
    }
}
static G::PokemonRecord specimen(const std::array<std::uint16_t,5>& stats,bool party){
    G::PokemonRecord p{};p.partyRecord=party;p.rawBodySize=party?44:33;
    p.rawBody[0]=p.rawSpecies=0x99;p.species=1;
    store(p,stats);return p;
}
static std::uint16_t saturated(unsigned prior,unsigned gain){
    return static_cast<std::uint16_t>(prior+gain>65535?65535:prior+gain);
}
static L::History common(G::SourceGame game,const std::array<std::uint16_t,5>& before){
    L::History h{};h.sourceGame=game;h.originalBeforeStatExp=before;
    h.strictPK1IndependentlyVerified=true;
    h.originalInitialSnapshotIndependentlyAttested=true;
    h.allOriginalStatExpEventsIndependentlyEnumerated=true;
    h.count=1;
    auto& e=h.events[0];e.kind=L::EventKind::Battle;
    e.originalEventId=1234;e.originalMonotonicEventSequence=42;
    e.originalGame=game;e.originalOrderingIndependentlyAttested=true;
    e.originalAfterStatExpIndependentlyAttested=true;
    return h;
}
int main(){
    static_assert(sizeof(std::uint16_t)==2);
    for(unsigned species=1;species<=151;++species)
      for(unsigned game=0;game<3;++game)
        for(unsigned recipients=1;recipients<=6;++recipients)
          for(unsigned prior:{0u,1u,255u,25000u,65530u,65535u}) {
            std::array<std::uint16_t,5> before{
                static_cast<std::uint16_t>(prior),
                static_cast<std::uint16_t>(prior/2),
                65025,1,65535};
            auto h=common(static_cast<G::SourceGame>(game),before);
            auto& e=h.events[0];
            e.defeatedNationalSpecies=static_cast<std::uint16_t>(species);
            e.originalDefeatIndependentlyAttested=true;
            e.sourceSpeciesIdentityIndependentlyAttested=true;
            e.expAllAbsentIndependentlyAttested=true;
            e.originalEligibleRecipientCount=static_cast<std::uint8_t>(recipients);
            e.originalRecipientCountIndependentlyAttested=true;
            e.singleEligibleRecipientIndependentlyAttested=recipients==1;
            std::array<std::uint16_t,5> after=before;
            for(unsigned i=0;i<5;++i)after[i]=saturated(
                before[i],Original::kPersonal[species][i]/recipients);
            e.originalAfterStatExp=after;
            auto p=specimen(after,game==2);
            auto result=L::evaluate(p,h);
            assert(result.status==L::Status::ConditionalOriginalBattleHistory);
            assert(result.replayed==after && result.eventsReplayed==1);
            checks+=2;
            auto changed=h;
            changed.events[0].originalRecipientCountIndependentlyAttested=false;
            changed.events[0].singleEligibleRecipientIndependentlyAttested=false;
            assert(L::evaluate(p,changed).status==L::Status::UnknownOriginalTimeline);++checks;
            changed=h;changed.events[0].originalEligibleRecipientCount=7;
            assert(L::evaluate(p,changed).status==L::Status::UnknownOriginalTimeline);++checks;
            changed=h;changed.events[0].originalEligibleRecipientCount=0;
            assert(L::evaluate(p,changed).status==L::Status::UnknownOriginalTimeline);++checks;
            changed=h;changed.events[0].expAllAbsentIndependentlyAttested=false;
            assert(L::evaluate(p,changed).status==L::Status::UnknownOriginalTimeline);++checks;
            changed=h;changed.allOriginalStatExpEventsIndependentlyEnumerated=false;
            assert(L::evaluate(p,changed).status==L::Status::IncompleteEventCoverage);++checks;
            changed=h;changed.events[0].originalAfterStatExpIndependentlyAttested=false;
            assert(L::evaluate(p,changed).status==L::Status::UnknownOriginalTimeline);++checks;
            changed=h;changed.events[0].originalRecipientCountIndependentlyAttested=false;
            if(recipients==1){
                assert(L::evaluate(p,changed).status==L::Status::ConditionalOriginalBattleHistory);
            }else{
                assert(L::evaluate(p,changed).status==L::Status::UnknownOriginalTimeline);
            }
            ++checks;
            changed=h;changed.events[0].originalAfterStatExp[0]^=1;
            assert(L::evaluate(p,changed).status==L::Status::ContradictedAttestedEvent);
            ++checks;
          }
    for(unsigned game=0;game<3;++game)
      for(unsigned kind=1;kind<=5;++kind)
        for(unsigned beforeValue:{0u,1u,255u,256u,25599u,25600u,30000u,65535u}) {
          std::array<std::uint16_t,5> before{100,200,300,400,500};
          const unsigned slot=kind-1;
          before[slot]=static_cast<std::uint16_t>(beforeValue);
          auto h=common(static_cast<G::SourceGame>(game),before);
          auto& e=h.events[0];
          e.kind=static_cast<L::EventKind>(kind);
          e.originalVitaminItemIndependentlyAttested=true;
          e.originalVitaminConsumedIndependentlyAttested=true;
          auto after=before;
          const bool usable=beforeValue<25600;
          if(usable)after[slot]=static_cast<std::uint16_t>(beforeValue+2560);
          e.originalAfterStatExp=after;
          auto p=specimen(after,false);
          auto result=L::evaluate(p,h);
          assert(result.status==(usable?L::Status::ConditionalOriginalBattleHistory:
                                      L::Status::ContradictedAttestedEvent));++checks;
          if(usable){
              assert((result.replayed[slot]&255u)==(beforeValue&255u));++checks;
          }
          auto altered=h;altered.events[0].originalVitaminItemIndependentlyAttested=false;
          assert(L::evaluate(p,altered).status==L::Status::UnknownOriginalTimeline);++checks;
          altered=h;altered.events[0].originalVitaminConsumedIndependentlyAttested=false;
          assert(L::evaluate(p,altered).status==L::Status::UnknownOriginalTimeline);++checks;
          altered=h;altered.events[0].kind=static_cast<L::EventKind>(255);
          assert(L::evaluate(p,altered).status==L::Status::UnknownOriginalTimeline);++checks;
          altered=h;altered.events[0].originalMonotonicEventSequence=0;
          assert(L::evaluate(p,altered).status==L::Status::UnknownOriginalTimeline);++checks;
          altered=h;altered.events[0].originalGame=static_cast<G::SourceGame>((game+1)%3);
          assert(L::evaluate(p,altered).status==L::Status::UnknownOriginalTimeline);++checks;
        }
    // Mixed chronology: two independently attested battle recipient counts,
    // then a successful vitamin. A forged duplicate ID or backwards
    // sequence must fail closed instead of forming false source certainty.
    for(unsigned game=0;game<3;++game){
        auto h=common(static_cast<G::SourceGame>(game),{0,0,0,0,0});
        h.count=3;
        auto& a=h.events[0];a.defeatedNationalSpecies=25;
        a.originalDefeatIndependentlyAttested=true;
        a.sourceSpeciesIdentityIndependentlyAttested=true;
        a.expAllAbsentIndependentlyAttested=true;
        a.originalRecipientCountIndependentlyAttested=true;
        a.originalEligibleRecipientCount=2;
        a.originalAfterStatExp={17,27,15,45,25};
        auto& b=h.events[1];b=a;b.originalEventId=1235;
        b.originalMonotonicEventSequence=43;
        b.defeatedNationalSpecies=151;
        b.originalEligibleRecipientCount=5;
        b.originalAfterStatExp={37,47,35,65,45};
        auto& c=h.events[2];c=a;c.originalEventId=1236;
        c.originalMonotonicEventSequence=44;
        c.kind=L::EventKind::Calcium;
        c.originalVitaminItemIndependentlyAttested=true;
        c.originalVitaminConsumedIndependentlyAttested=true;
        c.originalAfterStatExp={37,47,35,65,2605};
        auto p=specimen(c.originalAfterStatExp,true);
        assert(L::evaluate(p,h).status==L::Status::ConditionalOriginalBattleHistory);++checks;
        auto duplicate=h;duplicate.events[2].originalEventId=1235;
        assert(L::evaluate(p,duplicate).status==L::Status::UnknownOriginalTimeline);++checks;
        duplicate=h;duplicate.events[2].originalMonotonicEventSequence=41;
        assert(L::evaluate(p,duplicate).status==L::Status::UnknownOriginalTimeline);++checks;
        duplicate=h;duplicate.count=17;
        assert(L::evaluate(p,duplicate).status==L::Status::UnknownOriginalTimeline);++checks;
        duplicate=h;duplicate.events[1].originalAfterStatExp[3]^=1u;
        assert(L::evaluate(p,duplicate).status==L::Status::ContradictedAttestedEvent);++checks;
    }
    std::cout<<"Gen I original divided battle and five vitamin sources: "<<checks<<" checks PASS\n";
}
