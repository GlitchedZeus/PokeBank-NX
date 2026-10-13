#include "Legality/Gen14HistoricalEventPathEvidence.h"
#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
namespace P=Legality::Gen14HistoricalEventPathEvidence;
constexpr auto g3=P::Generation::III, g4=P::Generation::IV;
P::Snapshot filled(){P::Snapshot s{};s.known=P::kAllFields;for(size_t i=0;i<P::kFields;++i)s.value[i]=101u+i;return s;}
int main(){
  const auto original=filled(); P::Source source{};
  source.exactOriginalSourceIndependentlyAttested=true;
  source.originalFieldsIndependentlyExtracted=true;
  source.originalGeneration=g3;source.original=original;
  source.exhaustiveHistoricalChainVerified=true;
  const P::Step park{P::Action::PalPark,g3,g4,true,0,{}};
  auto r=P::evaluate(true,g4,original,source,std::array{park});
  assert(r.status==P::Status::UnknownHistoricalTransformation);
  assert((r.unknown&(P::NicknameRaw|P::OriginalTrainerRaw|P::MetDate))!=0);
  // A complete, verified snapshot after the encoding boundary permits
  // per-field comparisons, but still does not authenticate a distribution.
  auto fullPark=park;
  fullPark.mutated=P::NicknameRaw|P::OriginalTrainerRaw|P::MetDate;
  fullPark.verifiedAfter=original;
  r=P::evaluate(true,g4,original,source,std::array{fullPark});
  assert(r.status==P::Status::ConditionalCandidate&&!r.authenticated);
  auto wrong=original;wrong.value[3]++;
  r=P::evaluate(true,g4,wrong,source,std::array{fullPark});
  assert(r.status==P::Status::CandidateFieldContradicted);
  assert(r.contradicted==P::OriginalBall);
  // Even a perfect match with an unverified source/route is Unknown.
  auto s=source;s.exactOriginalSourceIndependentlyAttested=false;
  assert(P::evaluate(true,g4,wrong,s,std::array{fullPark}).status==P::Status::UnknownOriginalSource);
  s=source;s.exhaustiveHistoricalChainVerified=false;
  assert(P::evaluate(true,g4,wrong,s,std::array{fullPark}).status==P::Status::UnknownHistoricalTransformation);
  auto tampered=fullPark;tampered.independentlyVerified=false;
  assert(P::evaluate(true,g4,wrong,source,std::array{tampered}).status==P::Status::UnknownHistoricalTransformation);
  tampered=fullPark;tampered.mutated|=P::OriginalBall;
  assert(P::evaluate(true,g4,wrong,source,std::array{tampered}).status==P::Status::CandidateRouteContradicted);
  tampered=fullPark;tampered.to=g3;
  assert(P::evaluate(true,g4,wrong,source,std::array{tampered}).status==P::Status::CandidateRouteContradicted);
  // Trade cannot silently replace historical OT IDs; evolving can change
  // species/moves/nickname, item swap only the held item, contest only ribbon.
  constexpr auto same=P::Generation::IV;
  for(P::Action action:{P::Action::Trade,P::Action::Evolution,P::Action::Hatch,
      P::Action::MoveChange,P::Action::Renaming,P::Action::Contest,
      P::Action::ItemChange,P::Action::ClockChange}) {
    P::Source root=source;root.originalGeneration=same;
    for(unsigned f=0;f<P::kFields;++f){
      P::Step step{action,same,same,true,1u<<f,{}};
      step.verifiedAfter=original;
      const auto result=P::evaluate(true,same,original,root,std::array{step});
      const bool allowed=(P::maskFor(action)&(1u<<f))!=0;
      assert((result.status!=P::Status::CandidateRouteContradicted)==allowed);
    }
  }
  // Adversarial 15-field mask sweep; changed values in completely verified
  // carried fields contradict only the specific candidate. Evolving nickname
  // raw bytes are explicitly Unknown unless separately witnessed after.
  uint64_t negatives=0,conditional=0;
  for(uint32_t mask=0;mask<=P::kAllFields;++mask){
    P::Snapshot now=original;
    for(unsigned i=0;i<P::kFields;++i)if(mask&(1u<<i))now.value[i]++;
    const auto result=P::evaluate(true,g4,now,source,std::array{fullPark});
    assert(result.contradicted==mask);
    assert((result.status==P::Status::CandidateFieldContradicted)==(mask!=0));
    if(mask)++negatives;else ++conditional;
    // Marking one unknown final field can never make it a contradiction.
    now.known&=~P::OriginalBall;
    auto less=P::evaluate(true,g4,now,source,std::array{fullPark});
    assert((less.contradicted&P::OriginalBall)==0);
  }
  // Chronology: 16 maximal steps, 17 steps Unknown rather than overflow.
  std::array<P::Step,17> trades{};
  for(auto& t:trades)t={P::Action::Trade,g4,g4,true,0,{}};
  s=source;s.originalGeneration=g4;
  assert(P::evaluate(true,g4,original,s,std::span(trades.data(),16)).status==P::Status::ConditionalCandidate);
  assert(P::evaluate(true,g4,original,s,trades).status==P::Status::UnknownHistoricalTransformation);
  assert(!P::evaluate(false,g3,original,source).authenticated);
  std::cout<<"L4 Stage4 historical paths: "<<negatives<<" field-mask negatives, "<<conditional
           <<" consistent, 120 action/field combinations: PASS\n";
}
