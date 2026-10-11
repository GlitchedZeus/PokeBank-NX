#pragma once
// Source-free native Gen IV four-slot joint move-pool lower bound.
// This is only a finite compatibility/set-cover witness, NOT proof of
// acquisition timing, complete move coexistence, trades or event origins.
#include "Integration/Gen4/Gen4MoveCompatibility.h"
#include <array>
#include <cstdint>
#include <string_view>

namespace Legality::Gen4JointMoveEvidence {
using namespace PokeVault::Integration::Gen4MoveCompatibility;
enum class Bound : uint8_t { Empty, OnePool, MultiplePools, Unresolved };
struct Result {
    Bound bound=Bound::Unresolved;
    uint8_t minimumPools=0;
    uint8_t poolMask=0;
    bool originalPoolCovers=false;
};
constexpr uint8_t originalBit(std::string_view game) noexcept {
    if(game=="diamond_nds" || game=="pearl_nds")return 1;
    if(game=="platinum_nds")return 2;
    if(game=="heartgold_nds" || game=="soulsilver_nds")return 4;
    return 0;
}
constexpr uint8_t moveMask(uint16_t species,uint8_t form,
                           uint16_t move) noexcept {
    return (direct("diamond_nds",species,form,move)?1u:0u) |
           (direct("platinum_nds",species,form,move)?2u:0u) |
           (direct("heartgold_nds",species,form,move)?4u:0u);
}
constexpr uint8_t countBits(uint8_t b) noexcept {
    uint8_t n=0;
    for(;b;b>>=1)n+=static_cast<uint8_t>(b&1u);
    return n;
}
constexpr Result classify(std::string_view original,uint16_t species,
                          uint8_t form,
                          const std::array<uint16_t,4>& moves) noexcept {
    if(!originalBit(original))return {};
    uint8_t common=7,used=0;
    std::array<uint8_t,4> masks{};
    for(uint16_t move:moves) {
        if(!move)continue;
        const uint8_t mask=moveMask(species,form,move);
        // Missing from every audited pool may be event/legacy/transfer;
        // no hard negative source claim follows from the union tables.
        if(!mask)return {};
        masks[used++]=mask;
        common&=mask;
    }
    if(!used)return {Bound::Empty,0,0,false};
    const uint8_t own=originalBit(original);
    if(common) {
        const uint8_t chosen=(common&own)
            ? own
            : static_cast<uint8_t>(common & static_cast<uint8_t>(~common+1u));
        return {Bound::OnePool,1,chosen,(common&own)!=0};
    }
    uint8_t best=0,minCount=4;
    for(uint8_t subset=1;subset<8;++subset) {
        bool covers=true;
        for(uint8_t i=0;i<used;++i)
            if(!(subset&masks[i])){covers=false;break;}
        if(!covers)continue;
        const uint8_t n=countBits(subset);
        if(n<minCount || (n==minCount && (subset&own) && !(best&own))) {
            minCount=n;
            best=subset;
        }
    }
    return best?Result{Bound::MultiplePools,minCount,best,false}:Result{};
}
} // namespace Legality::Gen4JointMoveEvidence
