#pragma once

// Shared species-change move policy, Generation I-V (R2-BUG-004).
//
// On a REAL species change every editor replaces all four moves with the new species' native
// default moveset at the draft's level (Pokemon/SpeciesMoveDefaults.h: pret WriteMonMoves /
// FillMoves / GiveMonInitialMoveset, PKHeX Learnset.SetEncounterMoves), current PP = the
// move's base PP for that generation and PP Ups = 0 (the games give a new move base PP and
// no PP Ups). Reselecting the same species changes nothing, and Cancel never reaches this.
// A species with no learnset row gets four EMPTY slots: the old species' moves are never
// carried over, because they are the source of "Invalid" moves after a species change.
//
// Only the staged draft is touched; nothing here reads or writes a save.

#include "Enums/GameVersion.h"
#include "Pokemon/SpeciesMoveDefaults.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <type_traits>

namespace PokeBank::UIModel::SpeciesMoveDefaults {

struct Slots {
    std::array<uint16_t, 4> moves{};
    std::array<uint8_t, 4> pp{};
    std::array<uint8_t, 4> ppUps{};
    bool known = false;  // false: no learnset row, all four slots empty
};

// basePP(moveId) -> that generation's base PP (Gen I StagedPokemonEditor::moveBasePP, Gen II
// gen2MoveBasePP, Gen III/IV Names::getMoveBasePP, Gen V basePP5).
template <class BasePP>
[[nodiscard]] Slots forSpecies(::PokeBank::Moves::LevelUpTable15 table, uint16_t species, uint8_t form,
                               uint8_t level, BasePP&& basePP) {
    const ::PokeBank::Moves::DefaultMoves defaults = ::PokeBank::Moves::defaultMoves(table, species, form, level);
    Slots slots;
    slots.known = defaults.known;
    for (std::size_t i = 0; i < 4; ++i) {
        slots.moves[i] = defaults.moves[i];
        slots.pp[i] = defaults.moves[i] ? static_cast<uint8_t>(basePP(defaults.moves[i])) : uint8_t{0};
    }
    return slots;
}

// Array-backed drafts (Gen I BoxPokemonCreate/Edit, Gen II PokemonRecord, Gen III
// StagedPokemonRecord): moves / pp / ppUps are four-element arrays of any integer type.
template <class MoveArray, class PPArray, class UpsArray>
void writeArrays(const Slots& slots, MoveArray& moves, PPArray& pp, UpsArray& ppUps) {
    for (std::size_t i = 0; i < 4; ++i) {
        moves[i] = static_cast<std::decay_t<decltype(moves[i])>>(slots.moves[i]);
        pp[i] = static_cast<std::decay_t<decltype(pp[i])>>(slots.pp[i]);
        ppUps[i] = static_cast<std::decay_t<decltype(ppUps[i])>>(slots.ppUps[i]);
    }
}

// The one call an array-backed apply path makes: refresh all four slots for the new species.
template <class MoveArray, class PPArray, class UpsArray, class BasePP>
bool refreshArrays(::PokeBank::Moves::LevelUpTable15 table, uint16_t species, uint8_t form, uint8_t level,
                   BasePP&& basePP, MoveArray& moves, PPArray& pp, UpsArray& ppUps) {
    const Slots slots = forSpecies(table, species, form, level, basePP);
    writeArrays(slots, moves, pp, ppUps);
    return slots.known;
}

// Gen I/II edit requests carry optional arrays: fill all three from the slots.
template <class Edit>
void writeEdit(const Slots& slots, Edit& edit) {
    typename std::decay_t<decltype(edit.moves)>::value_type moves{};
    typename std::decay_t<decltype(edit.pp)>::value_type pp{};
    typename std::decay_t<decltype(edit.ppUps)>::value_type ppUps{};
    writeArrays(slots, moves, pp, ppUps);
    edit.moves = moves;
    edit.pp = pp;
    edit.ppUps = ppUps;
}

// Gen IV Pokemon4Mutable-style records: setMove(slot, id) writes the move with base PP and no
// PP Ups. Stops at the first refusal so the caller can roll the whole draft back.
template <class Record>
[[nodiscard]] bool setMoves(Record& record, const Slots& slots) {
    for (std::size_t slot = 0; slot < 4; ++slot)
        if (!record.setMove(slot, slots.moves[slot])) return false;
    return true;
}

// Gen V StagedPokemon5Record / Gen5SharedPokemonSession: Move, then PP Ups 0, then PP, per slot
// -- the order Keep and Review replay, so the replay reproduces the staged bytes exactly.
template <auto MoveField, auto UpsField, auto PPField, class Draft>
[[nodiscard]] bool stageMoves(Draft& draft, const Slots& slots, std::string& error) {
    for (std::size_t slot = 0; slot < 4; ++slot) {
        if (!draft.stage(MoveField, slot, slots.moves[slot], error) ||
            !draft.stage(UpsField, slot, 0u, error) ||
            !draft.stage(PPField, slot, slots.pp[slot], error))
            return false;
    }
    return true;
}

// Exact-game table selection from what each editor knows.
[[nodiscard]] constexpr uint8_t gen3VersionForSourceId(std::string_view id, uint8_t originFallback) noexcept {
    return id == "sapphire_gba" ? 1 : id == "ruby_gba" ? 2 : id == "emerald_gba" ? 3
         : id == "firered_gba" ? 4 : id == "leafgreen_gba" ? 5 : originFallback;
}
[[nodiscard]] constexpr ::PokeBank::Moves::LevelUpTable15 gen4TableForGroup(::Enums::GameVersion group) noexcept {
    using T = ::PokeBank::Moves::LevelUpTable15;
    return group == ::Enums::GameVersion::DP ? T::DiamondPearl
         : group == ::Enums::GameVersion::PT ? T::Platinum
         : group == ::Enums::GameVersion::HGSS ? T::HeartGoldSoulSilver : T::Count;
}
[[nodiscard]] constexpr ::PokeBank::Moves::LevelUpTable15 gen5Table(bool black2White2) noexcept {
    return black2White2 ? ::PokeBank::Moves::LevelUpTable15::Black2White2
                        : ::PokeBank::Moves::LevelUpTable15::BlackWhite;
}

inline constexpr const char* kMovesRefreshedStatus = "Moves reset to the new species' level-up moves";

} // namespace PokeBank::UIModel::SpeciesMoveDefaults
