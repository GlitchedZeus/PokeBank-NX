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
//
// Round 12 (R12-004 / R12-005, owner H14 / H08): that full list is no longer the DEFAULT. The
// shared policy (Integration/Encounter/MetLocationPolicy.h) splits it: the default list holds the
// stored row, the verified rows of the species AND its pre-evolutions whose level the Pokémon has
// reached, the game's own markers and one "Other location (unverified)..." row; every other
// native location (and each verified row above the Pokémon's level) is in the unverified list.
#include "Integration/Encounter/BreedingPolicy.h"
#include "Integration/Encounter/EncounterContext45.h"
#include "Integration/Encounter/EncounterGuardrails.h"
#include "Integration/Encounter/GiftStaticEncounters.h"
#include "Integration/Encounter/MetLocationPolicy.h"
#include "Names/LocationNames.h"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace PokeBank::UIModel::ExactGameMetLocations {

// Picker row values: catch templates are their index (< 0x8000); an other-native row carries its
// location id with this flag; 0xFFFF stays the Gen III keep-stored row.
inline constexpr uint16_t kOtherNativeRowFlag = 0x8000;
inline constexpr uint16_t kKeepStoredRowValue = 0xFFFF;
// Round 12: the default list's last row, "Other location (unverified)...", which opens the
// unverified list. Not a location and not an other-native row.
inline constexpr uint16_t kUnverifiedListRowValue = PokeVault::Integration::MetLocationPolicy::kUnverifiedListRow;

constexpr uint16_t otherNativeRow(uint16_t location) noexcept {
    return static_cast<uint16_t>(kOtherNativeRowFlag | (location & 0x7FFF));
}
// Round 13 (owner decision 5): "Hatched from a Day-Care egg" (MetLocationPolicy::kHatchedEggRow).
inline constexpr uint16_t kHatchedEggRowValue = PokeVault::Integration::MetLocationPolicy::kHatchedEggRow;
constexpr bool isOtherNativeRow(uint16_t row) noexcept {
    return row != kKeepStoredRowValue && row != kUnverifiedListRowValue && row != kHatchedEggRowValue &&
           (row & kOtherNativeRowFlag) != 0;
}
static_assert(!isOtherNativeRow(kUnverifiedListRowValue) && !isOtherNativeRow(kKeepStoredRowValue) &&
              !isOtherNativeRow(kHatchedEggRowValue));
constexpr uint16_t otherNativeRowLocation(uint16_t row) noexcept {
    return static_cast<uint16_t>(row & 0x7FFF);
}

// Honest detail text for an other-native row: a place where this species is not a listed catch
// in this game, or one of the game's own history markers. Short so every row fits the 620 px
// Gen III window and Crystal's 640 px window (tests/test_met_location_fit.cpp).
inline constexpr const char* kOtherPlaceDetail = "Not a listed catch; unverified";
inline constexpr const char* kTradeMarkerDetail = "Trade marker; unverified";
inline constexpr const char* kEventMarkerDetail = "Event marker; unverified";
inline constexpr const char* kEggMarkerDetail = "Gift-egg marker; unverified";
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

// The game's own history markers (Gen III gift egg 253, in-game trade 254, fateful encounter
// 255), labelled as markers.
constexpr bool gen3MarkerLocation(uint16_t location) noexcept { return location >= 253 && location <= 255; }
// Round 13 (owner decision 5, "no generic egg histories presented as verified"): the DEFAULT list
// offers only the trade / fateful markers. 253 is the Met Location of an UNHATCHED gift egg
// (pokeemerald METLOC_SPECIAL_EGG; replaced by the hatch place when it hatches), never a bred
// egg's: the verified gift-egg templates (Emerald Wynaut, FR/LG Togepi) are template rows, a bred
// egg is the "Hatched from a Day-Care egg" row, and the bare marker is in the unverified list.
constexpr bool gen3DefaultMarker(uint16_t location) noexcept { return location == 254 || location == 255; }

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

// Crystal's own markers (Gift 0x7E, Event 0x7F): offered in the default list, labelled as markers.
constexpr bool crystalMarkerLocation(uint16_t location) noexcept { return location == 0x7E || location == 0x7F; }

constexpr const char* crystalOtherDetail(uint16_t location) noexcept {
    return location == 0x7E ? kGiftMarkerDetail : location == 0x7F ? kEventMarkerDetail : kOtherPlaceDetail;
}

// ---- Shared composition ----

