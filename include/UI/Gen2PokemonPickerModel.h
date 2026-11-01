#pragma once

#include "UI/Gen12DVReroll.h"
#include "UI/Gen2NativePresentation.h"
#include "UI/Gen2PokemonSession.h"
#include "Integration/Encounter/EncounterGuardrails.h"
#include "UI/ExactGameMetLocations.h"

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

enum class Kind : uint8_t { None, Species, Move, Location };

struct Model {
    Kind kind = Kind::None;
    int index = 0;
    bool previewShiny = false;
    std::size_t moveSlot = 0;
    std::vector<Encounter::EncounterTemplate> encounterChoices;
    // Round 11 FRLG / Crystal: the other native Crystal landmarks, after the catch templates
    // (ExactGameMetLocations.h). Row i >= encounterChoices.size() is otherLocations[i - size].
    std::vector<uint16_t> otherLocations;

    bool active() const noexcept { return kind != Kind::None; }
    void close() noexcept { kind = Kind::None; encounterChoices.clear(); otherLocations.clear(); }

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

    void openLocation(std::string_view sourceGameId, uint16_t species,
                      uint8_t currentLocation, uint8_t currentLevel) {
        kind = Kind::Location;
        encounterChoices = Encounter::forGameSpecies(sourceGameId, species);
        // Never a closed list: a species with no Crystal catch template (traded in, evolved, or
        // not wild in Crystal) still gets every native landmark, labelled as unverified history.
        namespace MetRows = PokeBank::UIModel::ExactGameMetLocations;
        otherLocations.clear();
        for (const auto row : MetRows::otherNativeRows(
                 MetRows::crystalOfferedLocations(),
                 [&](uint16_t id) {
                     return std::any_of(encounterChoices.begin(), encounterChoices.end(),
                                        [id](const auto& e) { return e.location == id; });
                 },
                 false, 0))
            otherLocations.push_back(MetRows::otherNativeRowLocation(row));
        index = 0;
        for (int i = 0; i < static_cast<int>(encounterChoices.size()); ++i) {
            const auto& encounter = encounterChoices[static_cast<std::size_t>(i)];
            if (encounter.location == currentLocation &&
                (currentLevel <= 1 || encounter.containsLevel(currentLevel))) {
                index = i;
                return;
            }
        }
        for (std::size_t i = 0; i < otherLocations.size(); ++i)
            if (currentLocation != 0 && otherLocations[i] == currentLocation) {
                index = static_cast<int>(encounterChoices.size() + i);
                return;
            }
    }
    int locationCount() const noexcept {
        return static_cast<int>(encounterChoices.size() + otherLocations.size());
    }
    // The catch template under the cursor, or nullptr on an other-native row.
    const Encounter::EncounterTemplate* locationChoice() const noexcept {
        return index >= 0 && index < static_cast<int>(encounterChoices.size())
            ? &encounterChoices[static_cast<std::size_t>(index)] : nullptr;
    }
    // The landmark of an other-native row under the cursor (no template; unverified history).
    std::optional<uint8_t> otherLocationChoice() const noexcept {
        const int other = index - static_cast<int>(encounterChoices.size());
        if (index < 0 || other < 0 || other >= static_cast<int>(otherLocations.size())) return std::nullopt;
        return static_cast<uint8_t>(otherLocations[static_cast<std::size_t>(other)]);
    }
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
    if (encounter.maxLevel != encounter.minLevel)
        out.detail += "–" + std::to_string(encounter.maxLevel);
    const auto time = encounterTimeLabel(encounter.timeMask);
    if (!time.empty()) out.detail += " · " + time;
    return out;
}

// Row `row` of the open Crystal Met Location chooser: a catch template, or an other native landmark
// whose detail says it is not a listed catch (or names the Gift / Event marker), never a catch.
inline LocationRowText locationRowTextAt(const Model& model, int row) {
    if (row >= 0 && row < static_cast<int>(model.encounterChoices.size()))
        return locationRowText(model.encounterChoices[static_cast<std::size_t>(row)]);
    const int other = row - static_cast<int>(model.encounterChoices.size());
    if (row < 0 || other >= static_cast<int>(model.otherLocations.size())) return {"Unknown location", {}};
    const auto location = model.otherLocations[static_cast<std::size_t>(other)];
    return {Gen2Native::crystalCaughtLocationName(static_cast<uint8_t>(location)),
            PokeBank::UIModel::ExactGameMetLocations::crystalOtherDetail(location)};
}

} // namespace PokeBank::UIModel::Gen2PokemonPicker
