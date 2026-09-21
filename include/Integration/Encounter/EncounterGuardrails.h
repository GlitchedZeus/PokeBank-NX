#pragma once
#include <algorithm>
#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

namespace PokeVault::Integration::EncounterGuardrails {

enum class Method : uint8_t {
    Grass = 0, Surf = 1, OldRod = 2, GoodRod = 3, SuperRod = 4, RockSmash = 5,
    Headbutt = 6, HeadbuttSpecial = 7, BugContest = 8,
    Static = 9, Gift = 10, Egg = 11, Event = 12,
};

struct EncounterTemplate {
    std::string_view sourceGameId;
    uint16_t species;
    uint16_t location;
    uint8_t minLevel;
    uint8_t maxLevel;
    Method method;
    uint8_t timeMask; // Gen II EncounterTime bits; 0 means unrestricted / not applicable.
    constexpr bool fixedLevel() const noexcept { return minLevel == maxLevel; }
    constexpr bool containsLevel(uint8_t level) const noexcept {
        return level >= minLevel && level <= maxLevel;
    }
};

constexpr const char* methodName(Method method) noexcept {
    switch (method) {
        case Method::Grass: return "Wild";
        case Method::Surf: return "Surf";
        case Method::OldRod: return "Old Rod";
        case Method::GoodRod: return "Good Rod";
        case Method::SuperRod: return "Super Rod";
        case Method::RockSmash: return "Rock Smash";
        case Method::Headbutt: return "Headbutt";
        case Method::HeadbuttSpecial: return "Headbutt (special)";
        case Method::BugContest: return "Bug Contest";
        case Method::Static: return "Static";
        case Method::Gift: return "Gift";
        case Method::Egg: return "Egg";
        case Method::Event: return "Event";
    }
    return "Encounter";
}

#include "Integration/Encounter/EncounterGuardrailsData.inc"

inline std::vector<EncounterTemplate> forGameSpecies(std::string_view sourceGameId, uint16_t species) {
    std::vector<EncounterTemplate> out;
    for (const auto& encounter : kEncounterTemplates)
        if (encounter.sourceGameId == sourceGameId && encounter.species == species)
            out.push_back(encounter);
    return out;
}

inline std::vector<EncounterTemplate> forSpeciesAllSupported(uint16_t species) {
    std::vector<EncounterTemplate> out;
    for (const auto& encounter : kEncounterTemplates)
        if (encounter.species == species) out.push_back(encounter);
    return out;
}

inline std::optional<uint8_t> minimumLevel(std::string_view sourceGameId,
                                           uint16_t species) noexcept {
    std::optional<uint8_t> result;
    for (const auto& encounter : kEncounterTemplates) {
        if (encounter.sourceGameId != sourceGameId || encounter.species != species ||
            encounter.minLevel < 1 || encounter.minLevel > 100)
            continue;
        if (!result || encounter.minLevel < *result) result = encounter.minLevel;
    }
    return result;
}

inline const EncounterTemplate* find(std::string_view sourceGameId, uint16_t species,
                                     uint16_t location, uint8_t level) noexcept {
    for (const auto& encounter : kEncounterTemplates)
        if (encounter.sourceGameId == sourceGameId && encounter.species == species &&
            encounter.location == location && encounter.containsLevel(level))
            return &encounter;
    return nullptr;
}

inline bool locationAllowed(std::string_view sourceGameId, uint16_t species, uint16_t location) noexcept {
    for (const auto& encounter : kEncounterTemplates)
        if (encounter.sourceGameId == sourceGameId && encounter.species == species &&
            encounter.location == location)
            return true;
    return false;
}

} // namespace PokeVault::Integration::EncounterGuardrails
