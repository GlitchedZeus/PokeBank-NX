#include "Encryption/Encryption4.h"
#include "Legality/Legality.h"
#include "Legality/Gen4TradeEvidence.h"
#include "Legality/Gen3JointMoveEvidence.h"
#include "Legality/Gen3ShedinjaEvolutionMoveEvidence.h"
#include "Legality/Gen3EmeraldLevelUpEvidence.h"
#include "Legality/Gen3EmeraldLevelUpTimeline.h"
#include "Legality/Gen3EmeraldPalParkJointPath.h"
#include "Legality/Gen34PostPalParkLearning.h"
#include "Legality/Gen3EmeraldCandidateMoveEvidence.h"
#include "Legality/Gen34OrdinaryEggMoveConstruction.h"
#include "Legality/Gen4NativeBreedingParentEvidence.h"
#include "Legality/Gen4JointEggCandidateEvidence.h"
#include "Legality/Gen4EggCandidateUniverse.h"
#include "Legality/Gen4ContestRibbonEvidence.h"
#include "Legality/Gen4RawTextEvidence.h"
#include "Legality/Gen4JointMoveEvidence.h"
#include "Legality/Gen34PalParkMoves.h"
#include "Legality/Gen34PalParkHMHistory.h"
#include "Legality/Gen34PalParkSpecies.h"
#include "Legality/Gen4RangerManaphy.h"
#include "Legality/Gen4EventTemplate.h"
#include "Legality/Gen4EventRibbonCandidate.h"
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

Legality::Report analyzeNativeGen4RawText(
        bool nicknameTerminates,bool otTerminates,bool withTrailing,
        bool exactSource=false) {
    std::vector<std::byte> raw(Encryption::SIZE_STORED4,std::byte{0});
    wr32(raw,0x00,0x12345678u);
    wr16(raw,0x08,1);  // Bulbasaur native PK4
    wr16(raw,0x0C,12345);
    wr16(raw,0x0E,54321);
    wr32(raw,0x10,135);
    raw[0x17]=std::byte{2};
    raw[0x5F]=std::byte{12};
    raw[0x83]=std::byte{4};
    wr16(raw,0x48,0x0041);
    wr16(raw,0x68,0x0052);
    if(nicknameTerminates)wr16(raw,0x4A,0xFFFFu);
    if(otTerminates)wr16(raw,0x6A,0xFFFFu);
    if(withTrailing) {
        wr16(raw,0x4C,0xBEEFu);
        wr16(raw,0x6C,0xA5A5u);
    }
    const auto enc=Encryption::encryptArray4(raw);
    Pokemon::Pokemon4ReadOnly native(enc,Enums::GameVersion::PT);
    assert(native.valid() && native.checksumValid());
    Pokemon::Pokemon4ReadOnlyView view(native);
    std::array<std::byte,22> nick{};
    std::array<std::byte,16> ot{};
    assert(view.gen4RawTextFields(nick,ot));
    assert(nick[0]==std::byte{0x41} && ot[0]==std::byte{0x52});
    const auto evidence=Legality::Gen4RawTextEvidence::classify(true,nick,ot);
    assert(evidence.nickname.terminatorPresent==nicknameTerminates);
    assert(evidence.originalTrainer.terminatorPresent==otTerminates);
    if(nicknameTerminates && otTerminates && withTrailing) {
        assert(evidence.nickname.postTerminatorNonzero);
        assert(evidence.originalTrainer.postTerminatorNonzero);
        assert(nick[4]==std::byte{0xEF} && ot[4]==std::byte{0xA5});
    }
    return exactSource
        ? Legality::analyze(view,Enums::GameVersion::PT,"platinum_nds")
        : Legality::analyze(view,Enums::GameVersion::PT);
}

Legality::Report analyzeNativeGen4RibbonWords(uint32_t a,uint32_t b,uint32_t c) {
    std::vector<std::byte> raw(Encryption::SIZE_STORED4,std::byte{0});
    wr32(raw,0x00,0x12345678u);
    wr16(raw,0x08,1);
    wr16(raw,0x0C,12345);
    wr16(raw,0x0E,54321);
    wr32(raw,0x10,135);
    wr32(raw,0x24,a);
    wr32(raw,0x3C,b);
    wr32(raw,0x60,c);
    raw[0x17]=std::byte{2};
    raw[0x5F]=std::byte{12};
    raw[0x83]=std::byte{4};
    const auto enc=Encryption::encryptArray4(raw);
    Pokemon::Pokemon4ReadOnly pk(enc,Enums::GameVersion::PT);
    assert(pk.valid() && pk.checksumValid());
    Pokemon::Pokemon4ReadOnlyView view(pk);
    std::array<uint32_t,3> observed{};
    assert(view.gen4RawRibbonWords(observed));
    assert((observed==std::array<uint32_t,3>{a,b,c}));
    return Legality::analyze(view,Enums::GameVersion::PT);
}

Legality::Report analyzeNativeGen4ContestRanks(
        uint32_t ranks,bool exactSource=false) {
    std::vector<std::byte> raw(Encryption::SIZE_STORED4,std::byte{0});
    wr32(raw,0x00,0x12345678u);
    wr16(raw,0x08,1);
    wr16(raw,0x0C,12345);
    wr16(raw,0x0E,54321);
    wr32(raw,0x10,135);
    wr32(raw,0x60,ranks);
    raw[0x17]=std::byte{2};
    raw[0x5F]=std::byte{12};
    raw[0x83]=std::byte{4};
    const auto enc=Encryption::encryptArray4(raw);
    Pokemon::Pokemon4ReadOnly native(enc,Enums::GameVersion::PT);
    assert(native.valid() && native.checksumValid());
    assert((native.ribbons()[2]&0xFFFFFu)==(ranks&0xFFFFFu));
    Pokemon::Pokemon4ReadOnlyView view(native);
    std::array<uint8_t,5> decoded{};
    assert(view.gen4ContestRibbonRanks(decoded));
    assert(decoded[0]==(ranks&0xFu));
    return exactSource
        ? Legality::analyze(view,Enums::GameVersion::PT,"platinum_nds")
        : Legality::analyze(view,Enums::GameVersion::PT);
}

