#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace Legality::Gen34EggMove {

enum class Group : uint8_t {
    None,
    Gen3,
    DiamondPearl,
    Platinum,
    HeartGoldSoulSilver,
};

constexpr Group groupForId(std::string_view id) noexcept {
    if (id == "ruby_gba" || id == "sapphire_gba" || id == "emerald_gba" ||
        id == "firered_gba" || id == "leafgreen_gba")
        return Group::Gen3;
    if (id == "diamond_nds" || id == "pearl_nds")
        return Group::DiamondPearl;
    if (id == "platinum_nds")
        return Group::Platinum;
    if (id == "heartgold_nds" || id == "soulsilver_nds")
        return Group::HeartGoldSoulSilver;
    return Group::None;
}

#include "Legality/Gen34EggMoveData.inc"

constexpr bool bit(const std::array<uint64_t, 8>& row, uint16_t move) noexcept {
    if (move == 0 || move > 467)
        return false;
    return ((row[move >> 6] >> (move & 63)) & 1ULL) != 0;
}

constexpr const std::array<uint64_t, 8>* rowFor(Group group, uint16_t species) noexcept {
    if (species == 0 || species > 493)
        return nullptr;
    switch (group) {
        case Group::Gen3: return &kEggMovesGen3[species];
        case Group::DiamondPearl: return &kEggMovesDiamondPearl[species];
        case Group::Platinum: return &kEggMovesPlatinum[species];
        case Group::HeartGoldSoulSilver: return &kEggMovesHeartGoldSoulSilver[species];
        case Group::None: break;
    }
    return nullptr;
}

constexpr bool isEggMove(std::string_view exactGameId, uint16_t species,
                         uint16_t move) noexcept {
    const auto* row = rowFor(groupForId(exactGameId), species);
    return row && bit(*row, move);
}

constexpr bool speciesHasEggMoves(std::string_view exactGameId,
                                  uint16_t species) noexcept {
    const auto* row = rowFor(groupForId(exactGameId), species);
    if (!row) return false;
    for (const uint64_t word : *row)
        if (word != 0) return true;
    return false;
}

} // namespace Legality::Gen34EggMove
