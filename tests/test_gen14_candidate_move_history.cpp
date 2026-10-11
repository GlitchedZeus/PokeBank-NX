#include "Legality/Gen14CandidateMoveHistory.h"
#include <cassert>
#include <iostream>
using namespace Legality::Gen14CandidateMoveHistory;
constexpr Witness event(uint16_t move,uint32_t cand,uint16_t dist){
    return {move,cand,Method::Event,5,100,0,dist,0,true};
}
constexpr Witness sibling(uint16_t move,uint32_t cand,uint8_t level){
    return {move,cand,Method::SiblingEvolution,level,level,1,0,0,true};
}
constexpr Witness tutor(uint16_t move,uint32_t cand,uint8_t level){
    return {move,cand,Method::Tutor,level,100,2,0,0,true};
}
constexpr Slot slot(uint16_t move,Witness a,bool exhaustive=true){
    Slot s{};s.move=move;s.options[0]=a;s.count=1;s.exhaustive=exhaustive;return s;
}
constexpr Candidate full{42,50,5,true};
static_assert(evaluate(full,{slot(97,sibling(97,42,38)),slot(226,sibling(226,42,45)),{}, {}}).status==Status::CandidateContradicted);
static_assert(evaluate(full,{slot(97,sibling(97,42,38)),slot(226,sibling(226,42,38)),{}, {}}).status==Status::WitnessesCompatible);
static_assert(evaluate(full,{slot(97,sibling(97,42,38)),slot(226,tutor(226,42,40)),{}, {}}).status==Status::WitnessesCompatible);
static_assert(evaluate(full,{slot(97,event(97,42,10)),slot(226,event(226,42,11)),{}, {}}).status==Status::CandidateContradicted);
static_assert(evaluate(full,{slot(97,event(97,42,10)),slot(226,event(226,42,10)),{}, {}}).status==Status::WitnessesCompatible);
static_assert(evaluate(full,{slot(97,event(97,42,10)),slot(226,event(226,41,10)),{}, {}}).status==Status::CandidateContradicted);
static_assert(evaluate(full,{slot(97,event(97,42,10)),slot(226,event(226,41,10),false),{}, {}}).status==Status::Unresolved);
static_assert(evaluate(full,{slot(97,event(97,42,0)),{}, {}, {}}).status==Status::Unresolved);
static_assert(evaluate(full,{slot(97,Witness{97,42,Method::BroadPool,5,100,0,0,0,false}),{}, {}, {}}).status==Status::Unresolved);
static_assert(evaluate(full,{}).status==Status::Empty);
static_assert(evaluate(Candidate{42,50,5,true,0},{slot(97,sibling(97,42,38)),{}, {}, {}}).status==Status::CandidateContradicted);
constexpr Slot alternativeEvent() {
    Slot s=slot(226,event(226,42,11));
    s.options[1]=event(226,42,10);s.count=2;return s;
}
static_assert(evaluate(full,{slot(97,event(97,42,10)),alternativeEvent(),{}, {}}).status==Status::WitnessesCompatible);
static_assert(evaluate({42,30,5,true},{slot(97,sibling(97,42,38)),{}, {}, {}}).status==Status::CandidateContradicted);
static_assert(evaluate({42,50,40,true},{slot(97,sibling(97,42,38)),{}, {}, {}}).status==Status::CandidateContradicted);

// TRANSFER-001/MOVE-001 joint candidate history: a verified Pal Park
// boundary at stage 2 eliminates OLD Gen III HM retention, but a newly
// learned Gen IV move after stage 2 remains a possible candidate witness.
// Exhaustive here only covers the explicitly hypothetical route, not all
// encounter, transfer, evolution, egg or later teaching histories.
constexpr Candidate palParkCandidate{42,70,5,true,5,2};
constexpr Witness oldSurf{57,42,Method::TransferRetained,5,70,1,0,0,true};
constexpr Witness laterSurf{57,42,Method::LaterAcquisition,5,70,3,0,0,true};
constexpr Witness tooEarlySurf{57,42,Method::LaterAcquisition,5,70,2,0,0,true};
constexpr Witness oldThunderbolt{85,42,Method::TransferRetained,5,70,1,0,0,true};
static_assert(evaluate(palParkCandidate,{slot(57,oldSurf),{},{},{}}).status==
              Status::CandidateContradicted);
static_assert(evaluate(palParkCandidate,{slot(57,laterSurf),{},{},{}}).status==
              Status::WitnessesCompatible);
static_assert(evaluate(palParkCandidate,{slot(57,tooEarlySurf),{},{},{}}).status==
              Status::CandidateContradicted);
static_assert(evaluate(palParkCandidate,{slot(85,oldThunderbolt),{},{},{}}).status==
              Status::WitnessesCompatible);
static_assert(evaluate(Candidate{42,70,5,false,5,2},
                       {slot(57,oldSurf),{},{},{}}).status==
              Status::Unresolved);
// Without proven Pal Park provenance, never fabricate the HM boundary.
static_assert(evaluate(full,{slot(57,oldSurf),{},{},{}}).status==
              Status::WitnessesCompatible);
constexpr Slot alternativeSurf() {
    Slot s=slot(57,oldSurf);
    s.options[1]=laterSurf;s.count=2;return s;
}
static_assert(evaluate(palParkCandidate,{slot(57,alternativeSurf()),
                                         slot(85,oldThunderbolt),{},{}}).status==
              Status::WitnessesCompatible);

int main(){
    auto a=evaluate(full,{slot(97,sibling(97,42,38)),slot(226,sibling(226,42,45)),{}, {}});
    assert(a.status==Status::CandidateContradicted && a.storedSlots==2);
    auto b=evaluate({42,50,5,false},{slot(97,sibling(97,42,38)),slot(226,sibling(226,42,45)),{}, {}});
    assert(b.status==Status::Unresolved);
    std::cout<<"Candidate-scoped I-IV move-history evidence: PASS\n";
}
