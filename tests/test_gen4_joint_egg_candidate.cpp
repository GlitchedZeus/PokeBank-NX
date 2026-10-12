#include "Legality/Gen4JointEggCandidateEvidence.h"
#include <cassert>
#include <iostream>
namespace H=Legality::Gen4JointEggCandidateEvidence;
namespace G=Legality::Gen4EggIVInheritanceStage;
struct Fixture {
    bool hgss;uint32_t seed,finalSeed;std::array<uint16_t,2> items;
    G::IVs initial,parent0,parent1,child;
    std::array<uint16_t,6> draws;uint8_t count;
    std::array<uint8_t,3> selected,donors;
};
#include "gen4_egg_iv_stage_vectors.inc"
H::Input input(const Fixture& f,bool fatherFirst=true) {
    H::Input in{};auto& m=in.moves;
    m.game=f.hgss?"heartgold_nds":"platinum_nds";m.species=1;
    m.fatherId=fatherFirst?101:202;m.motherId=fatherFirst?202:101;
    m.snapshotsKnown=true;m.fatherSpecies=m.motherSpecies=1;
    m.pairProof=H::Move::PairProof::IndependentlyCompatible;
    m.base=m.father=m.mother=m.observed={33,45,0,0};
    in.daycareParents={H::Parent{101,true,f.parent0,true,f.items[0]},
                       H::Parent{202,true,f.parent1,true,f.items[1]}};
    in.daycareOrderVerified=in.sharedCollectionStageVerified=true;
    in.moveStageToken=in.ivStageToken=1234;
    in.initialIVsKnown=in.finalRNGStateKnown=true;
    in.initialIVs=f.initial;in.observedIVs=f.child;in.finalRNGState=f.finalSeed;
    return in;
}
int main(){
    for(const auto& f:fixtures)for(bool fatherFirst:{true,false}) {
        auto in=input(f,fatherFirst);
        assert(H::evaluate(in).status==H::Status::KnownCandidateConsistent);
        in.observedIVs[0]=(in.observedIVs[0]+1)%32;
        assert(H::evaluate(in).status==H::Status::IVCandidateContradicted);
    }
    const auto original=input(fixtures[0]);
    auto check=[&](auto mutate,H::Status expected){auto in=original;mutate(in);assert(H::evaluate(in).status==expected);};
    check([](auto& x){x.moves.game="diamond_nds";},H::Status::UnsupportedGame);
    check([](auto& x){x.moves.snapshotsKnown=false;},H::Status::UnknownParentSnapshots);
    check([](auto& x){x.daycareParents[0].id=303;},H::Status::IncoherentParents);
    check([](auto& x){x.daycareParents[0].id=202;},H::Status::IncoherentParents);
    check([](auto& x){x.moves.motherId=x.moves.fatherId;},H::Status::IncoherentParents);
    check([](auto& x){x.daycareOrderVerified=false;},H::Status::UnknownDaycareOrder);
    check([](auto& x){x.sharedCollectionStageVerified=false;},H::Status::UnknownSharedStage);
    check([](auto& x){x.ivStageToken=0;},H::Status::UnknownSharedStage);
    check([](auto& x){x.ivStageToken=5678;},H::Status::IncoherentStage);
    check([](auto& x){x.moves.father[0]=468;},H::Status::MalformedMoveInput);
    check([](auto& x){x.moves.mother={33,0,45,0};},H::Status::MalformedMoveInput);
    check([](auto& x){x.moves.base={33,33,0,0};},H::Status::MalformedMoveInput);
    check([](auto& x){x.moves.pairProof=static_cast<H::Move::PairProof>(255);},H::Status::MalformedMoveInput);
    check([](auto& x){x.moves.pairProof=H::Move::PairProof::Unknown;},H::Status::MoveHistoryUnresolved);
    check([](auto& x){x.moves.pairProof=H::Move::PairProof::IndependentlyIncompatible;},H::Status::MoveCandidateContradicted);
    check([](auto& x){x.moves.observed={33,0,0,0};},H::Status::MoveCandidateContradicted);
    check([](auto& x){x.daycareParents[0].ivsKnown=false;},H::Status::IVHistoryUnresolved);
    check([](auto& x){x.daycareParents[0].heldItemKnown=false;},H::Status::IVHistoryUnresolved);
    check([](auto& x){x.initialIVsKnown=false;},H::Status::IVHistoryUnresolved);
    check([](auto& x){x.finalRNGStateKnown=false;},H::Status::IVHistoryUnresolved);
    check([](auto& x){x.observedIVs[0]=32;},H::Status::MalformedIVInput);
    // Pichu uses the SAME item snapshots for Light Ball and IV Power effects.
    auto pichu=original;pichu.moves.species=172;
    pichu.moves.fatherSpecies=pichu.moves.motherSpecies=25;
    pichu.moves.base=pichu.moves.father=pichu.moves.mother={84,204,0,0};
    pichu.moves.observed={84,204,344,0};
    pichu.daycareParents[0].heldItem=236; // Light Ball, no Power effect
    pichu.moves.lightBallKnown=false; // derived from actual same-stage items
    assert(H::evaluate(pichu).status==H::Status::KnownCandidateConsistent);
    pichu.daycareParents[0].heldItem=0;
    pichu.moves.lightBallKnown=pichu.moves.lightBall=true; // stale independent claim
    assert(H::evaluate(pichu).status==H::Status::MoveCandidateContradicted);
    pichu.daycareParents[0].heldItemKnown=false;
    auto unresolved=H::evaluate(pichu);
    assert(unresolved.status==H::Status::MoveHistoryUnresolved);
    assert(unresolved.moves.status==H::Move::Status::UnknownLightBall);
    // One parent can hold Light Ball while the other supplies a Power stat.
    const Fixture* mixedFixture=nullptr;
    for(const auto& f:fixtures)
        if(f.hgss&&f.seed==0&&f.items[0]==0&&f.items[1]==294){mixedFixture=&f;break;}
    assert(mixedFixture);auto mixed=input(*mixedFixture);
    mixed.moves.species=172;mixed.moves.fatherSpecies=mixed.moves.motherSpecies=25;
    mixed.moves.base=mixed.moves.father=mixed.moves.mother={84,204,0,0};
    mixed.moves.observed={84,204,344,0};mixed.daycareParents[0].heldItem=236;
    assert(H::evaluate(mixed).status==H::Status::KnownCandidateConsistent);
    // Swapping just the items preserves Light Ball but changes the IV donor.
    mixed.daycareParents[0].heldItem=294;mixed.daycareParents[1].heldItem=236;
    assert(H::evaluate(mixed).status==H::Status::IVCandidateContradicted);
    // Platinum IVs ignore items, but Pichu's move history still requires them.
    const Fixture* platinum=nullptr;
    for(const auto& f:fixtures)if(!f.hgss){platinum=&f;break;}
    assert(platinum);auto pt=input(*platinum);
    pt.daycareParents[0].heldItemKnown=pt.daycareParents[1].heldItemKnown=false;
    assert(H::evaluate(pt).status==H::Status::KnownCandidateConsistent);
    pt.moves.species=172;pt.moves.fatherSpecies=pt.moves.motherSpecies=25;
    pt.moves.base=pt.moves.father=pt.moves.mother=pt.moves.observed={84,204,0,0};
    assert(H::evaluate(pt).status==H::Status::MoveHistoryUnresolved);
    std::cout<<"Gen IV joint egg: 276 fixed stage/role cases and coherence controls PASS\n";
}
