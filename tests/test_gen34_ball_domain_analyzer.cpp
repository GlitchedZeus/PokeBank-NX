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
        uint8_t metLevel, uint8_t ball, uint8_t originVersion = 7) {
    std::vector<std::byte> raw(Encryption::SIZE_STORED4, std::byte{0});
    wr32(raw, 0x00, 0x12345678u);
    wr16(raw, 0x08, species);
    wr16(raw, 0x0C, 12345);
    wr16(raw, 0x0E, 54321);
    wr32(raw, 0x10, 3375u);
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

int main() {
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
