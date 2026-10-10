#include "Encryption/Encryption4.h"
#include "Legality/Legality.h"
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
                                 uint16_t species=1, uint16_t move=0) {
    std::vector<std::byte> decrypted(Encryption::SIZE_STORED4, std::byte{0});
    wr32(decrypted, 0x00, 0x12345678u);
    wr16(decrypted, 0x08, species);  // native stored PK4 species
    wr16(decrypted, 0x0C, 12345);
    wr16(decrypted, 0x0E, 54321);
    wr32(decrypted, 0x10, 135);     // level 5, Medium Slow
    wr16(decrypted, 0x28, move);   // native stored PK4 first move
    decrypted[0x15] = std::byte{65}; // Overgrow
    decrypted[0x17] = std::byte{2};  // English
    decrypted[0x5F] = std::byte{12}; // Platinum origin value; exact provenance is not under test.
    decrypted[0x83] = static_cast<std::byte>(ball);

    const auto encrypted = Encryption::encryptArray4(decrypted);
    assert(encrypted.size() == Encryption::SIZE_STORED4);
    Pokemon::Pokemon4ReadOnly source(encrypted, Enums::GameVersion::PT);
    assert(source.valid());
    assert(source.ballDPPt() == ball);
    assert(source.species() == species && source.moves()[0] == move);
    Pokemon::Pokemon4ReadOnlyView view(source);
    assert(view.ball() == ball);

    return exactSource
        ? Legality::analyze(view, Enums::GameVersion::PT, "platinum_nds")
        : Legality::analyze(view, Enums::GameVersion::PT);
}

Legality::Report analyzeNativeHgssSpecialBall(
        uint16_t species, uint16_t metLocation,
        uint8_t metLevel, uint8_t ball, uint8_t originVersion = 7,
        uint32_t experience = 3375, uint32_t pid = 0x12345678u,
        uint16_t eggLocation = 0) {
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
    return Legality::analyze(
        view, Enums::GameVersion::HGSS, "heartgold_nds");
}
}

Legality::Report analyzeNativeDiamondMarshBall(
        uint16_t species, uint16_t metLocation, uint8_t metLevel,
        uint8_t ball, uint8_t originVersion = 10,
        uint32_t experience = 3375) {
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
    return Legality::analyze(view, Enums::GameVersion::DP, "diamond_nds");
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
                                   uint8_t ballHGSS,bool isEgg=false) {
    // Actual stored/encrypted PK4 format. The exact container game identity
    // is intentionally absent; stored Gen III origin remains independently
    // recoverable and can still prove universal transfer field rules.
    std::vector<std::byte> raw(Encryption::SIZE_STORED4,std::byte{0});
    wr32(raw,0x00,0x12345678u);
    wr16(raw,0x08,25); // Pikachu
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

int main() {
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

    // Source-free known native PK3/PK4 formats have generation-specific
    // species/move ceilings regardless of their exact save identity.
    const auto g3Known=Legality::analyze(gen3WithBall(4),Enums::GameVersion::FRLG);
    assert(!hasInvalidText(g3Known,"cannot exist in a Generation 3 save"));
    auto g3NewSpecies=gen3WithBall(4);
    g3NewSpecies.setSpecies(387); // Turtwig is Gen IV.
    const auto g3SpeciesBad=Legality::analyze(g3NewSpecies,Enums::GameVersion::FRLG);
    assert(hasInvalidText(g3SpeciesBad,"Species 387 cannot exist in a Generation 3 save"));
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
    const auto unknownOrigin=sourceFreePalPark(0,0,0,4,0);
    assert(hasText(unknownOrigin,
        "PK4 origin game could not be mapped to a known generation"));
    assert(!hasInvalidText(unknownOrigin,"Pal Park"));
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
