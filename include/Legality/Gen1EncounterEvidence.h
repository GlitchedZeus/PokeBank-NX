#pragma once

#include "Legality/Gen1CatchRateEvidence.h"

#include <array>
#include <cstdint>
#include <string_view>

namespace Legality::Gen1Encounter {

struct StaticEntry {
    uint16_t species;
    uint8_t minimumLevel;
    uint8_t nativeGameMask; // Red=1, Blue=2, Yellow=4
};

struct TradeEntry {
    uint16_t species;
    uint8_t minimumLevelRBY;
    uint8_t minimumLevelAfterTradeback;
    uint8_t nativeGameMask;
    bool evolvesOnTrade;
};

struct Match {
    bool matched = false;
    bool nativeToContainer = false;
    uint16_t originalSpecies = 0;
    uint8_t minimumLevel = 0;
};

// Independently pinned original pret ROM tables retain the original offer
// ordinal and unreachable NPC entries, instead of merging by final species.
struct PretTradeEntry {
    uint8_t gameMask;
    uint8_t originalOfferIndex;
    uint16_t requestedSpecies;
    uint16_t receivedSpecies;
    bool reachable;
    std::string_view nickname;
};
enum class PretStaticKind : uint8_t { OverworldBattle, Gift };
struct PretStaticEntry {
    uint8_t gameMask;
    std::string_view originalMap;
    uint8_t sceneIndex;
    uint16_t species;
    uint8_t level;
    PretStaticKind kind;
};
#include "Legality/Gen1EncounterEvidenceData.inc"
#include "Legality/Gen1PretTrades.inc"
#include "Legality/Gen1PretStaticGifts.inc"
static_assert(kPretTradeOffers.size()==20);
static_assert(kPretStaticSources.size()==20);
constexpr const PretTradeEntry* originalPretTrade(uint8_t mask,
                                                  uint8_t sourceIndex) noexcept {
    if(mask!=1&&mask!=2&&mask!=4)return nullptr;
    for(const auto& row:kPretTradeOffers) {
        if((row.gameMask&mask)!=0 && row.originalOfferIndex==sourceIndex)
            return &row;
    }
    return nullptr;
}


constexpr uint8_t gameMaskForId(std::string_view id) noexcept {
    if (id == "red_gb") return 0x01;
    if (id == "blue_gb") return 0x02;
    if (id == "yellow_gb") return 0x04;
    return 0;
}

constexpr bool canDescendFrom(uint16_t currentSpecies, uint16_t originalSpecies) noexcept {
    if (currentSpecies == 0 || currentSpecies > 151 ||
        originalSpecies == 0 || originalSpecies > currentSpecies)
        return false;
    const uint16_t base = static_cast<uint16_t>(
        currentSpecies - Gen1CatchRate::evolutionStage(currentSpecies));
    return originalSpecies >= base && originalSpecies <= currentSpecies;
}

constexpr bool rateMatchesTemplate(uint8_t nativeMask, uint16_t originalSpecies,
                                   uint8_t catchRate) noexcept {
    if (Gen1CatchRate::isPossibleTimeCapsuleHeldItem(catchRate))
        return true;
    if ((nativeMask & 0x03u) != 0 &&
        catchRate == Gen1CatchRate::kCatchRateRB[originalSpecies])
        return true;
    if ((nativeMask & 0x04u) != 0 &&
        catchRate == Gen1CatchRate::kCatchRateY[originalSpecies])
        return true;
    return false;
}

inline Match matchStatic(std::string_view containerGameId, uint16_t currentSpecies,
                         uint8_t currentLevel, uint8_t catchRate) noexcept {
    const uint8_t container = gameMaskForId(containerGameId);
    if (container == 0 || currentSpecies == 0)
        return {};
    for (const auto& row : kStaticEntries) {
        if (!canDescendFrom(currentSpecies, row.species) ||
            currentLevel < row.minimumLevel ||
            !rateMatchesTemplate(row.nativeGameMask, row.species, catchRate))
            continue;
        return {true, (row.nativeGameMask & container) != 0, row.species, row.minimumLevel};
    }
    return {};
}

inline Match matchTrade(std::string_view containerGameId, uint16_t currentSpecies,
                        uint8_t currentLevel, uint8_t catchRate,
                        bool hasTradeOriginalTrainer) noexcept {
    const uint8_t container = gameMaskForId(containerGameId);
    if (container == 0 || currentSpecies == 0 || !hasTradeOriginalTrainer)
        return {};
    for (const auto& row : kTradeEntries) {
        if (!canDescendFrom(currentSpecies, row.species) ||
            currentLevel < row.minimumLevelAfterTradeback ||
            !rateMatchesTemplate(row.nativeGameMask, row.species, catchRate))
            continue;
        return {true, (row.nativeGameMask & container) != 0,
                row.species, row.minimumLevelAfterTradeback};
    }
    return {};
}

} // namespace Legality::Gen1Encounter
