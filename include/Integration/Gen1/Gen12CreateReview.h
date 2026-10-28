#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

// Round 6 P3 (owner Oct 25; CG74 row shape agreed with lane S): the Review of a Pokemon created in a
// Gen I / II staged session lists what was created, not only "Empty -> species". The header row is
// "<loc> / Add Pokémon" with "Empty -> <Species> Lv <n>"; then one named row per stored, non-default
// value (Moves, Held Item, OT / TID, DVs, Shiny, Stat Exp, ...). These helpers only format values the
// verified created record already holds; no field the format does not store is shown.
namespace PokeVault::Integration::Gen12CreateReview {

inline constexpr std::string_view kAddPokemon = "Add Pokémon";
// A Pokemon placed from a native file (Gen I .pk1; Gen II says "Imported" in its own rows).
inline constexpr std::string_view kImported = "Imported";

inline std::string header(std::string_view species, unsigned level) {
    return std::string(species) + " Lv " + std::to_string(level);
}

// "Thunder Shock, Growl"; empty when the created record has no moves.
template <class Name>
std::string moves(const std::array<uint8_t, 4>& ids, Name&& name) {
    std::string out;
    for (const uint8_t id : ids) {
        if (!id) continue;
        if (!out.empty()) out += ", ";
        out += name(id);
    }
    return out;
}

// "Atk 9, Spc 12": only non-zero values, in native order; empty when every value is zero.
template <class T, std::size_t N>
std::string nonZero(const std::array<const char*, N>& labels, const std::array<T, N>& values) {
    std::string out;
    for (std::size_t i = 0; i < N; ++i) {
        if (!values[i]) continue;
        if (!out.empty()) out += ", ";
        out += std::string(labels[i]) + " " + std::to_string(values[i]);
    }
    return out;
}

inline std::string trainer(std::string_view ot, uint16_t tid) { return std::string(ot) + " / " + std::to_string(tid); }

} // namespace PokeVault::Integration::Gen12CreateReview
