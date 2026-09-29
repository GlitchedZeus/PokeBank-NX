#pragma once

#include "Legality/Gen4WildEncounter.h"

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace Legality::Gen4Static {

// Packed layout:
//   species      [0..8]   (9 bits)
//   location     [9..20]  (12 bits)
//   level        [21..27] (7 bits)
//   form         [28..35] (8 bits)
//   egg location [36..47] (12 bits)
//   roaming      [48]     (1 bit)
//   game         [49..51] (3 bits; Gen4Wild::Game)
#include "Legality/Gen4StaticEncounterData.inc"

constexpr uint16_t species(uint64_t v) noexcept {
    return static_cast<uint16_t>(v & 0x1FFu);
}
constexpr uint16_t location(uint64_t v) noexcept {
    return static_cast<uint16_t>((v >> 9) & 0x0FFFu);
}
constexpr uint8_t level(uint64_t v) noexcept {
    return static_cast<uint8_t>((v >> 21) & 0x7Fu);
}
constexpr uint8_t form(uint64_t v) noexcept {
    return static_cast<uint8_t>((v >> 28) & 0xFFu);
}
constexpr uint16_t eggLocation(uint64_t v) noexcept {
    return static_cast<uint16_t>((v >> 36) & 0x0FFFu);
}
constexpr bool roaming(uint64_t v) noexcept {
    return ((v >> 48) & 1u) != 0;
}
constexpr Gen4Wild::Game game(uint64_t v) noexcept {
    return static_cast<Gen4Wild::Game>((v >> 49) & 0x07u);
}

inline bool hasSpecies(std::string_view exactGameId, uint16_t speciesId) noexcept {
    const auto wanted = Gen4Wild::gameForId(exactGameId);
    if (wanted == Gen4Wild::Game::Invalid || speciesId == 0) return false;
    for (const uint64_t row : kPackedGen4StaticEncounters)
        if (game(row) == wanted && species(row) == speciesId) return true;
    return false;
}

inline bool matches(std::string_view exactGameId, uint16_t speciesId,
                    uint16_t metLocation, uint8_t metLevel, uint8_t pokemonForm,
                    uint16_t pokemonEggLocation) noexcept {
    const auto wanted = Gen4Wild::gameForId(exactGameId);
    if (wanted == Gen4Wild::Game::Invalid || speciesId == 0 || metLevel == 0)
        return false;
    for (const uint64_t row : kPackedGen4StaticEncounters) {
        if (game(row) != wanted || species(row) != speciesId)
            continue;
        if (location(row) != metLocation || level(row) != metLevel ||
            form(row) != pokemonForm)
            continue;
        const uint16_t expectedEgg = eggLocation(row);
        if (expectedEgg != 0 && expectedEgg != pokemonEggLocation)
            continue;
        return true;
    }
    return false;
}

inline std::size_t countForGame(std::string_view exactGameId) noexcept {
    const auto wanted = Gen4Wild::gameForId(exactGameId);
    if (wanted == Gen4Wild::Game::Invalid) return 0;
    std::size_t count = 0;
    for (const uint64_t row : kPackedGen4StaticEncounters)
        if (game(row) == wanted) ++count;
    return count;
}

} // namespace Legality::Gen4Static
