#pragma once
// L3 Stage 2: read-only Gen I/II same-individual, four-slot multi-stage witness.
// Original tables: pret/pokered@af519899719f0754965776faac0e836a3b906e6d,
// pret/pokeyellow@e89ead154b9968aa50eed9328ff2b38b6c194382,
// pret/pokegold@ef0201d8daf47e8b3ea1518eacf890f37d4cd5e8,
// pret/pokecrystal@3bc8daa4173e96a7f4011dad3922eb6fa5dad5c6.
// Conditional original-source witness ONLY. No original Pokémon, parent,
// machine-receipt, evolution event or historical trade is authenticated here.
#include <array>
#include <cstdint>
namespace Legality::Gen12CandidateMoveMultiStage {
enum class Game : std::uint8_t { Unknown, Red, Blue, Yellow, Gold, Silver, Crystal };
enum class Action : std::uint8_t {
    Learn, Decline, AlreadyKnown, TeachTM, TeachHM, Evolve, VisitGame,
    ForwardToGen2, ToGen1, ReturnToGen2, DeleteMove, Breeding, Tutor, Reminder
};
enum class Status : std::uint8_t { Unknown, CandidateConsistent, CandidateContradicted };
struct Offer { std::uint8_t level=0; std::uint16_t move=0; };
constexpr std::uint8_t generation(Game game) noexcept {
    switch(game) {
    case Game::Red:case Game::Blue:case Game::Yellow:return 1;
    case Game::Gold:case Game::Silver:case Game::Crystal:return 2;
    default:return 0;
    }
}
// Complete retail level-up rows for the following ORIGINAL species/game
// pairs. Species missing from this bounded set must not become Invalid.
inline constexpr std::array<Offer,7> kBulba1{{
    {7,73},{13,22},{20,77},{27,75},{34,74},{41,79},{48,76}}};
inline constexpr std::array<Offer,7> kIvy1{{
    {7,73},{13,22},{22,77},{30,75},{38,74},{46,79},{54,76}}};
inline constexpr std::array<Offer,7> kVenu1{{
    {7,73},{13,22},{22,77},{30,75},{43,74},{55,79},{65,76}}};
inline constexpr std::array<Offer,6> kCharm1{{
    {9,52},{15,43},{22,99},{30,163},{38,53},{46,83}}};
inline constexpr std::array<Offer,6> kCharme1{{
    {9,52},{15,43},{24,99},{33,163},{42,53},{56,83}}};
inline constexpr std::array<Offer,11> kBulba2{{
    {1,33},{4,45},{7,73},{10,22},{15,77},{15,79},{20,75},
    {25,230},{32,74},{39,235},{46,76}}};
inline constexpr std::array<Offer,13> kIvy2{{
    {1,33},{1,45},{1,73},{4,45},{7,73},{10,22},{15,77},
    {15,79},{22,75},{29,230},{38,74},{47,235},{56,76}}};
inline constexpr std::array<Offer,14> kVenu2{{
    {1,33},{1,45},{1,73},{1,22},{4,45},{7,73},{10,22},
    {15,77},{15,79},{22,75},{29,230},{41,74},{53,235},{65,76}}};
inline constexpr std::array<Offer,8> kCynda2{{
    {1,33},{1,43},{6,108},{12,52},{19,98},{27,172},{36,129},{46,53}}};
inline constexpr std::array<Offer,9> kQuilava2{{
    {1,33},{1,43},{1,108},{6,108},{12,52},{21,98},
    {31,172},{42,129},{54,53}}};
constexpr bool supported(Game game,std::uint16_t species) noexcept {
    switch(generation(game)) {
    case 1:return species==1||species==2||species==3||species==4||species==5;
    case 2:return species==1||species==2||species==3||species==155||species==156;
    default:return false;
    }
}
constexpr std::uint8_t count(Game game,std::uint16_t species) noexcept {
    if(!supported(game,species))return 0;
    if(generation(game)==1) {
        return species==1||species==2||species==3 ? 7 : 6;
    }
    switch(species) {
    case 1:return 11;case 2:return 13;case 3:return 14;
    case 155:return 8;case 156:return 9;
    default:return 0;
    }
}
constexpr Offer offer(Game game,std::uint16_t species,std::uint8_t i) noexcept {
    if(i>=count(game,species))return {};
    if(generation(game)==1) {
        switch(species) {
        case 1:return kBulba1[i];case 2:return kIvy1[i];
        case 3:return kVenu1[i];case 4:return kCharm1[i];
        case 5:return kCharme1[i];default:return {};
        }
    }
    switch(species) {
    case 1:return kBulba2[i];case 2:return kIvy2[i];
    case 3:return kVenu2[i];case 155:return kCynda2[i];
    case 156:return kQuilava2[i];default:return {};
    }
}
constexpr std::uint8_t nextAfter(Game game,std::uint16_t species,
                                 std::uint8_t level) noexcept {
    std::uint8_t i=0;
    while(i<count(game,species) && offer(game,species,i).level<=level)++i;
    return i;
}
constexpr bool contains(const std::array<std::uint16_t,4>& m,std::uint16_t x) noexcept {
    for(auto v:m)if(v==x)return true;
    return false;
}
constexpr std::uint8_t vacant(const std::array<std::uint16_t,4>& m) noexcept {
    for(std::uint8_t i=0;i<4;++i)if(m[i]==0)return i;
    return 4;
}
constexpr bool validSlots(const std::array<std::uint16_t,4>& m,std::uint16_t max) noexcept {
    bool gap=false;
    for(std::uint8_t i=0;i<4;++i) {
        if(m[i]==0){gap=true;continue;}
        if(m[i]>max||gap)return false;
        for(std::uint8_t j=0;j<i;++j)if(m[i]==m[j])return false;
    }
    return true;
}
// Exact original machine-to-move witnesses ONLY for Bulbasaur on currently
// modeled cartridges (source compatible at the original species stage).
// This is NOT an actual consumed TM/HM receipt.
constexpr std::uint16_t bulbasaurMachine(Game game,Action action,
                                        std::uint8_t number) noexcept {
    if(action==Action::TeachHM && number==1)return 15; // Cut
    if(action!=Action::TeachTM)return 0;
    if(generation(game)==1) {
        if(number==6)return 92;
        if(number==8)return 34;
        if(number==10)return 38;
    } else if(generation(game)==2) {
        if(number==6)return 92;
        if(number==10)return 237;
    }
    return 0;
}
// Pinned evolution levels from retail original evos_moves / evos_attacks.
constexpr bool levelEvolution(Game game,std::uint16_t from,
                              std::uint16_t to,std::uint8_t level) noexcept {
    if(!generation(game))return false;
    if(from==1&&to==2)return level>=16;
    if(from==2&&to==3)return level>=32;
    if(generation(game)==1&&from==4&&to==5)return level>=16;
    if(generation(game)==2&&from==155&&to==156)return level>=14;
    return false;
}
constexpr bool supportedEvolution(Game game,std::uint16_t from,
                                  std::uint16_t to) noexcept {
    return (from==1&&to==2)||(from==2&&to==3)||
           (generation(game)==1&&from==4&&to==5)||
           (generation(game)==2&&from==155&&to==156);
}
struct Step {
    Game game=Game::Unknown;      // source game for method / post-transition game
    std::uint16_t species=0;     // species AFTER action, 0 for non-evolution
    std::uint8_t level=0;
    Action action=Action::Learn;
    std::uint16_t move=0;
    std::uint8_t slot=0;
    std::uint8_t machineNumber=0;
    bool originalMethodWitness=false; // independently verified hypothetical stage
};
struct Path {
    std::uint32_t candidateId=0;
    std::uint16_t initialSpecies=0,observedSpecies=0;
    Game originalGame=Game::Unknown,observedGame=Game::Unknown;
    std::uint8_t initialLevel=0,observedLevel=0;
    std::array<std::uint16_t,4> initialMoves{},observedMoves{};
    bool initialSnapshotAttested=false;
    bool sameIndividualAttested=false;
    bool everyInterveningActionAttested=false;
    bool timeCapsulePolicyAttested=false;
    bool sourceWasNotEggAttested=false;
    std::array<Step,48> steps{};
    std::uint8_t stepCount=0;
};
struct Result {
    Status status=Status::Unknown;
    std::array<std::uint16_t,4> moves{};
    std::uint16_t currentSpecies=0;
    std::uint8_t levelOffers=0,machines=0,evolutions=0,visits=0,deletions=0;
    bool toGen1=false,fromGen1=false;
};
constexpr Result evaluate(const Path& path) noexcept {
    Result out{};
    if(!path.candidateId||!path.initialSnapshotAttested||
       !path.sameIndividualAttested||!path.everyInterveningActionAttested||
       !path.initialLevel||path.initialLevel>100||
       path.observedLevel<path.initialLevel||path.observedLevel>100||
       path.stepCount>path.steps.size()||
       !supported(path.originalGame,path.initialSpecies)||
       !supported(path.observedGame,path.observedSpecies)||
       !path.initialMoves[0]||!path.observedMoves[0]||
       !validSlots(path.initialMoves,generation(path.originalGame)==1?165:251)||
       !validSlots(path.observedMoves,generation(path.observedGame)==1?165:251))return out;
    // Preflight ALL methods so that a future unsupported egg, tutor, reminder,
    // unknown TM, migration or game visit cannot be masked by an earlier
    // candidate-specific contradiction. Never invent provenance from PK1/2.
    for(std::uint8_t i=0;i<path.stepCount;++i) {
        const Step& s=path.steps[i];
        if(!s.originalMethodWitness||(!supported(s.game,s.species?s.species:path.initialSpecies) &&
            s.action!=Action::Evolve))return out;
        if(s.action==Action::Breeding||s.action==Action::Tutor||
           s.action==Action::Reminder)return out;
        if(s.action==Action::TeachTM||s.action==Action::TeachHM) {
            if(!s.machineNumber || !s.move ||
               !bulbasaurMachine(s.game,s.action,s.machineNumber) ||
               (s.species!=0 && s.species!=1))return out;
        }
        if(s.action==Action::ForwardToGen2||s.action==Action::ToGen1||
           s.action==Action::ReturnToGen2) {
            if(!path.timeCapsulePolicyAttested||!path.sourceWasNotEggAttested)
                return out;
        }
        if(s.action==Action::Evolve &&
           (!s.species||!supported(s.game,s.species)||s.move||s.slot||
            s.machineNumber))return out;
        if((s.action==Action::Learn||s.action==Action::Decline||
            s.action==Action::AlreadyKnown) &&
           (s.machineNumber || (s.action!=Action::Learn && s.slot)))return out;
        if((s.action==Action::VisitGame||s.action==Action::ForwardToGen2||
            s.action==Action::ToGen1||s.action==Action::ReturnToGen2) &&
           (s.move||s.slot||s.machineNumber))return out;
        // Evolution edges are proved relative to the *current* species in
        // the replay, not the initial species or the target species here.
    }
    out.moves=path.initialMoves;
    Game current=path.originalGame,returnGame=Game::Unknown;
    std::uint16_t species=path.initialSpecies;
    std::uint8_t level=path.initialLevel,cursor=nextAfter(current,species,level);
    for(std::uint8_t i=0;i<path.stepCount;++i) {
        const Step& s=path.steps[i];
        if(s.level<level||s.level>path.observedLevel||!s.level)return out;
        const bool levelAction=s.action==Action::Learn||s.action==Action::Decline||
                               s.action==Action::AlreadyKnown;
        const auto pending=offer(current,species,cursor);
        if(levelAction) {
            if(s.game!=current||(s.species && s.species!=species) ||
               s.machineNumber||!pending.level||
               s.level!=pending.level||s.move!=pending.move) {
                out.status=Status::CandidateContradicted;return out;
            }
            ++cursor;++out.levelOffers;
            const auto free=vacant(out.moves);
            const bool known=contains(out.moves,s.move);
            if(s.action==Action::AlreadyKnown) {
                if(!known){out.status=Status::CandidateContradicted;return out;}
            } else if(s.action==Action::Decline) {
                if(known||free<4){out.status=Status::CandidateContradicted;return out;}
            } else {
                if(known||s.slot>=4||(free<4&&s.slot!=free)){
                    out.status=Status::CandidateContradicted;return out;
                }
                out.moves[s.slot]=s.move;
            }
            level=s.level;continue;
        }
        if(pending.level && pending.level<=s.level) {
            out.status=Status::CandidateContradicted;return out;
        }
        if(s.action==Action::TeachTM||s.action==Action::TeachHM) {
            if(s.game!=current||species!=1||(s.species && s.species!=species))return out;
            if(bulbasaurMachine(current,s.action,s.machineNumber)!=s.move) {
                out.status=Status::CandidateContradicted;return out;
            }
            const auto free=vacant(out.moves);
            if(contains(out.moves,s.move)||s.slot>=4||
               (free<4&&s.slot!=free)) {
                out.status=Status::CandidateContradicted;return out;
            }
            out.moves[s.slot]=s.move;++out.machines;
        } else if(s.action==Action::Evolve) {
            if(s.game!=current||!supported(current,s.species)||
               !supportedEvolution(current,species,s.species))return out;
            if(!levelEvolution(current,species,s.species,s.level)) {
                out.status=Status::CandidateContradicted;return out;
            }
            // Evolution-time learning can have game-specific additional
            // offers. Without an independently modelled list it is Unknown.
            const auto nextCursor=nextAfter(current,s.species,s.level);
            if(nextCursor && offer(current,s.species,static_cast<std::uint8_t>(nextCursor-1)).level==s.level)
                return out;
            species=s.species;cursor=nextCursor;++out.evolutions;
        } else if(s.action==Action::DeleteMove) {
            // The Move Deleter exists in Gen II only; cannot infer a Gen I
            // service or invent which old move was erased.
            if(generation(current)!=2||s.game!=current||s.species||s.move||
               s.machineNumber||s.slot>=4||!out.moves[s.slot]||
               !out.moves[1])return out;
            for(std::uint8_t j=s.slot;j<3;++j)out.moves[j]=out.moves[j+1];
            out.moves[3]=0;++out.deletions;
        } else {
            if((s.species&&s.species!=species)||s.move||s.machineNumber||s.slot||
               !supported(s.game,species))return out;
            if(s.action==Action::VisitGame) {
                if(s.game==current||generation(current)!=generation(s.game))return out;
            } else if(s.action==Action::ForwardToGen2) {
                if(generation(current)!=1||generation(s.game)!=2||species>151||
                   returnGame!=Game::Unknown||!validSlots(out.moves,165))return out;
                out.fromGen1=true;
            } else if(s.action==Action::ToGen1) {
                if(generation(current)!=2||generation(s.game)!=1||species>151||
                   returnGame!=Game::Unknown)return out;
                if(!validSlots(out.moves,165)) {
                    out.status=Status::CandidateContradicted;return out;
                }
                returnGame=current;out.toGen1=true;
            } else if(s.action==Action::ReturnToGen2) {
                if(!out.toGen1||generation(current)!=1||s.game!=returnGame)return out;
                returnGame=Game::Unknown;out.fromGen1=true;
            } else return out;
            current=s.game;cursor=nextAfter(current,species,s.level);++out.visits;
        }
        level=s.level;
    }
    if(current!=path.observedGame||species!=path.observedSpecies)return out;
    const auto next=offer(current,species,cursor);
    if(next.level && next.level<=path.observedLevel) {
        out.status=Status::CandidateContradicted;return out;
    }
    if(!validSlots(out.moves,generation(current)==1?165:251))return out;
    out.currentSpecies=species;
    out.status=out.moves==path.observedMoves?
        Status::CandidateConsistent:Status::CandidateContradicted;
    return out;
}
struct Alternatives {
    std::array<Path,12> paths{};
    std::uint8_t count=0;
    bool everyActualEncounterAndHistoryEnumerated=false;
};
constexpr Status evaluate(const Alternatives& paths) noexcept {
    if(!paths.count||paths.count>paths.paths.size()||!paths.paths[0].candidateId)
        return Status::Unknown;
    bool unknown=false;
    for(std::uint8_t i=0;i<paths.count;++i) {
        if(paths.paths[i].candidateId!=paths.paths[0].candidateId)return Status::Unknown;
        switch(evaluate(paths.paths[i]).status) {
        case Status::CandidateConsistent:return Status::CandidateConsistent;
        case Status::Unknown:unknown=true;break;
        case Status::CandidateContradicted:break;
        }
    }
    return unknown||!paths.everyActualEncounterAndHistoryEnumerated ?
        Status::Unknown : Status::CandidateContradicted;
}
static_assert(offer(Game::Red,2,4).move==74);
static_assert(offer(Game::Crystal,2,6).move==77);
static_assert(offer(Game::Crystal,1,4).level==15&&
              offer(Game::Crystal,1,5).level==15);
static_assert(bulbasaurMachine(Game::Red,Action::TeachTM,10)==38&&
              bulbasaurMachine(Game::Gold,Action::TeachTM,10)==237);
static_assert(levelEvolution(Game::Red,1,2,16)&&
              !levelEvolution(Game::Red,1,2,15));
} // namespace Legality::Gen12CandidateMoveMultiStage
