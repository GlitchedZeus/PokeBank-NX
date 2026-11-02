#pragma once
#include "Integration/Encounter/BreedingPolicy.h"
#include "Integration/Encounter/EncounterGuardrails.h"
#include "Integration/Encounter/GiftStaticEncounters.h"
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
    // Owner decision (Nov 2): some reached slot here needs no access prerequisite the save cannot
    // show (ticket / distribution, event Pokémon, Pokémon from another game). A reached row without
    // one is listed as "encounter in the game" but is never a verified encounter (Incomplete).
    bool accessKnown = true;
};

// The source-backed table row a Catalogue example came from (nullptr for a wild slot).
inline const GiftStaticEncounters::Row* dataRow(const WildRow& row) noexcept {
    return GiftStaticEncounters::rowOf(row.encounter);
}

// Round 12 (R12-004 / R12-005, Integration/Encounter/MetLocationPolicy.h): choices() is the
// DEFAULT list -- the stored location (row 1), the wild rows of the species and its ancestry whose
// level the record has reached, the game's own trade / egg markers, then the one
// "Other location (unverified)..." row (MetLocationPolicy::kUnverifiedListRow). choices(true) is
// the UNVERIFIED list -- the stored location (row 1) and every other native location, including
// the wild rows above the record's level ("level is below this encounter").
// PKHeX Locations.LinkTrade4NPC: the Met Location of a Gen IV in-game (NPC) trade.
inline constexpr uint16_t kNpcTrade4 = 2001;

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
        // Round 13 (owner decision 7): the game's source-backed gift / static / in-game trade /
        // fossil / prize / roaming / event rows (Integration/Encounter/GiftStaticEncounters.h) are
        // verified rows exactly like a wild row -- species or ancestry, level-checked -- labelled
        // with their kind ("Gift Lv. 5", "Trade Lv. 30"). An egg gift is met where it hatched, so
        // it is not a row here; event distributions have no data and stay unverified.
        std::vector<EncounterGuardrails::EncounterTemplate> encounters;
        for (const auto& entry : kWild45)
            if (entry.sourceGameId == gameId(origin) && parents[entry.species]) encounters.push_back(entry);
        for (const auto& gift : GiftStaticEncounters::kRows)
            if (gift.game == gameId(origin) && GiftStaticEncounters::metLocationRow(gift) &&
                gift.species < parents.size() && parents[gift.species])
                encounters.push_back(GiftStaticEncounters::templateOf(gift));
        for (const auto& entry : encounters) {
            const bool reached = Policy::levelReachesEncounter(entry.minLevel, currentLevel);
            const auto* gift = GiftStaticEncounters::rowOf(entry);
            const bool known = reached && (!gift || GiftStaticEncounters::fullyVerifiable(*gift));
            auto row = std::find_if(wild_.begin(), wild_.end(), [&](const auto& r) {
                return r.encounter.location == entry.location;
            });
            if (row == wild_.end()) wild_.push_back({entry, 0, reached, known});
            else {
                ++row->variants;
                // The example is a slot the record's level reaches when there is one, then one whose
                // access is known; among those, a direct encounter is preferred over ancestor data.
                const bool better = (reached && !row->levelReached) || (known && !row->accessKnown) ||
                    (reached == row->levelReached && known == row->accessKnown &&
                     entry.species == species && row->encounter.species != species);
                if (better) row->encounter = entry;
                row->levelReached = row->levelReached || reached;
                row->accessKnown = row->accessKnown || known;
            }
        }
        for (auto& row : wild_) row.accessKnown = row.accessKnown && row.levelReached;
        // A verified row's place is a native met location even where the editor's name table
        // stops short of it (the Gen IV in-game trade location 2001).
        for (const auto& row : wild_)
            if (std::find(all.begin(), all.end(), row.encounter.location) == all.end())
                all.push_back(row.encounter.location);
        // Round 13 (owner decisions 5 / 6): Gen IV's Day-Care / trade markers are judged per record.
        // 2001 (in-game trade) is verified by the encdata rows above (GiftStaticEncounters, the one
        // source of truth for Gen IV NPC trades: PKHeX TradeGift_DPPtIngame / TradeGift_HGSS); when
        // no such row verifies it, it is offered in the unverified list with the reason.
        const uint8_t gen = generation(origin);
        egg_ = BreedingPolicy::eggHistory(gen, species, currentLevel);
        std::vector<uint16_t> gen4Rows;   // the record's judged Gen IV rows, verified or not
        if (gen == 4) {
            // An unhatched egg traded by link records 2002 as its Met Location (PKHeX
            // EncounterVerifier.VerifyUnhatchedEgg); an egg is the species itself.
            eggTraded_ = history.egg && BreedingPolicy::hatchable(4, species);
            // An unhatched egg has not hatched: it gets no hatched-egg row.
            gen4Rows = {kNpcTrade4, 2002};
            if (!history.egg) gen4Rows.insert(gen4Rows.begin(), Policy::kHatchedEggRow);
        }
        focused_.push_back(stored);
        for (const auto id : all) {
            if (id == stored) continue;
            const auto* wild = wildRow(id);
            // (a reached row whose access is unknown is listed as the encounter, never counted verified)
            if (wild && wild->levelReached) { focused_.push_back(id); if (wild->accessKnown) ++verified_; }
        }
        for (const auto id : gen4Rows)
            if (id != stored && gen4RowVerified(id) &&
                std::find(focused_.begin(), focused_.end(), id) == focused_.end())   // (2001: a verified trade row)
                focused_.push_back(id);
        for (const auto id : all)
            if (id != stored && Policy::gen45DefaultMarker(generation(origin), id) &&
                std::find(focused_.begin(), focused_.end(), id) == focused_.end())   // (a verified trade row)
                focused_.push_back(id);
        focused_.push_back(Policy::kUnverifiedListRow);
        native_.push_back(stored);
        for (const auto id : gen4Rows)
            if (id != stored && !gen4RowVerified(id) && std::find(all.begin(), all.end(), id) == all.end())
                native_.push_back(id);   // (a place in `all` is listed below)
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
        if (value == kNpcTrade4) return verifiedEncounter(value);
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
        return row && row->levelReached && row->accessKnown;
    }
    // Owner decision (Nov 2): the encounter is in the game and the level reaches it, but its access
    // prerequisite (ticket, event Pokémon, other game) cannot be known from the save.
    bool incompleteEncounter(uint16_t location) const noexcept {
        const auto* row = wildRow(location);
        return row && row->levelReached && !row->accessKnown;
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
        // 2001 without a verified trade row of the species / its ancestry at this level.
        if (generation(origin_) == 4 && location == kNpcTrade4 && !current && !verifiedEncounter(location)) {
            if (const auto* row = wildRow(location))
                return "NPC trade Lv. " + std::to_string(row->encounter.minLevel) + "+" + via(row->encounter.species) +
                       "; " + MetLocationPolicy::kLevelBelowEncounter;
            return "NPC trade; not this species here";
        }
        if (generation(origin_) == 4 && location == 2002 && !current)
            return eggTraded_ ? "Egg traded before hatching" : "Link trade; only an unhatched egg";
        // A verified row: its method / kind and level ("Wild Lv. 3–5", "Gift Lv. 5", an in-game
        // trade arriving at the traded Pokémon's level "Lv. 20+"), the ancestor it was met as, and
        // how many more slots share the place (short so rows fit 620 px).
        const auto verifiedText = [&](const WildRow& row) {
            const auto& encounter = row.encounter;
            std::string text = std::string(EncounterGuardrails::methodName(encounter.method)) + " Lv. " +
                               std::to_string(encounter.minLevel);
            if (encounter.maxLevel == 100 && encounter.minLevel < 100) text += "+";
            else if (encounter.maxLevel != encounter.minLevel) text += "–" + std::to_string(encounter.maxLevel);
            if (encounter.species != species_) text += " via " + std::string(name(encounter.species));
            if (row.variants) text += " (+" + std::to_string(row.variants) + ")";
            return text;
        };
        // Stored history takes priority over wild tables. No edit invents a matching history.
        if (current && history_.egg) return "Stored egg; hatch history unverified";
        if (current && history_.eggLocation) return "Stored egg/hatch history; unverified";
        // Round 13 (owner decision 7): a verified gift / static / trade row of the species or its
        // ancestry names its kind, even at a trade location that is also the game's marker. A stored
        // fateful flag keeps its own label unless the game sets that flag for this encounter.
        if (const auto* row = wildRow(location); row && row->levelReached) {
            const auto* gift = dataRow(*row);
            const bool fatefulRow = gift && (gift->flags & GiftStaticEncounters::kFateful);
            if (!(current && history_.fateful) || fatefulRow)
                return tagged(row->accessKnown ? verifiedText(*row)
                                               : verifiedText(*row) + "; " + GiftStaticEncounters::kAccessUnverified);
        }
        // Round 7 B05: transfer / trade / egg markers and event locations say what the pinned
        // tables and native names establish, and their native id (Names/SpecialMetLocationNotes.h).
        if (auto special = Names::specialMetLocation(origin_, location); !special.empty())
            return tagged(std::move(special.row));
        if (current && history_.fateful) return "Stored fateful flag; event/gift unverified";
        if (const auto* row = wildRow(location)) {
            const auto& encounter = row->encounter;
            // R12-005: above the record's level -- the reason it is in the unverified list (short,
            // without the method, so it fits beside the place name; "Lv. 51+" is the minimum it misses).
            std::string text = "Lv. " + std::to_string(encounter.minLevel) + "+";
            if (encounter.species != species_) text += " via " + std::string(name(encounter.species));
            return tagged(text + "; " + MetLocationPolicy::kLevelBelowEncounter);
        }
        // Round 14 (R06): 0 (Mystery Zone) is "no place recorded" -- a new Pokemon's placeholder --,
        // not a gift / static history.
        return tagged(location == 0 ? "no place recorded; unverified" : "Gift / static / other history; unverified");
    }

private:
    uint8_t origin_ = 0;
    uint16_t species_ = 0, stored_ = 0;
    uint8_t currentLevel_ = 0;
    size_t verified_ = 0;
    History history_{};
    BreedingPolicy::EggVerdict egg_{};
    bool eggTraded_ = false;
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
    // Owner decision (Nov 2): a verified data row says it has a single source (PKHeX); a wild slot
    // keeps "" as before. An encounter whose access prerequisite is unknown reads Incomplete.
    if (context.verifiedEncounter(location)) {
        const auto* gift = dataRow(*context.wildRow(location));
        return gift ? GiftStaticEncounters::singleSourceNote(*gift) : std::string();
    }
    if (context.incompleteEncounter(location))
        if (const auto* gift = dataRow(*context.wildRow(location))) return GiftStaticEncounters::incompleteNote(*gift);
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
