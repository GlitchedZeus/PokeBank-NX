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

// Legality.cpp keeps historical Trainer namespace wrappers. Keep this regression
// independent of the full Trainer UI implementation, matching the existing
// exact-source legality-context test harness.
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

Pokemon::Pokemon3FRLG makeCandidate(uint8_t language) {
    std::array<uint8_t, 80> raw{};
    Pokemon::Pokemon3FRLG p(std::span<const std::byte>(
        reinterpret_cast<const std::byte*>(raw.data()), raw.size()));
    p.setPID(0x12345678u);
    p.setTID16(12345);
    p.setSID16(54321);
    p.setLanguage(language);
    p.setSpecies(1);       // Bulbasaur
    p.setOTName(u"RED");
    p.setNickname(u"BULBASAUR");
    p.setOriginGame(2);    // Ruby; a Gen IV exact-source context treats this as Pal Park history.
    p.setBall(4);
    p.setMetLevel(5);
    p.setLevel(5);
    p.refreshChecksum();
    return p;
}

void put16(std::vector<std::byte>& data, size_t offset, uint16_t value) {
    data[offset] = static_cast<std::byte>(value & 0xFFu);
    data[offset + 1] = static_cast<std::byte>(value >> 8);
}
void put32(std::vector<std::byte>& data, size_t offset, uint32_t value) {
    data[offset] = static_cast<std::byte>(value & 0xFFu);
    data[offset + 1] = static_cast<std::byte>((value >> 8) & 0xFFu);
    data[offset + 2] = static_cast<std::byte>((value >> 16) & 0xFFu);
    data[offset + 3] = static_cast<std::byte>((value >> 24) & 0xFFu);
}

// Source-backed encrypted PK4 -> immutable reader -> shared view -> analyzer.
// A PK3 fixture with exact Gen IV source context alone cannot prove PK4 decoding.
Legality::Report analyzeNativePK4Language(uint8_t language, bool exactSource=true) {
    std::vector<std::byte> decrypted(Encryption::SIZE_STORED4, std::byte{0});
    put32(decrypted, 0x00, 0x12345678u);
    put16(decrypted, 0x08, 1);       // Bulbasaur
    put16(decrypted, 0x0C, 12345);   // Trainer ID
    put16(decrypted, 0x0E, 54321);   // Secret ID
    put32(decrypted, 0x10, 135);     // Exp (level 5)
    decrypted[0x15] = std::byte{65}; // Overgrow
    decrypted[0x17] = static_cast<std::byte>(language);
    decrypted[0x5F] = std::byte{12}; // Platinum origin marker
    decrypted[0x83] = std::byte{4};  // Poké Ball
    const auto encrypted = Encryption::encryptArray4(decrypted);
    assert(encrypted.size() == Encryption::SIZE_STORED4);
    Pokemon::Pokemon4ReadOnly source(encrypted, Enums::GameVersion::PT);
    assert(source.valid());
    assert(source.language() == language);
    Pokemon::Pokemon4ReadOnlyView view(source);
    assert(view.language() == language);
    return exactSource
        ? Legality::analyze(view, Enums::GameVersion::PT, "platinum_nds")
        : Legality::analyze(view, Enums::GameVersion::PT);
}
}

