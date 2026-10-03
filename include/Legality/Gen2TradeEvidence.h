#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace Legality::Gen2Trade {

inline constexpr uint8_t kNpcTradeLocation = 126;

#include "Legality/Gen2TradeEvidenceData.inc"

constexpr uint16_t species(uint64_t v) noexcept { return static_cast<uint16_t>(v & 0xFFu); }
constexpr uint8_t level(uint64_t v) noexcept { return static_cast<uint8_t>((v >> 8) & 0x7Fu); }
constexpr bool hasFixedDvs(uint64_t v) noexcept { return ((v >> 15) & 1u) != 0; }
constexpr uint16_t tid(uint64_t v) noexcept { return static_cast<uint16_t>((v >> 16) & 0xFFFFu); }
constexpr uint8_t atkDv(uint64_t v) noexcept { return static_cast<uint8_t>((v >> 32) & 0xFu); }
constexpr uint8_t defDv(uint64_t v) noexcept { return static_cast<uint8_t>((v >> 36) & 0xFu); }
constexpr uint8_t speDv(uint64_t v) noexcept { return static_cast<uint8_t>((v >> 40) & 0xFu); }
constexpr uint8_t specialDv(uint64_t v) noexcept { return static_cast<uint8_t>((v >> 44) & 0xFu); }
constexpr uint8_t templateIndex(uint64_t v) noexcept { return static_cast<uint8_t>((v >> 48) & 0xFu); }

constexpr bool isGen2Game(std::string_view id) noexcept {
    return id=="gold_gbc" || id=="silver_gbc" || id=="crystal_gbc";
}
constexpr uint8_t caughtMetLevel(uint16_t caughtData) noexcept {
    return static_cast<uint8_t>((caughtData >> 8) & 0x3Fu);
}
constexpr uint8_t caughtMetLocation(uint16_t caughtData) noexcept {
    return static_cast<uint8_t>(caughtData & 0x7Fu);
}

inline bool hasSpecies(uint16_t speciesId) noexcept {
    for(uint64_t row:kPackedGen2Trades) if(species(row)==speciesId) return true;
    return false;
}

inline bool matches(std::string_view exactGameId, uint16_t speciesId,
                    uint8_t currentLevel, uint16_t trainerId,
                    const std::array<uint8_t,5>& dvs,
                    uint16_t caughtData) noexcept {
    if(!isGen2Game(exactGameId) || speciesId==0) return false;
    for(uint64_t row:kPackedGen2Trades){
        if(species(row)!=speciesId || currentLevel<level(row)) continue;

        // Crystal stores caught-data for NPC trades: link-trade location, met level 0.
        if(caughtData!=0 &&
           (caughtMetLocation(caughtData)!=kNpcTradeLocation || caughtMetLevel(caughtData)!=0))
            continue;

        if(hasFixedDvs(row)){
            if(trainerId!=tid(row)) continue;
            // PokeBank DV order: HP / Atk / Def / Spe / Special.
            if(dvs[1]!=atkDv(row) || dvs[2]!=defDv(row) ||
               dvs[3]!=speDv(row) || dvs[4]!=specialDv(row))
                continue;
        }
        return true;
    }
    return false;
}

inline std::size_t count() noexcept {
    return sizeof(kPackedGen2Trades)/sizeof(kPackedGen2Trades[0]);
}

} // namespace Legality::Gen2Trade
