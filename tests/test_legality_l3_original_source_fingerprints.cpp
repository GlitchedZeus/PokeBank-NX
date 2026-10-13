// L3 exact-source full National Dex fingerprint regression.
// Independently pinned original pret source data (not generated from this
// application's arrays) must match all GBA retail title-specific offers.
// No historical encounters, TM receipts, saves or legality Invalid inferred.
#include "Legality/Gen14CandidateMoveGbaLevelUpTimeline.h"
#include "Legality/Gen3GbaOriginalSourceFingerprints.h"
#include <array>
#include <cstdint>
#include <iostream>
namespace L=Legality::Gen3GbaVersionedLevelUpTimeline;
namespace F=Legality::Gen3GbaOriginalSourceFingerprints;
using G=L::Game;
struct Family {
    G game;
    const std::array<std::uint64_t,387>* original;
    std::uint32_t offers;
};
int main() {
    const std::array<Family,5> families{{
        {G::Ruby,&F::kRuby,F::kRubyOffers},
        {G::Sapphire,&F::kRuby,F::kRubyOffers},
        {G::Emerald,&F::kEmerald,F::kEmeraldOffers},
        {G::FireRed,&F::kFireRed,F::kFireRedOffers},
        {G::LeafGreen,&F::kLeafGreen,F::kLeafGreenOffers}
    }};
    std::uint32_t checks=0,offers=0;
    for(const auto& source:families) {
        std::uint32_t gameOffers=0;
        for(std::uint16_t sp=1;sp<=386;++sp) {
            if(!L::supported(source.game,sp)) {
                std::cerr<<"L3 missing original game/species "<<static_cast<int>(source.game)<<' '<<sp<<'\n';
                return 1;
            }
            const std::uint16_t count=L::offerCount(source.game,sp);
            if(count==0 || count>32) {
                std::cerr<<"L3 original count unsupported "<<sp<<'\n';return 1;
            }
            bool structurallyValid=true;
            std::uint8_t lastLevel=0;
            auto pack=[&](std::uint16_t index) noexcept -> std::uint16_t {
                const auto o=L::offerAt(source.game,sp,index);
                if(o.level<1 || o.level>100 || o.move<1 || o.move>354 ||
                   o.level<lastLevel)structurallyValid=false;
                lastLevel=o.level;
                return static_cast<std::uint16_t>(
                    (static_cast<std::uint16_t>(o.level)<<9)|o.move);
            };
            const auto actual=F::fingerprint(sp,count,pack);
            if(!structurallyValid || actual!=(*source.original)[sp]) {
                std::cerr<<"L3 original source fingerprint mismatch: game="
                         <<static_cast<int>(source.game)<<" national="<<sp
                         <<" count="<<count<<" actual=0x"<<std::hex<<actual
                         <<" expected=0x"<<(*source.original)[sp]<<std::dec<<'\n';
                return 1;
            }
            gameOffers+=count;
            ++checks;
        }
        if(gameOffers!=source.offers) {
            std::cerr<<"L3 source total mismatch game="<<static_cast<int>(source.game)
                     <<" actual="<<gameOffers<<" expected="<<source.offers<<'\n';
            return 1;
        }
        offers+=gameOffers;
    }
    unsigned rubyVsEmerald=0,redVsGreen=0,rubyVsGreen=0;
    for(std::uint16_t sp=1;sp<=386;++sp) {
        rubyVsEmerald+=(F::kRuby[sp]!=F::kEmerald[sp]);
        redVsGreen+=(F::kFireRed[sp]!=F::kLeafGreen[sp]);
        rubyVsGreen+=(F::kRuby[sp]!=F::kLeafGreen[sp]);
    }
    if(rubyVsEmerald!=1 || redVsGreen!=2 || rubyVsGreen!=60) {
        std::cerr<<"L3 cartridge-branch identity mismatch\n";return 1;
    }
    if(L::offerCount(G::FireRed,386)!=12 ||
       L::offerCount(G::LeafGreen,386)!=14 ||
       L::offerCount(G::FireRed,51)!=13 ||
       L::offerCount(G::LeafGreen,51)!=13) return 1;
    if(checks!=1930 || offers!=19865) return 1;
    std::cout<<"L3 pinned original complete level-up source: "<<checks
             <<" source/species rows, "<<offers
             <<" chronologically ordered offers PASS\n";
}
