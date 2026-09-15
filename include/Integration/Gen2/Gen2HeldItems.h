#pragma once
#include <array>
#include <cstdint>
namespace PokeVault::Integration::Gen2 {
// PKHeX ItemStorage2.GetAllHeld: General + Balls + first 50 Machine (TMs).
// Pinned 77dcd3a7895bceaafbbff12d25bdf77c1acd8ca5; see docs/GEN2_AUDIT_RESEARCH.md.
inline constexpr std::array<uint8_t, 192> kGen2HeldItems{
    1, 2, 3, 4, 5, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18,
    19, 20, 21, 22, 23, 24, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35,
    36, 37, 38, 39, 40, 41, 42, 43, 44, 46, 47, 48, 49, 51, 52, 53,
    57, 60, 62, 63, 64, 65, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81,
    82, 83, 84, 85, 86, 87, 88, 89, 91, 92, 93, 94, 95, 96, 97, 98,
    99, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 117,
    118, 119, 121, 122, 123, 124, 125, 126, 131, 132, 138, 139, 140, 143, 144, 146,
    150, 151, 152, 156, 157, 158, 159, 160, 161, 163, 164, 165, 166, 167, 168, 169,
    170, 172, 173, 174, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 191, 192,
    193, 194, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207, 208, 209,
    210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 221, 222, 223, 224, 225, 226,
    227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239, 240, 241, 242,
};
constexpr bool selectableHeldItem(uint8_t item) noexcept {
    if (item == 0) return true;
    for (auto candidate : kGen2HeldItems) if (candidate == item) return true;
    return false;
}
} // namespace PokeVault::Integration::Gen2
