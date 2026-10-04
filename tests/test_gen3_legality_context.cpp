#include "Legality/Legality.h"
#include "Legality/Gen3XdPokeSpotEvidence.h"
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

Pokemon::Pokemon3FRLG blankPk3() {
    std::array<uint8_t, 80> raw{};
    return Pokemon::Pokemon3FRLG(std::span<const std::byte>(
        reinterpret_cast<const std::byte*>(raw.data()), raw.size()));
}

void setStoredFateful(Pokemon::Pokemon3FRLG& p) {
    p.wr32(0x4C, p.rd32(0x4C) | 0x80000000u);
    p.refreshChecksum();
    assert(p.isFatefulEncounter());
}

Pokemon::Pokemon3FRLG bulbasaurWithSwordsDance() {
    auto p = blankPk3();
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
    auto p = blankPk3();

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
    setStoredFateful(p);
    return p;
}

Pokemon::Pokemon3FRLG xdAntiTeddiursa() {
    auto p = xdTeddiursaWithStoredFatefulFlag();
    p.setPID(0xB7951831u);
    p.setTID16(12345);
    p.setSID16(35598);
    p.setIV(0, 9);
    p.setIV(1, 31);
    p.setIV(2, 12);
    p.setIV(3, 31);
    p.setIV(4, 25);
    p.setIV(5, 22);
    p.refreshChecksum();
    return p;
}

Pokemon::Pokemon3FRLG coloMakuhita() {
    auto p = blankPk3();
    p.setPID(0xC252FEBAu);
    p.setTID16(1);
    p.setSID16(2);
    p.setLanguage(2);
    p.setSpecies(296);
    p.setOTName(u"WES");
    p.setNickname(u"MAKUHITA");
    p.setOriginGame(15);
    p.setBall(4);
    p.setMetLevel(30);
    p.setMetLocation(5);
    p.setLevel(30);
    p.setIV(0, 15);
    p.setIV(1, 9);
    p.setIV(2, 17);
    p.setIV(3, 16);
    p.setIV(4, 24);
    p.setIV(5, 22);
    p.refreshChecksum();
    return p;
}

Pokemon::Pokemon3FRLG coloUmbreonStarter() {
    auto p = blankPk3();
    p.setPID(0xC7047D49u);
    p.setTID16(46057);
    p.setSID16(23359);
    p.setLanguage(2);
    p.setSpecies(197);
    p.setOTName(u"WES");
    p.setNickname(u"UMBREON");
    p.setOriginGame(15);
    p.setBall(4);
    p.setMetLevel(26);
    p.setMetLocation(254);
    p.setLevel(26);
    p.setIV(0, 22);
    p.setIV(1, 28);
    p.setIV(2, 22);
    p.setIV(3, 21);
    p.setIV(4, 28);
    p.setIV(5, 13);
    p.refreshChecksum();
    assert(p.gender() == 0);
    return p;
}

Pokemon::Pokemon3FRLG coloEReaderTogepi() {
    auto p = blankPk3();
    p.setPID(0x6BD12F1Du);
    p.setTID16(1);
    p.setSID16(2);
    p.setLanguage(1);
    p.setSpecies(175);
    p.setOriginGame(15);
    p.setBall(4);
    p.setMetLevel(20);
    p.setMetLocation(128);
    p.setLevel(20);
    // e-Reader shadow IVs are all fixed zero.
    for (int i = 0; i < 6; ++i) p.setIV(i, 0);
    p.refreshChecksum();
    return p;
}

Pokemon::Pokemon3FRLG xdPokeSpotZubat() {
    auto p = blankPk3();
    // Deterministic vector derived directly from pinned MethodPokeSpot/XDRNG:
    // PID activation and IV/level animation histories are independently valid.
    p.setPID(0x002D7220u);
    p.setTID16(1);
    p.setSID16(2);
    p.setLanguage(2);
    p.setSpecies(41);       // Cave Poke Spot slot 0: Zubat
    p.setOTName(u"MICHAEL");
    p.setNickname(u"ZUBAT");
    p.setOriginGame(15);
    p.setBall(4);
    p.setMetLevel(13);
    p.setMetLocation(92);   // Cave Poke Spot
    p.setLevel(13);
    p.setIV(0, 13);
    p.setIV(1, 1);
    p.setIV(2, 0);
    p.setIV(3, 0);
    p.setIV(4, 17);
    p.setIV(5, 28);
    setStoredFateful(p);
    return p;
}

Pokemon::Pokemon3FRLG method1CxdFemaleCollision() {
    auto p = blankPk3();
    // Pinned PKHeX Method 1 vector. The CXD OT-gender invariant must still run
    // before the handheld PID method causes addGen3PidEvidence to return early.
    p.setPID(0xE97E0000u);
    p.setTID16(1);
    p.setSID16(2);
    p.setLanguage(2);
    p.setSpecies(1);
    p.setOriginGame(15);
    p.setMetLevel(5);
    p.setLevel(5);
    p.setIV(0, 17);
    p.setIV(1, 19);
    p.setIV(2, 20);
    p.setIV(3, 16);
    p.setIV(4, 13);
    p.setIV(5, 12);
    p.refreshChecksum();
    return p;
}

