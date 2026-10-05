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

constexpr Result matchRowWithPressure(bool hgss, uint64_t row,
                                      uint8_t pressureLevel,
                                      uint32_t prePidSeed, uint32_t pid,
                                      uint8_t metLevel) noexcept {
    const uint8_t type = Gen4Wild::method(row);
    if (pressureLevel == 0 || metLevel != pressureLevel ||
        !Gen4LeadFailure::supportedType(hgss, type))
        return {};
    if (Gen4LeadFrame::isBugContest(type) &&
        !Gen4LeadFrame::directMinimum31Satisfied(prePidSeed))
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
