#include "Legality/Legality.h"
#include "Legality/Gen12FormatDomainEvidence.h"
#include "Legality/Gen12JointMoveEvidence.h"
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
bool hasInvalidText(const Legality::Report& r,const std::string& needle) {
    for(const auto& issue:r.issues)
        if(issue.severity==Legality::Severity::Invalid &&
           issue.text.find(needle)!=std::string::npos) return true;
    return false;
}
bool hasInfo(const Legality::Report& r,const std::string& needle) {
    for(const auto& issue : r.issues)
        if(issue.severity==Legality::Severity::Info &&
           issue.text.find(needle)!=std::string::npos) return true;
    return false;
}
Legality::Report report(uint16_t species, uint8_t level,
                        uint16_t caughtData,bool egg=false,
                        const std::string& source="crystal_gbc",
                        uint16_t firstMove=33) {
    PokeVault::Integration::Gen2::PokemonRecord rec{};
    rec.species=species;
    rec.level=level;
    rec.trainerId=12345;
    rec.experience=static_cast<uint32_t>(level)*level*level;
    // Native PK2 move slots are bytes. Source-proof boundary vectors
    // 251/252 fit in one byte, but the fixture accepts uint16_t to make
    // the requested move explicit. Do not implicitly narrow a runtime
    // value under -Werror.
    assert(firstMove<=255);
    rec.moves={static_cast<uint8_t>(firstMove),0,0,0};
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
    // Source-free PK2 still proves Gen II species/move maxima independently
    // from which exact Gold/Silver/Crystal save was used.
    assert(!hasInvalidText(report(251,30,0,false,"",251),
        "Move id 251 cannot exist in a Generation 2 save"));
    assert(hasInvalidText(report(25,30,0,false,"",252),
        "Move id 252 cannot exist in a Generation 2 save"));
    assert(hasInvalidText(report(252,30,0,false,"",33),
        "Species 252 cannot exist in a Generation 2 save"));
    // Strict native PK2 with a contradictory Yellow label remains Gen II.
    assert(!hasInvalidText(report(25,20,0,false,"yellow_gb",251),
        "Move id 251 cannot exist in a Generation 1 save"));
    assert(hasInvalidText(report(25,20,0,false,"yellow_gb",252),
        "Move id 252 cannot exist in a Generation 2 save"));

    // Gold/Silver and Crystal can expose disjoint compatibility
    // evidence for the same native PK2 four-slot record. Gen II compatibility
    // includes Gen I Time Capsule move pools, but no actual trade is proven.
    namespace Gen12=Legality::Gen12JointMoveEvidence;
    const auto splitSpinarak=Gen12::classify(2,167,{103,97,0,0});
    assert(splitSpinarak.bound==Gen12::Bound::TwoFamilies);
    assert(splitSpinarak.minFamilies==2 && splitSpinarak.familyMask==3);
    const auto crystalSpinarak=Gen12::classify(2,167,{97,0,0,0});
    assert(crystalSpinarak.bound==Gen12::Bound::OneFamily);
    assert(crystalSpinarak.familyMask==2);
    assert(Gen12::classify(2,167,{0,0,0,0}).bound==
           Gen12::Bound::Empty);
    assert(Gen12::classify(2,167,{103,252,0,0}).bound==
           Gen12::Bound::Unresolved);
    PokeVault::Integration::Gen2::PokemonRecord spinarak{};
    spinarak.species=167;
    spinarak.level=25;
    spinarak.trainerId=12345;
    spinarak.moves={103,97,0,0};
    spinarak.originalTrainer="RED";
    spinarak.nickname="SPINARAK";
    const Pokemon::Pokemon2ReadOnly nativeSpinarak(spinarak);
    const auto sourceFreeReport=Legality::analyze(
        nativeSpinarak,Enums::GameVersion::GSC);
    assert(hasInfo(sourceFreeReport,
        "Source-free native PK2 joint move bound: current moves require both"));
    assert(!hasInfo(Legality::analyze(
        nativeSpinarak,Enums::GameVersion::GSC,"crystal_gbc"),
        "Source-free native PK2 joint move bound"));

    // Source-free immutable native PK2: universally known current
    // Time Capsule eligibility is still reportable. The unknown source
    // game must NOT trigger Crystal-only positive encounter provenance.
    constexpr auto canTrade="Current PK2 species/moves are compatible with a Gen II -> Gen I Time Capsule trade";
    constexpr auto wrongMove="Current PK2 moves cannot currently enter the Time Capsule";
    constexpr auto wrongSpecies="Current PK2 species cannot currently enter the Time Capsule";
    constexpr auto eggRejected="Current PK2 is an Egg and cannot currently enter the Time Capsule";
    const auto sourcefreeCanTrade=report(25,20,0,false,"",33);
    assert(hasInfo(sourcefreeCanTrade,canTrade));
    const auto sourcefreeMove=report(25,20,0,false,"",166);
    assert(hasInfo(sourcefreeMove,wrongMove));
    assert(!hasInfo(sourcefreeMove,canTrade));
    const auto sourcefreeEvolved=report(169,40,0,false,"",33);
    assert(hasInfo(sourcefreeEvolved,wrongSpecies));
    const auto sourcefreeGen2Only=report(152,10,0,false,"",33);
    assert(hasInfo(sourcefreeGen2Only,wrongSpecies));
    const auto sourcefreeEgg=report(25,20,0,true,"",33);
    assert(hasInfo(sourcefreeEgg,eggRejected));
    assert(!hasInfo(sourcefreeEgg,canTrade));
    const auto sourcefreeCaught=report(16,2,(1u<<14)|(2u<<8)|2u,false,"",33);
    // Historical Gen I ancestry is independent of whether the current
    // Gen II evolved species or move list can still enter Time Capsule.
    constexpr auto potentialGen1=
        "Source-free native PK2 may descend from a Generation I ancestor";
    assert(hasInfo(sourcefreeCanTrade,potentialGen1));
    assert(hasInfo(sourcefreeMove,potentialGen1)); // Move #166 learned later.
    assert(hasInfo(sourcefreeEvolved,potentialGen1)); // Crobat <- Zubat.
    assert(!hasInfo(sourcefreeGen2Only,potentialGen1));
    assert(!hasInfo(sourcefreeEgg,potentialGen1));
    assert(!hasInfo(sourcefreeCaught,potentialGen1));
    assert(!hasInfo(report(25,20,0,false,"crystal_gbc",33),potentialGen1));
    assert(!hasInfo(sourcefreeCaught,"pinned Crystal wild encounter"));
    assert(!hasInfo(sourcefreeCaught,"pinned Generation II static/gift encounter"));

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


    // Newly bounded Crystal level-up ancestry, through the immutable PK2
    // wrapper and full production legality report. All inputs are real
    // source-row species/location/level/time combinations.
    const uint16_t caterpie=(1u<<14)|(3u<<8)|4u;
    const uint16_t weedle=(1u<<14)|(3u<<8)|4u;
    const uint16_t rattata=(1u<<14)|(2u<<8)|2u;
    const uint16_t spearow=(1u<<14)|(6u<<8)|11u;
    const uint16_t zubat=(3u<<14)|(3u<<8)|4u;
    const uint16_t sentret=(1u<<14)|(2u<<8)|2u;
    assert(hasInfo(report(11,7,caterpie),evolvedText));
    assert(hasInfo(report(12,10,caterpie),evolvedText));
    assert(hasInfo(report(14,7,weedle),evolvedText));
    assert(hasInfo(report(15,10,weedle),evolvedText));
    assert(hasInfo(report(20,20,rattata),evolvedText));
    assert(hasInfo(report(22,20,spearow),evolvedText));
    assert(hasInfo(report(42,22,zubat),evolvedText));
    assert(hasInfo(report(162,15,sentret),evolvedText));
    assert(!hasInfo(report(12,9,caterpie),evolvedText));
    assert(!hasInfo(report(15,9,weedle),evolvedText));
    assert(!hasInfo(report(162,14,sentret),evolvedText));
    assert(!hasInfo(report(42,22,(1u<<14)|(3u<<8)|4u),evolvedText));
    assert(!hasInfo(report(20,20,rattata,true),evolvedText));
    assert(!hasInfo(report(20,20,rattata,false,"silver_gbc"),evolvedText));

    // Real pinned Crystal surfing Tentacool at location1 level20.
    constexpr uint16_t tentacool=(1u<<14)|(20u<<8)|1u;
    assert(hasInfo(report(73,30,tentacool),evolvedText) ||
           hasInfo(report(73,30,tentacool),wildText));
    assert(!hasInfo(report(73,29,tentacool),evolvedText));
    return 0;
}
