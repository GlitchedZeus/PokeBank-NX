#pragma once
#include "Integration/Encounter/BreedingPolicy.h"
#include "Integration/Encounter/EncounterGuardrails.h"
#include "Integration/Encounter/InGameTrades4.h"
#include "Integration/Encounter/MetLocationPolicy.h"
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
    // R12-005: some slot here has a minimum level the record's current level reaches. A row
    // without one is never a verified encounter for this record; it moves to the unverified list.
    bool levelReached = true;
};

// Round 12 (R12-004 / R12-005, Integration/Encounter/MetLocationPolicy.h): choices() is the
// DEFAULT list -- the stored location (row 1), the wild rows of the species and its ancestry whose
// level the record has reached, the game's own trade / egg markers, then the one
// "Other location (unverified)..." row (MetLocationPolicy::kUnverifiedListRow). choices(true) is
// the UNVERIFIED list -- the stored location (row 1) and every other native location, including
// the wild rows above the record's level ("level is below this encounter").
class Catalogue {
public:
    Catalogue() = default;
    Catalogue(uint8_t origin, uint16_t species, uint16_t stored,
              std::vector<uint16_t> native, History history, uint8_t currentLevel = 0)
        : origin_(origin), species_(species), stored_(stored), currentLevel_(currentLevel), history_(history) {
        namespace Policy = MetLocationPolicy;
        // Preserve the stored value even if it is unusual or absent from today's name table.
        std::vector<uint16_t> all{stored};
        for (const auto id : native)
            if (std::find(all.begin(), all.end(), id) == all.end()) all.push_back(id);
        const auto parents = ancestry(generation(origin), species);
        for (const auto& entry : kWild45) {
            if (entry.sourceGameId != gameId(origin) || !parents[entry.species]) continue;
            const bool reached = Policy::levelReachesEncounter(entry.minLevel, currentLevel);
            auto row = std::find_if(wild_.begin(), wild_.end(), [&](const auto& r) {
                return r.encounter.location == entry.location;
            });
            if (row == wild_.end()) wild_.push_back({entry, 0, reached});
            else {
                ++row->variants;
                // The example is a slot the record's level reaches when there is one; among those,
                // a direct encounter is preferred over ancestor data.
                const bool better = (reached && !row->levelReached) ||
                    (reached == row->levelReached && entry.species == species && row->encounter.species != species);
                if (better) row->encounter = entry;
                row->levelReached = row->levelReached || reached;
            }
        }
        // Round 13 (owner decisions 5 / 6): Gen IV's Day-Care / trade markers are judged per record.
        const uint8_t gen = generation(origin);
        egg_ = BreedingPolicy::eggHistory(gen, species, currentLevel);
        std::vector<uint16_t> gen4Rows;   // the record's Gen IV marker rows, verified or not
        if (gen == 4) {
            for (uint16_t member = 1; member <= 493; ++member) {
                const auto* trade = parents[member] ? InGameTrades4::find(origin, member) : nullptr;
                if (!trade) continue;
                const bool reached = Policy::levelReachesEncounter(trade->level, currentLevel);
                // Prefer a trade the level reaches; among those, the species itself.
                if (!trade_ || (reached && !tradeReached_) ||
                    (reached == tradeReached_ && member == species && trade_->species != species)) {
                    trade_ = trade;
                    tradeReached_ = reached;
                }
            }
            // An unhatched egg traded by link records 2002 as its Met Location (PKHeX
            // EncounterVerifier.VerifyUnhatchedEgg); an egg is the species itself.
            eggTraded_ = history.egg && BreedingPolicy::hatchable(4, species);
            // An unhatched egg has not hatched: it gets no hatched-egg row.
            gen4Rows = {InGameTrades4::kMetLocation, 2002};
            if (!history.egg) gen4Rows.insert(gen4Rows.begin(), Policy::kHatchedEggRow);
        }
        focused_.push_back(stored);
        for (const auto id : all) {
            if (id == stored) continue;
            const auto* wild = wildRow(id);
            if (wild && wild->levelReached) { focused_.push_back(id); ++verified_; }
        }
        for (const auto id : gen4Rows)
            if (id != stored && gen4RowVerified(id)) focused_.push_back(id);
        for (const auto id : all)
            if (id != stored && Policy::gen45DefaultMarker(gen, id)) focused_.push_back(id);
        focused_.push_back(Policy::kUnverifiedListRow);
        native_.push_back(stored);
        for (const auto id : gen4Rows)
            if (id != stored && !gen4RowVerified(id)) native_.push_back(id);
        for (const auto id : all)
            if (id != stored && std::find(focused_.begin(), focused_.end(), id) == focused_.end())
                native_.push_back(id);
    }

