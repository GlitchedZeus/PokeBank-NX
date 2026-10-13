#pragma once
// L3 all 151 Gen I candidates: exhaustive *attested path* four-slot replay.
// Original source: Gen1RbyRetailPackedCatalog.h (pokered and pokeyellow pins).
// Source catalog != a Pokémon's trainer decisions, trade receipts, item
// consumption, original encounter, complete alternate routes, or legality.
#include "Legality/Gen1RbyRetailPackedCatalog.h"
#include "Legality/Gen1RbyOriginalJointHistory.h" // bounded original TM/HM methods
#include "Legality/Gen12TimeCapsuleEvidence.h" // original-boundary restrictions
#include <array>
#include <cstdint>
namespace Legality::Gen1RbyAllSpeciesJointHistory {
namespace R=Gen1RbyRetailPackedCatalog;
namespace B=Gen1RbyOriginalJointHistory;
using Game=R::Game;
enum class Action : std::uint8_t {
    Learn, Decline, AlreadyKnown, TeachMachine,
    EvolveLevel, EvolveStone, Trade, TradeEvolve,
    CapsuleToGen2,
};
enum class Outcome : std::uint8_t {Unknown, CandidateConsistent, CandidateContradicted};
struct Step {
    Action action=Action::Learn;
    Game game=Game::Unknown; // stage game AFTER trade, else original game
    std::uint8_t level=0;
    std::uint16_t move=0;
    std::uint8_t slot=0;
    B::Method machineMethod=B::Method::None;
    std::uint8_t machineNumber=0;
    std::uint64_t receipt=0;
    std::uint16_t evolutionTarget=0;
    std::uint8_t evolutionCondition=0;
    bool independentlyAttested=false;
    bool sourceMethodAttested=false;
    bool yellowPartnerNotSubjectToStone=false; // true only with proof this is NOT the Yellow starter partner
};
struct Path {
    std::uint64_t candidate=0,individual=0;
    Game source=Game::Unknown,observed=Game::Unknown;
    std::uint16_t firstSpecies=0,lastSpecies=0;
    std::uint8_t firstLevel=0,lastLevel=0;
    std::array<std::uint16_t,4> firstMoves{},lastMoves{};
    bool originalPhysicalSnapshotAttested=false,observedPhysicalSnapshotAttested=false;
    bool sameIndividualAttested=false,allTrainerDecisionsAttested=false;
    bool allInterveningMethodsCovered=false;
    bool originalCapsuleServiceAttested=false;
    bool lastObservedInGen2=false; // terminal boundary only, no Gen II level-up inference
    std::array<Step,64> steps{};
    std::uint8_t count=0;
};
struct Result {
    Outcome outcome=Outcome::Unknown;
    std::array<std::uint16_t,4> finalMoves{};
    std::uint16_t species=0;
    std::uint8_t offers=0, teaches=0, evolutions=0, trades=0;
    bool terminalCapsule=false;
    bool globalInvalid=false;
};
constexpr bool validMoves(const std::array<std::uint16_t,4>& a) noexcept {
    if(!a[0])return false;
    bool gap=false;
    for(std::uint8_t i=0;i<4;++i){
        const auto move=a[i];if(!move){gap=true;continue;}
        if(gap||move>165)return false;
        for(std::uint8_t j=0;j<i;++j)if(a[j]==move)return false;
    }
    return true;
}
constexpr bool contains(const std::array<std::uint16_t,4>& a,std::uint16_t v) noexcept {
    for(const auto x:a)if(x==v)return true;
    return false;
}
constexpr std::uint8_t firstFree(const std::array<std::uint16_t,4>& a) noexcept {
    for(std::uint8_t i=0;i<4;++i)if(!a[i])return i;
    return 4;
}
constexpr std::uint16_t firstOfferAfter(Game g,std::uint16_t species,std::uint8_t level) noexcept {
    std::uint16_t i=0;
    while(i<R::offerCount(g,species) && R::offerAt(g,species,i).level<=level)++i;
    return i;
}
constexpr B::Game mapped(Game g) noexcept {
    switch(g){case Game::Red:return B::Game::Red;case Game::Blue:return B::Game::Blue;
              case Game::Yellow:return B::Game::Yellow;default:return B::Game::Unknown;}
}
constexpr R::Evolution findEvolution(std::uint16_t species,std::uint16_t target,
                                      std::uint8_t kind) noexcept {
    for(std::uint16_t i=0;i<R::evolutionCount(species);++i){
        const auto ev=R::evolutionAt(species,i);
        if(ev.target==target && ev.kind==kind)return ev;
    }
    return {};
}
constexpr bool tradeEvolutionPossible(std::uint16_t species) noexcept {
    for(std::uint16_t i=0;i<R::evolutionCount(species);++i)
        if(R::evolutionAt(species,i).kind==3)return true;
    return false;
}
constexpr Result evaluate(const Path& p) noexcept {
    Result r{};
    if(!p.candidate||!p.individual||!R::known(p.source)||!R::known(p.observed)||
       !R::validSpecies(p.firstSpecies)||!R::validSpecies(p.lastSpecies)||
       !p.firstLevel||p.lastLevel<p.firstLevel||p.lastLevel>100||p.count>p.steps.size()||
       !p.originalPhysicalSnapshotAttested||!p.observedPhysicalSnapshotAttested||
       !p.sameIndividualAttested||!p.allTrainerDecisionsAttested||
       !p.allInterveningMethodsCovered||!validMoves(p.firstMoves)||!validMoves(p.lastMoves))return r;
    // Preflight every proof. A plausible local contradiction cannot override
    // a later missing witness, unavailable machine compatibility, or unknown
    // original retail method. This is candidate-only, never global Invalid.
    for(std::uint8_t i=0;i<p.count;++i){
        const auto s=p.steps[i];
        if(!s.independentlyAttested||!s.sourceMethodAttested||!R::known(s.game)||
           s.level<p.firstLevel||s.level>p.lastLevel)return r;
        if(s.action==Action::TeachMachine) {
            if(!s.receipt||!s.machineNumber||s.machineMethod==B::Method::None||!s.move)return r;
        } else if(s.machineMethod!=B::Method::None||s.machineNumber)return r;
        if((s.action==Action::EvolveStone||s.action==Action::TradeEvolve) &&
           !s.evolutionTarget)return r;
        if((s.action==Action::Trade||s.action==Action::TradeEvolve||
            s.action==Action::CapsuleToGen2)&&!s.receipt)return r;
        if(s.action==Action::CapsuleToGen2 &&
           (!p.originalCapsuleServiceAttested||i+1!=p.count||!p.lastObservedInGen2))return r;
    }
    // Source-method preflight over the proposed species/cartridge sequence.
    // An unsupported later machine must beat an earlier hypothesized bad
    // move choice: neither is evidence for global Invalid.
    auto previewSpecies=p.firstSpecies;
    auto previewGame=p.source;
    for(std::uint8_t i=0;i<p.count;++i){
        const auto s=p.steps[i];
        if(s.action==Action::TeachMachine){
            if(!B::sourceMachine(mapped(previewGame),previewSpecies,
                                 s.machineMethod,s.machineNumber))return r;
        } else if(s.action==Action::EvolveLevel||s.action==Action::EvolveStone||
                  s.action==Action::TradeEvolve){
            const auto method=s.action==Action::EvolveLevel?1:
                              s.action==Action::EvolveStone?2:3;
            if(!findEvolution(previewSpecies,s.evolutionTarget,method).target)return r;
            previewSpecies=s.evolutionTarget;
            previewGame=s.game;
        } else if(s.action==Action::Trade)previewGame=s.game;
    }
    auto game=p.source;auto species=p.firstSpecies;
    auto level=p.firstLevel;auto cursor=firstOfferAfter(game,species,level);
    r.finalMoves=p.firstMoves;r.species=species;
    std::array<std::uint64_t,64> usedReceipts{};std::uint8_t receiptCount=0;
    bool capsule=false;
    for(std::uint8_t i=0;i<p.count;++i){
        const auto s=p.steps[i];
        if(s.level<level)return r;
        const auto expected=R::offerAt(game,species,cursor);
        const bool isOffer=s.action==Action::Learn||s.action==Action::Decline||s.action==Action::AlreadyKnown;
        if(isOffer){
            if(s.game!=game||!expected.level||expected.level!=s.level||expected.move!=s.move||
               s.evolutionTarget||s.receipt){r.outcome=Outcome::CandidateContradicted;return r;}
            const bool known=contains(r.finalMoves,s.move);
            const auto free=firstFree(r.finalMoves);
            if(s.action==Action::AlreadyKnown){if(!known){r.outcome=Outcome::CandidateContradicted;return r;}}
            else if(s.action==Action::Decline){if(known||free<4){r.outcome=Outcome::CandidateContradicted;return r;}}
            else if(known||s.slot>=4||(free<4&&s.slot!=free)){
                r.outcome=Outcome::CandidateContradicted;return r;
            } else r.finalMoves[s.slot]=s.move;
            ++cursor;++r.offers;
        } else {
            if(expected.level&&expected.level<=s.level){r.outcome=Outcome::CandidateContradicted;return r;}
            if(s.action==Action::TeachMachine) {
                if(s.game!=game||s.evolutionTarget||s.slot>=4)return r;
                // A positive machine move also requires per-species original
                // compatibility; do NOT treat a species-wide move union as proof.
                const auto move=B::sourceMachine(mapped(game),species,s.machineMethod,s.machineNumber);
                if(!move)return r;
                if(move!=s.move){r.outcome=Outcome::CandidateContradicted;return r;}
                for(std::uint8_t j=0;j<receiptCount;++j)if(usedReceipts[j]==s.receipt){
                    r.outcome=Outcome::CandidateContradicted;return r;
                }
                usedReceipts[receiptCount++]=s.receipt;
                const auto free=firstFree(r.finalMoves);
                if(contains(r.finalMoves,s.move)||(free<4&&s.slot!=free)){
                    r.outcome=Outcome::CandidateContradicted;return r;
                }
                r.finalMoves[s.slot]=s.move;++r.teaches;
            } else if(s.action==Action::EvolveLevel||s.action==Action::EvolveStone||
                      s.action==Action::TradeEvolve) {
                if(s.move||s.slot||s.game==Game::Unknown)return r;
                const std::uint8_t method=s.action==Action::EvolveLevel?1:
                                          s.action==Action::EvolveStone?2:3;
                const auto ev=findEvolution(species,s.evolutionTarget,method);
                if(!ev.target)return r; // not a documented original evolution
                if(method==1){
                    if(s.game!=game||s.level<ev.condition||s.receipt)return r;
                } else if(method==2){
                    if(s.game!=game||s.evolutionCondition!=ev.condition||!s.receipt||
                       (game==Game::Yellow&&species==25&&!s.yellowPartnerNotSubjectToStone))return r;
                    // Caller must attest that Yellow Pikachu was not its
                    // non-evolving starter partner. A PK1 alone cannot know.
                } else if(s.game==game)return r; // trade-evolution must cross cartridges
                if(method==2||method==3){
                    for(std::uint8_t j=0;j<receiptCount;++j)if(usedReceipts[j]==s.receipt){
                        r.outcome=Outcome::CandidateContradicted;return r;
                    }
                    usedReceipts[receiptCount++]=s.receipt;
                }
                species=s.evolutionTarget;game=s.game;
                cursor=firstOfferAfter(game,species,s.level);++r.evolutions;
                if(method==3)++r.trades;
            } else if(s.action==Action::Trade) {
                if(s.game==game||s.move||s.slot||!s.receipt||s.evolutionTarget)return r;
                if(tradeEvolutionPossible(species))
                    return r; // no authenticated Gen I non-evolution exception yet
                for(std::uint8_t j=0;j<receiptCount;++j)if(usedReceipts[j]==s.receipt){
                    r.outcome=Outcome::CandidateContradicted;return r;
                }
                usedReceipts[receiptCount++]=s.receipt;
                game=s.game;cursor=firstOfferAfter(game,species,s.level);++r.trades;
            } else if(s.action==Action::CapsuleToGen2) {
                if(s.game!=game||s.move||s.slot||!s.receipt||s.evolutionTarget||
                   !p.originalCapsuleServiceAttested||!p.lastObservedInGen2||
                   !Gen12TimeCapsule::canCurrentlyTradeToGen1(species,false,r.finalMoves)||
                   !validMoves(r.finalMoves))return r;
                for(std::uint8_t j=0;j<receiptCount;++j)if(usedReceipts[j]==s.receipt){
                    r.outcome=Outcome::CandidateContradicted;return r;
                }
                usedReceipts[receiptCount++]=s.receipt;
                capsule=true;
            } else return r;
        }
        level=s.level;
    }
    if(p.lastObservedInGen2!=capsule || game!=p.observed||species!=p.lastSpecies)return r;
    const auto next=R::offerAt(game,species,cursor);
    if(next.level&&next.level<=p.lastLevel){r.outcome=Outcome::CandidateContradicted;return r;}
    r.terminalCapsule=capsule;r.species=species;
    r.outcome=r.finalMoves==p.lastMoves?Outcome::CandidateConsistent:Outcome::CandidateContradicted;
    return r;
}
struct Alternatives {std::array<Path,8> candidates{};std::uint8_t count=0;bool allOriginalRoutesAuthenticated=false;};
constexpr Outcome evaluate(const Alternatives& a) noexcept {
    if(!a.count||a.count>a.candidates.size()||!a.candidates[0].candidate||!a.candidates[0].individual)return Outcome::Unknown;
    bool unknown=false;
    for(std::uint8_t i=0;i<a.count;++i){
        if(a.candidates[i].candidate!=a.candidates[0].candidate||
           a.candidates[i].individual!=a.candidates[0].individual)return Outcome::Unknown;
        const auto x=evaluate(a.candidates[i]).outcome;
        if(x==Outcome::CandidateConsistent)return x;
        if(x==Outcome::Unknown)unknown=true;
    }
    return unknown||!a.allOriginalRoutesAuthenticated?Outcome::Unknown:Outcome::CandidateContradicted;
}
static_assert(R::offerCount(Game::Blue,25)==5&&R::offerCount(Game::Yellow,25)==9);
} // namespace Legality::Gen1RbyAllSpeciesJointHistory
