#pragma once

#include <cstdint>

namespace Legality::Gen34WurmpleEvolution {

enum class Branch : uint8_t {
    SilcoonBeautifly = 0,
    CascoonDustox = 1,
};

enum class Status : uint8_t {
    NotApplicable = 0,
    Compatible,
    Incompatible,
};

struct Result {
    Status status = Status::NotApplicable;
    Branch pidBranch = Branch::SilcoonBeautifly;
};

constexpr Branch branchForPid(uint32_t pid) noexcept {
    const uint32_t high = pid >> 16;
    return static_cast<Branch>((high % 10u) / 5u);
}

constexpr bool isWurmpleEvolution(uint16_t species) noexcept {
    return species >= 266 && species <= 269;
}

constexpr Branch branchForEvolution(uint16_t species) noexcept {
    // 266 Silcoon / 267 Beautifly => 0
    // 268 Cascoon / 269 Dustox   => 1
    return static_cast<Branch>((species - 266u) >> 1u);
}

constexpr Result evaluate(uint16_t encounterSpecies,
                          uint16_t currentSpecies,
                          uint32_t pid) noexcept {
    // Mirrors the pinned PKHeX verifier boundary: the PID branch constraint only
    // exists when the matched encounter species itself was Wurmple. Directly
    // encountered Silcoon/Cascoon (and any other non-Wurmple source) must not be
    // rejected by this helper.
    if (encounterSpecies != 265)
        return {};

    const Branch pidBranch = branchForPid(pid);

    // A surviving Wurmple only carries a prediction of its future branch; there
    // is no evolved-species contradiction to report yet.
    if (currentSpecies == 265)
        return {Status::Compatible, pidBranch};

    if (!isWurmpleEvolution(currentSpecies))
        return {};

    return {
        branchForEvolution(currentSpecies) == pidBranch
            ? Status::Compatible
            : Status::Incompatible,
        pidBranch
    };
}

constexpr const char* branchName(Branch branch) noexcept {
    return branch == Branch::SilcoonBeautifly
        ? "Silcoon/Beautifly"
        : "Cascoon/Dustox";
}

} // namespace Legality::Gen34WurmpleEvolution
