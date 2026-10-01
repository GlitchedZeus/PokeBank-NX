#pragma once

#include "Legality/Gen34EggMoveEvidence.h"

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
    uint8_t abilityNumber;
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

struct Evidence {
    bool matched = false;
    uint16_t sourceSpecies = 0;
    bool evolved = false;
};

constexpr bool matchesPersistentFields(const Entry& row, uint8_t mask,
                                       uint32_t pid, uint32_t id32,
                                       uint8_t gender, uint8_t otGender,
                                       uint8_t abilityNumber, uint32_t packedIVs,
                                       uint16_t metLocation,
                                       uint8_t metLevel) noexcept {
    if ((row.gameMask & mask) == 0)
        return false;
    if (row.pid != pid || row.id32 != id32 || row.gender != gender ||
        row.otGender != otGender || row.abilityNumber != abilityNumber ||
        row.ivPack != packedIVs)
        return false;
    if (row.metLocation == 2001)
        return metLocation == 2001 && metLevel >= row.level;
    return metLocation == row.metLocation && metLevel == row.level;
}

inline Evidence matchDirect(std::string_view exactGameId, uint16_t speciesId,
                            uint32_t pid, uint32_t id32,
                            uint8_t gender, uint8_t otGender,
                            uint8_t abilityNumber,
                            const std::array<uint8_t, 6>& ivs,
                            uint16_t metLocation,
                            uint8_t metLevel) noexcept {
    const uint8_t mask = gameMaskForId(exactGameId);
    if (mask == 0 || speciesId == 0)
        return {};
    const uint32_t packedIVs = packIVs(ivs);
    for (const auto& row : kEntries) {
        if (row.species != speciesId)
            continue;
        if (matchesPersistentFields(
                row, mask, pid, id32, gender, otGender, abilityNumber,
                packedIVs, metLocation, metLevel))
            return {true, row.species, false};
    }
    return {};
}

inline Evidence matchEvolutionLine(
    std::string_view exactGameId, uint16_t speciesId,
    uint32_t pid, uint32_t id32, uint8_t gender, uint8_t otGender,
    uint8_t abilityNumber, const std::array<uint8_t, 6>& ivs,
    uint16_t metLocation, uint8_t metLevel) noexcept {
    if (const auto direct = matchDirect(
            exactGameId, speciesId, pid, id32, gender, otGender,
            abilityNumber, ivs, metLocation, metLevel);
        direct.matched)
        return direct;

    const uint8_t mask = gameMaskForId(exactGameId);
    if (mask == 0 || speciesId == 0)
        return {};
    const uint32_t packedIVs = packIVs(ivs);

    uint16_t ancestor = Gen34EggMove::preEvolution(exactGameId, speciesId);
    for (int depth = 0; ancestor != 0 && depth < 8; ++depth) {
        for (const auto& row : kEntries) {
            if (row.species != ancestor)
                continue;
            if (matchesPersistentFields(
                    row, mask, pid, id32, gender, otGender, abilityNumber,
                    packedIVs, metLocation, metLevel))
                return {true, row.species, true};
        }
        const uint16_t next =
            Gen34EggMove::preEvolution(exactGameId, ancestor);
        if (next == ancestor)
            break;
        ancestor = next;
    }
    return {};
}

inline bool matches(std::string_view exactGameId, uint16_t speciesId,
                    uint32_t pid, uint32_t id32, uint8_t gender, uint8_t otGender,
                    uint8_t abilityNumber, const std::array<uint8_t, 6>& ivs,
                    uint16_t metLocation, uint8_t metLevel) noexcept {
    return matchDirect(exactGameId, speciesId, pid, id32, gender, otGender,
                       abilityNumber, ivs, metLocation, metLevel).matched;
}

} // namespace Legality::Gen4Trade