void forceFemaleOt(Pokemon::Pokemon3FRLG& p) {
    p.wr16(0x46, static_cast<uint16_t>(p.origins() | 0x8000u));
    p.refreshChecksum();
    assert(p.otGender() == 1);
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
    assert(hasText(xd, "Pokemon XD shadow identity and recursive team/anti-shiny history match pinned source data"));
    assert(hasText(xd, "Pokemon XD XDRNG class required by the pinned shadow identity"));

    // XD target-PID anti-shiny rerolls are family-aware: the same IV origin is
    // accepted only after persistent fields establish an XD shadow identity.
    auto antiTeddiursa = xdAntiTeddiursa();
    const auto xdAnti = Legality::analyze(
        antiTeddiursa, Enums::GameVersion::FRLG, "firered_gba");
    assert(hasText(xdAnti, "CXDAnti target-PID reroll class required by the pinned shadow identity"));
    assert(hasText(xdAnti, "Pokemon XD shadow identity and recursive team/anti-shiny history match pinned source data"));

    // Normal Colosseum uses standard CXD plus its own prior-team rules; it must no
    // longer be routed through the XD anti-shiny/team-lock policy.
    auto makuhita = coloMakuhita();
    const auto colo = Legality::analyze(
        makuhita, Enums::GameVersion::FRLG, "firered_gba");
    assert(hasText(colo, "standard Pokemon Colosseum XDRNG class required by the pinned shadow identity"));
    assert(hasText(colo, "Pokemon Colosseum shadow identity and recursive prior-team history match pinned source data"));
    assert(!hasText(colo, "Pokemon XD shadow identity"));

    // Fixed GameCube starters use a source-specific generation sequence rather than
    // the ordinary shadow correlation path.
    auto umbreon = coloUmbreonStarter();
    const auto fixed = Legality::analyze(
        umbreon, Enums::GameVersion::FRLG, "firered_gba");
    assert(hasText(fixed, "source-specific GameCube starter/gift/trade RNG policy"));
    assert(hasText(fixed, "pinned direct Pokemon Colosseum/XD starter, gift or in-game trade template"));

    // Japanese Colosseum e-Reader shadows are fixed-zero-IV records with their own
    // PID recovery + four-lock history path.
    auto togepi = coloEReaderTogepi();
    const auto eReader = Legality::analyze(
        togepi, Enums::GameVersion::FRLG, "firered_gba");
    assert(hasText(eReader, "fixed-zero-IV history match the Japanese Pokemon Colosseum e-Reader shadow generation path"));
    assert(hasText(eReader, "e-Reader shadow identity and recursive prior-team history match pinned source data"));

    // Pinned MethodPokeSpot evidence: PID activation and IV/level animation are
    // independent histories and must both be proven for the same encounter slot.
    namespace Spot = Legality::Gen3XdPokeSpotEvidence;
    constexpr Spot::Candidate zubatCandidate{
        41, 15, 2, 0, 13, 92, false, true, true,
        0x002D7220u, {13, 1, 0, 0, 17, 28}
    };
    constexpr auto zubatEvidence = Spot::analyze(zubatCandidate);
    static_assert(zubatEvidence.identityMatched);
    static_assert(zubatEvidence.pidMatched);
    static_assert(zubatEvidence.ivMatched);
    static_assert(zubatEvidence.slot == 0);
    static_assert(zubatEvidence.generatedLevel == 13);
    static_assert(zubatEvidence.setup == Spot::Setup::Munchlax);
    static_assert(zubatEvidence.pidOriginSeed == 0xEA585929u);
    static_assert(zubatEvidence.ivOriginSeed == 0x234CE543u);

    auto wrongLevel = zubatCandidate;
    wrongLevel.metLevel = 14;
    const auto wrongLevelEvidence = Spot::analyze(wrongLevel);
    assert(wrongLevelEvidence.identityMatched);
    assert(wrongLevelEvidence.pidMatched);
    assert(!wrongLevelEvidence.ivMatched);

    auto wrongIvs = zubatCandidate;
    wrongIvs.ivs[0] = 14;
    const auto wrongIvEvidence = Spot::analyze(wrongIvs);
    assert(wrongIvEvidence.identityMatched);
    assert(!wrongIvEvidence.ivMatched);

    auto wrongSlot = zubatCandidate;
    wrongSlot.species = 304; // Cave slot 1 Aron; PID activation belongs to slot 0.
    const auto wrongSlotEvidence = Spot::analyze(wrongSlot);
    assert(wrongSlotEvidence.identityMatched);
    assert(!wrongSlotEvidence.pidMatched);
    assert(wrongSlotEvidence.ivMatched);

    auto spotZubat = xdPokeSpotZubat();
    const auto spot = Legality::analyze(
        spotZubat, Enums::GameVersion::FRLG, "firered_gba");
    assert(hasText(spot, "PID activation and IV/level animation histories match the Pokemon XD Poke Spot RNG path"));
    assert(hasText(spot, "Species/location/level match a pinned Pokemon XD Poke Spot slot"));

    // Pinned PKHeX CXDVerifier invariant: GameCube-origin PK3 cannot have a female OT.
    auto badOt = coloMakuhita();
    forceFemaleOt(badOt);
    const auto femaleOt = Legality::analyze(
        badOt, Enums::GameVersion::FRLG, "firered_gba");
    assert(femaleOt.hasInvalid());
    assert(hasText(femaleOt, "Colosseum/XD-origin PK3 cannot have a female OT gender"));

    // The same invariant must survive an early handheld Method 1 correlation match.
    // This is the regression for the former ordering bug.
    auto collision = method1CxdFemaleCollision();
    forceFemaleOt(collision);
    const auto collisionReport = Legality::analyze(
        collision, Enums::GameVersion::FRLG, "firered_gba");
    assert(collisionReport.hasInvalid());
    assert(hasText(collisionReport, "Colosseum/XD-origin PK3 cannot have a female OT gender"));
    assert(hasText(collisionReport, "PID/IV spread matches Gen III Method 1"));

    std::cout << "Gen III exact-source + central GameCube/Poke Spot legality routing: PASS\n";
}
