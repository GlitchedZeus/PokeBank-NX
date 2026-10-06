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

    return 0;
}
