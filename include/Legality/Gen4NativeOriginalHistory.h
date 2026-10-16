#pragma once
// L3 Stage 6: independent original-title catalogs, no save mutation or global Invalid.
// Diamond and Pearl original 493-species level-up/machine rows; HGSS original
// 493-species machine/tutor rows; Platinum 12 separately pinned source species.
// UNKNOWN is intentional for HGSS binary wotbl level-up, unsupported forms,
// unknown DP/HGSS numbered TM move mappings, and unpinned Platinum species.
#include "Legality/Gen4NativeOriginalHistoryCore.h"
#include "Legality/Gen4NativeOriginalSourceData.h"
#include "Legality/Gen4PlatinumNativeMethodData.h"
#include "Legality/Gen4PlatinumPinnedMachines.h"
#include <array>
#include <cstdint>
#include <span>
namespace Legality::Gen4NativeOriginalHistory {
namespace H=Gen4NativeOriginalHistoryCore;
namespace D=Gen4NativeOriginalSourceData;
namespace P=Gen4PlatinumNativeMethodData;
namespace M=Gen4PlatinumPinnedMachines;
using Game=H::Game;
using Status=H::Status;
inline constexpr std::array<std::uint16_t,100> kPlatinumConfirmedItems=[] {
    std::array<std::uint16_t,100> values{};
    for(const auto& item:M::kPinned) {
        if(item.number==0||item.number>(item.hidden?8:92))continue;
        values[item.hidden?91+item.number:item.number-1]=item.move;
    }
    return values;
}();
struct Original {
    static constexpr H::Catalog catalog(Game game,std::uint16_t species) noexcept {
        H::Catalog c{};
        if(!H::supported(game)||!D::nationalSpecies(species))return c;
        c.game=game;c.species=species;
        if(game==Game::Diamond||game==Game::Pearl){
            const auto row=D::kDiamondPearlRanges[species];
            c.orderedLevelSourceComplete=D::completeOrderedOffers(species);
            c.packedOffers=std::span<const std::uint16_t>(
                D::kDiamondPearlOffers.data()+row.offset,row.count);
            c.machineCompatibilitySourceComplete=true;
            c.machineLow=D::kDiamondPearlMachineLow[species];
            c.machineHigh=D::kDiamondPearlMachineHigh[species];
            c.originalRelearnerSourceComplete=c.orderedLevelSourceComplete;
            // DP original TM number -> move mapping not independently pinned yet.
        } else if(game==Game::HeartGold||game==Game::SoulSilver){
            c.machineCompatibilitySourceComplete=true;
            c.machineLow=D::kHeartGoldSoulSilverMachineLow[species];
            c.machineHigh=D::kHeartGoldSoulSilverMachineHigh[species];
            c.tutorSourceComplete=true;
            c.tutorMoves=D::kHeartGoldSoulSilverTutorMoves;
            c.tutorMask=D::kHeartGoldSoulSilverTutorMask[species];
            // HGSS level-up binary wotbl.narc unresolved -> no positive level timeline.
        } else if(game==Game::Platinum){
            // Index lookup, not a null-pointer test: GCC 13 rejects comparing
            // the address of an inline constexpr array element with null in
            // a constant expression.
            const auto index=P::rowIndex(species);
            if(index>=P::kRows.size())return c; // No source for this species: Unknown.
            const auto& row=P::kRows[index];
            c.orderedLevelSourceComplete=true;
            c.originalRelearnerSourceComplete=true;
            c.packedOffers=std::span<const std::uint16_t>(
                P::kOffers.data()+row.offerStart,row.offerCount);
            c.machineCompatibilitySourceComplete=true;
            // Normalize Platinum's original 100..107 HM bit positions to
            // shared TM01..92/HM01..08 positions 92..99, preserving identity.
            for(std::uint8_t n=1;n<=92;++n)if(P::machineCompatible(species,false,n)){
                const auto bit=n-1;
                if(bit<64)c.machineLow|=std::uint64_t{1}<<bit;
                else c.machineHigh|=std::uint64_t{1}<<(bit-64);
            }
            for(std::uint8_t n=1;n<=8;++n)if(P::machineCompatible(species,true,n))
                c.machineHigh|=std::uint64_t{1}<<(27+n); // index 91+n minus 64
            c.numberedMachineMoves=kPlatinumConfirmedItems;
            c.tutorSourceComplete=true;
            c.tutorMoves=std::span<const std::uint16_t>(
                P::kTutors.data()+row.tutorStart,row.tutorCount);
            c.tutorMask=(std::uint64_t{1}<<row.tutorCount)-1;
        }
        return c;
    }
};
constexpr H::Result evaluate(const H::History& path) noexcept {
    return H::evaluate<Original>(path);
}
constexpr Status evaluate(const H::Alternatives& paths) noexcept {
    return H::evaluate<Original>(paths);
}
static_assert(D::offerCount(1)==14);
static_assert(Original::catalog(Game::Diamond,1).orderedLevelSourceComplete);
static_assert(Original::catalog(Game::Pearl,354).orderedLevelSourceComplete==false);
static_assert(!Original::catalog(Game::HeartGold,1).orderedLevelSourceComplete);
static_assert(Original::catalog(Game::SoulSilver,1).tutorSourceComplete);
static_assert(Original::catalog(Game::Platinum,1).orderedLevelSourceComplete);
static_assert(!Original::catalog(Game::Platinum,252).orderedLevelSourceComplete);
static_assert(kPlatinumConfirmedItems[52]==412);
} // namespace Legality::Gen4NativeOriginalHistory
