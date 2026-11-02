#pragma once

// Shared species-change move policy, Generation I-V.
//
// Round 11 (owner H28 / Extra 5 / Extra 6, replacing the round 3 R2-BUG-004 refill): a species
// change NEVER adds a move. Each of the four current moves that the new species can use in this
// exact game is kept with its own PP and PP Ups; every other move is cleared, and the kept moves
// move up so the empty slots are at the end. That keeps H28 ("changing species clears
// incompatible moves": no "Invalid" move survives) without inventing a moveset the user did not
// pick. A blank Create draft therefore stays blank. Reselecting the same species changes nothing,
// and Cancel never reaches this. The "can use" predicate is each generation's own exact-game
// compatibility table (Gen1/Gen2 MoveCompatibility, Gen3 Learnset, Gen4/Gen5MoveCompatibility).
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
};

// The slots after a real species change: the old moves the new species can learn (canLearn(move)),
// in their old order, each with its stored PP and PP Ups; the rest empty (move 0, PP 0, PP Ups 0).
template <class MoveArray, class PPArray, class UpsArray, class CanLearn>
[[nodiscard]] Slots keepLearnable(const MoveArray& moves, const PPArray& pp, const UpsArray& ppUps,
                                  CanLearn&& canLearn) {
    Slots slots;
    std::size_t next = 0;
    for (std::size_t i = 0; i < 4; ++i) {
        const auto move = static_cast<uint16_t>(moves[i]);
        if (move == 0 || !canLearn(move)) continue;
        slots.moves[next] = move;
        slots.pp[next] = static_cast<uint8_t>(pp[i]);
        slots.ppUps[next] = static_cast<uint8_t>(ppUps[i]);
        ++next;
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

// The one call an array-backed apply path makes: keep the learnable moves in place.
template <class MoveArray, class PPArray, class UpsArray, class CanLearn>
void refreshArrays(MoveArray& moves, PPArray& pp, UpsArray& ppUps, CanLearn&& canLearn) {
    const Slots slots = keepLearnable(moves, pp, ppUps, canLearn);
    writeArrays(slots, moves, pp, ppUps);
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
// PP Ups; a record that also offers setPPUps / setPP then gets the kept move's own values back.
// Stops at the first refusal so the caller can roll the whole draft back.
template <class Record>
[[nodiscard]] bool setMoves(Record& record, const Slots& slots) {
    for (std::size_t slot = 0; slot < 4; ++slot) {
        if (!record.setMove(slot, slots.moves[slot])) return false;
        if constexpr (requires { record.setPPUps(slot, uint8_t{}); record.setPP(slot, uint8_t{}); }) {
            if (slots.moves[slot] != 0 &&
                (!record.setPPUps(slot, slots.ppUps[slot]) || !record.setPP(slot, slots.pp[slot])))
                return false;
        }
    }
    return true;
}

// Gen V StagedPokemon5Record / Gen5SharedPokemonSession: Move, then PP Ups, then PP, per slot
// -- the order Keep and Review replay, so the replay reproduces the staged bytes exactly.
template <auto MoveField, auto UpsField, auto PPField, class Draft>
[[nodiscard]] bool stageMoves(Draft& draft, const Slots& slots, std::string& error) {
    for (std::size_t slot = 0; slot < 4; ++slot) {
        if (!draft.stage(MoveField, slot, slots.moves[slot], error) ||
            !draft.stage(UpsField, slot, slots.ppUps[slot], error) ||
            !draft.stage(PPField, slot, slots.pp[slot], error))
            return false;
    }
    return true;
}

// Exact-game identification from what each editor knows.
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

} // namespace PokeBank::UIModel::SpeciesMoveDefaults
