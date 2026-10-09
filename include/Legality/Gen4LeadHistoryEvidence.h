#pragma once

#include "Legality/Gen4BugContestSynchronizeFailureEvidence.h"
#include "Legality/Gen4BugContestMixedSyncEvidence.h"
#include "Legality/Gen4PressureLeadEvidence.h"
#include "Legality/Gen4StaticMagnetLeadEvidence.h"

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace Legality::Gen4LeadHistory {

enum class Path : uint16_t {
    None = 0,
    StaticSuccess = 1u << 0,
    MagnetPullSuccess = 1u << 1,
    PressureSuccess = 1u << 2,
    SynchronizeFailure = 1u << 3,
    CuteCharmFailure = 1u << 4,
    PressureFailure = 1u << 5,
    StaticMagnetFailure = 1u << 6,
    IntimidateContinue = 1u << 7,
    SynchronizeMixedSuccessThenFailure = 1u << 8,
};

struct Result {
    uint16_t mask = 0;

    constexpr bool matched() const noexcept { return mask != 0; }
    constexpr bool has(Path path) const noexcept {
        return (mask & static_cast<uint16_t>(path)) != 0;
    }
    constexpr void add(Path path) noexcept {
        mask = static_cast<uint16_t>(mask | static_cast<uint16_t>(path));
    }
};

inline constexpr std::size_t kSourceCount =
    sizeof(Gen4Wild::kPackedGen4WildEncounters) /
    sizeof(Gen4Wild::kPackedGen4WildEncounters[0]);
inline constexpr std::size_t kLeadMetaCount =
    sizeof(Gen4Wild::kPackedGen4WildLeadMeta) /
    sizeof(Gen4Wild::kPackedGen4WildLeadMeta[0]);
inline constexpr std::size_t kPressureCount =
    sizeof(Gen4Wild::kPackedGen4WildPressureLevel) /
    sizeof(Gen4Wild::kPackedGen4WildPressureLevel[0]);
static_assert(kSourceCount == kLeadMetaCount,
              "Gen IV wild rows and lead metadata must stay index-aligned");
static_assert(kSourceCount == kPressureCount,
              "Gen IV wild rows and PressureLevel metadata must stay index-aligned");

// Positive-only existential reconstruction over the accepted lead primitives.
// A PK4 does not persist the lead ability used for its encounter, and more than
// one historical lead path can therefore explain the same saved identity. Keep
// every proven branch in a bitmask instead of selecting one and implying that it
// was the unique historical lead.
//
// This layer intentionally does not include the older no-lead / successful
// Synchronize / successful Cute Charm correlation paths already handled by the
// existing Method J/K verifier. It combines only the newly accepted extended
// lead-history branches so production reporting can be integrated separately.
constexpr Result matchIndexedRow(bool hgss, uint64_t row,
                                 uint32_t leadMeta,
                                 uint8_t pressureLevel,
                                 uint32_t prePidSeed,
                                 uint32_t pid,
                                 uint8_t metLevel) noexcept {
    Result result{};

    const auto attraction = Gen4StaticMagnetLead::matchRow(
        hgss, row, leadMeta, prePidSeed, pid, metLevel);
    if (attraction.lead == Gen4StaticMagnetLead::Lead::Static)
        result.add(Path::StaticSuccess);
    else if (attraction.lead == Gen4StaticMagnetLead::Lead::MagnetPull)
        result.add(Path::MagnetPullSuccess);

    if (Gen4PressureLead::matchRowWithPressure(
            hgss, row, pressureLevel,
            prePidSeed, pid, metLevel).matched())
        result.add(Path::PressureSuccess);

    if (Gen4LeadFailure::matchRow(
            hgss, row, prePidSeed, pid, metLevel,
            Gen4LeadFailure::Lead::Synchronize).matched() ||
        (hgss && Gen4BugContestSynchronizeFailure::match(
            row, prePidSeed, pid, metLevel).matched()))
        result.add(Path::SynchronizeFailure);

    // Pinned Method K RecurseReject: first attempt successful Sync, but
    // rejected for lacking 31 IV; retained attempt fails Sync and rolls its
    // nature independently. Proof is positive-only and distinct from the
    // existing all-failed Synchronize bit.
    if (hgss && Gen4BugContestMixedSync::matchSuccessThenFailure(
            row, prePidSeed, pid, metLevel).matched())
        result.add(Path::SynchronizeMixedSuccessThenFailure);

    if (Gen4LeadFailure::matchRow(
            hgss, row, prePidSeed, pid, metLevel,
            Gen4LeadFailure::Lead::CuteCharm).matched())
        result.add(Path::CuteCharmFailure);

    if (Gen4LeadFailure::matchRow(
            hgss, row, prePidSeed, pid, metLevel,
            Gen4LeadFailure::Lead::PressureHustleVitalSpirit).matched())
        result.add(Path::PressureFailure);

    if (Gen4LeadFailure::matchRow(
            hgss, row, prePidSeed, pid, metLevel,
            Gen4LeadFailure::Lead::StaticMagnetPull).matched())
        result.add(Path::StaticMagnetFailure);

    if (Gen4LeadFailure::matchRow(
            hgss, row, prePidSeed, pid, metLevel,
            Gen4LeadFailure::Lead::IntimidateKeenEye).matched())
        result.add(Path::IntimidateContinue);

    return result;
}

// Source-aware positive analyzer. Each generated alias retains exact parent-area
// Static/Magnet and Pressure history. Persisted-identical aliases are all tested;
// their proof masks are ORed because the save cannot distinguish which source
// EncounterArea4 produced the PK4.
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
    Result result{};

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

        const Result source = matchIndexedRow(
            hgss,
            row,
            Gen4Wild::kPackedGen4WildLeadMeta[i],
            Gen4Wild::kPackedGen4WildPressureLevel[i],
            prePidSeed,
            pid,
            metLevel);
        result.mask = static_cast<uint16_t>(result.mask | source.mask);
    }
    return result;
}

constexpr const char* pathName(Path path) noexcept {
    switch (path) {
        case Path::StaticSuccess: return "Static success";
        case Path::MagnetPullSuccess: return "Magnet Pull success";
        case Path::PressureSuccess:
            return "Pressure/Hustle/Vital Spirit success";
        case Path::SynchronizeFailure: return "Synchronize fail";
        case Path::CuteCharmFailure: return "Cute Charm fail";
        case Path::PressureFailure:
            return "Pressure/Hustle/Vital Spirit fail";
        case Path::StaticMagnetFailure: return "Static/Magnet Pull fail";
        case Path::IntimidateContinue:
            return "Intimidate/Keen Eye encounter-continues";
        case Path::SynchronizeMixedSuccessThenFailure:
            return "Synchronize success then failure (BCC reroll)";
        case Path::None: break;
    }
    return "No extended lead-history evidence";
}

} // namespace Legality::Gen4LeadHistory
