#pragma once
// L3 M06/M07/M11/M12/M17: source-scoped, read-only four-slot move replay.
// Each witness describes ONE explicitly hypothesized history. A detached PK3
// or PK4 cannot certify the trainer's teaching choices or visited games.
// A contradictory route never proves the species globally Invalid.
// Retail source evidence: pret/pokeruby@63a8cbf src/pokemon_1.c,
// pret/pokeemerald@5eff786 src/pokemon.c,
// pret/pokefirered@c75f352 src/pokemon.c (move-slot mechanics).
#include "Legality/Gen34PalParkHMIds.h"
#include <array>
#include <cstdint>

namespace Legality::Gen14CandidateMoveChronology {

enum class Game : uint8_t {
    Unknown, Ruby, Sapphire, Emerald, FireRed, LeafGreen,
    Diamond, Pearl, Platinum, HeartGold, SoulSilver
};
constexpr uint8_t generation(Game game) noexcept {
    switch(game) {
    case Game::Ruby: case Game::Sapphire: case Game::Emerald:
    case Game::FireRed: case Game::LeafGreen: return 3;
    case Game::Diamond: case Game::Pearl: case Game::Platinum:
    case Game::HeartGold: case Game::SoulSilver: return 4;
    default: return 0;
    }
}
enum class Method : uint8_t {
    LevelUp, TM, HM, Tutor, Reminder
};
enum class Action : uint8_t {
    Learn, Decline, AlreadyKnown, DeleteMove, VisitGame, PalPark
};
enum class Status : uint8_t {
    Unresolved, CandidateConsistent, CandidateContradicted, Unsupported
};
struct Step {
    Game game=Game::Unknown;  // Exact retail game for this proposed action.
    uint8_t level=0;         // Level when it occurred (no invented level drop).
    Action action=Action::Learn;
    Method method=Method::LevelUp;
    uint16_t move=0;
    uint8_t slot=0;          // Newly filled/replaced/deleted move slot.
    bool evidenceVerified=false;  // Exact-game acquisition/visit attested.
    // On a Learn/Decline/AlreadyKnown action, the cited METHOD must have
    // been independently verified in exactly this game's version; broad
    // species compatibility, editor picker or a native move ID is not proof.
    Game methodSource=Game::Unknown;
};
struct Path {
    uint32_t candidateId=0;
    uint16_t species=0;
    Game origin=Game::Unknown;
    uint8_t initialLevel=0, observedLevel=0;
    std::array<uint16_t,4> initialMoves{}, observedMoves{};
    bool initialSnapshotVerified=false;
    bool samePokemonAndSpeciesVerified=false;
    bool fullChronologyVerified=false;
    std::array<Step,32> steps{};
    uint8_t stepCount=0;
};
struct Result {
    Status status=Status::Unresolved;
    std::array<uint16_t,4> rebuilt{};
    uint8_t applied=0, declined=0, deleted=0, visits=0;
    bool transferred=false;
};
constexpr bool structural(const std::array<uint16_t,4>& moves,
                          uint16_t maxMove) noexcept {
    bool hadEmpty=false;
    for(uint8_t i=0;i<4;++i) {
        if(!moves[i]) {hadEmpty=true;continue;}
        if(moves[i]>maxMove || hadEmpty)return false;
        for(uint8_t j=0;j<i;++j)
            if(moves[j]==moves[i])return false;
    }
    return true;
}
constexpr uint8_t freeSlot(const std::array<uint16_t,4>& moves) noexcept {
    for(uint8_t i=0;i<4;++i)if(!moves[i])return i;
    return 4;
}
constexpr bool contains(const std::array<uint16_t,4>& moves,
                        uint16_t move) noexcept {
    for(uint16_t value:moves)if(value==move)return true;
    return false;
}
constexpr bool teachingMethod(Method method) noexcept {
    switch(method) {
    case Method::LevelUp: case Method::TM: case Method::HM:
    case Method::Tutor: case Method::Reminder:return true;
    }
    return false;
}
// All actions refer to the same original species: ancestor/evolution-form
// histories require a separately attested model (M13), not a fake identity.
constexpr Result evaluate(const Path& path) noexcept {
    Result result{};
    const uint8_t startGen=generation(path.origin);
    if(!startGen || !path.species ||
       path.species>(startGen==3?386u:493u) ||
       !path.initialLevel || path.initialLevel>100 ||
       path.observedLevel<path.initialLevel || path.observedLevel>100)
        {result.status=Status::Unsupported;return result;}
    // A current PK4 may have Gen III origin; the input explicitly describes
    // its original game, not an inferred game from the current save file.
    if(!path.candidateId || !path.initialSnapshotVerified ||
       !path.samePokemonAndSpeciesVerified || !path.fullChronologyVerified ||
       path.stepCount>path.steps.size())return result;
    if(!structural(path.initialMoves,startGen==3?354:467) ||
       !structural(path.observedMoves,467))return result;
    result.rebuilt=path.initialMoves;
    Game current=path.origin;
    uint8_t lastLevel=path.initialLevel;
    for(uint8_t i=0;i<path.stepCount;++i) {
        const Step step=path.steps[i];
        const uint8_t gen=generation(current);
        if(!generation(step.game) || !step.level || step.level<lastLevel ||
           step.level>path.observedLevel || !step.evidenceVerified)
            return result; // Missing/contradictory attestation is Unknown.
        lastLevel=step.level;
        switch(step.action) {
        case Action::VisitGame:
            if(step.game==current || generation(step.game)!=gen ||
               step.move || step.methodSource!=Game::Unknown)return result;
            current=step.game;++result.visits;
            continue;
        case Action::PalPark:
            if(gen!=3 || generation(step.game)!=4 ||
               result.transferred || path.species>386 || step.move ||
               step.methodSource!=Game::Unknown)return result;
            // The verified departure cannot include an old Gen III HM.
            // A re-taught Gen IV HM *after* this edge is permitted.
            for(uint16_t move:result.rebuilt)
                if(Gen34PalParkHMIds::isGen3HM(move)) {
                    result.status=Status::CandidateContradicted;
                    return result;
                }
            current=step.game;result.transferred=true;++result.visits;
            continue;
        case Action::DeleteMove:
            if(step.game!=current || step.methodSource!=Game::Unknown ||
               step.move || step.slot>=4 || !result.rebuilt[step.slot]) {
                result.status=Status::CandidateContradicted;return result;
            }
            // Deleting the sole remaining move is not attested here.
            if(result.rebuilt[1]==0)return result;
            for(uint8_t j=step.slot;j<3;++j)
                result.rebuilt[j]=result.rebuilt[j+1];
            result.rebuilt[3]=0;++result.deleted;
            continue;
        case Action::Learn:
        case Action::Decline:
        case Action::AlreadyKnown:break;
        default:return result;
        }
        if(step.game!=current || step.methodSource!=current ||
           !teachingMethod(step.method) || !step.move ||
           step.move>(gen==3?354u:467u))return result;
        const bool known=contains(result.rebuilt,step.move);
        const uint8_t open=freeSlot(result.rebuilt);
        if(step.action==Action::AlreadyKnown) {
            if(step.method!=Method::LevelUp || !known) {
                result.status=Status::CandidateContradicted;return result;
            }
            continue;
        }
        if(step.action==Action::Decline) {
            if(step.method!=Method::LevelUp || known || open<4) {
                result.status=Status::CandidateContradicted;return result;
            }
            ++result.declined;continue;
        }
        if(known || step.slot>=4 || (open<4 && step.slot!=open)) {
            result.status=Status::CandidateContradicted;return result;
        }
        result.rebuilt[step.slot]=step.move;++result.applied;
    }
    if(generation(current)==3 &&
       !structural(path.observedMoves,354))return result;
    result.status=result.rebuilt==path.observedMoves?
        Status::CandidateConsistent:Status::CandidateContradicted;
    return result;
}
// A compact candidate-history DAG is supplied as independently attested
// alternate paths. This is never a proof that every genuine source/history
// has been enumerated. An unknown branch wins over a local contradiction.
struct Paths {
    std::array<Path,8> alternatives{};
    uint8_t count=0;
    bool allCandidateHistoriesEnumerated=false;
};
struct PathsResult {
    Status status=Status::Unresolved;
    uint8_t consistent=0, contradicted=0, unresolved=0;
};
constexpr PathsResult evaluate(const Paths& paths) noexcept {
    PathsResult out{};
    if(!paths.count || paths.count>paths.alternatives.size())return out;
    uint32_t id=paths.alternatives[0].candidateId;
    if(!id)return out;
    for(uint8_t i=0;i<paths.count;++i) {
        if(paths.alternatives[i].candidateId!=id)return out;
        const auto result=evaluate(paths.alternatives[i]);
        switch(result.status) {
        case Status::CandidateConsistent:++out.consistent;break;
        case Status::CandidateContradicted:++out.contradicted;break;
        case Status::Unresolved:case Status::Unsupported:
            ++out.unresolved;break;
        }
    }
    if(out.consistent)out.status=Status::CandidateConsistent;
    else if(!out.unresolved && paths.allCandidateHistoriesEnumerated)
        out.status=Status::CandidateContradicted;
    return out;
}
// This is a compile-time smoke contract for existing registered host tests.
// The complete positive/negative/Unknown suite is tests/test_legality_l3_*.
static_assert(generation(Game::Ruby)==3 && generation(Game::LeafGreen)==3);
static_assert(generation(Game::Platinum)==4 && generation(Game::SoulSilver)==4);
static_assert(structural({33,45,0,0},354));
static_assert(!structural({33,0,45,0},354));
} // namespace Legality::Gen14CandidateMoveChronology
