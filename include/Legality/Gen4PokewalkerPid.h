#pragma once

#include <cstdint>

namespace Legality::Gen4PokewalkerPid {

// Port of PKHeX PokewalkerRNG.GetPID at the pinned legality reference.
// This is PID evidence only; course slot and IV seed provenance are separate layers.
constexpr uint32_t expected(uint32_t id32, uint8_t nature,
                            uint8_t gender, uint8_t genderRatio) noexcept {
    const uint16_t tid = static_cast<uint16_t>(id32);
    const uint16_t sid = static_cast<uint16_t>(id32 >> 16);
    uint32_t requestedNature = nature;
    if (requestedNature >= 24)
        requestedNature = 0;

    uint32_t pid =
        (((static_cast<uint32_t>(tid ^ sid) >> 8) ^ 0xFFu) & 0xFFu) << 24;
    pid += requestedNature - (pid % 25u);

    if (genderRatio == 0 || genderRatio >= 0xFE)
        return pid;

    const uint8_t pidGender = (pid & 0xFFu) < genderRatio ? 1 : 0;
    if (gender == pidGender)
        return pid;

    if (gender == 0) {
        pid += (((genderRatio - (pid & 0xFFu)) / 25u) + 1u) * 25u;
        if ((requestedNature & 1u) != (pid & 1u))
            pid += 25u;
    } else {
        pid -= ((((pid & 0xFFu) - genderRatio) / 25u) + 1u) * 25u;
        if ((requestedNature & 1u) != (pid & 1u))
            pid -= 25u;
    }
    return pid;
}

constexpr bool matches(uint32_t pid, uint32_t id32, uint8_t nature,
                       uint8_t gender, uint8_t genderRatio) noexcept {
    return pid == expected(id32, nature, gender, genderRatio);
}

} // namespace Legality::Gen4PokewalkerPid
