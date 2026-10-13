#pragma once
// L3 RBY read-only candidate chronology. Original source: pret/pokered
// data/pokemon/evos_moves.asm blob 6a6704f98b8b075f2fd81c91d41a1dda0f2a047d;
// pret/pokeyellow corresponding blob 8bee626ae17d052a17562be9959a13b6102146f7.
// Original Pikachu/Raichu base stats: blob 4a7f5e8067a31d5aceb91e602a64baf9bd309e8c.
// Source facts are not evidence of a player's history. Hypothetical contradiction
// is never global illegality. TM/HM source identity requires external receipts.
#include "Legality/Gen1OriginalMachineTables.h"
#include <array>
#include <cstdint>
namespace Legality::Gen1RbyOriginalJointHistory {
enum class Game : std::uint8_t {Unknown,Red,Blue,Yellow};
enum class Action : std::uint8_t {OfferLearn,OfferDecline,OfferKnown,Machine,Evolve,Trade};
enum class Method : std::uint8_t {None,TM,HM};
enum class Status : std::uint8_t {Unknown,Consistent,Contradicted};
struct Offer {std::uint8_t level=0;std::uint16_t move=0;};
inline constexpr std::array<Offer,5> kPikachuRB{{{9,86},{16,98},{26,129},{33,97},{43,87}}};
inline constexpr std::array<Offer,9> kPikachuYellow{{{6,39},{8,86},{11,98},{15,104},{20,21},{26,85},{33,97},{41,87},{50,113}}};
inline constexpr std::array<Offer,6> kCharmander{{{9,52},{15,43},{22,99},{30,163},{38,53},{46,83}}};
inline constexpr std::array<Offer,7> kBulbasaur{{{7,73},{13,22},{20,77},{27,75},{34,74},{41,79},{48,76}}};
constexpr bool known(Game g){return g==Game::Red||g==Game::Blue||g==Game::Yellow;}
constexpr std::uint8_t count(Game g,std::uint16_t species){
 if(!known(g))return 0;
 switch(species){case 1:return 7;case 4:return 6;case 25:return g==Game::Yellow?9:5;case 26:return 0;default:return 0;}
}
constexpr bool catalogComplete(Game g,std::uint16_t s){return known(g)&&(s==1||s==4||s==25||s==26);}
constexpr Offer offer(Game g,std::uint16_t sp,std::uint8_t n){
 if(n>=count(g,sp))return {};
 if(sp==1)return kBulbasaur[n];
 if(sp==4)return kCharmander[n];
 if(sp==25)return g==Game::Yellow?kPikachuYellow[n]:kPikachuRB[n];
 return {};
}
constexpr bool movesValid(const std::array<std::uint16_t,4>& a){
 bool gap=false;for(unsigned i=0;i<4;++i){const auto m=a[i];if(!m){gap=true;continue;}if(gap||m>165)return false;for(unsigned j=0;j<i;++j)if(a[j]==m)return false;}return a[0]!=0;
}
constexpr std::uint8_t freeSlot(const std::array<std::uint16_t,4>& a){for(std::uint8_t i=0;i<4;++i)if(!a[i])return i;return 4;}
constexpr bool has(const std::array<std::uint16_t,4>& a,std::uint16_t m){for(auto v:a)if(v==m)return true;return false;}
// These exact move-to-item identities and species compatibility must both be
// source attested. Full machine coverage for all 151 species is NOT asserted.
// Compatibility is source-attested for all 151 species, not an individual
// trainer's machine receipt. Missing historical custody still yields Unknown.
constexpr std::uint16_t sourceMachine(Game g,std::uint16_t species,Method method,std::uint8_t number){
 if(!known(g)||method==Method::None)return 0;
 return Gen1OriginalMachineTables::moveFor(static_cast<std::uint8_t>(g),species,
         method==Method::HM,number);
}

struct Step {
 Game game=Game::Unknown;std::uint8_t level=0;Action action=Action::OfferLearn;
 std::uint16_t move=0;std::uint8_t slot=0;Method method=Method::None;
 std::uint8_t machine=0;std::uint32_t receipt=0;
 std::uint16_t evolvedSpecies=0;bool sourceVerified=false;bool actionVerified=false;
};
struct Path {
 std::uint32_t candidate=0,individual=0;Game origin=Game::Unknown,observedGame=Game::Unknown;
 std::uint16_t species=0,observedSpecies=0;std::uint8_t firstLevel=0,lastLevel=0;
 std::array<std::uint16_t,4> firstMoves{},lastMoves{};
 bool sameIndividualVerified=false,firstSnapshotVerified=false,lastSnapshotVerified=false,completeChoicesVerified=false;
 std::array<Step,32> steps{};std::uint8_t stepCount=0;
};
struct Result {Status status=Status::Unknown;std::array<std::uint16_t,4> moves{};std::uint8_t offers=0,machines=0,trades=0,evolutions=0;bool globalInvalid=false;};
constexpr std::uint8_t after(Game g,std::uint16_t species,std::uint8_t level){std::uint8_t n=0;while(n<count(g,species)&&offer(g,species,n).level<=level)++n;return n;}
constexpr Result evaluate(const Path& p){
 Result out{};
 if(!p.candidate||!p.individual||!known(p.origin)||!known(p.observedGame)||!catalogComplete(p.origin,p.species)||!catalogComplete(p.observedGame,p.observedSpecies)||!p.firstLevel||p.lastLevel<p.firstLevel||p.lastLevel>100||p.stepCount>p.steps.size()||!p.sameIndividualVerified||!p.firstSnapshotVerified||!p.lastSnapshotVerified||!p.completeChoicesVerified||!movesValid(p.firstMoves)||!movesValid(p.lastMoves))return out;
 // Preflight all historical evidence before claiming any path contradiction.
 for(std::uint8_t i=0;i<p.stepCount;++i){const auto& s=p.steps[i];if(!s.sourceVerified||!s.actionVerified||!known(s.game)||s.level<p.firstLevel||s.level>p.lastLevel)return out;
  if(s.action==Action::Machine){if(!s.move||!s.receipt||!s.machine||s.method==Method::None)return out;}
  else if(s.method!=Method::None||s.machine||s.receipt)return out;
  if(s.action==Action::Trade||s.action==Action::Evolve){if(s.move||s.slot)return out;}
  else if(s.action!=Action::Machine && (!s.move||s.evolvedSpecies))return out;
 }
 auto game=p.origin;auto species=p.species;auto level=p.firstLevel;auto cursor=after(game,species,level);out.moves=p.firstMoves;
 std::array<std::uint32_t,32> consumed{};std::uint8_t used=0;
 for(std::uint8_t i=0;i<p.stepCount;++i){const auto s=p.steps[i];if(s.level<level)return out;
  const auto expected=offer(game,species,cursor);
  const bool isOffer=s.action==Action::OfferLearn||s.action==Action::OfferDecline||s.action==Action::OfferKnown;
  if(isOffer){if(s.game!=game||expected.level!=s.level||expected.move!=s.move){out.status=Status::Contradicted;return out;}
   ++cursor;++out.offers;const bool knownMove=has(out.moves,s.move);const auto empty=freeSlot(out.moves);
   if(s.action==Action::OfferKnown){if(!knownMove){out.status=Status::Contradicted;return out;}}
   else if(s.action==Action::OfferDecline){if(knownMove||empty<4){out.status=Status::Contradicted;return out;}}
   else {if(knownMove||s.slot>=4||(empty<4&&s.slot!=empty)){out.status=Status::Contradicted;return out;}out.moves[s.slot]=s.move;}
  }else{
   if(expected.level&&expected.level<=s.level){out.status=Status::Contradicted;return out;}
   if(s.action==Action::Machine){if(s.game!=game||sourceMachine(game,species,s.method,s.machine)==0)return out;
    if(sourceMachine(game,species,s.method,s.machine)!=s.move){out.status=Status::Contradicted;return out;}
    for(std::uint8_t k=0;k<used;++k)if(consumed[k]==s.receipt){out.status=Status::Contradicted;return out;}
    consumed[used++]=s.receipt;
    const auto empty=freeSlot(out.moves);if(has(out.moves,s.move)||s.slot>=4||(empty<4&&s.slot!=empty)){out.status=Status::Contradicted;return out;}
    out.moves[s.slot]=s.move;++out.machines;
   }else if(s.action==Action::Trade){if(s.game==game)return out;game=s.game;cursor=after(game,species,s.level);++out.trades;
   }else if(s.action==Action::Evolve){if(s.game!=game||species!=25||s.evolvedSpecies!=26)return out;species=26;cursor=after(game,species,s.level);++out.evolutions;
   }else return out;
  }level=s.level;
 }
 if(game!=p.observedGame||species!=p.observedSpecies)return out;
 const auto expected=offer(game,species,cursor);if(expected.level&&expected.level<=p.lastLevel){out.status=Status::Contradicted;return out;}
 out.status=out.moves==p.lastMoves?Status::Consistent:Status::Contradicted;return out;
}
static_assert(count(Game::Red,25)==5 && count(Game::Yellow,25)==9);
static_assert(offer(Game::Red,25,0).move==86 && offer(Game::Yellow,25,0).move==39);
static_assert(sourceMachine(Game::Yellow,25,Method::HM,5)==148);
} // namespace Legality::Gen1RbyOriginalJointHistory