Legality::Report analyzeNativeGen4EggMoveFamilies(
        uint8_t originVersion,uint16_t first,uint16_t second,
        bool exactSource=false,bool realEggOrigin=true) {
    // Real encrypted native PK4 -> read-only parser -> production analysis.
    std::vector<std::byte> raw(Encryption::SIZE_STORED4,std::byte{0});
    wr32(raw,0x00,0x12345678u);
    wr16(raw,0x08,1); // Bulbasaur
    wr16(raw,0x0C,12345);
    wr16(raw,0x0E,54321);
    wr32(raw,0x10,135);
    wr16(raw,0x28,first);
    wr16(raw,0x2A,second);
    raw[0x17]=std::byte{2};
    raw[0x5F]=static_cast<std::byte>(originVersion);
    raw[0x83]=std::byte{4};
    if(realEggOrigin) {
        wr16(raw,0x44,2000);
        wr32(raw,0x38,0x40000000u);
    }
    const auto enc=Encryption::encryptArray4(raw);
    Pokemon::Pokemon4ReadOnly stored(enc,Enums::GameVersion::PT);
    assert(stored.valid() && stored.checksumValid());
    assert(stored.originVersion()==originVersion);
    Pokemon::Pokemon4ReadOnlyView view(stored);
    return exactSource
        ? Legality::analyze(view,Enums::GameVersion::PT,"platinum_nds")
        : Legality::analyze(view,Enums::GameVersion::PT);
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


void testGen4NativeBreedingParentSnapshots() {
    // Real encrypted native PK4 -> strict checksum reader -> immutable
    // parent projection -> independently attested conditional daycare pair.
    // Parent *histories* and group source rows remain explicit attestations.
    namespace N=Legality::Gen4NativeBreedingParentEvidence;
    namespace B=Legality::Gen34BreedingPairEligibility;
    const auto makeRaw=[](uint32_t id,uint8_t storedGender,bool egg) {
        std::vector<std::byte> raw(Encryption::SIZE_STORED4,std::byte{0});
        wr32(raw,0x00,0x12345678u);
        wr16(raw,0x08,1); // native Bulbasaur record
        wr32(raw,0x0C,id); // complete source OT ID
        wr32(raw,0x10,135);
        raw[0x40]=static_cast<std::byte>(storedGender<<1);
        raw[0x5F]=std::byte{12}; // original Platinum; valid later HGSS trade
        if(egg)wr32(raw,0x38,0x40000000u);
        return Encryption::encryptArray4(raw);
    };
    const auto femaleBytes=makeRaw(0xAAAA2222u,1,false);
    const auto maleBytes=makeRaw(0xBBBB2222u,0,false);
    for(const auto game:{"platinum_nds","heartgold_nds","soulsilver_nds"}) {
        const auto group=N::requiredGroup(game);
        Pokemon::Pokemon4ReadOnly female(femaleBytes,group);
        Pokemon::Pokemon4ReadOnly male(maleBytes,group);
        assert(female.valid() && male.valid());
        assert(female.gender()==1 && male.gender()==0);
        assert(female.id32()==0xAAAA2222u && male.id32()==0xBBBB2222u);
        const auto beforeFemale=female.originalEncryptedBytes();
        const std::vector<std::byte> untouched(beforeFemale.begin(),beforeFemale.end());
        N::ParentWitness fw{&female,101,true,true,{game,1,0,1,7,true}};
        N::ParentWitness mw{&male,202,true,true,{game,1,0,1,7,true}};
        for(bool swapped:{false,true}) {
            N::Input in{};
            in.game=game;
            in.slot0=swapped?mw:fw;
            in.slot1=swapped?fw:mw;
            in.orderedPairStageIndependentlyVerified=true;
            const auto out=N::evaluate(in);
            assert(out.status==N::Status::Projected && out.pairEvaluated);
            assert(out.eligibility.status==B::Status::Compatible);
            assert(out.eligibility.scoreKnown &&
                   out.eligibility.compatibilityScore==70);
            assert(out.eligibility.fatherId==202 &&
                   out.eligibility.motherId==101);
            assert(out.eligibility.slot0Id==(swapped?202u:101u));
            assert(out.eligibility.slot1Id==(swapped?101u:202u));
            assert(out.evaluatedInput.slot0.fullOTId==
                   (swapped?0xBBBB2222u:0xAAAA2222u));
            assert(std::equal(untouched.begin(),untouched.end(),
                              female.originalEncryptedBytes().begin()));
            auto noGroup=in;
            noGroup.slot0.eggGroups.independentlyVerified=false;
            assert(N::evaluate(noGroup).status==
                   N::Status::UnknownEggGroupResource);
            auto alienGroup=in;
            alienGroup.slot0.eggGroups.species=25;
            assert(N::evaluate(alienGroup).status==
                   N::Status::EggGroupIdentityMismatch);
            auto wrongGame=in;
            wrongGame.slot0.eggGroups.game="diamond_nds";
            assert(N::evaluate(wrongGame).status==
                   N::Status::EggGroupIdentityMismatch);
            auto wrongForm=in;
            wrongForm.slot0.eggGroups.form=1;
            assert(N::evaluate(wrongForm).status==
                   N::Status::EggGroupIdentityMismatch);
            auto noSnapshot=in;
            noSnapshot.slot0.stageSnapshotIndependentlyVerified=false;
            assert(N::evaluate(noSnapshot).status==
                   N::Status::UnknownHistoricalStage);
            auto missingSlotEvidence=in;
            missingSlotEvidence.orderedPairStageIndependentlyVerified=false;
            auto uncertain=N::evaluate(missingSlotEvidence);
            assert(uncertain.status==N::Status::UnknownHistoricalStage);
            assert(!uncertain.pairEvaluated);
            auto dup=in;
            dup.slot0.snapshotId=dup.slot1.snapshotId;
            auto duplicated=N::evaluate(dup);
            assert(duplicated.pairEvaluated &&
                   duplicated.eligibility.status==B::Status::IncoherentParents);
            auto noSource=in;
            noSource.slot0.nativeSourceGameIndependentlyVerified=false;
            assert(N::evaluate(noSource).status==N::Status::UnknownNativeSource);
        }
        // The retained record may be imported from Platinum into HGSS.
        // The historical breeding *container* is the independently
        // attested current-game source, not inferred from originVersion.
        assert(female.originVersion()==12);
        if(group==Enums::GameVersion::HGSS)assert(female.sourceGroup()==group);
        auto eggBytes=makeRaw(0xAAAA2222u,1,true);
        Pokemon::Pokemon4ReadOnly egg(eggBytes,group);
        auto eggWitness=fw;eggWitness.native=&egg;
        assert(N::project(game,eggWitness).status==N::Status::EggOrEmptyRecord);
        auto damaged=femaleBytes;damaged[6]^=std::byte{0x01};
        Pokemon::Pokemon4ReadOnly bad(damaged,group);
        assert(!bad.checksumValid());
        auto badWitness=fw;badWitness.native=&bad;
        assert(N::project(game,badWitness).status==N::Status::InvalidNativeRecord);
        const auto gender3=makeRaw(0xAAAA2222u,3,false);
        Pokemon::Pokemon4ReadOnly unknownGender(gender3,group);
        assert(unknownGender.valid());
        auto sexUnknown=fw;sexUnknown.native=&unknownGender;
        assert(N::project(game,sexUnknown).status==N::Status::UnknownNativeGender);
        // A wrong game-family personal table cannot become positive.
        Pokemon::Pokemon4ReadOnly wrongFamily(femaleBytes,Enums::GameVersion::DP);
        auto wrongSource=fw;wrongSource.native=&wrongFamily;
        assert(N::project(game,wrongSource).status==N::Status::UnknownNativeSource);
    }
    N::Input noGame{};noGame.game="diamond_nds";
    assert(N::evaluate(noGame).status==N::Status::UnsupportedGame);
}


void testGen4JointNativeParentMoveIvHistories() {
    // BR-003 / RNG-003: checksum-valid actual encrypted PK4 parent readers
    // feed ONE source-attested move/IV history, not a union of donors.
    namespace J=Legality::Gen4JointEggCandidateEvidence;
    namespace N=Legality::Gen4NativeBreedingParentEvidence;
    namespace IV=Legality::Gen4EggIVInheritanceStage;
    constexpr std::array<uint8_t,6> initial{0,3,6,9,12,15};
    constexpr std::array<uint8_t,6> fivs{7,12,17,22,27,0};
    constexpr std::array<uint8_t,6> mivs{13,24,3,14,25,4};
    const auto packIV=[](const std::array<uint8_t,6>& a) {
        uint32_t bits=0;
        for(uint32_t k=0;k<6;++k)bits|=uint32_t(a[k])<<(k*5u);
        return bits;
    };
    const auto makeParent=[&](uint32_t id,uint8_t gender,
                              const std::array<uint8_t,6>& ivs,uint16_t item) {
        std::vector<std::byte> raw(Encryption::SIZE_STORED4,std::byte{0});
        wr32(raw,0x00,0x12345678u);
        wr16(raw,0x08,1); // Bulbasaur, valid source-game personal row
        wr16(raw,0x0A,item);
        wr32(raw,0x0C,id);
        wr32(raw,0x10,135);
        wr32(raw,0x38,packIV(ivs));
        raw[0x40]=static_cast<std::byte>(gender<<1);
        raw[0x5F]=std::byte{12}; // origin history != daycare container
        return Encryption::encryptArray4(raw);
    };
    constexpr std::array<std::array<uint16_t,2>,5> held{{
        {{0,0}},{{294,0}},{{0,294}},{{294,294}},{{294,289}}
    }};
    unsigned consistent=0;
    for(const char* game:{"platinum_nds","heartgold_nds","soulsilver_nds"}) {
        for(bool reversed:{false,true}) {
            for(const auto& items:held) {
                // Held items belong to physical slots. Role and slot
                // identity can invert without changing move donor roles.
                const auto femaleBytes=makeParent(0xAAAA2222u,1,mivs,
                    reversed?items[0]:items[1]);
                const auto maleBytes=makeParent(0xBBBB2222u,0,fivs,
                    reversed?items[1]:items[0]);
                const auto group=N::requiredGroup(game);
                Pokemon::Pokemon4ReadOnly female(femaleBytes,group);
                Pokemon::Pokemon4ReadOnly male(maleBytes,group);
                assert(female.valid() && male.valid());
                const auto& slot0=reversed?male:female;
                const auto& slot1=reversed?female:male;
                J::Input in{};
                in.parents.game=game;
                in.parents.slot0={&slot0,reversed?202u:101u,true,true,
                                  {game,1,0,1,7,true}};
                in.parents.slot1={&slot1,reversed?101u:202u,true,true,
                                  {game,1,0,1,7,true}};
                in.parents.orderedPairStageIndependentlyVerified=true;
                auto& m=in.moveHistory;
                m.game=game;m.species=1;
                m.fatherId=202;m.motherId=101;
                m.snapshotsKnown=true;m.fatherSpecies=1;m.motherSpecies=1;
                m.base={33,45,0,0};m.observed=m.base;
                m.father=male.moves();m.mother=female.moves();
                m.lightBallKnown=true;
                in.sharedStageIndependentlyVerified=true;
                in.nativePairStageToken=0x7711;
                in.moveParentStageToken=0x7711;
                in.ivParentStageToken=0x7711;
                in.initialChildIVs=initial;
                in.initialChildIVsIndependentlyVerified=true;
                in.observedChildIVsIndependentlyVerified=true;
                in.heldItemsAtStageIndependentlyVerified=true;
                in.finalRNGStateIndependentlyVerified=true;
                const auto forward=IV::replay(game,0,initial,
                    slot0.ivs(),slot1.ivs(),
                    slot0.heldItem(),slot1.heldItem());
                assert(forward.status==IV::Status::ReplayedStage);
                in.observedChildIVs=forward.childIVs;
                in.knownFinalRNGState=forward.finalSeed;
                const auto good=J::evaluate(in);
                assert(good.status==J::Status::KnownCandidateConsistent);
                // The actual test-constructed ENCRYPTED UNHATCHED PK4
                // carries the child species/moves/IVs. The independent
                // historical attestation is still a caller-supplied proof.
                std::vector<std::byte> childRaw(Encryption::SIZE_STORED4,std::byte{0});
                wr32(childRaw,0x00,0x87654321u);
                wr16(childRaw,0x08,1);
                wr32(childRaw,0x0C,0xCDEF1234u);
                wr32(childRaw,0x10,135);
                wr16(childRaw,0x28,33);
                wr16(childRaw,0x2A,45);
                wr32(childRaw,0x38,packIV(in.observedChildIVs)|0x40000000u);
                childRaw[0x40]=std::byte{0};
                childRaw[0x5F]=std::byte{12};
                const auto childBytes=Encryption::encryptArray4(childRaw);
                Pokemon::Pokemon4ReadOnly child(childBytes,group);
                assert(child.valid() && child.isEgg());
                const J::NativeEggWitness childHistory{
                    &child,true,true,in.nativePairStageToken
                };
                const auto complete=J::evaluateWithNativeEgg(in,childHistory);
                assert(complete.status==J::Status::KnownCandidateConsistent);
                // BREED-004: evaluate alternate attested histories for the
                // SAME original unhatched encrypted child, never union moves
                // or IVs from different candidate parents.
                namespace U=Legality::Gen4EggCandidateUniverse;
                U::Input universe{};
                universe.game=game;
                universe.originalUnhatchedEgg=&child;
                universe.originalEggSourceIndependentlyVerified=true;
                universe.count=3;
                for(uint8_t k=0;k<universe.count;++k) {
                    auto& route=universe.alternatives[k];
                    route.routeIdentity=1000u+k;
                    route.hypothesisIndependentlyGrounded=true;
                    route.fullCandidateMethodCoverageIndependentlyVerified=true;
                    route.history=in;
                    route.originalEgg=childHistory;
                }
                // Alternative #1 has incompatible final RNG evidence;
                // alternative #2 has a genuinely UNKNOWN final state.
                universe.alternatives[1].history.knownFinalRNGState^=1u;
                universe.alternatives[2].history.finalRNGStateIndependentlyVerified=false;
                auto setResult=U::evaluate(universe);
                assert(setResult.status==U::Status::OneConditionalHistory);
                assert(setResult.consistent==1 && setResult.contradicted==1 &&
                       setResult.unresolved==1);
                // A completed negative candidate set is scoped to the
                // independently enumerated DAYCARE alternatives only.
                auto exhaustedSet=universe;
                exhaustedSet.alternatives[0].history.knownFinalRNGState^=2u;
                exhaustedSet.alternatives[2].history.knownFinalRNGState^=16u;
                exhaustedSet.alternatives[2].history.finalRNGStateIndependentlyVerified=true;
                assert(U::evaluate(exhaustedSet).status==
                       U::Status::UnknownCandidateEnumeration);
                exhaustedSet.allDaycareHypothesesIndependentlyEnumerated=true;
                auto exhausted=U::evaluate(exhaustedSet);
                assert(exhausted.status==U::Status::AllAttestedDaycareHistoriesContradicted);
                assert(exhausted.contradicted==3 && exhausted.consistent==0);
                auto lostHistory=exhaustedSet;
                lostHistory.alternatives[1].fullCandidateMethodCoverageIndependentlyVerified=false;
                assert(U::evaluate(lostHistory).status==U::Status::UnknownCandidateEvidence);
                auto lostProvenance=exhaustedSet;
                lostProvenance.alternatives[1].hypothesisIndependentlyGrounded=false;
                assert(U::evaluate(lostProvenance).status==U::Status::UnknownCandidateProvenance);
                auto duplicateRoute=exhaustedSet;
                duplicateRoute.alternatives[1].routeIdentity=
                    duplicateRoute.alternatives[0].routeIdentity;
                assert(U::evaluate(duplicateRoute).status==U::Status::UnknownCandidateIdentity);
                auto missingRoute=exhaustedSet;
                missingRoute.alternatives[1].routeIdentity=0;
                assert(U::evaluate(missingRoute).status==U::Status::UnknownCandidateIdentity);
                auto missingUniverseEgg=exhaustedSet;
                missingUniverseEgg.originalUnhatchedEgg=nullptr;
                assert(U::evaluate(missingUniverseEgg).status==U::Status::UnknownOriginalEggSource);
                auto unauthenticatedChild=exhaustedSet;
                unauthenticatedChild.originalEggSourceIndependentlyVerified=false;
                assert(U::evaluate(unauthenticatedChild).status==U::Status::UnknownOriginalEggSource);
                auto alienSource=exhaustedSet;
                alienSource.alternatives[1].history.parents.game="diamond_nds";
                assert(U::evaluate(alienSource).status==U::Status::UnknownCandidateProvenance);
                auto borrowedChild=exhaustedSet;
                auto foreignRaw=childRaw;wr16(foreignRaw,0x08,25);
                Pokemon::Pokemon4ReadOnly foreignChild(
                    Encryption::encryptArray4(foreignRaw),group);
                assert(foreignChild.valid() && foreignChild.isEgg());
                borrowedChild.alternatives[1].originalEgg.record=&foreignChild;
                assert(U::evaluate(borrowedChild).status==U::Status::UnknownCandidateProvenance);
                auto noChildStage=exhaustedSet;
                noChildStage.alternatives[1].originalEgg.originalEggStageToken=0;
                assert(U::evaluate(noChildStage).status==U::Status::UnknownCandidateEvidence);
                auto wrongChildStage=exhaustedSet;
                wrongChildStage.alternatives[1].originalEgg.originalEggStageToken=0x9F33u;
                assert(U::evaluate(wrongChildStage).status==
                       U::Status::AllAttestedDaycareHistoriesContradicted);
                // RNG-stage seed and initial random-IV record are LOST;
                // the source-attested parents and original child can still
                // exclude impossible NECESSARY inherited-stat patterns.
                auto noSeedImpossible=in;
                const auto iv0=slot0.ivs(),iv1=slot1.ivs();
                for(uint8_t stat=0;stat<6;++stat) {
                    uint8_t fresh=0;
                    while(fresh==iv0[stat]||fresh==iv1[stat])++fresh;
                    noSeedImpossible.observedChildIVs[stat]=fresh;
                }
                noSeedImpossible.finalRNGStateIndependentlyVerified=false;
                noSeedImpossible.initialChildIVsIndependentlyVerified=false;
                auto necessaryOnly=J::evaluate(noSeedImpossible);
                assert(necessaryOnly.status==J::Status::IVNecessaryPatternContradicted);
                // Exact immutable original unhatched PK4 with all six IVs
                // inconsistent with EVERY claimed historical parent stat.
                // Even without initial rolls or a final RNG seed, this
                // positively excludes these PARTICULAR daycare candidates.
                auto impossibleRaw=childRaw;
                wr32(impossibleRaw,0x38,
                     packIV(noSeedImpossible.observedChildIVs)|0x40000000u);
                Pokemon::Pokemon4ReadOnly impossibleEgg(
                    Encryption::encryptArray4(impossibleRaw),group);
                assert(impossibleEgg.valid() && impossibleEgg.isEgg());
                auto impossibleWitness=childHistory;
                impossibleWitness.record=&impossibleEgg;
                assert(J::evaluateWithNativeEgg(noSeedImpossible,
                       impossibleWitness).status==
                       J::Status::IVNecessaryPatternContradicted);
                auto allImpossible=exhaustedSet;
                allImpossible.originalUnhatchedEgg=&impossibleEgg;
                for(auto& historicalRoute:allImpossible.alternatives) {
                    historicalRoute.history.observedChildIVs=
                        noSeedImpossible.observedChildIVs;
                    historicalRoute.history.finalRNGStateIndependentlyVerified=false;
                    historicalRoute.history.initialChildIVsIndependentlyVerified=false;
                    historicalRoute.originalEgg=impossibleWitness;
                }
                assert(U::evaluate(allImpossible).status==
                       U::Status::AllAttestedDaycareHistoriesContradicted);
                auto unverifiedIV=allImpossible;
                unverifiedIV.alternatives[0].history.observedChildIVsIndependentlyVerified=false;
                assert(U::evaluate(unverifiedIV).status==
                       U::Status::UnknownCandidateEvidence);
                auto unsupportedUniverse=exhaustedSet;
                unsupportedUniverse.game="diamond_nds";
                assert(U::evaluate(unsupportedUniverse).status==U::Status::UnsupportedGame);
                auto overCapacity=exhaustedSet;overCapacity.count=9;
                assert(U::evaluate(overCapacity).status==U::Status::CapacityExceededUnknown);
                auto none=exhaustedSet;none.count=0;
                assert(U::evaluate(none).status==U::Status::NoCandidates);
                auto noChildIVProvenance=in;
                noChildIVProvenance.observedChildIVsIndependentlyVerified=false;
                assert(J::evaluateWithNativeEgg(noChildIVProvenance,childHistory).status==
                       J::Status::UnknownObservedChildIVs);
                const auto childBefore=std::vector<std::byte>(
                    child.originalEncryptedBytes().begin(),
                    child.originalEncryptedBytes().end());
                assert(std::equal(childBefore.begin(),childBefore.end(),
                                  child.originalEncryptedBytes().begin()));
                auto noHistoricalChild=childHistory;
                noHistoricalChild.originalUnhatchedStageIndependentlyVerified=false;
                assert(J::evaluateWithNativeEgg(in,noHistoricalChild).status==
                       J::Status::UnknownNativeEggRecord);
                auto noEggStageId=childHistory;
                noEggStageId.originalEggStageToken=0;
                assert(J::evaluateWithNativeEgg(in,noEggStageId).status==
                       J::Status::UnknownNativeEggRecord);
                auto mixedEggEvent=childHistory;
                mixedEggEvent.originalEggStageToken=0xBEEFu;
                assert(J::evaluateWithNativeEgg(in,mixedEggEvent).status==
                       J::Status::IncoherentNativeEggRecord);
                auto missingChild=childHistory;missingChild.record=nullptr;
                assert(J::evaluateWithNativeEgg(in,missingChild).status==
                       J::Status::UnknownNativeEggRecord);
                auto noSourceChild=childHistory;noSourceChild.containingGameIndependentlyVerified=false;
                assert(J::evaluateWithNativeEgg(in,noSourceChild).status==
                       J::Status::UnknownNativeEggRecord);
                auto changedEggMoves=childRaw;wr16(changedEggMoves,0x28,999);
                Pokemon::Pokemon4ReadOnly eggMovesBad(
                    Encryption::encryptArray4(changedEggMoves),group);
                auto movesBadWitness=childHistory;movesBadWitness.record=&eggMovesBad;
                assert(J::evaluateWithNativeEgg(in,movesBadWitness).status==
                       J::Status::IncoherentNativeEggRecord);
                auto alteredEggIV=childRaw;
                wr32(alteredEggIV,0x38,packIV(initial)|0x40000000u);
                Pokemon::Pokemon4ReadOnly eggIVBad(
                    Encryption::encryptArray4(alteredEggIV),group);
                auto ivBadWitness=childHistory;ivBadWitness.record=&eggIVBad;
                const auto ivMismatch=J::evaluateWithNativeEgg(in,ivBadWitness);
                // The altered IV may occasionally equal the observed IV
                // for a synthetic stage; use a forced alternate first IV
                // for a guaranteed contradiction below.
                (void)ivMismatch;
                auto guaranteedEggIV=childRaw;
                const auto observedPack=packIV(in.observedChildIVs);
                const auto changedHP=(in.observedChildIVs[0]+1u)%32u;
                wr32(guaranteedEggIV,0x38,
                     ((observedPack&~0x1Fu)|changedHP)|0x40000000u);
                Pokemon::Pokemon4ReadOnly wrongIvEgg(
                    Encryption::encryptArray4(guaranteedEggIV),group);
                ivBadWitness.record=&wrongIvEgg;
                assert(J::evaluateWithNativeEgg(in,ivBadWitness).status==
                       J::Status::IncoherentNativeEggRecord);
                auto noEggFlag=childRaw;
                wr32(noEggFlag,0x38,packIV(in.observedChildIVs));
                Pokemon::Pokemon4ReadOnly hatched(
                    Encryption::encryptArray4(noEggFlag),group);
                auto nonEggWitness=childHistory;nonEggWitness.record=&hatched;
                assert(J::evaluateWithNativeEgg(in,nonEggWitness).status==
                       J::Status::UnknownNativeEggRecord);
                assert(good.moves.status==
                    Legality::Gen34SameParentEggHistory::Status::CandidateCompatible);
                assert(good.ivs==IV::CandidateCheck::Matched);
                assert(good.parents.eligibility.fatherId==202 &&
                       good.parents.eligibility.motherId==101);
                assert(good.parents.eligibility.slot0Id==(reversed?202u:101u));
                assert(good.parents.eligibility.slot1Id==(reversed?101u:202u));
                assert(good.necessaryIVs.status==
                    Legality::Gen4EggIVStructuralBounds::Status::NecessaryPatternNotExcluded);
                auto unknownSourceChild=noSeedImpossible;
                unknownSourceChild.observedChildIVsIndependentlyVerified=false;
                assert(J::evaluate(unknownSourceChild).status==
                       J::Status::UnknownObservedChildIVs);
                if(IV::family(game)==IV::Family::HeartGoldSoulSilver) {
                    const unsigned powers=(IV::powerStat(items[0])>=0)+
                        (IV::powerStat(items[1])>=0);
                    assert(forward.drawCount==(powers==2?5u:powers==1?4u:6u));
                } else assert(forward.drawCount==6);
                ++consistent;
                auto badRole=in;badRole.moveHistory.fatherId=101;
                assert(J::evaluate(badRole).status==J::Status::IncoherentParentIdentity);
                auto staleMove=in;staleMove.moveHistory.father[0]=999;
                assert(J::evaluate(staleMove).status==J::Status::IncoherentMoveSnapshot);
                auto diffStage=in;diffStage.ivParentStageToken=0x9977;
                assert(J::evaluate(diffStage).status==J::Status::IncoherentSharedStage);
                auto noStage=in;noStage.sharedStageIndependentlyVerified=false;
                assert(J::evaluate(noStage).status==J::Status::UnknownSharedStage);
                auto unknownParent=in;unknownParent.parents.slot0.stageSnapshotIndependentlyVerified=false;
                assert(J::evaluate(unknownParent).status==J::Status::UnknownNativeParentEvidence);
                auto missingOrder=in;missingOrder.parents.orderedPairStageIndependentlyVerified=false;
                assert(J::evaluate(missingOrder).status==J::Status::UnknownNativeParentEvidence);
                auto wrongEggGroup=in;wrongEggGroup.parents.slot0.eggGroups.species=25;
                assert(J::evaluate(wrongEggGroup).status==J::Status::UnknownNativeParentEvidence);
                auto absentChild=in;absentChild.initialChildIVsIndependentlyVerified=false;
                assert(J::evaluate(absentChild).status==J::Status::UnknownInitialChildIVs);
                auto missingObserved=in;missingObserved.observedChildIVsIndependentlyVerified=false;
                assert(J::evaluate(missingObserved).status==J::Status::UnknownObservedChildIVs);
                auto noSeed=in;noSeed.finalRNGStateIndependentlyVerified=false;
                assert(J::evaluate(noSeed).status==J::Status::UnknownFinalRNGState);
                auto noItems=in;noItems.heldItemsAtStageIndependentlyVerified=false;
                assert(J::evaluate(noItems).status==
                    (IV::family(game)==IV::Family::Platinum?
                     J::Status::KnownCandidateConsistent:J::Status::UnknownPowerItemHistory));
                auto contradiction=in;
                contradiction.observedChildIVs[0]=32;
                assert(J::evaluate(contradiction).status==J::Status::MalformedIVDomain);
                auto changedObservation=in;
                changedObservation.observedChildIVs[0]=
                    static_cast<uint8_t>(changedObservation.observedChildIVs[0]==31?30:31);
                const auto changedStatus=J::evaluate(changedObservation).status;
                assert(changedStatus==J::Status::IVCandidateContradicted ||
                       changedStatus==J::Status::IVNecessaryPatternContradicted);
                auto incompatible=in;
                incompatible.parents.slot0.eggGroups.group1=15;
                assert(J::evaluate(incompatible).status==J::Status::CandidatePairIncompatible);
                auto badMove=in;badMove.moveHistory.observed={1,2,3,4};
                assert(J::evaluate(badMove).status==J::Status::MoveCandidateContradicted);
            }
        }
    }
    assert(consistent==30);
    // No invented DP Gen IV inheritance algorithm.
    J::Input unknown{};unknown.parents.game="diamond_nds";
    assert(J::evaluate(unknown).status==J::Status::UnsupportedGame);
}

int main() {
    testGen4JointNativeParentMoveIvHistories();
    testGen4NativeBreedingParentSnapshots();
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

    // MOVE-003: actual native PK3 -> immutable analyzer; an ordinary
    // Emerald Nincada→Ninjask→Shedinja sibling copy occurs at a single
    // evolution level, not at both Ninjask's Agility (38) and Baton (45).
    using Legality::Gen3ShedinjaEvolutionMove::classify;
    static_assert(classify(292,50,{97,226,0,0}).oneEvolutionCannotIntroduceBoth);
    static_assert(classify(292,38,{97,0,0,0}).ordinaryAgilityCopyPossible);
    static_assert(!classify(292,37,{97,0,0,0}).ordinaryAgilityCopyPossible);
    static_assert(classify(292,45,{226,0,0,0}).ordinaryBatonCopyPossible);
    static_assert(!classify(292,44,{226,0,0,0}).ordinaryBatonCopyPossible);
    static_assert(!classify(292,50,{14,226,0,0}).oneEvolutionCannotIntroduceBoth);
    static_assert(!classify(291,50,{97,226,0,0}).oneEvolutionCannotIntroduceBoth);
    constexpr auto shedConflict="Emerald Shedinja candidate move-history bound:";
    constexpr auto shedPositive="Emerald Shedinja candidate move-history evidence:";
    auto shed=gen3WithBall(4);
    shed.setSpecies(292);
    shed.setOriginGame(3); // Stored original Emerald GBA.
    shed.setLevel(50);
    shed.setMove(0,97);
    shed.setMove(1,226);
    shed.refreshChecksum();
    const auto shedDetached=Legality::analyze(shed,Enums::GameVersion::FRLG);
    assert(hasInfo(shedDetached,shedConflict));
    assert(!hasInvalidText(shedDetached,shedConflict));
    assert(hasInfo(Legality::analyze(
        shed,Enums::GameVersion::FRLG,"emerald_gba"),shedConflict));
    shed.setMove(1,14); // Swords Dance: a separate acquisition alternative.
    shed.refreshChecksum();
    assert(!hasInfo(Legality::analyze(shed,Enums::GameVersion::FRLG),shedConflict));
    shed.setMove(1,0);
    shed.setLevel(38);
    shed.refreshChecksum();
    assert(hasInfo(Legality::analyze(shed,Enums::GameVersion::FRLG),shedPositive));
    shed.setLevel(37);
    shed.refreshChecksum();
    assert(!hasText(Legality::analyze(shed,Enums::GameVersion::FRLG),
                    "Emerald Shedinja candidate move-history"));
    shed.setLevel(50);
    shed.setOriginGame(2); // Wrong retained original game: Ruby.
    shed.setMove(1,226);
    shed.refreshChecksum();
    assert(!hasText(Legality::analyze(shed,Enums::GameVersion::FRLG),
                    "Emerald Shedinja candidate move-history"));
    shed.setOriginGame(3);
    shed.setSpecies(291); // Ninjask, not Shedinja.
    shed.refreshChecksum();
    assert(!hasText(Legality::analyze(shed,Enums::GameVersion::FRLG),
                    "Emerald Shedinja candidate move-history"));

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

    // Original GameCube code 15 survives in a native PK3, but no single
    // intermediate GBA game is encoded by that code. Compare real pinned
    // move pools without naming Ruby, Emerald or FRLG as the true source.
    namespace G3Joint=Legality::Gen3JointMoveEvidence;
    const auto gcSingle=G3Joint::classifyUnanchored(
        1,{33,74,0,0});
    assert(gcSingle.bound==G3Joint::Bound::OnePool);
    assert(gcSingle.minimumPools==1);
    const auto gcMixed=G3Joint::classifyUnanchored(
        4,{7,232,0,0});
    assert(gcMixed.bound==G3Joint::Bound::MultiplePools);
    assert(gcMixed.minimumPools==2);
    assert(G3Joint::classifyUnanchored(
        4,{7,354,0,0}).bound==G3Joint::Bound::Unresolved);
    assert(G3Joint::classifyUnanchored(
        4,{0,0,0,0}).bound==G3Joint::Bound::Empty);

    constexpr auto gcPoolText=
        "Source-free GameCube-origin PK3: 2 stored move(s) appear in at least one audited GBA game pool";
    constexpr auto gcJointText=
        "Source-free GameCube-origin PK3 joint move bound: at least 2 audited GBA game-family pools";
    auto gcNative=gen3WithBall(4);
    gcNative.setSpecies(4);
    gcNative.setOriginGame(15); // GameCube original code; no GBA identity.
    gcNative.setMove(0,7);       // Emerald compatibility
    gcNative.setMove(1,232);     // FRLG compatibility
    gcNative.refreshChecksum();
    assert(gcNative.originGame()==15);
    const auto gcReport=Legality::analyze(gcNative,Enums::GameVersion::FRLG);
    assert(hasInfo(gcReport,gcPoolText));
    assert(hasInfo(gcReport,gcJointText));
    assert(!hasInvalidText(gcReport,gcJointText));
    assert(!hasInfo(gcReport,"Source-free PK3 joint move bound:"));
    assert(!hasInfo(gcReport,
        "Source-free PK3 move is in the audited direct move pool"));
    assert(!hasInfo(Legality::analyze(
        gcNative,Enums::GameVersion::FRLG,"ruby_gba"),gcJointText));

    gcNative.setMove(1,354);
    gcNative.refreshChecksum();
    assert(!hasText(Legality::analyze(
        gcNative,Enums::GameVersion::FRLG),"GameCube-origin PK3 joint move bound"));
    gcNative.setOriginGame(0); // Unknown is not automatically GameCube.
    gcNative.setMove(1,232);
    gcNative.refreshChecksum();
    assert(!hasText(Legality::analyze(
        gcNative,Enums::GameVersion::FRLG),"GameCube-origin PK3"));
    // Unlike Gen III's broad 'can learn someday' union, this pinned
    // Emerald table preserves actual native per-level learning entries.
    // Positive method evidence is NOT proof the move was actually learned
    // that way; lack of a level-up opportunity cannot disprove TM/egg/event.
    namespace EmeraldLv=Legality::Gen3EmeraldLevelUp;
    static_assert(EmeraldLv::firstLevelAtOrBefore(1,33,1)==1); // Tackle
    static_assert(EmeraldLv::firstLevelAtOrBefore(1,75,19)==0); // Razor Leaf
    static_assert(EmeraldLv::firstLevelAtOrBefore(1,75,20)==20);
    static_assert(EmeraldLv::firstLevelAtOrBefore(4,52,6)==0); // Ember
    static_assert(EmeraldLv::firstLevelAtOrBefore(4,52,7)==7);
    static_assert(EmeraldLv::firstLevelAtOrBefore(1,14,100)==0); // Tutor
    static_assert(EmeraldLv::firstLevelAtOrBefore(0,33,50)==0);
    static_assert(EmeraldLv::firstLevelAtOrBefore(387,33,50)==0);
    static_assert(EmeraldLv::firstLevelAtOrBefore(1,355,100)==0);

    constexpr auto emeraldLevelText=
        "Emerald level-up method evidence:";
    auto emeraldLevelMon=gen3WithBall(4);
    emeraldLevelMon.setSpecies(1);
    emeraldLevelMon.setOriginGame(3);
    emeraldLevelMon.setLevel(20);
    emeraldLevelMon.setMove(0,75); // Bulbasaur Razor Leaf at Lv20.
    emeraldLevelMon.refreshChecksum();
    assert(emeraldLevelMon.level()==20);
    const auto emeraldSourceFree=Legality::analyze(
        emeraldLevelMon,Enums::GameVersion::FRLG);
    assert(hasInfo(emeraldSourceFree,emeraldLevelText));
    assert(hasInfo(emeraldSourceFree,
        "from level 20, no higher than its current level"));
    assert(hasInfo(Legality::analyze(
        emeraldLevelMon,Enums::GameVersion::FRLG,"emerald_gba"),
        emeraldLevelText));
    assert(!hasInfo(Legality::analyze(
        emeraldLevelMon,Enums::GameVersion::FRLG,"ruby_gba"),
        emeraldLevelText));
    emeraldLevelMon.setLevel(19);
    emeraldLevelMon.refreshChecksum();
    assert(!hasInfo(Legality::analyze(
        emeraldLevelMon,Enums::GameVersion::FRLG),emeraldLevelText));
    emeraldLevelMon.setLevel(20);
    emeraldLevelMon.setOriginGame(2); // Ruby origin != Emerald.
    emeraldLevelMon.refreshChecksum();
    assert(!hasInfo(Legality::analyze(
        emeraldLevelMon,Enums::GameVersion::FRLG),emeraldLevelText));
    emeraldLevelMon.setOriginGame(3);
    emeraldLevelMon.setMove(0,14); // Swords Dance compatibility via tutor.
    emeraldLevelMon.refreshChecksum();
    assert(!hasInfo(Legality::analyze(
        emeraldLevelMon,Enums::GameVersion::FRLG),emeraldLevelText));


    // MOVE-002: pinned level-aware method witnesses are attached to *one*
    // candidate identity; the encounter-level bound excludes moves learned
    // only before that candidate could have been encountered. Missing direct
    // methods and source mismatches are unresolved, never global Invalid.
    namespace EmeraldCandidate=Legality::Gen3EmeraldCandidateMoveEvidence;
    using Candidate=Legality::Gen14CandidateMoveHistory::Candidate;
    using WitnessStatus=Legality::Gen14CandidateMoveHistory::Status;
    constexpr Candidate earlyEmerald{11,20,1,false,1};
    constexpr auto twoDirect=EmeraldCandidate::classify(
        earlyEmerald,1,{33,75,0,0});
    static_assert(twoDirect.storedMoves==2 &&
                  twoDirect.directLevelUpSlots==2 &&
                  twoDirect.joint.status==WitnessStatus::WitnessesCompatible);
    constexpr Candidate laterEncounter{11,20,10,false,1};
    constexpr auto lateRoute=EmeraldCandidate::classify(
        laterEncounter,1,{33,75,0,0});
    static_assert(lateRoute.directLevelUpSlots==1 &&
                  lateRoute.joint.status==WitnessStatus::Unresolved);
    static_assert(Legality::Gen3EmeraldLevelUp::firstLevelBetween(
        1,33,10,20)==0);
    static_assert(Legality::Gen3EmeraldLevelUp::firstLevelBetween(
        1,75,10,20)==20);
    static_assert(EmeraldCandidate::classify(
        earlyEmerald,1,{33,14,0,0}).joint.status==
        WitnessStatus::Unresolved);
    static_assert(EmeraldCandidate::classify(
        Candidate{},1,{33,75,0,0}).joint.status==
        WitnessStatus::Unresolved);

    // Read-only native PK3 -> exact conditional Emerald move-history
    // bridge: current native moves are factual, but the original trainer
    // decisions and same-game/same-species history are NOT stored.
    auto nativeTimeline=gen3WithBall(4);
    nativeTimeline.setOriginGame(3); // Original Emerald
    nativeTimeline.setSpecies(1);
    nativeTimeline.setLevel(32);
    nativeTimeline.setMove(0,77);
    nativeTimeline.setMove(1,79);
    nativeTimeline.setMove(2,75);
    nativeTimeline.setMove(3,22);
    nativeTimeline.refreshChecksum();
    assert(nativeTimeline.checksumValid());
    namespace Timeline=Legality::Gen3EmeraldLevelUpTimeline;
    Timeline::Input nativeHypothesis{};
    nativeHypothesis.game="emerald_gba";
    nativeHypothesis.species=nativeTimeline.speciesID();
    nativeHypothesis.encounterLevel=5;
    nativeHypothesis.observedLevel=nativeTimeline.level();
    nativeHypothesis.initialSnapshotVerified=true;
    nativeHypothesis.sameSpeciesVerified=true;
    nativeHypothesis.sameEmeraldGameThroughoutVerified=true;
    nativeHypothesis.noOtherMoveChangesVerified=true;
    nativeHypothesis.allOffersAndDecisionsVerified=true;
    nativeHypothesis.initialMoves={33,45,0,0};
    nativeHypothesis.observedMoves={nativeTimeline.move(0),
                                   nativeTimeline.move(1),
                                   nativeTimeline.move(2),
                                   nativeTimeline.move(3)};
    nativeHypothesis.stepCount=7;
    nativeHypothesis.steps[0]={7,73,Timeline::Decision::AutoLearn,2};
    nativeHypothesis.steps[1]={10,22,Timeline::Decision::AutoLearn,3};
    nativeHypothesis.steps[2]={15,77,Timeline::Decision::ReplaceSlot,0};
    nativeHypothesis.steps[3]={15,79,Timeline::Decision::ReplaceSlot,1};
    nativeHypothesis.steps[4]={20,75,Timeline::Decision::ReplaceSlot,2};
    nativeHypothesis.steps[5]={25,230,Timeline::Decision::Decline,0};
    nativeHypothesis.steps[6]={32,74,Timeline::Decision::Decline,0};
    const auto nativeResult=Timeline::evaluate(nativeHypothesis);
    assert(nativeResult.status==Timeline::Status::CandidateConsistent);
    auto detachedHistory=nativeHypothesis;
    detachedHistory.sameEmeraldGameThroughoutVerified=false;
    assert(Timeline::evaluate(detachedHistory).status==
           Timeline::Status::UnknownGameContinuity);
    // Production native-format PK3 + encrypted PK4 transfer boundary:
    // the analyzer can read the exact stored four move slots from each.
    // This is a SYNTHETIC PAIR of well-formed native records, not an
    // original paired retail Pal Park save or inferred actual history.
    std::vector<std::byte> rawTransferred(Encryption::SIZE_STORED4,
                                           std::byte{0});
    wr32(rawTransferred,0x00,0x12345678u);
    wr16(rawTransferred,0x08,1); // Bulbasaur
    wr16(rawTransferred,0x0C,12345);
    wr16(rawTransferred,0x0E,54321);
    wr16(rawTransferred,0x28,77); // Poison Powder
    wr16(rawTransferred,0x2A,79); // Sleep Powder
    wr16(rawTransferred,0x2C,75); // Razor Leaf
    wr16(rawTransferred,0x2E,22); // Vine Whip
    rawTransferred[0x17]=std::byte{2};
    rawTransferred[0x5F]=std::byte{3}; // Original Emerald
    rawTransferred[0x83]=std::byte{4};
    const auto encryptedTransferred=Encryption::encryptArray4(rawTransferred);
    Pokemon::Pokemon4ReadOnly nativeArrival(
        encryptedTransferred,Enums::GameVersion::PT);
    assert(nativeArrival.valid() && nativeArrival.checksumValid());
    Pokemon::Pokemon4ReadOnlyView arrivalView(nativeArrival);
    assert(arrivalView.speciesID()==nativeTimeline.speciesID());
    namespace EmeraldPark=Legality::Gen3EmeraldPalParkJointPath;
    EmeraldPark::Input paired{};
    paired.learning=nativeHypothesis;
    paired.migration.sourceGame="emerald_gba";
    paired.migration.destinationGame="platinum_nds";
    paired.migration.species=arrivalView.speciesID();
    paired.migration.independentlyVerifiedPreTransfer=true;
    paired.migration.independentlyVerifiedImmediateArrival=true;
    paired.migration.unchangedSpeciesThroughArrivalVerified=true;
    paired.migration.outboundGbaMoves={nativeTimeline.move(0),
        nativeTimeline.move(1),nativeTimeline.move(2),nativeTimeline.move(3)};
    paired.migration.immediateNdsMoves={arrivalView.move(0),
        arrivalView.move(1),arrivalView.move(2),arrivalView.move(3)};
    paired.sameOriginalSourceSnapshotVerified=true;
    paired.originalLearningStageToken=0x1234u;
    paired.outboundGbaStageToken=0x1234u;
    assert(EmeraldPark::evaluate(paired).status==
           EmeraldPark::Status::KnownConditionalCombinedPath);
    auto notHistoricallyLinked=paired;
    notHistoricallyLinked.sameOriginalSourceSnapshotVerified=false;
    assert(EmeraldPark::evaluate(notHistoricallyLinked).status==
           EmeraldPark::Status::UnknownSharedSource);

    // One explicitly hypothetical subsequent encrypted Gen IV PK4
    // snapshot after migration; retained identity does not prove its history.
    auto rawLater=rawTransferred;
    wr16(rawLater,0x28,15);
    wr16(rawLater,0x2A,92);
    auto encryptedLater=Encryption::encryptArray4(rawLater);
    Pokemon::Pokemon4ReadOnly nativeLater(
        encryptedLater,Enums::GameVersion::PT);
    assert(nativeLater.valid() && nativeLater.checksumValid());
    Pokemon::Pokemon4ReadOnlyView laterView(nativeLater);
    namespace Later=Legality::Gen34PostPalParkLearning;
    Later::Input post{};
    post.crossing=paired.migration;
    post.samePokemonAndGameContinuityVerified=true;
    post.fullPostArrivalSequenceVerified=true;
    post.arrivalLevel=32;post.observedLevel=50;
    post.stepCount=2;
    post.steps[0]={40,1,15,Later::Method::HiddenMachine,0,true};
    post.steps[1]={50,2,92,Later::Method::TechnicalMachine,1,true};
    post.observedMoves={laterView.move(0),laterView.move(1),
                        laterView.move(2),laterView.move(3)};
    assert(Later::evaluate(post).status==Later::Status::CandidateConsistent);
    auto unknownTeaching=post;
    unknownTeaching.steps[1].independentlyVerifiedMethod=false;
    assert(Later::evaluate(unknownTeaching).status==
           Later::Status::UnknownMethodSource);
    auto illegalGbaCarry=post;
    illegalGbaCarry.crossing.outboundGbaMoves[0]=15;
    assert(Later::evaluate(illegalGbaCarry).status==
           Later::Status::CandidateContradicted);

    // Calling the full analyzer by itself must not imply a witnessed
    // historical sequence; no stored PK3 encodes these seven choices.
    constexpr auto jointEmeraldText="Emerald independent direct level-up opportunities:";
    auto emeraldJoint=gen3WithBall(4);
    emeraldJoint.setOriginGame(3);
    emeraldJoint.setSpecies(1);
    emeraldJoint.setLevel(20);
    emeraldJoint.setMove(0,33);
    emeraldJoint.setMove(1,75);
    emeraldJoint.refreshChecksum();
    assert(hasInfo(Legality::analyze(
        emeraldJoint,Enums::GameVersion::FRLG),jointEmeraldText));
    assert(hasInfo(Legality::analyze(
        emeraldJoint,Enums::GameVersion::FRLG,"emerald_gba"),jointEmeraldText));
    emeraldJoint.setMove(1,14); // Valid tutor possibility; no level-up witness.
    emeraldJoint.refreshChecksum();
    assert(!hasInfo(Legality::analyze(
        emeraldJoint,Enums::GameVersion::FRLG),jointEmeraldText));
    emeraldJoint.setMove(1,75);
    emeraldJoint.setLevel(19);
    emeraldJoint.refreshChecksum();
    assert(!hasInfo(Legality::analyze(
        emeraldJoint,Enums::GameVersion::FRLG),jointEmeraldText));
    emeraldJoint.setLevel(20);
    emeraldJoint.setOriginGame(2); // Original Ruby != Emerald.
    emeraldJoint.refreshChecksum();
    assert(!hasInfo(Legality::analyze(
        emeraldJoint,Enums::GameVersion::FRLG),jointEmeraldText));

    // BREED-001: construct ordered III/IV ordinary eggs from explicitly
    // hypothetical already-compatible parents, never infer parent identity
    // from observed child move membership. Duplicates are suppressed and
    // overflow drops the oldest move, without sorting the move IDs.
    namespace OrdinaryEgg=Legality::Gen34OrdinaryEggMoveConstruction;
    constexpr auto basePichu=OrdinaryEgg::Input{
        "emerald_gba",172,{84,204,0,0},{},{},{},true,false};
    constexpr auto baseEgg=OrdinaryEgg::construct(basePichu);
    static_assert(baseEgg.matches({84,204,0,0}));
    constexpr auto voltEgg=OrdinaryEgg::construct(OrdinaryEgg::Input{
        "emerald_gba",172,{84,204,0,0},{},{},{},true,true});
    static_assert(voltEgg.matches({84,204,344,0}));
    static_assert(!voltEgg.matches({344,84,204,0}));
    constexpr auto noVoltRuby=OrdinaryEgg::construct(OrdinaryEgg::Input{
        "ruby_gba",172,{84,204,0,0},{},{},{},true,true});
    static_assert(noVoltRuby.matches({84,204,0,0}));
    constexpr auto gen4Volt=OrdinaryEgg::construct(OrdinaryEgg::Input{
        "platinum_nds",172,{84,204,0,0},{},{},{},true,true});
    static_assert(gen4Volt.matches({84,204,344,0}));
    constexpr auto parentalUnknown=OrdinaryEgg::construct(
        OrdinaryEgg::Input{"emerald_gba",172,{84,204,0,0}});
    static_assert(parentalUnknown.status==
        OrdinaryEgg::Status::UnknownLightBallState);
    constexpr auto shiftedEgg=OrdinaryEgg::construct(OrdinaryEgg::Input{
        "emerald_gba",25,{1,2,3,4},{5,6,0,0},{6,7,0,0},
        {8,8,0,0},true,false});
    static_assert(shiftedEgg.matches({5,6,7,8}));
    constexpr auto absentGame=OrdinaryEgg::construct(
        OrdinaryEgg::Input{"unknown",172,{84,204,0,0}});
    static_assert(absentGame.status==OrdinaryEgg::Status::UnsupportedGame);

    // Actual native PK3 -> immutable production analyzer route:
    // retained Emerald origin plus consistent unhatched met-level-0 field.
    constexpr auto ordinaryText="Emerald Pichu ordinary egg construction:";
    auto pichuEgg=gen3WithBall(4);
    pichuEgg.setSpecies(172);
    pichuEgg.setOriginGame(3);
    pichuEgg.setMetLevel(0);
    pichuEgg.setEgg(true);
    pichuEgg.setMove(0,84);
    pichuEgg.setMove(1,204);
    pichuEgg.setMove(2,344);
    pichuEgg.refreshChecksum();
    assert(hasInfo(Legality::analyze(
        pichuEgg,Enums::GameVersion::FRLG),ordinaryText));
    assert(!hasInvalidText(Legality::analyze(
        pichuEgg,Enums::GameVersion::FRLG),ordinaryText));
    pichuEgg.setMove(0,344);
    pichuEgg.setMove(1,84);
    pichuEgg.setMove(2,204);
    pichuEgg.refreshChecksum();
    assert(!hasInfo(Legality::analyze(
        pichuEgg,Enums::GameVersion::FRLG),ordinaryText));
    pichuEgg.setMove(0,84);
    pichuEgg.setMove(1,204);
    pichuEgg.setMove(2,344);
    pichuEgg.setEgg(false); // Hatched can reorder and relearn moves.
    pichuEgg.refreshChecksum();
    assert(!hasInfo(Legality::analyze(
        pichuEgg,Enums::GameVersion::FRLG),ordinaryText));
    pichuEgg.setEgg(true);
    pichuEgg.setOriginGame(2); // Ruby was before Volt Tackle breeding.
    pichuEgg.refreshChecksum();
    assert(!hasInfo(Legality::analyze(
        pichuEgg,Enums::GameVersion::FRLG),ordinaryText));

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
    // META-001/EVENT-001: native encrypted PK4 keeps exact raw OT and
    // nickname bytes; neither encoded terminators nor post-terminator
    // evidence can be reconstructed from converted Unicode strings.
    constexpr std::array<std::byte,22> rawNickWithTerm{
        std::byte{0x41},std::byte{0},std::byte{0xFF},std::byte{0xFF},
        std::byte{0xEF},std::byte{0xBE}};
    constexpr std::array<std::byte,16> rawOTWithTerm{
        std::byte{0x52},std::byte{0},std::byte{0xFF},std::byte{0xFF},
        std::byte{0xA5},std::byte{0xA5}};
    namespace RawText=Legality::Gen4RawTextEvidence;
    static_assert(RawText::classify(false,rawNickWithTerm,rawOTWithTerm)
                  .status==RawText::Status::Unavailable);
    static_assert(RawText::classify(true,rawNickWithTerm,rawOTWithTerm)
                  .fullyTerminated());
    static_assert(RawText::classify(true,rawNickWithTerm,rawOTWithTerm)
                  .nickname.postTerminatorNonzero);
    constexpr auto rawTextFinding="Native PK4 raw nickname and OT text fields";
    assert(hasInfo(analyzeNativeGen4RawText(true,true,true),rawTextFinding));
    assert(hasInfo(analyzeNativeGen4RawText(true,true,false,true),rawTextFinding));
    assert(!hasText(analyzeNativeGen4RawText(true,false,true),rawTextFinding));
    assert(!hasText(analyzeNativeGen4RawText(false,true,true),rawTextFinding));
    assert(!hasText(analyzeNativeGen4RawText(false,false,false),rawTextFinding));
    assert(!hasInvalidText(analyzeNativeGen4RawText(true,true,true),
                           rawTextFinding));
    std::array<std::byte,22> absentNickname{};
    std::array<std::byte,16> absentOT{};
    assert(!gen3WithBall(4).gen4RawTextFields(absentNickname,absentOT));

    // EVENT-001: native immutable PK4 ribbon DWORD capability. The
    // candidate checker never infers template ribbons from missing PCD data.
    namespace EventRibbons=Legality::Gen4EventRibbonCandidate;
    constexpr auto eventSource=Legality::Gen4EventTemplate::Candidate{
        4,7200,58364,0x0f199b50u,40,3053,16,0,0,1,7,0,true};
    constexpr EventRibbons::Requirement requirement{
        true,{1u,2u,4u},{8u,16u,32u}
    };
    constexpr auto match=EventRibbons::evaluate(
        eventSource,67,true,{1u,2u,4u},requirement);
    static_assert(match.status==EventRibbons::Status::CandidateCompatible);
    static_assert(EventRibbons::evaluate(
        eventSource,67,true,{0u,2u,4u},requirement).status==
        EventRibbons::Status::CandidateContradicted);
    static_assert(EventRibbons::evaluate(
        eventSource,67,false,{1u,2u,4u},requirement).status==
        EventRibbons::Status::UnavailableRibbonData);
    static_assert(EventRibbons::evaluate(
        eventSource,67,true,{1u,2u,4u},{}).status==
        EventRibbons::Status::UnknownEventRibbonTemplate);
    static_assert(EventRibbons::evaluate(
        eventSource,0,true,{1u,2u,4u},requirement).status==
        EventRibbons::Status::IdentityMismatch);
    assert(!hasInvalidText(analyzeNativeGen4RibbonWords(1,2,4),
                           "event ribbon"));

    // META-001: native encrypted PK4 0x60 low twenty ribbon bits.
    // Prefixes are rank-chain only; they do not establish actual wins.
    namespace Ribbons=Legality::Gen4ContestRibbon;
    static_assert(Ribbons::evaluate(false,{8,0,0,0,0}).status==
                  Ribbons::Status::Unavailable);
    static_assert(Ribbons::evaluate(true,{0,1,3,7,15}).status==
                  Ribbons::Status::Consistent);
    static_assert(Ribbons::evaluate(true,{15,15,15,15,15}).status==
                  Ribbons::Status::Consistent);
    static_assert(Ribbons::evaluate(true,{8,0,0,0,0}).status==
                  Ribbons::Status::RankGap);
    static_assert(Ribbons::evaluate(true,{8,0,5,0,0}).gapCategoriesMask==5);
    constexpr auto ribbonGap="Native PK4 Sinnoh contest-ribbon rank sequence";
    const auto ribbonGood=analyzeNativeGen4ContestRanks(0xF7310,false);
    assert(!hasText(ribbonGood,ribbonGap));
    const auto ribbonOnlyMaster=analyzeNativeGen4ContestRanks(8,false);
    assert(hasText(ribbonOnlyMaster,ribbonGap));
    assert(!hasInvalidText(ribbonOnlyMaster,ribbonGap));
    assert(hasText(analyzeNativeGen4ContestRanks(8,true),ribbonGap));
    assert(hasText(analyzeNativeGen4ContestRanks(0x505,false),ribbonGap));
    assert(!hasText(analyzeNativeGen4ContestRanks(0,false),ribbonGap));
    assert(!hasText(Legality::analyze(
        gen3WithBall(4),Enums::GameVersion::FRLG),ribbonGap));

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
    // TRANSFER-001: Gen III HM #148 Flash in a native encrypted
    // Pal Park PK4 can have a post-transfer Gen IV candidate (TM70).
    // A stored Gen III HM never proves that the old HM survived the edge.
    // No hard Invalid follows from unsupported or incomplete histories.
    namespace PalHM=Legality::Gen34PalParkHMHistory;
    static_assert(PalHM::isGen3HM(15) && PalHM::isGen3HM(19) &&
                  PalHM::isGen3HM(57) && PalHM::isGen3HM(70) &&
                  PalHM::isGen3HM(148) && PalHM::isGen3HM(249) &&
                  PalHM::isGen3HM(127) && PalHM::isGen3HM(291));
    static_assert(!PalHM::isGen3HM(85) &&
                  !PalHM::isGen3HM(0) &&
                  !PalHM::isGen3HM(468));
    const auto hmPositive=PalHM::classify(25,0,{148,85,0,0});
    assert(hmPositive.storedGen3HM==1 &&
           hmPositive.gen4PoolCandidates==1 &&
           hmPositive.unmodeledReacquisitions==0);
    const auto hmUnmodeled=PalHM::classify(25,0,{291,85,0,0});
    assert(hmUnmodeled.storedGen3HM==1 &&
           hmUnmodeled.gen4PoolCandidates==0 &&
           hmUnmodeled.unmodeledReacquisitions==1);
    const auto hmNone=PalHM::classify(25,0,{85,0,0,0});
    assert(hmNone.storedGen3HM==0);
    constexpr auto hmText="Pal Park HM chronology:";
    const auto nativeHM=analyzeDetachedPalParkMoves(2,148,85);
    assert(hasInfo(nativeHM,hmText));
    assert(hasInfo(nativeHM,"possible Gen IV learning pools"));
    assert(!hasInvalidText(nativeHM,hmText));
    const auto exactNativeHM=analyzeDetachedPalParkMoves(2,148,85,true);
    assert(hasInfo(exactNativeHM,hmText));
    const auto nativeUnknownHM=analyzeDetachedPalParkMoves(2,291,85);
    assert(hasInfo(nativeUnknownHM,hmText));
    assert(hasInfo(nativeUnknownHM,"alternative event and transfer histories"));
    assert(!hasInvalidText(nativeUnknownHM,hmText));
    assert(!hasText(analyzeDetachedPalParkMoves(
        2,148,85,false,false),hmText)); // No Pal Park marker.
    assert(!hasText(analyzeDetachedPalParkMoves(
        2,148,85,false,true,true),hmText)); // Invalid Ball.
    assert(!hasText(analyzeDetachedPalParkMoves(
        2,148,85,false,true,false,true),hmText)); // Invalid split fields.
    assert(!hasText(analyzeDetachedPalParkMoves(2,85,86),hmText));

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

    // GameCube origin code 15 can be retained through Pal Park via
    // an *unspecified* intermediary GBA save. Only the universal PK3
    // move-ID ceiling survives. No named Ruby/Emerald/FRLG move pool.
    using Legality::Gen34PalParkMoves::postMigrationMoveIds;
    static_assert(postMigrationMoveIds({85,363,467,0})==2);
    static_assert(postMigrationMoveIds({85,354,468,0})==0);
    static_assert(postMigrationMoveIds({0,0,0,0})==0);
    constexpr auto gcFree="Source-free GameCube-origin Pal Park PK4 move-stage evidence:";
    constexpr auto gcExact="Exact-source GameCube-origin Pal Park PK4 move-stage evidence:";
    const auto gamecubeFree=analyzeDetachedPalParkMoves(15,85,363);
    assert(hasInfo(gamecubeFree,gcFree));
    assert(!hasText(gamecubeFree,"Source-free Pal Park PK4 joint history bound:"));
    assert(!hasText(gamecubeFree,"original GBA game's move pool"));
    const auto gamecubeExact=analyzeDetachedPalParkMoves(15,85,363,true);
    assert(hasInfo(gamecubeExact,gcExact));
    assert(!hasText(gamecubeExact,gcFree));
    assert(!hasText(analyzeDetachedPalParkMoves(15,85,468),gcFree));
    assert(!hasText(analyzeDetachedPalParkMoves(15,85,354),gcFree));
    assert(!hasText(analyzeDetachedPalParkMoves(15,85,363,false,true,false,false,387),gcFree));
    assert(!hasText(analyzeDetachedPalParkMoves(15,85,363,true,true,false,false,387),gcExact));
    assert(!hasText(analyzeDetachedPalParkMoves(15,85,363,false,false),gcFree));
    assert(!hasText(analyzeDetachedPalParkMoves(
        15,85,363,false,true,false,true),gcFree));
    assert(!hasText(analyzeDetachedPalParkMoves(
        15,85,363,false,true,true,false),gcFree));
    assert(!hasText(analyzeDetachedPalParkMoves(2,85,363),gcFree));
    assert(!hasText(analyzeDetachedPalParkMoves(10,85,363),gcFree));
    // Stored original game, NOT current Platinum save, controls which
    // egg-move family is an original-family compatibility candidate.
    constexpr auto sourceEggFamily="Gen IV egg-origin four-slot evidence:";
    const auto diamondEgg=analyzeNativeGen4EggMoveFamilies(10,80,124);
    assert(hasInfo(diamondEgg,
        "1 move(s) match an audited egg-move pool in the stored original retail game's family"));
    assert(hasInfo(diamondEgg,
        "1 move(s) match an audited different Gen IV egg-move family"));
    const auto hgssEgg=analyzeNativeGen4EggMoveFamilies(7,80,124);
    assert(hasInfo(hgssEgg,
        "2 move(s) match an audited egg-move pool in the stored original retail game's family"));
    assert(!hasInfo(hgssEgg,
        "different Gen IV egg-move family"));
    assert(hasInfo(analyzeNativeGen4EggMoveFamilies(10,80,124,true),
        "different Gen IV egg-move family"));
    assert(!hasText(analyzeNativeGen4EggMoveFamilies(10,80,124,false,false),
        sourceEggFamily));
    assert(!hasText(analyzeNativeGen4EggMoveFamilies(2,80,124),
        sourceEggFamily));
    assert(!hasText(analyzeNativeGen4EggMoveFamilies(10,57,0),
        sourceEggFamily));

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
