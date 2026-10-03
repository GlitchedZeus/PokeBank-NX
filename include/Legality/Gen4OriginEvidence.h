#pragma once

#include <cstdint>
#include <string_view>

namespace Legality::Gen4Origin {

enum class Kind : uint8_t {
    Unknown,
    Gen3Handheld,
    Gen3GameCube,
    Gen4Retail,
    Gen4BattleRevolution,
};

constexpr Kind kind(uint8_t version) noexcept {
    switch (version) {
        case 1: // Sapphire
        case 2: // Ruby
        case 3: // Emerald
        case 4: // FireRed
        case 5: // LeafGreen
            return Kind::Gen3Handheld;
        case 15:
            return Kind::Gen3GameCube;
        case 7:  // HeartGold
        case 8:  // SoulSilver
        case 10: // Diamond
        case 11: // Pearl
        case 12: // Platinum
            return Kind::Gen4Retail;
        case 16:
            return Kind::Gen4BattleRevolution;
        default:
            return Kind::Unknown;
    }
}

constexpr std::string_view exactRetailGameId(uint8_t version) noexcept {
    switch (version) {
        case 7:  return "heartgold_nds";
        case 8:  return "soulsilver_nds";
        case 10: return "diamond_nds";
        case 11: return "pearl_nds";
        case 12: return "platinum_nds";
        default: return {};
    }
}

constexpr std::string_view exactGen3GameId(uint8_t version) noexcept {
    switch (version) {
        case 1: return "sapphire_gba";
        case 2: return "ruby_gba";
        case 3: return "emerald_gba";
        case 4: return "firered_gba";
        case 5: return "leafgreen_gba";
        default: return {};
    }
}

constexpr bool isNativeRetailGen4(uint8_t version) noexcept {
    return kind(version) == Kind::Gen4Retail;
}

constexpr bool isPalParkOrigin(uint8_t version) noexcept {
    const auto k = kind(version);
    return k == Kind::Gen3Handheld || k == Kind::Gen3GameCube;
}

} // namespace Legality::Gen4Origin
