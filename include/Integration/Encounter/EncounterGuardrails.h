#pragma once
#include <algorithm>
#include <array>
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

struct Gen3EvolutionParent {
    uint16_t currentSpecies;
    uint16_t originSpecies;
    uint8_t minimumEvolutionLevel; // 0 when the evolution is not level-gated.
};

// Verified ancestry edges used by the current Gen III encounter-provenance guardrail.
// This stays data-driven so UI code never special-cases individual species.
inline constexpr std::array<Gen3EvolutionParent, 1> kGen3EvolutionParents{{
    {117, 116, 32}, // Horsea -> Seadra, Level 32
}};

inline std::optional<Gen3EvolutionParent> gen3EvolutionParent(uint16_t currentSpecies) noexcept {
    for (const auto& edge : kGen3EvolutionParents)
        if (edge.currentSpecies == currentSpecies) return edge;
    return std::nullopt;
}

inline std::vector<Gen3EvolutionParent> gen3EvolutionAncestry(uint16_t currentSpecies) {
    std::vector<Gen3EvolutionParent> out;
    uint16_t species = currentSpecies;
    for (std::size_t depth = 0; depth < 8; ++depth) {
        const auto parent = gen3EvolutionParent(species);
        if (!parent) break;
        out.push_back(*parent);
        if (parent->originSpecies == species) break;
        species = parent->originSpecies;
    }
    return out;
}


inline std::vector<EncounterTemplate> forGameSpecies(std::string_view sourceGameId, uint16_t species) {
    std::vector<EncounterTemplate> out;
    for (const auto& encounter : kEncounterTemplates)
        if (encounter.sourceGameId == sourceGameId && encounter.species == species)
            out.push_back(encounter);
    return out;
}

struct EncounterProvenanceChoice {
    EncounterTemplate encounter;
    uint16_t currentSpecies = 0;
    uint16_t originalEncounterSpecies = 0;
    uint8_t minimumEvolutionLevel = 0;
    bool evolved = false;
};

inline std::vector<EncounterProvenanceChoice> forGameSpeciesWithGen3Provenance(
    std::string_view sourceGameId, uint16_t currentSpecies) {
    std::vector<EncounterProvenanceChoice> out;
    const auto direct = forGameSpecies(sourceGameId, currentSpecies);
    if (!direct.empty()) {
        out.reserve(direct.size());
        for (const auto& encounter : direct)
            out.push_back({encounter, currentSpecies, currentSpecies, 0, false});
        return out;
    }
    for (const auto& edge : gen3EvolutionAncestry(currentSpecies)) {
        const auto inherited = forGameSpecies(sourceGameId, edge.originSpecies);
        for (const auto& encounter : inherited)
            out.push_back({encounter, currentSpecies, edge.originSpecies,
                           edge.minimumEvolutionLevel, true});
        if (!out.empty()) break;
    }
    return out;
}

inline bool locationAllowedWithGen3Provenance(std::string_view sourceGameId,
                                               uint16_t currentSpecies,
                                               uint16_t location) {
    const auto choices = forGameSpeciesWithGen3Provenance(sourceGameId, currentSpecies);
    return std::any_of(choices.begin(), choices.end(), [location](const auto& choice) {
        return choice.encounter.location == location;
    });
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
