#pragma once
// Round 13 owner decision 6 ("Support Gen IV Day-Care/trade location markers where native semantics
// and history are verified; distinguish unverified choices"). The Gen IV in-game (NPC) trades whose
// Met Location is the native marker 2001 (PKHeX Locations.LinkTrade4NPC, "Link trade" in
// text_hgss_02000).
//
// Source: kwsch/PKHeX 6501f0ab46e8f8ca048539dbaf8cae8cb104e722
//   Legality/Encounters/Data/Gen4/Encounters4DPPt.cs  TradeGift_DPPtIngame (version group DPPt)
//   Legality/Encounters/Data/Gen4/Encounters4HGSS.cs  TradeGift_HGSS (version group HGSS)
//   Legality/Encounters/Templates/Gen4/EncounterTrade4PID.cs: Location => MetLocation == 0 ?
//     LinkTrade4NPC (2001) : MetLocation, and LevelMin = Level.
// HG/SS's two "gift" entries (Webster's Spearow, Kirk's Shuckle) carry real met locations (183 /
// 130), not 2001, so they are not listed here. My Pokémon Ranch trades use other locations and are
// not 2001 either.
#include <cstdint>

namespace PokeVault::Integration::InGameTrades4 {

struct Trade { bool hgss; uint16_t species; uint8_t level; };

inline constexpr Trade kTrades[] = {
    {false, 63, 1},    // Abra (for Machop)
    {false, 441, 1},   // Chatot (for Buizel)
    {false, 93, 35},   // Haunter (for Medicham)
    {false, 129, 1},   // Magikarp (for Finneon)
    {true, 95, 1},     // Onix (for Bellsprout)
    {true, 66, 1},     // Machop (for Drowzee)
    {true, 100, 1},    // Voltorb (for Krabby)
    {true, 85, 15},    // Dodrio (for Dragonair)
    {true, 82, 19},    // Magneton (for Dugtrio)
    {true, 178, 16},   // Xatu (for Haunter)
    {true, 25, 2},     // Pikachu
    {true, 374, 31},   // Beldum (for Forretress)
    {true, 111, 1},    // Rhyhorn (for Bonsly)
    {true, 208, 1},    // Steelix (for any)
};

inline constexpr uint16_t kMetLocation = 2001;   // PKHeX Locations.LinkTrade4NPC

constexpr bool hgssOrigin(uint8_t origin) noexcept { return origin == 7 || origin == 8; }
constexpr bool dpptOrigin(uint8_t origin) noexcept { return origin == 10 || origin == 11 || origin == 12; }

// The in-game trade of `species` in the record's exact Gen IV origin game, or nullptr.
constexpr const Trade* find(uint8_t origin, uint16_t species) noexcept {
    if (!hgssOrigin(origin) && !dpptOrigin(origin)) return nullptr;
    for (const auto& trade : kTrades)
        if (trade.hgss == hgssOrigin(origin) && trade.species == species) return &trade;
    return nullptr;
}

} // namespace PokeVault::Integration::InGameTrades4
