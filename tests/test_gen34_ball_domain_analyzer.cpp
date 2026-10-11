#include "Encryption/Encryption4.h"
#include "Legality/Legality.h"
#include "Legality/Gen4TradeEvidence.h"
#include "Legality/Gen3JointMoveEvidence.h"
#include "Legality/Gen4JointMoveEvidence.h"
#include "Legality/Gen34PalParkMoves.h"
#include "Legality/Gen34PalParkSpecies.h"
#include "Legality/Gen4RangerManaphy.h"
#include "Legality/Gen4EventTemplate.h"
#include "Names/ItemNames.h"
#include "Names/SpeciesNames.h"
#include "Pokemon/Pokemon3FRLG.h"
#include "Pokemon/Pokemon4ReadOnly.h"
#include "Pokemon/Pokemon4ReadOnlyView.h"

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace Trainer {
const char* getSpeciesName(uint16_t speciesId) { return Names::getSpeciesName(speciesId); }
const char* getItemName(uint16_t itemId) { return Names::getItemName(itemId); }
}

namespace {
bool hasText(const Legality::Report& report, const std::string& needle) {
    for (const auto& issue : report.issues)
        if (issue.text.find(needle) != std::string::npos) return true;
    return false;
}

bool hasInvalidText(const Legality::Report& report,const std::string& needle) {
    for(const auto& issue:report.issues)
        if(issue.severity==Legality::Severity::Invalid &&
           issue.text.find(needle)!=std::string::npos)return true;
    return false;
}

bool hasInfo(const Legality::Report& report, const std::string& needle) {
    for (const auto& issue : report.issues)
        if (issue.severity == Legality::Severity::Info &&
            issue.text.find(needle) != std::string::npos) return true;
    return false;
}

Pokemon::Pokemon3FRLG gen3WithBall(uint8_t ball) {
    std::array<uint8_t, 80> raw{};
    Pokemon::Pokemon3FRLG p(std::span<const std::byte>(
        reinterpret_cast<const std::byte*>(raw.data()), raw.size()));
    p.setPID(0x12345678u);
    p.setTID16(12345);
    p.setSID16(54321);
    p.setLanguage(2);
    p.setSpecies(1);
    p.setOriginGame(2);
    p.setBall(ball);
    p.setLevel(5);
    p.setOTName(u"RED");
    p.setNickname(u"BULBASAUR");
    p.refreshChecksum();
    return p;
}

void wr16(std::vector<std::byte>& data, size_t offset, uint16_t value) {
    data[offset] = static_cast<std::byte>(value & 0xFFu);
    data[offset + 1] = static_cast<std::byte>(value >> 8);
}

void wr32(std::vector<std::byte>& data, size_t offset, uint32_t value) {
    data[offset] = static_cast<std::byte>(value & 0xFFu);
    data[offset + 1] = static_cast<std::byte>((value >> 8) & 0xFFu);
    data[offset + 2] = static_cast<std::byte>((value >> 16) & 0xFFu);
    data[offset + 3] = static_cast<std::byte>((value >> 24) & 0xFFu);
}

Legality::Report analyzeGen4Ball(uint8_t ball, bool exactSource = true,
                                 uint16_t species=1, uint16_t move=0,
                                 uint8_t originVersion=12,
                                 const char* sourceOverride=nullptr) {
    std::vector<std::byte> decrypted(Encryption::SIZE_STORED4, std::byte{0});
    wr32(decrypted, 0x00, 0x12345678u);
    wr16(decrypted, 0x08, species);  // native stored PK4 species
    wr16(decrypted, 0x0C, 12345);
    wr16(decrypted, 0x0E, 54321);
    wr32(decrypted, 0x10, 135);     // level 5, Medium Slow
    wr16(decrypted, 0x28, move);   // native stored PK4 first move
    decrypted[0x15] = std::byte{65}; // Overgrow
    decrypted[0x17] = std::byte{2};  // English
    decrypted[0x5F] = static_cast<std::byte>(originVersion); // native PK4 stored origin
    decrypted[0x83] = static_cast<std::byte>(ball);

    const auto encrypted = Encryption::encryptArray4(decrypted);
    assert(encrypted.size() == Encryption::SIZE_STORED4);
    Pokemon::Pokemon4ReadOnly source(encrypted, Enums::GameVersion::PT);
    assert(source.valid());
    assert(source.ballDPPt() == ball);
    assert(source.originVersion()==originVersion);
    assert(source.species() == species && source.moves()[0] == move);
    Pokemon::Pokemon4ReadOnlyView view(source);
    assert(view.ball() == ball);

    if(sourceOverride)
        return Legality::analyze(view,Enums::GameVersion::PT,sourceOverride);
    return exactSource
        ? Legality::analyze(view, Enums::GameVersion::PT, "platinum_nds")
        : Legality::analyze(view, Enums::GameVersion::PT);
}

Legality::Report analyzeSourceFreeNativeGen4Moves(
        uint8_t originalVersion,uint16_t species,
        uint16_t first,uint16_t second) {
    // The native encrypted PK4 -> read-only view -> central analyzer path.
    std::vector<std::byte> raw(Encryption::SIZE_STORED4,std::byte{0});
    wr32(raw,0x00,0x12345678u);
    wr16(raw,0x08,species);
    wr16(raw,0x0C,12345);
    wr16(raw,0x0E,54321);
    wr32(raw,0x10,3375);
    wr16(raw,0x28,first);
    wr16(raw,0x2A,second);
    raw[0x17]=std::byte{2};
    raw[0x5F]=static_cast<std::byte>(originalVersion);
    raw[0x83]=std::byte{4};
    const auto enc=Encryption::encryptArray4(raw);
    Pokemon::Pokemon4ReadOnly pk(enc,Enums::GameVersion::PT);
    assert(pk.valid() && pk.checksumValid());
    assert(pk.originVersion()==originalVersion);
    assert(pk.moves()[0]==first && pk.moves()[1]==second);
    Pokemon::Pokemon4ReadOnlyView view(pk);
    return Legality::analyze(view,Enums::GameVersion::PT);
}

Legality::Report analyzeDetachedPalParkMoves(
        uint8_t originalVersion,uint16_t move1,uint16_t move2,
        bool exactSource=false,bool withPalParkMarker=true,
        bool incompatibleBall=false,bool invalidSplit=false,
        uint16_t species=25) {
    std::vector<std::byte> raw(Encryption::SIZE_STORED4,std::byte{0});
    wr32(raw,0x00,0x12345678u);
    wr16(raw,0x08,species); // National species ID for PK4 lineage tests.
    wr16(raw,0x0C,12345);
    wr16(raw,0x0E,54321);
    wr32(raw,0x10,3375);
    wr16(raw,0x28,move1);
    wr16(raw,0x2A,move2);
    raw[0x17]=std::byte{2};
    raw[0x5F]=static_cast<std::byte>(originalVersion);
    raw[0x83]=static_cast<std::byte>(incompatibleBall ? 13 : 4);
    if(withPalParkMarker) {
        wr16(raw,0x80,invalidSplit ? 0x36 : 0x37);
        wr16(raw,0x46,0x37);
    }
    const auto encrypted=Encryption::encryptArray4(raw);
    Pokemon::Pokemon4ReadOnly pk(encrypted,Enums::GameVersion::PT);
    assert(pk.valid() && pk.checksumValid());
    assert(pk.originVersion()==originalVersion);
    assert(pk.moves()[0]==move1 && pk.moves()[1]==move2);
    Pokemon::Pokemon4ReadOnlyView view(pk);
    if(withPalParkMarker)assert(view.metLocation()==0x37);
    return exactSource
        ? Legality::analyze(view,Enums::GameVersion::PT,"platinum_nds")
        : Legality::analyze(view,Enums::GameVersion::PT);
}

Legality::Report analyzeNativeHgssSpecialBall(
        uint16_t species, uint16_t metLocation,
        uint8_t metLevel, uint8_t ball, uint8_t originVersion = 7,
        uint32_t experience = 3375, uint32_t pid = 0x12345678u,
        uint16_t eggLocation = 0,
        bool exactSource = true) {
    std::vector<std::byte> raw(Encryption::SIZE_STORED4, std::byte{0});
    wr32(raw, 0x00, pid);
    wr16(raw, 0x08, species);
    wr16(raw, 0x0C, 12345);
    wr16(raw, 0x0E, 54321);
    wr32(raw, 0x10, experience);
    raw[0x17] = std::byte{2};
    raw[0x5F] = static_cast<std::byte>(originVersion);
    wr16(raw, 0x44, eggLocation); // Persistent Gen IV EggLocation.
    wr16(raw, 0x46, metLocation); // Extended Pt/HGSS location.
    raw[0x83] = std::byte{4};     // D/P/Pt Poké Ball shadow byte.
    raw[0x84] = static_cast<std::byte>(metLevel);
    raw[0x86] = static_cast<std::byte>(ball); // HG/SS special ball byte.
    const auto encrypted = Encryption::encryptArray4(raw);
    Pokemon::Pokemon4ReadOnly source(encrypted, Enums::GameVersion::HGSS);
    assert(source.valid());
    assert(source.originVersion() == originVersion);
    assert(source.metLocationExtended() == metLocation);
    assert(source.eggLocationExtended() == eggLocation);
    assert(source.metLevel() == metLevel);
    assert(source.ballHGSS() == ball);
    Pokemon::Pokemon4ReadOnlyView view(source);
    assert(view.metLocation() == metLocation);
    // The shared view prefers a known HGSS ball over the stored D/P/Pt
    // Poké Ball shadow. Both valid special balls are greater than ID 4.
    assert(view.ball() == ball);
    return exactSource
        ? Legality::analyze(view, Enums::GameVersion::HGSS, "heartgold_nds")
        : Legality::analyze(view, Enums::GameVersion::HGSS);
}
}

