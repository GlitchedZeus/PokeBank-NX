#include "Legality/Legality.h"
#include "Names/SpeciesNames.h"
#include "Names/ItemNames.h"
#include "Pokemon/Pokemon2ReadOnly.h"

#include <cassert>
#include <cstdint>
#include <string>

namespace Trainer {
const char* getSpeciesName(uint16_t id) { return Names::getSpeciesName(id); }
const char* getItemName(uint16_t id) { return Names::getItemName(id); }
}

namespace {
bool hasInfo(const Legality::Report& r,const std::string& needle) {
    for(const auto& issue : r.issues)
        if(issue.severity==Legality::Severity::Info &&
           issue.text.find(needle)!=std::string::npos) return true;
    return false;
}
Legality::Report report(uint16_t species, uint8_t level,
                        uint16_t caughtData,bool egg=false,
                        const std::string& source="crystal_gbc") {
    PokeVault::Integration::Gen2::PokemonRecord rec{};
    rec.species=species;
    rec.level=level;
    rec.trainerId=12345;
    rec.experience=static_cast<uint32_t>(level)*level*level;
    rec.moves={33,0,0,0};
    rec.pp={35,0,0,0};
    rec.dvs={12,8,9,10,11};
    rec.friendship=70;
    rec.caughtData=caughtData;
    rec.isEgg=egg;
    rec.originalTrainer="RED";
    rec.nickname="BIRD";
    Pokemon::Pokemon2ReadOnly pk(rec);
    assert(pk.getGameGroup()==Pokemon::Pokemon2ReadOnly::kReadOnlyGameGroup);
    assert(pk.speciesID()==species&&pk.level()==level);
    assert(pk.caughtData()==caughtData&&pk.isEgg()==egg);
    assert(pk.strictRecord().caughtData==caughtData);
    return Legality::analyze(pk,Enums::GameVersion::GSC,source);
}
}

int main() {
    // Crystal location 2 Pidgey #16 at level2, valid daytime mask2.
    constexpr uint16_t caught=(1u<<14)|(2u<<8)|2u;
    constexpr auto wildText="PK2 caught-data location/level/time matches a pinned Crystal wild encounter slot";
    constexpr auto evolvedText="PK2 caught-data is compatible with a pinned Crystal wild pre-evolution capture";
    assert(hasInfo(report(16,2,caught),wildText));
    assert(hasInfo(report(17,18,caught),evolvedText));
    assert(hasInfo(report(18,36,caught),evolvedText));
    assert(!hasInfo(report(18,35,caught),evolvedText));
    assert(!hasInfo(report(18,36,(3u<<14)|(2u<<8)|2u),evolvedText));
    assert(!hasInfo(report(18,36,(1u<<14)|(2u<<8)|127u),evolvedText));
    assert(!hasInfo(report(18,36,caught,true),evolvedText));
    // An unhatched Egg record must not be claimed as a direct wild catch,
    // even when its malformed caught-data field resembles a real slot.
    assert(!hasInfo(report(16,2,caught,true),wildText));
    // Time/marshalling semantics remain specific to the actual source.
    assert(!hasInfo(report(18,36,caught,false,"gold_gbc"),evolvedText));

    // Real pinned Crystal surfing Tentacool at location1 level20.
    constexpr uint16_t tentacool=(1u<<14)|(20u<<8)|1u;
    assert(hasInfo(report(73,30,tentacool),evolvedText) ||
           hasInfo(report(73,30,tentacool),wildText));
    assert(!hasInfo(report(73,29,tentacool),evolvedText));
    return 0;
}
