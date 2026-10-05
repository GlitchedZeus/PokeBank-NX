#pragma once

#include "Legality/Gen4LeadFailureEvidence.h"

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace Legality::Gen4PressureLead {

enum class Lead : uint8_t {
    None,
    PressureHustleVitalSpirit,
};

struct Result {
    Lead lead = Lead::None;
    uint32_t encounterSeed = 0;
    uint8_t slot = 0;
    uint8_t pressureLevel = 0;
    uint8_t rerollDepth = 0;

    constexpr bool matched() const noexcept { return lead != Lead::None; }
};

inline constexpr std::size_t kSourceCount =
    sizeof(Gen4Wild::kPackedGen4WildEncounters) /
    sizeof(Gen4Wild::kPackedGen4WildEncounters[0]);
inline constexpr std::size_t kPressureCount =
    sizeof(Gen4Wild::kPackedGen4WildPressureLevel) /
    sizeof(Gen4Wild::kPackedGen4WildPressureLevel[0]);
static_assert(kSourceCount == kPressureCount,
              "Gen IV wild rows and PressureLevel metadata must stay index-aligned");

constexpr uint8_t sourcePressureLevel(std::size_t index) noexcept {
    return index < kPressureCount
        ? Gen4Wild::kPackedGen4WildPressureLevel[index]
        : 0;
}

constexpr uint8_t exactPressureLevel(uint64_t row) noexcept {
    return Gen4Wild::method(row) == 0 ? 0 : Gen4Wild::maxLevel(row);
}

// Proves one complete Pressure/Hustle/Vital Spirit origin attempt. For HG/SS
// minimum-31 encounters this intentionally does not enforce the IV gate; an
// earlier rejected origin has no 31 IV by definition and is validated by the
// reroll chain wrapper below.
constexpr Result matchAttemptWithPressure(bool hgss, uint64_t row,
                                          uint8_t pressureLevel,
                                          uint32_t prePidSeed, uint32_t pid,
                                          uint8_t metLevel) noexcept {
    const uint8_t type = Gen4Wild::method(row);
    if (pressureLevel == 0 || metLevel != pressureLevel ||
        !Gen4LeadFailure::supportedType(hgss, type))
        return {};

    const uint8_t nature = static_cast<uint8_t>(pid % 25u);
    const int frames = Gen4LeadFrame::reversalWindow(prePidSeed, nature);
    if (frames < 0)
        return {};

    uint32_t candidate = prePidSeed;
    for (int i = 0; i <= frames; ++i) {
        const uint16_t natureRand = static_cast<uint16_t>(candidate >> 16);
        const uint32_t natureRoll = hgss
            ? (natureRand % 25u)
            : (natureRand / 0x0A3Eu);

        if (natureRoll == nature) {
            const uint32_t seed1 = Gen3PidIv::Detail::prev(candidate);
            const uint16_t prev1 = static_cast<uint16_t>(seed1 >> 16);
            if (Gen4LeadEffect::pressureHustleVitalSpiritPass(
                    Gen4LeadFailure::leadMethod(hgss), prev1)) {
                const uint32_t seed2 = Gen3PidIv::Detail::prev(seed1);
                const uint16_t prev2 = static_cast<uint16_t>(seed2 >> 16);

                if (!Gen4LeadFailure::levelIsRandom(hgss, type)) {
                    const uint8_t slot =
                        Gen4LeadFailure::rolledSlot(hgss, row, prev2);
                    if (slot == Gen4Wild::slot(row)) {
                        return {
                            Lead::PressureHustleVitalSpirit,
                            candidate,
                            slot,
                            pressureLevel,
                            0,
                        };
                    }
                } else {
                    const uint32_t seed3 = Gen3PidIv::Detail::prev(seed2);
                    const uint16_t prev3 = static_cast<uint16_t>(seed3 >> 16);
                    const uint8_t slot =
                        Gen4LeadFailure::rolledSlot(hgss, row, prev3);

                    if (slot == Gen4Wild::slot(row)) {
                        const uint32_t activationSeed =
                            Gen3PidIv::Detail::prev(seed3);
                        if (Gen4LeadFailure::normalActivationAllows(
                                hgss, row, activationSeed, true)) {
                            return {
                                Lead::PressureHustleVitalSpirit,
                                candidate,
                                slot,
                                pressureLevel,
                                0,
                            };
                        }
                    }
                }
            }
        }

        candidate = Gen3PidIv::Detail::prev(
            Gen3PidIv::Detail::prev(candidate));
    }
    return {};
}

// Mirrors the non-Synchronize Method K RecurseReject path for HG/SS Bug
// Catching Contest. Each retry must land on its generated PID nature, the
// immediately preceding attempt must fail the minimum-31 IV gate, and after the
// requested number of rejections the earliest attempt must prove the complete
// Pressure lead / slot / activation history. This is deliberately restricted to
// Pressure-family success; other lead families keep their existing unresolved
// behavior until their own recursion constraints are reconstructed.
constexpr Result matchBugContestRerollWithPressure(
        bool hgss, uint64_t row, uint8_t pressureLevel,
        uint32_t prePidSeed, uint32_t pid, uint8_t metLevel,
        uint8_t depth) noexcept {
    const uint8_t type = Gen4Wild::method(row);
    if (!hgss || !Gen4LeadFrame::isBugContest(type) ||
        depth == 0 || depth > 3 ||
        !Gen4LeadFrame::minimum31IvChainAllows(prePidSeed, depth))
        return {};

    uint32_t attemptSeed = prePidSeed;
    uint32_t attemptPid = pid;
    for (uint8_t i = 0; i < depth; ++i) {
        const uint32_t generatedPid = Gen4LeadFrame::sequentialPid(attemptSeed);
        if (generatedPid != attemptPid ||
            ((attemptSeed >> 16) % 25u) != (attemptPid % 25u) ||
            !Gen4LeadFrame::previousRerollAttemptRejected(attemptSeed))
            return {};

        attemptSeed = Gen4LeadFrame::previousRerollNatureSeed(attemptSeed);
        attemptPid = Gen4LeadFrame::sequentialPid(attemptSeed);
    }

    Result origin = matchAttemptWithPressure(
        hgss, row, pressureLevel, attemptSeed, attemptPid, metLevel);
    if (!origin.matched())
        return {};
    origin.rerollDepth = depth;
    return origin;
}

constexpr Result matchRowWithPressure(bool hgss, uint64_t row,
                                      uint8_t pressureLevel,
                                      uint32_t prePidSeed, uint32_t pid,
                                      uint8_t metLevel) noexcept {
    const uint8_t type = Gen4Wild::method(row);
    if (pressureLevel == 0 || metLevel != pressureLevel ||
        !Gen4LeadFailure::supportedType(hgss, type))
        return {};

    if (!Gen4LeadFrame::isBugContest(type))
        return matchAttemptWithPressure(
            hgss, row, pressureLevel, prePidSeed, pid, metLevel);

    if (Gen4LeadFrame::directMinimum31Satisfied(prePidSeed)) {
        const Result direct = matchAttemptWithPressure(
            hgss, row, pressureLevel, prePidSeed, pid, metLevel);
        if (direct.matched())
            return direct;
    }

    for (uint8_t depth = 1; depth <= 3; ++depth) {
        const Result rerolled = matchBugContestRerollWithPressure(
            hgss, row, pressureLevel, prePidSeed, pid, metLevel, depth);
        if (rerolled.matched())
            return rerolled;
    }
    return {};
}

constexpr Result matchRow(bool hgss, uint64_t row,
                          uint32_t prePidSeed, uint32_t pid,
                          uint8_t metLevel) noexcept {
    return matchRowWithPressure(
        hgss, row, exactPressureLevel(row), prePidSeed, pid, metLevel);
}

inline Result analyzeSupported(std::string_view exactGameId,
                               uint16_t speciesId,
                               uint16_t metLocation,
                               uint8_t metLevel,
                               uint8_t pokemonForm,
                               uint32_t id32,
                               uint32_t prePidSeed,
                               uint32_t pid) noexcept {
    const auto wanted = Gen4Wild::gameForId(exactGameId);
    if (wanted == Gen4Wild::Game::Invalid || speciesId == 0 ||
        metLocation > 0xFF || metLevel == 0)
        return {};

    const bool hgss =
        wanted == Gen4Wild::Game::HeartGold ||
        wanted == Gen4Wild::Game::SoulSilver;

    for (std::size_t i = 0; i < kSourceCount; ++i) {
        const uint64_t row = Gen4Wild::kPackedGen4WildEncounters[i];
        if (Gen4Wild::game(row) != wanted ||
            Gen4Wild::species(row) != speciesId ||
            Gen4Wild::location(row) != metLocation ||
            !Gen4Wild::formMatches(Gen4Wild::form(row), pokemonForm))
            continue;

        if (speciesId == 446 && Gen4Wild::method(row) == 9 &&
            !Gen4Wild::isMunchlaxTreeLocation(id32, metLocation))
            continue;

        const uint8_t pressureLevel = sourcePressureLevel(i);
        if (pressureLevel != metLevel)
            continue;

        const auto result = matchRowWithPressure(
            hgss, row, pressureLevel, prePidSeed, pid, metLevel);
        if (result.matched())
            return result;
    }
    return {};
}

constexpr const char* leadName(Lead lead) noexcept {
    switch (lead) {
        case Lead::PressureHustleVitalSpirit:
            return "Pressure/Hustle/Vital Spirit";
        case Lead::None:
            break;
    }
    return "No successful pressure lead evidence";
}

} // namespace Legality::Gen4PressureLead
