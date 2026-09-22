#pragma once

#include "Integration/Gen2/Gen2ReadOnlySave.h"

#include <array>
#include <cstdint>
#include <optional>
#include <string>

namespace PokeBank::UIModel::Gen2Native {

// Crystal stores these four values in two bytes at PK2 offset 0x1D. The bit layout is
// independently mirrored by PKHeX PK2 and pret/pokecrystal's SetCaughtData/GetCaught* routines.
// Keep the raw uint16_t in PokemonRecord as the authoritative bytes; this type is presentation only.
struct CrystalCaughtData {
    bool present = false;
    uint8_t timeOfDay = 0;   // 0 unknown, 1 morning, 2 day, 3 night
    uint8_t levelCode = 0;   // 0 unknown, 1 egg marker, otherwise native level 2..63
    bool originalTrainerFemale = false;
    uint8_t location = 0;    // 7-bit Crystal landmark index
};

constexpr CrystalCaughtData decodeCrystalCaughtData(uint16_t raw) noexcept {
    return {
        raw != 0,
        static_cast<uint8_t>((raw >> 14) & 0x03),
        static_cast<uint8_t>((raw >> 8) & 0x3F),
        ((raw >> 7) & 0x01) != 0,
        static_cast<uint8_t>(raw & 0x7F),
    };
}

constexpr uint16_t encodeCrystalCaughtData(const CrystalCaughtData& caught) noexcept {
    return static_cast<uint16_t>(((caught.timeOfDay & 3) << 14) |
        ((caught.levelCode & 63) << 8) | (caught.originalTrainerFemale ? 0x80 : 0) |
        (caught.location & 127));
}

constexpr const char* crystalMetTimeName(uint8_t code) noexcept {
    switch (code & 0x03) {
        case 1: return "Morning";
        case 2: return "Day";
        case 3: return "Night";
        default: return "Unknown";
    }
}

inline std::string crystalCaughtLevelText(const CrystalCaughtData& caught) {
    if (!caught.present || caught.levelCode == 0) return "Unknown";
    if (caught.levelCode == 1) return "Egg marker";
    return "Lv " + std::to_string(caught.levelCode);
}

constexpr const char* crystalOriginalTrainerGenderText(const CrystalCaughtData& caught) noexcept {
    return !caught.present ? "Unknown" : (caught.originalTrainerFemale ? "Female" : "Male");
}

enum class CaughtLevelLegality : uint8_t {
    NotRecorded,
    UnknownMetLevel,
    EggMarker,
    Valid,
    CurrentBelowMet,
};

constexpr CaughtLevelLegality caughtLevelLegality(
    uint8_t currentLevel, const CrystalCaughtData& caught) noexcept {
    if (!caught.present) return CaughtLevelLegality::NotRecorded;
    if (caught.levelCode == 0) return CaughtLevelLegality::UnknownMetLevel;
    if (caught.levelCode == 1) return CaughtLevelLegality::EggMarker;
    return currentLevel < caught.levelCode ? CaughtLevelLegality::CurrentBelowMet
                                           : CaughtLevelLegality::Valid;
}

constexpr const char* caughtLevelLegalityText(CaughtLevelLegality status) noexcept {
    switch (status) {
        case CaughtLevelLegality::NotRecorded: return "Not recorded";
        case CaughtLevelLegality::UnknownMetLevel: return "Met level unknown";
        case CaughtLevelLegality::EggMarker: return "Egg marker";
        case CaughtLevelLegality::Valid: return "Level history OK";
        case CaughtLevelLegality::CurrentBelowMet: return "Current level below met level";
    }
    return "Not checked";
}

// Exact retail Crystal landmark order from pret/pokecrystal landmark_constants.asm + landmarks.asm.
inline constexpr std::array<const char*, 0x60> crystalLandmarkNames{{
    "Unknown", "New Bark Town", "Route 29", "Cherrygrove City", "Route 30", "Route 31",
    "Violet City", "Sprout Tower", "Route 32", "Ruins of Alph", "Union Cave", "Route 33",
    "Azalea Town", "Slowpoke Well", "Ilex Forest", "Route 34", "Goldenrod City", "Radio Tower",
    "Route 35", "National Park", "Route 36", "Route 37", "Ecruteak City", "Tin Tower",
    "Burned Tower", "Route 38", "Route 39", "Olivine City", "Lighthouse", "Battle Tower",
    "Route 40", "Whirl Islands", "Route 41", "Cianwood City", "Route 42", "Mt. Mortar",
    "Mahogany Town", "Route 43", "Lake of Rage", "Route 44", "Ice Path", "Blackthorn City",
    "Dragon's Den", "Route 45", "Dark Cave", "Route 46", "Silver Cave", "Pallet Town",
    "Route 1", "Viridian City", "Route 2", "Pewter City", "Route 3", "Mt. Moon", "Route 4",
    "Cerulean City", "Route 24", "Route 25", "Route 5", "Underground", "Route 6",
    "Vermilion City", "Diglett's Cave", "Route 7", "Route 8", "Route 9", "Rock Tunnel",
    "Route 10", "Power Plant", "Lavender Town", "Lav Radio Tower", "Celadon City", "Saffron City",
    "Route 11", "Route 12", "Route 13", "Route 14", "Route 15", "Route 16", "Route 17",
    "Route 18", "Fuchsia City", "Route 19", "Route 20", "Seafoam Islands", "Cinnabar Island",
    "Route 21", "Route 22", "Victory Road", "Route 23", "Indigo Plateau", "Route 26", "Route 27",
    "Tohjo Falls", "Route 28", "Fast Ship",
}};

constexpr const char* crystalCaughtLocationName(uint8_t location) noexcept {
    if (location < crystalLandmarkNames.size()) return crystalLandmarkNames[location];
    if (location == 0x7E) return "Gift";
    if (location == 0x7F) return "Event";
    return "Unused / invalid";
}

// Generation II party status is a byte: low three bits are remaining sleep turns, then
// poison/burn/freeze/paralysis bits. Retail UI handles fainting from HP before the status byte.
inline std::string partyStatusText(uint8_t status, uint16_t currentHP) {
    if (currentHP == 0) return "Fainted";
    if (status & 0x08) return "Poisoned";
    if (status & 0x10) return "Burned";
    if (status & 0x20) return "Frozen";
    if (status & 0x40) return "Paralyzed";
    const uint8_t sleepTurns = status & 0x07;
    if (sleepTurns != 0) return "Asleep (" + std::to_string(sleepTurns) + ")";
    return "OK";
}

struct PartyViewData {
    uint16_t currentHP = 0;
    uint16_t maxHP = 0;
    uint8_t rawStatus = 0;
    std::string statusText;
};

inline std::optional<PartyViewData> partyViewData(
    const PokeVault::Integration::Gen2::PokemonRecord& record) {
    if (!record.partyRecord) return std::nullopt;
    return PartyViewData{record.currentHP, record.maxHP, record.status,
                         partyStatusText(record.status, record.currentHP)};
}

struct PokerusState {
    uint8_t strain = 0;
    uint8_t days = 0;
    bool present = false;
    bool active = false;
};

constexpr PokerusState decodePokerus(uint8_t raw) noexcept {
    return {static_cast<uint8_t>(raw >> 4), static_cast<uint8_t>(raw & 0x0F), raw != 0,
            raw != 0 && (raw & 0x0F) != 0};
}

constexpr uint8_t encodePokerus(uint8_t strain, uint8_t days) noexcept {
    return static_cast<uint8_t>(((strain & 0x0F) << 4) | (days & 0x0F));
}

inline std::string pokerusText(uint8_t raw) {
    const auto state = decodePokerus(raw);
    if (!state.present) return "None";
    return state.active ? "Infected" : "Cured";
}

// PKSE-style three-state editor semantics. Merely displaying a raw Pokérus byte
// never normalizes it; only an explicit user activation advances the state.
// None -> Infected uses a canonical valid active value. Existing infected data
// keeps its strain when becoming cured, then Cured -> None clears the byte.
constexpr uint8_t cyclePokerusState(uint8_t raw) noexcept {
    const auto state = decodePokerus(raw);
    if (!state.present) return encodePokerus(1, 1);
    if (state.active) return encodePokerus(state.strain == 0 ? 1 : state.strain, 0);
    return 0;
}

} // namespace PokeBank::UIModel::Gen2Native
