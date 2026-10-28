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
    assert(input.eof()&&cases==33&&invalid==4&&partial==16&&clean==13);

    // Construct actual four-slot parent/birth recipes, beyond a union of donors.
    struct Proof {std::array<uint16_t,4> moves;uint16_t baby;uint8_t version,father,shared;};
    const std::array<Proof,4> proofs{{
        {{1,45,118,516},113,22,12,8},
        {{1,357,113,263},238,23,14,0},
        {{33,227,343,0},314,21,6,4},
        {{28,226,404,86},632,23,14,0},
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
    // The release accepts this order, but our exact forward witness cannot yet
    // reproduce it. Search failure must remain Incomplete, never Invalid.
    Breeding::Gen35Ancestry unverified({118,516,1,45});
    assert(unverified.possible(5,113,15,14));
    assert(!unverified.provenBirth5(113,22,15,14));
    std::cout<<"Joint Gen III-V ancestry / Gen V adapter:7 retained Incomplete and4 exclusive-paternal Invalid,paired order/source controls,4 concrete parent recipes and intermediate Durant birth PASS\n";
}
