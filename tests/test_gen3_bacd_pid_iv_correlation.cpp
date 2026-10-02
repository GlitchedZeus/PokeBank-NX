#include "Legality/Gen3BacdPidIvCorrelation.h"
#include "Legality/Gen3WishmkrEventTemplate.h"
#include "Legality/Gen3BerryFixEventTemplate.h"

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

    // Canonical PKHeX Berry Fix Zigzagoon: forced-shiny BA-CD_S, seed 0x20.
    const auto berryFix = analyzeWithTrainer(
        0x38CA4EA0u, {0, 20, 28, 11, 19, 0}, 30317, 0);
    assert(berryFix.matched());
    assert(berryFix.variant == Variant::ForceShiny);
    assert(berryFix.originSeed == 0x20u);
    assert(berryFix.restrictedSeed);
    assert(berryFix.originSeed <= 213u);

    using Legality::Gen3BerryFixEvent::matchesTemplate;
    assert(matchesTemplate(
        263, 30317, 0, 1, 2, 1, 5, 255, 4, false, false, u"RUBY"));
    assert(matchesTemplate(
        263, 30317, 0, 1, 2, 0, 5, 255, 4, false, false, u"SAPHIRE"));
    assert(matchesTemplate(
        263, 21121, 0, 1, 1, 1, 5, 255, 4, false, false, u"ルビー"));
    assert(matchesTemplate(
        263, 21121, 0, 1, 1, 0, 5, 255, 4, false, false, u"サファイア"));
    assert(!matchesTemplate(
        263, 30317, 0, 2, 2, 1, 5, 255, 4, false, false, u"RUBY"));
    assert(!matchesTemplate(
        263, 30317, 0, 1, 2, 0, 5, 255, 4, false, false, u"RUBY"));
    assert(!matchesTemplate(
        263, 30317, 0, 1, 1, 1, 5, 255, 4, false, false, u"RUBY"));

    // Restricted regular BACD example from PKHeX should expose the 16-bit seed class.
    const auto restricted = analyzeWithTrainer(
        0x0000E97Eu, {17, 19, 20, 16, 13, 12}, 0, 0);
    assert(restricted.matched());
    assert(restricted.variant == Variant::Regular);
    assert(restricted.restrictedSeed);

    // WISHMKR Jirachi is a restricted-seed BACD_R event with fixed
    // persistent distribution fields in the pinned PKHeX table.
    using Legality::Gen3WishmkrEvent::matchesTemplate;
    assert(matchesTemplate(
        385, 20043, 0, 2, 2, 0, 5, 255, 4, false, false, u"WISHMKR"));
    assert(!matchesTemplate(
        385, 20043, 0, 1, 2, 0, 5, 255, 4, false, false, u"WISHMKR"));
    assert(!matchesTemplate(
        385, 20043, 0, 2, 1, 0, 5, 255, 4, false, false, u"WISHMKR"));
    assert(!matchesTemplate(
        385, 20043, 0, 2, 2, 1, 5, 255, 4, false, false, u"WISHMKR"));
    assert(!matchesTemplate(
        385, 20043, 0, 2, 2, 0, 0, 255, 4, false, false, u"WISHMKR"));
    assert(!matchesTemplate(
        385, 20043, 0, 2, 2, 0, 5, 0, 4, false, false, u"WISHMKR"));
    assert(!matchesTemplate(
        385, 20043, 0, 2, 2, 0, 5, 255, 4, false, false, u"CHANNEL"));

    std::cout << "Gen III BA-CD event PID/IV correlation variants: PASS\n";
}
