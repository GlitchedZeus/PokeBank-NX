#include "Legality/Gen3CxdPidIvCorrelation.h"
#include "Legality/Gen3ColoEReaderShadowEvidence.h"
#include "Legality/Gen3GameCubeFixedEncounterEvidence.h"
#include "Legality/Gen3GameCubeStarterCorrelation.h"

#include <cassert>
#include <iostream>

int main() {
    using Legality::Gen3CxdPidIv::Variant;
    using Legality::Gen3CxdPidIv::analyze;
    using Legality::Gen3CxdPidIv::analyzeWithTrainer;
    namespace EReader = Legality::Gen3ColoEReaderShadowEvidence;
    namespace Fixed = Legality::Gen3GameCubeFixedEncounterEvidence;
    namespace Starter = Legality::Gen3GameCubeStarterCorrelation;

    const auto cxd = analyze(0x0985A297u, {6, 1, 0, 7, 17, 7});
    assert(cxd.matched);
    assert(cxd.variant == Variant::Standard);
    assert(!cxd.antiShiny());
    assert(!cxd.searchLimited);

    uint32_t seed = cxd.originSeed;
    const auto next = [](uint32_t s) constexpr {
        return s * 0x000343FDu + 0x00269EC3u;
    };
    seed = next(seed);
    const uint32_t iv1 = (seed >> 16) & 0x7FFFu;
    seed = next(seed);
    const uint32_t iv2 = (seed >> 16) & 0x7FFFu;
    seed = next(seed); // ability
    seed = next(seed);
    const uint32_t pidHigh = seed >> 16;
    seed = next(seed);
    const uint32_t pidLow = seed >> 16;

    assert(iv1 == (6u | (1u << 5) | (0u << 10)));
    assert(iv2 == (7u | (17u << 5) | (7u << 10)));
    assert(((pidHigh << 16) | pidLow) == 0x0985A297u);
    assert(!analyze(0x0985A297u, {6, 1, 0, 7, 18, 7}).matched);

    // Deterministic CXDAnti fixture derived directly from the pinned XDRNG
    // sequence. Origin 0x12345678 yields IVs 9/31/12/31/25/22. For player
    // TID/SID 12345/35598, the first target PID E0A15B96 is shiny and is
    // therefore skipped; the recorded non-shiny reroll is B7951831.
    constexpr uint16_t antiTid = 12345;
    constexpr uint16_t antiSid = 35598;
    const auto anti = analyzeWithTrainer(
        0xB7951831u, {9, 31, 12, 31, 25, 22}, antiTid, antiSid);
    assert(anti.matched);
    assert(anti.variant == Variant::AntiShiny);
    assert(anti.antiShiny());
    assert(!anti.searchLimited);
    assert(anti.originSeed == 0x12345678u);

    assert(!analyze(0xB7951831u, {9, 31, 12, 31, 25, 22}).matched);

    seed = anti.originSeed;
    seed = next(seed);
    const uint32_t antiIv1 = (seed >> 16) & 0x7FFFu;
    seed = next(seed);
    const uint32_t antiIv2 = (seed >> 16) & 0x7FFFu;
    seed = next(seed); // ability
    seed = next(seed);
    const uint32_t firstPidHigh = seed >> 16;
    seed = next(seed);
    const uint32_t firstPidLow = seed >> 16;
    const uint32_t firstPid = (firstPidHigh << 16) | firstPidLow;
    seed = next(seed);
    const uint32_t rerollPidHigh = seed >> 16;
    seed = next(seed);
    const uint32_t rerollPidLow = seed >> 16;
    const uint32_t rerollPid = (rerollPidHigh << 16) | rerollPidLow;
    const uint32_t trainerShinyValue =
        static_cast<uint32_t>(antiTid ^ antiSid) >> 3;
    const auto shinyValue = [](uint32_t pid) constexpr {
        return ((pid >> 16) ^ (pid & 0xFFFFu)) >> 3;
    };

    assert(antiIv1 == (9u | (31u << 5) | (12u << 10)));
    assert(antiIv2 == (31u | (25u << 5) | (22u << 10)));
    assert(firstPid == 0xE0A15B96u);
    assert(rerollPid == 0xB7951831u);
    assert(shinyValue(firstPid) == trainerShinyValue);
    assert(shinyValue(rerollPid) != trainerShinyValue);

    assert(!analyzeWithTrainer(
        0xB7951831u, {9, 31, 12, 31, 25, 22}, antiTid, 35590).matched);
    assert(!analyzeWithTrainer(
        0xB7951831u, {9, 31, 12, 31, 25, 21}, antiTid, antiSid).matched);

    // Colosseum starters have their own correlation. Origin 0x12345678 produces
    // TID/SID 46057/23359, then Umbreon first and Espeon second.
    constexpr uint16_t starterTid = 46057;
    constexpr uint16_t starterSid = 23359;
    const auto umbreon = Starter::analyzeColosseum(
        197, 0xC7047D49u, {22, 28, 22, 21, 28, 13},
        starterTid, starterSid);
    assert(umbreon.matched);
    assert(!umbreon.searchLimited);
    assert(umbreon.variant == Starter::Variant::ColosseumUmbreon);
    assert(umbreon.originSeed == 0x12345678u);

    const auto espeon = Starter::analyzeColosseum(
        196, 0x0D09CC6Fu, {1, 11, 11, 3, 4, 2},
        starterTid, starterSid);
    assert(espeon.matched);
    assert(!espeon.searchLimited);
    assert(espeon.variant == Starter::Variant::ColosseumEspeon);
    assert(espeon.originSeed == 0x12345678u);

    const auto xdEevee = Starter::analyzeXdEevee(
        133, 0xC7047D49u, {22, 28, 22, 21, 28, 13},
        starterTid, starterSid);
    assert(xdEevee.matched);
    assert(!xdEevee.searchLimited);
    assert(xdEevee.variant == Starter::Variant::XdEevee);
    assert(xdEevee.originSeed == 0x12345678u);

    assert(!Starter::analyzeColosseum(
        197, 0xC7047D49u, {22, 28, 22, 21, 28, 13},
        starterTid, static_cast<uint16_t>(starterSid + 1)).matched);
    assert(!Starter::analyzeColosseum(
        197, 0xC7047D49u, {22, 28, 22, 21, 28, 12},
        starterTid, starterSid).matched);
    assert(!Starter::analyzeColosseum(
        133, 0xC7047D49u, {22, 28, 22, 21, 28, 13},
        starterTid, starterSid).matched);
    assert(!Starter::analyzeXdEevee(
        197, 0xC7047D49u, {22, 28, 22, 21, 28, 13},
        starterTid, starterSid).matched);

    const auto boundedEspeon = Starter::analyzeColosseum(
        196, 0x0D09CC6Fu, {1, 11, 11, 3, 4, 2},
        starterTid, starterSid, 0);
    assert(!boundedEspeon.matched);
    assert(boundedEspeon.searchLimited);

    // Fixed GameCube direct-template evidence. These 14 source rows span
    // Colosseum starters/Plusle, XD gifts/trades, the Japanese Bonus Disc
    // Pikachu/Celebi, and Mt. Battle Ho-Oh.
    static_assert(Fixed::kTemplateCount == 14);

    Fixed::Candidate fixedUmbreon{};
    fixedUmbreon.species = 197;
    fixedUmbreon.originGame = 15;
    fixedUmbreon.language = 2;
    fixedUmbreon.otGender = 0;
    fixedUmbreon.pokemonGender = 0;
    fixedUmbreon.tid = starterTid;
    fixedUmbreon.sid = starterSid;
    fixedUmbreon.metLevel = 26;
    fixedUmbreon.metLocation = 254;
    fixedUmbreon.ball = 4;
    fixedUmbreon.pid = 0xC7047D49u;
    fixedUmbreon.ivs = {22, 28, 22, 21, 28, 13};
    const auto fixedUmbreonEvidence = Fixed::analyze(fixedUmbreon);
    assert(fixedUmbreonEvidence.identityMatched);
    assert(fixedUmbreonEvidence.rngMatched);
    assert(fixedUmbreonEvidence.kind == Fixed::Kind::ColosseumUmbreon);
    assert(fixedUmbreonEvidence.starterVariant == Starter::Variant::ColosseumUmbreon);

    Fixed::Candidate fixedEspeon = fixedUmbreon;
    fixedEspeon.species = 196;
    fixedEspeon.metLevel = 25;
    fixedEspeon.pid = 0x0D09CC6Fu;
    fixedEspeon.ivs = {1, 11, 11, 3, 4, 2};
    const auto fixedEspeonEvidence = Fixed::analyze(fixedEspeon);
    assert(fixedEspeonEvidence.identityMatched);
    assert(fixedEspeonEvidence.rngMatched);
    assert(fixedEspeonEvidence.kind == Fixed::Kind::ColosseumEspeon);

    const std::array<uint8_t, 6> standardIvs{6, 1, 0, 7, 17, 7};
    Fixed::Candidate plusle{};
    plusle.species = 311;
    plusle.originGame = 15;
    plusle.language = 2;
    plusle.otGender = 0;
    plusle.tid = 37149;
    plusle.sid = 0;
    plusle.metLevel = 13;
    plusle.metLocation = 254;
    plusle.ball = 4;
    plusle.pid = 0x0985A297u;
    plusle.ivs = standardIvs;
    plusle.otName = u"DUKING";
    const auto plusleEvidence = Fixed::analyze(plusle);
    assert(plusleEvidence.identityMatched);
    assert(plusleEvidence.rngMatched);
    assert(plusleEvidence.kind == Fixed::Kind::ColosseumPlusle);
    assert(plusleEvidence.cxdVariant == Variant::Standard);

    // Fixed-trainer Plusle can also use the source anti-shiny reroll class.
    // Origin 0x00000D80 yields shiny target 00419158 for TID/SID 37149/0,
    // then records non-shiny reroll 40EF42C4 with the same IV origin.
    auto antiPlusle = plusle;
    antiPlusle.pid = 0x40EF42C4u;
    antiPlusle.ivs = {28, 1, 11, 4, 13, 27};
    const auto antiPlusleEvidence = Fixed::analyze(antiPlusle);
    assert(antiPlusleEvidence.identityMatched);
    assert(antiPlusleEvidence.rngMatched);
    assert(antiPlusleEvidence.cxdVariant == Variant::AntiShiny);

    Fixed::Candidate fixedXdEevee{};
    fixedXdEevee.species = 133;
    fixedXdEevee.originGame = 15;
    fixedXdEevee.language = 2;
    fixedXdEevee.otGender = 0;
    fixedXdEevee.tid = starterTid;
    fixedXdEevee.sid = starterSid;
    fixedXdEevee.metLevel = 10;
    fixedXdEevee.metLocation = 0;
    fixedXdEevee.ball = 4;
    fixedXdEevee.fateful = true;
    fixedXdEevee.pid = 0xC7047D49u;
    fixedXdEevee.ivs = {22, 28, 22, 21, 28, 13};
    const auto fixedXdEeveeEvidence = Fixed::analyze(fixedXdEevee);
    assert(fixedXdEeveeEvidence.identityMatched);
    assert(fixedXdEeveeEvidence.rngMatched);
    assert(fixedXdEeveeEvidence.kind == Fixed::Kind::XdEevee);
    assert(fixedXdEeveeEvidence.starterVariant == Starter::Variant::XdEevee);

    // Johto XD gifts are fateful player-owned encounters and permit CXDAnti.
    Fixed::Candidate chikorita{};
    chikorita.species = 152;
    chikorita.originGame = 15;
    chikorita.language = 2;
    chikorita.otGender = 0;
    chikorita.tid = antiTid;
    chikorita.sid = antiSid;
    chikorita.metLevel = 5;
    chikorita.metLocation = 16;
    chikorita.ball = 4;
    chikorita.fateful = true;
    chikorita.pid = 0xB7951831u;
    chikorita.ivs = {9, 31, 12, 31, 25, 22};
    const auto chikoritaEvidence = Fixed::analyze(chikorita);
    assert(chikoritaEvidence.identityMatched);
    assert(chikoritaEvidence.rngMatched);
    assert(chikoritaEvidence.kind == Fixed::Kind::XdChikorita);
    assert(chikoritaEvidence.cxdVariant == Variant::AntiShiny);

    auto cyndaquil = chikorita;
    cyndaquil.species = 155;
    cyndaquil.pid = 0x0985A297u;
    cyndaquil.ivs = standardIvs;
    assert(Fixed::analyze(cyndaquil).kind == Fixed::Kind::XdCyndaquil);
    assert(Fixed::analyze(cyndaquil).rngMatched);

    auto totodile = cyndaquil;
    totodile.species = 158;
    assert(Fixed::analyze(totodile).kind == Fixed::Kind::XdTotodile);
    assert(Fixed::analyze(totodile).rngMatched);

    // XD trades have fixed TID/OT identity but player-derived SID. Unlike the
    // gifts, their pinned template only accepts standard CXD correlation.
    Fixed::Candidate elekid{};
    elekid.species = 239;
    elekid.originGame = 15;
    elekid.language = 2;
    elekid.otGender = 0;
    elekid.tid = 41400;
    elekid.sid = 1234;
    elekid.metLevel = 20;
    elekid.metLocation = 164;
    elekid.ball = 4;
    elekid.fateful = true;
    elekid.pid = 0x0985A297u;
    elekid.ivs = standardIvs;
    elekid.otName = u"HORDEL";
    elekid.nickname = u"ZAPRONG";
    const auto elekidEvidence = Fixed::analyze(elekid);
    assert(elekidEvidence.identityMatched);
    assert(elekidEvidence.rngMatched);
    assert(elekidEvidence.kind == Fixed::Kind::XdElekidTrade);
    assert(elekidEvidence.cxdVariant == Variant::Standard);

    auto meditite = elekid;
    meditite.species = 307;
    meditite.tid = 37149;
    meditite.metLocation = 116;
    meditite.otName = u"DUKING";
    meditite.nickname = {};
    assert(Fixed::analyze(meditite).kind == Fixed::Kind::XdMedititeTrade);
    assert(Fixed::analyze(meditite).rngMatched);

    auto shuckle = meditite;
    shuckle.species = 213;
    assert(Fixed::analyze(shuckle).kind == Fixed::Kind::XdShuckleTrade);
    assert(Fixed::analyze(shuckle).rngMatched);

    auto larvitar = meditite;
    larvitar.species = 246;
    assert(Fixed::analyze(larvitar).kind == Fixed::Kind::XdLarvitarTrade);
    assert(Fixed::analyze(larvitar).rngMatched);

    // Anti-shiny-like PID/IV evidence cannot be upgraded to an XD trade proof.
    auto antiElekid = elekid;
    antiElekid.pid = 0x713BBC92u;
    antiElekid.ivs = {20, 15, 1, 4, 14, 15};
    const auto antiElekidEvidence = Fixed::analyze(antiElekid);
    assert(antiElekidEvidence.identityMatched);
    assert(!antiElekidEvidence.rngMatched);

    Fixed::Candidate bonusPikachu{};
    bonusPikachu.species = 25;
    bonusPikachu.originGame = 2;
    bonusPikachu.language = 1;
    bonusPikachu.otGender = 0;
    bonusPikachu.tid = 31121;
    bonusPikachu.sid = 0;
    bonusPikachu.metLevel = 10;
    bonusPikachu.metLocation = 255;
    bonusPikachu.ball = 4;
    bonusPikachu.pid = 0x0985A297u;
    bonusPikachu.ivs = standardIvs;
    bonusPikachu.otName = u"コロシアム";
    const auto bonusPikachuEvidence = Fixed::analyze(bonusPikachu);
    assert(bonusPikachuEvidence.identityMatched);
    assert(bonusPikachuEvidence.rngMatched);
    assert(bonusPikachuEvidence.kind == Fixed::Kind::ColosseumBonusPikachu);

    auto agetoCelebi = bonusPikachu;
    agetoCelebi.species = 251;
    agetoCelebi.otGender = 1;
    agetoCelebi.otName = u"アゲト";
    const auto agetoEvidence = Fixed::analyze(agetoCelebi);
    assert(agetoEvidence.identityMatched);
    assert(agetoEvidence.rngMatched);
    assert(agetoEvidence.kind == Fixed::Kind::ColosseumAgetoCelebi);

    Fixed::Candidate mtBattleHoOh{};
    mtBattleHoOh.species = 250;
    mtBattleHoOh.originGame = 1;
    mtBattleHoOh.language = 2;
    mtBattleHoOh.otGender = 0;
    mtBattleHoOh.tid = 10048;
    mtBattleHoOh.sid = 0;
    mtBattleHoOh.metLevel = 70;
    mtBattleHoOh.metLocation = 255;
    mtBattleHoOh.ball = 4;
    mtBattleHoOh.pid = 0x0985A297u;
    mtBattleHoOh.ivs = standardIvs;
    mtBattleHoOh.otName = u"MATTLE";
    const auto hoOhEvidence = Fixed::analyze(mtBattleHoOh);
    assert(hoOhEvidence.identityMatched);
    assert(hoOhEvidence.rngMatched);
    assert(hoOhEvidence.kind == Fixed::Kind::ColosseumMtBattleHoOh);

    auto wrongBonusLanguage = bonusPikachu;
    wrongBonusLanguage.language = 2;
    wrongBonusLanguage.otName = u"COLOS";
    assert(!Fixed::analyze(wrongBonusLanguage).identityMatched);

    auto wrongCelebiGender = agetoCelebi;
    wrongCelebiGender.otGender = 0;
    assert(!Fixed::analyze(wrongCelebiGender).identityMatched);

    auto wrongElekidNickname = elekid;
    wrongElekidNickname.nickname = u"ELEKID";
    assert(!Fixed::analyze(wrongElekidNickname).identityMatched);

    auto wrongXdFateful = chikorita;
    wrongXdFateful.fateful = false;
    assert(!Fixed::analyze(wrongXdFateful).identityMatched);

    auto wrongBall = plusle;
    wrongBall.ball = 3;
    assert(!Fixed::analyze(wrongBall).identityMatched);

    auto wrongFixedSid = plusle;
    wrongFixedSid.sid = 1;
    assert(!Fixed::analyze(wrongFixedSid).identityMatched);

    // Japanese Colosseum e-Reader shadows use a separate path: all IVs are 0,
    // the stored PID is reversed with XDRNG.GetSeeds, then Next4(seed) is fed
    // into the four-lock Colosseum team-history validator.
    EReader::Candidate togepi{};
    togepi.species = 175;
    togepi.originGame = 15;
    togepi.language = 1;
    togepi.otGender = 0;
    togepi.metLevel = 20;
    togepi.metLocation = 128;
    togepi.pid = 0x6BD12F1Du;
    const auto togepiEvidence = EReader::analyze(togepi);
    assert(togepiEvidence.identityMatched);
    assert(togepiEvidence.matched);
    assert(!togepiEvidence.searchLimited);
    assert(togepiEvidence.encounter == 0);
    assert(togepiEvidence.originSeed == 0x0000001Fu);

    EReader::Candidate mareep{};
    mareep.species = 179;
    mareep.originGame = 15;
    mareep.language = 1;
    mareep.otGender = 0;
    mareep.metLevel = 37;
    mareep.metLocation = 128;
    mareep.pid = 0xF8F56E5Du;
    const auto mareepBounded = EReader::analyze(mareep, 16384);
    assert(mareepBounded.identityMatched);
    assert(!mareepBounded.matched);
    assert(mareepBounded.searchLimited);
    const auto mareepEvidence = EReader::analyze(mareep);
    assert(mareepEvidence.matched);
    assert(!mareepEvidence.searchLimited);
    assert(mareepEvidence.encounter == 1);
    assert(mareepEvidence.originSeed == 0x0000001Du);

    EReader::Candidate scizor{};
    scizor.species = 212;
    scizor.originGame = 15;
    scizor.language = 1;
    scizor.otGender = 0;
    scizor.metLevel = 50;
    scizor.metLocation = 128;
    scizor.pid = 0x66F24BC0u;
    const auto scizorEvidence = EReader::analyze(scizor);
    assert(scizorEvidence.identityMatched);
    assert(scizorEvidence.matched);
    assert(!scizorEvidence.searchLimited);
    assert(scizorEvidence.encounter == 2);
    assert(scizorEvidence.originSeed == 0x00000016u);

    auto wrongLanguage = togepi;
    wrongLanguage.language = 2;
    assert(!EReader::analyze(wrongLanguage).identityMatched);
    auto wrongIvs = togepi;
    wrongIvs.ivs[0] = 1;
    assert(!EReader::analyze(wrongIvs).identityMatched);
    auto wrongOtGender = togepi;
    wrongOtGender.otGender = 1;
    assert(!EReader::analyze(wrongOtGender).identityMatched);
    auto wrongFateful = togepi;
    wrongFateful.fateful = true;
    assert(!EReader::analyze(wrongFateful).identityMatched);
    auto wrongLocation = togepi;
    wrongLocation.metLocation = 127;
    assert(!EReader::analyze(wrongLocation).identityMatched);

    std::cout << "Gen III GameCube standard + CXDAnti + starters + fixed encounters + Colosseum e-Reader evidence: PASS\n";
}
