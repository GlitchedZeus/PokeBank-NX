#include "Legality/Gen4EggIVHistoricalCandidateSet.h"
#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
namespace H=Legality::Gen4EggIVHistoricalCandidateSet;
namespace IV=Legality::Gen4EggIVInheritanceStage;

// Pinned original HGSS C: pret/pokeheartgold@9d8b7591,
// get_egg.c InheritIVs/_IVList_Remove/Daycare_TryGetForcedInheritedIV.
// The published RNG-003 research independently compared 25,382 retail-C
// and Python stages. All observations here are SYNTHETIC STAGE fixtures,
// not preserved real parental or child Pokémon.
constexpr std::array<uint8_t,6> initial{0,3,6,9,12,15};
constexpr std::array<uint8_t,6> slot0{7,12,17,22,27,0};
constexpr std::array<uint8_t,6> slot1{13,24,3,14,25,4};
constexpr std::array<uint16_t,7> items{0,294,289,290,293,291,292};
H::Input witness(const char* game,uint16_t item0,uint16_t item1,
                 uint32_t seed=0) {
    H::Input in{};
    in.game=game;
    in.initialChild=initial;in.physicalSlot0=slot0;in.physicalSlot1=slot1;
    const auto generated=IV::replay(game,seed,initial,slot0,slot1,item0,item1);
    assert(generated.status==IV::Status::ReplayedStage);
    in.observedChild=generated.childIVs;
    in.originalPhysicalParentSnapshotsVerified=true;
    in.originalPhysicalSlotOrderVerified=true;
    in.originalInitialChildIVsVerified=true;
    in.originalUnhatchedChildIVsVerified=true;
    in.originalStageIdentityIndependentlyVerified=true;
    in.originalStageIdentity=0xAABBCCDDu;
    in.allIVStageHypothesesIndependentlyEnumerated=true;
    in.count=1;
    auto& h=in.histories[0];
    h.routeIdentity=1;h.stageIdentity=in.originalStageIdentity;
    h.independentSourceAttestation=true;
    h.historicalHeldItemsKnown=true;
    h.physicalSlot0Item=item0;h.physicalSlot1Item=item1;
    h.finalRNGStateKnown=true;h.finalRNGState=generated.finalSeed;
    return in;
}
int main(){
    unsigned checks=0;
    // Permanent unchanged original retail fixture controls. Synthetic data
    // and exact original replay outputs are fixed, NOT generated expectations.
    {
        auto n=witness("heartgold_nds",0,0);
        assert((n.observedChild==std::array<uint8_t,6>({7,3,17,9,12,0})));
        assert(n.histories[0].finalRNGState==0xE2CCA5EEu);
        assert(H::evaluate(n).status==H::Status::ConditionalHistoryMatched);
        auto one=witness("soulsilver_nds",294,0);
        assert((one.observedChild==std::array<uint8_t,6>({7,24,6,9,27,15})));
        assert(one.histories[0].finalRNGState==0x31B0DDE4u);
        auto reverse=witness("heartgold_nds",0,294);
        assert((reverse.observedChild==std::array<uint8_t,6>({13,24,6,9,27,15})));
        auto same=witness("heartgold_nds",294,294);
        assert((same.observedChild==std::array<uint8_t,6>({13,3,17,9,12,0})));
        assert(same.histories[0].finalRNGState==0x8E425287u);
        assert(IV::drawsFor(IV::Family::HeartGoldSoulSilver,294,294)==5);
        auto diff=witness("heartgold_nds",294,289);
        assert((diff.observedChild==std::array<uint8_t,6>({0,24,17,9,12,0})));
        assert(H::evaluate(one).status==H::Status::ConditionalHistoryMatched);
        assert(H::evaluate(reverse).status==H::Status::ConditionalHistoryMatched);
        assert(H::evaluate(same).status==H::Status::ConditionalHistoryMatched);
        assert(H::evaluate(diff).status==H::Status::ConditionalHistoryMatched);
        checks+=5;
    }
    // All six targeted Power stats, physical slot0 and slot1, identical
    // and distinct two-holder items, 259 original research seed controls.
    for(uint32_t seed=0;seed<259;++seed) {
        for(uint16_t a:items)for(uint16_t b:items) {
            auto hgss=witness("heartgold_nds",a,b,seed);
            const auto replay=IV::replay("heartgold_nds",seed,initial,
                                         slot0,slot1,a,b);
            assert(replay.distinctInheritedStats==3);
            assert(replay.drawCount==IV::drawsFor(IV::Family::HeartGoldSoulSilver,a,b));
            const auto r=H::evaluate(hgss);
            assert(r.status==H::Status::ConditionalHistoryMatched);
            assert(r.matched==1 && r.contradicted==0 && r.unknown==0);
            // Pt's independently pinned stage ignores Power items. The
            // same physical donors can give repeated inherited indices.
            auto pt=witness("platinum_nds",a,b,seed);
            pt.histories[0].historicalHeldItemsKnown=false;
            assert(H::evaluate(pt).status==H::Status::ConditionalHistoryMatched);
            ++checks;
        }
    }
    // A divergent historical candidate is a LOCAL contradiction. Even
    // all-negative hypotheses are not a global Pokemon Invalid verdict.
    auto known=witness("heartgold_nds",294,0);
    {
        auto bad=known;bad.histories[0].finalRNGState^=0x101u;
        assert(H::evaluate(bad).status==H::Status::AllEnumeratedStageHypothesesContradicted);
        bad.allIVStageHypothesesIndependentlyEnumerated=false;
        assert(H::evaluate(bad).status==H::Status::UnknownHistoryEnumeration);
        auto original=bad;
        original.count=2;original.histories[1]=known.histories[0];
        original.histories[1].routeIdentity=2;
        assert(H::evaluate(original).status==H::Status::ConditionalHistoryMatched);
        assert(H::evaluate(original).matched==1);
        checks+=4;
    }
    // Never use present-day item values or missing historical item
    // evidence to manufacture the 4-/5-/6-draw boundary.
    {
        auto missing=known;missing.histories[0].historicalHeldItemsKnown=false;
        assert(H::evaluate(missing).status==H::Status::UnresolvedCandidateHistory);
        assert(H::evaluate(missing).candidates[0]==IV::CandidateCheck::UnknownHeldItemHistory);
        auto absentRNG=known;absentRNG.histories[0].finalRNGStateKnown=false;
        assert(H::evaluate(absentRNG).status==H::Status::UnresolvedCandidateHistory);
        auto absentAttestation=known;absentAttestation.histories[0].independentSourceAttestation=false;
        assert(H::evaluate(absentAttestation).status==H::Status::UnresolvedCandidateHistory);
        auto wrongDay=known;wrongDay.histories[0].stageIdentity=0xAABBCCDEu;
        assert(H::evaluate(wrongDay).status==H::Status::UnresolvedCandidateHistory);
        auto zeroToken=known;zeroToken.originalStageIdentity=0;
        assert(H::evaluate(zeroToken).status==H::Status::IncoherentStageIdentity);
        auto noParent=known;noParent.originalPhysicalParentSnapshotsVerified=false;
        assert(H::evaluate(noParent).status==H::Status::UnknownOriginalParentStage);
        auto unknownOrder=known;unknownOrder.originalPhysicalSlotOrderVerified=false;
        assert(H::evaluate(unknownOrder).status==H::Status::UnknownOriginalParentStage);
        auto originalIVUnknown=known;originalIVUnknown.originalInitialChildIVsVerified=false;
        assert(H::evaluate(originalIVUnknown).status==H::Status::UnknownOriginalParentStage);
        auto unknownEgg=known;unknownEgg.originalUnhatchedChildIVsVerified=false;
        assert(H::evaluate(unknownEgg).status==H::Status::UnknownOriginalEggStage);
        checks+=9;
    }
    {
        auto zero=witness("heartgold_nds",0,0);
        zero.histories[0].routeIdentity=0;
        assert(H::evaluate(zero).status==H::Status::MissingOrDuplicateRouteIdentity);
        auto twice=known;twice.count=2;twice.histories[1]=twice.histories[0];
        const auto duplicateWithMatch=H::evaluate(twice);
        assert(duplicateWithMatch.status==H::Status::ConditionalHistoryMatched);
        assert(duplicateWithMatch.matched==1 && duplicateWithMatch.unknown==1);
        twice.histories[0].independentSourceAttestation=false;
        assert(H::evaluate(twice).status==H::Status::MissingOrDuplicateRouteIdentity);
        twice=known;twice.count=2;twice.histories[1]=twice.histories[0];
        twice.histories[1].routeIdentity=2;
        twice.histories[1].independentSourceAttestation=false;
        assert(H::evaluate(twice).status==H::Status::ConditionalHistoryMatched);
        auto malformed=known;malformed.initialChild[0]=32;
        assert(H::evaluate(malformed).status==H::Status::MalformedOriginalIVs);
        auto empty=known;empty.count=0;
        assert(H::evaluate(empty).status==H::Status::NoCandidates);
        auto tooMany=known;tooMany.count=17;
        assert(H::evaluate(tooMany).status==H::Status::CapacityExceeded);
        auto diamond=known;diamond.game="diamond_nds";
        assert(H::evaluate(diamond).status==H::Status::UnsupportedGame);
        auto pearl=known;pearl.game="pearl_nds";
        assert(H::evaluate(pearl).status==H::Status::UnsupportedGame);
        checks+=8;
    }
    std::cout<<"L2 B16 attested Gen IV IV hypotheses: "<<checks
             <<" independently gated cases PASS (stage-only, no PK4 authenticity)\n";
}
