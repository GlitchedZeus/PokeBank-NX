// Native swarm map/type gating: preserve real outbreak slots; reject default
// fishSwarm placeholders on maps where a fishing outbreak cannot occur.
#include "Legality/Gen4Legality.h"
#include "Legality/Gen4LegalityData.h"
#include "Names/ItemNames.h"
#include "Names/SpeciesNames.h"
#include <array>
#include <cassert>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
namespace Trainer {
const char* getSpeciesName(uint16_t species) { return Names::getSpeciesName(species); }
const char* getItemName(uint16_t item) { return Names::getItemName(item); }
}
int main() {
    namespace D=Legality::Gen4Data;
    size_t slots=0;
    // Native EncSlots_Update_FishingSwarm replaces ALL five Super Rod slots.
    // The current released reference duplicates slot3 and omits1/4; do not copy
    // that table defect into the existing native-supported alternatives.
    for (uint8_t version : {D::kHG,D::kSS})
        for (const auto [location,species] : std::array<std::array<uint16_t,2>,4>{{{128,340},{160,369},{180,211},{192,223}}})
            for (uint8_t slot=0;slot<5;++slot) {
                bool found=false;
                for(size_t i=0;i<D::kAreaCount;++i) {
                    const auto& a=D::kAreas[i];
                    if(a.version!=version||a.location!=location||a.type!=4)continue;
                    for(size_t j=a.slotStart;j<a.slotStart+a.slotCount;++j)
                        if(D::kSlots[j].species==species&&D::kSlots[j].slotNumber==slot&&D::kSlots[j].levelMin==40&&D::kSlots[j].levelMax==40)found=true;
                }
                assert(found);++slots;
            }
    std::ifstream input("tests/fixtures/gen15_session2/hgss_swarm.txt");assert(input);
    std::string id,game,expected,hex;size_t cases=0,positives=0,negatives=0,failures=0;
    while(input>>id>>game>>expected>>hex) {
        std::vector<uint8_t> bytes;
        for(size_t i=0;i<hex.size();i+=2)bytes.push_back(static_cast<uint8_t>(std::stoul(hex.substr(i,2),nullptr,16)));
        const auto before=bytes;assert(bytes.size()==136);
        assert(Legality::Gen4::checksum(bytes)==static_cast<uint16_t>(bytes[6]|(bytes[7]<<8)));
        const auto wanted=expected=="NOPROBLEMS"?Legality::Verdict::NoProblemsFound:Legality::Verdict::Invalid;
        const auto report=Legality::analyzePk4(bytes,game);
        if(report.verdict()!=wanted){++failures;std::cerr<<"FAIL "<<id<<": expected "<<expected<<"\n";}
        assert(bytes==before);
        if(wanted==Legality::Verdict::Invalid)++negatives;else ++positives;
        ++cases;
    }
    assert(input.eof()&&cases==14&&positives==7&&negatives==7&&slots==40);
    if(failures)return 1;
    std::cout<<"HGSS native swarm and Safari gates:7 unchanged positives,7 false-clean negatives,40 valid outbreak slot alternatives PASS\n";
}
