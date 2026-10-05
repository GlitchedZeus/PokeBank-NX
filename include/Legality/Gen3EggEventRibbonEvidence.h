#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace Legality::Gen3EggEventRibbon {
// PKHeX 6501f0ab46e8f8ca048539dbaf8cae8cb104e722:
// PKHeX.Core/Legality/Verifiers/Ribbons/RibbonVerifierEvent3.cs, ParseEgg.
// This is only an unhatched-egg contradiction check. Parse's encounter-template
// rules and Earth Ribbon's later Mt. Battle history are deliberately separate.
enum class Status : uint8_t { NotApplicable, Consistent, Invalid };
struct Context {
    bool isEgg;
    bool ribbonEarth;
    bool ribbonNational;
    bool ribbonCountry;
    bool ribbonChampionBattle;
    bool ribbonChampionRegional;
    bool ribbonChampionNational;
};
inline constexpr std::array<std::string_view, 6> kNames{
    "Earth", "National", "Country", "Champion Battle", "Champion Regional", "Champion National"
};
struct Result {
    Status status;
    // Same order as kNames; retain every contradiction, not just the first.
    std::array<bool, 6> contradictions{};
};
constexpr Result evaluate(const Context& c) noexcept {
    if (!c.isEgg) return {Status::NotApplicable, {}};
    const std::array<bool, 6> ribbons{
        c.ribbonEarth, c.ribbonNational, c.ribbonCountry,
        c.ribbonChampionBattle, c.ribbonChampionRegional, c.ribbonChampionNational
    };
    for (bool present : ribbons)
        if (present) return {Status::Invalid, ribbons};
    return {Status::Consistent, {}};
}
} // namespace Legality::Gen3EggEventRibbon
