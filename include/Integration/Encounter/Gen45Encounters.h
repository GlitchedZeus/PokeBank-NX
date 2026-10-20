#pragma once
#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

// NEW-12 / R2-CHECK-011 (Round 3): where a species is actually met in each Generation IV / V game,
// generated from PKHeX 6501f0ab by tools/gen_gen45_encounters.py (wild slots, fixed-location static
// and gift encounters, pre-evolution lineage). The Met Location picker lists these spots first,
// with the method and level, and keeps every other native location as an egg / trade / event /
// transfer origin. Nothing here rejects a value: a Pokemon met somewhere else may still be
// legitimate (hatched there, traded, an event, Dream World, Pokewalker, a roamer).
namespace PokeVault::Integration::Gen45Encounters {

struct Gen45Encounter {
    uint8_t game;       // 1 D, 2 P, 3 Pt, 4 HG, 5 SS, 6 B, 7 W, 8 B2, 9 W2
    uint16_t species;
    uint16_t location;  // the game's native met location id
    uint8_t minLevel;
    uint8_t maxLevel;
    uint8_t method;     // methodName()
};

#include "Integration/Encounter/Gen45EncounterData.inc"

// PKHeX GameVersion of the record's origin -> table game id (0 = not a Gen IV / V game).
constexpr uint8_t gameForVersion(uint8_t version) noexcept {
    switch (version) {
        case 10: return 1;   // Diamond
        case 11: return 2;   // Pearl
        case 12: return 3;   // Platinum
        case 7: return 4;    // HeartGold
        case 8: return 5;    // SoulSilver
        case 21: return 6;   // Black
        case 20: return 7;   // White
        case 23: return 8;   // Black 2
        case 22: return 9;   // White 2
        default: return 0;
    }
}

constexpr const char* methodName(uint8_t method) noexcept {
    switch (method) {
        case 0: return "Grass";
        case 1: return "Surf";
        case 2: return "Old Rod";
        case 3: return "Good Rod";
        case 4: return "Super Rod";
        case 5: return "Rock Smash";
        case 6: case 7: return "Headbutt";
        case 8: return "Bug Contest";
        case 9: return "Honey Tree";
        case 10: return "Safari Grass";
        case 11: return "Safari Surf";
        case 12: return "Safari Old Rod";
        case 13: return "Safari Good Rod";
        case 14: return "Safari Super Rod";
        case 20: return "Wild";
        case 21: return "Grass";
        case 22: return "Surf";
        case 23: return "Super Rod";
        case 24: return "Swarm";
        case 25: return "Hidden Grotto";
        case 40: return "Gift / static";
        default: return "Encounter";
    }
}

constexpr uint16_t preEvolution(uint16_t species) noexcept {
    return species < sizeof(kGen45PreEvolution) / sizeof(kGen45PreEvolution[0]) ? kGen45PreEvolution[species] : 0;
}

// One spot where this Pokemon (or the pre-evolution it was met as) is found.
struct Spot {
    uint16_t location = 0;
    uint16_t metAs = 0;       // species met at this spot (== the Pokemon's species, or a pre-evolution)
    uint8_t minLevel = 0, maxLevel = 0;
    uint8_t method = 0;
};

// Every encounter spot for `species` and its pre-evolutions in `game`, the species' own spots
// first, then each pre-evolution's (nearest first). One Spot per (location, metAs), merging
// methods by keeping the widest level range and the first method seen.
inline std::vector<Spot> spotsFor(uint8_t game, uint16_t species) {
    std::vector<Spot> out;
    uint16_t current = species;
    for (int depth = 0; current != 0 && depth < 4; ++depth, current = preEvolution(current)) {
        for (const auto& e : kGen45Encounters) {
            if (e.game != game || e.species != current) continue;
            bool merged = false;
            for (auto& s : out)
                if (s.location == e.location && s.metAs == current) {
                    if (e.minLevel < s.minLevel) s.minLevel = e.minLevel;
                    if (e.maxLevel > s.maxLevel) s.maxLevel = e.maxLevel;
                    merged = true;
                }
            if (!merged) out.push_back({e.location, current, e.minLevel, e.maxLevel, e.method});
        }
    }
    return out;
}

// "Grass Lv. 10-12" / "Gift / static Lv. 5"; `metAsName` is non-empty when the spot belongs to a
// pre-evolution ("as Spheal: Safari Grass Lv. 44-45").
inline std::string spotDetail(const Spot& s, const std::string& metAsName) {
    std::string text = metAsName.empty() ? std::string() : "as " + metAsName + ": ";
    text += methodName(s.method);
    text += " Lv. " + std::to_string(s.minLevel);
    if (s.maxLevel != s.minLevel) text += "\xE2\x80\x93" + std::to_string(s.maxLevel);
    return text;
}

// Detail for a native location with no encounter for this species: still a valid met location
// (hatched there, traded, event, transfer), never a rejection.
inline constexpr const char* kOtherOriginDetail = "Egg / trade / event";

// Reorder the native met location list: the spots' locations first (each once, in spot order,
// only those the list offers), then every other native location in its native order. Nothing is
// removed. Returns how many locations lead the list.
inline int orderBySpots(std::vector<uint16_t>& values, const std::vector<Spot>& spots) {
    std::vector<uint16_t> ordered;
    ordered.reserve(values.size());
    for (const auto& spot : spots)
        if (std::find(values.begin(), values.end(), spot.location) != values.end() &&
            std::find(ordered.begin(), ordered.end(), spot.location) == ordered.end())
            ordered.push_back(spot.location);
    const int leading = static_cast<int>(ordered.size());
    for (const auto value : values)
        if (std::find(ordered.begin(), ordered.end(), value) == ordered.end()) ordered.push_back(value);
    values = std::move(ordered);
    return leading;
}

// The row detail for `location`: its encounter (first spot there) or the other-origin note.
template <class SpeciesName>
inline std::string detailFor(const std::vector<Spot>& spots, uint16_t location, uint16_t species,
                             SpeciesName speciesName) {
    for (const auto& spot : spots)
        if (spot.location == location)
            return spotDetail(spot, spot.metAs == species ? std::string() : std::string(speciesName(spot.metAs)));
    return kOtherOriginDetail;
}

} // namespace PokeVault::Integration::Gen45Encounters
