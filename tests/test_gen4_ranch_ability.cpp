// Retained CI bytes and paired controls, through both native and shared reports.
#include "Legality/Gen4Legality.h"
#include "Encryption/Encryption4.h"
#include "Pokemon/Pokemon4ReadOnly.h"
#include "Pokemon/Pokemon4ReadOnlyView.h"
#include "Names/ItemNames.h"
#include "Names/SpeciesNames.h"
#include <array>
#include <cassert>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
namespace Trainer {
const char* getSpeciesName(uint16_t s){return Names::getSpeciesName(s);}
const char* getItemName(uint16_t s){return Names::getItemName(s);}
}
int main(){
    std::ifstream in("tests/fixtures/gen15_session2/ranch_ability.txt");assert(in);
    const std::array<const char*,5> games{{"diamond_nds","pearl_nds","platinum_nds","heartgold_nds","soulsilver_nds"}};
    std::string id,expected,hex;size_t cases=0,reports=0;
    while(in>>id>>expected>>hex){
        std::vector<uint8_t> bytes;
        for(size_t i=0;i<hex.size();i+=2)bytes.push_back(std::stoul(hex.substr(i,2),nullptr,16));
        assert(bytes.size()==136&&Legality::Gen4::checksum(bytes)==static_cast<uint16_t>(bytes[6]|(bytes[7]<<8)));
        const auto before=bytes;
        const auto wanted=expected=="INVALID"?Legality::Verdict::Invalid:expected=="INCOMPLETE"?Legality::Verdict::Incomplete:Legality::Verdict::NoProblemsFound;
        std::vector<std::byte> raw;
        for(auto b:bytes)raw.push_back(static_cast<std::byte>(b));
        const auto stored=Encryption::encryptArray4(raw);
        for(size_t i=0;i<games.size();++i){
            const auto group=i<2?Enums::GameVersion::DP:i==2?Enums::GameVersion::PT:Enums::GameVersion::HGSS;
            const Pokemon::Pokemon4ReadOnly native(stored,group);
            const Pokemon::Pokemon4ReadOnlyView view(native);
            const auto direct=Legality::analyzePk4(bytes,games[i]);
            auto shared=Legality::analyze(view,group,games[i]);
            if(direct.verdict()!=wanted||shared.verdict()!=wanted){std::cerr<<id<<" on "<<games[i]<<": wrong direct/shared verdict\n";return 1;}
            Legality::applyGen4Analyzer(shared,view,games[i]);
            assert(shared.verdict()==wanted&&bytes==before);
            if(wanted==Legality::Verdict::Incomplete)assert(!shared.hasInvalid()&&shared.coverage.pidRng==Legality::CoverageLevel::Partial);
            ++reports;
        }
        ++cases;
    }
    assert(in.eof()&&cases==5&&reports==25);
    std::cout<<"Ranch ability:retained uncertainty,PID-matched and foreign-ability controls,ordinary PID negatives,direct/shared/encrypted paths on all5 titles and repeated hook PASS\n";
}