// Round 12 (owner H08 "evolved"): the exact game's verified catch / gift / static / egg templates
// for the species, then for each of its pre-evolutions (an evolved Pokémon was met as an ancestor).
// Round 13 (owner decision 7): the gift / static / in-game trade / fossil / prize / roaming / event
// rows come from the source-backed table (Integration/Encounter/GiftStaticEncounters.h), each with
// its kind as the method ("Gift Lv. 25", "Trade Lv. 5+"), level-checked like a wild row.
// Ancestry comes from the EncounterContext45 Gen IV evolution edges, which cover every Gen I-III
// family; a member above `maxSpecies` (a later-generation baby) is not of this game. A row says the
// species it was met as (originalEncounterSpecies); the Gen III Horsea edge keeps its level gate.
inline std::vector<PokeVault::Integration::EncounterGuardrails::EncounterProvenanceChoice>
templatesWithAncestry(std::string_view gameId, uint16_t species, uint16_t maxSpecies) {
    namespace Encounter = PokeVault::Integration::EncounterGuardrails;
    std::vector<Encounter::EncounterProvenanceChoice> out;
    if (gameId.empty() || species == 0 || species > maxSpecies) return out;
    namespace Gifts = PokeVault::Integration::GiftStaticEncounters;
    for (const auto& encounter : Gifts::metTemplates(gameId, species))
        out.push_back({encounter, species, species, 0, false});
    const auto family = PokeVault::Integration::EncounterContext45::ancestry(4, species);
    const auto edge = Encounter::gen3EvolutionParent(species);
    for (uint16_t member = 1; member <= maxSpecies; ++member) {
        if (member == species || !family[member]) continue;
        const uint8_t gate = edge && edge->originSpecies == member ? edge->minimumEvolutionLevel : 0;
        for (const auto& encounter : Gifts::metTemplates(gameId, member))
            out.push_back({encounter, species, member, gate, true});
    }
    return out;
}

// Crystal's plain-template view of it (species + pre-evolutions, 1-251).
inline std::vector<PokeVault::Integration::EncounterGuardrails::EncounterTemplate>
crystalTemplatesWithAncestry(std::string_view gameId, uint16_t species) {
    std::vector<PokeVault::Integration::EncounterGuardrails::EncounterTemplate> out;
    for (const auto& choice : templatesWithAncestry(gameId, species, 251)) out.push_back(choice.encounter);
    return out;
}

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

// Round 12 (R12-004 / R12-005): the Crystal editor / Review line for recorded caught data. "" when
// the landmark and level code are a verified Crystal template of the species or a pre-evolution
// that the current level reaches; otherwise an honest unverified label (markers name themselves).
// Round 13 (owner decisions 7 / 8): a gift / trade at LANDMARK_GIFT (0x7E) is verified with the
// caught level and time Crystal writes for it (pret SetGiftMonCaughtData: level 0, time 0), and a
// recorded time of day (1 morning, 2 day, 3 night; 0 = not judged) must be one the slot occurs at.
inline std::string crystalUnverifiedNote(std::string_view gameId, uint16_t species, uint8_t location,
                                         uint8_t levelCode, uint8_t level, uint8_t timeOfDay = 0) {
    namespace Encounter = PokeVault::Integration::EncounterGuardrails;
    namespace Policy = PokeVault::Integration::MetLocationPolicy;
    namespace Gifts = PokeVault::Integration::GiftStaticEncounters;
    bool templateHere = false, otherTime = false;
    for (const auto& encounter : crystalTemplatesWithAncestry(gameId, species)) {
        if (encounter.location != location) continue;
        templateHere = true;
        const auto* gift = Gifts::rowOf(encounter);
        const bool unrecorded = gift && (gift->flags & Gifts::kCaughtLevelUnrecorded);
        const bool levelFits = unrecorded ? levelCode == 0
                             : levelCode == 1 ? encounter.method == Encounter::Method::Egg
                                              : levelCode >= 2 && encounter.containsLevel(levelCode);
        const bool timeFits = unrecorded ? timeOfDay == 0
                                         : timeOfDay == 0 || Encounter::timeAllowed(encounter, timeOfDay);
        if (!levelFits || !Policy::levelReachesEncounter(encounter.minLevel, level)) continue;
        if (timeFits) return {};
        otherTime = true;
    }
    if (crystalMarkerLocation(location)) return crystalOtherDetail(location);
    if (otherTime) return "Not a listed catch at this time of day; unverified";
    return templateHere ? "Not a listed catch at this level; unverified" : kOtherPlaceDetail;
}

} // namespace PokeBank::UIModel::ExactGameMetLocations
