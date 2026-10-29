// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include <array>
#include <cstdint>
namespace Legality::Gen4RanchIV {
// PKHeX26.10.18 MRNG / VerifyIVsRanch4: three rand() outputs, with
// the middle output unused. PID comes from a different source.
// Exhaust the complete16-bit unknown part of the first output state.
// Its high sign-bit alias gives identical15-bit outputs, so this is
// exhaustive, not a capped search or a PID/IV correlation heuristic.
inline bool hasState(const std::array<uint8_t,6>& ivs) noexcept {
    for (auto iv : ivs) if (iv > 31) return false;
    const uint32_t first = uint32_t(ivs[3] | (ivs[4] << 5) | (ivs[5] << 10)) << 16;
    const uint32_t third = ivs[0] | (ivs[1] << 5) | (ivs[2] << 10);
    for (uint32_t low = 0; low < 65536; ++low) {
        uint32_t state = first | low;
        state = state * 0x41C64E6Du + 12345u;
        state = state * 0x41C64E6Du + 12345u;
        if (((state >> 16) & 0x7FFFu) == third) return true;
    }
    return false;
}
} // namespace Legality::Gen4RanchIV
