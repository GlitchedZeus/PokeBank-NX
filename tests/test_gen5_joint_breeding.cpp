// Synthetic retained failures and paired controls; no historical custody claim.
#include "Legality/Gen5Legality.h"
#include "Legality/Gen35BreedingAncestry.h"
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
    size_t cases=0,invalid=0,clean=0,partial=0;
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
        const auto wanted=expected=="INVALID"?Verdict::Invalid:expected=="INCOMPLETE"?Verdict::Incomplete:Verdict::NoProblemsFound;
        if(report.verdict()!=wanted){std::cerr<<id<<": wrong verdict\n";return 1;}
        assert(Gen5TestHelpers::consistent(report));
        if(wanted==Verdict::Incomplete)assert(report.coverage.eggBreeding==CoverageLevel::Partial&&report.coverage.moves==CoverageLevel::Partial);
        assert(bytes==before);
        if(wanted==Verdict::Invalid)++invalid;
        else if(wanted==Verdict::Incomplete)++partial;
        else ++clean;
        ++cases;
    }
    assert(input.eof()&&cases==41&&invalid==2&&partial==23&&clean==16);

    // Construct actual four-slot parent/birth recipes, beyond a union of donors.
    struct Proof {std::array<uint16_t,4> moves;uint16_t baby;uint8_t version,father,shared;};
    const std::array<Proof,6> proofs{{
        {{1,45,118,516},113,22,12,8},
        {{1,357,113,263},238,23,14,0},
        {{33,227,343,0},314,21,6,4},
        {{28,226,404,86},632,23,14,0},
        {{4,43,306,263},115,22,12,0},
        {{33,97,17,0},16,23,6,6},
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
    std::cout<<"Joint Gen III-V ancestry / Gen V adapter:41 records,7 retained Incomplete,exclusive-paternal negative,parent-role uncertainty,native B2W2 shared-order pair,Japanese guard and constructive intermediate births PASS\n";
}
