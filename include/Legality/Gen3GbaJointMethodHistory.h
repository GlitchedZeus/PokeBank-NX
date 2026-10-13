#pragma once
// L3 Stage 3 exact original Gen III title, species, machine and relearner
// adapter. Pinned data are independently verified against original pret
// source fingerprints before a history is even eligible for candidate proof.
// Production Legality.cpp is intentionally untouched.
#include "Legality/Gen3GbaJointMethodHistoryCore.h"
#include "Legality/Gen14CandidateMoveGbaLevelUpTimeline.h"
#include "Legality/Gen3GbaOriginalMachineData.h"
#include "Legality/Gen3GbaOriginalSourceFingerprints.h"
#include <array>
#include <cstdint>
#include <span>

namespace Legality::Gen3GbaJointMethodHistory {
namespace H=Gen3GbaJointMethodHistoryCore;
namespace L=Gen3GbaVersionedLevelUpTimeline;
namespace M=Gen3GbaOriginalMachineData;
namespace F=Gen3GbaOriginalSourceFingerprints;
namespace C=Gen14CandidateMoveChronology;
constexpr C::Game originalGame(H::Game game) noexcept {
    switch(game) {
    case H::Game::Ruby:return C::Game::Ruby;
    case H::Game::Sapphire:return C::Game::Sapphire;
    case H::Game::Emerald:return C::Game::Emerald;
    case H::Game::FireRed:return C::Game::FireRed;
    case H::Game::LeafGreen:return C::Game::LeafGreen;
    default:return C::Game::Unknown;
    }
}
constexpr std::uint64_t independentOriginalHash(H::Game game,
                                                std::uint16_t species) noexcept {
    if(species<1||species>386)return 0;
    switch(game) {
    case H::Game::Ruby:case H::Game::Sapphire:return F::kRuby[species];
    case H::Game::Emerald:return F::kEmerald[species];
    case H::Game::FireRed:return F::kFireRed[species];
    case H::Game::LeafGreen:return F::kLeafGreen[species];
    default:return 0;
    }
}
struct Evidence {
    std::array<H::Offer,64> offers{};
    std::uint16_t count=0;
    std::uint64_t machines=0;
    bool pinned=false;
};
constexpr Evidence originalEvidence(H::Game game,std::uint16_t species) noexcept {
    Evidence data{};
    const auto sourceGame=originalGame(game);
    if(!H::supported(game)||species<1||species>386||
       !L::supported(sourceGame,species))return data;
    data.count=L::offerCount(sourceGame,species);
    if(!data.count||data.count>data.offers.size()){
        data.count=0;return data;
    }
    for(std::uint16_t i=0;i<data.count;++i) {
        const auto source=L::offerAt(sourceGame,species,i);
        data.offers[i]={source.level,source.move};
    }
    // This FNV manifest was generated exclusively from independently pinned
    // original Ruby/Emerald/FRLG source rather than from these application
    // arrays, and covers exact species and offer ordering.
    const auto actual=F::fingerprint(species,data.count,[&](std::uint16_t i){
        return static_cast<std::uint16_t>(
            (static_cast<std::uint16_t>(data.offers[i].level)<<9)|
             data.offers[i].move);
    });
    data.machines=game==H::Game::FireRed||game==H::Game::LeafGreen?
        M::kFireRedLeafGreenMasks[species]:M::kRubySapphireMasks[species];
    data.pinned=actual==independentOriginalHash(game,species) &&
        (data.machines>>58)==0;
    return data;
}
constexpr H::Result evaluate(const H::History& path) noexcept {
    const auto data=originalEvidence(path.game,path.species);
    const H::Catalog c{
        path.game,path.species,
        std::span<const H::Offer>(data.offers.data(),data.count),
        std::span<const std::uint16_t>(M::kMachineMove.data(),M::kMachineMove.size()),
        data.machines,data.pinned,data.pinned,data.pinned,data.pinned
    };
    return H::evaluate(c,path);
}
constexpr H::Status evaluate(const H::Alternatives& paths) noexcept {
    if(!paths.count||paths.count>paths.paths.size())return H::Status::Unknown;
    const auto game=paths.paths[0].game;
    const auto species=paths.paths[0].species;
    for(std::uint8_t i=1;i<paths.count;++i)
        if(paths.paths[i].game!=game||paths.paths[i].species!=species)
            return H::Status::Unknown;
    const auto data=originalEvidence(game,species);
    const H::Catalog c{
        game,species,std::span<const H::Offer>(data.offers.data(),data.count),
        std::span<const std::uint16_t>(M::kMachineMove.data(),M::kMachineMove.size()),
        data.machines,data.pinned,data.pinned,data.pinned,data.pinned
    };
    return H::evaluate(c,paths);
}
static_assert(originalGame(H::Game::Ruby)==C::Game::Ruby);
static_assert(originalGame(H::Game::LeafGreen)==C::Game::LeafGreen);
static_assert(independentOriginalHash(H::Game::FireRed,386)!=
              independentOriginalHash(H::Game::LeafGreen,386));
} // namespace Legality::Gen3GbaJointMethodHistory
