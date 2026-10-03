#include "Legality/Legality.h"
#include "Names/ItemNames.h"
#include "Names/SpeciesNames.h"
#include "Pokemon/Pokemon3FRLG.h"

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <span>
#include <string>

// Legality.cpp uses the historical Trainer namespace name wrappers. Keep this focused host
// regression independent of the full Trainer implementation.
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

Pokemon::Pokemon3FRLG bulbasaurWithSwordsDance() {
    std::array<uint8_t, 80> raw{};
    Pokemon::Pokemon3FRLG p(std::span<const std::byte>(
        reinterpret_cast<const std::byte*>(raw.data()), raw.size()));
    p.setPID(0x12345678u);
    p.setTID16(12345);
    p.setSID16(54321);
    p.setLanguage(2);
    p.setSpecies(1);       // Bulbasaur
    p.setOTName(u"RED");
    p.setNickname(u"BULBASAUR");
    p.setOriginGame(2);    // Ruby
    p.setBall(4);
    p.setMetLevel(5);
    p.setLevel(10);
    p.setMove(0, 14);      // Swords Dance
    p.setMovePP(0, 30);
    return p;
}

Pokemon::Pokemon3FRLG xdTeddiursaWithStoredFatefulFlag() {
    std::array<uint8_t, 80> raw{};
    Pokemon::Pokemon3FRLG p(std::span<const std::byte>(
        reinterpret_cast<const std::byte*>(raw.data()), raw.size()));

    // Deterministic standard CXD vector from origin 0x12345678.
    p.setPID(0xE0A15B96u);
    p.setTID16(1);
    p.setSID16(2);
    p.setLanguage(2);
    p.setSpecies(216);      // Teddiursa: pinned XD shadow index 1
    p.setOTName(u"MICHAEL");
    p.setNickname(u"TEDDIURSA");
    p.setOriginGame(15);    // CXD
    p.setBall(4);           // fixed Poke Ball for this pinned XD row
    p.setMetLevel(11);
    p.setMetLocation(143);
    p.setLevel(11);
    p.setIV(0, 9);
    p.setIV(1, 31);
    p.setIV(2, 12);
    p.setIV(3, 31);
    p.setIV(4, 25);
    p.setIV(5, 22);

    assert(!p.isFatefulEncounter());
    p.wr32(0x4C, p.rd32(0x4C) | 0x80000000u);
    p.refreshChecksum();
    assert(p.isFatefulEncounter());
    return p;
}
}

int main() {
    auto p = bulbasaurWithSwordsDance();

    // Generated Gen III tables: Swords Dance is not direct for Bulbasaur in Ruby/Sapphire,
    // but it is direct in FireRed/LeafGreen. The old generic path treated every PK3 as FRLG.
    const auto ruby = Legality::analyze(p, Enums::GameVersion::FRLG, "ruby_gba");
    assert(hasText(ruby, "not native to this exact Gen III game"));
    assert(ruby.coverage.sourceGame == Legality::CoverageLevel::Complete);
    assert(ruby.coverage.encounter == Legality::CoverageLevel::Partial);
    assert(ruby.verdict() == Legality::Verdict::Incomplete);

    const auto fireRed = Legality::analyze(p, Enums::GameVersion::FRLG, "firered_gba");
    assert(!hasText(fireRed, "not native to this exact Gen III game"));

    // A later-generation move id stored in a PK3 must fail the exact-format ceiling even if
    // the global move-name table knows that id.
    p.setMove(1, 467);
    const auto impossible = Legality::analyze(p, Enums::GameVersion::FRLG, "ruby_gba");
    assert(hasText(impossible, "cannot exist in a Generation 3 save"));
    assert(impossible.hasInvalid());
    assert(impossible.verdict() == Legality::Verdict::Invalid);

    // No exact container context is intentional for Bank records; retain generic-format behavior.
    const auto bank = Legality::analyze(p, Enums::GameVersion::FRLG);
    assert(!hasText(bank, "not native to this exact Gen III game"));

    // PK3 persists the Gen III fateful/obedience flag in the ribbons word. XD shadow
    // transfers set it; the legality path must read the stored bit instead of the base
    // Pokemon default false value.
    auto teddiursa = xdTeddiursaWithStoredFatefulFlag();
    assert(teddiursa.checksumValid());
    const auto xd = Legality::analyze(
        teddiursa, Enums::GameVersion::FRLG, "firered_gba");
    assert(hasText(
        xd,
        "recursive Pokemon XD shadow-team/anti-shiny history match pinned source data"));

    std::cout << "Gen III exact-source legality context: PASS\n";
}