Legality::Report analyzeNativeDiamondMarshBall(
        uint16_t species, uint16_t metLocation, uint8_t metLevel,
        uint8_t ball, uint8_t originVersion = 10,
        uint32_t experience = 3375,
        bool exactSource = true) {
    std::vector<std::byte> raw(Encryption::SIZE_STORED4, std::byte{0});
    wr32(raw, 0x00, 0x12345678u);
    wr16(raw, 0x08, species);
    wr16(raw, 0x0C, 12345);
    wr16(raw, 0x0E, 54321);
    wr32(raw, 0x10, experience);
    raw[0x17] = std::byte{2};
    raw[0x5F] = static_cast<std::byte>(originVersion);
    wr16(raw, 0x80, metLocation); // D/P native location.
    raw[0x83] = static_cast<std::byte>(ball);
    raw[0x84] = static_cast<std::byte>(metLevel);
    const auto encrypted = Encryption::encryptArray4(raw);
    Pokemon::Pokemon4ReadOnly source(encrypted, Enums::GameVersion::DP);
    assert(source.valid());
    assert(source.originVersion() == originVersion);
    assert(source.metLocationDP() == metLocation);
    assert(source.ballDPPt() == ball);
    Pokemon::Pokemon4ReadOnlyView view(source);
    assert(view.metLocation() == metLocation);
    assert(view.ball() == ball);
    return exactSource
        ? Legality::analyze(view, Enums::GameVersion::DP, "diamond_nds")
        : Legality::analyze(view, Enums::GameVersion::DP);
}


Legality::Report analyzeGen4EggBall(uint8_t ball, bool isEgg,
                                    uint16_t eggLocation,
                                    uint8_t metLevel=0,
                                    uint8_t originVersion=7,
                                    uint16_t hatchLocation=0,
                                    bool exactSource=true) {
    std::vector<std::byte> raw(Encryption::SIZE_STORED4,std::byte{0});
    wr32(raw,0x00,0x12345678u);
    wr16(raw,0x08,25);
    wr16(raw,0x0C,12345);
    wr16(raw,0x0E,54321);
    raw[0x17]=std::byte{2};
    raw[0x5F]=static_cast<std::byte>(originVersion); // stored PK4 origin
    wr16(raw,0x44,eggLocation);
    wr16(raw,0x46,hatchLocation); // HGSS extended MetLocation
    raw[0x83]=static_cast<std::byte>(ball);
    raw[0x86]=static_cast<std::byte>(ball);
    raw[0x84]=static_cast<std::byte>(metLevel);
    if(isEgg)wr32(raw,0x38,0x40000000u);
    const auto encrypted=Encryption::encryptArray4(raw);
    Pokemon::Pokemon4ReadOnly source(encrypted,Enums::GameVersion::HGSS);
    assert(source.valid() && source.checksumValid());
    assert(source.isEgg()==isEgg);
    assert(source.eggLocationExtended()==eggLocation);
    Pokemon::Pokemon4ReadOnlyView view(source);
    assert(view.eggLocation()==eggLocation && view.ball()==ball);
    if(hatchLocation!=0)assert(view.metLocation()==hatchLocation);
    return exactSource
        ? Legality::analyze(view,Enums::GameVersion::HGSS,"heartgold_nds")
        : Legality::analyze(view,Enums::GameVersion::HGSS);
}


Legality::Report sourceFreePalPark(uint8_t originVersion,uint16_t dp,
                                   uint16_t extended,uint8_t ballDPPt,
                                   uint8_t ballHGSS,bool isEgg=false,
                                   uint16_t species=25) {
    // Actual stored/encrypted PK4 format. The exact container game identity
    // is intentionally absent; stored Gen III origin remains independently
    // recoverable and can still prove universal transfer field rules.
    std::vector<std::byte> raw(Encryption::SIZE_STORED4,std::byte{0});
    wr32(raw,0x00,0x12345678u);
    wr16(raw,0x08,species); // National PK4 species, possibly evolved post-transfer.
    wr16(raw,0x0C,12345);
    wr16(raw,0x0E,54321);
    wr32(raw,0x10,3375);
    raw[0x17]=std::byte{2};
    raw[0x5F]=static_cast<std::byte>(originVersion);
    wr16(raw,0x80,dp);
    wr16(raw,0x46,extended);
    raw[0x83]=static_cast<std::byte>(ballDPPt);
    raw[0x86]=static_cast<std::byte>(ballHGSS);
    if(isEgg)wr32(raw,0x38,0x40000000u);
    const auto encrypted=Encryption::encryptArray4(raw);
    Pokemon::Pokemon4ReadOnly stored(encrypted,Enums::GameVersion::HGSS);
    assert(stored.valid() && stored.checksumValid());
    assert(stored.originVersion()==originVersion);
    assert(stored.metLocationDP()==dp);
    assert(stored.metLocationExtended()==extended);
    assert(stored.ballDPPt()==ballDPPt);
    assert(stored.ballHGSS()==ballHGSS);
    Pokemon::Pokemon4ReadOnlyView view(stored);
    assert(view.originGame()==originVersion);
    return Legality::analyze(view,Enums::GameVersion::HGSS);
}

// Pinned Diamond NPC Abra is a fixed PID/TID/IV/OT-gender template.
// Create actual encrypted PK4, never a synthetic stand-in helper report.
Legality::Report analyzeDetachedNpcAbra(uint16_t species=63,
                                        uint8_t originalGame=10,
                                        uint8_t language=1,
                                        uint32_t pid=0x0000008Eu,
                                        uint8_t atkIV=15,
                                        bool exactSource=false) {
    std::vector<std::byte> raw(Encryption::SIZE_STORED4,std::byte{0});
    wr32(raw,0x00,pid);
    wr16(raw,0x08,species);
    wr16(raw,0x0C,25643);
    wr16(raw,0x0E,0);
    wr32(raw,0x10,3375);
    raw[0x17]=static_cast<std::byte>(language);
    raw[0x5F]=static_cast<std::byte>(originalGame);
    wr32(raw,0x38,Legality::Gen4Trade::packIVs({
        15,atkIV,15,20,25,25}));
    wr16(raw,0x80,2001); // link-trade met location
    raw[0x83]=std::byte{4}; // native Poke Ball
    raw[0x84]=std::byte{0x81}; // female OT, received level1
    const auto encrypted=Encryption::encryptArray4(raw);
    Pokemon::Pokemon4ReadOnly stored(encrypted,Enums::GameVersion::DP);
    assert(stored.valid() && stored.checksumValid());
    assert(stored.species()==species && stored.pid()==pid);
    assert(stored.originVersion()==originalGame);
    assert(stored.metLocationDP()==2001);
    assert(stored.originalTrainerGender()==1);
    Pokemon::Pokemon4ReadOnlyView view(stored);
    assert(view.metLocation()==2001 && view.metLevel()==1);
    assert(view.abilityNumber()==static_cast<uint8_t>(1u<<(pid&1u)));
    return exactSource
        ? Legality::analyze(view,Enums::GameVersion::DP,"diamond_nds")
        : Legality::analyze(view,Enums::GameVersion::DP);
}

Legality::Report analyzeDetachedRanger(uint32_t pid=0x5271E97Eu,
                                       uint8_t language=2,
                                       uint8_t ball=4,
                                       uint16_t eggLocation=3001,
                                       uint8_t originVersion=10,
                                       uint8_t gender=2) {
    std::vector<std::byte> raw(Encryption::SIZE_STORED4,std::byte{0});
    wr32(raw,0x00,pid);
    wr16(raw,0x08,490); // Ranger Manaphy
    wr16(raw,0x0C,12345);wr16(raw,0x0E,54321);
    wr32(raw,0x10,3375);
    raw[0x17]=static_cast<std::byte>(language);
    raw[0x40]=static_cast<std::byte>(1u | (gender<<1)); // fateful/gender
    wr16(raw,0x44,eggLocation);
    raw[0x5F]=static_cast<std::byte>(originVersion);
    wr32(raw,0x38,0x40000000u | Legality::Gen4Trade::packIVs({
        16,13,12,2,18,3}));
    raw[0x83]=static_cast<std::byte>(ball);
    const auto encrypted=Encryption::encryptArray4(raw);
    Pokemon::Pokemon4ReadOnly stored(encrypted,Enums::GameVersion::DP);
    assert(stored.valid() && stored.checksumValid());
    assert(stored.isEgg() && stored.species()==490);
    assert(stored.originVersion()==originVersion);
    Pokemon::Pokemon4ReadOnlyView view(stored);
    assert(view.eggLocation()==eggLocation && view.ball()==ball);
    assert(view.gender()==gender && view.isFatefulEncounter());
    return Legality::analyze(view,Enums::GameVersion::DP);
}

Legality::Report analyzeDetachedPcd(uint16_t species=25,
                                    uint8_t originVersion=10,
                                    uint32_t pid=0x246e13afu,
                                    uint16_t metLocation=3060,
                                    uint8_t metLevel=50,
                                    uint8_t ball=16,
                                    uint8_t language=2) {
    std::vector<std::byte> raw(Encryption::SIZE_STORED4,std::byte{0});
    wr32(raw,0x00,pid);
    wr16(raw,0x08,species);
    wr16(raw,0x0C,8107);wr16(raw,0x0E,20846);
    wr32(raw,0x10,125000);
    raw[0x17]=static_cast<std::byte>(language);
    raw[0x40]=std::byte{1}; // male, fateful, form0
    raw[0x5F]=static_cast<std::byte>(originVersion);
    wr16(raw,0x80,metLocation);
    raw[0x83]=static_cast<std::byte>(ball);
    raw[0x84]=static_cast<std::byte>(metLevel);
    const auto encrypted=Encryption::encryptArray4(raw);
    Pokemon::Pokemon4ReadOnly stored(encrypted,Enums::GameVersion::DP);
    assert(stored.valid() && stored.checksumValid());
    assert(stored.pid()==pid && stored.originVersion()==originVersion);
    Pokemon::Pokemon4ReadOnlyView view(stored);
    assert(view.speciesID()==species && view.metLocation()==metLocation);
    assert(view.metLevel()==metLevel && view.ball()==ball);
    return Legality::analyze(view,Enums::GameVersion::DP);
}