int main() {
    // Pinned PKHeX GetMaxLanguageID: Gen III tops out at Spanish (7).
    auto gen3Bad = makeCandidate(8);
    const auto rubyBad = Legality::analyze(
        gen3Bad, Enums::GameVersion::FRLG, "ruby_gba");
    assert(rubyBad.hasInvalid());
    assert(hasText(rubyBad, "Language id 8 cannot exist in Generation 3"));

    auto gen3Good = makeCandidate(7);
    const auto rubyGood = Legality::analyze(
        gen3Good, Enums::GameVersion::FRLG, "ruby_gba");
    assert(!hasText(rubyGood, "Language id 7 cannot exist in Generation 3"));

    // Zero remains conservative/unresolved because PokeBank uses it for unwired/unknown fields.
    auto gen3Unknown = makeCandidate(0);
    const auto rubyUnknown = Legality::analyze(
        gen3Unknown, Enums::GameVersion::FRLG, "ruby_gba");
    assert(!hasText(rubyUnknown, "Language id 0 cannot exist"));
    assert(!hasText(rubyUnknown, "Invalid language id (0)"));

    // Pinned PKHeX GetMaxLanguageID: Gen IV tops out at Korean (8).
    // The concrete record here remains PK3 deliberately: exactGeneration is a source-context
    // property, so this proves the central analyzer dispatch rather than a format accessor.
    auto gen4Bad = makeCandidate(9);
    const auto platinumBad = Legality::analyze(
        gen4Bad, Enums::GameVersion::FRLG, "platinum_nds");
    assert(platinumBad.hasInvalid());
    assert(hasText(platinumBad, "Language id 9 cannot exist in Generation 4"));

    auto gen4Good = makeCandidate(8);
    const auto platinumGood = Legality::analyze(
        gen4Good, Enums::GameVersion::FRLG, "platinum_nds");
    assert(!hasText(platinumGood, "Language id 8 cannot exist in Generation 4"));

    auto gen4Unknown = makeCandidate(0);
    const auto platinumUnknown = Legality::analyze(
        gen4Unknown, Enums::GameVersion::FRLG, "platinum_nds");
    assert(!hasText(platinumUnknown, "Language id 0 cannot exist"));
    assert(!hasText(platinumUnknown, "Invalid language id (0)"));

    // Source-free PK3 remains format-known: the Generation III language
    // maximum and unused ID 6 are provable without knowing the source game.
    auto genericEight = makeCandidate(8);
    const auto bankEight = Legality::analyze(genericEight, Enums::GameVersion::FRLG);
    assert(hasText(bankEight, "Language id 8 cannot exist in Generation 3"));
    assert(bankEight.hasInvalid());

    auto genericUnused = makeCandidate(6);
    const auto bankUnused = Legality::analyze(genericUnused, Enums::GameVersion::FRLG);
    assert(hasText(bankUnused, "Language id 6 cannot exist in Generation 3"));
    assert(bankUnused.hasInvalid());
    auto genericSeven = makeCandidate(7);
    const auto bankSeven = Legality::analyze(genericSeven, Enums::GameVersion::FRLG);
    assert(!hasText(bankSeven,"Language id 7 cannot exist in Generation 3"));


    // Pinned PKHeX LanguageVerifier rejects unused 6; Legal.GetMaxLanguageID
    // accepts Gen IV up to Korean 8. Test the real encrypted PK4 read path.
    const auto nativeUnused = analyzeNativePK4Language(6);
    assert(nativeUnused.hasInvalid());
    assert(hasText(nativeUnused, "Language id 6 cannot exist in Generation 4"));

    const auto nativeMaximum = analyzeNativePK4Language(8);
    assert(!hasText(nativeMaximum, "Language id 8 cannot exist in Generation 4"));

    const auto nativeTooHigh = analyzeNativePK4Language(9);
    assert(nativeTooHigh.hasInvalid());
    assert(hasText(nativeTooHigh, "Language id 9 cannot exist in Generation 4"));

    // Native encrypted PK4, with no exact source save ID, still has
    // independently verified Generation IV language domain evidence.
    assert(hasText(analyzeNativePK4Language(9,false),
                   "Language id 9 cannot exist in Generation 4"));
    assert(!hasText(analyzeNativePK4Language(8,false),
                    "Language id 8 cannot exist in Generation 4"));

    // Value zero is an unresolved/unwired sentinel, not hard-invalid.
    const auto nativeUnwired = analyzeNativePK4Language(0);
    assert(!hasText(nativeUnwired, "Language id 0 cannot exist"));
    assert(!hasText(nativeUnwired, "Invalid language id (0)"));

    return 0;
}
