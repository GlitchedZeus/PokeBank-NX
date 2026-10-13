#pragma once
// L4 ROM-script-only NPC trade metadata, NOT an encounter slot table.
// English R/B and Yellow: pret/pokered and pret/pokeyellow data/events/trades.asm.
// Unused rows are present in the ROM but cannot be assumed obtainable.
// Original trainer ID is randomized at exchange and not fixed by these rows.
#include <array>
#include <cstdint>
#include <string_view>
namespace Legality::Gen1OriginalNPCTradeMetadata {
enum class Edition:uint8_t { Unknown, EnglishRedBlue, EnglishYellow };
struct Row {
    uint16_t offered=0,received=0;
    bool reachable=false;
    std::string_view originalNickname{};
};
struct RawNickname {
    bool valid=false;
    std::array<uint8_t,11> bytes{};
};
// The English Gen I A-Z charset maps to 0x80..0x99; 0x50 terminates and
// pads to NAME_LENGTH. Japanese/other regional strings require a different
// independently attested encoding and MUST NOT pass this helper.
constexpr RawNickname encodeEnglishNickname(std::string_view text) noexcept {
    RawNickname result{};
    result.bytes.fill(0x50);
    if (text.empty() || text.size()>10)return result;
    for(size_t i=0;i<text.size();++i) {
        if(text[i]<'A'||text[i]>'Z')return result;
        result.bytes[i]=static_cast<uint8_t>(0x80+(text[i]-'A'));
    }
    result.valid=true;
    return result;
}

inline constexpr std::array<Row,10> kRB{{
 {33,30,true,"TERRY"},  // Nidorino for Nidorina / Terry
 {63,122,true,"MARCEL"}, // Abra for Mr. Mime / Marcel
 {12,15,false,"CHIKUCHIKU"},// unused Butterfree for Beedrill
 {77,86,true,"SAILOR"}, // Ponyta for Seel / Sailor
 {21,83,true,"DUX"}, // Spearow for Farfetch'd / Dux
 {80,108,true,"MARC"},// Slowbro for Lickitung / Marc
 {61,124,true,"LOLA"},// Poliwhirl for Jynx / Lola
 {26,101,true,"DORIS"},// Raichu for Electrode / Doris
 {48,114,true,"CRINKLES"},// Venonat for Tangela / Crinkles
 {32,29,true,"SPOT"}  // Nidoran-M for Nidoran-F / Spot
}};
inline constexpr std::array<Row,10> kY{{
 {108,51,true,"GURIO"}, // Lickitung for Dugtrio / Gurio
 {35,122,true,"MILES"}, // Clefairy for Mr. Mime / Miles
 {12,15,false,"STINGER"}, // unused Butterfree for Beedrill
 {115,89,true,"STICKY"}, // Kangaskhan for Muk / Sticky
 {151,151,false,"BART"}, // unused Mew for Mew / Bart
 {114,47,true,"SPIKE"}, // Tangela for Parasect / Spike
 {18,18,false,"MARTY"}, // unused Pidgeot for Pidgeot / Marty
 {55,112,true,"BUFFY"}, // Golduck for Rhydon / Buffy
 {58,87,true,"CEZANNE"}, // Growlithe for Dewgong / Cezanne
 {104,67,true,"RICKY"} // Cubone for Machoke / Ricky
}};
constexpr Row row(Edition e,uint8_t index) noexcept {
    if(index>=10)return {};
    if(e==Edition::EnglishRedBlue)return kRB[index];
    if(e==Edition::EnglishYellow)return kY[index];
    return {};
}
static_assert(encodeEnglishNickname("TERRY").bytes[0]==0x93);
static_assert(encodeEnglishNickname("CHIKUCHIKU").bytes[10]==0x50);
static_assert(!encodeEnglishNickname("invalid").valid);
static_assert(row(Edition::EnglishRedBlue,1).offered==63);
static_assert(row(Edition::EnglishYellow,4).received==151 && !row(Edition::EnglishYellow,4).reachable);
} // namespace Legality::Gen1OriginalNPCTradeMetadata
