// Synthetic retained failures and paired controls; no historical custody claim.
#include "Legality/Gen5Legality.h"
#include "Legality/Gen35BreedingAncestry.h"
#include "Legality/Gen5LegalityData.h"
#include "legality_gen5_test_helpers.h"
#include <algorithm>
#include <array>
#include <cassert>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

int main() {
    using namespace Legality;
    std::ifstream input("tests/fixtures/gen15_session2/gen5_joint_breeding.txt");
    assert(input);
    std::string id,expected,hex;
    size_t cases=0,invalid=0,clean=0,partial=0,pairedUnverified=0;
    const auto bwPairNeedsHistory=[](const Gen5::Pk5& pk) {
        if(!pk.isEgg||(pk.version!=20&&pk.version!=21)||(pk.species!=29&&pk.species!=32&&pk.species!=313&&pk.species!=314))return false;
        std::vector<uint16_t> base;
        for(auto i=Gen5Data::kLevelUpBWStart[pk.species];i<Gen5Data::kLevelUpBWStart[pk.species+1];++i)
            if(Gen5Data::kLevelUpBW[i].level<=1)base.push_back(Gen5Data::kLevelUpBW[i].move);
        if(base.size()>4)base.erase(base.begin(),base.end()-4);
        std::array<uint16_t,4> initial{};std::copy(base.begin(),base.end(),initial.begin());
        return initial!=pk.moves;
    };
    while(input>>id>>expected>>hex) {
        std::vector<uint8_t> bytes;
        for(size_t i=0;i<hex.size();i+=2)bytes.push_back(static_cast<uint8_t>(std::stoul(hex.substr(i,2),nullptr,16)));
        assert(bytes.size()==136);
        uint16_t sum=0;
        for(size_t i=8;i<136;i+=2)sum=static_cast<uint16_t>(sum+bytes[i]+(bytes[i+1]<<8));
        assert(sum==static_cast<uint16_t>(bytes[6]|(bytes[7]<<8)));
        const auto before=bytes;
        const auto report=analyzePk5(bytes,{.encrypted=false});
        if(id=="native-unhatched-initial-only") {Gen5::Pk5 pk;assert(Gen5::read(bytes,pk)&&pk.isEgg);}
        if(id=="pidgey-japanese-order-uncertified") {Gen5::Pk5 pk;assert(Gen5::read(bytes,pk)&&pk.isEgg&&pk.language==1);}
        const auto baseline=expected=="INVALID"?Verdict::Invalid:expected=="INCOMPLETE"?Verdict::Incomplete:Verdict::NoProblemsFound;
        Gen5::Pk5 pk;assert(Gen5::read(bytes,pk));
        // Preserve the frozen baseline text. The new rule is keyed to native
        // BW paired offspring dispatch, whose source has not been replayed.
        // A generic move recipe alone cannot certify that additional family.
        const bool paired=baseline==Verdict::NoProblemsFound&&bwPairNeedsHistory(pk);
        const auto wanted=paired?Verdict::Incomplete:baseline;
        if(paired)++pairedUnverified;
        if(report.verdict()!=wanted){std::cerr<<id<<": wrong verdict\n";return 1;}
        assert(Gen5TestHelpers::consistent(report));
        if(wanted==Verdict::Incomplete)assert(report.coverage.eggBreeding==CoverageLevel::Partial&&report.coverage.moves==CoverageLevel::Partial);
        assert(bytes==before);
        if(baseline==Verdict::Invalid)++invalid;
        else if(baseline==Verdict::Incomplete)++partial;
        else ++clean;
        ++cases;
    }
    assert(input.eof()&&cases==41&&invalid==2&&partial==23&&clean==16);
    assert(pairedUnverified==1);

    // Independent fresh BW paired-family disagreements remain in the raw
    // corpus. A model recipe does not establish native offspring-role dispatch.
    std::ifstream pairedInput("tests/fixtures/gen15_session2/gen5_bw_paired_family.txt");
    assert(pairedInput);size_t pairedCases=0;
    while(pairedInput>>id>>expected>>hex){
        assert(expected=="INCOMPLETE");
        auto bytes=Gen5TestHelpers::fromHex(hex.c_str());const auto before=bytes;
        const auto report=analyzePk5(bytes,{.encrypted=false});
        assert(bytes==before&&report.verdict()==Verdict::Incomplete);
        assert(report.coverage.moves==CoverageLevel::Partial&&report.coverage.eggBreeding==CoverageLevel::Partial);
        assert(Gen5TestHelpers::hasText(report,Severity::Warning,"Black/White paired-species offspring and parental role path"));
        // Retain the same paired-family identity and acquisition, but use only
        // actual BW level-1 moves: no inherited parental role is applicable.
        Gen5::Pk5 pk;assert(Gen5::read(bytes,pk));std::vector<uint16_t> base;
        for(auto i=Gen5Data::kLevelUpBWStart[pk.species];i<Gen5Data::kLevelUpBWStart[pk.species+1];++i)
            if(Gen5Data::kLevelUpBW[i].level<=1)base.push_back(Gen5Data::kLevelUpBW[i].move);
        if(base.size()>4)base.erase(base.begin(),base.end()-4);
        for(size_t i=0;i<4;++i){const uint16_t m=i<base.size()?base[i]:0;bytes[0x28+2*i]=static_cast<uint8_t>(m);bytes[0x29+2*i]=static_cast<uint8_t>(m>>8);bytes[0x30+i]=Gen5Data::kMovePP[m];bytes[0x34+i]=0;}
        Gen5TestHelpers::refreshChecksum(bytes);
        const auto initial=analyzePk5(bytes,{.encrypted=false});
        assert(initial.verdict()==Verdict::NoProblemsFound);
        const auto hatch=[](std::vector<uint8_t> record){
            record[0x3b]&=static_cast<uint8_t>(~0x40u);
            for(size_t i=0;i<11;++i){const char* name="Illumise";const uint16_t c=i<8?static_cast<uint8_t>(name[i]):uint16_t{0xffff};record[0x48+2*i]=static_cast<uint8_t>(c);record[0x49+2*i]=static_cast<uint8_t>(c>>8);}
            std::copy(record.begin()+0x78,record.begin()+0x7b,record.begin()+0x7b);
            record[0x80]=64;record[0x81]=0;
            Gen5TestHelpers::refreshChecksum(record);return record;
        };
        const auto inheritedHatch=analyzePk5(hatch(before),{.encrypted=false});
        assert(inheritedHatch.verdict()==Verdict::Incomplete&&inheritedHatch.coverage.moves==CoverageLevel::Partial&&inheritedHatch.coverage.eggBreeding==CoverageLevel::Partial);
        assert(Gen5TestHelpers::hasText(inheritedHatch,Severity::Warning,"Black/White paired-species offspring and parental role path"));
        auto taught=bytes;
        // Retain the original record's fourth move (Thunder / Aerial Ace),
        // which each specimen can learn from a BW TM after an ordinary hatch.
        const uint16_t tm=static_cast<uint16_t>(before[0x2e]|(before[0x2f]<<8));
        assert(tm==87||tm==332);
        taught[0x2a]=static_cast<uint8_t>(tm);taught[0x2b]=static_cast<uint8_t>(tm>>8);taught[0x31]=Gen5Data::kMovePP[tm];
        const auto teachableHatch=analyzePk5(hatch(taught),{.encrypted=false});
        assert(teachableHatch.verdict()==Verdict::NoProblemsFound&&Gen5TestHelpers::consistent(teachableHatch));
        ++pairedCases;
    }
    assert(pairedInput.eof()&&pairedCases==2);

    // Native adapters share the actual version-specific birth writer. These
    // are joint shared-level recipes, not independent per-move donors.
    for(const auto game:std::array<uint8_t,5>{1,2,3,4,5}) {
        Breeding::Gen35Ancestry ancestry({33,45,73,0});
        assert(ancestry.provenBirth(3,1,game,4,4,true));
        assert(Breeding::Gen35Ancestry::provenOrdinaryBirth(3,1,game,{33,45,73,0},4,4,true));
        assert(ancestry.certifiedBirths().back().generation==3);
    }
    for(const auto game:std::array<uint8_t,5>{7,8,10,11,12}) {
        Breeding::Gen35Ancestry ancestry({33,45,73,0});
        assert(ancestry.provenBirth(4,1,game,6,6,true));
        assert(Breeding::Gen35Ancestry::provenOrdinaryBirth(4,1,game,{33,45,73,0},6,6,true));
        assert(ancestry.certifiedBirths().back().generation==4);
        // A hatched survivor need not occupy its birth slot.
        Breeding::Gen35Ancestry hatched({73,33,45,0});
        assert(hatched.provenBirth(4,1,game,1,1,false));
        assert(Breeding::Gen35Ancestry::provenOrdinaryBirth(4,1,game,{73,33,45,0},1,1,false));
        assert(!hatched.provenBirth(4,1,game,1,1,true));
    }
    {
        Breeding::Gen35Ancestry invalidQuery({33,45,73,0});
        assert(!invalidQuery.provenBirth(3,1,22,4,4,true));
        assert(!invalidQuery.provenBirth(4,650,12,4,4,true));
        assert(!invalidQuery.provenBirth(5,1,22,4,8,true));
    }
    // Ordinary parents do not suffice: both histories need intermediate eggs.
    for(const auto era:std::array<uint8_t,3>{3,4,5}){
        const uint16_t baby=era==3?152:114;
        const uint8_t game=era==3?3:era==4?12:20;
        Breeding::Gen35Ancestry ancestry({175,0,0,0});
        assert(!Breeding::Gen35Ancestry::provenOrdinaryBirth(era,baby,game,{175,0,0,0},1,0,false));
        assert(ancestry.provenHatchedBirth(era,baby,game,1));
        assert(ancestry.certifiedBirths().size()>=2&&ancestry.certifiedBirths().back().baby==baby);
        assert(ancestry.proofStates()<=256);
        std::cout<<"Recursive survivor era="<<unsigned(era)<<" baby="<<baby<<" states="<<ancestry.proofStates()<<" recipes="<<ancestry.certifiedBirths().size()<<'\n';
        if(era<=4){
            const auto games=era==3?std::array<uint8_t,5>{1,2,3,4,5}:std::array<uint8_t,5>{7,8,10,11,12};
            for(auto version:games){
                Breeding::Gen35Ancestry native({175,0,0,0});
                assert(native.provenBirth(era,baby,version,1,0,false));
                assert(native.certifiedBirths().size()>=2);
            }
        }
        Breeding::Gen35Ancestry bounded({175,0,0,0},0);
        assert(!bounded.provenHatchedBirth(era,baby,game,1)&&bounded.proofExhausted());
        assert(bounded.certifiedBirths().empty());
    }
    {
        // The independent frozen b3184 record needs Sewaddle -> Paras ->
        // Grass-group carrier -> Tangela, then Bind and Grass Knot on one dad.
        Breeding::Gen35Ancestry ancestry({132,175,20,447});
        assert(!Breeding::Gen35Ancestry::provenOrdinaryBirth(5,114,20,{132,175,20,447},14,4,true));
        assert(ancestry.provenBirth(5,114,20,14,4,true));
        assert(ancestry.certifiedBirths().size()>=3);
        assert((ancestry.certifiedBirths().back().bornMoves==std::array<uint16_t,4>{132,175,20,447}));
        std::ifstream tangelaInput("tests/fixtures/gen15_session2/gen5_recursive_tangela.txt");
        assert(tangelaInput>>id>>expected>>hex);assert(expected=="NOPROBLEMS");
        auto bytes=Gen5TestHelpers::fromHex(hex.c_str());const auto before=bytes;
        assert(analyzePk5(bytes,{.encrypted=false}).verdict()==Verdict::NoProblemsFound&&bytes==before);
        // Hatch the retained model specimen at permitted BW location 64. No source move is
        // invented; it retains the same inherited quartet and original OT.
        bytes[0x3b]&=static_cast<uint8_t>(~0x40u);
        for(size_t i=0;i<11;++i){const char* name="Tangela";const uint16_t c=i<7?static_cast<uint8_t>(name[i]):uint16_t{0xffff};bytes[0x48+2*i]=static_cast<uint8_t>(c);bytes[0x49+2*i]=static_cast<uint8_t>(c>>8);}
        std::copy(bytes.begin()+0x78,bytes.begin()+0x7b,bytes.begin()+0x7b);
        assert(Gen5::validHatchLocation(64,20));
        bytes[0x80]=64;bytes[0x81]=0;
        Gen5TestHelpers::refreshChecksum(bytes);
        const auto hatched=analyzePk5(bytes,{.encrypted=false});
        if(hatched.verdict()!=Verdict::NoProblemsFound)for(const auto& issue:hatched.issues)std::cerr<<"Tangela hatched: "<<issue.text<<"\n";
        assert(hatched.verdict()==Verdict::NoProblemsFound&&Gen5TestHelpers::consistent(hatched));
    }

    // Construct actual four-slot parent/birth recipes, beyond a union of donors.
    struct Proof {std::array<uint16_t,4> moves;uint16_t baby;uint8_t version,father,shared;};
    const std::array<Proof,7> proofs{{
        {{1,45,118,516},113,22,12,8},
        {{1,357,113,263},238,23,14,0},
        {{33,227,343,0},314,21,6,4},
        {{28,226,404,86},632,23,14,0},
        {{4,43,306,263},115,22,12,0},
        {{33,97,17,0},16,23,6,6},
        // Retained egg-b11015: alternate recursive searches must not exhaust
        // the shared budget before its already constructive incense route.
        {{316,33,363,256},446,20,12,8},
    }};
    bool intermediateBirth=false;
    for(const auto& p:proofs) {
        Breeding::Gen35Ancestry ancestry(p.moves);
        assert(ancestry.possible(5,p.baby,p.father,p.shared));
        assert(ancestry.provenBirth5(p.baby,p.version,p.father,p.shared));
        const auto& recipes=ancestry.certifiedBirths();
        assert(!recipes.empty());
        assert(recipes.back().baby==p.baby&&recipes.back().version==p.version&&recipes.back().bornMoves==p.moves);
        for(const auto& recipe:recipes) {
            assert(recipe.father&&recipe.mother);
            assert(std::any_of(recipe.fatherMoves.begin(),recipe.fatherMoves.end(),[](auto m){return m!=0;}));
            assert(std::any_of(recipe.motherMoves.begin(),recipe.motherMoves.end(),[](auto m){return m!=0;}));
            std::cout<<"Recipe target="<<p.baby<<" generation="<<unsigned(recipe.generation)<<" version="<<unsigned(recipe.version)<<" baby="<<recipe.baby<<" father="<<recipe.father<<" mother="<<recipe.mother<<" dad=";
            for(auto m:recipe.fatherMoves)std::cout<<m<<',';
            std::cout<<" mom=";for(auto m:recipe.motherMoves)std::cout<<m<<',';
            std::cout<<" born=";for(auto m:recipe.bornMoves)std::cout<<m<<',';
            std::cout<<'\n';
        }
        if(p.baby==632)intermediateBirth=recipes.size()>1;
    }
    assert(intermediateBirth);
    // B2W2 source evidence must not silently certify an order-dependent BW
    // history. A missing certificate still does not prove an impossible birth.
    Breeding::Gen35Ancestry bwUnverified({33,17,97,0});
    assert(!bwUnverified.provenBirth5(16,20,6,6));
    // The release accepts this order, but our exact forward witness cannot yet
    // reproduce it. Search failure must remain Incomplete, never Invalid.
    Breeding::Gen35Ancestry unverified({118,516,1,45});
    assert(unverified.possible(5,113,15,14));
    assert(!unverified.provenBirth5(113,22,15,14));
    // One known family carrier is a necessary relaxation of uncertified role
    // selection, never a clean certificate or a union of separate parents.
    Breeding::Gen35Ancestry roles({4,43,306,526});
    assert(!roles.possible(5,115,12,0));
    assert(roles.possibleWithUncertifiedParentRole5(115,12));
    assert(!roles.provenBirth5(115,22,12,0));
    Breeding::Gen35Ancestry exclusive({1,357,252,263});
    assert(!exclusive.possible(5,238,14,0));
    assert(!exclusive.possibleWithUncertifiedParentRole5(238,14));
    std::cout<<"Joint Gen III-V ancestry / Gen V adapter:41 frozen records,recursive native III/IV/V births and bound controls,Tangela exact/hatched controls,1 newly uncertified BW paired family,2 fresh paired-family regressions with initial-only clean controls,7 retained Incomplete,exclusive-paternal negative,parent-role uncertainty,native B2W2 shared-order pair,Japanese guard and constructive intermediate births PASS\n";
}
