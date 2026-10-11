#pragma once
// Positive-only joint four-slot bounds against the pinned pret-derived Gen III
// game pools. These unions include egg, tutor, level-up and ancestor moves but
// do not prove timing, parents, actual trades or exclusive event history.
#include "Integration/Gen3/Gen3LearnsetTable.h"
#include <array>
#include <cstdint>

namespace Legality::Gen3JointMoveEvidence {
using Source=PokeVault::Integration::Gen3::SourceGame;
enum class Bound : uint8_t { Empty, OnePool, MultiplePools, Unresolved };
struct Result {
    Bound bound=Bound::Unresolved;
    uint8_t minimumPools=0;
    uint8_t poolMask=0;
    bool originalPoolCovers=false;
};

constexpr uint8_t bitFor(Source s) noexcept {
    switch(s) {
        case Source::RubyGBA:
        case Source::SapphireGBA:return 1;
        case Source::EmeraldGBA:return 2;
        case Source::FireRedGBA:
        case Source::LeafGreenGBA:return 4;
    }
    return 0;
}
inline uint8_t moveMask(uint16_t species,uint16_t move) noexcept {
    using PokeVault::Integration::Gen3::Learnset::directlyLearnable;
    return (directlyLearnable(Source::RubyGBA,species,move)?1u:0u) |
           (directlyLearnable(Source::EmeraldGBA,species,move)?2u:0u) |
           (directlyLearnable(Source::FireRedGBA,species,move)?4u:0u);
}
constexpr uint8_t countBits(uint8_t b) noexcept {
    uint8_t n=0;
    for(;b;b>>=1)n+=static_cast<uint8_t>(b&1u);
    return n;
}
inline Result classify(Source origin,uint16_t species,
                       const std::array<uint16_t,4>& moves) noexcept {
    std::array<uint8_t,4> masks{};
    uint8_t used=0,common=7;
    for(uint16_t move:moves) {
        if(move==0)continue;
        const uint8_t mask=moveMask(species,move);
        // An unmodeled event/transfer move is UNKNOWN, not proof of Invalid.
        if(mask==0)return {};
        masks[used++]=mask;
        common&=mask;
    }
    if(used==0)return {Bound::Empty,0,0,false};
    const uint8_t original=bitFor(origin);
    if(common) {
        const uint8_t chosen=(common&original)
            ? original
            : static_cast<uint8_t>(common & static_cast<uint8_t>(~common+1u));
        return {Bound::OnePool,1,chosen,(common&original)!=0};
    }
    // Exact set-cover search across all three audited game-family pools.
    uint8_t best=0,minCount=4;
    for(uint8_t subset=1;subset<8;++subset) {
        bool covers=true;
        for(uint8_t i=0;i<used;++i)
            if((subset&masks[i])==0){covers=false;break;}
        if(!covers)continue;
        const uint8_t n=countBits(subset);
        if(n<minCount || (n==minCount && (subset&original) &&
                          !(best&original))) {
            minCount=n;
            best=subset;
        }
    }
    return best?Result{Bound::MultiplePools,minCount,best,false}:Result{};
}
// A retained GameCube (Colosseum/XD) origin is not a Ruby/Sapphire,
// Emerald or FRLG original-cartridge identity. The union of those three
// pinned GBA pools provides only *possible* intermediate game evidence.
// No game-family mask is labeled as original or proven visited.
struct UnanchoredResult {
    Bound bound=Bound::Unresolved;
    uint8_t minimumPools=0;
};

inline UnanchoredResult classifyUnanchored(
    uint16_t species,const std::array<uint16_t,4>& moves) noexcept {
    std::array<uint8_t,4> masks{};
    uint8_t used=0,common=7;
    for(const uint16_t move:moves) {
        if(!move)continue;
        const uint8_t mask=moveMask(species,move);
        if(mask==0)return {}; // Unmodeled GameCube/event move is unknown.
        masks[used++]=mask;
        common&=mask;
    }
    if(!used)return {Bound::Empty,0};
    if(common)return {Bound::OnePool,1};
    uint8_t minCount=4;
    for(uint8_t subset=1;subset<8;++subset) {
        bool covers=true;
        for(uint8_t i=0;i<used;++i)
            if((subset&masks[i])==0){covers=false;break;}
        if(covers) {
            const auto count=countBits(subset);
            if(count<minCount)minCount=count;
        }
    }
    return minCount<=3
        ? UnanchoredResult{Bound::MultiplePools,minCount}
        : UnanchoredResult{};
}

} // namespace Legality::Gen3JointMoveEvidence
