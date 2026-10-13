#pragma once
// L4 ROM-script-only NPC trade metadata, NOT an encounter slot table.
// English R/B and Yellow: pret/pokered and pret/pokeyellow data/events/trades.asm.
// Unused rows are present in the ROM but cannot be assumed obtainable.
// Original trainer ID is randomized at exchange and not fixed by these rows.
#include <array>
#include <cstdint>
namespace Legality::Gen1OriginalNPCTradeMetadata {
enum class Edition:uint8_t { Unknown, EnglishRedBlue, EnglishYellow };
struct Row {uint16_t offered=0,received=0;bool reachable=false;};
inline constexpr std::array<Row,10> kRB{{
 {33,30,true},  // Nidorino for Nidorina / Terry
 {63,122,true}, // Abra for Mr. Mime / Marcel
 {12,15,false},// unused Butterfree for Beedrill
 {77,86,true}, // Ponyta for Seel / Sailor
 {21,83,true}, // Spearow for Farfetch'd / Dux
 {80,108,true},// Slowbro for Lickitung / Marc
 {61,124,true},// Poliwhirl for Jynx / Lola
 {26,101,true},// Raichu for Electrode / Doris
 {48,114,true},// Venonat for Tangela / Crinkles
 {32,29,true}  // Nidoran-M for Nidoran-F / Spot
}};
inline constexpr std::array<Row,10> kY{{
 {108,51,true}, // Lickitung for Dugtrio / Gurio
 {35,122,true}, // Clefairy for Mr. Mime / Miles
 {12,15,false}, // unused Butterfree for Beedrill
 {115,89,true}, // Kangaskhan for Muk / Sticky
 {151,151,false}, // unused Mew for Mew / Bart
 {114,47,true}, // Tangela for Parasect / Spike
 {18,18,false}, // unused Pidgeot for Pidgeot / Marty
 {55,112,true}, // Golduck for Rhydon / Buffy
 {58,87,true}, // Growlithe for Dewgong / Cezanne
 {104,67,true} // Cubone for Machoke / Ricky
}};
constexpr Row row(Edition e,uint8_t index) noexcept {
    if(index>=10)return {};
    if(e==Edition::EnglishRedBlue)return kRB[index];
    if(e==Edition::EnglishYellow)return kY[index];
    return {};
}
static_assert(row(Edition::EnglishRedBlue,1).offered==63);
static_assert(row(Edition::EnglishYellow,4).received==151 && !row(Edition::EnglishYellow,4).reachable);
} // namespace Legality::Gen1OriginalNPCTradeMetadata
