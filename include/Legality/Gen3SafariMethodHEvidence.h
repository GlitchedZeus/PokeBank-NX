#pragma once

#include "Legality/Gen3MethodHSlot.h"
#include "Legality/Gen3PidIvCorrelation.h"
#include "Legality/Gen3SafariEncounter.h"
#include "Legality/Gen34EggMoveEvidence.h"

#include <cstdint>
#include <string_view>

namespace Legality::Gen3SafariMethodH {

// This module mirrors the pinned PKHeX Method-H "Regular" path only:
// - Ruby / Sapphire
// - FireRed / LeafGreen
//
// Emerald deliberately remains unsupported here because pinned MethodH permits
// a broader lead-ability search there (Synchronize, Cute Charm,
// Static/Magnet Pull, Pressure/Hustle/Vital Spirit, etc.).
//
// Method 3 also remains unresolved in this first frame-evidence tranche. The
// classic Method-3 PID contains a skipped RNG frame between PID halves, while
// pinned MethodH derives its reversal-window nature from the correlation seed.
// That corner is kept separate rather than inferred.
//
// This is positive encounter-frame evidence only. Failure to match is
// Unresolved and MUST NOT be interpreted as Invalid.

enum class Resolution : uint8_t {
    Unresolved,
    FrameMatched,
};

enum class Path : uint8_t {
    None,
    RegularNature,
    HoennSafariBlock,
};

struct Evidence {
    Resolution resolution = Resolution::Unresolved;
    Path path = Path::None;
    uint8_t requiredBall = 0;
    uint16_t sourceSpecies = 0;
    bool evolved = false;
    uint8_t encounterType = 0xFF;
    uint8_t slot = 0xFF;
    uint32_t pidSeed = 0;
    uint32_t frameSeed = 0;
};

constexpr bool supportedGame(Gen3Safari::Game game) noexcept {
    return game == Gen3Safari::Game::Ruby ||
           game == Gen3Safari::Game::Sapphire ||
           game == Gen3Safari::Game::FireRed ||
           game == Gen3Safari::Game::LeafGreen;
}

constexpr bool supportedCorrelation(Gen3PidIv::Method method) noexcept {
    return method == Gen3PidIv::Method::Method1 ||
           method == Gen3PidIv::Method::Method2 ||
           method == Gen3PidIv::Method::Method4;
}

namespace Detail {

constexpr uint16_t upper16(uint32_t seed) noexcept {
    return static_cast<uint16_t>(seed >> 16);
}

constexpr uint32_t sequentialPid(uint32_t seed) noexcept {
    const uint32_t first = Gen3PidIv::Detail::next(seed);
    const uint32_t second = Gen3PidIv::Detail::next(first);
    return (second & 0xFFFF0000u) | (first >> 16);
}

// Mirrors pinned MethodJ.GetReversalWindow, which MethodH reuses for its
// non-Emerald, non-Unown "Regular" condition.
inline uint32_t reversalWindow(uint32_t seed, uint8_t nature) noexcept {
    uint32_t count = 0;
    uint32_t b = upper16(seed);
    for (;;) {
        seed = Gen3PidIv::Detail::prev(seed);
        const uint32_t a = upper16(seed);
        const uint32_t pid = (b << 16) | a;
        if ((pid % 25u) == nature)
            return count;

        seed = Gen3PidIv::Detail::prev(seed);
        b = upper16(seed);
        ++count;
    }
}

constexpr bool rockSmashActivation(const Gen3Safari::Entry& row,
                                   uint32_t seed) noexcept {
    // Pinned MethodH.GetEncounterRate with the default assumptions used by the
    // legality checker: areaRate * 16, then +50% (White Flute assumption).
    uint32_t encounterRate = static_cast<uint32_t>(row.rate) * 16u;
    encounterRate += encounterRate >> 1;
    return (upper16(seed) % 2880u) < encounterRate;
}

constexpr bool frameMatches(const Gen3Safari::Entry& row,
                            uint8_t metLevel,
                            uint32_t frameSeed) noexcept {
    if (row.method > 5 || row.maxLevel < row.minLevel)
        return false;

    const uint32_t seed1 = Gen3PidIv::Detail::prev(frameSeed);
    const uint32_t seed2 = Gen3PidIv::Detail::prev(seed1);
    const uint32_t seed3 = Gen3PidIv::Detail::prev(seed2);

    const uint32_t span =
        1u + static_cast<uint32_t>(row.maxLevel) - row.minLevel;
    const uint8_t generatedLevel = static_cast<uint8_t>(
        (upper16(seed1) % span) + row.minLevel);
    if (generatedLevel != metLevel)
        return false;

    const auto type = static_cast<Gen3MethodHSlot::Type>(row.method);
    if (Gen3MethodHSlot::get(type, upper16(seed2)) != row.slot)
        return false;

    if (row.method == 5 && !rockSmashActivation(row, seed3))
        return false;

    return true;
}

constexpr uint32_t hoennSafariBlockSeed(uint32_t seed) noexcept {
    // Pinned MethodH 300-previous-call transform.
    return (0xC048C851u * seed) + 0x196302B4u;
}

constexpr bool hoennSafariBlockProc(uint32_t seed) noexcept {
    return (upper16(seed) % 100u) < 80u;
}

struct CandidateMatch {
    Path path = Path::None;
    uint32_t frameSeed = 0;
};

constexpr CandidateMatch matchCandidate(const Gen3Safari::Entry& row,
                                        uint8_t metLevel,
                                        uint32_t candidateSeed,
                                        uint8_t nature,
                                        bool hoennSafari) noexcept {
    if (hoennSafari) {
        const uint32_t blockSeed = hoennSafariBlockSeed(candidateSeed);
        if (hoennSafariBlockProc(blockSeed) &&
            frameMatches(row, metLevel, blockSeed))
            return {Path::HoennSafariBlock, blockSeed};
    }

    if ((upper16(candidateSeed) % 25u) != nature)
        return {};

    const uint32_t regularSeed = hoennSafari
        ? Gen3PidIv::Detail::prev(candidateSeed)
        : candidateSeed;
    if (frameMatches(row, metLevel, regularSeed))
        return {Path::RegularNature, regularSeed};

    return {};
}

inline bool sourceSpeciesMatches(std::string_view exactGameId,
                                 uint16_t currentSpecies,
                                 uint8_t currentForm,
                                 const Gen3Safari::Entry& row,
                                 bool& evolved) noexcept {
    if (row.species == currentSpecies) {
        evolved = false;
        return row.form == currentForm;
    }

    uint16_t ancestor = Gen34EggMove::preEvolution(
        exactGameId, currentSpecies);
    for (int depth = 0; ancestor != 0 && depth < 8; ++depth) {
        if (ancestor == row.species) {
            // The accepted Gen III Safari rows are ordinary base-form sources;
            // do not project the current evolved form onto the ancestor.
            evolved = true;
            return row.form == 0;
        }

        const uint16_t next = Gen34EggMove::preEvolution(
            exactGameId, ancestor);
        if (next == ancestor)
            break;
        ancestor = next;
    }
    return false;
}

} // namespace Detail

inline Evidence analyze(std::string_view exactGameId,
                        uint16_t currentSpecies,
                        uint16_t metLocation,
                        uint8_t metLevel,
                        uint8_t currentForm,
                        uint32_t pid,
                        const Gen3PidIv::Result& correlation) noexcept {
    const Gen3Safari::Game game = Gen3Safari::gameForId(exactGameId);
    if (!supportedGame(game) ||
        !supportedCorrelation(correlation.method) ||
        currentSpecies == 0 || currentSpecies > 386 ||
        metLocation > 0xFF || metLevel == 0 ||
        !Gen3Safari::isSafariLocation(game, metLocation))
        return {};

    // For Methods 1/2/4, the accepted correlation seed must regenerate the
    // persisted PID via the normal two sequential PID calls. Do not accept a
    // hand-constructed or incompatible correlation object.
    if (Detail::sequentialPid(correlation.originSeed) != pid)
        return {};

    const uint8_t nature = static_cast<uint8_t>(pid % 25u);
    const uint32_t reverseCount =
        Detail::reversalWindow(correlation.originSeed, nature);
    const bool hoennSafari =
        game == Gen3Safari::Game::Ruby ||
        game == Gen3Safari::Game::Sapphire;

    uint32_t candidateSeed = correlation.originSeed;
    for (uint32_t reverse = 0; reverse <= reverseCount; ++reverse) {
        for (const Gen3Safari::Entry& row : Gen3Safari::kGen3SafariEntries) {
            if (row.game != static_cast<uint8_t>(game) ||
                row.location != metLocation ||
                metLevel < row.minLevel || metLevel > row.maxLevel)
                continue;

            bool evolved = false;
            if (!Detail::sourceSpeciesMatches(
                    exactGameId, currentSpecies, currentForm, row, evolved))
                continue;

            const auto match = Detail::matchCandidate(
                row, metLevel, candidateSeed, nature, hoennSafari);
            if (match.path == Path::None)
                continue;

            Evidence out{};
            out.resolution = Resolution::FrameMatched;
            out.path = match.path;
            out.requiredBall = Gen3Safari::kSafariBall;
            out.sourceSpecies = row.species;
            out.evolved = evolved;
            out.encounterType = row.method;
            out.slot = row.slot;
            out.pidSeed = candidateSeed;
            out.frameSeed = match.frameSeed;
            return out;
        }

        if (reverse != reverseCount)
            candidateSeed = Gen3PidIv::Detail::prev(
                Gen3PidIv::Detail::prev(candidateSeed));
    }

    return {};
}

} // namespace Legality::Gen3SafariMethodH
