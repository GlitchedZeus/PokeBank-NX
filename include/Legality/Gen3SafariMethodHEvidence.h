#pragma once

#include "Legality/Gen3MethodHSlot.h"
#include "Legality/Gen3PidIvCorrelation.h"
#include "Legality/Gen3SafariEncounter.h"
#include "Legality/Gen34EggMoveEvidence.h"

#include <cstdint>
#include <string_view>

namespace Legality::Gen3SafariMethodH {

// This module proves a bounded subset of pinned PKHeX Method-H Safari
// histories:
// - no-lead histories for Ruby / Sapphire / Emerald / FireRed / LeafGreen
// - Synchronize-success histories for Emerald
//
// Emerald also permits broader lead-ability histories. Only an isolated
// Synchronize-failure and Cute Charm-failure positive paths with proven
// no-block framing are added here. A bounded Pressure/Hustle/Vital Spirit
// failed-proc (lowered-level) and positive Pressure-family proc for Safari
// encounters are reconstructed. Grass-area Pressure uses original per-area
// species grouping; a bounded Emerald Static-success grass path is also
// reconstructed, including independent Hoenn Safari-block Static and grass
// Pressure/Hustle/Vital Spirit success paths, plus a bounded Safari-block
// failed Cute Charm lead-proc history and failed Synchronize history.
// Intimidate/Keen Eye non-repelled encounter checks are also bounded here; Cute Charm success and other leads remain unsupported.
//
// Method 3 uses an A_C PID shape: one RNG frame is skipped between the two
// persisted PID halves. Pinned LeadFinder still passes PIDIV.OriginSeed into
// MethodH.GetSeed, and MethodH derives its reversal nature from the sequential
// A+B PID generated from that seed. That source behavior is preserved here.
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
    EmeraldSynchronize,
    EmeraldSynchronizeFailed,
    EmeraldCuteCharmFailed,
    EmeraldPressureHustleFailed,
    EmeraldPressureHustleSuccess,
    EmeraldStaticSuccess,
    EmeraldIntimidateKeenEyeCheckFailed,
    EmeraldSafariBlockStaticSuccess,
    EmeraldSafariBlockPressureSuccess,
    EmeraldSafariBlockCuteCharmFailed,
    EmeraldSafariBlockSynchronizeFailed,
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
           game == Gen3Safari::Game::Emerald ||
           game == Gen3Safari::Game::FireRed ||
           game == Gen3Safari::Game::LeafGreen;
}

