#pragma once

#include "UI/Gen12DVReroll.h"
#include "UI/Gen2NativePresentation.h"
#include "UI/Gen2PokemonSession.h"
#include "Integration/Encounter/EncounterGuardrails.h"
#include "UI/ExactGameMetLocations.h"
#include "Names/SpeciesNames.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace PokeBank::UIModel::Gen2PokemonPicker {
namespace Encounter = PokeVault::Integration::EncounterGuardrails;

inline std::string encounterTimeLabel(uint8_t timeMask) {
    if (timeMask == 0) return {};
    std::string out;
    const auto add = [&](const char* text) {
        if (!out.empty()) out += "/";
        out += text;
    };
    if (timeMask & (1u << 1)) add("Morning");
    if (timeMask & (1u << 2)) add("Day");
    if (timeMask & (1u << 3)) add("Night");
    return out;
}

// Round 13 (owner decision 8: "Crystal time rows can be grouped visually, but do not lose
// encounter-time distinctions"): the short time list a grouped Crystal row shows -- "morn/day",
// "night", and "any time" for a slot met morning, day AND night (said, never dropped; short so the
// row fits the 640 px chooser). "" for a gift / static row (no time restriction in the data).
inline std::string encounterTimeShortLabel(uint8_t timeMask) {
    if ((timeMask & 0x0E) == 0x0E) return "any time";
    std::string out;
    const auto add = [&](const char* text) {
        if (!out.empty()) out += "/";
        out += text;
    };
    if (timeMask & (1u << 1)) add("morn");
    if (timeMask & (1u << 2)) add("day");
    if (timeMask & (1u << 3)) add("night");
    return out;
}

enum class Kind : uint8_t { None, Species, Move, Location };

struct Model {
    Kind kind = Kind::None;
    int index = 0;
    bool previewShiny = false;
    std::size_t moveSlot = 0;
    // Round 12 (R12-004 / R12-005, Integration/Encounter/MetLocationPolicy.h): the Crystal Met
    // Location chooser shows ONE of two lists.
    //  DEFAULT (unverifiedList == false): encounterChoices = the verified Crystal templates of the
    //    species and its pre-evolutions that its current level reaches; otherLocations = the Gift /
    //    Event markers; then the one "Other location (unverified)..." row (unverifiedEntry).
    //  UNVERIFIED (unverifiedList == true): levelBlocked = the templates above the current level
    //    ("level is below this encounter"), then otherLocations = every other native landmark.
    // Rows run encounterChoices, levelBlocked, otherLocations, then the entry row.
    std::vector<Encounter::EncounterTemplate> encounterChoices;
    std::vector<Encounter::EncounterTemplate> levelBlocked;
    std::vector<uint16_t> otherLocations;
    bool unverifiedList = false;
    bool unverifiedEntry = false;
    uint16_t locationSpecies = 0;   // the species the list was built for ("via Ekans" on ancestry rows)

    bool active() const noexcept { return kind != Kind::None; }
    void close() noexcept {
        kind = Kind::None;
        encounterChoices.clear();
        levelBlocked.clear();
        otherLocations.clear();
        unverifiedList = unverifiedEntry = false;
    }

    void openSpecies(uint16_t current, bool shiny = false) noexcept {
        previewShiny = shiny;
        kind = Kind::Species;
        index = std::clamp<int>(current, 1, 251) - 1;
    }
    uint16_t speciesChoice() const noexcept {
        return static_cast<uint16_t>(std::clamp(index, 0, 250) + 1);
    }

    void openMove(uint16_t current, std::size_t slot) noexcept {
        kind = Kind::Move;
        index = std::clamp<int>(current, 0, 251);
        moveSlot = std::min<std::size_t>(slot, 3);
    }
    uint16_t moveChoice() const noexcept {
        return static_cast<uint16_t>(std::clamp(index, 0, 251));
    }

