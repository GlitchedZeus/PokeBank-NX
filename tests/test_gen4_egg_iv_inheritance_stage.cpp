#include "Legality/Gen4EggIVInheritanceStage.h"
#include <cassert>
#include <iostream>
namespace G=Legality::Gen4EggIVInheritanceStage;
struct Fixture {
    bool hgss;
    uint32_t seed,finalSeed;
    std::array<uint16_t,2> items;
    G::IVs initial,parent0,parent1,child;
    std::array<uint16_t,6> draws;
    uint8_t count;
    std::array<uint8_t,3> selected,donors;
};
#include "gen4_egg_iv_stage_vectors.inc"
G::Input input(const Fixture& f) {
    G::Input in{};in.game=f.hgss?"heartgold_nds":"platinum_nds";
    in.parentSlotsKnown=in.initialIVsKnown=in.heldItemsKnown=in.rngStateKnown=true;
    in.parents={f.parent0,f.parent1};in.initialIVs=f.initial;
    in.heldItems=f.items;in.rngState=f.seed;return in;
}
int main() {
    for(const auto& f:fixtures) {
        auto in=input(f);const auto r=G::replay(in);
        assert(r.status==G::Status::ReplayedStage);
        assert(r.childIVs==f.child && r.finalSeed==f.finalSeed);
        assert(r.draws==f.draws && r.drawCount==f.count);
        assert(r.selectedStats==f.selected && r.donorSlots==f.donors);
        assert(G::stageDrawCount(in)==f.count);
        if(f.hgss) {
            assert(r.selectedStats[0]!=r.selectedStats[1]);
            assert(r.selectedStats[0]!=r.selectedStats[2]);
            assert(r.selectedStats[1]!=r.selectedStats[2]);
            in.game="soulsilver_nds";
            assert(G::replay(in).childIVs==f.child);
        }
        in.rngState=f.finalSeed;
        assert(G::verifyFinalState(in,f.child)==G::CandidateCheck::Matched);
        auto bad=f.child;bad[0]=(bad[0]+1)%32;
        assert(G::verifyFinalState(in,bad)==G::CandidateCheck::CandidateMismatch);
        bad=f.child;bad[0]=32;
        assert(G::verifyFinalState(in,bad)==G::CandidateCheck::MalformedIVDomain);
    }
    auto in=input(fixtures[0]); // HGSS, seed 0, no items
    auto check=[&](G::Status s,G::CandidateCheck c) {
        assert(G::replay(in).status==s);
        assert(G::verifyFinalState(in,fixtures[0].child)==c);
    };
    in.game="diamond_nds";check(G::Status::UnsupportedGame,G::CandidateCheck::UnsupportedGame);
    in.game="emerald_gba";check(G::Status::UnsupportedGame,G::CandidateCheck::UnsupportedGame);
    in.game="heartgold_nds";
    in.parentSlotsKnown=false;check(G::Status::UnknownParentSlots,G::CandidateCheck::UnknownParentSlots);
    in.parentSlotsKnown=true;in.initialIVsKnown=false;
    check(G::Status::UnknownInitialIVs,G::CandidateCheck::UnknownInitialIVs);
    in.initialIVsKnown=true;in.heldItemsKnown=false;
    check(G::Status::UnknownHeldItems,G::CandidateCheck::UnknownHeldItems);
    assert(G::stageDrawCount(in)==0);
    in.heldItemsKnown=true;in.rngStateKnown=false;
    check(G::Status::UnknownRNGState,G::CandidateCheck::UnknownRNGState);
    in.rngStateKnown=true;
    for(unsigned which=0;which<3;++which) {
        auto malformed=in;
        (which==0?malformed.initialIVs:malformed.parents[which-1])[5]=255;
        assert(G::replay(malformed).status==G::Status::MalformedIVDomain);
        assert(G::verifyFinalState(malformed,fixtures[0].child)==G::CandidateCheck::MalformedIVDomain);
    }
    in.game="platinum_nds";in.heldItemsKnown=false;
    assert(G::replay(in).status==G::Status::ReplayedStage);
    in.rngState=2;const auto repeated=G::replay(in);
    assert((repeated.selectedStats==std::array<uint8_t,3>{4,1,1}));
    in.game="heartgold_nds";in.heldItemsKnown=true;in.rngState=0;
    in.heldItems={294,294};auto two=G::replay(in);
    assert(two.drawCount==5 && two.donorSlots[0]==1); // even selects slot 1
    in.heldItems={294,0};assert(G::replay(in).drawCount==4);
    in.heldItems={1,0};assert(G::replay(in).drawCount==6); // known non-Power item
    // Even all-identical IV values do not reveal which stats were selected.
    in.initialIVs.fill(31);in.parents[0].fill(31);in.parents[1].fill(31);
    assert(G::replay(in).childIVs==in.initialIVs);
    std::cout<<"Gen IV IV stage: 138 retail fixtures, inverse/mutation and Unknown controls PASS\n";
}
