#include "Legality/Legality.h"
#include "Legality/Gen12FormatDomainEvidence.h"
#include "Legality/LegalityContext.h"
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
Legality::Report report(uint16_t species,uint8_t rate,
                        uint16_t move=33,const std::string& source="yellow_gb") {
    PokeVault::Integration::Gen1::PokemonRecord rec{};
    rec.species=species;
    rec.catchRate=rate;
    rec.level=20;
    rec.experience=8000;
    rec.trainerId=12345;
    rec.moves={static_cast<uint8_t>(move),0,0,0};
    rec.originalTrainer="RED";
    rec.nickname="EEVEE";
    const Pokemon::Pokemon1ReadOnly pk(rec);
    assert(pk.speciesID()==species && pk.strictRecord().catchRate==rate);
    return Legality::analyze(pk,Enums::GameVersion::RBY,source);
}
bool hasInfo(const Legality::Report& r,const std::string& needle){
    for(const auto& issue:r.issues)
        if(issue.severity==Legality::Severity::Info &&
           issue.text.find(needle)!=std::string::npos)return true;
    return false;
}
}
bool invalidMessage(const Legality::Report& r,const std::string& needle){
    for(const auto& issue:r.issues)
        if(issue.severity==Legality::Severity::Invalid &&
           issue.text.find(needle)!=std::string::npos) return true;
    return false;
}
int main() {
    namespace G = Legality::Gen12FormatDomain;
    static_assert(G::generationFromGroup(Enums::GameVersion::RBY)==1);
    static_assert(G::generationFromGroup(Enums::GameVersion::GSC)==2);
    static_assert(G::generationFromGroup(Enums::GameVersion::Invalid)==0);
    static_assert(G::maxSpecies(1)==151 && G::maxMove(1)==165);
    static_assert(G::maxSpecies(2)==251 && G::maxMove(2)==251);
    static_assert(Legality::sourceGameProfile("red_gb")->maxSpecies==G::maxSpecies(1));
    static_assert(Legality::sourceGameProfile("yellow_gb")->maxMove==G::maxMove(1));
    static_assert(Legality::sourceGameProfile("crystal_gbc")->maxSpecies==G::maxSpecies(2));
    static_assert(Legality::sourceGameProfile("gold_gbc")->maxMove==G::maxMove(2));
    // Source-free immutable PK1 still proves Gen I ceiling 151/165.
    // The PK1 move byte can represent 166; the retail Gen I moveset cannot.
    assert(!invalidMessage(report(25,190,165,""),
        "Move id 165 cannot exist in a Generation 1 save"));
    assert(invalidMessage(report(25,190,166,""),
        "Move id 166 cannot exist in a Generation 1 save"));
    assert(invalidMessage(report(152,45,33,""),
        "Species 152 cannot exist in a Generation 1 save"));

    constexpr auto ancestor="species/pre-evolution provenance";
    constexpr auto native="current species in this exact Generation I game";
    // The actual pinned Yellow rates for Eevee and ALL three branches
    // are 45, so a native-evolved specimen receives the NATIVE rate
    // message, not a distinguishable pre-evolution rate message.
    for(uint16_t s:{134u,135u,136u})
        assert(hasInfo(report(s,45),native));
    // Synthetic rates from outside the real family do not prove
    // Eevee ancestry; they might still be alternate held-item history.
    assert(!hasInfo(report(135,27),ancestor));
    assert(!hasInfo(report(136,9),ancestor));
    assert(!hasInfo(report(134,3),ancestor));
    assert(!hasInfo(report(135,9),native));
    assert(!hasInfo(report(136,3),native));
    // No hard Invalid: Gen II held items, alternative trades and event
    // histories cannot be excluded based on this byte alone.
    // The absence of this specific positive lineage is not an Invalid
    // encounter finding, even if unrelated structural fields need review.
    for(const auto& issue:report(135,27).issues)
        assert(!(issue.severity==Legality::Severity::Invalid &&
                 issue.identifier==Legality::CheckIdentifier::Encounter));
    return 0;
}