    // `currentLocation` / `currentLevel` are the stored caught landmark / level code (initial
    // focus); `pokemonLevel` is the Pokémon's CURRENT level (R12-005: 0 = not judged).
    void openLocation(std::string_view sourceGameId, uint16_t species,
                      uint8_t currentLocation, uint8_t currentLevel,
                      uint8_t pokemonLevel = 0, bool unverified = false) {
        namespace MetRows = PokeBank::UIModel::ExactGameMetLocations;
        namespace Policy = PokeVault::Integration::MetLocationPolicy;
        kind = Kind::Location;
        locationSpecies = species;
        unverifiedList = unverified;
        unverifiedEntry = !unverified;
        encounterChoices.clear();
        levelBlocked.clear();
        otherLocations.clear();
        // Never a closed list: a species with no Crystal catch template (traded in, evolved, or
        // not wild in Crystal) still has the markers and every native landmark (unverified list).
        // One row per slot: Crystal lists a slot once per time of day; rows that differ only in
        // time are one row whose detail names every time ("morn/day"), so the list stays short.
        // Round 13 (owner decision 8): only rows with the SAME level range merge, so a place whose
        // times differ in level keeps one row per level, each naming its own times; the caught
        // time written for a row spanning several times is chosen by caughtForTemplate (stated).
        std::vector<Encounter::EncounterTemplate> all;
        for (const auto& choice : MetRows::templatesWithAncestry(sourceGameId, species, 251)) {
            const auto& e = choice.encounter;
            const auto same = std::find_if(all.begin(), all.end(), [&](const auto& a) {
                return a.species == e.species && a.location == e.location && a.method == e.method &&
                       a.minLevel == e.minLevel && a.maxLevel == e.maxLevel && a.timeMask != 0 && e.timeMask != 0;
            });
            if (same != all.end()) same->timeMask = static_cast<uint8_t>(same->timeMask | e.timeMask);
            else all.push_back(e);
        }
        for (const auto& encounter : all)
            (Policy::levelReachesEncounter(encounter.minLevel, pokemonLevel) ? encounterChoices : levelBlocked)
                .push_back(encounter);
        if (unverified) encounterChoices.clear();
        else levelBlocked.clear();
        const auto reachedAt = [&](uint16_t id) {
            return std::any_of(all.begin(), all.end(), [&](const auto& e) {
                return e.location == id && Policy::levelReachesEncounter(e.minLevel, pokemonLevel);
            });
        };
        const auto templateAt = [&](uint16_t id) {
            return std::any_of(all.begin(), all.end(), [id](const auto& e) { return e.location == id; });
        };
        for (const auto id : MetRows::crystalOfferedLocations()) {
            if (MetRows::crystalMarkerLocation(id) ? (unverified || reachedAt(id)) : (!unverified || templateAt(id)))
                continue;
            otherLocations.push_back(id);
        }
        index = 0;
        for (int i = 0; i < static_cast<int>(encounterChoices.size()); ++i) {
            const auto& encounter = encounterChoices[static_cast<std::size_t>(i)];
            if (encounter.location == currentLocation &&
                (currentLevel <= 1 || encounter.containsLevel(currentLevel))) {
                index = i;
                return;
            }
        }
        for (int i = 0; i < locationCount(); ++i)
            if (const auto other = otherLocationChoiceAt(i); other && currentLocation != 0 && *other == currentLocation) {
                index = i;
                return;
            }
    }
    int locationCount() const noexcept {
        return static_cast<int>(encounterChoices.size() + levelBlocked.size() + otherLocations.size()) +
               (unverifiedEntry ? 1 : 0);
    }
    // The verified catch template under the cursor (default list), or nullptr on any other row.
    const Encounter::EncounterTemplate* locationChoice() const noexcept {
        return index >= 0 && index < static_cast<int>(encounterChoices.size())
            ? &encounterChoices[static_cast<std::size_t>(index)] : nullptr;
    }
    // A template above the current level at `row` (unverified list), or nullptr.
    const Encounter::EncounterTemplate* levelBlockedAt(int row) const noexcept {
        const int blocked = row - static_cast<int>(encounterChoices.size());
        return row >= 0 && blocked >= 0 && blocked < static_cast<int>(levelBlocked.size())
            ? &levelBlocked[static_cast<std::size_t>(blocked)] : nullptr;
    }
    // "Other location (unverified)..." at `row`: not a location; it opens the unverified list.
    bool unverifiedEntryAt(int row) const noexcept { return unverifiedEntry && row == locationCount() - 1; }
    bool unverifiedEntrySelected() const noexcept { return unverifiedEntryAt(index); }
    // The landmark of a row that is not a verified template: a marker / other landmark, or the place
    // of a template above the current level (no template is applied; unverified history).
    std::optional<uint8_t> otherLocationChoiceAt(int row) const noexcept {
        if (const auto* blocked = levelBlockedAt(row)) return static_cast<uint8_t>(blocked->location);
        const int other = row - static_cast<int>(encounterChoices.size() + levelBlocked.size());
        if (row < 0 || other < 0 || other >= static_cast<int>(otherLocations.size())) return std::nullopt;
        return static_cast<uint8_t>(otherLocations[static_cast<std::size_t>(other)]);
    }
    std::optional<uint8_t> otherLocationChoice() const noexcept { return otherLocationChoiceAt(index); }
    void stepList(int delta) noexcept {
        const int count = kind == Kind::Species ? 251 : kind == Kind::Move ? 252 :
                          kind == Kind::Location ? locationCount() : 0;
        if (count == 0) return;
        if (kind == Kind::Species) { index = std::clamp(index + delta, 0, count - 1); return; }
        int next = (index + delta) % count;
        if (next < 0) next += count;
        index = next;
    }

};

