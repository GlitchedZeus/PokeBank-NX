#ifndef POKEBANK_UI_CLASSIC_MACHINE_ARTWORK_H
#define POKEBANK_UI_CLASSIC_MACHINE_ARTWORK_H
// Pinned move-TYPE evidence, not an approximation based on TM number or item ID.
// Gen I: pret/pokered d2704a63; Gen II: pret/pokecrystal e058e4f5;
// Gen III: pret/pokeemerald c587cc9e; Gen IV: Gen III original-type
// continuity + veekun/pokedex moves.csv for newly introduced moves.
// Type 0 is Curse's original typeless/??? status and has no invented disc.
#include <array>
#include <cstdint>
#include <string>
#include "Enums/GameVersion.h"
#include "Names/MachineDisplay.h"
namespace UI::ClassicMachineArtwork {
inline constexpr std::array<std::uint8_t, 166> Gen1Types = {{
    0, 1, 1, 1, 1, 1, 1, 10, 15, 13, 1, 1, 1, 1, 1, 1, 1, 3, 1, 3,
    1, 1, 12, 1, 2, 1, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    4, 7, 7, 1, 1, 1, 1, 1, 1, 1, 1, 4, 10, 10, 15, 11, 11, 11, 15, 15,
    14, 11, 15, 1, 3, 3, 2, 2, 2, 2, 1, 12, 12, 12, 1, 12, 12, 4, 12, 12,
    12, 7, 16, 10, 13, 13, 13, 13, 6, 5, 5, 5, 4, 14, 14, 14, 14, 14, 1, 1,
    14, 8, 1, 1, 1, 1, 1, 1, 1, 8, 11, 1, 14, 14, 15, 14, 1, 1, 1, 3,
    1, 1, 8, 4, 4, 5, 10, 11, 11, 1, 1, 1, 1, 14, 14, 1, 2, 1, 14, 4,
    1, 7, 1, 3, 1, 11, 1, 12, 1, 14, 1, 4, 11, 1, 1, 5, 14, 6, 1, 1,
    1, 1, 1, 1, 1, 1,
}};
inline constexpr std::array<std::uint8_t, 252> Gen2Types = {{
    0, 1, 2, 1, 1, 1, 1, 10, 15, 13, 1, 1, 1, 1, 1, 1, 3, 3, 1, 3,
    1, 1, 12, 1, 2, 1, 2, 2, 5, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    4, 7, 7, 1, 17, 1, 1, 1, 1, 1, 1, 4, 10, 10, 15, 11, 11, 11, 15, 15,
    14, 11, 15, 1, 3, 3, 2, 2, 2, 2, 1, 12, 12, 12, 1, 12, 12, 4, 12, 12,
    12, 7, 16, 10, 13, 13, 13, 13, 6, 5, 5, 5, 4, 14, 14, 14, 14, 14, 1, 1,
    14, 8, 1, 1, 1, 1, 1, 1, 1, 8, 11, 1, 14, 14, 15, 14, 1, 1, 1, 3,
    1, 1, 8, 4, 4, 5, 10, 11, 11, 1, 1, 1, 1, 14, 14, 1, 2, 1, 14, 4,
    1, 7, 1, 3, 1, 11, 1, 12, 1, 14, 1, 4, 11, 1, 1, 5, 14, 6, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 2, 17, 7, 1, 8, 10, 1, 0, 1, 1, 3, 12, 2,
    8, 15, 1, 2, 1, 17, 1, 1, 4, 5, 11, 5, 13, 1, 8, 1, 15, 2, 5, 1,
    16, 6, 12, 1, 1, 6, 1, 1, 1, 13, 7, 9, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 10, 5, 2, 7, 16, 1, 1, 17, 1, 1, 9, 9, 2, 1, 12, 1, 1, 2, 16,
    11, 10, 17, 14, 1, 1, 6, 8, 14, 2, 11, 17,
}};
inline constexpr std::array<std::uint8_t, 355> Gen3Types = {{
    1, 1, 2, 1, 1, 1, 1, 10, 15, 13, 1, 1, 1, 1, 1, 1, 3, 3, 1, 3,
    1, 1, 12, 1, 2, 1, 2, 2, 5, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    4, 7, 7, 1, 17, 1, 1, 1, 1, 1, 1, 4, 10, 10, 15, 11, 11, 11, 15, 15,
    14, 11, 15, 1, 3, 3, 2, 2, 2, 2, 1, 12, 12, 12, 1, 12, 12, 4, 12, 12,
    12, 7, 16, 10, 13, 13, 13, 13, 6, 5, 5, 5, 4, 14, 14, 14, 14, 14, 1, 1,
    14, 8, 1, 1, 1, 1, 1, 1, 1, 8, 11, 1, 14, 14, 15, 14, 1, 1, 1, 3,
    1, 1, 8, 4, 4, 5, 10, 11, 11, 1, 1, 1, 1, 14, 14, 1, 2, 1, 14, 4,
    1, 7, 1, 3, 1, 11, 1, 12, 1, 14, 1, 4, 11, 1, 1, 5, 14, 6, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 2, 17, 7, 1, 8, 10, 1, 0, 1, 1, 3, 12, 2,
    8, 15, 1, 2, 1, 17, 1, 1, 4, 5, 11, 5, 13, 1, 8, 1, 15, 2, 5, 1,
    16, 6, 12, 1, 1, 6, 1, 1, 1, 13, 7, 9, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 10, 5, 2, 7, 16, 1, 1, 17, 1, 1, 9, 9, 2, 1, 12, 1, 1, 2, 16,
    11, 10, 17, 14, 1, 1, 6, 8, 14, 2, 11, 17, 1, 1, 1, 1, 1, 10, 15, 17,
    17, 10, 17, 1, 2, 1, 1, 1, 13, 17, 1, 14, 14, 1, 1, 12, 2, 14, 1, 2,
    2, 1, 17, 1, 10, 14, 14, 1, 8, 17, 1, 11, 2, 1, 7, 14, 14, 3, 1, 10,
    5, 15, 12, 1, 1, 4, 1, 10, 11, 9, 8, 1, 12, 17, 3, 10, 1, 6, 7, 9,
    12, 1, 14, 11, 7, 8, 14, 2, 5, 15, 11, 12, 3, 15, 9, 1, 1, 16, 12, 2,
    3, 5, 4, 1, 13, 12, 11, 14, 12, 16, 6, 13, 11, 9, 14,
}};
inline constexpr std::array<std::uint8_t, 468> Gen4Types = {{
    1, 1, 2, 1, 1, 1, 1, 10, 15, 13, 1, 1, 1, 1, 1, 1, 3, 3, 1, 3,
    1, 1, 12, 1, 2, 1, 2, 2, 5, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    4, 7, 7, 1, 17, 1, 1, 1, 1, 1, 1, 4, 10, 10, 15, 11, 11, 11, 15, 15,
    14, 11, 15, 1, 3, 3, 2, 2, 2, 2, 1, 12, 12, 12, 1, 12, 12, 4, 12, 12,
    12, 7, 16, 10, 13, 13, 13, 13, 6, 5, 5, 5, 4, 14, 14, 14, 14, 14, 1, 1,
    14, 8, 1, 1, 1, 1, 1, 1, 1, 8, 11, 1, 14, 14, 15, 14, 1, 1, 1, 3,
    1, 1, 8, 4, 4, 5, 10, 11, 11, 1, 1, 1, 1, 14, 14, 1, 2, 1, 14, 4,
    1, 7, 1, 3, 1, 11, 1, 12, 1, 14, 1, 4, 11, 1, 1, 5, 14, 6, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 2, 17, 7, 1, 8, 10, 1, 0, 1, 1, 3, 12, 2,
    8, 15, 1, 2, 1, 17, 1, 1, 4, 5, 11, 5, 13, 1, 8, 1, 15, 2, 5, 1,
    16, 6, 12, 1, 1, 6, 1, 1, 1, 13, 7, 9, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 10, 5, 2, 7, 16, 1, 1, 17, 1, 1, 9, 9, 2, 1, 12, 1, 1, 2, 16,
    11, 10, 17, 14, 1, 1, 6, 8, 14, 2, 11, 17, 1, 1, 1, 1, 1, 10, 15, 17,
    17, 10, 17, 1, 2, 1, 1, 1, 13, 17, 1, 14, 14, 1, 1, 12, 2, 14, 1, 2,
    2, 1, 17, 1, 10, 14, 14, 1, 8, 17, 1, 11, 2, 1, 7, 14, 14, 3, 1, 10,
    5, 15, 12, 1, 1, 4, 1, 10, 11, 9, 8, 1, 12, 17, 3, 10, 1, 6, 7, 9,
    12, 1, 14, 11, 7, 8, 14, 2, 5, 15, 11, 12, 3, 15, 9, 1, 1, 16, 12, 2,
    3, 5, 4, 1, 13, 12, 11, 14, 12, 16, 6, 13, 11, 9, 14, 3, 14, 14, 2, 2,
    9, 14, 11, 1, 1, 3, 3, 1, 9, 7, 2, 17, 17, 17, 17, 14, 1, 14, 1, 14,
    4, 1, 1, 1, 14, 14, 17, 1, 12, 17, 4, 14, 11, 13, 10, 2, 2, 6, 4, 17,
    17, 11, 12, 3, 7, 7, 16, 16, 6, 2, 2, 2, 12, 3, 5, 17, 1, 17, 9, 15,
    15, 8, 13, 15, 10, 8, 5, 14, 14, 9, 9, 1, 3, 14, 16, 13, 10, 12, 12, 6,
    4, 4, 9, 9, 6, 1, 6, 12, 3, 1, 7, 13, 12, 11, 7, 7, 7, 6, 1, 16,
    16, 14, 1, 10, 17, 12, 8, 8,
}};
inline constexpr std::array<std::uint8_t, 55> Gen1Machines = {{
    1, 1, 1, 1, 1, 4, 1, 1, 1, 1, 11, 11, 15, 15, 1, 1, 2, 2, 2, 1,
    12, 12, 16, 13, 13, 5, 5, 5, 14, 14, 1, 1, 14, 1, 1, 1, 1, 10, 1, 1,
    1, 14, 3, 14, 13, 14, 1, 6, 1, 1, 1, 3, 11, 1, 1,
}};
inline constexpr const char* typeName(std::uint8_t id) noexcept {
    constexpr std::array<const char*, 19> names = {{
        "", "normal", "fighting", "flying", "poison", "ground", "rock",
        "bug", "ghost", "steel", "fire", "water", "grass", "electric",
        "psychic", "ice", "dragon", "dark", "fairy"
    }};
    return id < names.size() ? names[id] : "";
}
inline constexpr std::uint8_t moveType(Enums::GameVersion game, std::uint16_t move) noexcept {
    using G = Enums::GameVersion;
    switch (game) {
        case G::RBY: return move < Gen1Types.size() ? Gen1Types[move] : 0;
        case G::GSC: return move < Gen2Types.size() ? Gen2Types[move] : 0;
        case G::FRLG: case G::FR: case G::LG:
            return move < Gen3Types.size() ? Gen3Types[move] : 0;
        case G::D: case G::P: case G::DP: case G::Pt: case G::PT:
        case G::HG: case G::SS: case G::HGSS:
            return move < Gen4Types.size() ? Gen4Types[move] : 0;
        default: return 0;
    }
}
// RBY raw item IDs 196..200 are HM01-05, 201..250 are TM01-50.
// Never reinterpret the same numeric IDs in another generation.
inline std::string artKey(Enums::GameVersion game, std::uint16_t itemId) {
    using G = Enums::GameVersion;
    const char* prefix = nullptr;
    std::uint8_t type = 0;
    if (game == G::RBY) {
        if (itemId >= 196 && itemId <= 200) {
            prefix = "hm-"; type = Gen1Machines[50 + itemId - 196];
        } else if (itemId >= 201 && itemId <= 250) {
            prefix = "tm-"; type = Gen1Machines[itemId - 201];
        }
    } else {
        const auto machine = Names::getMachineDescriptor(game, itemId);
        if (!machine) return {};
        prefix = machine.kind == Names::MachineKind::HM ? "hm-" :
                 machine.kind == Names::MachineKind::TM ? "tm-" : nullptr;
        type = moveType(game, machine.moveId);
    }
    if (!prefix || !type) return {};
    return std::string(prefix) + typeName(type);
}
} // namespace UI::ClassicMachineArtwork
#endif
