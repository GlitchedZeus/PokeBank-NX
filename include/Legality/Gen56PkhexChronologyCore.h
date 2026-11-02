#pragma once
// L3 Phase A (Gen V/VI). Read-only source-provenance and move-slot history gate.
// Uses PKHeX LearnGroup5/6 ordering and title-specific LearnSource split as
// reference, not a claim of PKHeX-wide or retail historical parity.
// GPL-3.0-or-later provenance at docs/legality/l3/PHASE_A_GEN56_20261012.md.
#include "Legality/Gen56PkhexMachineSources.h"
#include <array>
#include <cstdint>
namespace Legality::Gen56PkhexChronologyCore {
namespace P=Gen56PkhexMachineSources;
using Game=P::Game;
enum class Status : std::uint8_t { Unknown, CandidateConsistent, CandidateContradicted };
enum class Action : std::uint8_t {
 Teach, OfferDecline, OfferAlreadyKnown, DeleteMove, VisitGame, Evolve,
 PokeTransfer4to5, BankTransfer5to6
};
enum class Method : std::uint8_t { None, LevelUp, TM, HM, Tutor, Relearner, Encounter };
struct Snapshot {
 std::uint64_t candidateId=0, individualId=0, snapshotId=0;
 Game game=Game::Unknown;
 std::uint16_t species=0;
 std::uint8_t form=0,level=0;
 std::array<std::uint16_t,4> moves{};
 bool independentlyAuthenticated=false;
};
struct Proof {
 std::uint64_t actionReceipt=0, originalSourceRecord=0;
 Game exactGame=Game::Unknown;
 std::uint16_t exactSpecies=0;
 std::uint8_t exactForm=0;
 // All required fields are independent of a detached PK5/PK6 current moveset.
 bool originalActionAuthenticated=false;
 bool sameIndividualAuthenticated=false;
 bool exactSourceAuthenticated=false;
 bool allIntermediateLevelDecisionsAuthenticated=false;
 // External PKHeX or retail data MUST independently attest species/form method.
 bool speciesCompatibleAuthenticated=false;
 bool cartridgeOrServiceAuthenticated=false;
 // Transfer witness must separately establish the genuine service, pairing,
 // met/location and exact snapshot preservation. No fake Bank history from PK6.
 bool transferRouteAuthenticated=false;
 bool destinationMarkerAuthenticated=false;
 bool fourSlotsPreservedAuthenticated=false;
 std::uint8_t machineNumber=0;
};
struct Step {
 Action action=Action::Teach;
 Method method=Method::None;
 std::uint16_t move=0;
 std::uint8_t slot=0;
 Snapshot before{},after{};
 Proof proof{};
};
struct Path {
 Snapshot initial{},observed{};
 std::array<Step,24> steps{};
 std::uint8_t stepCount=0;
 bool completeIntermediateSequenceAuthenticated=false;
};
struct Result {
 Status status=Status::Unknown;
 std::array<std::uint16_t,4> rebuilt{};
 std::uint8_t teachings=0,offers=0,visits=0,evolutions=0,transfers=0;
 bool globalInvalid=false;
};
constexpr bool validSlots(const std::array<std::uint16_t,4>& m, std::uint16_t max) noexcept {
 if(!m[0])return false;
 bool gap=false;
 for(unsigned i=0;i<4;++i){
  if(!m[i]){gap=true;continue;}
  if(gap||m[i]>max)return false;
  for(unsigned j=0;j<i;++j)if(m[j]==m[i])return false;
 }
 return true;
}
constexpr bool valid(const Snapshot& s) noexcept {
 return s.independentlyAuthenticated&&s.candidateId&&s.individualId&&s.snapshotId&&
  P::generation(s.game)&&s.species&&s.species<=P::maxSpecies(s.game)&&
  s.level&&s.level<=100&&validSlots(s.moves,P::maxMove(s.game));
}
constexpr bool equal(const Snapshot& a,const Snapshot& b) noexcept {
 return a.candidateId==b.candidateId&&a.individualId==b.individualId&&
 a.snapshotId==b.snapshotId&&a.game==b.game&&a.species==b.species&&
 a.form==b.form&&a.level==b.level&&a.moves==b.moves&&
 a.independentlyAuthenticated==b.independentlyAuthenticated;
}
constexpr std::uint8_t freeSlot(const std::array<std::uint16_t,4>& m) noexcept {
 for(std::uint8_t i=0;i<4;++i)if(!m[i])return i;
 return 4;
}
constexpr bool contains(const std::array<std::uint16_t,4>& m,std::uint16_t v) noexcept {
 for(auto x:m)if(x==v)return true;
 return false;
}
constexpr bool sameContext(const Snapshot& b,const Snapshot& a) noexcept {
 return b.game==a.game&&b.species==a.species&&b.form==a.form;
}
constexpr bool sourceMethod(const Step& s) noexcept {
 const auto& b=s.before;const auto& pr=s.proof;
 if(!pr.originalSourceRecord||!pr.exactSourceAuthenticated||
    pr.exactGame!=b.game||pr.exactSpecies!=b.species||pr.exactForm!=b.form||
    !pr.speciesCompatibleAuthenticated)return false;
 if(s.method==Method::TM||s.method==Method::HM){
   if(!pr.cartridgeOrServiceAuthenticated||!pr.machineNumber)return false;
   // Wrong TM number + move with complete exact-source proof contradicts
   // only this claimed route, handled after all source preflight.
   if(P::machine(b.game,s.method==Method::HM,pr.machineNumber,s.move)==P::Status::Unknown)
       return false;
 }
 if(s.method==Method::Tutor||s.method==Method::Relearner||
    s.method==Method::Encounter)return pr.cartridgeOrServiceAuthenticated;
 return s.method==Method::TM||s.method==Method::HM||s.method==Method::LevelUp;
}
// Preflight EVERY step, not only until the first contradicted action:
// a later unattested branch must preserve Unknown.
constexpr bool preflight(const Path& p) noexcept {
 if(!valid(p.initial)||!valid(p.observed)||!p.stepCount||
    p.stepCount>p.steps.size()||!p.completeIntermediateSequenceAuthenticated)return false;
 if(p.initial.candidateId!=p.observed.candidateId||
    p.initial.individualId!=p.observed.individualId)return false;
 Snapshot previous=p.initial;
 for(unsigned i=0;i<p.stepCount;++i){
  const auto& s=p.steps[i];const auto& b=s.before;const auto& a=s.after;const auto& q=s.proof;
  if(!valid(b)||!valid(a)||!equal(b,previous)||
     b.snapshotId==a.snapshotId||b.candidateId!=a.candidateId||
     b.individualId!=a.individualId||a.level<b.level||
     !q.actionReceipt||!q.originalActionAuthenticated||
     !q.sameIndividualAuthenticated||!q.allIntermediateLevelDecisionsAuthenticated)return false;
  // A receipt or distinct immutable snapshot can never be reused to forge a path.
  for(unsigned j=0;j<i;++j){
   if(q.actionReceipt==p.steps[j].proof.actionReceipt||
      a.snapshotId==p.steps[j].after.snapshotId)return false;
  }
  const auto genB=P::generation(b.game),genA=P::generation(a.game);
  switch(s.action){
   case Action::Teach:case Action::OfferDecline:case Action::OfferAlreadyKnown:
    if(!sameContext(b,a)||genB<5||!s.move||s.move>P::maxMove(b.game)||
       !sourceMethod(s)||!q.exactSourceAuthenticated)return false;
    if(s.action!=Action::Teach&&s.method!=Method::LevelUp)return false;
    break;
   case Action::DeleteMove:
    if(!sameContext(b,a)||s.method!=Method::None||s.move||!q.cartridgeOrServiceAuthenticated)return false;
    break;
   case Action::VisitGame:
    if(genB<5||genA!=genB||b.game==a.game||b.species!=a.species||
       b.form!=a.form||s.method!=Method::None||s.move||!q.cartridgeOrServiceAuthenticated)return false;
    break;
   case Action::Evolve:
    if(genB<5||b.game!=a.game||(b.species==a.species&&b.form==a.form)||
       s.method!=Method::None||s.move||!q.exactSourceAuthenticated||
       !q.originalSourceRecord)return false;
    break;
   case Action::PokeTransfer4to5:
   case Action::BankTransfer5to6:
    if((s.action==Action::PokeTransfer4to5&&(genB!=4||genA!=5))||
       (s.action==Action::BankTransfer5to6&&(genB!=5||genA!=6))||
       b.species!=a.species||b.form!=a.form||
       s.method!=Method::None||s.move||!q.transferRouteAuthenticated||
       !q.destinationMarkerAuthenticated||!q.fourSlotsPreservedAuthenticated||
       !q.cartridgeOrServiceAuthenticated)return false;
    break;
  }
  previous=a;
 }
 return equal(previous,p.observed);
}
constexpr Result evaluate(const Path& p) noexcept {
 Result r{};
 if(!preflight(p))return r;
 auto current=p.initial;
 r.rebuilt=current.moves;
 for(unsigned i=0;i<p.stepCount;++i){
  const auto& s=p.steps[i];const auto& q=s.proof;
  auto expected=current.moves;
  switch(s.action){
  case Action::Teach:{
   if(s.method==Method::TM||s.method==Method::HM){
    if(P::machine(current.game,s.method==Method::HM,q.machineNumber,s.move)!=P::Status::Matches){
     r.status=Status::CandidateContradicted;return r;
    }
   }
   if(contains(expected,s.move)||s.slot>=4||
      (freeSlot(expected)<4&&s.slot!=freeSlot(expected))){
    r.status=Status::CandidateContradicted;return r;
   }
   expected[s.slot]=s.move;++r.teachings;break;
  }
  case Action::OfferDecline:
   if(contains(expected,s.move)||freeSlot(expected)!=4){
    r.status=Status::CandidateContradicted;return r;
   }
   ++r.offers;break;
  case Action::OfferAlreadyKnown:
   if(!contains(expected,s.move)){
    r.status=Status::CandidateContradicted;return r;
   }
   ++r.offers;break;
  case Action::DeleteMove:
   if(s.slot>=4||!expected[s.slot]||!expected[1]){
    r.status=Status::CandidateContradicted;return r;
   }
   for(unsigned j=s.slot;j<3;++j)expected[j]=expected[j+1];
   expected[3]=0;break;
  case Action::VisitGame:++r.visits;break;
  case Action::Evolve:++r.evolutions;break;
  case Action::PokeTransfer4to5:case Action::BankTransfer5to6:++r.transfers;break;
  }
  if(expected!=s.after.moves){r.status=Status::CandidateContradicted;return r;}
  current=s.after;r.rebuilt=expected;
 }
 r.status=r.rebuilt==p.observed.moves?Status::CandidateConsistent:Status::CandidateContradicted;
 return r;
}
// This is about a PROPOSED candidate path, never the legality of a Pokemon.
struct Alternatives {
 std::array<Path,4> paths{};std::uint8_t count=0;
 bool allCandidateHistoriesEnumerated=false;
};
constexpr Status evaluate(const Alternatives& a) noexcept {
 if(!a.count||a.count>a.paths.size())return Status::Unknown;
 std::uint64_t id=a.paths[0].initial.candidateId;
 bool unknown=false;
 for(unsigned i=0;i<a.count;++i){
  if(!id||a.paths[i].initial.candidateId!=id)return Status::Unknown;
  const auto result=evaluate(a.paths[i]).status;
  if(result==Status::CandidateConsistent)return result;
  if(result==Status::Unknown)unknown=true;
 }
 return !unknown&&a.allCandidateHistoriesEnumerated?Status::CandidateContradicted:Status::Unknown;
}
static_assert(P::generation(Game::Black)==5&&P::generation(Game::OmegaRuby)==6);
} // namespace Legality::Gen56PkhexChronologyCore