inline bool applySpeciesChoice(Gen2PokemonEditor::Session& session, uint16_t species,
                               std::string_view sourceGameId = {}) noexcept {
    return session.setSpecies(species, sourceGameId);
}

// Appearance is translated into the native DVs, never a stored shiny flag.
inline bool applySpeciesAppearance(Gen2PokemonEditor::Session& session, uint16_t species,
                                   bool shiny, std::string_view sourceGameId = {}) noexcept {
    if (!session.setSpecies(species, sourceGameId)) return false;
    namespace G = PokeVault::Integration::Gen2;
    auto dvs = Gen2PokemonEditor::storedDVs(session.working);
    if (G::StagedEditor::isShinyDVs(dvs) != shiny) {
        if (!shiny) dvs[1] = 9; // breaks shiny while preserving Attack DV / gender
        else {
            const auto originalGender = G::genderFromAttackDV(species, dvs[0]);
            int best = 2, bestScore = 1000;
            for (uint8_t candidate : {2, 3, 6, 7, 10, 11, 14, 15}) {
                const auto gender = G::genderFromAttackDV(species, candidate);
                const int score = (gender == originalGender ? 0 : 100) +
                    std::abs(static_cast<int>(candidate) - static_cast<int>(dvs[0]));
                if (score < bestScore) { best = candidate; bestScore = score; }
            }
            dvs = {static_cast<uint8_t>(best), 10, 10, 10};
        }
        for (std::size_t i = 0; i < 4; ++i) session.working.dvs[i + 1] = dvs[i];
        session.working.dvs[0] = G::StagedEditor::derivedHPDV(dvs);
        session.working.shiny = G::StagedEditor::isShinyDVs(dvs);
        session.working.gender = static_cast<uint8_t>(G::genderFromAttackDV(species, dvs[0]));
        session.working.form = species == 201 ? static_cast<uint8_t>(Gen12DVReroll::unownLetter(dvs)) : 0;
    }
    return true;
}

// Preview exactly what the real A action would create, on a COPY of the
// editor session. Never change working DVs or source saves while browsing.
[[nodiscard]] inline int previewUnownForm(const Gen2PokemonEditor::Session& session,
                                          bool shiny, std::string_view sourceGameId = {}) noexcept {
    Gen2PokemonEditor::Session hypothetical = session;
    if (!applySpeciesAppearance(hypothetical, 201, shiny, sourceGameId)) return -1;
    return static_cast<int>(hypothetical.working.form);
}

