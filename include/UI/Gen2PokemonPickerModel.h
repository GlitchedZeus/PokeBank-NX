#pragma once

#include "UI/Gen2NativePresentation.h"
#include "UI/Gen2PokemonSession.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>

namespace PokeBank::UIModel::Gen2PokemonPicker {

enum class Kind : uint8_t { None, Species, Move, Pokerus };
enum class PokerusMode : uint8_t { None, Active, Cured };

struct Model {
    Kind kind = Kind::None;
    int index = 0;
    std::size_t moveSlot = 0;
    PokerusMode pokerusMode = PokerusMode::None;
    uint8_t strain = 1;
    uint8_t days = 1;
    int pokerusRow = 0;

    bool active() const noexcept { return kind != Kind::None; }
    void close() noexcept { kind = Kind::None; }

    void openSpecies(uint16_t current) noexcept {
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

    void stepList(int delta) noexcept {
        const int count = kind == Kind::Species ? 251 : kind == Kind::Move ? 252 : 0;
        if (count == 0) return;
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
        int next = (pokerusRow + delta) % 3;
        if (next < 0) next += 3;
        pokerusRow = next;
    }

    void adjustPokerus(int delta) noexcept {
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

    uint8_t pokerusRaw() const noexcept {
        switch (pokerusMode) {
            case PokerusMode::None: return 0;
            case PokerusMode::Cured: return Gen2Native::encodePokerus(strain, 0);
            case PokerusMode::Active: return Gen2Native::encodePokerus(strain, days);
        }
        return 0;
    }
};

inline bool applySpeciesChoice(Gen2PokemonEditor::Session& session, uint16_t species) noexcept {
    return session.setSpecies(species);
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
