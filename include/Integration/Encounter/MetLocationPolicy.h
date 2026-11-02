#pragma once
// Round 12 (R12-004 / R12-005; owner H14 "gen 3 met locations are wrong pikachu has like 87 met
// locaations we need just the legit ones for each pokemon", H08 "all gens 1-5 need proper met
// locations for all pokemon, evolved,traded gifted"). The ONE policy every Met Location picker
// (Crystal, Gen III, Gen IV, Gen V) follows; each generation builds its rows from this header.
//
// DEFAULT list (what the picker opens on), only rows the data already in the repository supports:
//   1. the record's stored location ("Keep stored" / "row 1 = stored"), never judged (H25: an
//      existing Pokémon's history is flagged, never deleted);
//   2. verified catch / gift / static / egg rows of the exact game for the species AND its
//      pre-evolutions (an evolved Pokémon was met as an ancestor), whose minimum level the Pokémon
//      has reached (R12-005: a Lv. 5 Pidgeotto was never met at Route 229 Lv. 51);
//   3. the exact game's own trade / gift / event / egg markers, labelled as markers ("unverified");
//   4. ONE last row, "Other location (unverified)…", that opens the UNVERIFIED list.
// It is never empty: a species with no verified row there shows its markers and a note saying so.
//
// UNVERIFIED list: every other native location of the exact game, plus the verified rows the
// Pokémon's level has not reached (labelled "level is below this encounter"). Choosing one is an
// explicit choice (Create's required Met Location) and stays labelled unverified; nothing here is
// ever called legal, and nothing is deleted from the data.
//
// Dependency-free so both the Integration layer (Gen IV / V EncounterContext45::Catalogue) and the
// UI pickers (ExactGameMetLocations.h) share it.
#include <cstdint>

namespace PokeVault::Integration::MetLocationPolicy {

// The value of the "Other location (unverified)…" row. Not a location in any Gen II-V table:
// Crystal landmarks are 7-bit, Gen III ids are 8-bit (row values use 0x8000 | id and 0xFFFF for
// keep-stored), Gen IV ids stop at 3076 and Gen V ids at 60003.
inline constexpr uint16_t kUnverifiedListRow = 0xFFFE;

// Short so every row fits the 620 px Gen III-V window and Crystal's 640 px window.
inline constexpr const char* kUnverifiedListRowPlace = "Other location (unverified)\xE2\x80\xA6";
inline constexpr const char* kUnverifiedListRowDetail = "Opens the unverified list";
inline constexpr const char* kUnverifiedListTitle = "Choose Met Location (unverified)";
// Header / caption lines (fit the Gen IV/V header column of 620 - 195 px).
inline constexpr const char* kDefaultListScope = "Verified encounters + markers; row 1 = stored";
inline constexpr const char* kMarkersOnlyScope = "No verified encounter here; markers / unverified only";
inline constexpr const char* kUnverifiedListScope = "Unverified; \"level too low\" = below the encounter";
// R12-005: the reason a verified row the Pokémon's level has not reached moved to the unverified
// list. The row says it in short ("Lv. 51+ via Pidgey; level too low") so it fits beside the place
// name; the editor / Review line and the unverified list's header say it in full.
inline constexpr const char* kLevelBelowEncounter = "level too low";
inline constexpr const char* kLevelBelowEncounterFull = "Level is below this encounter; unverified";

// R12-005 met-level semantics: a Pokémon is never below the level it was met at (evolution keeps
// the level), so an encounter whose minimum level is above the CURRENT level cannot be this
// record's verified encounter. 0 is "level not known" and is not judged.
constexpr bool levelReachesEncounter(uint8_t encounterMinLevel, uint8_t currentLevel) noexcept {
    return currentLevel == 0 || encounterMinLevel <= currentLevel;
}

// Gen IV / V default markers (PKHeX Locations.cs, Names/SpecialMetLocationNotes.h): the in-game /
// link trade and Day-Care / Breeder egg markers a Gen IV / V origin record can carry. Transfer
// markers (Pal Park 55, Poké Transporter 30001, Crown 30010-30013) imply an OLDER origin game, so
// they are not offered by default to a record whose origin is this game; event locations (Gen IV
// 3000-3076, Gen V 40001-49999) are specific distributions and stay in the unverified list, each
// with its own Event label.
constexpr bool gen45DefaultMarker(uint8_t generation, uint16_t location) noexcept {
    if (generation == 4) return location == 2000 || location == 2001 || location == 2002;
    if (generation == 5) return location == 30002 || location == 30003 || location == 60002 || location == 60003;
    return false;
}

} // namespace PokeVault::Integration::MetLocationPolicy
