#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace Legality::Gen4Trade {

struct Entry {
    uint32_t pid;
    uint32_t id32;
    uint32_t ivPack;
    uint16_t species;
    uint16_t metLocation;
    uint8_t level;
    uint8_t gender;
    uint8_t otGender;
    uint8_t gameMask;
};

#include "Legality/Gen4TradeEvidenceData.inc"

constexpr uint8_t gameMaskForId(std::string_view id) noexcept {
    if (id == "diamond_nds") return 0x01;
    if (id == "pearl_nds") return 0x02;
    if (id == "platinum_nds") return 0x04;
    if (id == "heartgold_nds") return 0x08;
    if (id == "soulsilver_nds") return 0x10;
    return 0;
}

constexpr uint32_t packIVs(const std::array<uint8_t, 6>& ivs) noexcept {
    return (static_cast<uint32_t>(ivs[0] & 31u)      ) |
           (static_cast<uint32_t>(ivs[1] & 31u) <<  5) |
           (static_cast<uint32_t>(ivs[2] & 31u) << 10) |
           (static_cast<uint32_t>(ivs[3] & 31u) << 15) |
           (static_cast<uint32_t>(ivs[4] & 31u) << 20) |
           (static_cast<uint32_t>(ivs[5] & 31u) << 25);
}

inline bool hasSpecies(std::string_view exactGameId, uint16_t speciesId) noexcept {
    const uint8_t mask = gameMaskForId(exactGameId);
    if (mask == 0 || speciesId == 0) return false;
    for (const auto& row : kEntries)
        if ((row.gameMask & mask) != 0 && row.species == speciesId) return true;
    return false;
}

inline bool matches(std::string_view exactGameId, uint16_t speciesId,
                    uint32_t pid, uint32_t id32, uint8_t gender, uint8_t otGender,
                    const std::array<uint8_t, 6>& ivs,
                    uint16_t metLocation, uint8_t metLevel) noexcept {
    const uint8_t mask = gameMaskForId(exactGameId);
    if (mask == 0 || speciesId == 0) return false;
    const uint32_t packedIVs = packIVs(ivs);
    for (const auto& row : kEntries) {
        if ((row.gameMask & mask) == 0 || row.species != speciesId)
            continue;
        if (row.pid != pid || row.id32 != id32 || row.gender != gender ||
            row.otGender != otGender || row.ivPack != packedIVs)
            continue;
        if (row.metLocation == 2001) {
            if (metLocation != 2001 || metLevel < row.level)
                continue;
        } else if (metLocation != row.metLocation || metLevel != row.level) {
            continue;
        }
        return true;
    }
    return false;
}

} // namespace Legality::Gen4Trade
