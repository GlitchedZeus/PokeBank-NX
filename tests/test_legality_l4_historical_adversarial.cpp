#include "Legality/Gen14HistoricalEventPathEvidence.h"
#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
namespace P=Legality::Gen14HistoricalEventPathEvidence;
int main(){
  P::Snapshot original{},observed{};
  original.known=observed.known=P::kAllFields;
  for(size_t i=0;i<P::kFields;++i)original.value[i]=observed.value[i]=i+400;
  P::Source src{};src.exactOriginalSourceIndependentlyAttested=true;
  src.originalFieldsIndependentlyExtracted=true;
  src.originalGeneration=P::Generation::IV;
  src.original=original;src.exhaustiveHistoricalChainVerified=true;
  // Every field mutation must be independently projected after the stated
  // event, otherwise it is Unknown rather than invalidating a gift.
  uint64_t conditional=0,unknown=0,conflicting=0;
  for(size_t field=0;field<P::kFields;++field)
  for(size_t retained=0;retained<2;++retained)
  for(size_t changed=0;changed<2;++changed)
  for(size_t finalKnown=0;finalKnown<2;++finalKnown) {
    const uint32_t bit=1u<<field;
    P::Step s{P::Action::Trade,P::Generation::IV,P::Generation::IV,true,0,{}};
    s.mutated=(P::maskFor(s.action)&bit)?bit:0;
    if(retained)s.verifiedAfter.known=bit;
    s.verifiedAfter.value[field]=original.value[field]+changed;
    auto end=observed;end.value[field]+=changed;
    if(!finalKnown)end.known&=~bit;
    auto result=P::evaluate(true,P::Generation::IV,end,src,std::array{s});
    assert(!result.authenticated);
    if(result.status==P::Status::CandidateFieldContradicted)++conflicting;
    else if(result.status==P::Status::UnknownHistoricalTransformation)++unknown;
    else if(result.status==P::Status::ConditionalCandidate)++conditional;
    else assert(false);
  }
  // Two honest consecutive independently attested changes produce an
  // exact final witness, without presuming intermediate timestamps are
  // monotonic or that a Pokémon has an authenticated archive.
  std::array<P::Step,2> steps{};
  steps[0]={P::Action::MoveChange,P::Generation::IV,P::Generation::IV,true,P::MoveSet,{}};
  steps[0].verifiedAfter.known=P::MoveSet;steps[0].verifiedAfter.value[7]=1234;
  steps[1]={P::Action::Contest,P::Generation::IV,P::Generation::IV,true,P::Ribbons,{}};
  steps[1].verifiedAfter.known=P::Ribbons;steps[1].verifiedAfter.value[8]=55;
  auto now=observed;now.value[7]=1234;now.value[8]=55;
  auto result=P::evaluate(true,P::Generation::IV,now,src,steps);
  assert(result.status==P::Status::ConditionalCandidate);
  now.value[8]=54;
  result=P::evaluate(true,P::Generation::IV,now,src,steps);
  assert(result.contradicted==P::Ribbons);
  // Documented capsule crossing between Gen I and II is possible; an
  // unattested direct Gen I -> IV jump is not magically inferred.
  P::Source i=src;i.originalGeneration=P::Generation::I;
  const P::Step capsule{P::Action::TimeCapsule,P::Generation::I,P::Generation::II,true,0,{}};
  result=P::evaluate(true,P::Generation::II,observed,i,std::array{capsule});
  assert(result.status==P::Status::UnknownHistoricalTransformation);
  const P::Step direct{P::Action::PalPark,P::Generation::I,P::Generation::IV,true,0,{}};
  assert(P::evaluate(true,P::Generation::IV,observed,i,std::array{direct}).status==P::Status::CandidateRouteContradicted);
  // No observed native fields is never a conditional provenance witness.
  now.known=0;
  assert(P::evaluate(true,P::Generation::IV,now,src).status==P::Status::UnknownHistoricalTransformation);
  std::cout<<"L4 Stage4 adversarial timeline: "<<conditional<<" conditional, "<<unknown
           <<" Unknown, "<<conflicting<<" candidate-only contradictions: PASS\n";
}
