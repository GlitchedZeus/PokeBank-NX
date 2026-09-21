#pragma once

#include "UI/Gen2NativePresentation.h"
#include "UI/Gen2PokemonSession.h"
#include "Integration/Encounter/EncounterGuardrails.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <optional>
#include <string_view>
#include <vector>

namespace PokeBank::UIModel::Gen2PokemonPicker {
namespace Encounter = PokeVault::Integration::EncounterGuardrails;

enum class Kind : uint8_t { None, Species, Move, Pokerus, Location };
enum class PokerusMode : uint8_t { None, Active, Cured };
inline constexpr int pokerusFieldRows = 3;
inline constexpr int pokerusApplyRow = 3;
inline constexpr int pokerusRowCount = 4;

struct Model {
    Kind kind = Kind::None;
    int index = 0;
    bool previewShiny = false;
    std::size_t moveSlot = 0;
    PokerusMode pokerusMode = PokerusMode::None;
    uint8_t strain = 1;
    uint8_t days = 1;
    int pokerusRow = 0;
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

    void openPokerus(uint8_t raw) noexcept {
        kind = Kind::Pokerus;
        const auto decoded = Gen2Native::decodePokerus(raw);
        pokerusMode = !decoded.present ? PokerusMode::None
                    : decoded.active ? PokerusMode::Active : PokerusMode::Cured;
        strain = decoded.strain == 0 ? 1 : decoded.strain;
        days = decoded.days == 0 ? 1 : decoded.days;
        pokerusRow = 0;
    }

    void stepPokerusRow(int delta) noexcept {
        int next = (pokerusRow + delta) % pokerusRowCount;
        if (next < 0) next += pokerusRowCount;
        pokerusRow = next;
    }

    void adjustPokerus(int delta) noexcept {
        if (pokerusRow == pokerusApplyRow) return;
        if (pokerusRow == 0) {
            int value = static_cast<int>(pokerusMode);
            value = (value + delta) % 3;
            if (value < 0) value += 3;
            pokerusMode = static_cast<PokerusMode>(value);
            return;
        }
        if (pokerusRow == 1) {
            int value = (static_cast<int>(strain) - 1 + delta) % 15;
            if (value < 0) value += 15;
            strain = static_cast<uint8_t>(value + 1);
            return;
        }
        int value = (static_cast<int>(days) - 1 + delta) % 15;
        if (value < 0) value += 15;
        days = static_cast<uint8_t>(value + 1);
    }

    void setPokerusStrain(uint8_t value) noexcept {
        strain = std::clamp<uint8_t>(value, 1, 15);
    }
    void setPokerusDays(uint8_t value) noexcept {
        days = std::clamp<uint8_t>(value, 1, 15);
    }
    bool pokerusApplyFocused() const noexcept { return pokerusRow == pokerusApplyRow; }

    uint8_t pokerusRaw() const noexcept {
        switch (pokerusMode) {
            case PokerusMode::None: return 0;
            case PokerusMode::Cured: return Gen2Native::encodePokerus(strain, 0);
            case PokerusMode::Active: return Gen2Native::encodePokerus(strain, days);
        }
        return 0;
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

inline bool applyPokerusChoice(Gen2PokemonEditor::Session& session, uint8_t raw) noexcept {
    if (!session.editable()) return false;
    session.working.pokerus = raw;
    return true;
}

constexpr const char* pokerusModeText(PokerusMode mode) noexcept {
    switch (mode) {
        case PokerusMode::None: return "None";
        case PokerusMode::Active: return "Active";
        case PokerusMode::Cured: return "Cured / inactive";
    }
    return "None";
}

} // namespace PokeBank::UIModel::Gen2PokemonPicker
