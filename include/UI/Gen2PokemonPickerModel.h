#pragma once

#include "UI/Gen2NativePresentation.h"
#include "UI/Gen2PokemonSession.h"
#include "Integration/Encounter/EncounterGuardrails.h"

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

    bool active() const noexcept { return kind != Kind::None; }
    void close() noexcept { kind = Kind::None; encounterChoices.clear(); }

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
        index = 0;
        for (int i = 0; i < static_cast<int>(encounterChoices.size()); ++i) {
            const auto& encounter = encounterChoices[static_cast<std::size_t>(i)];
            if (encounter.location == currentLocation &&
                (currentLevel <= 1 || encounter.containsLevel(currentLevel))) {
                index = i;
                break;
            }
        }
    }
    int locationCount() const noexcept { return static_cast<int>(encounterChoices.size()); }
    const Encounter::EncounterTemplate* locationChoice() const noexcept {
        return index >= 0 && index < static_cast<int>(encounterChoices.size())
            ? &encounterChoices[static_cast<std::size_t>(index)] : nullptr;
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
    }
    return true;
}

// Re-selecting the already stored move is intentionally a no-op. This preserves unusual existing
// PP / PP-Up bytes rather than normalizing them just because the user opened and accepted a picker.
inline bool applyMoveChoice(Gen2PokemonEditor::Session& session, std::size_t slot,
                            uint16_t move) noexcept {
    if (!session.editable() || slot >= session.working.moves.size()) return false;
    if (session.working.moves[slot] == move) return true;
    return session.setMove(slot, move);
}

} // namespace PokeBank::UIModel::Gen2PokemonPicker
