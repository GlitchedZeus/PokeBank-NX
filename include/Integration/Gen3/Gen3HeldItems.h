#pragma once
// Held-item representation is distinct from Bag availability. PKHeX GetAllHeld
// includes Safari Ball / Berry Juice even though IsLegal filters those from Bag.
// Source: kwsch/PKHeX@6042efa241a9925ce57e645329ea7f976deabdd3
// PKHeX.Core/Items/ItemStorage3RS.cs: GetAllHeld (General, Balls, Berry, MachineOnlyTM).
// https://github.com/kwsch/PKHeX/blob/6042efa241a9925ce57e645329ea7f976deabdd3/PKHeX.Core/Items/ItemStorage3RS.cs
// This is native field support, not evidence of encounter/obtainability legality.
#include <algorithm>
#include <array>
#include <cstdint>
namespace PokeVault::Integration::Gen3 {
inline constexpr std::array<uint16_t, 244> kNativeHeldItems{{
    1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,
    17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,
    33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,
    49,50,51,63,64,65,66,67,68,69,70,71,73,74,75,76,
    77,78,79,80,81,83,84,85,86,93,94,95,96,97,98,103,
    104,106,107,108,109,110,111,121,122,123,124,125,126,127,128,129,
    130,131,132,133,134,135,136,137,138,139,140,141,142,143,144,145,
    146,147,148,149,150,151,152,153,154,155,156,157,158,159,160,161,
    162,163,164,165,166,167,168,169,170,171,172,173,174,175,179,180,
    181,182,183,184,185,186,187,188,189,190,191,192,193,194,195,196,
    197,198,199,200,201,202,203,204,205,206,207,208,209,210,211,212,
    213,214,215,216,217,218,219,220,221,222,223,224,225,254,255,256,
    257,258,289,290,291,292,293,294,295,296,297,298,299,300,301,302,
    303,304,305,306,307,308,309,310,311,312,313,314,315,316,317,318,
    319,320,321,322,323,324,325,326,327,328,329,330,331,332,333,334,
    335,336,337,338,
}};
inline bool nativeHeldItemSupported(uint16_t item) noexcept {
    return item==0 || std::binary_search(kNativeHeldItems.begin(),kNativeHeldItems.end(),item);
}
// Held items that were never released as held items in Generation III.
// Source: kwsch/PKHeX@6501f0ab PKHeX.Core/Items/ItemStorage3RS.cs
// `Unreleased = [005 Safari Ball, 044 Berry Juice]`, and ItemRestrictions
// ReleasedHeldItems_3 = GetPermitList(..., HeldItems_RS, ItemStorage3RS.Unreleased).
// Owner decision (Oct 25 2026): new choices follow PKHeX legality, so these
// are never offered or written. A record that already holds one is read as-is.
inline constexpr std::array<uint16_t, 2> kUnreleasedHeldItems{{5, 44}};
inline bool releasedHeldItem(uint16_t item) noexcept {
    return nativeHeldItemSupported(item) &&
           std::find(kUnreleasedHeldItems.begin(),kUnreleasedHeldItems.end(),item)==kUnreleasedHeldItems.end();
}
}