constexpr bool supportedCorrelation(Gen3PidIv::Method method) noexcept {
    return method == Gen3PidIv::Method::Method1 ||
           method == Gen3PidIv::Method::Method2 ||
           method == Gen3PidIv::Method::Method3 ||
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

// Classic Method 3 is A_CDE: A is the first PID half, B is a VBlank/skipped
// RNG frame, and C is the second persisted PID half.
constexpr uint32_t method3Pid(uint32_t seed) noexcept {
    const uint32_t first = Gen3PidIv::Detail::next(seed);
    const uint32_t skipped = Gen3PidIv::Detail::next(first);
    const uint32_t third = Gen3PidIv::Detail::next(skipped);
    return (third & 0xFFFF0000u) | (first >> 16);
}

// Mirrors pinned MethodJ.GetReversalWindow. For Emerald gendered species,
// pinned GetReversalWindowCute records NoLead at the first matching-nature PID;
// that NoLead count is therefore this same reversal window. Cute Charm may
// extend beyond it, but that separate Emerald lead path stays unresolved here.
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

constexpr CandidateMatch matchEmeraldSynchronize(
        const Gen3Safari::Entry& row,
        uint8_t metLevel,
        uint32_t candidateSeed) noexcept {
    // Pinned MethodH captures p0 before applying the Hoenn Safari one-call
    // no-block rewind. Synchronize succeeds when p0's low bit is zero.
    const uint16_t p0 = upper16(candidateSeed);
    if ((p0 & 1u) != 0)
        return {};

    const uint32_t frameSeed = Gen3PidIv::Detail::prev(candidateSeed);
    if (!frameMatches(row, metLevel, frameSeed))
        return {};

    return {Path::EmeraldSynchronize, frameSeed};
}

constexpr CandidateMatch matchEmeraldSynchronizeFail(
        const Gen3Safari::Entry& row,
        uint8_t metLevel,
        uint32_t candidateSeed,
        uint8_t nature) noexcept {
    // Pinned MethodH.TryGetMatch explores SynchronizeFail only after the
    // normal nature call matches, then TryGetMatchNoSync checks:
    // -1 Synchronize proc FAIL, -2 generated level, -3 slot,
    // Seed4 encounter activation. Restrict this positive-evidence subset to
    // a failed Hoenn Safari nature-preference block and a p0 that cannot be
    // Synchronize-success. All other histories remain Unresolved.
    const uint16_t p0 = upper16(candidateSeed);
    if ((p0 % 25u) != nature || (p0 & 1u) == 0u ||
        hoennSafariBlockProc(hoennSafariBlockSeed(candidateSeed)))
        return {};

    const uint32_t frameSeed = Gen3PidIv::Detail::prev(candidateSeed);
    const uint32_t procSeed = Gen3PidIv::Detail::prev(frameSeed);
    if ((upper16(procSeed) & 1u) == 0u)
        return {}; // Synchronize proc succeeded, not failed.

    const uint32_t levelSeed = Gen3PidIv::Detail::prev(procSeed);
    const uint32_t slotSeed = Gen3PidIv::Detail::prev(levelSeed);
    const uint32_t activationSeed = Gen3PidIv::Detail::prev(slotSeed);
    if (row.method > 5 || row.maxLevel < row.minLevel)
        return {};

    const uint32_t span = 1u + row.maxLevel - row.minLevel;
    const uint8_t level = static_cast<uint8_t>(
        (upper16(levelSeed) % span) + row.minLevel);
    if (level != metLevel)
        return {};
    if (Gen3MethodHSlot::get(
            static_cast<Gen3MethodHSlot::Type>(row.method),
            upper16(slotSeed)) != row.slot)
        return {};
    if (row.method == 5 && !rockSmashActivation(row, activationSeed))
        return {};

    return {Path::EmeraldSynchronizeFailed, frameSeed};
}

constexpr CandidateMatch matchEmeraldCuteCharmFail(
        const Gen3Safari::Entry& row,
        uint8_t metLevel,
        uint32_t candidateSeed,
        uint8_t nature) noexcept {
    // Pinned MethodH.TryGetMatchNoSync explores CuteCharmFail after the
    // regular nature condition. IsCuteCharmPass(proc) is (proc % 3) != 0,
    // so a failed proc is divisible by 3. The level/slot are at -2/-3,
    // just as for SynchronizeFail.
    //
    // This positive-evidence subset requires a failed Safari nature block
    // and an odd p0 (no Synchronize-success). Restrict to even proc values,
    // ensuring the same frame cannot also be SynchronizeFail.
    const uint16_t p0 = upper16(candidateSeed);
    if ((p0 % 25u) != nature || (p0 & 1u) == 0u ||
        hoennSafariBlockProc(hoennSafariBlockSeed(candidateSeed)))
        return {};
    const uint32_t frameSeed = Gen3PidIv::Detail::prev(candidateSeed);
    const uint16_t proc = upper16(Gen3PidIv::Detail::prev(frameSeed));
    if (proc % 3u != 0u || (proc & 1u) != 0u)
        return {};

    const uint32_t levelSeed = Gen3PidIv::Detail::prev(
        Gen3PidIv::Detail::prev(frameSeed));
    const uint32_t slotSeed = Gen3PidIv::Detail::prev(levelSeed);
    const uint32_t activationSeed = Gen3PidIv::Detail::prev(slotSeed);
    if (row.method > 5 || row.maxLevel < row.minLevel)
        return {};
    const uint32_t span = 1u + row.maxLevel - row.minLevel;
    const uint8_t level = static_cast<uint8_t>(
        (upper16(levelSeed) % span) + row.minLevel);
    if (level != metLevel ||
        Gen3MethodHSlot::get(
            static_cast<Gen3MethodHSlot::Type>(row.method),
            upper16(slotSeed)) != row.slot)
        return {};
    if (row.method == 5 && !rockSmashActivation(row, activationSeed))
        return {};
    return {Path::EmeraldCuteCharmFailed, frameSeed};
}

constexpr CandidateMatch matchEmeraldPressureHustleFail(
        const Gen3Safari::Entry& row,
        uint8_t metLevel,
        uint32_t candidateSeed,
        uint8_t nature) noexcept {
    // Pinned MethodH.IsSlotValidHustleVitalFail:
    // p0 = nature; -1 proc FAIL (upper16 & 1 == 0), -2 level
    // (GetRandomLevelMinus1), -3 encounter slot. CheckEncounterActivation
    // consumes Seed4 for Rock Smash. Positive-only and deliberately limited
    // to a failed Hoenn Safari nature-preference block, odd p0 and no lead
    // Synchronize-success at the same frame.
    const uint16_t p0 = upper16(candidateSeed);
    if ((p0 % 25u) != nature || (p0 & 1u) == 0u ||
        hoennSafariBlockProc(hoennSafariBlockSeed(candidateSeed)))
        return {};
    const uint32_t frameSeed = Gen3PidIv::Detail::prev(candidateSeed);
    const uint32_t procSeed = Gen3PidIv::Detail::prev(frameSeed);
    if ((upper16(procSeed) & 1u) != 0u)
        return {}; // Pressure/Hustle/Vital Spirit procced instead.
    const uint32_t levelSeed = Gen3PidIv::Detail::prev(procSeed);
    const uint32_t slotSeed = Gen3PidIv::Detail::prev(levelSeed);
    const uint32_t activationSeed = Gen3PidIv::Detail::prev(slotSeed);
    if (row.method > 5 || row.maxLevel < row.minLevel)
        return {};
    const uint32_t span = 1u + row.maxLevel - row.minLevel;
    uint32_t levelBias = upper16(levelSeed) % span;
    if (levelBias != 0u)
        --levelBias;
    const uint8_t level = static_cast<uint8_t>(row.minLevel + levelBias);
    if (level != metLevel ||
        Gen3MethodHSlot::get(
            static_cast<Gen3MethodHSlot::Type>(row.method),
            upper16(slotSeed)) != row.slot)
        return {};
    if (row.method == 5 && !rockSmashActivation(row, activationSeed))
        return {};
    return {Path::EmeraldPressureHustleFailed, frameSeed};
}

// Mirrors pinned EncounterArea3.GetPressureMax: for grass, check every slot
// within the SAME EncounterArea3, restricting to the same original species.
// Location alone is insufficient because multiple areas share Safari location.
constexpr uint8_t grassPressureLevel(const Gen3Safari::Entry& row) noexcept {
    uint8_t highest = row.maxLevel;
    for (const auto& other : Gen3Safari::kGen3SafariEntries) {
        if (other.game != row.game || other.areaIndex != row.areaIndex ||
            other.species != row.species)
            continue;
        if (other.maxLevel > highest)
            highest = other.maxLevel;
    }
    return highest;
}

constexpr CandidateMatch matchEmeraldPressureHustleSuccess(
        const Gen3Safari::Entry& row,
        uint8_t metLevel,
        uint32_t candidateSeed,
        uint8_t nature) noexcept {
    // Pinned MethodH.IsSlotValidHustleVital uses a successful proc at -1,
    // ignores the random level at -2, and forces EncounterSlot3.PressureLevel.
    // For non-grass PressureLevel is row.maxLevel; grass instead uses pinned
    // Parent.GetPressureMax(species, maxLevel) over the original area group.
    const uint16_t p0 = upper16(candidateSeed);
    if ((p0 % 25u) != nature || (p0 & 1u) == 0u ||
        hoennSafariBlockProc(hoennSafariBlockSeed(candidateSeed)))
        return {};
    if (row.method > 5 || row.maxLevel < row.minLevel)
        return {};
    const uint8_t pressureLevel = row.method == 0
        ? grassPressureLevel(row) : row.maxLevel;
    if (metLevel != pressureLevel)
        return {};
    const uint32_t frameSeed = Gen3PidIv::Detail::prev(candidateSeed);
    const uint32_t procSeed = Gen3PidIv::Detail::prev(frameSeed);
    if ((upper16(procSeed) & 1u) == 0u)
        return {}; // Failed proc belongs to a different lead history.
    const uint32_t levelSeed = Gen3PidIv::Detail::prev(procSeed);
    const uint32_t slotSeed = Gen3PidIv::Detail::prev(levelSeed);
    const uint32_t activationSeed = Gen3PidIv::Detail::prev(slotSeed);
    if (Gen3MethodHSlot::get(
            static_cast<Gen3MethodHSlot::Type>(row.method),
            upper16(slotSeed)) != row.slot)
        return {};
    if (row.method == 5 && !rockSmashActivation(row, activationSeed))
        return {};
    return {Path::EmeraldPressureHustleSuccess, frameSeed};
}

// Pinned MethodH.IsSlotValidStaticMagnet and IMagnetStatic:
// -3 Static proc succeeds on an even half-word; -2 ESV redirects among the
// eligible Static slots (roll % StaticCount == StaticIndex); -1 produces the
// ordinary encounter level and 0 chooses nature. Only Emerald Safari grass
// entries with source Static metadata are included; Safari has no Magnet Pull
// eligible rows, so this does not claim a Magnet Pull path.
constexpr CandidateMatch matchEmeraldStaticSuccess(
        const Gen3Safari::Entry& row,
        uint8_t metLevel,
        uint32_t candidateSeed,
        uint8_t nature) noexcept {
    const uint16_t p0 = upper16(candidateSeed);
    if ((p0 % 25u) != nature || (p0 & 1u) == 0u ||
        hoennSafariBlockProc(hoennSafariBlockSeed(candidateSeed)))
        return {};
    if (row.method != 0 || row.staticCount == 0 ||
        row.staticIndex >= row.staticCount ||
        row.maxLevel < row.minLevel)
        return {};

    // Hoenn Safari failed nature preference consumes a one-call rewind.
    const uint32_t frameSeed = Gen3PidIv::Detail::prev(candidateSeed);
    const uint32_t levelSeed = Gen3PidIv::Detail::prev(frameSeed);
    const uint32_t slotSeed = Gen3PidIv::Detail::prev(levelSeed);
    const uint32_t procSeed = Gen3PidIv::Detail::prev(slotSeed);
    if ((upper16(procSeed) & 1u) != 0u)
        return {}; // Static proc failed, not this successful lead history.

    const uint32_t span = 1u + row.maxLevel - row.minLevel;
    const uint8_t level = static_cast<uint8_t>(
        row.minLevel + (upper16(levelSeed) % span));
    if (level != metLevel ||
        upper16(slotSeed) % row.staticCount != row.staticIndex)
        return {};
    return {Path::EmeraldStaticSuccess, frameSeed};
}

// Pinned MethodH.TryGetMatch / TryGetMatchNoSync and IsSlotValidStaticMagnet:
// a successful Hoenn Safari block proc uses the dedicated 300-call rewind
// block seed as frame 0. Static still consumes the -3 success proc, -2
// Static-eligible ESV and -1 ordinary level; unlike no-block Static, this
// path is independent of p0's regular-nature and Synchronize preconditions.
// Only source-backed Emerald grass Static rows are admitted. Other block
// lead histories, including Magnet Pull, remain unresolved.
constexpr CandidateMatch matchEmeraldSafariBlockStaticSuccess(
        const Gen3Safari::Entry& row,
        uint8_t metLevel,
        uint32_t candidateSeed) noexcept {
    if (row.method != 0 || row.staticCount == 0 ||
        row.staticIndex >= row.staticCount ||
        row.maxLevel < row.minLevel)
        return {};
    const uint32_t blockSeed = hoennSafariBlockSeed(candidateSeed);
    if (!hoennSafariBlockProc(blockSeed))
        return {};

    const uint32_t levelSeed = Gen3PidIv::Detail::prev(blockSeed);
    const uint32_t slotSeed = Gen3PidIv::Detail::prev(levelSeed);
    const uint32_t procSeed = Gen3PidIv::Detail::prev(slotSeed);
    if ((upper16(procSeed) & 1u) != 0u ||
        upper16(slotSeed) % row.staticCount != row.staticIndex)
        return {};

    const uint32_t span = 1u + row.maxLevel - row.minLevel;
    const uint8_t level = static_cast<uint8_t>(
        row.minLevel + (upper16(levelSeed) % span));
    if (level != metLevel)
        return {};
    return {Path::EmeraldSafariBlockStaticSuccess, blockSeed};
}

// Pinned MethodH.TryGetMatch uses the 300-call Safari nature-block seed as
// frame 0 and then TryGetMatchNoSync can prove Pressure/Hustle/Vital Spirit
// success. MethodH.IsSlotValidHustleVital checks -1 successful proc, ignores
// the -2 level roll, and checks the -3 ordinary encounter slot. For grass,
// EncounterSlot3.PressureLevel uses Parent.GetPressureMax(Species, LevelMax)
// *within the original EncounterArea3*, not the maximum for the location.
// This remains positive-only, Emerald-only, and confined to grass.
constexpr CandidateMatch matchEmeraldSafariBlockPressureSuccess(
        const Gen3Safari::Entry& row,
        uint8_t metLevel,
        uint32_t candidateSeed) noexcept {
    if (row.method != 0 || row.maxLevel < row.minLevel ||
        metLevel != grassPressureLevel(row))
        return {};
    const uint32_t blockSeed = hoennSafariBlockSeed(candidateSeed);
    if (!hoennSafariBlockProc(blockSeed))
        return {};

    const uint32_t procSeed = Gen3PidIv::Detail::prev(blockSeed);
    const uint32_t levelSeed = Gen3PidIv::Detail::prev(procSeed);
    const uint32_t slotSeed = Gen3PidIv::Detail::prev(levelSeed);
    if ((upper16(procSeed) & 1u) == 0u ||
        Gen3MethodHSlot::get(Gen3MethodHSlot::Type::Grass,
                            upper16(slotSeed)) != row.slot)
        return {};

    // A successful Pressure-family lead overrides the sampled level. The
    // -2 level RNG call still occurs but is intentionally not compared.
    return {Path::EmeraldSafariBlockPressureSuccess, blockSeed};
}

// Pinned MethodH.TryGetMatchNoSync / IsSlotValidCuteCharmFail:
// when the independent Hoenn Safari 300-call block succeeds, Cute Charm's
// -1 proc fails if rand % 3 == 0. The normal level is then at -2 and the
// ordinary slot at -3. Restrict to *even* failed-proc values so this positive
// frame proof does not overlap the Synchronize-failure history (odd proc).
// Emerald Safari grass only. No hard-invalid inference from a non-match.
constexpr CandidateMatch matchEmeraldSafariBlockCuteCharmFail(
        const Gen3Safari::Entry& row,
        uint8_t metLevel,
        uint32_t candidateSeed) noexcept {
    if (row.method != 0 || row.maxLevel < row.minLevel)
        return {};
    const uint32_t blockSeed = hoennSafariBlockSeed(candidateSeed);
    if (!hoennSafariBlockProc(blockSeed))
        return {};

    const uint32_t procSeed = Gen3PidIv::Detail::prev(blockSeed);
    const uint16_t proc = upper16(procSeed);
    if (proc % 3u != 0u || (proc & 1u) != 0u)
        return {};
    const uint32_t levelSeed = Gen3PidIv::Detail::prev(procSeed);
    const uint32_t slotSeed = Gen3PidIv::Detail::prev(levelSeed);
    const uint32_t span = 1u + row.maxLevel - row.minLevel;
    const uint8_t level = static_cast<uint8_t>(
        row.minLevel + (upper16(levelSeed) % span));
    if (level != metLevel ||
        Gen3MethodHSlot::get(Gen3MethodHSlot::Type::Grass,
                            upper16(slotSeed)) != row.slot)
        return {};
    return {Path::EmeraldSafariBlockCuteCharmFailed, blockSeed};
}

// Pinned MethodH.TryGetMatchNoSync / IsSlotValidSyncFail:
// after a successful Hoenn Safari nature-block proc, the 300-call rewind
// establishes an independent frame 0. A failed Synchronize lead consumes a
// -1 proc with odd upper-half RNG; -2 is the ordinary level and -3 the normal
// encounter slot. This is positive-only and bounded to Emerald Safari grass.
// Check before successful Pressure because the same PID/IV can have distinct
// valid levels with different held lead abilities.
constexpr CandidateMatch matchEmeraldSafariBlockSynchronizeFail(
        const Gen3Safari::Entry& row,
        uint8_t metLevel,
        uint32_t candidateSeed) noexcept {
    if (row.method != 0 || row.maxLevel < row.minLevel)
        return {};
    const uint32_t blockSeed = hoennSafariBlockSeed(candidateSeed);
    if (!hoennSafariBlockProc(blockSeed))
        return {};

    const uint32_t procSeed = Gen3PidIv::Detail::prev(blockSeed);
    if ((upper16(procSeed) & 1u) == 0u)
        return {};
    const uint32_t levelSeed = Gen3PidIv::Detail::prev(procSeed);
    const uint32_t slotSeed = Gen3PidIv::Detail::prev(levelSeed);
    const uint32_t span = 1u + row.maxLevel - row.minLevel;
    const uint8_t level = static_cast<uint8_t>(
        row.minLevel + (upper16(levelSeed) % span));
    if (level != metLevel ||
        Gen3MethodHSlot::get(Gen3MethodHSlot::Type::Grass,
                            upper16(slotSeed)) != row.slot)
        return {};
    return {Path::EmeraldSafariBlockSynchronizeFailed, blockSeed};
}

// Pinned MethodH.IsSlotValidIntimidate: an encounter occurs only when the
// -1 adequacy check does NOT reject it (even RNG upper half). The -2 level,
// -3 ordinary slot and optional Rock Smash activation are then checked.
// Only an isolated positive Emerald Safari no-block, matching-nature window
// is reconstructed. A rejected encounter cannot produce a Pokémon.
constexpr CandidateMatch matchEmeraldIntimidateKeenEyeCheckFailed(
        const Gen3Safari::Entry& row,
        uint8_t metLevel,
        uint32_t candidateSeed,
        uint8_t nature) noexcept {
    const uint16_t p0 = upper16(candidateSeed);
    if ((p0 % 25u) != nature || (p0 & 1u) == 0u ||
        hoennSafariBlockProc(hoennSafariBlockSeed(candidateSeed)))
        return {};
    if (row.method > 5 || row.maxLevel < row.minLevel)
        return {};

    const uint32_t frameSeed = Gen3PidIv::Detail::prev(candidateSeed);
    const uint32_t procSeed = Gen3PidIv::Detail::prev(frameSeed);
    const uint16_t proc = upper16(procSeed);
    if ((proc & 1u) != 0u || (proc % 3u) == 0u)
        return {}; // Encounter rejected, or Cute Charm-fail path overlaps.
    const uint32_t levelSeed = Gen3PidIv::Detail::prev(procSeed);
    const uint32_t slotSeed = Gen3PidIv::Detail::prev(levelSeed);
    const uint32_t activationSeed = Gen3PidIv::Detail::prev(slotSeed);
    const uint32_t span = 1u + row.maxLevel - row.minLevel;
    const uint8_t level = static_cast<uint8_t>(
        row.minLevel + (upper16(levelSeed) % span));
    if (level != metLevel ||
        Gen3MethodHSlot::get(
            static_cast<Gen3MethodHSlot::Type>(row.method),
            upper16(slotSeed)) != row.slot)
        return {};
    if (row.method == 5 && !rockSmashActivation(row, activationSeed))
        return {};
    return {Path::EmeraldIntimidateKeenEyeCheckFailed, frameSeed};
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

    // Validate the persisted PID against the correlation shape. Methods 1/2/4
    // are sequential A+B; Method 3 persists A+C with one skipped RNG frame.
    const uint32_t regeneratedPid =
        correlation.method == Gen3PidIv::Method::Method3
            ? Detail::method3Pid(correlation.originSeed)
            : Detail::sequentialPid(correlation.originSeed);
    if (regeneratedPid != pid)
        return {};

    // Pinned MethodH.GetSeed always derives the reversal nature from
    // ClassicEraRNG.GetSequentialPID(originSeed), even for Method 3. For
    // Method 3 this A+B PID can have a different nature than persisted A+C.
    const uint8_t nature = static_cast<uint8_t>(
        Detail::sequentialPid(correlation.originSeed) % 25u);
    const uint32_t reverseCount =
        Detail::reversalWindow(correlation.originSeed, nature);
    const bool hoennSafari =
        game == Gen3Safari::Game::Ruby ||
        game == Gen3Safari::Game::Sapphire ||
        game == Gen3Safari::Game::Emerald;

    uint32_t candidateSeed = correlation.originSeed;
    for (uint32_t reverse = 0; reverse <= reverseCount; ++reverse) {
        for (const Gen3Safari::Entry& row : Gen3Safari::kGen3SafariEntries) {
            if (row.game != static_cast<uint8_t>(game) ||
                row.location != metLocation ||
                metLevel < row.minLevel ||
                (metLevel > row.maxLevel &&
                 !(game == Gen3Safari::Game::Emerald && row.method == 0)))
                continue;

            bool evolved = false;
            if (!Detail::sourceSpeciesMatches(
                    exactGameId, currentSpecies, currentForm, row, evolved))
                continue;

            auto match = Detail::matchCandidate(
                row, metLevel, candidateSeed, nature, hoennSafari);
            if (match.path == Path::None &&
                game == Gen3Safari::Game::Emerald) {
                match = Detail::matchEmeraldSynchronize(
                    row, metLevel, candidateSeed);
                if (match.path == Path::None)
                    match = Detail::matchEmeraldSynchronizeFail(
                        row, metLevel, candidateSeed, nature);
                if (match.path == Path::None)
                    match = Detail::matchEmeraldCuteCharmFail(
                        row, metLevel, candidateSeed, nature);
                if (match.path == Path::None)
                    match = Detail::matchEmeraldPressureHustleFail(
                        row, metLevel, candidateSeed, nature);
                if (match.path == Path::None)
                    match = Detail::matchEmeraldPressureHustleSuccess(
                        row, metLevel, candidateSeed, nature);
                if (match.path == Path::None)
                    match = Detail::matchEmeraldStaticSuccess(
                        row, metLevel, candidateSeed, nature);
                if (match.path == Path::None)
                    match = Detail::matchEmeraldSafariBlockSynchronizeFail(
                        row, metLevel, candidateSeed);
                if (match.path == Path::None)
                    match = Detail::matchEmeraldSafariBlockStaticSuccess(
                        row, metLevel, candidateSeed);
                if (match.path == Path::None)
                    match = Detail::matchEmeraldSafariBlockPressureSuccess(
                        row, metLevel, candidateSeed);
                if (match.path == Path::None)
                    match = Detail::matchEmeraldSafariBlockCuteCharmFail(
                        row, metLevel, candidateSeed);
                if (match.path == Path::None)
                    match = Detail::matchEmeraldIntimidateKeenEyeCheckFailed(
                        row, metLevel, candidateSeed, nature);
            }
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
