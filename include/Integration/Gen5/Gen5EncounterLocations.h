#ifndef POKEBANK_GEN5_ENCOUNTER_LOCATIONS_H
#define POKEBANK_GEN5_ENCOUNTER_LOCATIONS_H

// Native Generation V encounter-location picker candidate IDs, pinned to
// PKHeX.Core/Game/Locations/Locations5.cs, PKHeX tree
// 66ef5983a5d349efa2dd615f2926a1b1ee97306e.
// This is a list of actual PK5 location IDs (including trade/event banks),
// NOT proof that any particular species can originate at that location.
// Existing unusual/source-provenanced met locations are preserved on open.
#include <array>
#include <cstddef>
#include <cstdint>
namespace PokeVault::Integration::Gen5 {
inline constexpr std::array<uint16_t,276> NativeMetLocations5{{
        0,     1,     2,     4,     5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
       15,    16,    17,    18,    19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
       29,    30,    31,    32,    33,    34,    35,    36,    37,    38,    39,    40,    41,    42,
       43,    44,    45,    46,    47,    48,    49,    50,    51,    52,    53,    54,    55,    56,
       57,    58,    59,    60,    61,    62,    63,    64,    65,    66,    67,    68,    69,    70,
       71,    72,    73,    74,    75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
       85,    86,    87,    88,    89,    90,    91,    92,    93,    94,    95,    96,    97,    98,
       99,   100,   101,   102,   103,   104,   105,   106,   107,   108,   109,   110,   111,   112,
      113,   114,   115,   116,   117,   118,   119,   120,   121,   122,   123,   124,   125,   126,
      127,   128,   129,   130,   131,   132,   133,   134,   135,   136,   137,   139,   140,   141,
      142,   143,   144,   145,   146,   147,   148,   149,   150,   151,   152,   153, 30001, 30002,
    30004, 30005, 30006, 30007, 30008, 30010, 30011, 30012, 30013, 30014, 30015, 40001, 40002, 40003,
    40004, 40005, 40006, 40007, 40008, 40009, 40010, 40011, 40012, 40013, 40014, 40015, 40016, 40017,
    40018, 40019, 40020, 40021, 40022, 40023, 40024, 40025, 40026, 40027, 40028, 40029, 40030, 40031,
    40032, 40033, 40034, 40035, 40036, 40037, 40038, 40039, 40040, 40041, 40042, 40043, 40044, 40045,
    40046, 40047, 40048, 40049, 40050, 40051, 40052, 40053, 40054, 40055, 40056, 40057, 40058, 40059,
    40060, 40061, 40062, 40063, 40064, 40065, 40066, 40067, 40068, 40069, 40070, 40071, 40072, 40073,
    40074, 40075, 40076, 40077, 40078, 40079, 40080, 40081, 40082, 40083, 40084, 40085, 40086, 40087,
    40088, 40089, 40090, 40091, 40092, 40093, 40094, 40095, 40096, 40097, 40098, 40099, 40100, 40101,
    40102, 40103, 40104, 40105, 40106, 40107, 40108, 40109, 60001, 60003,
}};
constexpr bool isNativeMetLocation5(uint32_t id) noexcept {
    for(const auto candidate:NativeMetLocations5)
        if(id==candidate)return true;
    return false;
}
constexpr size_t locationIndex5(uint32_t id) noexcept {
    for(size_t i=0;i<NativeMetLocations5.size();++i)
        if(id==NativeMetLocations5[i])return i;
    return NativeMetLocations5.size();
}
static_assert(isNativeMetLocation5(0) && isNativeMetLocation5(1));
static_assert(!isNativeMetLocation5(3) && !isNativeMetLocation5(138));
static_assert(isNativeMetLocation5(153));
static_assert(isNativeMetLocation5(30001) && isNativeMetLocation5(40001));
static_assert(isNativeMetLocation5(60003) && !isNativeMetLocation5(65000));
} // namespace PokeVault::Integration::Gen5
#endif
