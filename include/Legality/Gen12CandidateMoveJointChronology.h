#pragma once
// L3 Stage 1: a bounded historical Gen I/II four-slot *candidate* replay.
// Retail source pins: pret/pokered@af519899, pokeyellow@e89ead15,
// pokegold@ef0201d8, pokecrystal@3bc8daa4, data/pokemon/evos_*.asm,
// constants/item_constants.asm and base_stats/bulbasaur.asm.
// This is not a species-wide legality verdict: an unobserved egg, trade,
// evolution, event, reminder or alternate source remains Unknown.
#include <array>
#include <cstdint>

namespace Legality::Gen12CandidateMoveJointChronology {
enum class Game : uint8_t { Unknown, Red, Blue, Yellow, Gold, Silver, Crystal };
enum class Action : uint8_t {
    OfferLearn, OfferDecline, OfferAlreadyKnown, TeachTM,
    VisitGame, ToGen2, ToGen1, ReturnToGen2
};
enum class Status : uint8_t { Unknown, CandidateConsistent, CandidateContradicted };
struct Offer { uint8_t level; uint16_t move; };
constexpr uint8_t generation(Game g) noexcept {
    return g==Game::Red || g==Game::Blue || g==Game::Yellow ? 1 :
           g==Game::Gold || g==Game::Silver || g==Game::Crystal ? 2 : 0;
}
// Deliberately *complete* original level-up lists only for the specified
// source/species pairs. Every other species stays Unknown, not disproved.
constexpr std::array<Offer,7> kBulbasaurRby{{
    {7,73},{13,22},{20,77},{27,75},{34,74},{41,79},{48,76}}};
constexpr std::array<Offer,10> kBulbasaurGsc{{
    {4,45},{7,73},{10,22},{15,77},{15,79},{20,75},{25,230},
    {32,74},{39,235},{46,76}}};
constexpr std::array<Offer,6> kCharmanderRby{{
    {9,52},{15,43},{22,99},{30,163},{38,53},{46,83}}};
constexpr std::array<Offer,6> kCyndaquilGsc{{
    {6,108},{12,52},{19,98},{27,172},{36,129},{46,53}}};
constexpr bool supported(Game game, uint16_t species) noexcept {
    const auto gen=generation(game);
    return (species==1 && gen!=0) || (species==4 && gen==1) ||
           (species==155 && gen==2);
}
constexpr uint8_t offerCount(Game game,uint16_t species) noexcept {
    if(!supported(game,species))return 0;
    if(species==1)return generation(game)==1?7:10;
    return 6;
}
constexpr Offer offerAt(Game game,uint16_t species,uint8_t i) noexcept {
    if(i>=offerCount(game,species))return {};
    if(species==1)return generation(game)==1?kBulbasaurRby[i]:kBulbasaurGsc[i];
    if(species==4)return kCharmanderRby[i];
    return kCyndaquilGsc[i];
}
// Numerically identified *Bulbasaur* machine witnesses from retail item
// constants AND its base-stats TM/HM compatibility. Unlisted receipts are
// inconclusive. TM10 is Double-Edge (#38) in RBY but Hidden Power (#237)
// in GSC; a move pool or a bare TM number is never source identity.
constexpr uint16_t pinnedBulbasaurTM(Game game,uint8_t number) noexcept {
    if(generation(game)==1) {
        if(number==6)return 92; // Toxic
        if(number==8)return 34; // Body Slam
        if(number==10)return 38; // Double-Edge
    }
    if(generation(game)==2) {
        if(number==6)return 92; // Toxic
        if(number==10)return 237; // Hidden Power
    }
    return 0;
}
constexpr bool structural(const std::array<uint16_t,4>& a,uint16_t max) noexcept {
    bool empty=false;
    for(uint8_t i=0;i<4;++i) {
        if(!a[i]){empty=true;continue;}
        if(a[i]>max || empty)return false;
        for(uint8_t j=0;j<i;++j)if(a[j]==a[i])return false;
    }
    return true;
}
constexpr uint8_t freeSlot(const std::array<uint16_t,4>& a) noexcept {
    for(uint8_t i=0;i<4;++i)if(!a[i])return i;
    return 4;
}
constexpr bool contains(const std::array<uint16_t,4>& a,uint16_t move) noexcept {
    for(auto m:a)if(m==move)return true;
    return false;
}
struct Step {
    Game game=Game::Unknown; // Game inhabited AFTER the transition, if any
    uint8_t level=0;
    Action action=Action::OfferLearn;
    uint16_t move=0;
    uint8_t slot=0;
    uint8_t tmNumber=0;
    bool actionSourceVerified=false; // Exact-game original acquisition/visit
};
struct Path {
    uint32_t candidateId=0;
    uint16_t species=0;
    Game origin=Game::Unknown;
    Game observedGame=Game::Unknown; // exact observed cartridge, not inferred
    uint8_t initialLevel=0,observedLevel=0;
    std::array<uint16_t,4> initialMoves{}, observedMoves{};
    bool originalSnapshotVerified=false;
    bool samePokemonAndSpeciesVerified=false;
    bool fullVisibleHistoryVerified=false;
    bool retailTimeCapsulePolicyVerified=false; // no inference from PK2 alone
    uint16_t speciesAtGen1Entry=0; // independent original-stage witness
    std::array<Step,32> steps{};
    uint8_t stepCount=0;
};
struct Result {
    Status status=Status::Unknown;
    std::array<uint16_t,4> rebuilt{};
    uint8_t offers=0,machines=0,visits=0;
    bool visitedGen1=false,returnedToGen2=false;
};
constexpr uint8_t nextOfferAfter(Game game,uint16_t species,uint8_t level) noexcept {
    uint8_t i=0;
    while(i<offerCount(game,species) && offerAt(game,species,i).level<=level)++i;
    return i;
}
constexpr Result evaluate(const Path& p) noexcept {
    Result out{};
    if(!p.candidateId || !p.originalSnapshotVerified ||
       !p.samePokemonAndSpeciesVerified || !p.fullVisibleHistoryVerified ||
       !supported(p.origin,p.species) ||
       !supported(p.observedGame,p.species) || !p.initialLevel ||
       p.observedLevel<p.initialLevel || p.observedLevel>100 ||
       p.stepCount>p.steps.size() ||
       !structural(p.initialMoves,generation(p.origin)==1?165:251) ||
       !structural(p.observedMoves,251))return out;
    // Preflight evidence and method coverage before replay: an unsupported
    // later branch must not be obscured by an earlier guessed contradiction.
    for(uint8_t i=0;i<p.stepCount;++i) {
        const Step s=p.steps[i];
        if(!s.actionSourceVerified || !supported(s.game,p.species))return out;
        if(s.action==Action::TeachTM &&
           !(p.species==1 && pinnedBulbasaurTM(s.game,s.tmNumber)))return out;
        if(s.action==Action::ToGen2 &&
           (!p.retailTimeCapsulePolicyVerified || p.species>151))return out;
        if(s.action==Action::ToGen1 &&
           (!p.retailTimeCapsulePolicyVerified ||
            p.speciesAtGen1Entry!=p.species || p.species>151))return out;
    }
    Game current=p.origin,returnGame=Game::Unknown;
    uint8_t level=p.initialLevel, cursor=nextOfferAfter(current,p.species,level);
    out.rebuilt=p.initialMoves;
    for(uint8_t i=0;i<p.stepCount;++i) {
        const Step s=p.steps[i];
        if(!s.actionSourceVerified || !generation(s.game) ||
           s.level<level || s.level>p.observedLevel)return out;
        const auto expected=offerAt(current,p.species,cursor);
        const bool offer=s.action==Action::OfferLearn ||
                         s.action==Action::OfferDecline ||
                         s.action==Action::OfferAlreadyKnown;
        // Every original level-up offer, including two moves at the same
        // level, must have an ordered decision before any subsequent action.
        if(offer) {
            if(s.game!=current || !expected.level ||
               expected.level!=s.level || expected.move!=s.move ||
               s.tmNumber)return {Status::CandidateContradicted,out.rebuilt,
                                    out.offers,out.machines,out.visits,
                                    out.visitedGen1,out.returnedToGen2};
            ++cursor;++out.offers;
            const bool known=contains(out.rebuilt,s.move);
            const uint8_t free=freeSlot(out.rebuilt);
            if(s.action==Action::OfferAlreadyKnown) {
                if(!known) {out.status=Status::CandidateContradicted;return out;}
            } else if(s.action==Action::OfferDecline) {
                if(known || free<4) {out.status=Status::CandidateContradicted;return out;}
            } else {
                if(known || s.slot>=4 || (free<4 && s.slot!=free))
                    {out.status=Status::CandidateContradicted;return out;}
                out.rebuilt[s.slot]=s.move;
            }
            level=s.level;continue;
        }
        if(expected.level && expected.level<=s.level)
            {out.status=Status::CandidateContradicted;return out;}
        if(s.action==Action::TeachTM) {
            if(s.game!=current || !s.tmNumber || !s.move)return out;
            const uint16_t pinned=p.species==1?
                pinnedBulbasaurTM(current,s.tmNumber):0;
            if(!pinned)return out; // unmodeled original item/compatibility
            if(pinned!=s.move){out.status=Status::CandidateContradicted;return out;}
            const uint8_t free=freeSlot(out.rebuilt);
            if(contains(out.rebuilt,s.move) || s.slot>=4 ||
               (free<4 && s.slot!=free))
                {out.status=Status::CandidateContradicted;return out;}
            out.rebuilt[s.slot]=s.move;++out.machines;
        } else {
            if(s.move || s.tmNumber || s.slot || !supported(s.game,p.species))
                return out;
            if(s.action==Action::VisitGame) {
                if(s.game==current || generation(s.game)!=generation(current))return out;
            } else if(s.action==Action::ToGen2) {
                if(generation(current)!=1 || generation(s.game)!=2 ||
                   !p.retailTimeCapsulePolicyVerified || p.species>151)
                    return out;
            } else if(s.action==Action::ToGen1) {
                if(generation(current)!=2 || generation(s.game)!=1 ||
                   returnGame!=Game::Unknown)return out;
                if(!p.retailTimeCapsulePolicyVerified ||
                   p.speciesAtGen1Entry!=p.species || p.species>151)return out;
                if(!structural(out.rebuilt,165))
                    {out.status=Status::CandidateContradicted;return out;}
                returnGame=current;out.visitedGen1=true;
            } else if(s.action==Action::ReturnToGen2) {
                if(generation(current)!=1 || s.game!=returnGame ||
                   !out.visitedGen1 || out.returnedToGen2)return out;
                out.returnedToGen2=true;returnGame=Game::Unknown;
            } else return out;
            current=s.game;cursor=nextOfferAfter(current,p.species,s.level);
            ++out.visits;
        }
        level=s.level;
    }
    if(current!=p.observedGame)return out;
    const auto last=offerAt(current,p.species,cursor);
    if(last.level && last.level<=p.observedLevel)
        {out.status=Status::CandidateContradicted;return out;}
    if(!structural(out.rebuilt,generation(current)==1?165:251))return out;
    out.status=out.rebuilt==p.observedMoves?
        Status::CandidateConsistent:Status::CandidateContradicted;
    return out;
}
// Multiple independently proposed histories: a bad route cannot globally
// exclude traded, bred, evolved or event alternatives without proof of closure.
struct Alternatives {
    std::array<Path,8> paths{};
    uint8_t count=0;
    bool allGenuineRoutesEnumerated=false;
};
constexpr Status evaluate(const Alternatives& a) noexcept {
    if(!a.count || a.count>a.paths.size() || !a.paths[0].candidateId)
        return Status::Unknown;
    bool unknown=false;
    for(uint8_t i=0;i<a.count;++i) {
        if(a.paths[i].candidateId!=a.paths[0].candidateId)return Status::Unknown;
        const Status s=evaluate(a.paths[i]).status;
        if(s==Status::CandidateConsistent)return s;
        if(s==Status::Unknown)unknown=true;
    }
    return unknown || !a.allGenuineRoutesEnumerated?
        Status::Unknown:Status::CandidateContradicted;
}
static_assert(offerAt(Game::Red,1,1).move==22 &&
              offerAt(Game::Crystal,1,2).move==22);
static_assert(offerAt(Game::Crystal,1,3).level==15 &&
              offerAt(Game::Crystal,1,4).level==15);
static_assert(pinnedBulbasaurTM(Game::Red,10)==38 &&
              pinnedBulbasaurTM(Game::Gold,10)==237);
} // namespace Legality::Gen12CandidateMoveJointChronology
