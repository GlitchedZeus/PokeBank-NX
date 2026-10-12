#pragma once
// L3 M06/M13: curated Kanto starter evolution source-level offers (RSE/FRLG).
// Exact pinned retail source rows (no imported whole-species data table):
// pret/pokeruby@63a8cbf0016b351a4e68f7036fa0b77e23d2f2c1
// pret/pokeemerald@5eff78649e7170a877b961ef0b3da13b81a16038
// pret/pokefirered@c75f352304d529f6ba92d4f74b9cf8b5c3810788
//   src/data/pokemon/level_up_learnsets.h and include/constants/moves.h
// Only source offers, never evidence a particular specimen actually learned.
#include <array>
#include <cstdint>
namespace Legality::Gen14CandidateMoveGbaStarterTables {
struct Offer { uint8_t level; uint16_t move; };
inline constexpr std::array<Offer,13> kIvysaur{{
    {1,33},{1,45},{1,73},{4,45},{7,73},{10,22},
    {15,77},{15,79},{22,75},{29,230},{38,74},{47,235},{56,76}
}};
inline constexpr std::array<Offer,14> kVenusaur{{
    {1,33},{1,45},{1,73},{1,22},{4,45},{7,73},{10,22},
    {15,77},{15,79},{22,75},{29,230},{41,74},{53,235},{65,76}
}};
inline constexpr std::array<Offer,11> kCharmeleonRse{{
    {1,10},{1,45},{1,52},{7,52},{13,108},{20,99},
    {27,184},{34,53},{41,163},{48,82},{55,83}
}};
inline constexpr std::array<Offer,11> kCharmeleonFrlg{{
    {1,10},{1,45},{1,52},{7,52},{13,232},{20,108},
    {27,184},{34,53},{41,163},{48,82},{55,83}
}};
inline constexpr std::array<Offer,13> kCharizardRse{{
    {1,10},{1,45},{1,52},{1,108},{7,52},{13,108},
    {20,99},{27,184},{34,53},{36,17},{44,163},{54,82},{64,83}
}};
inline constexpr std::array<Offer,14> kCharizardFrlg{{
    {1,257},{1,10},{1,45},{1,52},{1,232},{7,52},{13,232},
    {20,108},{27,184},{34,53},{36,17},{44,163},{54,82},{64,83}
}};
inline constexpr std::array<Offer,11> kSquirtle{{
    {1,33},{4,39},{7,145},{10,110},{13,55},{18,44},
    {23,229},{28,182},{33,240},{40,130},{47,56}
}};
inline constexpr std::array<Offer,13> kWartortle{{
    {1,33},{1,39},{1,145},{4,39},{7,145},{10,110},
    {13,55},{19,44},{25,229},{31,182},{37,240},{45,130},
    {53,56}
}};
inline constexpr std::array<Offer,14> kBlastoise{{
    {1,33},{1,39},{1,145},{1,110},{4,39},{7,145},
    {10,110},{13,55},{19,44},{25,229},{31,182},
    {42,240},{55,130},{68,56}
}};
} // namespace Legality::Gen14CandidateMoveGbaStarterTables
