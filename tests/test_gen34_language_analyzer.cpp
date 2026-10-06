#include "Legality/Legality.h"
#include "Names/ItemNames.h"
#include "Names/SpeciesNames.h"
#include "Pokemon/Pokemon3FRLG.h"

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>

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

    // No exact source keeps the historical generic-format behavior: 8 is not generically
    // rejected, while unused language id 6 still is.
    auto genericEight = makeCandidate(8);
    const auto bankEight = Legality::analyze(genericEight, Enums::GameVersion::FRLG);
    assert(!hasText(bankEight, "Invalid language id (8)"));

    auto genericUnused = makeCandidate(6);
    const auto bankUnused = Legality::analyze(genericUnused, Enums::GameVersion::FRLG);
    assert(hasText(bankUnused, "Invalid language id (6)"));

    return 0;
}
