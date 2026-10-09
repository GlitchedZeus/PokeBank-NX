#pragma once

#include "Legality/Gen3CxdPidIvCorrelation.h"

#include <array>
#include <cstddef>
#include <cstdint>

namespace Legality::Gen3GameCubeStarterCorrelation {

enum class Variant : uint8_t {
    None,
    ColosseumUmbreon,
    ColosseumEspeon,
    XdEevee,
};

struct Result {
    bool matched = false;
    bool searchLimited = false;
    Variant variant = Variant::None;
    uint32_t originSeed = 0;
};

inline constexpr uint32_t kMaxPidRerolls = 65536;

namespace Detail {

constexpr uint32_t nextN(uint32_t seed, unsigned count) noexcept {
    while (count-- != 0)
        seed = Gen3CxdPidIv::Detail::next(seed);
    return seed;
}

constexpr bool isShiny(uint16_t tid, uint16_t sid, uint32_t pid) noexcept {
    return static_cast<uint16_t>(tid ^ sid ^ (pid >> 16) ^ (pid & 0xFFFFu)) < 8u;
}

constexpr bool isMaleEevee(uint32_t pid) noexcept {
    // Eevee-family female threshold is 31; Colosseum locks both starters male.
    return (pid & 0xFFu) >= 31u;
}

struct GeneratedPid {
    bool matched = false;
    bool searchLimited = false;
    uint32_t pid = 0;
    uint32_t endSeed = 0;
};

constexpr GeneratedPid generateColoStarterPid(uint32_t seed,
                                                uint16_t tid,
                                                uint16_t sid,
                                                uint32_t rerollLimit) noexcept {
    for (uint32_t i = 0; i < rerollLimit; ++i) {
        seed = Gen3CxdPidIv::Detail::next(seed);
        const uint32_t high = seed >> 16;
        seed = Gen3CxdPidIv::Detail::next(seed);
        const uint32_t low = seed >> 16;
        const uint32_t pid = (high << 16) | low;
        if (isMaleEevee(pid) && !isShiny(tid, sid, pid))
            return {true, false, pid, seed};
    }
    return {false, true, 0, seed};
}

constexpr bool ivPairMatches(uint32_t seed,
                             uint32_t iv1,
                             uint32_t iv2) noexcept {
    seed = Gen3CxdPidIv::Detail::next(seed);
    if (((seed >> 16) & 0x7FFFu) != iv1)
        return false;
    seed = Gen3CxdPidIv::Detail::next(seed);
    return ((seed >> 16) & 0x7FFFu) == iv2;
}

} // namespace Detail

// Mirrors pinned PKHeX MethodCXD.TryGetSeedStarterColo.
// Umbreon is generated first; Espeon is generated second after Umbreon's
// male/non-shiny PID lock and its own fake-PID/IV/ability sequence.
constexpr Result analyzeColosseum(uint16_t species,
                                  uint32_t pid,
                                  const std::array<uint8_t, 6>& ivs,
                                  uint16_t tid,
                                  uint16_t sid,
                                  uint32_t rerollLimit = kMaxPidRerolls) noexcept {
    if (species != 197 && species != 196)
        return {};

    const uint32_t iv1 = Gen3CxdPidIv::Detail::packIv1(ivs);
    const uint32_t iv2 = Gen3CxdPidIv::Detail::packIv2(ivs);
    const auto seeds = Gen3CxdPidIv::Detail::reversePid(
        static_cast<uint32_t>(tid) << 16,
        static_cast<uint32_t>(sid) << 16);

    bool limited = false;
    for (std::size_t i = 0; i < seeds.count; ++i) {
        const uint32_t origin = seeds.values[i];

        if (species == 197) {
            // TID, SID, fake PID x2, then Umbreon IV1/IV2.
            if (!Detail::ivPairMatches(Detail::nextN(origin, 4), iv1, iv2))
                continue;
        }

        // TID, SID, fake PID x2, IV1, IV2, ability => starter PID.
        const auto umbreon = Detail::generateColoStarterPid(
            Detail::nextN(origin, 7), tid, sid, rerollLimit);
        if (umbreon.searchLimited) {
            limited = true;
            continue;
        }
        if (!umbreon.matched)
            continue;

        if (species == 197) {
            if (umbreon.pid == pid)
                return {true, false, Variant::ColosseumUmbreon, origin};
            continue;
        }

        // Espeon follows Umbreon: fake PID x2, IV1/IV2, ability, then PID.
        const uint32_t afterFake = Detail::nextN(umbreon.endSeed, 2);
        if (!Detail::ivPairMatches(afterFake, iv1, iv2))
            continue;
        const auto espeon = Detail::generateColoStarterPid(
            Detail::nextN(afterFake, 3), tid, sid, rerollLimit);
        if (espeon.searchLimited) {
            limited = true;
            continue;
        }
        if (espeon.matched && espeon.pid == pid)
            return {true, false, Variant::ColosseumEspeon, origin};
    }

    return {false, limited, Variant::None, 0};
}

// Mirrors pinned PKHeX MethodCXD.TryGetSeedStarterXD. XD Eevee is generated
// immediately after TID/SID, fake PID x2, IV1/IV2 and ability; unlike the
// Colosseum starters, the PID is not re-rolled for male/non-shiny locks.
constexpr Result analyzeXdEevee(uint16_t species,
                                uint32_t pid,
                                const std::array<uint8_t, 6>& ivs,
                                uint16_t tid,
                                uint16_t sid) noexcept {
    if (species != 133)
        return {};

    const uint32_t iv1 = Gen3CxdPidIv::Detail::packIv1(ivs);
    const uint32_t iv2 = Gen3CxdPidIv::Detail::packIv2(ivs);
    const auto seeds = Gen3CxdPidIv::Detail::reversePid(
        static_cast<uint32_t>(tid) << 16,
        static_cast<uint32_t>(sid) << 16);

    for (std::size_t i = 0; i < seeds.count; ++i) {
        const uint32_t origin = seeds.values[i];
        if (!Detail::ivPairMatches(Detail::nextN(origin, 4), iv1, iv2))
            continue;

        uint32_t seed = Detail::nextN(origin, 7);
        seed = Gen3CxdPidIv::Detail::next(seed);
        const uint32_t high = seed >> 16;
        seed = Gen3CxdPidIv::Detail::next(seed);
        const uint32_t low = seed >> 16;
        if (((high << 16) | low) == pid)
            return {true, false, Variant::XdEevee, origin};
    }

    return {};
}

} // namespace Legality::Gen3GameCubeStarterCorrelation
