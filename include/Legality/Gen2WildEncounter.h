#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace Legality::Gen2Wild {

enum class Game : uint8_t { Gold = 0, Silver = 1, Crystal = 2, Invalid = 3 };

// Packed layout:
// species [0..7], location [8..15], encounter-time mask [16..23], slot type [24..27],
// min level [28..33], max level [34..39], slot number [40..43], game [44..45].
#include "Legality/Gen2WildEncounterData.inc"

constexpr uint8_t species(uint64_t v) noexcept { return static_cast<uint8_t>(v & 0xFFu); }
constexpr uint8_t location(uint64_t v) noexcept { return static_cast<uint8_t>((v >> 8) & 0xFFu); }
constexpr uint8_t timeMask(uint64_t v) noexcept { return static_cast<uint8_t>((v >> 16) & 0xFFu); }
constexpr uint8_t type(uint64_t v) noexcept { return static_cast<uint8_t>((v >> 24) & 0x0Fu); }
constexpr uint8_t minLevel(uint64_t v) noexcept { return static_cast<uint8_t>((v >> 28) & 0x3Fu); }
constexpr uint8_t maxLevel(uint64_t v) noexcept { return static_cast<uint8_t>((v >> 34) & 0x3Fu); }
constexpr uint8_t slot(uint64_t v) noexcept { return static_cast<uint8_t>((v >> 40) & 0x0Fu); }
constexpr Game game(uint64_t v) noexcept { return static_cast<Game>((v >> 44) & 0x03u); }

constexpr Game gameForId(std::string_view id) noexcept {
    if (id == "gold_gbc") return Game::Gold;
    if (id == "silver_gbc") return Game::Silver;
    if (id == "crystal_gbc") return Game::Crystal;
    return Game::Invalid;
}

constexpr bool timeAllows(uint8_t mask, uint8_t metTime) noexcept {
    return mask == 0 || (metTime < 4 && (mask & static_cast<uint8_t>(1u << metTime)) != 0);
}

inline bool hasSpecies(std::string_view exactGameId, uint16_t speciesId) noexcept {
    const auto wanted=gameForId(exactGameId);
    if (wanted==Game::Invalid || speciesId==0 || speciesId>255) return false;
    for(const uint64_t row : kPackedGen2WildEncounters)
        if (game(row)==wanted && species(row)==speciesId) return true;
    return false;
}

inline bool matchesCrystalCaughtData(uint16_t speciesId, uint16_t caughtData) noexcept {
    if (speciesId==0 || speciesId>255 || caughtData==0) return false;
    const uint8_t metTime=static_cast<uint8_t>((caughtData>>14)&0x03u);
    const uint8_t metLevel=static_cast<uint8_t>((caughtData>>8)&0x3Fu);
    const uint8_t metLocation=static_cast<uint8_t>(caughtData&0x7Fu);
    for(const uint64_t row : kPackedGen2WildEncounters) {
        if (game(row)!=Game::Crystal || species(row)!=speciesId) continue;
        if (location(row)!=metLocation) continue;
        if (metLevel<minLevel(row) || metLevel>maxLevel(row)) continue;
        if (!timeAllows(timeMask(row),metTime)) continue;
        return true;
    }
    return false;
}

inline std::size_t countForGame(Game wanted) noexcept {
    std::size_t count=0;
    for(const uint64_t row : kPackedGen2WildEncounters) if(game(row)==wanted) ++count;
    return count;
}

} // namespace Legality::Gen2Wild
