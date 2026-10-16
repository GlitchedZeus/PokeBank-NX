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

// Japanese Negai Boshi Jirachi (pinned PKHeX EncountersWC3): Ruby origin,
// TID 30719 / SID 0, OT "ネガイボシ", PID/IVs from the BA-CD event RNG.
Pokemon::Pokemon3FRLG negaiBoshiJirachi() {
    auto p = blankPk3();
    p.setSpecies(385);
    p.setTID16(30719);
    p.setSID16(0);
    p.setLanguage(1);
    p.setOTName(u"ネガイボシ");
    p.setNickname(u"ジラーチ");
    p.setOriginGame(2);
    p.setMetLevel(5);
    p.setMetLocation(255);
    p.setBall(4);
    p.setOTGender(0);
    const uint8_t ivs[6] = {10, 12, 22, 0, 7, 29}; // HP ATK DEF SPE SPA SPD
    for (int i = 0; i < 6; ++i) p.setIV(i, ivs[i]);
    p.setPID(0xF86484EAu);
    p.setLevel(5);
    p.setMove(0, 273); // Wish
    p.setMovePP(0, 10);
    p.refreshChecksum();
    return p;
}

bool hasNegaiTemplate(const Legality::Report& report) {
    return hasText(report, "match the pinned Japanese Negai Boshi Jirachi distribution template");
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

Pokemon::Pokemon3FRLG safariPikachu() {
    auto p = blankPk3();
    p.setPID(0x12345678u);
    p.setTID16(12345);
    p.setSID16(54321);
    p.setLanguage(2);
    p.setSpecies(25);   // Pinned Ruby Safari Pikachu, slot8 / level25.
    p.setOriginGame(2); // Ruby origin persists after Gen III trading.
    p.setMetLocation(57);
    p.setMetLevel(25);
    p.setBall(5);       // Safari Ball.
    p.setLevel(25);
    p.setOTName(u"RED");
    p.setNickname(u"PIKACHU");
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
    // Negai Boshi: the analyzer's event candidate compares the stored OT name.
    // That name must outlive the comparison (it was once a view of a destroyed
    // temporary; the sanitizer build of this test catches a regression).
    {
        auto jirachi = negaiBoshiJirachi();
        assert(jirachi.otName() == u"ネガイボシ");
        const auto matched = Legality::analyze(jirachi, Enums::GameVersion::FRLG, "ruby_gba");
        assert(hasNegaiTemplate(matched));
        // A different OT name is not the distribution, by the same comparison.
        jirachi.setOTName(u"ネガイ");
        jirachi.refreshChecksum();
        const auto renamed = Legality::analyze(jirachi, Enums::GameVersion::FRLG, "ruby_gba");
        assert(!hasNegaiTemplate(renamed));
    }

    // Source-backed Gen III Safari Ball *positive-only* production reporting.
    // Pinned Ruby encounter_r.pkl: Pikachu #25 at Safari location57, level25.
    auto safari = safariPikachu();
    const auto native = Legality::analyze(
        safari, Enums::GameVersion::FRLG, "ruby_gba");
    assert(hasText(native,
        "Safari Ball has a compatible direct Generation III Safari Zone capture source"));
    // PK3 remains Ruby-origin after being traded into a FireRed save.
    const auto traded = Legality::analyze(
        safari, Enums::GameVersion::FRLG, "firered_gba");
    assert(hasText(traded,
        "Safari Ball has a compatible direct Generation III Safari Zone capture source"));
    // Evolution keeps the capture ball/met data; current Raichu has
    // compatible Pikachu Safari ancestry, not a new Raichu Safari slot.
    safari.setSpecies(26);
    safari.setLevel(30);
    safari.refreshChecksum();
    const auto evolved = Legality::analyze(
        safari, Enums::GameVersion::FRLG, "ruby_gba");
    assert(hasText(evolved,
        "Safari Ball has a compatible Generation III Safari Zone capture via a pre-evolution"));
    safari.setBall(4);
    const auto wrongBall = Legality::analyze(
        safari, Enums::GameVersion::FRLG, "ruby_gba");
    assert(!hasText(wrongBall,
        "Safari Ball has a compatible Generation III Safari Zone capture"));
    safari.setBall(5);
    safari.setMetLocation(136); // FRLG Safari, not Ruby Safari origin.
    const auto wrongLocation = Legality::analyze(
        safari, Enums::GameVersion::FRLG, "ruby_gba");
    assert(!hasText(wrongLocation,
        "Safari Ball has a compatible Generation III Safari Zone capture"));
    safari.setMetLocation(57);
    safari.setOriginGame(15); // GameCube source, not native Safari.
    const auto wrongOrigin = Legality::analyze(
        safari, Enums::GameVersion::FRLG, "ruby_gba");
    assert(!hasText(wrongOrigin,
        "Safari Ball has a compatible Generation III Safari Zone capture"));

    // F5 capability: a real native Gen III Pokemon cannot masquerade as
    // Gen II PK2 stat experience merely because a caller supplies a label.
    // RTTI is disabled on Switch; virtual capability, not a downcast.
    auto unrelated = bulbasaurWithSwordsDance();
    const Pokemon::Pokemon& genericNative = unrelated;
    std::array<uint16_t,5> unavailable{1,1,1,1,1};
    assert(genericNative.nativePk1ReadOnly()==nullptr);
    assert(genericNative.nativePk2ReadOnly()==nullptr);
    assert(!genericNative.gen2NativeStatExperience(unavailable));
    assert((unavailable == std::array<uint16_t,5>{}));
    auto p = bulbasaurWithSwordsDance();

    // Generated Gen III tables: Swords Dance is not direct for Bulbasaur in Ruby/Sapphire,
    // but it is direct in FireRed/LeafGreen. The old generic path treated every PK3 as FRLG.
    const auto ruby = Legality::analyze(p, Enums::GameVersion::FRLG, "ruby_gba");
    assert(hasText(ruby, "not native to this exact Gen III game"));
    assert(ruby.coverage.sourceGame == Legality::CoverageLevel::Complete);
    assert(ruby.coverage.encounter == Legality::CoverageLevel::Partial);
    // The Gen III analyzer (like PKHeX: Encounter EggFMetLevel_0) rejects this synthetic
    // record: a Ruby-origin Bulbasaur can only have hatched, and hatching sets met level 0.
    assert(ruby.verdict() == Legality::Verdict::Invalid);
    assert(hasText(ruby, "Hatched Pokemon must have met level 0"));

    const auto fireRed = Legality::analyze(p, Enums::GameVersion::FRLG, "firered_gba");
    assert(!hasText(fireRed, "not native to this exact Gen III game"));

    // L3 complete ORIGINAL Ruby/Sapphire/FRLG level-up data must reach
    // the actual Legality::analyze production report on native encrypted PK3.
    // Original-game byte is retained even if this Ruby Pokemon is currently
    // being inspected inside a different (FireRed) source save.
    auto rubyOffer=bulbasaurWithSwordsDance();
    rubyOffer.setMove(0,73); // Leech Seed: original Bulbasaur offer at lv7
    rubyOffer.refreshChecksum();
    const auto rubyOfferReport=Legality::analyze(
        rubyOffer, Enums::GameVersion::FRLG, "firered_gba");
    assert(hasText(rubyOfferReport,
        "Original ruby_gba native Gen III level-up source:"));
    assert(!hasText(rubyOfferReport,
        "Original firered_gba native Gen III level-up source:"));

    // Level opportunity is bounded by the native CURRENT level: no
    // synthetic positive when the source offer has not yet occurred.
    rubyOffer.setLevel(5);
    rubyOffer.refreshChecksum();
    const auto early=Legality::analyze(
        rubyOffer, Enums::GameVersion::FRLG, "ruby_gba");
    assert(!hasText(early,
        "Original ruby_gba native Gen III level-up source:"));

    // Same game/version-specific original table, different eligible species.
    auto frlgOffer=bulbasaurWithSwordsDance();
    frlgOffer.setSpecies(4); // Charmander, original FRLG lv13 Metal Claw
    frlgOffer.setOriginGame(4); // FireRed
    frlgOffer.setLevel(13);
    frlgOffer.setMove(0,232);
    frlgOffer.refreshChecksum();
    const auto fr=Legality::analyze(
        frlgOffer, Enums::GameVersion::FRLG, "ruby_gba");
    assert(hasText(fr,
        "Original firered_gba native Gen III level-up source:"));
    frlgOffer.setOriginGame(5); // LeafGreen, same original level opportunity
    frlgOffer.refreshChecksum();
    const auto lg=Legality::analyze(
        frlgOffer, Enums::GameVersion::FRLG, "leafgreen_gba");
    assert(hasText(lg,
        "Original leafgreen_gba native Gen III level-up source:"));

    // GameCube origin does not inherit any of the GBA original-game tables.
    rubyOffer.setOriginGame(15);
    rubyOffer.setLevel(10);
    rubyOffer.refreshChecksum();
    const auto cube=Legality::analyze(
        rubyOffer, Enums::GameVersion::FRLG, "ruby_gba");
    assert(!hasText(cube,
        "native Gen III level-up source:"));

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
