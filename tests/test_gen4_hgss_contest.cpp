// Synthetic/controlled records: ordinary and contest routes share a map section,
// but the contest-only bank cannot supply an ordinary-ball capture alternative.
#include "Legality/Gen4Legality.h"
#include "Names/ItemNames.h"
#include "Names/SpeciesNames.h"
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
    std::ifstream in("tests/fixtures/gen15_session2/hgss_contest.txt");assert(in);
    std::string id,game,expected,hex;size_t cases=0,positives=0,negatives=0;
    while(in>>id>>game>>expected>>hex){
        std::vector<uint8_t> bytes;
        for(size_t i=0;i<hex.size();i+=2)bytes.push_back(std::stoul(hex.substr(i,2),nullptr,16));
        assert(bytes.size()==136);
        assert(Legality::Gen4::checksum(bytes)==static_cast<uint16_t>(bytes[6]|(bytes[7]<<8)));
        const auto before=bytes;
        const auto report=Legality::analyzePk4(bytes,game);
        const auto wanted=expected=="INVALID"?Legality::Verdict::Invalid:Legality::Verdict::NoProblemsFound;
        if(report.verdict()!=wanted){std::cerr<<id<<": wrong verdict\n";return 1;}
        assert(bytes==before);
        if(wanted==Legality::Verdict::Invalid)++negatives;else ++positives;
        assert(Legality::analyzePk4(bytes,game).verdict()==wanted);
        ++cases;
    }
    assert(in.eof()&&cases==8&&positives==6&&negatives==2);
    std::cout<<"HGSS contest bank:ordinary Poke/Heal and contest Sport positives in both games;2 contest Heal negatives including retained false-clean regression;input immutability and repeated analysis PASS\n";
}
