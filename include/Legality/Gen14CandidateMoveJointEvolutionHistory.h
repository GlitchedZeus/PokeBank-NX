#pragma once
// L3 Stage 3 — source-bound, same-individual Gen III four-slot timeline.
// Original retail mechanics: pret/pokeruby@63a8cbf, pokeemerald@5eff786,
// pokefirered@c75f352, src/pokemon*.c evolution/level-up move teaching,
// src/data/pokemon/level_up_learnsets.h and tmhm_learnsets.h.
// The caller supplies exact original source tables via Source. A hypothetical
// contradicted route is NEVER a global Invalid result; missing evidence is
// Unresolved even when a different proposed route fails.
#include <array>
#include <cstdint>

namespace Legality::Gen14CandidateMoveJointEvolutionHistory {
enum class Game : uint8_t { Unknown, Ruby, Sapphire, Emerald, FireRed, LeafGreen };
enum class Action : uint8_t {
    OfferedLearn, OfferedDecline, OfferedKnown, TeachMachine,
    Evolve, VisitGame, DeleteMove, UnsupportedRoute
};
enum class Machine : uint8_t { TM, HM };
enum class Status : uint8_t { Unresolved, CandidateConsistent, CandidateContradicted };
struct Offer { uint8_t level=0; uint16_t move=0; };
struct Step {
    Game game=Game::Unknown;
    uint8_t level=0;
    Action action=Action::UnsupportedRoute;
    uint16_t move=0;        // Offered, taught or deleted native move ID.
    uint8_t slot=0;         // Index in the SAME individual's four slots.
    uint16_t targetSpecies=0; // Evolve only.
    Machine machine=Machine::TM;
    uint8_t machineNumber=0;
    Game receiptGame=Game::Unknown;
    uint16_t receiptSpecies=0;
    bool originalActionVerified=false;
    bool methodReceiptVerified=false; // Exact item / evolution source.
    uint32_t receiptId=0; // Verified physical TM receipt; TMs are consumed.
};
struct Path {
    uint32_t candidateId=0;
    Game origin=Game::Unknown, observedGame=Game::Unknown;
    uint16_t originalSpecies=0, observedSpecies=0;
    uint8_t initialLevel=0, observedLevel=0;
    std::array<uint16_t,4> initialMoves{}, observedMoves{};
    bool originalSnapshotVerified=false;
    bool samePokemonVerified=false;
    bool everyLevelUpDecisionVerified=false;
    bool wholeClaimedHistoryVerified=false;
    std::array<Step,64> steps{};
    uint8_t count=0;
};
struct Result {
    Status status=Status::Unresolved;
    std::array<uint16_t,4> rebuilt{};
    uint8_t offers=0, machines=0, evolutions=0, visits=0, deletions=0;
    uint16_t finalSpecies=0;
    Game finalGame=Game::Unknown;
};
constexpr bool isGen3(Game g) noexcept { return g>=Game::Ruby && g<=Game::LeafGreen; }
constexpr bool validMoves(const std::array<uint16_t,4>& moves) noexcept {
    bool empty=false;
    for(uint8_t i=0;i<4;++i) {
        if(!moves[i]) {empty=true;continue;}
        if(empty || moves[i]>354)return false;
        for(uint8_t j=0;j<i;++j)if(moves[j]==moves[i])return false;
    }
    return true;
}
constexpr uint8_t freeSlot(const std::array<uint16_t,4>& moves) noexcept {
    for(uint8_t i=0;i<4;++i)if(!moves[i])return i;
    return 4;
}
constexpr bool hasMove(const std::array<uint16_t,4>& moves,uint16_t move) noexcept {
    for(auto m:moves)if(m==move)return true;
    return false;
}
// Independently attested original level evolutions; all other lines and
// stone/trade/happiness evolution mechanisms MUST be separately modeled.
constexpr uint8_t minimumLevel(uint16_t from,uint16_t to) noexcept {
    if(from==1 && to==2)return 16;
    if(from==2 && to==3)return 32;
    if(from==4 && to==5)return 16;
    if(from==5 && to==6)return 36;
    if(from==7 && to==8)return 16;
    if(from==8 && to==9)return 36;
    return 0;
}
// Source: static bool supported(Game, species), uint16_t offerCount(Game,
// species), Offer offerAt(Game,species,index), bool supportsMachine(Game,
// species,Machine,number,move). Use *original retail* per-version tables.
// The provider must never return supported=true for partial level-up data.
template<class Source> constexpr Result evaluate(const Path& p) noexcept {
    Result out{};
    if(!p.candidateId || !p.originalSnapshotVerified || !p.samePokemonVerified ||
       !p.everyLevelUpDecisionVerified || !p.wholeClaimedHistoryVerified ||
       !isGen3(p.origin) || !isGen3(p.observedGame) ||
       !p.originalSpecies || p.originalSpecies>386 ||
       !p.observedSpecies || p.observedSpecies>386 ||
       !p.initialLevel || p.observedLevel<p.initialLevel || p.observedLevel>100 ||
       p.count>p.steps.size() || !validMoves(p.initialMoves) ||
       !validMoves(p.observedMoves) || !Source::supported(p.origin,p.originalSpecies))
        return out;
    // Preflight missing/unsupported receipts across the WHOLE route before
    // finding local contradictions. One unsupported later service must not
    // promote a guessed earlier decision into a global negative.
    for(uint8_t i=0;i<p.count;++i) {
        const Step& s=p.steps[i];
        if(!s.originalActionVerified || !isGen3(s.game) ||
           s.level<p.initialLevel || s.level>p.observedLevel ||
           s.action==Action::UnsupportedRoute)return out;
        if((s.action==Action::Evolve || s.action==Action::TeachMachine) &&
           !s.methodReceiptVerified)return out;
        if(s.action==Action::TeachMachine &&
           (!isGen3(s.receiptGame) || !s.receiptSpecies ||
            !s.machineNumber || !s.move || s.move>354 ||
            (s.machine!=Machine::TM && s.machine!=Machine::HM) ||
            (s.machine==Machine::TM && !s.receiptId)))return out;
        if(s.action==Action::DeleteMove && !s.methodReceiptVerified)return out;
        if(s.action==Action::Evolve && !s.targetSpecies)return out;
    }
    Game game=p.origin;
    uint16_t species=p.originalSpecies;
    uint8_t level=p.initialLevel;
    out.rebuilt=p.initialMoves;
    // The original source list is ordered, including simultaneous offers.
    auto nextAfter=[](Game g,uint16_t sp,uint8_t level,bool inclusive) constexpr {
        uint16_t i=0;
        while(i<Source::offerCount(g,sp)) {
            const auto offer=Source::offerAt(g,sp,i);
            if(offer.level>level || (inclusive && offer.level==level))break;
            ++i;
        }
        return i;
    };
    uint16_t next=nextAfter(game,species,level,false);
    std::array<uint32_t,64> usedTMReceipts{};
    uint8_t receiptCount=0;
    for(uint8_t i=0;i<p.count;++i) {
        const Step& s=p.steps[i];
        if(s.level<level)return out; // A decreasing level is not attested.
        if(!Source::supported(game,species))return out;
        const auto expected=next<Source::offerCount(game,species)
            ?Source::offerAt(game,species,next):Offer{};
        const bool isOffer=s.action==Action::OfferedLearn ||
                           s.action==Action::OfferedDecline ||
                           s.action==Action::OfferedKnown;
        // Before ANY other action at a proposed level, all already-due
        // original species offers must be accounted for, in source order.
        if(isOffer) {
            if(s.game!=game || !expected.move ||
               s.level!=expected.level || s.move!=expected.move ||
               s.machineNumber || s.targetSpecies) {
                out.status=Status::CandidateContradicted;return out;
            }
            const uint8_t slot=freeSlot(out.rebuilt);
            const bool known=hasMove(out.rebuilt,s.move);
            if(s.action==Action::OfferedKnown) {
                if(!known){out.status=Status::CandidateContradicted;return out;}
            } else if(s.action==Action::OfferedDecline) {
                if(known || slot!=4){out.status=Status::CandidateContradicted;return out;}
            } else {
                if(known || s.slot>=4 || (slot<4 && s.slot!=slot)) {
                    out.status=Status::CandidateContradicted;return out;
                }
                out.rebuilt[s.slot]=s.move;
            }
            ++next;++out.offers;
        } else {
            if(expected.move && expected.level<=s.level) {
                out.status=Status::CandidateContradicted;return out;
            }
            if(s.action==Action::TeachMachine) {
                if(s.game!=game || s.receiptGame!=game ||
                   s.receiptSpecies!=species)return out;
                // An item usable in this species must match its *numbered*
                // original TM/HM. Verified incompatible receipt contradicts
                // this claimed method only, not alternative tutors/events.
                if(!Source::supportsMachine(game,species,s.machine,
                                            s.machineNumber,s.move)) {
                    out.status=Status::CandidateContradicted;return out;
                }
                const auto slot=freeSlot(out.rebuilt);
                if(!s.move || s.slot>=4 || hasMove(out.rebuilt,s.move) ||
                   (slot<4 && s.slot!=slot)) {
                    out.status=Status::CandidateContradicted;return out;
                }
                if(s.machine==Machine::TM) {
                    for(uint8_t j=0;j<receiptCount;++j)
                        if(usedTMReceipts[j]==s.receiptId) {
                            out.status=Status::CandidateContradicted;return out;
                        }
                    usedTMReceipts[receiptCount++]=s.receiptId;
                }
                out.rebuilt[s.slot]=s.move;++out.machines;
            } else if(s.action==Action::Evolve) {
                if(s.game!=game || s.move || s.machineNumber || s.slot ||
                   !minimumLevel(species,s.targetSpecies))return out;
                if(s.level<minimumLevel(species,s.targetSpecies))return out;
                if(!Source::supported(game,s.targetSpecies))return out;
                species=s.targetSpecies;++out.evolutions;
                // The evolved form is changed BEFORE it offers any move at
                // the evolution level. It does not retroactively gain moves
                // offered at earlier levels on the new species learnset.
                next=nextAfter(game,species,s.level,true);
            } else if(s.action==Action::VisitGame) {
                if(s.game==game || s.move || s.machineNumber ||
                   s.targetSpecies || s.slot ||
                   !Source::supported(s.game,species))return out;
                game=s.game;++out.visits;
                // Merely changing cartridge is not a level-up event.
                next=nextAfter(game,species,s.level,false);
            } else if(s.action==Action::DeleteMove) {
                if(s.game!=game || !s.move || s.slot>=4 ||
                   out.rebuilt[s.slot]!=s.move || !out.rebuilt[1]) {
                    out.status=Status::CandidateContradicted;return out;
                }
                for(uint8_t j=s.slot;j<3;++j)out.rebuilt[j]=out.rebuilt[j+1];
                out.rebuilt[3]=0;++out.deletions;
            } else return out;
        }
        level=s.level;
    }
    if(!Source::supported(game,species))return out;
    const auto remaining=next<Source::offerCount(game,species)
        ?Source::offerAt(game,species,next):Offer{};
    if(remaining.move && remaining.level<=p.observedLevel) {
        out.status=Status::CandidateContradicted;return out;
    }
    out.finalSpecies=species;out.finalGame=game;
    out.status=(game==p.observedGame && species==p.observedSpecies &&
                out.rebuilt==p.observedMoves)
        ?Status::CandidateConsistent:Status::CandidateContradicted;
    return out;
}
struct Alternatives {
    std::array<Path,8> paths{};
    uint8_t count=0;
    // Never mark this true based on merely scanning learnsets or 4 slots.
    bool allGenuineHistoriesEnumerated=false;
};
template<class Source> constexpr Status evaluate(const Alternatives& alternatives) noexcept {
    if(!alternatives.count || alternatives.count>alternatives.paths.size() ||
       !alternatives.paths[0].candidateId)return Status::Unresolved;
    bool unknown=false;
    for(uint8_t i=0;i<alternatives.count;++i) {
        if(alternatives.paths[i].candidateId!=alternatives.paths[0].candidateId)
            return Status::Unresolved;
        const auto result=evaluate<Source>(alternatives.paths[i]).status;
        if(result==Status::CandidateConsistent)return result;
        if(result==Status::Unresolved)unknown=true;
    }
    return unknown || !alternatives.allGenuineHistoriesEnumerated
        ?Status::Unresolved:Status::CandidateContradicted;
}
static_assert(minimumLevel(5,6)==36 && minimumLevel(8,9)==36);
static_assert(!minimumLevel(92,94)); // unsupported evolution remains Unknown
} // namespace Legality::Gen14CandidateMoveJointEvolutionHistory
