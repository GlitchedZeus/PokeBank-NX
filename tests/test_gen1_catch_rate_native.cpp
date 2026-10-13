#include "Legality/Legality.h"
#include "Legality/Gen12FormatDomainEvidence.h"
#include "Legality/LegalityContext.h"
#include "Legality/Gen12JointMoveEvidence.h"
#include "Legality/GBLegalityData.h"
#include "Names/ItemNames.h"
#include "Names/SpeciesNames.h"
#include "Pokemon/Experience.h"
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
                        uint16_t move=33,const std::string& source="yellow_gb",
                        uint8_t level=20) {
    PokeVault::Integration::Gen1::PokemonRecord rec{};
    rec.species=species;
    rec.catchRate=rate;
    rec.level=level;
    rec.experience=Pokemon::getExpForLevel(level,Pokemon::getGrowthRate(species));
    rec.trainerId=12345;
    // Move 33 (Tackle) stands for a natural moveset: Eevee-line Pokemon start with
    // Tackle and Sand-Attack and cannot forget either in Gen I.
    rec.moves={static_cast<uint8_t>(move),static_cast<uint8_t>(move==33 ? 28 : 0),0,0};
    if (species<=151) {
        rec.type1=Legality::GBData::kPersonalRB[species].types[0];
        rec.type2=Legality::GBData::kPersonalRB[species].types[1];
    }
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
    // Native PK1 has no stored original cartridge identity. These
    // real pinned R/B/Y compatibility pools only provide source-free
    // candidates, never a claim that Yellow was the actual source.
    namespace Gen12=Legality::Gen12JointMoveEvidence;
    const auto g1Common=Gen12::classify(1,25,{84,85,0,0});
    assert(g1Common.bound==Gen12::Bound::OneFamily);
    assert(g1Common.familyMask==3);
    const auto yellowOnly=Gen12::classify(1,25,{21,84,0,0});
    assert(yellowOnly.bound==Gen12::Bound::OneFamily);
    assert(yellowOnly.familyMask==2);
    assert(Gen12::classify(1,25,{21,166,0,0}).bound==
           Gen12::Bound::Unresolved);
    assert(Gen12::classify(1,25,{0,0,0,0}).bound==
           Gen12::Bound::Empty);
    // The analyzer searches every Gen I/II origin, so the save the PK1 sits
    // in does not change its verdict, with or without a source label.
    PokeVault::Integration::Gen1::PokemonRecord sourcefree{};
    sourcefree.species=25;
    sourcefree.catchRate=190;
    sourcefree.level=20;
    sourcefree.experience=Pokemon::getExpForLevel(20,Pokemon::getGrowthRate(25));
    sourcefree.trainerId=12345;
    sourcefree.moves={84,45,86,0}; // ThunderShock, Growl, Thunder Wave (level 9)
    sourcefree.type1=sourcefree.type2=0x17;
    sourcefree.originalTrainer="RED";
    sourcefree.nickname="PIKACHU";
    const Pokemon::Pokemon1ReadOnly pk1(sourcefree);
    for (const char* label : {"", "red_gb", "yellow_gb"}) {
        const auto r=Legality::analyze(pk1,Enums::GameVersion::RBY,label);
        assert(r.verdict()==Legality::Verdict::NoProblemsFound);
        assert(hasInfo(r,"Possible origin: wild Pikachu"));
    }

    // Strict native PK1 with a contradictory Crystal label must still
    // reject Gen II-only moves using its true Gen I storage ceiling.
    const auto mismatchedPk1=report(25,190,166,"crystal_gbc");
    assert(invalidMessage(mismatchedPk1,
        "Move id 166 cannot exist in a Generation 1 save"));
    assert(!invalidMessage(mismatchedPk1,
        "Move id 166 cannot exist in a Generation 2 save"));

    // Eevee is only obtainable at level 25 in Red/Blue/Yellow. Catch rate 45
    // (Eevee's own rate, not a Gen II item) rules out a Time Capsule history,
    // so a level-20 Eeveelution has no origin; at level 30 it is fine.
    for(uint16_t s:{134u,135u,136u}) {
        assert(invalidMessage(report(s,45),"No Red/Blue/Yellow encounter can produce"));
        const auto grown=report(s,45,33,"yellow_gb",30);
        assert(grown.verdict()==Legality::Verdict::NoProblemsFound);
        assert(hasInfo(grown,"Possible origin: gift/static Eevee"));
    }
    // Catch rate 27 is a Gen II held item, so the same level-20 Jolteon can be
    // the Goldenrod City Eevee (level 20) evolved and traded back.
    {
        const auto tradeback=report(135,27);
        assert(tradeback.verdict()==Legality::Verdict::NoProblemsFound);
        assert(hasInfo(tradeback,"Time Capsule"));
    }
    // A rate that is neither an Eevee-line rate nor a Gen II item matches nothing.
    assert(invalidMessage(report(135,7,33,"yellow_gb",30),"atch rate"));
    return 0;
}
