#pragma once
#include "Integration/Encounter/EncounterGuardrails.h"
#include "Names/SpecialMetLocationNotes.h"
#include <array>
#include <string>

namespace PokeVault::Integration::EncounterContext45 {

// This catalogue informs the editor. It never validates or rewrites a Pokémon.
// Evolution edges establish possible ancestry, not satisfaction of its conditions.
struct EvolutionEdge { uint8_t generation; uint16_t child, parent; };
#include "Integration/Encounter/EncounterContext45Data.inc"

constexpr std::string_view gameId(uint8_t origin) noexcept {
    switch (origin) {
        case 7: return "heartgold_nds";
        case 8: return "soulsilver_nds";
        case 10: return "diamond_nds";
        case 11: return "pearl_nds";
        case 12: return "platinum_nds";
        case 20: return "white_nds";
        case 21: return "black_nds";
        case 22: return "white2_nds";
        case 23: return "black2_nds";
        default: return {};
    }
}
constexpr uint8_t generation(uint8_t origin) noexcept {
    return origin == 7 || origin == 8 || (origin >= 10 && origin <= 12) ? 4 :
           origin >= 20 && origin <= 23 ? 5 : 0;
}

inline std::array<bool, 650> ancestry(uint8_t gen, uint16_t species) {
    std::array<bool, 650> seen{};
    if ((gen != 4 && gen != 5) || species == 0 || species > (gen == 4 ? 493 : 649))
        return seen;
    std::vector<uint16_t> pending{species};
    seen[species] = true;
    for (size_t i = 0; i < pending.size(); ++i)
        for (const auto& edge : kEvolution45)
            if (edge.generation == gen && edge.child == pending[i] && !seen[edge.parent]) {
                seen[edge.parent] = true;
                pending.push_back(edge.parent);
            }
    return seen;
}

struct History {
    uint16_t eggLocation = 0;
    bool egg = false;
    bool fateful = false;
};
struct WildRow {
    EncounterGuardrails::EncounterTemplate encounter;
    size_t variants = 0;
};

class Catalogue {
public:
    Catalogue() = default;
    Catalogue(uint8_t origin, uint16_t species, uint16_t stored,
              std::vector<uint16_t> native, History history)
        : origin_(origin), species_(species), stored_(stored), history_(history) {
        // Preserve the stored value even if it is unusual or absent from today's name table.
        native_.push_back(stored);
        for (const auto id : native)
            if (std::find(native_.begin(), native_.end(), id) == native_.end()) native_.push_back(id);
        const auto parents = ancestry(generation(origin), species);
        for (const auto& entry : kWild45) {
            if (entry.sourceGameId != gameId(origin) || !parents[entry.species]) continue;
            auto row = std::find_if(wild_.begin(), wild_.end(), [&](const auto& r) {
                return r.encounter.location == entry.location;
            });
            if (row == wild_.end()) wild_.push_back({entry, 0});
            else {
                ++row->variants;
                // Prefer a direct encounter as the example if both direct and ancestor data exist.
                if (entry.species == species && row->encounter.species != species) row->encounter = entry;
            }
        }
        focused_.push_back(stored);
        for (const auto id : native_)
            if (id != stored && wildRow(id)) focused_.push_back(id);
    }

    const std::vector<uint16_t>& choices(bool otherHistories = false) const noexcept {
        return otherHistories ? native_ : focused_;
    }
    uint16_t stored() const noexcept { return stored_; }
    uint8_t origin() const noexcept { return origin_; }
    const WildRow* wildRow(uint16_t location) const noexcept {
        const auto row = std::find_if(wild_.begin(), wild_.end(), [&](const auto& r) {
            return r.encounter.location == location;
        });
        return row == wild_.end() ? nullptr : &*row;
    }
    const char* scope(bool otherHistories) const noexcept {
        // Row 1 of either list is always the stored location (focused_ / native_ start with it).
        return otherHistories ? "Other native histories; unverified; row 1 = stored"
                              : "Wild data, species + ancestry; unchecked; row 1 = stored";
    }
    template <class SpeciesName>
    std::string detail(uint16_t location, SpeciesName name) const {
        const bool current = location == stored_;
        const auto tagged = [&](std::string text) { return current ? "Stored: " + text : text; };
        // Stored history takes priority over wild tables. No edit invents a matching history.
        if (current && history_.egg) return "Stored egg; hatch history unverified";
        if (current && history_.eggLocation) return "Stored egg/hatch history; unverified";
        // Round 7 B05: transfer / trade / egg markers and event locations say what the pinned
        // tables and native names establish, and their native id (Names/SpecialMetLocationNotes.h).
        if (auto special = Names::specialMetLocation(origin_, location); !special.empty())
            return tagged(std::move(special.row));
        if (current && history_.fateful) return "Stored fateful flag; event/gift unverified";
        if (const auto* row = wildRow(location)) {
            const auto& encounter = row->encounter;
            std::string text = std::string(EncounterGuardrails::methodName(encounter.method)) +
                " Lv. " + std::to_string(encounter.minLevel);
            if (encounter.maxLevel != encounter.minLevel) text += "–" + std::to_string(encounter.maxLevel);
            if (encounter.species != species_) text += " via " + std::string(name(encounter.species));
            if (row->variants) text += " (+" + std::to_string(row->variants) + ")";   // more slots here; short so rows fit 620 px
            return tagged(text);
        }
        return tagged("Gift / static / other history; unverified");
    }

private:
    uint8_t origin_ = 0;
    uint16_t species_ = 0, stored_ = 0;
    History history_{};
    std::vector<uint16_t> native_, focused_;
    std::vector<WildRow> wild_;
};
} // namespace PokeVault::Integration::EncounterContext45
