#include "Legality/Legality.h"
#include "Names/ItemNames.h"
#include "Names/SpeciesNames.h"
#include "Pokemon/Pokemon1ReadOnly.h"
#include <cassert>
#include <cstdint>
#include <initializer_list>
#include <string>

namespace Trainer {
const char* getSpeciesName(uint16_t id) { return Names::getSpeciesName(id); }
const char* getItemName(uint16_t id) { return Names::getItemName(id); }
}

namespace {
Legality::Report report(uint16_t species,uint8_t rate) {
    PokeVault::Integration::Gen1::PokemonRecord rec{};
    rec.species=species;
    rec.catchRate=rate;
    rec.level=20;
    rec.experience=8000;
    rec.trainerId=12345;
    rec.moves={33,0,0,0};
    rec.originalTrainer="RED";
    rec.nickname="EEVEE";
    const Pokemon::Pokemon1ReadOnly pk(rec);
    assert(pk.speciesID()==species && pk.strictRecord().catchRate==rate);
    return Legality::analyze(pk,Enums::GameVersion::RBY,"yellow_gb");
}
bool hasInfo(const Legality::Report& r,const std::string& needle){
    for(const auto& issue:r.issues)
        if(issue.severity==Legality::Severity::Info &&
           issue.text.find(needle)!=std::string::npos)return true;
    return false;
}
}
int main() {
    constexpr auto ancestor="species/pre-evolution provenance";
    // All Yellow Eevee evolutions retain a compatible Eevee rate of 45.
    for(uint16_t s:{134u,135u,136u})
        assert(hasInfo(report(s,45),ancestor));
    // A Yellow Jolteon cannot have Vaporeon's 27 as a Gen I ancestor.
    assert(!hasInfo(report(135,27),ancestor));
    // A Yellow Flareon cannot have Jolteon's 9 as an ancestor.
    assert(!hasInfo(report(136,9),ancestor));
    assert(!hasInfo(report(134,3),ancestor));
    // The current branch remains source-compatible.
    assert(hasInfo(report(135,9),ancestor) ||
           hasInfo(report(135,9),"current species in this exact"));
    assert(hasInfo(report(136,3),ancestor) ||
           hasInfo(report(136,3),"current species in this exact"));
    // No hard Invalid: Gen II held items, alternative trades and event
    // histories cannot be excluded based on this byte alone.
    // The absence of this specific positive lineage is not an Invalid
    // encounter finding, even if unrelated structural fields need review.
    for(const auto& issue:report(135,27).issues)
        assert(!(issue.severity==Legality::Severity::Invalid &&
                 issue.identifier==Legality::CheckIdentifier::Encounter));
    return 0;
}
