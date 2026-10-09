#pragma once

#include "Legality/Gen3PidIvCorrelation.h"

#include <array>
#include <cstdint>

namespace Legality::Gen4PidIv {

enum class Method : uint8_t {
    None,
    Method1,
};

struct Result {
    Method method = Method::None;
    uint32_t originSeed = 0;

    constexpr bool matched() const noexcept { return method != Method::None; }
};

constexpr const char* methodName(Method method) noexcept {
    return method == Method::Method1 ? "Method 1" : "No normal Method 1 match";
}

// Normal Gen IV static encounters and the base PID/IV correlation used by wild
// Method J/K encounters use the same sequential Method-1 PID -> IV relationship.
// This deliberately does NOT claim lead-frame legality. Cute Charm, Chain Shiny,
// Pokewalker, MG anti-shiny and other modified PID classes are separate evidence.
constexpr Result analyze(uint32_t pid, const std::array<uint8_t, 6>& ivs) noexcept {
    const auto base = Gen3PidIv::analyze(pid, ivs, false);
    return base.method == Gen3PidIv::Method::Method1
        ? Result{Method::Method1, base.originSeed}
        : Result{};
}

} // namespace Legality::Gen4PidIv
