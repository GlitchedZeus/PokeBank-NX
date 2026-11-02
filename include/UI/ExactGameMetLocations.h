#pragma once
// Round 11 FRLG (owner: "leaf green and fire red on gba so gen 3 dont have a working met location
// for pokemon ... i dont think i can get a rayquaza in leaf green or fire red"). The Gen III and
// Crystal Met Location pickers were closed lists of the exact game's catch templates and refused
// to open when a species had none: every evolved species without its own wild slot (Charizard,
// Raichu, Dragonite) and every traded-in species (Rayquaza in FireRed / LeafGreen). A traded
// Pokémon is legitimately in any save, and the species list is the whole National Dex, so the
// species is never restricted here.
//
// The shared rule both pickers now follow (the Gen IV / V Catalogue already works this way):
//   1. the record's stored location first ("Keep stored", R2-BUG-008), never judged;
//   2. the exact game's catch templates for the species (native catches: "Wild Lv. 3–5");
//   3. every other location native to the record's own origin game, labelled as unverified
//      trade / gift / hatch history, never as a catch. Markers (in-game trade, gift, event) say
//      what they are.
// Only the record's own origin game is used: a FireRed / LeafGreen record is offered Kanto and
// Sevii ids (Names::isGen3NativeMetLocation), never the Hoenn list, and the reverse for R/S/E.
#include "Names/LocationNames.h"

#include <cstdint>
#include <string_view>
#include <vector>

namespace PokeBank::UIModel::ExactGameMetLocations {

// Picker row values: catch templates are their index (< 0x8000); an other-native row carries its
// location id with this flag; 0xFFFF stays the Gen III keep-stored row.
inline constexpr uint16_t kOtherNativeRowFlag = 0x8000;
inline constexpr uint16_t kKeepStoredRowValue = 0xFFFF;

constexpr uint16_t otherNativeRow(uint16_t location) noexcept {
    return static_cast<uint16_t>(kOtherNativeRowFlag | (location & 0x7FFF));
}
constexpr bool isOtherNativeRow(uint16_t row) noexcept {
    return row != kKeepStoredRowValue && (row & kOtherNativeRowFlag) != 0;
}
constexpr uint16_t otherNativeRowLocation(uint16_t row) noexcept {
    return static_cast<uint16_t>(row & 0x7FFF);
}

// Honest detail text for an other-native row: a place where this species is not a listed catch
// in this game, or one of the game's own history markers. Short so every row fits the 620 px
// Gen III window and Crystal's 640 px window (tests/test_met_location_fit.cpp).
inline constexpr const char* kOtherPlaceDetail = "Not a listed catch; unverified";
inline constexpr const char* kTradeMarkerDetail = "Trade marker; unverified";
inline constexpr const char* kEventMarkerDetail = "Event marker; unverified";
inline constexpr const char* kEggMarkerDetail = "Egg marker; unverified";
inline constexpr const char* kGiftMarkerDetail = "Gift marker; unverified";

// ---- Generation III (PK3 origin 1 Sapphire, 2 Ruby, 3 Emerald, 4 FireRed, 5 LeafGreen) ----

constexpr std::string_view gen3GameIdForOrigin(uint8_t origin) noexcept {
    switch (origin) {
        case 1: return "sapphire_gba";
        case 2: return "ruby_gba";
        case 3: return "emerald_gba";
        case 4: return "firered_gba";
        case 5: return "leafgreen_gba";
        default: return {};
    }
}

// What the picker offers: the native ids of the origin game, minus 0 (shown as "None" and used as
// "not chosen" by every Gen III editor) and PKHeX's three "(unused)" duplicate Hoenn map sections
// (64 Meteor Falls, 75 Fiery Path, 77 Jagged Pass), which display the same name as the real one.
// A stored value outside this list is still kept through the keep-stored row.
constexpr bool gen3PickerOffers(uint8_t origin, uint16_t location) noexcept {
    return location != 0 && location != 64 && location != 75 && location != 77 &&
           Names::isGen3NativeMetLocation(origin, location);
}

inline std::vector<uint16_t> gen3OfferedLocations(uint8_t origin) {
    std::vector<uint16_t> out;
    for (uint16_t id = 1; id <= 255; ++id)
        if (gen3PickerOffers(origin, id)) out.push_back(id);
    return out;
}

constexpr const char* gen3OtherDetail(uint16_t location) noexcept {
    switch (location) {
        case 253: return kEggMarkerDetail;
        case 254: return kTradeMarkerDetail;
        case 255: return kEventMarkerDetail;
        default: return kOtherPlaceDetail;
    }
}

// ---- Crystal caught data (7-bit landmark; pret/pokecrystal landmark_constants.asm) ----

// 1..0x5F are the retail landmarks (0 is "Unknown"), 0x7E Gift and 0x7F Event are the game's
// markers. 0x60..0x7D are unused.
constexpr bool crystalNativeCaughtLocation(uint16_t location) noexcept {
    return (location >= 1 && location <= 0x5F) || location == 0x7E || location == 0x7F;
}

inline std::vector<uint16_t> crystalOfferedLocations() {
    std::vector<uint16_t> out;
    for (uint16_t id = 1; id <= 0x7F; ++id)
        if (crystalNativeCaughtLocation(id)) out.push_back(id);
    return out;
}

constexpr const char* crystalOtherDetail(uint16_t location) noexcept {
    return location == 0x7E ? kGiftMarkerDetail : location == 0x7F ? kEventMarkerDetail : kOtherPlaceDetail;
}

// ---- Shared composition ----

// The other-native rows: every offered id that is neither a template location (already listed as a
// catch) nor `skip` (the stored location when it has its own keep-stored row).
template <typename IsTemplateLocation>
inline std::vector<uint16_t> otherNativeRows(const std::vector<uint16_t>& offered,
                                             IsTemplateLocation isTemplateLocation,
                                             bool skipStored, uint16_t stored) {
    std::vector<uint16_t> rows;
    rows.reserve(offered.size());
    for (const auto id : offered) {
        if (skipStored && id == stored) continue;
        if (isTemplateLocation(id)) continue;
        rows.push_back(otherNativeRow(id));
    }
    return rows;
}

} // namespace PokeBank::UIModel::ExactGameMetLocations
