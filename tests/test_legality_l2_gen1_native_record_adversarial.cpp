#include <cassert>
// Exact 0x21 boxed and 0x2C party PK1 payloads, not Gen III PKM.
#define main l2_gen1_dv_source_matrix_entry
#include "test_legality_l2_gen1_native_dv_stat.cpp"
#undef main
#include <cassert>
#include <iostream>
int main(){
    std::uint64_t assertions=0;
    constexpr L::SourceContext fresh{true,true,true,true};
    constexpr L::SourceContext ambiguous{true,false,false,true};
    const std::array<std::uint8_t,4> dv{8,13,5,9};
    const std::array<std::uint16_t,5> exp{22850,23140,17280,24795,19625};
    auto pikachu=specimen(true,81,dv,exp);
    pikachu.rawSpecies=pikachu.rawBody[0]=0x54;
    pikachu.species=25;
    const auto correct=G::StagedPokemonEditor::calculateBattleStats(25,81,dv,exp);
    assert((correct.asArray()==std::array<std::uint16_t,5>{189,137,101,190,128}));
    ++assertions;
    pikachu.maxHP=correct.hp;pikachu.attack=correct.attack;
    pikachu.defense=correct.defense;pikachu.speed=correct.speed;
    pikachu.special=correct.special;
    const auto stats=correct.asArray();
    for(unsigned i=0;i<5;++i)be16(pikachu.rawBody,0x22+2*i,stats[i]);
    pikachu.currentHP=189;be16(pikachu.rawBody,1,pikachu.currentHP);
    assert(L::evaluate(pikachu,fresh).status==L::Status::ConditionalPartyStatMatch);++assertions;
    auto stale=pikachu;stale.attack+=1;be16(stale.rawBody,0x24,stale.attack);
    assert(L::evaluate(stale,ambiguous).status==L::Status::UnresolvedPartyCache);++assertions;
    assert(L::evaluate(stale,fresh).status==L::Status::ContradictedAttestedPartyStats);++assertions;
    auto over=pikachu;over.currentHP=200;be16(over.rawBody,1,200);
    assert(L::evaluate(over,ambiguous).status==L::Status::UnresolvedPartyCache);++assertions;
    assert(L::evaluate(over,fresh).status==L::Status::ContradictedAttestedPartyStats);++assertions;
    auto boxLevel=pikachu;boxLevel.rawBody[3]=23; // party level lives at 0x21, box at 0x03
    auto level=L::evaluate(boxLevel,fresh);
    assert(level.status==L::Status::ConditionalPartyStatMatch);
    assert(level.boxLevelDiffersFromPartyLevel);assertions+=2;
    auto box=specimen(false,100,dv,{65535,65535,65535,65535,65535});
    assert(L::evaluate(box,fresh).status==L::Status::ConditionalNativeFields);++assertions;
    // Field-by-field corruption: normalized read-only wrappers must not
    // disagree with their exact packed source; no out-of-bound accesses.
    for(unsigned i=0;i<5;++i){
        auto x=box;x.statExperience[i]^=0x1u;
        assert(L::evaluate(x,fresh).status==L::Status::InvalidNativeRecord);++assertions;
    }
    for(unsigned i=0;i<5;++i){
        auto x=box;x.dvs[i]^=0x1u;
        assert(L::evaluate(x,fresh).status==L::Status::InvalidNativeRecord);++assertions;
    }
    for(unsigned i=1;i<5;++i){
        auto invalidDV=box;invalidDV.dvs[i]=16;
        assert(L::evaluate(invalidDV,fresh).status==L::Status::InvalidNativeRecord);
        ++assertions;
        invalidDV=box;invalidDV.dvs[i]=255;
        assert(L::evaluate(invalidDV,fresh).status==L::Status::InvalidNativeRecord);
        ++assertions;
    }
    // Five 16-bit Stat Experience values of 65535 are fully valid;
    // there is no sixth Special Defense EV in the native record.
    assert(L::evaluate(box,fresh).status==L::Status::ConditionalNativeFields);
    ++assertions;
    for(unsigned i=0;i<4;++i){
        auto x=box;x.ppUps[i]=static_cast<std::uint8_t>(4u);
        assert(L::evaluate(x,fresh).status==L::Status::InvalidNativeRecord);++assertions;
    }
    for(unsigned i=0;i<4;++i){
        auto x=box;x.moves[i]=167;
        assert(L::evaluate(x,fresh).status==L::Status::InvalidNativeRecord);++assertions;
    }
    for(unsigned i=0;i<4;++i){
        auto x=box;x.pp[i]=64;
        assert(L::evaluate(x,fresh).status==L::Status::InvalidNativeRecord);++assertions;
    }
    for(unsigned i=0;i<4;++i){
        auto x=box;x.rawBody[0x1D+i]^=0x40;
        assert(L::evaluate(x,fresh).status==L::Status::InvalidNativeRecord);++assertions;
    }
    for(unsigned i=0;i<2;++i){
        auto x=box;x.rawBody[0x1B+i]^=0x80;
        assert(L::evaluate(x,fresh).status==L::Status::InvalidNativeRecord);++assertions;
    }
    auto wrong=box;wrong.rawSpecies=0;
    assert(L::evaluate(wrong,fresh).status==L::Status::InvalidNativeRecord);++assertions;
    wrong=box;wrong.species=152;
    assert(L::evaluate(wrong,fresh).status==L::Status::InvalidNativeRecord);++assertions;
    wrong=box;wrong.experience=0x1000000u;
    assert(L::evaluate(wrong,fresh).status==L::Status::InvalidNativeRecord);++assertions;
    wrong=box;wrong.trainerId++;
    assert(L::evaluate(wrong,fresh).status==L::Status::InvalidNativeRecord);++assertions;
    wrong=box;wrong.rawBodySize=32;
    assert(L::evaluate(wrong,fresh).status==L::Status::InvalidNativeRecord);++assertions;
    wrong=box;wrong.partyRecord=true;
    assert(L::evaluate(wrong,fresh).status==L::Status::InvalidNativeRecord);++assertions;
    wrong=box;wrong.level=101;wrong.rawBody[3]=101;
    assert(L::evaluate(wrong,fresh).status==L::Status::InvalidNativeRecord);++assertions;
    wrong=box;wrong.level=0;wrong.rawBody[3]=0;
    assert(L::evaluate(wrong,fresh).status==L::Status::UnknownLevelContext);++assertions;
    // Neither a lack of modern held item, nature, sex nor ability exists
    // as a negative Gen I source field. The record type has none of them.
    assert(L::evaluate(box,{true,false,false,false}).status==L::Status::UnknownLevelContext);
    ++assertions;
    // This validator never mutates the raw body under any evaluation.
    auto immutable=pikachu;const auto original=immutable.rawBody;
    (void)L::evaluate(immutable,fresh);
    assert(immutable.rawBody==original);++assertions;
    std::cout<<"L2 Gen I adversarial PK1 source fields: "<<assertions<<" PASS\n";
}
