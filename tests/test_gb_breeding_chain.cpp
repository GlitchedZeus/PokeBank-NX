#include "Legality/GBLegality.h"
#include "gb_native_fixture.h"
#include "Names/ItemNames.h"
#include "Names/SpeciesNames.h"
#include <fstream>
#include <algorithm>
#include <iostream>
#include <map>
namespace Trainer {
const char* getSpeciesName(uint16_t s){return Names::getSpeciesName(s);}
const char* getItemName(uint16_t s){return Names::getItemName(s);}
}
int main() {
    using V=Legality::Verdict;using namespace Legality::GB;
    const std::map<std::string,V> expected{{"7517",V::Invalid},{"7527",V::Incomplete},{"7553",V::Invalid},{"7575",V::Incomplete},{"7582",V::Invalid},{"9123",V::Invalid},{"9429",V::Invalid},{"9484",V::Invalid},{"9615",V::NoProblemsFound},{"9744",V::NoProblemsFound},{"9869",V::Invalid},{"9891",V::Invalid}};
    std::ifstream cases("tests/fixtures/gen15_session2/breeding12.txt");assert(cases);std::string line;size_t count=0;
    while(std::getline(cases,line)) {
        const auto r=GBFixture::analyze(line);
        if(r.report.verdict()!=expected.at(r.id)) {std::cerr<<r.id<<" unexpected verdict\n";for(const auto& i:r.report.issues)std::cerr<<i.text<<'\n';}
        assert(r.report.verdict()==expected.at(r.id));++count;
    }
    assert(count==12);
    std::ifstream p0("tests/fixtures/gen15_session2/false_clean38.txt");assert(p0);count=0;
    while(std::getline(p0,line)){const auto r=GBFixture::analyze(line);assert(r.report.verdict()==V::NoProblemsFound);++count;}assert(count==38);
    // CI seed 2: a still-unhatched Smoochum cannot receive Blizzard from any
    // native compatible father. Hatched TM use and an obtainable inherited TM
    // are valid alternatives; do not reject those with the egg constraint.
    std::ifstream ci("tests/fixtures/gen15_session2/ci_smoochum.txt");assert(ci);count=0;
    while(std::getline(ci,line)) {
        const auto r=GBFixture::analyze(line);
        assert(r.report.verdict()==(count==0?V::Invalid:V::NoProblemsFound));++count;
    }
    assert(count==3);
    // Cartridge-backed alternate ancestry: Gen I TM first, Gen II level move
    // afterwards, then one compatible father passes BOTH moves in one egg.
    auto proof=proveBreeding2(1,{130,219,0,0},false,false,Language::International);
    assert(proof.supported&&proof.possible&&proof.father!=0);
    assert(std::any_of(proof.steps.begin(),proof.steps.end(),[](const auto& s){return s.species==131;}));
    assert(std::any_of(proof.steps.begin(),proof.steps.end(),[](const auto& s){return s.kind==Legality::Breeding::StepKind::PreviousEra;}));
    proof=proveBreeding2(102,{72,236,0,0},true,false,Language::Japanese);assert(proof.possible&&proof.father==43);
    // Each move is possible separately, yet no one father can provide the pair.
    assert(proveBreeding2(72,{114,0,0,0},false,false,Language::International).possible);
    assert(proveBreeding2(72,{243,0,0,0},false,false,Language::International).possible);
    assert(!proveBreeding2(72,{114,243,0,0},false,false,Language::International).possible);
    assert(!proveBreeding2(115,{146,0,0,0},false,false,Language::International).possible);
    // Genderless and male-only species must use their own line with Ditto.
    assert(!proveBreeding2(100,{130,0,0,0},false,false,Language::International).possible);
    assert(!proveBreeding2(236,{227,0,0,0},false,false,Language::International).possible);
    assert(!proveBreeding2(0,{1,2,3,4},false,false,Language::Unknown).supported);
    std::cout<<"Native breeding production: 12 retained cases, 38 source-adjudicated clean cases, father witnesses, incompatible combinations and sex/Ditto controls PASS\n";
}