// Re-selecting the already stored move is intentionally a no-op. This preserves unusual existing
// PP / PP-Up bytes rather than normalizing them just because the user opened and accepted a picker.
inline bool applyMoveChoice(Gen2PokemonEditor::Session& session, std::size_t slot,
                            uint16_t move) noexcept {
    if (!session.editable() || slot >= session.working.moves.size()) return false;
    if (session.working.moves[slot] == move) return true;
    return session.setMove(slot, move);
}

// The Crystal Met Location chooser is sized to its choices (most species have two to four
// encounter templates): up to 9 rows of 44 px, never a fixed full-height window. Draw and input
// both use this one geometry, so hit-testing always matches what is shown.
struct LocationPickerGeometry {
    int x = 0, y = 0, width = 640, height = 0;
    int listX = 0, listY = 0, listWidth = 0;
    int visible = 1;
    static constexpr int rowStep = 44;
    static constexpr int rowHeight = 40;
    // NEW-10 (Round 3 L02 note): title + caption end at 72 px, rows start at 80; 16 px under them.
    static constexpr int headerH = 80;
    static constexpr int padBottom = 16;
};

constexpr LocationPickerGeometry locationPickerGeometry(int count, int screenW = 1280, int screenH = 720,
                                                        int footerH = 56) noexcept {
    LocationPickerGeometry g{};
    g.visible = std::clamp(count, 1, 9);
    g.height = LocationPickerGeometry::headerH + g.visible * LocationPickerGeometry::rowStep +
               LocationPickerGeometry::padBottom;
    g.x = (screenW - g.width) / 2;
    g.y = (screenH - footerH - g.height) / 2;
    g.listX = g.x + 20;
    g.listY = g.y + LocationPickerGeometry::headerH;
    g.listWidth = g.width - 40;
    return g;
}

// One Crystal encounter row: the landmark name, then the method, level range and time of day in a
// quieter caption. Names only; never a raw landmark index, method code or source tag.
struct LocationRowText {
    std::string place;
    std::string detail;
};

inline LocationRowText locationRowText(const Encounter::EncounterTemplate& encounter) {
    LocationRowText out;
    out.place = Gen2Native::crystalCaughtLocationName(static_cast<uint8_t>(encounter.location));
    out.detail = std::string(Encounter::methodName(encounter.method)) + " · Lv. " +
                 std::to_string(encounter.minLevel);
    // Round 13: an in-game trade arrives at the traded Pokémon's level (PKHeX EncounterTrade2:
    // LevelMin = Level, LevelMax = 100), so it reads "Lv. 5+".
    if (encounter.maxLevel == 100 && encounter.minLevel < 100) out.detail += "+";
    else if (encounter.maxLevel != encounter.minLevel)
        out.detail += "–" + std::to_string(encounter.maxLevel);
    // Round 13 (owner decision 8; was: a slot met at every time showed no time, Round 12): every
    // timed row names the times it occurs, short so the row fits ("morn/day", "night", "any time").
    const auto time = encounterTimeShortLabel(encounter.timeMask);
    if (!time.empty()) out.detail += " · " + time;
    return out;
}

