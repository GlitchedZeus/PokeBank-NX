#include "Legality/Gen3BacdPidIvCorrelation.h"
#include "Legality/Gen3WishmkrEventTemplate.h"
#include "Legality/Gen3BerryFixEventTemplate.h"
#include "Legality/Gen3NegaiBoshiEventTemplate.h"
#include "Legality/Gen3PokeParkEggEventTemplate.h"
#include "Legality/Gen3PcjpFifthEggEventTemplate.h"

#include <cassert>
#include <iostream>

int main() {
    using Legality::Gen3BacdPidIv::Variant;
    using Legality::Gen3BacdPidIv::analyze;
    using Legality::Gen3BacdPidIv::analyzeWithTrainer;

    // Canonical PKHeX unrestricted regular BACD event vector.
    const auto bacd = analyze(0x67DBFC33u, {12, 25, 27, 30, 2, 31});
    assert(bacd.matched());
    assert(bacd.variant == Variant::Regular);

    // Replay from the recovered origin: A/B generate PID, C/D generate IV halves.
    uint32_t seed = bacd.originSeed;
    const auto next = [](uint32_t s) constexpr {
        return s * 0x41C64E6Du + 0x00006073u;
    };
    seed = next(seed);
    const uint32_t a16 = seed >> 16;
    seed = next(seed);
    const uint32_t b16 = seed >> 16;
    seed = next(seed);
    const uint32_t iv1 = (seed >> 16) & 0x7FFFu;
    seed = next(seed);
    const uint32_t iv2 = (seed >> 16) & 0x7FFFu;

    assert(((a16 << 16) | b16) == 0x67DBFC33u);
    assert(iv1 == (12u | (25u << 5) | (27u << 10)));
    assert(iv2 == (30u | (2u << 5) | (31u << 10)));

    assert(!analyze(0x67DBFC33u, {12, 25, 27, 30, 3, 31}).matched());

    // Canonical PKHeX unrestricted regular-antishiny event vector.
    const auto anti = analyzeWithTrainer(
        0x67DBFC38u, {12, 25, 27, 30, 2, 31}, 1337, 40657);
    assert(anti.matched());
    assert(anti.variant == Variant::RegularAntiShiny);

    // Canonical PKHeX unrestricted force-antishiny event vector.
    const auto forced = analyzeWithTrainer(
        0xBD3DF676u, {0, 15, 5, 4, 21, 5}, 80, 0);
    assert(forced.matched());
    assert(forced.variant == Variant::ForceAntiShiny);

    assert(!analyzeWithTrainer(
        0xBD3DF676u, {0, 15, 5, 4, 21, 6}, 80, 0).matched());

    // Negai Boshi Jirachi uses the unrestricted BACD_U_AX forced-antishiny
    // method with fixed Japanese distribution fields. This vector is generated from
    // origin seed 0x12345678 using the same A/B/C/D LCRNG sequence.
    const auto negaiRng = analyzeWithTrainer(
        0xF86484EAu, {10, 12, 22, 0, 7, 29}, 30719, 0);
    assert(negaiRng.matched());
    assert(negaiRng.variant == Variant::ForceAntiShiny);
    assert(negaiRng.originSeed == 0x12345678u);

    namespace Negai = Legality::Gen3NegaiBoshiEvent;
    Negai::Candidate negai{
        385, 30719, 0, 2, 1, 0,
        5, 255, 4, false, false, false, u"ネガイボシ"
    };
    assert(Negai::matches(negai, negaiRng));

    auto negaiSapphire = negai;
    negaiSapphire.originGame = 1;
    negaiSapphire.otGender = 1; // recipient OT gender can be either value.
    assert(Negai::matches(negaiSapphire, negaiRng));

    auto negaiWrongLanguage = negai;
    negaiWrongLanguage.language = 2;
    assert(!Negai::matches(negaiWrongLanguage, negaiRng));

    auto negaiWrongOt = negai;
    negaiWrongOt.otName = u"WISHMKR";
    assert(!Negai::matches(negaiWrongOt, negaiRng));

    auto negaiWrongRng = negaiRng;
    negaiWrongRng.variant = Variant::RegularAntiShiny;
    assert(!Negai::matches(negai, negaiWrongRng));

    // The other pinned Negai Boshi Jirachi path is BACD_TA: the normal
    // BA-CD origin is exactly two RNG calls after a restricted 16-bit table seed.
    const auto negaiTableRng = analyzeWithTrainer(
        0x4340FFF1u, {25, 16, 16, 30, 13, 26}, 30719, 0);
    assert(negaiTableRng.matched());
    assert(negaiTableRng.variant == Variant::Regular);
    assert(Negai::table2OriginSeed(negaiTableRng) == 0x1234u);

    auto negaiTable = negai;
    negaiTable.originGame = 2;
    negaiTable.otGender = 0;
    assert(Negai::matchesRestrictedTable2(negaiTable, negaiTableRng));
    assert(Negai::matches(negaiTable, negaiTableRng));

    auto negaiTableSapphire = negaiTable;
    negaiTableSapphire.originGame = 1;
    assert(!Negai::matchesRestrictedTable2(
        negaiTableSapphire, negaiTableRng));

    auto negaiTableFemaleOt = negaiTable;
    negaiTableFemaleOt.otGender = 1;
    assert(!Negai::matchesRestrictedTable2(
        negaiTableFemaleOt, negaiTableRng));

    // Canonical PKHeX Berry Fix Zigzagoon: forced-shiny BA-CD_S, seed 0x20.
    const auto berryFix = analyzeWithTrainer(
        0x38CA4EA0u, {0, 20, 28, 11, 19, 0}, 30317, 0);
    assert(berryFix.matched());
    assert(berryFix.variant == Variant::ForceShiny);
    assert(berryFix.originSeed == 0x20u);
    assert(berryFix.restrictedSeed);
    assert(Legality::Gen3BerryFixEvent::validOriginSeed(berryFix.originSeed));
    static_assert(!Legality::Gen3BerryFixEvent::validOriginSeed(0u));
    static_assert(!Legality::Gen3BerryFixEvent::validOriginSeed(2u));
    static_assert(Legality::Gen3BerryFixEvent::validOriginSeed(3u));
    static_assert(Legality::Gen3BerryFixEvent::validOriginSeed(213u));
    static_assert(!Legality::Gen3BerryFixEvent::validOriginSeed(214u));

    namespace BerryFix = Legality::Gen3BerryFixEvent;
    assert(BerryFix::matchesTemplate(
        263, 30317, 0, 1, 2, 1, 5, 255, 4, false, false, u"RUBY"));
    assert(BerryFix::matchesTemplate(
        263, 30317, 0, 1, 2, 0, 5, 255, 4, false, false, u"SAPHIRE"));
    assert(BerryFix::matchesTemplate(
        263, 21121, 0, 1, 1, 1, 5, 255, 4, false, false, u"ルビー"));
    assert(BerryFix::matchesTemplate(
        263, 21121, 0, 1, 1, 0, 5, 255, 4, false, false, u"サファイア"));
    assert(!BerryFix::matchesTemplate(
        263, 30317, 0, 2, 2, 1, 5, 255, 4, false, false, u"RUBY"));
    assert(!BerryFix::matchesTemplate(
        263, 30317, 0, 1, 2, 0, 5, 255, 4, false, false, u"RUBY"));
    assert(!BerryFix::matchesTemplate(
        263, 30317, 0, 1, 1, 1, 5, 255, 4, false, false, u"RUBY"));

    // Restricted regular BACD example from PKHeX should expose the 16-bit seed class.
    const auto restricted = analyzeWithTrainer(
        0x0000E97Eu, {17, 19, 20, 16, 13, 12}, 0, 0);
    assert(restricted.matched());
    assert(restricted.variant == Variant::Regular);
    assert(restricted.restrictedSeed);

    // PokePark DS Download eggs are restricted regular BACD_R events.
    const auto pokeParkEggRng = analyzeWithTrainer(
        0x0000E97Eu, {17, 19, 20, 16, 13, 12}, 50318, 0);
    assert(pokeParkEggRng.matched());
    assert(pokeParkEggRng.variant == Variant::Regular);
    assert(pokeParkEggRng.restrictedSeed);

    namespace PokeParkEgg = Legality::Gen3PokeParkEggEvent;
    PokeParkEgg::Candidate pokeParkEgg{
        54, 50318, 0, 2, 0, 5, 255, 4, true, false, u"ポケパーク"
    };
    assert(PokeParkEgg::matches(pokeParkEgg, pokeParkEggRng));
    auto pokeParkHatched = pokeParkEgg;
    pokeParkHatched.isEgg = false;
    assert(!PokeParkEgg::matches(pokeParkHatched, pokeParkEggRng));
    auto pokeParkWrongSpecies = pokeParkEgg;
    pokeParkWrongSpecies.species = 55;
    assert(!PokeParkEgg::matches(pokeParkWrongSpecies, pokeParkEggRng));

    // PCJP Fifth Anniversary event eggs use a weighted table chosen two RNG
    // calls before the BA-CD sequence. Seed 0 selects non-shiny Ralts / Charm.
    const auto pcjpRaltsRng = analyzeWithTrainer(
        0x527131B0u, {2, 18, 3, 12, 22, 24}, 12345, 54321);
    assert(pcjpRaltsRng.matched());
    assert(pcjpRaltsRng.variant == Variant::Regular);
    namespace Pcjp5 = Legality::Gen3PcjpFifthEggEvent;
    assert(Pcjp5::tableSeed(pcjpRaltsRng) == 0u);
    auto pcjpRalts = Pcjp5::Candidate{
        280, 2, 1, 0, 0, 255, 4, true, false, false,
        u"オヤＮＡＭＥ", {45, 204, 0, 0}
    };
    assert(Pcjp5::matches(pcjpRalts, pcjpRaltsRng));

    // Seed 79 selects the shiny Pichu / Wish table slice.
    const auto pcjpShinyPichuRng = analyzeWithTrainer(
        0xF93E1D37u, {11, 30, 16, 26, 9, 5}, 12345, 54321);
    assert(pcjpShinyPichuRng.matched());
    assert(pcjpShinyPichuRng.variant == Variant::ForceShiny);
    assert(Pcjp5::tableSeed(pcjpShinyPichuRng) == 79u);
    auto pcjpPichu = Pcjp5::Candidate{
        172, 2, 1, 0, 0, 255, 4, true, false, true,
        u"オヤＮＡＭＥ", {84, 204, 273, 0}
    };
    assert(Pcjp5::matches(pcjpPichu, pcjpShinyPichuRng));

    auto pcjpWrongMove = pcjpPichu;
    pcjpWrongMove.moves = {84, 204, 298, 0};
    assert(!Pcjp5::matches(pcjpWrongMove, pcjpShinyPichuRng));
    auto pcjpHatched = pcjpRalts;
    pcjpHatched.isEgg = false;
    assert(!Pcjp5::matches(pcjpHatched, pcjpRaltsRng));

    // WISHMKR Jirachi is a restricted-seed BACD_R event with fixed
    // persistent distribution fields in the pinned PKHeX table.
    namespace Wishmkr = Legality::Gen3WishmkrEvent;
    assert(Wishmkr::matchesTemplate(
        385, 20043, 0, 2, 2, 0, 5, 255, 4, false, false, u"WISHMKR"));
    assert(!Wishmkr::matchesTemplate(
        385, 20043, 0, 1, 2, 0, 5, 255, 4, false, false, u"WISHMKR"));
    assert(!Wishmkr::matchesTemplate(
        385, 20043, 0, 2, 1, 0, 5, 255, 4, false, false, u"WISHMKR"));
    assert(!Wishmkr::matchesTemplate(
        385, 20043, 0, 2, 2, 1, 5, 255, 4, false, false, u"WISHMKR"));
    assert(!Wishmkr::matchesTemplate(
        385, 20043, 0, 2, 2, 0, 0, 255, 4, false, false, u"WISHMKR"));
    assert(!Wishmkr::matchesTemplate(
        385, 20043, 0, 2, 2, 0, 5, 0, 4, false, false, u"WISHMKR"));
    assert(!Wishmkr::matchesTemplate(
        385, 20043, 0, 2, 2, 0, 5, 255, 4, false, false, u"CHANNEL"));

    std::cout << "Gen III BA-CD event PID/IV correlation variants: PASS\n";
}
