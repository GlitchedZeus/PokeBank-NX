#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace Legality::Gen4Wild {

enum class Game : uint8_t {
    Diamond = 0,
    Pearl = 1,
    Platinum = 2,
    HeartGold = 3,
    SoulSilver = 4,
    Invalid = 0xFF,
};

constexpr Game gameForId(std::string_view id) noexcept {
    if (id == "diamond_nds") return Game::Diamond;
    if (id == "pearl_nds") return Game::Pearl;
    if (id == "platinum_nds") return Game::Platinum;
    if (id == "heartgold_nds") return Game::HeartGold;
    if (id == "soulsilver_nds") return Game::SoulSilver;
    return Game::Invalid;
}

// Packed layout:
//   species  [0..8]   (9 bits)
//   location [9..16]  (8 bits; Gen IV wild locations are <=255)
//   min lvl  [17..23] (7 bits)
//   max lvl  [24..30] (7 bits)
//   method   [31..34] (4 bits, PKHeX SlotType4)
//   form     [35..42] (8 bits; >=30 is PKHeX dynamic/random form sentinel)
//   game     [43..45] (3 bits)
#include "Legality/Gen4WildEncounterData.inc"

constexpr uint16_t species(uint64_t v) noexcept {
    return static_cast<uint16_t>(v & 0x1FFu);
}
constexpr uint8_t location(uint64_t v) noexcept {
    return static_cast<uint8_t>((v >> 9) & 0xFFu);
}
constexpr uint8_t minLevel(uint64_t v) noexcept {
    return static_cast<uint8_t>((v >> 17) & 0x7Fu);
}
constexpr uint8_t maxLevel(uint64_t v) noexcept {
    return static_cast<uint8_t>((v >> 24) & 0x7Fu);
}
constexpr uint8_t method(uint64_t v) noexcept {
    return static_cast<uint8_t>((v >> 31) & 0x0Fu);
}
constexpr uint8_t form(uint64_t v) noexcept {
    return static_cast<uint8_t>((v >> 35) & 0xFFu);
}
constexpr Game game(uint64_t v) noexcept {
    return static_cast<Game>((v >> 43) & 0x07u);
}
constexpr bool formMatches(uint8_t encounterForm, uint8_t pokemonForm) noexcept {
    // PKHeX EncounterUtil.FormDynamic=30 and FormRandom=31 on the pinned reference.
    return encounterForm >= 30 || encounterForm == pokemonForm;
}
constexpr bool levelMatches(uint64_t v, uint8_t level) noexcept {
    return level >= minLevel(v) && level <= maxLevel(v);
}

inline bool hasSpecies(std::string_view exactGameId, uint16_t speciesId) noexcept {
    const Game wanted = gameForId(exactGameId);
    if (wanted == Game::Invalid || speciesId == 0) return false;
    for (const uint64_t row : kPackedGen4WildEncounters)
        if (game(row) == wanted && species(row) == speciesId) return true;
    return false;
}

inline bool matches(std::string_view exactGameId, uint16_t speciesId,
                    uint16_t metLocation, uint8_t metLevel,
                    uint8_t pokemonForm) noexcept {
    const Game wanted = gameForId(exactGameId);
    if (wanted == Game::Invalid || speciesId == 0 || metLocation > 0xFF || metLevel == 0)
        return false;
    for (const uint64_t row : kPackedGen4WildEncounters) {
        if (game(row) != wanted || species(row) != speciesId)
            continue;
        if (location(row) != metLocation || !levelMatches(row, metLevel))
            continue;
        if (formMatches(form(row), pokemonForm))
            return true;
    }
    return false;
}

inline std::size_t countForGame(std::string_view exactGameId) noexcept {
    const Game wanted = gameForId(exactGameId);
    if (wanted == Game::Invalid) return 0;
    std::size_t count = 0;
    for (const uint64_t row : kPackedGen4WildEncounters)
        if (game(row) == wanted) ++count;
    return count;
}

} // namespace Legality::Gen4Wild
