#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace Legality::Gen4PokewalkerEncounter {

// Packed layout:
// species [0..8], level [9..15], gender [16..17], course [18..22], slot [23..25],
// moves 1..4 [26..61] as four 9-bit move ids.
#include "Legality/Gen4PokewalkerEncounterData.inc"

struct Evidence {
    bool matched = false;
    uint8_t course = 0;
    uint8_t slot = 0;
};

constexpr uint16_t species(uint64_t v) noexcept { return static_cast<uint16_t>(v & 0x1FFu); }
constexpr uint8_t level(uint64_t v) noexcept { return static_cast<uint8_t>((v >> 9) & 0x7Fu); }
constexpr uint8_t gender(uint64_t v) noexcept { return static_cast<uint8_t>((v >> 16) & 0x03u); }
constexpr uint8_t course(uint64_t v) noexcept { return static_cast<uint8_t>((v >> 18) & 0x1Fu); }
constexpr uint8_t slot(uint64_t v) noexcept { return static_cast<uint8_t>((v >> 23) & 0x07u); }
constexpr uint16_t move(uint64_t v, int index) noexcept {
    return index >= 0 && index < 4
        ? static_cast<uint16_t>((v >> (26 + index * 9)) & 0x1FFu) : 0;
}

inline bool hasSpecies(uint16_t speciesId) noexcept {
    if (speciesId == 0) return false;
    for (const uint64_t row : kPackedGen4PokewalkerEncounters)
        if (species(row) == speciesId) return true;
    return false;
}

inline Evidence match(uint16_t speciesId, uint8_t metLevel, uint8_t pokemonGender,
                      const std::array<uint16_t, 4>& moves) noexcept {
    for (const uint64_t row : kPackedGen4PokewalkerEncounters) {
        if (species(row) != speciesId || level(row) != metLevel || gender(row) != pokemonGender)
            continue;

        // A nonzero fixed PokéWalker move must still be present on an untouched encounter,
        // but edited/leveled Pokémon may legitimately forget it. Treat moves as positive
        // narrowing evidence only: reject only when all four current moves are nonzero and
        // none can match any nonzero template move.
        bool templateHasMove = false;
        bool sharesMove = false;
        for (int tm = 0; tm < 4; ++tm) {
            const uint16_t expected = move(row, tm);
            if (expected == 0) continue;
            templateHasMove = true;
            for (const uint16_t current : moves)
                if (current != 0 && current == expected) sharesMove = true;
        }
        bool currentHasMove = false;
        for (const uint16_t current : moves)
            if (current != 0) currentHasMove = true;
        if (templateHasMove && currentHasMove && !sharesMove)
            continue;

        return {true, course(row), slot(row)};
    }
    return {};
}

inline std::size_t count() noexcept {
    return sizeof(kPackedGen4PokewalkerEncounters) /
           sizeof(kPackedGen4PokewalkerEncounters[0]);
}

} // namespace Legality::Gen4PokewalkerEncounter