    const std::vector<uint16_t>& choices(bool otherHistories = false) const noexcept {
        return otherHistories ? native_ : focused_;
    }
    uint16_t stored() const noexcept { return stored_; }
    uint8_t origin() const noexcept { return origin_; }
    uint8_t currentLevel() const noexcept { return currentLevel_; }
    // The default list's verified wild rows (the stored row and markers excluded).
    size_t verifiedCount() const noexcept { return verified_; }
    static constexpr bool isUnverifiedListRow(uint16_t value) noexcept {
        return value == MetLocationPolicy::kUnverifiedListRow;
    }
    // Round 13: "Hatched from a Day-Care egg" (Gen IV), a row that writes the hatched-egg fields.
    static constexpr bool isHatchedEggRow(uint16_t value) noexcept {
        return value == MetLocationPolicy::kHatchedEggRow;
    }
    // The breeding verdict for this record (BreedingPolicy::eggHistory).
    const BreedingPolicy::EggVerdict& eggVerdict() const noexcept { return egg_; }
    // Round 13 (owner decision 6): a Gen IV marker row is verified for this record -- the hatched
    // row when the species / a pre-evolution hatches here at this level, 2001 when it (or a
    // pre-evolution) is this exact game's in-game trade at or below its level, 2002 for a hatchable
    // egg (traded before hatching).
    bool gen4RowVerified(uint16_t value) const noexcept {
        if (generation(origin_) != 4) return false;
        if (value == MetLocationPolicy::kHatchedEggRow) return egg_.verified();
        if (value == InGameTrades4::kMetLocation) return trade_ && tradeReached_;
        if (value == 2002) return eggTraded_;
        return false;
    }
    const WildRow* wildRow(uint16_t location) const noexcept {
        const auto row = std::find_if(wild_.begin(), wild_.end(), [&](const auto& r) {
            return r.encounter.location == location;
        });
        return row == wild_.end() ? nullptr : &*row;
    }
    // A verified wild encounter of the species / ancestry at `location` that the record's level
    // reaches (R12-005); a marker or any other row is not one.
    bool verifiedEncounter(uint16_t location) const noexcept {
        const auto* row = wildRow(location);
        return (row && row->levelReached) || (location == InGameTrades4::kMetLocation && gen4RowVerified(location));
    }
    const char* scope(bool otherHistories) const noexcept {
        // Row 1 of either list is always the stored location (focused_ / native_ start with it).
        if (otherHistories) return MetLocationPolicy::kUnverifiedListScope;
        return verified_ ? MetLocationPolicy::kDefaultListScope : MetLocationPolicy::kMarkersOnlyScope;
    }
    template <class SpeciesName>
    std::string detail(uint16_t location, SpeciesName name) const {
        if (isUnverifiedListRow(location)) return MetLocationPolicy::kUnverifiedListRowDetail;
        const bool current = location == stored_;
        const auto tagged = [&](std::string text) { return current ? "Stored: " + text : text; };
        const auto via = [&](uint16_t member) {
            return member != 0 && member != species_ ? " via " + std::string(name(member)) : std::string();
        };
        // Round 13 (owner decisions 5 / 6): the Gen IV Day-Care / trade rows say whether this record's
        // species and level verify them, and why not.
        if (isHatchedEggRow(location))
            return egg_.verified() ? MetLocationPolicy::kHatchedEggVerified + via(egg_.hatchedAs)
                                   : std::string(MetLocationPolicy::kHatchedEggUnverified) + "; " +
                                         BreedingPolicy::reasonText(egg_.reason);
        if (generation(origin_) == 4 && location == InGameTrades4::kMetLocation && !(current && history_.egg)) {
            if (gen4RowVerified(location))
                return tagged("NPC trade Lv. " + std::to_string(trade_->level) + via(trade_->species));
            if (trade_) return tagged("NPC trade Lv. " + std::to_string(trade_->level) + "+" + via(trade_->species) +
                                      "; " + MetLocationPolicy::kLevelBelowEncounter);
            return tagged("NPC trade; not this species here");
        }
        if (generation(origin_) == 4 && location == 2002 && !current)
            return eggTraded_ ? "Egg traded before hatching" : "Link trade; only an unhatched egg";
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
            // R12-005: above the record's level -- the reason it is in the unverified list (short,
            // without the method, so it fits beside the place name).
            std::string text = row->levelReached ? std::string(EncounterGuardrails::methodName(encounter.method)) + " " : "";
            text += "Lv. " + std::to_string(encounter.minLevel);
            // (a row above the level shows the minimum it misses: "Lv. 51+")
            if (!row->levelReached) text += "+";
            else if (encounter.maxLevel != encounter.minLevel) text += "–" + std::to_string(encounter.maxLevel);
            if (encounter.species != species_) text += " via " + std::string(name(encounter.species));
            if (!row->levelReached) return tagged(text + "; " + MetLocationPolicy::kLevelBelowEncounter);
            if (row->variants) text += " (+" + std::to_string(row->variants) + ")";   // more slots here; short so rows fit 620 px
            return tagged(text);
        }
        return tagged("Gift / static / other history; unverified");
    }

