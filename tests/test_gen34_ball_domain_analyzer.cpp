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

Legality::Report analyzeGen4Ball(uint8_t ball, bool exactSource = true) {
    std::vector<std::byte> decrypted(Encryption::SIZE_STORED4, std::byte{0});
    wr32(decrypted, 0x00, 0x12345678u);
    wr16(decrypted, 0x08, 1);       // Bulbasaur
    wr16(decrypted, 0x0C, 12345);
    wr16(decrypted, 0x0E, 54321);
    wr32(decrypted, 0x10, 135);     // level 5, Medium Slow
    decrypted[0x15] = std::byte{65}; // Overgrow
    decrypted[0x17] = std::byte{2};  // English
    decrypted[0x5F] = std::byte{12}; // Platinum origin value; exact provenance is not under test.
    decrypted[0x83] = static_cast<std::byte>(ball);

    const auto encrypted = Encryption::encryptArray4(decrypted);
    assert(encrypted.size() == Encryption::SIZE_STORED4);
    Pokemon::Pokemon4ReadOnly source(encrypted, Enums::GameVersion::PT);
    assert(source.valid());
    assert(source.ballDPPt() == ball);
    Pokemon::Pokemon4ReadOnlyView view(source);
    assert(view.ball() == ball);

    return exactSource
        ? Legality::analyze(view, Enums::GameVersion::PT, "platinum_nds")
        : Legality::analyze(view, Enums::GameVersion::PT);
}

Legality::Report analyzeNativeHgssSpecialBall(
        uint16_t species, uint16_t metLocation,
        uint8_t metLevel, uint8_t ball, uint8_t originVersion = 7,
        uint32_t experience = 3375, uint32_t pid = 0x12345678u) {
    std::vector<std::byte> raw(Encryption::SIZE_STORED4, std::byte{0});
    wr32(raw, 0x00, pid);
    wr16(raw, 0x08, species);
    wr16(raw, 0x0C, 12345);
    wr16(raw, 0x0E, 54321);
    wr32(raw, 0x10, experience);
    raw[0x17] = std::byte{2};
    raw[0x5F] = static_cast<std::byte>(originVersion);
    wr16(raw, 0x46, metLocation); // Extended Pt/HGSS location.
    raw[0x83] = std::byte{4};     // D/P/Pt Poké Ball shadow byte.
    raw[0x84] = static_cast<std::byte>(metLevel);
    raw[0x86] = static_cast<std::byte>(ball); // HG/SS special ball byte.
    const auto encrypted = Encryption::encryptArray4(raw);
    Pokemon::Pokemon4ReadOnly source(encrypted, Enums::GameVersion::HGSS);
    assert(source.valid());
    assert(source.originVersion() == originVersion);
    assert(source.metLocationExtended() == metLocation);
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
        uint8_t ball, uint8_t originVersion = 10) {
    std::vector<std::byte> raw(Encryption::SIZE_STORED4, std::byte{0});
    wr32(raw, 0x00, 0x12345678u);
    wr16(raw, 0x08, species);
    wr16(raw, 0x0C, 12345);
    wr16(raw, 0x0E, 54321);
    wr32(raw, 0x10, 3375u);
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
                                    uint8_t originVersion=7) {
    std::vector<std::byte> raw(Encryption::SIZE_STORED4,std::byte{0});
    wr32(raw,0x00,0x12345678u);
    wr16(raw,0x08,25);
    wr16(raw,0x0C,12345);
    wr16(raw,0x0E,54321);
    raw[0x17]=std::byte{2};
    raw[0x5F]=static_cast<std::byte>(originVersion); // stored PK4 origin
    wr16(raw,0x44,eggLocation);
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
    return Legality::analyze(view,Enums::GameVersion::HGSS,"heartgold_nds");
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
    // Pal Park/Gen III origin is NOT a native Gen IV egg history, even
    // if a malformed record supplies egg-location fields and Poké Ball.
    const auto palParkOrigin=analyzeGen4EggBall(4,true,2000,0,2);
    assert(!hasInfo(palParkOrigin,
        "Native Generation III/IV egg origin has a compatible Poke Ball"));
    const auto unknownOrigin=analyzeGen4EggBall(4,true,2000,0,0);
    assert(!hasInfo(unknownOrigin,
        "Native Generation III/IV egg origin has a compatible Poke Ball"));

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

    // Without exact source identity, retain the historical generic fallback rather
    // than inferring a generation from the concrete view class.
    const auto generic25 = analyzeGen4Ball(25, false);
    assert(!hasText(generic25, "cannot exist in Generation 4"));
    assert(!hasText(generic25, "Ball id out of range"));

    const auto generic38 = analyzeGen4Ball(38, false);
    assert(hasText(generic38, "Ball id out of range (38)"));
    assert(generic38.hasInvalid());

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
