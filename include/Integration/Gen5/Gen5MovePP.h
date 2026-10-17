#ifndef POKEBANK_GEN5_MOVE_PP_H
#define POKEBANK_GEN5_MOVE_PP_H

// Exact native Generation V move PP values, moves 0..559.
// Transcribed programmatically from PKHeX.Core/Moves/MoveInfo5.cs, pinned
// PKHeX git tree 66ef5983a5d349efa2dd615f2926a1b1ee97306e.
// Move PP is game-version-specific: NEVER fall back to Gen IV or modern PP.
// This table does not establish species compatibility / event legality.
#include <array>
#include <cstddef>
#include <cstdint>

namespace PokeVault::Integration::Gen5 {
inline constexpr uint16_t MaxNativeMove5 = 559;
inline constexpr std::array<uint8_t,560> NativeMovePP5{{
         0, 35, 25, 10, 15, 20, 20, 15, 15, 15, 35, 30,  5, 10, 30, 30, 35, 35, 20, 15,
        20, 20, 15, 20, 30,  5, 10, 15, 15, 15, 25, 20,  5, 35, 15, 20, 20, 10, 15, 30,
        35, 20, 20, 30, 25, 40, 20, 15, 20, 20, 20, 30, 25, 15, 30, 25,  5, 15, 10,  5,
        20, 20, 20,  5, 35, 20, 25, 20, 20, 20, 15, 25, 15, 10, 40, 25, 10, 35, 30, 15,
        10, 40, 10, 15, 30, 15, 20, 10, 15, 10,  5, 10, 10, 25, 10, 20, 40, 30, 30, 20,
        20, 15, 10, 40, 15, 10, 30, 20, 20, 10, 40, 40, 30, 30, 30, 20, 30, 10, 10, 20,
         5, 10, 30, 20, 20, 20,  5, 15, 15, 20, 15, 15, 35, 20, 15, 10, 10, 30, 15, 40,
        20, 15, 10,  5, 10, 30, 10, 15, 20, 15, 40, 40, 10,  5, 15, 10, 10, 10, 15, 30,
        30, 10, 10, 20, 10,  1,  1, 10, 10, 10,  5, 15, 25, 15, 10, 15, 30,  5, 40, 15,
        10, 25, 10, 30, 10, 20, 10, 10, 10, 10, 10, 20,  5, 40,  5,  5, 15,  5, 10,  5,
        10, 10, 10, 10, 20, 20, 40, 15, 10, 20, 20, 25,  5, 15, 10,  5, 20, 15, 20, 25,
        20,  5, 30,  5, 10, 20, 40,  5, 20, 40, 20, 15, 35, 10,  5,  5,  5, 15,  5, 20,
         5,  5, 15, 20, 10,  5,  5, 15, 10, 15, 15, 10, 10, 10, 20, 10, 10, 10, 10, 15,
        15, 15, 10, 20, 20, 10, 20, 20, 20, 20, 20, 10, 10, 10, 20, 20,  5, 15, 10, 10,
        15, 10, 20,  5,  5, 10, 10, 20,  5, 10, 20, 10, 20, 20, 20,  5,  5, 15, 20, 10,
        15, 20, 15, 10, 10, 15, 10,  5,  5, 10, 15, 10,  5, 20, 25,  5, 40, 10,  5, 40,
        15, 20, 20,  5, 15, 20, 30, 15, 15,  5, 10, 30, 20, 30, 15,  5, 40, 15,  5, 20,
         5, 15, 25, 40, 15, 20, 15, 20, 15, 20, 10, 20, 20,  5,  5, 10,  5, 40, 10, 10,
         5, 10, 10, 15, 10, 20, 30, 30, 10, 20,  5, 10, 10, 15, 10, 10,  5, 15,  5, 10,
        10, 30, 20, 20, 10, 10,  5,  5, 10,  5, 20, 10, 20, 10, 15, 10, 20, 20, 20, 15,
        15, 10, 15, 20, 15, 10, 10, 10, 20, 10, 30,  5, 10, 15, 10, 10,  5, 20, 30, 10,
        30, 15, 15, 15, 15, 30, 10, 20, 15, 10, 10, 20, 15,  5,  5, 15, 15,  5, 10,  5,
        20,  5, 15, 20,  5, 20, 20, 20, 20, 10, 20, 10, 15, 20, 15, 10, 10,  5, 10,  5,
         5, 10,  5,  5, 10,  5,  5,  5, 15, 10, 10, 10, 10, 10, 10, 15, 20, 15, 10, 15,
        10, 15, 10, 20, 10, 15, 10, 20, 20, 20, 20, 20, 15, 15, 15, 15, 15, 15, 20, 15,
        10, 15, 15, 15, 15, 10, 10, 10, 10, 10, 15, 15, 15, 15,  5,  5, 15,  5, 10, 10,
        10, 20, 20, 20, 10, 10, 30, 15, 15, 10, 15, 25, 10, 20, 10, 10, 10, 20, 10, 10,
        10, 10, 10, 15, 15,  5,  5, 10, 10, 10,  5,  5, 10,  5,  5, 15, 10,  5,  5,  5,
}};
static_assert(NativeMovePP5.size() == static_cast<size_t>(MaxNativeMove5)+1);
constexpr bool validNativeMove5(uint32_t id) noexcept {
    return id<=MaxNativeMove5;
}
constexpr uint8_t basePP5(uint32_t id) noexcept {
    return validNativeMove5(id)?NativeMovePP5[static_cast<size_t>(id)]:0;
}
constexpr uint8_t maxPP5(uint32_t id,uint32_t ppUps) noexcept {
    return validNativeMove5(id)&&ppUps<=3
        ? static_cast<uint8_t>((uint32_t(basePP5(id))*(5u+ppUps))/5u)
        : 0;
}
static_assert(basePP5(0)==0 && basePP5(1)==35 && basePP5(85)==15);
static_assert(basePP5(559)==5 && maxPP5(1,3)==56);
} // namespace PokeVault::Integration::Gen5
#endif