private:
    uint8_t origin_ = 0;
    uint16_t species_ = 0, stored_ = 0;
    uint8_t currentLevel_ = 0;
    size_t verified_ = 0;
    History history_{};
    BreedingPolicy::EggVerdict egg_{};
    const InGameTrades4::Trade* trade_ = nullptr;
    bool tradeReached_ = false, eggTraded_ = false;
    std::vector<uint16_t> native_, focused_;
    std::vector<WildRow> wild_;
};

// Round 12 (R12-004 / R12-005): the editor / Review line for a Gen IV / V record's stored Met
// Location. "" when it is a verified wild row of the species / its ancestry that the current level
// reaches (or the origin is not a Gen IV / V game, or nothing is recorded); otherwise an honest
// unverified label -- a marker or event names itself. Never "legal"; nothing is changed.
// Round 13 (owner decision 5): a hatched Pokémon (Met Level 0, a bred-egg Egg Location: Gen IV
// 2000 / 2002, Gen V 60002 / 30003 / 30002 -- PKHeX Locations.IsEggLocationBred4 / 5) reads as a
// verified hatch only when BreedingPolicy verifies its species (or a pre-evolution) hatches from a
// Day-Care egg in that generation; otherwise the note says why. The Met Location is the place it
// hatched; any native place of the game is one.
constexpr bool bredEggLocation45(uint8_t generation, uint16_t eggLocation) noexcept {
    return generation == 4 ? eggLocation == 2000 || eggLocation == 2002
         : generation == 5 ? eggLocation == 60002 || eggLocation == 30003 || eggLocation == 30002 : false;
}
inline std::string unverifiedMetNote45(uint8_t origin, uint16_t species, uint16_t location, uint8_t level,
                                       uint16_t eggLocation = 0, int metLevel = -1) {
    if (gameId(origin).empty() || location == 0) return {};
    const Catalogue context(origin, species, location, {}, {}, level);
    if (context.verifiedEncounter(location)) return {};
    if (metLevel == 0 && bredEggLocation45(generation(origin), eggLocation)) {
        const auto& egg = context.eggVerdict();
        if (egg.verified()) return {};
        return std::string("Hatched-egg history; ") + BreedingPolicy::reasonText(egg.reason) + "; unverified";
    }
    if (auto special = Names::specialMetLocation(origin, location); !special.empty())
        return special.row + "; unverified";
    if (context.wildRow(location)) return MetLocationPolicy::kLevelBelowEncounterFull;
    return "Not a listed encounter; unverified";
}
} // namespace PokeVault::Integration::EncounterContext45
