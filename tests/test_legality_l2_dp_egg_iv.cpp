#include "Legality/Gen4DiamondPearlEggIVStage.h"
#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
namespace DP=Legality::Gen4DiamondPearlEggIVStage;
constexpr std::array<uint8_t,6> kInitial{0,3,6,9,12,15};
constexpr std::array<uint8_t,6> kSlot0{7,12,17,22,27,0};
constexpr std::array<uint8_t,6> kSlot1{13,24,3,14,25,4};
struct Fixed {uint32_t entry,finish;std::array<uint8_t,6> child;
              std::array<uint8_t,3> selected,donor;};
// Independent hand-emulated Diamond/Pearl original Thumb control-flow:
// pret/pokediamond@b623abef..., ov05_021EC9D4 / ov05_021EC99C,
// plus LCRandom from arm9/src/math_util.c. These are SYNTHETIC STAGE
// vectors; no retail-save authenticity is represented.
constexpr std::array<Fixed,5> oracle{{
 {0x00000000u,0xE2CCA5EEu,{7,3,6,22,12,0},{0,5,3},{0,0,0}},
 {0x00000001u,0xB6A8FDE7u,{0,24,3,9,27,15},{2,1,4},{1,1,0}},
 {0x00000002u,0x8A8555E0u,{0,24,6,9,25,15},{4,1,1},{1,0,1}},
 {0x12345678u,0x983788A6u,{0,12,17,9,25,15},{1,2,4},{0,0,1}},
 {0xFFFFFFFFu,0x0EF04DF5u,{0,3,17,14,12,0},{2,3,5},{0,1,0}}
}};
int main() {
    unsigned checks=0, repeated=0;
    for(const char* game:{"diamond_nds","pearl_nds"}) {
        for(const auto& fixture:oracle) {
            auto r=DP::replay(game,fixture.entry,kInitial,kSlot0,kSlot1);
            assert(r.status==DP::Status::Replayed);
            assert(r.finalSeed==fixture.finish);
            assert(r.child==fixture.child);
            assert(r.selected==fixture.selected);
            assert(r.donor==fixture.donor);
            assert(DP::rewind(r.finalSeed)==fixture.entry);
            DP::OriginalStage witness{};
            witness.game=game;
            witness.initialChild=kInitial;witness.physicalSlot0=kSlot0;
            witness.physicalSlot1=kSlot1;witness.originalEggIVs=fixture.child;
            witness.finalSeed=fixture.finish;
            witness.historicalParentIVsVerified=true;
            witness.physicalDepositOrderVerified=true;
            witness.preInheritanceChildIVsVerified=true;
            witness.originalUnhatchedEggIVsVerified=true;
            witness.finalSeedIndependentlyVerified=true;
            assert(DP::verify(witness)==DP::Check::Matched);
            auto neg=witness;neg.originalEggIVs[0]=
                static_cast<uint8_t>(neg.originalEggIVs[0]==31?30:31);
            assert(DP::verify(neg)==DP::Check::Contradicted);
            auto noParent=witness;noParent.historicalParentIVsVerified=false;
            assert(DP::verify(noParent)==DP::Check::UnknownParentIVs);
            auto noSlots=witness;noSlots.physicalDepositOrderVerified=false;
            assert(DP::verify(noSlots)==DP::Check::UnknownDepositOrder);
            auto noPre=witness;noPre.preInheritanceChildIVsVerified=false;
            assert(DP::verify(noPre)==DP::Check::UnknownInitialChildIVs);
            auto noEgg=witness;noEgg.originalUnhatchedEggIVsVerified=false;
            assert(DP::verify(noEgg)==DP::Check::UnknownOriginalChildIVs);
            auto noSeed=witness;noSeed.finalSeedIndependentlyVerified=false;
            assert(DP::verify(noSeed)==DP::Check::UnknownFinalSeed);
            auto bad=witness;bad.physicalSlot1[2]=32;
            assert(DP::verify(bad)==DP::Check::MalformedIVs);
            if(r.distinctSelected<3)++repeated;
            ++checks;
        }
    }
    assert(repeated>=2); // DP original removal-index repetition, not HGSS.
    // Move-independent array/domain and game specificity.
    const auto badInitial=std::array<uint8_t,6>{32,0,0,0,0,0};
    assert(DP::replay("diamond_nds",0,badInitial,kSlot0,kSlot1).status==
           DP::Status::MalformedIVs);
    assert(DP::replay("platinum_nds",0,kInitial,kSlot0,kSlot1).status==
           DP::Status::UnsupportedGame);
    assert(DP::replay("heartgold_nds",0,kInitial,kSlot0,kSlot1).status==
           DP::Status::UnsupportedGame);
    // The original DP function has no Power item branch: always exactly
    // six LCRandom steps regardless of contemporary held-item claims.
    for(uint32_t seed=0;seed<4096;++seed) {
        for(const char* game:{"diamond_nds","pearl_nds"}) {
            const auto r=DP::replay(game,seed,kInitial,kSlot0,kSlot1);
            assert(r.status==DP::Status::Replayed);
            assert(DP::rewind(r.finalSeed)==seed);
            assert(r.distinctSelected>=1 && r.distinctSelected<=3);
            for(uint8_t i=0;i<6;++i)assert(r.child[i]<=31);
            ++checks;
        }
    }
    std::cout<<"L2 B12 original Diamond/Pearl assembly egg IV: "<<checks
             <<" positive + negative + Unknown controls PASS\n";
}