// Round 13 (owner decisions 7 / 8): the Crystal caught data a verified row writes (the one rule the
// chooser overlay and the tests use).
//  - A gift / trade Crystal records at LANDMARK_GIFT (0x7E) is written as the game writes it
//    (pret engine/pokemon/caught_data.asm SetGiftMonCaughtData): caught level 0 and time 0.
//  - Otherwise the level code is the row's minimum level (1 = the egg marker) and the time is
//    the pending Met Time when the row occurs at it; when it does not (or none is pending), the
//    row's first time is written (Morning for a row met at any time) and `status` STATES the
//    time written and the row's times, so a grouped row never changes the time silently. The
//    editor's Time row then offers only the times the row occurs at.
struct CaughtChoice {
    Gen2Native::CrystalCaughtData caught;
    bool timeDefaulted = false;
    std::string status;
};
inline CaughtChoice caughtForTemplate(const Encounter::EncounterTemplate& encounter, uint8_t pendingTime,
                                      bool otFemale) {
    namespace Gifts = PokeVault::Integration::GiftStaticEncounters;
    CaughtChoice out;
    out.caught.present = true;
    out.caught.location = static_cast<uint8_t>(encounter.location);
    out.caught.originalTrainerFemale = otFemale;
    if (const auto* gift = Gifts::rowOf(encounter); gift && (gift->flags & Gifts::kCaughtLevelUnrecorded)) {
        out.caught.levelCode = 0;
        out.caught.timeOfDay = 0;
        out.status = std::string("Met as a Crystal ") + Gifts::kindLabel(gift->kind) +
                     ": level and time not recorded";
        return out;
    }
    out.caught.levelCode = encounter.method == Encounter::Method::Egg
        ? 1 : static_cast<uint8_t>(std::clamp<int>(encounter.minLevel, 2, 63));
    uint8_t time = pendingTime;
    if (time < 1 || time > 3 || !Encounter::timeAllowed(encounter, time)) {
        out.timeDefaulted = true;
        for (uint8_t value = 1; value <= 3; ++value)
            if (Encounter::timeAllowed(encounter, value)) { time = value; break; }
    }
    out.caught.timeOfDay = time;
    const auto times = encounterTimeShortLabel(encounter.timeMask);
    const int timeCount = ((encounter.timeMask >> 1) & 1) + ((encounter.timeMask >> 2) & 1) + ((encounter.timeMask >> 3) & 1);
    const bool severalTimes = encounter.timeMask == 0 || timeCount > 1;
    if (out.timeDefaulted || severalTimes)
        out.status = std::string("Met time: ") + Gen2Native::crystalMetTimeName(time) + " (" +
                     (times.empty() ? std::string("any time") : times) + "); change it on the Time row";
    return out;
}

// Row `row` of the open Crystal Met Location chooser: a verified catch template (an ancestry row
// says the species it was met as), a template above the current level (says so), a Gift / Event
// marker or other native landmark (unverified), or the "Other location (unverified)..." row.
inline LocationRowText locationRowTextAt(const Model& model, int row) {
    namespace Policy = PokeVault::Integration::MetLocationPolicy;
    const auto via = [&](const Encounter::EncounterTemplate& encounter, LocationRowText text) {
        if (model.locationSpecies != 0 && encounter.species != model.locationSpecies)
            text.detail += " · via " + std::string(Names::getSpeciesName(encounter.species));
        return text;
    };
    if (model.unverifiedEntryAt(row)) return {Policy::kUnverifiedListRowPlace, Policy::kUnverifiedListRowDetail};
    if (row >= 0 && row < static_cast<int>(model.encounterChoices.size())) {
        const auto& encounter = model.encounterChoices[static_cast<std::size_t>(row)];
        auto text = via(encounter, locationRowText(encounter));
        // Owner decision (Nov 2): the encounter is in the game, its access (GS Ball) is not
        // verifiable from the save -- listed, never as a verified acquisition.
        namespace Gifts = PokeVault::Integration::GiftStaticEncounters;
        if (const auto* gift = Gifts::rowOf(encounter); gift && !Gifts::fullyVerifiable(*gift))
            text.detail += std::string("; ") + Gifts::kAccessUnverified;
        return text;
    }
    if (const auto* blocked = model.levelBlockedAt(row)) {
        // R12-005: kept, never offered as verified; method and time dropped so the reason fits.
        LocationRowText text{Gen2Native::crystalCaughtLocationName(static_cast<uint8_t>(blocked->location)),
                             "Lv. " + std::to_string(blocked->minLevel) + "+"};   // the minimum the level misses
        text = via(*blocked, text);
        text.detail += std::string("; ") + Policy::kLevelBelowEncounter;
        return text;
    }
    const auto location = model.otherLocationChoiceAt(row);
    if (!location) return {"Unknown location", {}};
    return {Gen2Native::crystalCaughtLocationName(*location),
            PokeBank::UIModel::ExactGameMetLocations::crystalOtherDetail(*location)};
}

} // namespace PokeBank::UIModel::Gen2PokemonPicker
