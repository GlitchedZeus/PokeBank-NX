#include "Legality/Gen3CxdPidIvCorrelation.h"
#include "Legality/Gen3ColoEReaderShadowEvidence.h"

#include <cassert>
#include <iostream>

int main() {
    using Legality::Gen3CxdPidIv::Variant;
    using Legality::Gen3CxdPidIv::analyze;
    using Legality::Gen3CxdPidIv::analyzeWithTrainer;
    namespace EReader = Legality::Gen3ColoEReaderShadowEvidence;

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

    // Japanese Colosseum e-Reader shadows use a separate path: all IVs are 0,
    // the stored PID is reversed with XDRNG.GetSeeds, then Next4(seed) is fed
    // into the four-lock Colosseum team-history validator. These deterministic
    // fixtures were derived directly from the pinned source locks.
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

    std::cout << "Gen III Colosseum/XD standard + CXDAnti + Colosseum e-Reader evidence: PASS\n";
}
