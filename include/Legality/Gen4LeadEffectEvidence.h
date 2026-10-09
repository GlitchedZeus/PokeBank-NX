#pragma once

#include <cstdint>

namespace Legality::Gen4LeadEffect {

// Deterministic rand16 lead-effect predicates from pinned PKHeX MethodJ/MethodK.
// These are evidence primitives only: a passing predicate does not by itself prove
// an encounter history because slot, level, activation and PID frame ordering must
// still be reconstructed by the Method J/K matcher.
enum class Method : uint8_t {
    J,
    K,
};

constexpr bool synchronizePass(Method method, uint16_t rand16) noexcept {
    return method == Method::J
        ? (rand16 >> 15) == 0u
        : (rand16 & 1u) == 0u;
}

constexpr bool cuteCharmPass(Method method, uint16_t rand16) noexcept {
    return method == Method::J
        ? (rand16 / 0x5556u) != 0u
        : (rand16 % 3u) != 0u;
}

constexpr bool staticMagnetPass(Method method, uint16_t rand16) noexcept {
    return method == Method::J
        ? (rand16 >> 15) == 0u
        : (rand16 & 1u) == 0u;
}

constexpr bool pressureHustleVitalSpiritPass(Method method,
                                              uint16_t rand16) noexcept {
    return method == Method::J
        ? (rand16 >> 15) == 1u
        : (rand16 & 1u) == 1u;
}

// For Intimidate / Keen Eye, the "pass" bit means the repel-style level check
// succeeds and the encounter routine aborts. Therefore an encounter that actually
// occurred through this lead requires the opposite branch.
constexpr bool intimidateKeenEyeAbort(Method method, uint16_t rand16) noexcept {
    return method == Method::J
        ? (rand16 >> 15) == 1u
        : (rand16 & 1u) == 1u;
}

constexpr bool intimidateKeenEyeEncounterContinues(Method method,
                                                    uint16_t rand16) noexcept {
    return !intimidateKeenEyeAbort(method, rand16);
}

constexpr bool synchronizeFail(Method method, uint16_t rand16) noexcept {
    return !synchronizePass(method, rand16);
}

constexpr bool cuteCharmFail(Method method, uint16_t rand16) noexcept {
    return !cuteCharmPass(method, rand16);
}

constexpr bool staticMagnetFail(Method method, uint16_t rand16) noexcept {
    return !staticMagnetPass(method, rand16);
}

constexpr bool pressureHustleVitalSpiritFail(Method method,
                                              uint16_t rand16) noexcept {
    return !pressureHustleVitalSpiritPass(method, rand16);
}

} // namespace Legality::Gen4LeadEffect
