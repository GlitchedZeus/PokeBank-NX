#include "Legality/Gen3CxdPidIvCorrelation.h"

#include <cassert>
#include <iostream>

int main() {
    using Legality::Gen3CxdPidIv::Variant;
    using Legality::Gen3CxdPidIv::analyze;
    using Legality::Gen3CxdPidIv::analyzeWithTrainer;

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

    // The same recorded PID/IVs are not standard CXD: trainer-aware reversal
    // is what proves the skipped shiny target PID.
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

    // The anti-shiny proof is trainer-specific and IV-specific.
    assert(!analyzeWithTrainer(
        0xB7951831u, {9, 31, 12, 31, 25, 22}, antiTid, 35590).matched);
    assert(!analyzeWithTrainer(
        0xB7951831u, {9, 31, 12, 31, 25, 21}, antiTid, antiSid).matched);

    std::cout << "Gen III Colosseum/XD standard + CXDAnti PID/IV correlation: PASS\n";
}
