// Standalone native property test for the exact L3 original-source digest.
// This exercises the production hash algorithm against original source
// witnesses and bounded mutation attacks, without requiring a checkout copy
// of the whole learnset table. The companion full-table test is mandatory.
#include "Legality/Gen3GbaOriginalSourceDigest.h"
#include <array>
#include <cstdint>
#include <iostream>
namespace D=Legality::Gen3GbaOriginalSourceFingerprints;
template<std::size_t N>
constexpr std::uint64_t digest(std::uint16_t species,
                               const std::array<std::uint16_t,N>& words) noexcept {
    return D::fingerprint(species,static_cast<std::uint16_t>(N),
                          [&](std::uint16_t i) {return words[i];});
}
// Independently transcribed ordered pret Ruby Bulbasaur level-up offers.
constexpr std::array<std::uint16_t,11> kBulbasaur{{
    (1u<<9)|33u,(4u<<9)|45u,(7u<<9)|73u,(10u<<9)|22u,
    (15u<<9)|77u,(15u<<9)|79u,(20u<<9)|75u,(25u<<9)|230u,
    (32u<<9)|74u,(39u<<9)|235u,(46u<<9)|76u
}};
static_assert(digest(1,kBulbasaur)==0x0738c167bf798326ULL);
// pret/pokefirered#51 same-level branch must not be unioned or reordered.
constexpr std::array<std::uint16_t,13> kDugtrioFireRed{{
    673,522,540,557,2605,4830,8795,10906,12989,13640,
    19619,26201,32858
}};
constexpr std::array<std::uint16_t,13> kDugtrioLeafGreen{{
    673,540,522,557,2605,4830,8795,10906,12989,13640,
    19619,26201,32858
}};
static_assert(digest(51,kDugtrioFireRed)==0x3fd4c2ca9398e9b9ULL);
static_assert(digest(51,kDugtrioLeafGreen)==0x5f7ed605e8702809ULL);
static_assert(digest(51,kDugtrioFireRed)!=digest(51,kDugtrioLeafGreen));
int main() {
    std::uint32_t checks=0;
    for(std::uint16_t species=1;species<=386;++species) {
        std::array<std::uint16_t,4> moves{{33,45,73,22}};
        const auto original=digest(species,moves);
        for(std::size_t slot=0;slot<moves.size();++slot) {
            for(unsigned bit=0;bit<16;++bit) {
                auto corrupt=moves;
                corrupt[slot]^=static_cast<std::uint16_t>(1u<<bit);
                if(digest(species,corrupt)==original) {
                    std::cerr<<"Fingerprint collision on bounded single-bit mutation\n";
                    return 1;
                }
                ++checks;
            }
        }
        if(species<386 && digest(static_cast<std::uint16_t>(species+1),moves)==original)
            return 1;
        ++checks;
        auto reordered=moves;
        const auto first=reordered[0];reordered[0]=reordered[1];reordered[1]=first;
        if(digest(species,reordered)==original)return 1;
        ++checks;
        if(D::fingerprint(species,3,[&](std::uint16_t i){return moves[i];})==original)
            return 1;
        ++checks;
    }
    // Catch all 16-bit accidental edits in the original source witness.
    const auto trusted=digest(1,kBulbasaur);
    for(std::size_t i=0;i<kBulbasaur.size();++i)
        for(unsigned bit=0;bit<16;++bit) {
            auto altered=kBulbasaur;
            altered[i]^=static_cast<std::uint16_t>(1u<<bit);
            if(digest(1,altered)==trusted)return 1;
            ++checks;
        }
    if(checks!=26038)return 1; // 386 * (64+3) + 11*16.
    std::cout<<"L3 original ordered source fingerprint primitives: "<<checks
             <<" bounded mutations PASS\n";
}