int main() {
    // Source-free exact-origin NPC trade proof: all native encrypted PK4
    // identity fields must match a pinned Diamond template. Wrong PID,
    // original retail game, language, or IV breaks only the positive
    // compatibility; absence does NOT establish global illegality.
    constexpr auto detachedTrade="Source-free native PK4 PID/trainer/IV/ability/language/met fields match a pinned Generation IV in-game NPC trade";
    assert(hasInfo(analyzeDetachedNpcAbra(),detachedTrade));
    assert(hasInfo(analyzeDetachedNpcAbra(64),detachedTrade));
    assert(hasInfo(analyzeDetachedNpcAbra(64),
                   "with source ancestor species 63"));
    assert(!hasInfo(analyzeDetachedNpcAbra(63,7),detachedTrade));
    assert(!hasInfo(analyzeDetachedNpcAbra(63,10,2),detachedTrade));
    assert(!hasInfo(analyzeDetachedNpcAbra(63,10,1,0x8Fu),detachedTrade));
    assert(!hasInfo(analyzeDetachedNpcAbra(63,10,1,0x8Eu,14),detachedTrade));

    // Encrypted native PK4 Ranger PGT, no known physical source save:
    // only a positive pinned Method 1 match supports RNG provenance.
    constexpr auto rangerPositive="Source-free native PK4 has compatible Ranger Manaphy PGT structure and";
    constexpr auto rangerMethod="Ranger Manaphy Method 1";
    const auto rangerEgg=analyzeDetachedRanger();
    assert(hasInfo(rangerEgg,rangerPositive));
    assert(hasInfo(rangerEgg,rangerMethod));
    assert(!hasInfo(analyzeDetachedRanger(0x5271E97Eu,2,16),
                    rangerPositive)); // incompatible Ball
    assert(!hasInfo(analyzeDetachedRanger(0x5271E97Eu,8),
                    rangerPositive)); // Korean language unsupported
    assert(!hasInfo(analyzeDetachedRanger(0x5271E97Eu,2,4,0),
                    rangerPositive)); // missing Ranger provenance marker
    assert(!hasInfo(analyzeDetachedRanger(0x5271E97Eu,2,4,3001,0),
                    rangerPositive)); // unknown native origin version
    assert(!hasInfo(analyzeDetachedRanger(0x5271E97Eu,2,4,3001,10,0),
                    rangerPositive)); // not genderless
    const auto rangerWrongPid=analyzeDetachedRanger(0x12345678u);
    assert(!hasInfo(rangerWrongPid,rangerMethod));

    // Pinned first WC4 Pokémon PCD: Diamond/Pearl fixed-PID Pikachu.
    // A detached encrypted native PK4 retains all the template identity
    // fields, but a match is only INFO-compatible event provenance.
    constexpr auto wc4="Source-free native PK4 persistent fields match a pinned Generation IV WC4/PCD Pokemon event template";
    assert(hasInfo(analyzeDetachedPcd(),wc4));
    const auto evolvedPcd=analyzeDetachedPcd(26);
    assert(hasInfo(evolvedPcd,wc4));
    assert(hasInfo(evolvedPcd,"after evolution from source species 25"));
    assert(!hasInfo(analyzeDetachedPcd(25,7),wc4));
    assert(!hasInfo(analyzeDetachedPcd(25,0),wc4));
    assert(!hasInfo(analyzeDetachedPcd(25,10,0x246e13aeu),wc4));
    assert(!hasInfo(analyzeDetachedPcd(25,10,0x246e13afu,3059),wc4));
    assert(!hasInfo(analyzeDetachedPcd(25,10,0x246e13afu,3060,51),wc4));
    assert(!hasInfo(analyzeDetachedPcd(25,10,0x246e13afu,3060,50,4),wc4));
    assert(!hasInfo(analyzeDetachedPcd(25,10,0x246e13afu,3060,50,16,5),wc4));

    // Real PK3 egg-state and encrypted PK4 egg-origin reporting.
    auto eggPk3=gen3WithBall(4);
    eggPk3.setEgg(true);
    eggPk3.setMetLevel(0);
    eggPk3.refreshChecksum();
    assert(eggPk3.isEgg());
    const auto eggG3=Legality::analyze(
        eggPk3,Enums::GameVersion::FRLG,"ruby_gba");
    assert(hasInfo(eggG3,
        "Native Generation III/IV egg origin has a compatible Poke Ball"));
    // Source-free native PK3 has a known Gen III format; preserve its
    // structurally necessary egg-level and source-compatible Ball rules.
    const auto sourcefreePk3 = Legality::analyze(
        eggPk3, Enums::GameVersion::FRLG);
    assert(hasInfo(sourcefreePk3,
        "PK3 unhatched egg state has the native met-level-0 structure"));
    assert(hasInfo(sourcefreePk3,
        "Native Generation III/IV egg origin has a compatible Poke Ball"));
    eggPk3.setMetLevel(1);
    const auto invalidSourcefreePk3 = Legality::analyze(
        eggPk3, Enums::GameVersion::FRLG);
    assert(hasInvalidText(invalidSourcefreePk3,
        "Egg-origin record has invalid met level"));
    eggPk3.setMetLevel(0);
    eggPk3.refreshChecksum();

    eggPk3.setOriginGame(15); // GameCube, not a native Gen III egg origin.
    const auto cubeEgg=Legality::analyze(
        eggPk3,Enums::GameVersion::FRLG,"ruby_gba");
    assert(!hasInfo(cubeEgg,
        "Native Generation III/IV egg origin has a compatible Poke Ball"));
    eggPk3.setOriginGame(2);
    eggPk3.setBall(5);
    const auto safariEgg=Legality::analyze(
        eggPk3,Enums::GameVersion::FRLG,"ruby_gba");
    assert(!hasInfo(safariEgg,
        "Native Generation III/IV egg origin has a compatible Poke Ball"));

    // Real encrypted PK4 without a known enclosing save remains
    // format-known. Reuse the same native structural, ball and hatch checks.
    const auto sourcefreePk4Egg=analyzeGen4EggBall(4,true,2000,0,7,0,false);
    assert(hasInfo(sourcefreePk4Egg,
        "PK4 egg-origin state has native met-level-0 structure"));
    assert(hasInfo(sourcefreePk4Egg,
        "Native Generation III/IV egg origin has a compatible Poke Ball"));
    const auto sourcefreeMetBad=analyzeGen4EggBall(4,true,2000,1,7,0,false);
    assert(hasInvalidText(sourcefreeMetBad,
        "Egg-origin record has invalid met level"));
    const auto sourcefreeMissingLoc=analyzeGen4EggBall(4,true,0,0,7,0,false);
    assert(hasInvalidText(sourcefreeMissingLoc,
        "Unhatched PK4 egg is missing egg location"));
    const auto sourcefreeKnownHatch=analyzeGen4EggBall(4,false,2000,0,10,126,false);
    assert(hasInvalidText(sourcefreeKnownHatch,
        "Hatch location is not valid for this Generation IV egg origin"));
    const auto sourcefreeUnknownHatch=analyzeGen4EggBall(4,false,2000,0,0,126,false);
    assert(hasInfo(sourcefreeUnknownHatch,
        "PK4 hatch origin game is unknown or unsupported"));
    assert(!hasInvalidText(sourcefreeUnknownHatch,
        "Hatch location is not valid for this Generation IV egg origin"));

    const auto nativePk4Egg=analyzeGen4EggBall(4,true,2000);
    assert(hasInfo(nativePk4Egg,
        "Native Generation III/IV egg origin has a compatible Poke Ball"));
    const auto hatchedPk4=analyzeGen4EggBall(4,false,2000);
    assert(hasInfo(hatchedPk4,
        "Native Generation III/IV egg origin has a compatible Poke Ball"));
    const auto wrongBallEgg=analyzeGen4EggBall(24,true,2000);
    assert(!hasInfo(wrongBallEgg,
        "Native Generation III/IV egg origin has a compatible Poke Ball"));
    const auto noEggOrigin=analyzeGen4EggBall(4,false,0);
    assert(!hasInfo(noEggOrigin,
        "Native Generation III/IV egg origin has a compatible Poke Ball"));
    const auto badMet=analyzeGen4EggBall(4,true,2000,1);
    assert(!hasInfo(badMet,
        "Native Generation III/IV egg origin has a compatible Poke Ball"));
    // The exact HGSS container can be known while a PK4 origin byte
    // is missing. Missing origin cannot prove an impossible hatch.
    // A known Diamond origin cannot hatch at HGSS-only location 126
    // without an egg's Link Trade marker.
    constexpr auto hatchError="Hatch location is not valid for this Generation IV egg origin";
    constexpr auto hatchUnknown="PK4 hatch origin game is unknown or unsupported";
    const auto unknownHatched=analyzeGen4EggBall(4,false,2000,0,0,126);
    assert(hasInfo(unknownHatched,hatchUnknown));
    assert(!hasInvalidText(unknownHatched,hatchError));
    const auto hgssHatched=analyzeGen4EggBall(4,false,2000,0,7,126);
    assert(hasInfo(hgssHatched,"hatch location is valid for its stored"));
    const auto diamondCannotHatch=analyzeGen4EggBall(4,false,2000,0,10,126);
    assert(hasInvalidText(diamondCannotHatch,hatchError));
    const auto linkedDiamondEgg=analyzeGen4EggBall(4,false,2002,0,10,126);
    assert(hasInfo(linkedDiamondEgg,"traded-egg hatch location is valid"));
    assert(!hasInvalidText(linkedDiamondEgg,hatchError));

    // Pal Park/Gen III origin is NOT a native Gen IV egg history, even
    // if a malformed record supplies egg-location fields and Poké Ball.
    const auto palParkOrigin=analyzeGen4EggBall(4,true,2000,0,2);
    assert(!hasInfo(palParkOrigin,
        "Native Generation III/IV egg origin has a compatible Poke Ball"));
    const auto unknownOrigin=analyzeGen4EggBall(4,true,2000,0,0);
    assert(!hasInfo(unknownOrigin,
        "Native Generation III/IV egg origin has a compatible Poke Ball"));

    // Source-free immutable PK3/PK4 still retains origin-game evidence
    // for a positive egg-move LEARNSET possibility, not a proven hatch.
    auto g3Egg=gen3WithBall(4);
    g3Egg.setMove(0,80); // Bulbasaur Petal Dance, Gen III egg.
    assert(hasInfo(Legality::analyze(g3Egg,Enums::GameVersion::FRLG),
                   "direct egg move"));
    auto g3NonEgg=gen3WithBall(4);
    g3NonEgg.setMove(0,57); // Surf is not a Bulbasaur egg move.
    assert(!hasInfo(Legality::analyze(g3NonEgg,Enums::GameVersion::FRLG),
                    "direct egg move"));
    // PK3 is intentionally non-copyable; construct another native record
    // and retain only the tested egg move when varying source identity.
    auto unknownG3=gen3WithBall(4);
    unknownG3.setMove(0,80);
    unknownG3.setOriginGame(0);
    assert(!hasInfo(Legality::analyze(unknownG3,Enums::GameVersion::FRLG),
                    "direct egg move"));

    // HGSS added Bulbasaur egg move #124; D/P/Pt did not.
    // Derive the group from the stored PK4 origin, never the current
    // save container when a source context is missing.
    assert(hasInfo(analyzeGen4Ball(4,false,1,124,7),"direct egg move"));
    assert(!hasInfo(analyzeGen4Ball(4,false,1,124,12),"direct egg move"));
    assert(!hasInfo(analyzeGen4Ball(4,false,1,124,0),"direct egg move"));
    // Gen III Bulbasaur -> Pal Park in Gen IV retains Gen III egg pool.
    assert(hasInfo(analyzeGen4Ball(4,false,1,80,2),"direct egg move"));
    // Evolved Ivysaur retains a Diamond-origin Bulbasaur egg move.
    assert(hasInfo(analyzeGen4Ball(4,false,2,437,10),
                   "retained pre-evolution egg move"));

    // Contradictory exact source labels must NOT rewrite the known
    // immutable storage generation or produce false encounter verdicts.
    auto pk3WrongLabel=gen3WithBall(4);
    pk3WrongLabel.setMove(0,355);
    const auto g3Mislabeled=Legality::analyze(
        pk3WrongLabel,Enums::GameVersion::FRLG,"platinum_nds");
    assert(hasText(g3Mislabeled,
        "Exact source-game label conflicts with the verified native PK1-PK4 storage format"));
    assert(hasInvalidText(g3Mislabeled,
        "Move id 355 cannot exist in a Generation 3 save"));
    assert(!hasInvalidText(g3Mislabeled,
        "Move id 355 cannot exist in a Generation 4 save"));

    // A verified encrypted PK4 is NOT converted to PK3 by a wrong Ruby
    // source label: ball #13 and move #467 are valid Gen IV format IDs
    // even though Gen III rejects them. Ball #25 is truly impossible.
    const auto pk4WrongLabel=analyzeGen4Ball(13,false,1,467,12,"ruby_gba");
    assert(hasText(pk4WrongLabel,
        "Exact source-game label conflicts with the verified native PK1-PK4 storage format"));
    assert(!hasInvalidText(pk4WrongLabel,
        "Ball id 13 cannot exist in Generation 3"));
    assert(!hasInvalidText(pk4WrongLabel,
        "Move id 467 cannot exist in a Generation 3 save"));
    const auto pk4RealBad=analyzeGen4Ball(25,false,1,467,12,"ruby_gba");
    assert(hasInvalidText(pk4RealBad,
        "Ball id 25 cannot exist in Generation 4"));

    // Native PK3 carries a stored original Ruby game byte independently
    // from the current physical save source. Ruby Route 101 Poochyena
    // #261, map 16, level2 is a pinned original-GBA wild slot.
    constexpr auto gen3DetachedWild=
        "Source-free native PK3 met location/level matches a pinned Generation III wild encounter for the stored original GBA game";
    auto rubyPooch=gen3WithBall(4);
    rubyPooch.setSpecies(261);
    rubyPooch.setOriginGame(2); // Ruby
    rubyPooch.setMetLocation(16);
    rubyPooch.setMetLevel(2);
    rubyPooch.refreshChecksum();
    assert(hasInfo(Legality::analyze(rubyPooch,Enums::GameVersion::FRLG),
                   gen3DetachedWild));
    auto rubyWrongMap=gen3WithBall(4);
    rubyWrongMap.setSpecies(261);
    rubyWrongMap.setOriginGame(2);
    rubyWrongMap.setMetLocation(20);
    rubyWrongMap.setMetLevel(2);
    assert(!hasInfo(Legality::analyze(rubyWrongMap,Enums::GameVersion::FRLG),
                    gen3DetachedWild));
    auto fireRedWrongOrigin=gen3WithBall(4);
    fireRedWrongOrigin.setSpecies(261);
    fireRedWrongOrigin.setOriginGame(4);
    fireRedWrongOrigin.setMetLocation(16);
    fireRedWrongOrigin.setMetLevel(2);
    assert(!hasInfo(Legality::analyze(fireRedWrongOrigin,Enums::GameVersion::FRLG),
                    gen3DetachedWild));
    auto rubyWrongLevel=gen3WithBall(4);
    rubyWrongLevel.setSpecies(261);
    rubyWrongLevel.setOriginGame(2);
    rubyWrongLevel.setMetLocation(16);
    rubyWrongLevel.setMetLevel(50);
    assert(!hasInfo(Legality::analyze(rubyWrongLevel,Enums::GameVersion::FRLG),
                    gen3DetachedWild));
    auto rubyUnknownOrigin=gen3WithBall(4);
    rubyUnknownOrigin.setSpecies(261);
    rubyUnknownOrigin.setOriginGame(0);
    rubyUnknownOrigin.setMetLocation(16);
    rubyUnknownOrigin.setMetLevel(2);
    assert(!hasInfo(Legality::analyze(rubyUnknownOrigin,Enums::GameVersion::FRLG),
                    gen3DetachedWild));

    // Entire pret-derived Gen III original-source learnset classifications
    // now reach the native-PK3 -> production legality report, even when
    // the original save container is absent. Compatibility is INFO-only.
    constexpr auto pk3Direct="Source-free PK3 move is in the audited direct move pool";
    constexpr auto pk3Other="Source-free PK3 move is available in another Generation III game's audited move pool";
    auto rubyTackle=gen3WithBall(4);
    rubyTackle.setSpecies(1);
    rubyTackle.setOriginGame(2);
    rubyTackle.setMove(0,33);
    rubyTackle.refreshChecksum();
    assert(hasInfo(Legality::analyze(rubyTackle,Enums::GameVersion::FRLG),pk3Direct));

    // Bulbasaur Swords Dance is not in pret Ruby's direct pool; the
    // Emerald/FRLG source tables include it. It is cross-game possible,
    // NOT proof that a specific physical transfer actually occurred.
    auto rubyTutor=gen3WithBall(4);
    rubyTutor.setSpecies(1);
    rubyTutor.setOriginGame(2);
    rubyTutor.setMove(0,14);
    rubyTutor.refreshChecksum();
    assert(hasInfo(Legality::analyze(rubyTutor,Enums::GameVersion::FRLG),pk3Other));
    assert(!hasInfo(Legality::analyze(rubyTutor,Enums::GameVersion::FRLG),pk3Direct));

    // The same move belongs to FireRed's direct Gen III source pool.
    auto fireRedTutor=gen3WithBall(4);
    fireRedTutor.setSpecies(1);
    fireRedTutor.setOriginGame(4);
    fireRedTutor.setMove(0,14);
    fireRedTutor.refreshChecksum();
    assert(hasInfo(Legality::analyze(fireRedTutor,Enums::GameVersion::FRLG),pk3Direct));

    // A missing/unsupported original game does NOT justify claiming
    // any source game or an impossible event/egg/transfer history.
    auto unknownGameTutor=gen3WithBall(4);
    unknownGameTutor.setSpecies(1);
    unknownGameTutor.setOriginGame(0);
    unknownGameTutor.setMove(0,14);
    unknownGameTutor.refreshChecksum();
    const auto unknownReport=Legality::analyze(unknownGameTutor,Enums::GameVersion::FRLG);
    assert(!hasInfo(unknownReport,pk3Direct));
    assert(!hasInfo(unknownReport,pk3Other));
    // Exact-source behavior remains separated from the source-free path.
    assert(!hasInfo(Legality::analyze(
        rubyTutor,Enums::GameVersion::FRLG,"ruby_gba"),pk3Other));

    // Joint four-slot source-pool intersection and minimum set cover.
    // Both moves may be individually available while NO single audited
    // game pool contains both (not a verdict of impossible trade history).
    using JBound=Legality::Gen3JointMoveEvidence::Bound;
    using JSource=PokeVault::Integration::Gen3::SourceGame;
    const auto nativeSet=Legality::Gen3JointMoveEvidence::classify(
        JSource::RubyGBA,1,std::array<uint16_t,4>{33,74,0,0});
    assert(nativeSet.bound==JBound::OnePool && nativeSet.originalPoolCovers);
    const auto otherSet=Legality::Gen3JointMoveEvidence::classify(
        JSource::RubyGBA,1,std::array<uint16_t,4>{33,14,0,0});
    assert(otherSet.bound==JBound::OnePool && !otherSet.originalPoolCovers);
    const auto mixedSet=Legality::Gen3JointMoveEvidence::classify(
        JSource::FireRedGBA,4,std::array<uint16_t,4>{7,232,0,0});
    assert(mixedSet.bound==JBound::MultiplePools);
    assert(mixedSet.minimumPools==2 && mixedSet.poolMask==6);
    assert(Legality::Gen3JointMoveEvidence::classify(
        JSource::FireRedGBA,4,std::array<uint16_t,4>{7,354,0,0}).bound
        ==JBound::Unresolved);
    assert(Legality::Gen3JointMoveEvidence::classify(
        JSource::RubyGBA,1,std::array<uint16_t,4>{0,0,0,0}).bound
        ==JBound::Empty);

    rubyTackle.setMove(1,74);
    rubyTackle.refreshChecksum();
    assert(hasInfo(Legality::analyze(rubyTackle,Enums::GameVersion::FRLG),
        "joint move bound: all stored moves share the original GBA game"));
    rubyTutor.setMove(0,33);
    rubyTutor.setMove(1,14);
    rubyTutor.refreshChecksum();
    assert(hasInfo(Legality::analyze(rubyTutor,Enums::GameVersion::FRLG),
        "joint move bound: all stored moves share another Gen III game"));
    auto mixedNative=gen3WithBall(4);
    mixedNative.setSpecies(4);    // Charmander, native PK3 internal index
    mixedNative.setOriginGame(4); // FireRed
    mixedNative.setMove(0,7);     // Emerald-only for this species
    mixedNative.setMove(1,232);   // FRLG-only for this species
    mixedNative.refreshChecksum();
    const auto mixedReport=Legality::analyze(mixedNative,Enums::GameVersion::FRLG);
    assert(hasInfo(mixedReport,
        "joint move bound: at least 2 distinct Gen III game pools"));
    assert(!hasInvalidText(mixedReport,"joint move bound"));
    mixedNative.setMove(1,354);
    mixedNative.refreshChecksum();
    assert(!hasText(Legality::analyze(mixedNative,Enums::GameVersion::FRLG),
                    "joint move bound"));
    assert(!hasText(Legality::analyze(unknownGameTutor,Enums::GameVersion::FRLG),
                    "joint move bound"));
    assert(!hasText(Legality::analyze(rubyTutor,Enums::GameVersion::FRLG,"ruby_gba"),
                    "joint move bound"));

    // Source-free known native PK3/PK4 formats have generation-specific
    // species/move ceilings regardless of their exact save identity.
    const auto g3Known=Legality::analyze(gen3WithBall(4),Enums::GameVersion::FRLG);
    assert(!hasInvalidText(g3Known,"cannot exist in a Generation 3 save"));
    // Native PK3 stores a Gen III INTERNAL species index. National #387
    // has no Gen III internal mapping: setSpecies(387) returns empty,
    // not an impossible retained National-Dex value. This negative
    // control must not invent an impossible source-format specimen.
    auto g3Unsupported=gen3WithBall(4);
    g3Unsupported.setSpecies(387);
    assert(g3Unsupported.speciesID()==0);
    auto g3NewMove=gen3WithBall(4);
    g3NewMove.setMove(0,355);
    const auto g3MoveBad=Legality::analyze(g3NewMove,Enums::GameVersion::FRLG);
    assert(hasInvalidText(g3MoveBad,"Move id 355 cannot exist in a Generation 3 save"));
    auto g3LastMove=gen3WithBall(4);
    g3LastMove.setMove(0,354);
    assert(!hasInvalidText(Legality::analyze(g3LastMove,Enums::GameVersion::FRLG),
                           "Move id 354 cannot exist in a Generation 3 save"));

    // Actual encrypted PK4 reader -> immutable view -> source-free report.
    const auto g4Upper=analyzeGen4Ball(4,false,493,467);
    assert(!hasInvalidText(g4Upper,"Species 493 cannot exist"));
    assert(!hasInvalidText(g4Upper,"Move id 467 cannot exist"));
    assert(hasInvalidText(analyzeGen4Ball(4,false,494),
                          "Species 494 cannot exist in a Generation 4 save"));
    assert(hasInvalidText(analyzeGen4Ball(4,false,1,468),
                          "Move id 468 cannot exist in a Generation 4 save"));


    // Source-free native PK4 retains an original Diamond, Platinum, or
    // HGSS game code even after separation from the physical save.
    using G4Bound=Legality::Gen4JointMoveEvidence::Bound;
    constexpr auto diamond="diamond_nds";
    constexpr auto directG4="Source-free native PK4 move is in its stored original";
    constexpr auto otherG4="Source-free native PK4 move appears in another";
    const auto g4Own=Legality::Gen4JointMoveEvidence::classify(
        diamond,1,0,std::array<uint16_t,4>{33,74,0,0});
    assert(g4Own.bound==G4Bound::OnePool && g4Own.originalPoolCovers);
    const auto g4Other=Legality::Gen4JointMoveEvidence::classify(
        diamond,6,0,std::array<uint16_t,4>{29,53,0,0});
    assert(g4Other.bound==G4Bound::OnePool && !g4Other.originalPoolCovers);
    const auto g4Split=Legality::Gen4JointMoveEvidence::classify(
        diamond,6,0,std::array<uint16_t,4>{29,432,0,0});
    assert(g4Split.bound==G4Bound::MultiplePools);
    assert(g4Split.minimumPools==2 && g4Split.poolMask==5);
    assert(Legality::Gen4JointMoveEvidence::classify(
        diamond,6,0,std::array<uint16_t,4>{29,467,0,0}).bound
           ==G4Bound::Unresolved);
    assert(Legality::Gen4JointMoveEvidence::classify(
        diamond,6,0,std::array<uint16_t,4>{0,0,0,0}).bound
           ==G4Bound::Empty);
    assert(Legality::Gen4JointMoveEvidence::classify(
        "invalid_origin",6,0,std::array<uint16_t,4>{29,432,0,0}).bound
           ==G4Bound::Unresolved);

    const auto nativeDiamond=analyzeSourceFreeNativeGen4Moves(10,1,33,74);
    assert(hasInfo(nativeDiamond,directG4));
    assert(hasInfo(nativeDiamond,
        "joint move bound: all moves share the stored original Gen IV game"));
    const auto nativeOther=analyzeSourceFreeNativeGen4Moves(10,6,29,53);
    assert(hasInfo(nativeOther,otherG4));
    assert(hasInfo(nativeOther,
        "joint move bound: all moves share another Gen IV game"));
    const auto nativeSplit=analyzeSourceFreeNativeGen4Moves(10,6,29,432);
    assert(hasInfo(nativeSplit,
        "joint move bound: at least 2 distinct Gen IV game pools"));
    assert(!hasInvalidText(nativeSplit,"joint move bound"));
    const auto nativeUnmodeled=analyzeSourceFreeNativeGen4Moves(10,6,29,467);
    assert(!hasText(nativeUnmodeled,"joint move bound"));
    const auto nativeUnknownOrigin=analyzeSourceFreeNativeGen4Moves(0,6,29,432);
    assert(!hasText(nativeUnknownOrigin,"Source-free native PK4 joint move bound"));
    const auto palParkStillGen3=analyzeSourceFreeNativeGen4Moves(2,6,29,432);
    assert(!hasText(palParkStillGen3,"Source-free native PK4 joint move bound"));

    // Cross-generation historical move-STAGE evidence: Pikachu's
    // Thunderbolt #85 is in Ruby's GBA pool; #363 exists only in the PK4
    // move-ID domain and Gen IV Pikachu pool. No actual learning claim.
    using G34Source=PokeVault::Integration::Gen3::SourceGame;
    const auto palSet=Legality::Gen34PalParkMoves::classify(
        G34Source::RubyGBA,25,0,std::array<uint16_t,4>{85,363,0,0});
    assert(palSet.storedMoves==2);
    assert(palSet.originalGbaCompatible==1);
    assert(palSet.gen4StorageOnly==1);
    assert(palSet.gen4CompatibleOnly==1);
    assert(palSet.allAudited());
    const auto unknownPalSet=Legality::Gen34PalParkMoves::classify(
        G34Source::RubyGBA,25,0,std::array<uint16_t,4>{85,467,0,0});
    assert(unknownPalSet.gen4StorageOnly==1);
    assert(unknownPalSet.unmodeled==1 && !unknownPalSet.allAudited());
    const auto structurallyImpossibleMove=Legality::Gen34PalParkMoves::classify(
        G34Source::RubyGBA,25,0,std::array<uint16_t,4>{85,468,0,0});
    assert(structurallyImpossibleMove.gen4StorageOnly==0);
    assert(structurallyImpossibleMove.unmodeled==1);
    assert(!structurallyImpossibleMove.allAudited());

    // A four-slot HISTORICAL LOWER BOUND, not just separate GBA-vs-NDS
    // move count. Pikachu's Body Slam #34 is in the pinned Gen III
    // pools but NOT any Gen IV pool. #363 is Gen IV-only. Both source
    // eras are required in the audited union tables, not necessarily a
    // verified historical sequence. Known unmodeled moves stay unknown.
    using Joint=Legality::Gen34PalParkMoves::JointBound;
    const auto mixedEra=Legality::Gen34PalParkMoves::classifyJoint(
        25,0,std::array<uint16_t,4>{34,363,0,0});
    assert(mixedEra.bound==Joint::MultiplePools);
    assert(mixedEra.minimumPools==2 && mixedEra.requiresBothEras);
    const auto singleEra=Legality::Gen34PalParkMoves::classifyJoint(
        25,0,std::array<uint16_t,4>{85,363,0,0});
    assert(singleEra.bound==Joint::OnePool);
    assert(singleEra.minimumPools==1 && !singleEra.requiresBothEras);
    assert(Legality::Gen34PalParkMoves::classifyJoint(
        25,0,std::array<uint16_t,4>{34,468,0,0}).bound==
        Joint::Unresolved);
    assert(Legality::Gen34PalParkMoves::classifyJoint(
        25,0,std::array<uint16_t,4>{0,0,0,0}).bound==
        Joint::Empty);

    const auto mixedEraSourceFree=analyzeDetachedPalParkMoves(2,34,363);
    assert(hasInfo(mixedEraSourceFree,
        "Source-free Pal Park PK4 joint history bound: at least 2 distinct"));
    assert(hasInfo(mixedEraSourceFree,
        "joint history bound: the audited move pools require both a Gen III-era and a Gen IV-era source"));
    const auto mixedEraExact=analyzeDetachedPalParkMoves(2,34,363,true);
    assert(hasInfo(mixedEraExact,
        "Exact-source Pal Park PK4 joint history bound: at least 2 distinct"));
    assert(hasInfo(mixedEraExact,
        "joint history bound: the audited move pools require both a Gen III-era and a Gen IV-era source"));
    const auto mixedEraUnknown=analyzeDetachedPalParkMoves(2,34,468);
    assert(!hasText(mixedEraUnknown,"joint history bound"));
    const auto mixedEraBadMarker=analyzeDetachedPalParkMoves(
        2,34,363,false,true,false,true);
    assert(!hasText(mixedEraBadMarker,"joint history bound"));
    constexpr auto stagePrefix="Source-free Pal Park PK4 move-stage evidence:";
    const auto actualPalMoves=analyzeDetachedPalParkMoves(2,85,363);
    assert(hasInfo(actualPalMoves,
        "stored move(s) exceed the Gen III PK3 move-ID ceiling"));
    assert(hasInfo(actualPalMoves,
        "stored move(s) occur in the audited original GBA game"));
    assert(hasInfo(actualPalMoves,
        "all nonempty move slots have at least one audited Gen III or IV"));
    assert(!hasInvalidText(actualPalMoves,stagePrefix));
    // A Pal Park marker in the extended field is insufficient when
    // either the D/P split location or the Gen III ball bytes contradict
    // the transfer. Structural Invalid remains independently reported.
    using namespace Legality::Gen34PalParkMoves;
    static_assert(verifiedTransferFields(0x37,0x37,4,0,false));
    static_assert(verifiedTransferFields(0x37,0,4,0,false));
    static_assert(!verifiedTransferFields(0x36,0x37,4,0,false));
    static_assert(!verifiedTransferFields(0x37,0x37,13,0,false));
    static_assert(!verifiedTransferFields(0x37,0x37,4,0,true));
    const auto badSplitStage=analyzeDetachedPalParkMoves(
        2,85,363,false,true,false,true);
    assert(hasInvalidText(badSplitStage,"inconsistent D/P and Pt/HGSS"));
    assert(!hasText(badSplitStage,stagePrefix));
    const auto badBallStage=analyzeDetachedPalParkMoves(
        2,85,363,false,true,true,false);
    assert(hasInvalidText(badBallStage,"split-ball fields are inconsistent"));
    assert(!hasText(badBallStage,stagePrefix));
    // Move ID 468 cannot exist in native Gen IV, so it must never be
    // presented as a legitimate post-migration acquisition stage.
    const auto impossibleSourceFree=analyzeDetachedPalParkMoves(2,85,468);
    assert(hasInvalidText(impossibleSourceFree,
        "Move id 468 cannot exist in a Generation 4 save"));
    assert(!hasInfo(impossibleSourceFree,
        "stored move(s) exceed the Gen III PK3 move-ID ceiling"));
    assert(!hasInfo(impossibleSourceFree,
        "all nonempty move slots have at least one audited Gen III or IV"));
    const auto impossibleExact=analyzeDetachedPalParkMoves(2,85,468,true);
    assert(hasInvalidText(impossibleExact,
        "Move id 468 cannot exist in a Generation 4 save"));
    assert(!hasInfo(impossibleExact,
        "move(s) exceed the Gen III PK3 move-ID ceiling and require post-migration"));
    const auto unknownPal=analyzeDetachedPalParkMoves(2,85,467);
    assert(!hasInfo(unknownPal,
        "all nonempty move slots have at least one audited Gen III or IV"));
    // A non-Pal-Park native Gen IV origin, an absent transfer marker and an
    // exact physical source cannot inherit source-free stage assertions.
    assert(!hasText(analyzeDetachedPalParkMoves(10,85,363),
                    stagePrefix));
    assert(!hasText(analyzeDetachedPalParkMoves(2,85,363,false,false),
                    stagePrefix));
    const auto exactPalMoves=analyzeDetachedPalParkMoves(2,85,363,true);
    assert(!hasText(exactPalMoves,stagePrefix));
    assert(hasInfo(exactPalMoves,
        "Exact-source Pal Park PK4 move-stage evidence: 1 move(s) exceed the Gen III PK3 move-ID ceiling"));
    assert(hasInfo(exactPalMoves,
        "move(s) occur in the audited retained original GBA game pool"));
    assert(hasInfo(exactPalMoves,
        "all nonempty move slots have a candidate Gen III or IV"));
    assert(!hasInvalidText(exactPalMoves,
        "Exact-source Pal Park PK4 move-stage evidence"));
    const auto exactSplitInvalid=analyzeDetachedPalParkMoves(
        2,85,363,true,true,false,true);
    assert(!hasText(exactSplitInvalid,"Exact-source Pal Park PK4 move-stage evidence"));
    const auto exactBallInvalid=analyzeDetachedPalParkMoves(
        2,85,363,true,true,true,false);
    assert(!hasText(exactBallInvalid,"Exact-source Pal Park PK4 move-stage evidence"));
    const auto exactNoMarker=analyzeDetachedPalParkMoves(
        2,85,363,true,false);
    assert(!hasText(exactNoMarker,"Exact-source Pal Park PK4 move-stage evidence"));
    assert(!hasText(analyzeDetachedPalParkMoves(0,85,363),
                    stagePrefix));

    // Pal Park retains the source generation after legitimate in-game
    // evolution. National species >386 alone is NOT an invalid Gen III
    // provenance claim: Magneton #82 -> Magnezone #462 and Porygon2
    // #233 -> Porygon-Z #474 become possible after entering Gen IV.
    // Unrelated Gen IV starter/new lineages cannot originate from GBA.
    using Lineage=Legality::Gen34PalParkSpecies::Evidence;
    static_assert(Legality::Gen34PalParkSpecies::classify(386)==Lineage::Gen3NativeSpecies);
    static_assert(Legality::Gen34PalParkSpecies::classify(462)==Lineage::Gen4EvolutionFromGen3);
    static_assert(Legality::Gen34PalParkSpecies::classify(474)==Lineage::Gen4EvolutionFromGen3);
    static_assert(Legality::Gen34PalParkSpecies::classify(473)==Lineage::Gen4EvolutionFromGen3);
    static_assert(Legality::Gen34PalParkSpecies::classify(387)==Lineage::NoGen3Ancestor);
    static_assert(Legality::Gen34PalParkSpecies::classify(445)==Lineage::NoGen3Ancestor);
    static_assert(Legality::Gen34PalParkSpecies::classify(493)==Lineage::NoGen3Ancestor);
    static_assert(Legality::Gen34PalParkSpecies::classify(494)==Lineage::Unknown);

    static_assert(Legality::Gen34PalParkSpecies::nearestGen3Ancestor(82)==82);
    static_assert(Legality::Gen34PalParkSpecies::nearestGen3Ancestor(462)==82);
    static_assert(Legality::Gen34PalParkSpecies::nearestGen3Ancestor(474)==233);
    static_assert(Legality::Gen34PalParkSpecies::nearestGen3Ancestor(473)==221);
    static_assert(Legality::Gen34PalParkSpecies::nearestGen3Ancestor(387)==0);
    static_assert(Legality::Gen34PalParkSpecies::nearestGen3Ancestor(445)==0);

    // The same evolved PK4 can preserve the ancestral GBA move #38
    // (Magneton in Emerald/FRLG, not R/S) and later learn Gen IV move #363 as Magnezone.
    using StageSource=PokeVault::Integration::Gen3::SourceGame;
    const auto evolvedStage=Legality::Gen34PalParkMoves::classify(
        StageSource::EmeraldGBA,462,0,{38,363,0,0});
    assert(evolvedStage.storedMoves==2);
    assert(evolvedStage.originalGbaCompatible==1);
    assert(evolvedStage.gen4StorageOnly==1);
    assert(evolvedStage.allAudited());
    const auto evolvedJoint=Legality::Gen34PalParkMoves::classifyJoint(
        462,0,{38,363,0,0});
    assert(evolvedJoint.bound==Legality::Gen34PalParkMoves::JointBound::MultiplePools);
    assert(evolvedJoint.minimumPools==2 && evolvedJoint.requiresBothEras);
    const auto noAncestorStage=Legality::Gen34PalParkMoves::classify(
        StageSource::RubyGBA,387,0,{38,363,0,0});
    assert(noAncestorStage.storedMoves==0 && !noAncestorStage.allAudited());
    assert(Legality::Gen34PalParkMoves::classifyJoint(
        387,0,{38,363,0,0}).bound==
        Legality::Gen34PalParkMoves::JointBound::Unresolved);

    const auto evolvedStageNative=analyzeDetachedPalParkMoves(
        3,38,363,false,true,false,false,462);
    assert(hasInfo(evolvedStageNative,
        "stored move(s) occur in the audited original GBA game"));
    assert(hasInfo(evolvedStageNative,
        "joint history bound: the audited move pools require both a Gen III-era and a Gen IV-era source"));
    const auto evolvedStageExact=analyzeDetachedPalParkMoves(
        3,38,363,true,true,false,false,462);
    assert(hasInfo(evolvedStageExact,
        "move(s) occur in the audited retained original GBA game pool"));
    assert(hasInfo(evolvedStageExact,
        "joint history bound: the audited move pools require both a Gen III-era and a Gen IV-era source"));
    const auto noAncestorNative=analyzeDetachedPalParkMoves(
        2,38,363,false,true,false,false,387);
    assert(!hasText(noAncestorNative,"joint history bound"));
    assert(!hasInfo(noAncestorNative,
        "move-stage evidence: all nonempty move slots"));

    constexpr auto noGbaLineage=
        "Gen III-origin PK4 species has no Gen III species in its audited Generation IV evolution lineage";
    constexpr auto evolvedAfter=
        "can descend from a Gen III species via Generation IV evolution";
    const auto turtwigSourceFree=sourceFreePalPark(2,0x37,0x37,4,0,false,387);
    assert(hasInvalidText(turtwigSourceFree,noGbaLineage));
    assert(!hasInfo(turtwigSourceFree,evolvedAfter));
    const auto magnezoneSourceFree=sourceFreePalPark(2,0x37,0x37,4,0,false,462);
    assert(!hasInvalidText(magnezoneSourceFree,noGbaLineage));
    assert(hasInfo(magnezoneSourceFree,evolvedAfter));
    const auto porygonZSourceFree=sourceFreePalPark(2,0x37,0x37,4,0,false,474);
    assert(!hasInvalidText(porygonZSourceFree,noGbaLineage));
    assert(hasInfo(porygonZSourceFree,evolvedAfter));
    assert(hasInvalidText(
        analyzeDetachedPalParkMoves(2,85,363,true,true,false,false,387),
        noGbaLineage));
    assert(!hasInvalidText(
        analyzeDetachedPalParkMoves(2,85,363,true,true,false,false,462),
        noGbaLineage));
    assert(hasInfo(
        analyzeDetachedPalParkMoves(2,85,363,true,true,false,false,462),
        evolvedAfter));
    // Origin version alone is insufficient: event/special-origin PK4
    // records without valid Pal Park structural fields must not inherit
    // an impossible-GBA-ancestor verdict from the origin byte alone.
    const auto noPalParkLineage=sourceFreePalPark(
        2,0,0,4,0,false,387);
    assert(!hasInvalidText(noPalParkLineage,noGbaLineage));
    const auto contradictoryBallLineage=sourceFreePalPark(
        2,0x37,0x37,13,0,false,387);
    assert(!hasInvalidText(contradictoryBallLineage,noGbaLineage));
    const auto contradictorySplitLineage=sourceFreePalPark(
        2,0x36,0x37,4,0,false,387);
    assert(!hasInvalidText(contradictorySplitLineage,noGbaLineage));
    assert(!hasInvalidText(
        analyzeDetachedPalParkMoves(2,85,363,true,false,false,false,387),
        noGbaLineage));
    // Without a Gen III origin byte, never infer the ancestry constraint.
    assert(!hasInvalidText(
        analyzeDetachedPalParkMoves(10,85,363,false,true,false,false,387),
        noGbaLineage));

    // Native Gen III -> IV Pal Park records retain origin and split
    // fields independently of an exact container game/source save ID.
    const auto dpTransfer=sourceFreePalPark(2,0x37,0,4,0);
    assert(hasInfo(dpTransfer,"Pal Park D/P split-field pattern"));
    assert(!hasInvalidText(dpTransfer,"Pal Park"));

    const auto hgssTransfer=sourceFreePalPark(2,0x37,0x37,4,4);
    assert(hasInfo(hgssTransfer,"Pal Park Pt/HGSS split-field pattern"));
    assert(!hasInvalidText(hgssTransfer,"Pal Park"));

    const auto badMarker=sourceFreePalPark(2,0,0,4,0);
    assert(hasInvalidText(badMarker,
        "Gen III-origin PK4 is missing the Pal Park transfer met location"));
    const auto splitMismatch=sourceFreePalPark(2,0x37,0x36,4,4);
    assert(hasInvalidText(splitMismatch,
        "Gen III-origin PK4 has inconsistent D/P and Pt/HGSS Pal Park location fields"));
    const auto badBall=sourceFreePalPark(2,0x37,0x37,13,0);
    assert(hasInvalidText(badBall,
        "Gen III -> IV Pal Park split-ball fields are inconsistent"));
    const auto eggCannotTransfer=sourceFreePalPark(2,0x37,0x37,4,4,true);
    assert(hasInvalidText(eggCannotTransfer,
        "Gen III-origin egg cannot be transferred through Pal Park"));
    const auto unknownTransferOrigin=sourceFreePalPark(0,0,0,4,0);
    assert(hasText(unknownTransferOrigin,
        "PK4 origin game could not be mapped to a known generation"));
    assert(!hasInvalidText(unknownTransferOrigin,"Pal Park"));
    // Later-generation origin is intrinsically impossible in a PK4,
    // independent of which Gen IV game save contains the entity.
    const auto gen5Origin=sourceFreePalPark(20,0x37,0x37,4,4);
    assert(hasInvalidText(gen5Origin,
        "cannot be stored directly in a retail Generation IV PK4"));

    // Exact Gen III: 12 is the pinned generation maximum, 13 is impossible.
    auto gen3Max = gen3WithBall(12);
    const auto g3Max = Legality::analyze(gen3Max, Enums::GameVersion::FRLG, "ruby_gba");
    assert(!hasText(g3Max, "Ball id 12 cannot exist in Generation 3"));

    auto gen3TooHigh = gen3WithBall(13);
    const auto g3Bad = Legality::analyze(gen3TooHigh, Enums::GameVersion::FRLG, "ruby_gba");
    assert(hasText(g3Bad, "Ball id 13 cannot exist in Generation 3"));
    assert(g3Bad.hasInvalid());

    auto gen3Zero = gen3WithBall(0);
    const auto g3Zero = Legality::analyze(gen3Zero, Enums::GameVersion::FRLG, "ruby_gba");
    assert(!hasText(g3Zero, "Ball id 0 cannot exist in Generation 3"));

    // Exact Gen IV: Sport Ball (24) is within the stored domain; Dream Ball (25)
    // did not exist until Gen V and is impossible in a native PK4 field.
    const auto g4Max = analyzeGen4Ball(24);
    assert(!hasText(g4Max, "Ball id 24 cannot exist in Generation 4"));

    const auto g4Bad = analyzeGen4Ball(25);
    assert(hasText(g4Bad, "Ball id 25 cannot exist in Generation 4"));
    assert(g4Bad.hasInvalid());

    const auto g4Zero = analyzeGen4Ball(0);
    assert(!hasText(g4Zero, "Ball id 0 cannot exist in Generation 4"));

    // Without an exact game source, the immutable encrypted PK4 format
    // still proves the Generation IV Ball maximum; this does NOT prove
    // any particular encounter/ball provenance.
    const auto generic24 = analyzeGen4Ball(24, false);
    assert(!hasText(generic24,"Ball id 24 cannot exist in Generation 4"));
    const auto generic25 = analyzeGen4Ball(25, false);
    assert(hasText(generic25,"Ball id 25 cannot exist in Generation 4"));
    assert(generic25.hasInvalid());

    const auto generic38 = analyzeGen4Ball(38, false);
    assert(hasText(generic38, "Ball id 38 cannot exist in Generation 4"));
    assert(generic38.hasInvalid());
    const auto genericPk3Good=Legality::analyze(
        gen3WithBall(12), Enums::GameVersion::FRLG);
    assert(!hasText(genericPk3Good,"Ball id 12 cannot exist in Generation 3"));
    const auto genericPk3Bad=Legality::analyze(
        gen3WithBall(13), Enums::GameVersion::FRLG);
    assert(hasText(genericPk3Bad,"Ball id 13 cannot exist in Generation 3"));
    assert(genericPk3Bad.hasInvalid());

    // Real encrypted native HGSS PK4, not a synthetic PK3 interface.
    // These checks exercise the production source/provenance report path,
    // which is INFO only and does not create Ball-source hard Invalid.
    const auto hgSport = analyzeNativeHgssSpecialBall(14, 207, 15, 24);
    assert(hasInfo(hgSport,
        "Sport Ball has a compatible HeartGold/SoulSilver Bug-Catching Contest source"));
    // Native encrypted HGSS PK4 Apricorn Ball -> read-only view ->
    // exact source analyzer: actual HeartGold Pidgey at location149/level2.
    // Evolved Pidgeot has no direct native Safari capture row at
    // location202/level15, but its Pidgey ancestor does. High stored EXP
    // makes a level-qualified evolved state more realistic in this fixture.
    // Source-aware native PK4 Wurmple branch counterexample:
    // Beautifly from HGSS Headbutt Wurmple level2 in Apricorn Ball.
    // PID high word 1 selects Silcoon/Beautifly, high6 selects
    // Cascoon/Dustox, so the latter cannot support this ancestry.
    const auto nativeBeauty=analyzeNativeHgssSpecialBall(
        267,138,2,17,7,200000,0x00010000u);
    const auto nativeWrongBeauty=analyzeNativeHgssSpecialBall(
        267,138,2,17,7,200000,0x00060000u);
    assert(hasInfo(nativeBeauty,
        "special Ball has a compatible wild pre-evolution capture source"));
    assert(!hasInfo(nativeWrongBeauty,
        "special Ball has a compatible wild pre-evolution capture source"));

    const auto evolvedSafari = analyzeNativeHgssSpecialBall(
        18, 202, 15, 5, 7, 200000);
    assert(hasInfo(evolvedSafari,
        "special Ball has a compatible wild pre-evolution capture source"));
    const auto evolvedApricorn = analyzeNativeHgssSpecialBall(
        18, 149, 2, 17, 7, 200000);
    assert(hasInfo(evolvedApricorn,
        "special Ball has a compatible wild pre-evolution capture source"));
    // A hatched Gen IV PK4 retains a nonzero egg-location. Even a
    // malformed record that also claims a wild met level/location must
    // NOT receive positive wild ancestry evidence by accident.
    const auto hatchedEvolved = analyzeNativeHgssSpecialBall(
        18,202,15,5,7,200000,0x12345678u,2000);
    assert(!hasInfo(hatchedEvolved,
        "special Ball has a compatible wild pre-evolution capture source"));

    const auto evolvedWrongGame = analyzeNativeHgssSpecialBall(
        18, 202, 15, 5, 12, 200000);
    assert(!hasInfo(evolvedWrongGame,
        "special Ball has a compatible wild pre-evolution capture source"));
    const auto evolvedWrongLevel = analyzeNativeHgssSpecialBall(
        18, 202, 1, 5, 7, 200000);
    assert(!hasInfo(evolvedWrongLevel,
        "special Ball has a compatible wild pre-evolution capture source"));
    const auto evolvedWrongBall = analyzeNativeHgssSpecialBall(
        18, 202, 15, 24, 7, 200000);
    assert(!hasInfo(evolvedWrongBall,
        "special Ball has a compatible wild pre-evolution capture source"));

    // Strict encrypted PK4 → immutable reader → shared view → report
    // WITHOUT original save identity. Stored HGSS retail origin still
    // supplies positive encounter and Kurt Apricorn Ball evidence.
    constexpr auto detachedWild="Source-free native PK4 met data matches a pinned Generation IV wild encounter slot for its stored retail origin";
    constexpr auto detachedApricorn="Source-free PK4 Apricorn Ball has a compatible HGSS wild encounter source";
    const auto detachedPidgey=analyzeNativeHgssSpecialBall(
        16,149,2,17,7,3375,0x12345678u,0,false);
    assert(hasInfo(detachedPidgey,detachedWild));
    assert(hasInfo(detachedPidgey,detachedApricorn));
    const auto detachedPidgeyWrongOrigin=analyzeNativeHgssSpecialBall(
        16,149,2,17,12,3375,0x12345678u,0,false);
    assert(!hasInfo(detachedPidgeyWrongOrigin,detachedWild));
    assert(!hasInfo(detachedPidgeyWrongOrigin,detachedApricorn));
    const auto detachedPidgeyUnknown=analyzeNativeHgssSpecialBall(
        16,149,2,17,0,3375,0x12345678u,0,false);
    assert(!hasInfo(detachedPidgeyUnknown,detachedWild));
    const auto detachedPidgeyWrongLevel=analyzeNativeHgssSpecialBall(
        16,149,1,17,7,3375,0x12345678u,0,false);
    assert(!hasInfo(detachedPidgeyWrongLevel,detachedWild));
    const auto detachedPidgeyEgg=analyzeNativeHgssSpecialBall(
        16,149,2,17,7,3375,0x12345678u,2002,false);
    assert(!hasInfo(detachedPidgeyEgg,detachedWild));
    const auto detachedPidgeyWrongBall=analyzeNativeHgssSpecialBall(
        16,149,2,4,7,3375,0x12345678u,0,false);
    assert(hasInfo(detachedPidgeyWrongBall,detachedWild));
    assert(!hasInfo(detachedPidgeyWrongBall,detachedApricorn));

    const auto apricorn = analyzeNativeHgssSpecialBall(16, 149, 2, 17);
    assert(hasInfo(apricorn,
        "Apricorn Ball has a compatible HeartGold/SoulSilver wild encounter source"));
    const auto apricornMoon = analyzeNativeHgssSpecialBall(16, 149, 2, 23);
    assert(hasInfo(apricornMoon,
        "Apricorn Ball has a compatible HeartGold/SoulSilver wild encounter source"));
    const auto apricornWrongLevel = analyzeNativeHgssSpecialBall(16, 149, 1, 17);
    assert(!hasInfo(apricornWrongLevel,
        "Apricorn Ball has a compatible HeartGold/SoulSilver"));
    const auto apricornWrongOrigin = analyzeNativeHgssSpecialBall(16, 149, 2, 17, 12);
    assert(!hasInfo(apricornWrongOrigin,
        "Apricorn Ball has a compatible HeartGold/SoulSilver"));
    const auto apricornWrongBall = analyzeNativeHgssSpecialBall(16, 149, 2, 16);
    assert(!hasInfo(apricornWrongBall,
        "Apricorn Ball has a compatible HeartGold/SoulSilver"));

    const auto hgSafari = analyzeNativeHgssSpecialBall(16, 202, 15, 5);
    assert(hasInfo(hgSafari,
        "Safari Ball has a compatible HeartGold/SoulSilver Safari Zone encounter source"));

    // Mismatched, unwired or wrong native-origin fields are unproven,
    // rather than being declared illegal due to source absence.
    const auto wrongSportBall = analyzeNativeHgssSpecialBall(14, 207, 15, 5);
    assert(!hasInfo(wrongSportBall,
        "Sport Ball has a compatible HeartGold/SoulSilver"));
    const auto wrongSafariBall = analyzeNativeHgssSpecialBall(16, 202, 15, 24);
    assert(!hasInfo(wrongSafariBall,
        "Safari Ball has a compatible HeartGold/SoulSilver"));
    const auto wrongMet = analyzeNativeHgssSpecialBall(14, 207, 1, 24);
    assert(!hasInfo(wrongMet,
        "Sport Ball has a compatible HeartGold/SoulSilver"));
    const auto platinumOrigin = analyzeNativeHgssSpecialBall(14, 207, 15, 24, 12);
    assert(!hasInfo(platinumOrigin,
        "Sport Ball has a compatible HeartGold/SoulSilver"));
    // Native encrypted Diamond PK4 -> read-only view -> source-aware
    // A real encrypted PK4 preserves its original Diamond gift Eevee
    // origin code, location and met level after detaching from its save.
    // INFO compatibility is reportable without claiming gift receipt.
    constexpr auto detachedGift="Source-free native PK4 fields are compatible with a pinned Generation IV static or gift encounter for its stored retail origin";
    const auto diamondGift=analyzeNativeDiamondMarshBall(
        133,10,5,4,10,3375,false);
    assert(hasInfo(diamondGift,detachedGift));
    const auto diamondGiftEvolved=analyzeNativeDiamondMarshBall(
        134,10,5,4,10,3375,false);
    assert(hasInfo(diamondGiftEvolved,detachedGift));
    const auto platinumWrongLevel=analyzeNativeDiamondMarshBall(
        133,10,5,4,12,3375,false);
    assert(!hasInfo(platinumWrongLevel,detachedGift));
    const auto unknownGiftOrigin=analyzeNativeDiamondMarshBall(
        133,10,5,4,0,3375,false);
    assert(!hasInfo(unknownGiftOrigin,detachedGift));
    const auto wrongGiftLocation=analyzeNativeDiamondMarshBall(
        133,11,5,4,10,3375,false);
    assert(!hasInfo(wrongGiftLocation,detachedGift));

    // Legality::analyze: Great Marsh is location 52, not HGSS Safari 202.
    // Native encrypted HGSS PK4: current Shedinja is not directly caught,
    // but a pinned Nincada Bug-Catching Contest ancestor at location207,
    // level26-36 can explain either a Sport or Poké Ball.
    const auto shedSport = analyzeNativeHgssSpecialBall(292, 207, 26, 24);
    assert(hasInfo(shedSport,
        "Shedinja's Sport or Poke Ball has a compatible HeartGold/SoulSilver Bug-Catching Contest Nincada pre-evolution origin"));
    const auto shedPoke = analyzeNativeHgssSpecialBall(292, 207, 26, 4);
    assert(hasInfo(shedPoke,
        "Shedinja's Sport or Poke Ball has a compatible HeartGold/SoulSilver Bug-Catching Contest Nincada pre-evolution origin"));
    // Shedinja also requires a native wild-captured Nincada source;
    // a retained egg location must not be misreported as BCC ancestry.
    const auto shedHatchedEgg = analyzeNativeHgssSpecialBall(
        292,207,26,24,7,200000,0x12345678u,2000);
    assert(!hasInfo(shedHatchedEgg,
        "Shedinja's Sport or Poke Ball has a compatible"));

    const auto shedBadLocation = analyzeNativeHgssSpecialBall(292, 206, 26, 24);
    assert(!hasInfo(shedBadLocation,
        "Shedinja's Sport or Poke Ball has a compatible"));
    const auto shedBadLevel = analyzeNativeHgssSpecialBall(292, 207, 1, 24);
    assert(!hasInfo(shedBadLevel,
        "Shedinja's Sport or Poke Ball has a compatible"));
    const auto shedWrongBall = analyzeNativeHgssSpecialBall(292, 207, 26, 5);
    assert(!hasInfo(shedWrongBall,
        "Shedinja's Sport or Poke Ball has a compatible"));
    const auto shedWrongOrigin = analyzeNativeHgssSpecialBall(292, 207, 26, 24, 12);
    assert(!hasInfo(shedWrongOrigin,
        "Shedinja's Sport or Poke Ball has a compatible"));

    // Evolved Diamond Great Marsh history: Golduck #55 retains Psyduck
    // #54's original Safari Ball, location52 and met level20.
    const auto evolvedMarsh = analyzeNativeDiamondMarshBall(
        55,52,20,5,10,200000);
    assert(hasInfo(evolvedMarsh,
        "special Ball has a compatible wild pre-evolution capture source"));
    const auto evolvedMarshWrongBall = analyzeNativeDiamondMarshBall(
        55,52,20,24,10,200000);
    assert(!hasInfo(evolvedMarshWrongBall,
        "special Ball has a compatible wild pre-evolution capture source"));
    const auto evolvedMarshWrongLevel = analyzeNativeDiamondMarshBall(
        55,52,1,5,10,200000);
    assert(!hasInfo(evolvedMarshWrongLevel,
        "special Ball has a compatible wild pre-evolution capture source"));
    const auto evolvedMarshWrongOrigin = analyzeNativeDiamondMarshBall(
        55,52,20,5,7,200000);
    assert(!hasInfo(evolvedMarshWrongOrigin,
        "special Ball has a compatible wild pre-evolution capture source"));

    const auto marshPositive = analyzeNativeDiamondMarshBall(24, 52, 20, 5);
    assert(hasInfo(marshPositive,
        "Safari Ball has a compatible Diamond/Pearl/Platinum Great Marsh encounter source"));
    const auto marshWrongBall = analyzeNativeDiamondMarshBall(24, 52, 20, 4);
    assert(!hasInfo(marshWrongBall,
        "Safari Ball has a compatible Diamond/Pearl/Platinum Great Marsh"));
    const auto marshWrongLevel = analyzeNativeDiamondMarshBall(24, 52, 1, 5);
    assert(!hasInfo(marshWrongLevel,
        "Safari Ball has a compatible Diamond/Pearl/Platinum Great Marsh"));
    const auto marshWrongOrigin = analyzeNativeDiamondMarshBall(24, 52, 20, 5, 7);
    assert(!hasInfo(marshWrongOrigin,
        "Safari Ball has a compatible Diamond/Pearl/Platinum Great Marsh"));

    return 0;
}
