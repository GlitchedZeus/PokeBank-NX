#include "Legality/Gen4ContestRibbonNativeEvidence.h"
#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
namespace C=Legality::Gen4ContestRibbonNativeEvidence;
int main(){
    using S=C::Status;
    assert(!C::eligibleAtStage(132,0));
    assert(!C::eligibleAtStage(201,0));
    assert(!C::eligibleAtStage(172,1));
    assert(C::eligibleAtStage(172,0));
    // Five native PK4 4-bit categories at byte offset 0x60-0x62.
    // All 16 rank masks per category, comparing against independent
    // prefix membership rather than treating 0x0F as "no ribbon".
    constexpr std::array<uint8_t,5> legal{0,1,3,7,15};
    for(unsigned category=0;category<5;++category) {
        for(unsigned mask=0;mask<16;++mask) {
            const uint32_t word=static_cast<uint32_t>(mask)<<(category*4);
            const auto out=C::evaluate(true,word);
            bool isPrefix=false;
            for(const auto allowed:legal)
                if(mask==allowed)isPrefix=true;
            assert((out.status==S::CandidateRankGap)==!isPrefix);
            assert(out.nativeRankMasks[category]==mask);
            assert(out.prefix.gapCategoriesMask==
                   (isPrefix?0u:(1u<<category)));
        }
    }
    C::ContestHistory exact{true,true,true};
    assert(C::evaluate(true,0x11111u,exact).status==S::RankPrefixConsistent);
    assert(C::evaluate(true,0x11111u).status==S::HistoryUnknown);
    assert(C::evaluate(true,0x11111u,{true,false,true}).status==S::CandidateCannotEnter);
    assert(C::evaluate(true,0x11111u,{true,true,false}).status==S::CandidateCannotEnter);
    assert(C::evaluate(true,0,{}).status==S::RankPrefixConsistent);
    assert(C::evaluate(false,0xfffffu,{}).status==S::Unavailable);
    // Low-to-high rank prefix and historical-source rules are independent.
    // HGSS residency alone does NOT invalidate Sinnoh contest ribbons.
    assert(C::evaluate(true,0x731fu,exact).status==S::RankPrefixConsistent);
    assert(C::evaluate(true,0x731fu).status==S::HistoryUnknown);
    std::cout<<"L4 V15 native Gen IV contest ranks + source history: PASS\n";
}
