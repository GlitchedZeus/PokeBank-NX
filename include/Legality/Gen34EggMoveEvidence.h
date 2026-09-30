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

enum class MoveEvidence : uint8_t {
    None,
    DirectEggMove,
    PreEvolutionEggMove,
};

struct MoveResult {
    MoveEvidence evidence = MoveEvidence::None;
    uint16_t sourceSpecies = 0;

    constexpr bool matched() const noexcept {
        return evidence != MoveEvidence::None;
    }
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

constexpr uint16_t preEvolution(std::string_view exactGameId,
                                uint16_t species) noexcept {
    if (species == 0 || species > 493)
        return 0;
    const Group group = groupForId(exactGameId);
    if (group == Group::Gen3)
        return kPreEvolutionGen3[species];
    if (group == Group::DiamondPearl || group == Group::Platinum ||
        group == Group::HeartGoldSoulSilver)
        return kPreEvolutionGen4[species];
    return 0;
}

constexpr MoveResult classify(std::string_view exactGameId, uint16_t species,
                              uint16_t move) noexcept {
    if (isEggMove(exactGameId, species, move))
        return {MoveEvidence::DirectEggMove, species};

    uint16_t ancestor = preEvolution(exactGameId, species);
    for (int depth = 0; ancestor != 0 && depth < 8; ++depth) {
        if (isEggMove(exactGameId, ancestor, move))
            return {MoveEvidence::PreEvolutionEggMove, ancestor};
        const uint16_t next = preEvolution(exactGameId, ancestor);
        if (next == ancestor)
            break;
        ancestor = next;
    }
    return {};
}

constexpr const char* evidenceName(MoveEvidence evidence) noexcept {
    switch (evidence) {
        case MoveEvidence::DirectEggMove: return "direct egg move";
        case MoveEvidence::PreEvolutionEggMove: return "retained pre-evolution egg move";
        case MoveEvidence::None: break;
    }
    return "no egg-move evidence";
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
