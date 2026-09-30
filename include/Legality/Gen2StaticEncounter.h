#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace Legality::Gen2Static {

enum class Game : uint8_t { Gold = 0, Silver = 1, Crystal = 2, Invalid = 3 };

// Packed layout:
// species [0..7], location [8..14], level [15..21], egg [22],
// shiny policy [23..24] (0 random, 1 always, 2 never), roaming [25], game [26..27].
#include "Legality/Gen2StaticEncounterData.inc"

constexpr Game gameForId(std::string_view id) noexcept {
    if (id == "gold_gbc") return Game::Gold;
    if (id == "silver_gbc") return Game::Silver;
    if (id == "crystal_gbc") return Game::Crystal;
    return Game::Invalid;
}
constexpr uint16_t species(uint32_t v) noexcept { return static_cast<uint16_t>(v & 0xFFu); }
constexpr uint8_t location(uint32_t v) noexcept { return static_cast<uint8_t>((v >> 8) & 0x7Fu); }
constexpr uint8_t level(uint32_t v) noexcept { return static_cast<uint8_t>((v >> 15) & 0x7Fu); }
constexpr bool isEgg(uint32_t v) noexcept { return ((v >> 22) & 1u) != 0; }
constexpr uint8_t shinyPolicy(uint32_t v) noexcept { return static_cast<uint8_t>((v >> 23) & 0x03u); }
constexpr bool roaming(uint32_t v) noexcept { return ((v >> 25) & 1u) != 0; }
constexpr Game game(uint32_t v) noexcept { return static_cast<Game>((v >> 26) & 0x03u); }

constexpr uint8_t caughtMetLevel(uint16_t caughtData) noexcept {
    return static_cast<uint8_t>((caughtData >> 8) & 0x3Fu);
}
constexpr uint8_t caughtMetLocation(uint16_t caughtData) noexcept {
    return static_cast<uint8_t>(caughtData & 0x7Fu);
}

constexpr bool roamerLocationAllowed(uint8_t loc) noexcept {
    // PKHeX EncounterStatic2 RoamLocations: Routes 29-46 except 40/41.
    constexpr uint64_t mask =
        0b10'1000'1010'0100'0000'0110'0011'0100'1000'1001'0011'0100ULL;
    return loc < 64 && ((mask >> loc) & 1ULL) != 0;
}

inline bool hasSpecies(std::string_view exactGameId, uint16_t speciesId) noexcept {
    const auto wanted=gameForId(exactGameId);
    if(wanted==Game::Invalid || speciesId==0) return false;
    for(uint32_t row:kPackedGen2StaticEncounters)
        if(game(row)==wanted && species(row)==speciesId) return true;
    return false;
}

// Positive static/gift evidence. Gold/Silver do not preserve Crystal caught-data,
// so caughtData==0 cannot prove location; it can only establish compatibility.
inline bool matches(std::string_view exactGameId, uint16_t speciesId,
                    uint8_t currentLevel, uint16_t caughtData,
                    bool pokemonIsEgg, bool pokemonIsShiny) noexcept {
    const auto wanted=gameForId(exactGameId);
    if(wanted==Game::Invalid || speciesId==0) return false;
    for(uint32_t row:kPackedGen2StaticEncounters){
        if(game(row)!=wanted || species(row)!=speciesId) continue;
        const uint8_t requiredLevel=level(row);
        if(currentLevel < requiredLevel) continue;

        const uint8_t shiny=shinyPolicy(row);
        if(shiny==1 && !pokemonIsShiny) continue;
        if(shiny==2 && pokemonIsShiny) continue;

        if(isEgg(row)){
            // This first tranche proves only an unhatched gift/odd egg directly.
            // Hatched-egg provenance needs the dedicated breeding/hatch layer.
            if(!pokemonIsEgg) continue;
            return true;
        }
        if(pokemonIsEgg) continue;

        if(caughtData!=0){
            if(caughtMetLevel(caughtData)!=requiredLevel) continue;
            const uint8_t metLoc=caughtMetLocation(caughtData);
            if(roaming(row)){
                if(!roamerLocationAllowed(metLoc)) continue;
            } else if(metLoc!=location(row)) {
                continue;
            }
        }
        return true;
    }
    return false;
}

inline std::size_t countForGame(std::string_view exactGameId) noexcept {
    const auto wanted=gameForId(exactGameId);
    if(wanted==Game::Invalid) return 0;
    std::size_t count=0;
    for(uint32_t row:kPackedGen2StaticEncounters) if(game(row)==wanted) ++count;
    return count;
}

} // namespace Legality::Gen2Static
