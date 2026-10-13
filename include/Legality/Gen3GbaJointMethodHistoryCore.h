#pragma once
// L3 Stage 3: source-complete, same-original-cartridge Gen III candidate
// history. All results are local to one externally authenticated history.
// This module neither invents machine receipts nor labels a Pokemon Invalid.
#include <array>
#include <cstdint>
#include <span>
namespace Legality::Gen3GbaJointMethodHistoryCore {
enum class Game : std::uint8_t { Unknown, Ruby, Sapphire, Emerald, FireRed, LeafGreen };
enum class Action : std::uint8_t {
    LevelLearn, LevelDecline, LevelAlreadyKnown, TeachTM, TeachHM,
    Relearn, DeleteMove, UnknownService
};
enum class Status : std::uint8_t { Unknown, CandidateConsistent, CandidateContradicted };
struct Offer { std::uint8_t level=0; std::uint16_t move=0; };
struct Catalog {
    Game game=Game::Unknown;
    std::uint16_t species=0;
    std::span<const Offer> offers{};
    std::span<const std::uint16_t> machineMoves{}; // ordered TM01..50, HM01..08
    std::uint64_t machineMask=0;
    bool originalSourceVerified=false;
    bool fullLevelUpSourceVerified=false;
    bool fullMachineSourceVerified=false;
    bool originalRelearnerFunctionVerified=false;
};
struct Step {
    Game game=Game::Unknown;
    std::uint8_t level=0;
    Action action=Action::UnknownService;
    std::uint16_t move=0;
    std::uint8_t slot=0;
    std::uint8_t machineNumber=0;
    bool originalActionReceiptVerified=false;
};
struct History {
    std::uint32_t candidateId=0;
    Game game=Game::Unknown;
    std::uint16_t species=0;
    std::uint8_t initialLevel=0, observedLevel=0;
    std::array<std::uint16_t,4> initialMoves{}, observedMoves{};
    bool initialSnapshotVerified=false;
    bool samePokemonAndGameVerified=false;
    bool allPlayerActionsVerified=false;
    bool allAlternativeAcquisitionRoutesExcluded=false; // intentionally not required or inferred
    std::array<Step,64> steps{};
    std::uint8_t stepCount=0;
};
struct Result {
    Status status=Status::Unknown;
    std::array<std::uint16_t,4> rebuilt{};
    std::uint16_t mandatoryOffers=0, machines=0, reminders=0, deletions=0;
    bool globalLegalityClaim=false;
};
constexpr bool supported(Game game) noexcept {
    return game>=Game::Ruby && game<=Game::LeafGreen;
}
constexpr bool structural(const std::array<std::uint16_t,4>& a) noexcept {
    if(!a[0])return false;
    bool gap=false;
    for(unsigned i=0;i<4;++i) {
        if(!a[i]) {gap=true;continue;}
        if(a[i]>354||gap)return false;
        for(unsigned j=0;j<i;++j)if(a[j]==a[i])return false;
    }
    return true;
}
constexpr bool contains(const std::array<std::uint16_t,4>& a,std::uint16_t m) noexcept {
    for(auto x:a)if(x==m)return true;
    return false;
}
constexpr std::uint8_t firstFree(const std::array<std::uint16_t,4>& a) noexcept {
    for(std::uint8_t i=0;i<4;++i)if(!a[i])return i;
    return 4;
}
constexpr bool pinnedCatalog(const Catalog& c) noexcept {
    if(!supported(c.game)||c.species<1||c.species>386||
       !c.originalSourceVerified||!c.fullLevelUpSourceVerified||
       !c.fullMachineSourceVerified||!c.originalRelearnerFunctionVerified||
       c.offers.empty()||c.offers.size()>64||c.machineMoves.size()!=58||
       (c.machineMask>>58))return false;
    std::uint8_t last=0;
    for(const auto& o:c.offers) {
        if(o.level<1||o.level>100||o.move<1||o.move>354||o.level<last)return false;
        last=o.level;
    }
    for(auto m:c.machineMoves)if(m<1||m>354)return false;
    return true;
}
// A verified original relearner menu contains current-species offers at or
// below the current level, excluding already known moves, with duplicates
// removed in the original order. A detached PK3 proves no service visit.
constexpr bool inRelearnerMenu(const Catalog& source,std::uint8_t level,
                               const std::array<std::uint16_t,4>& known,
                               std::uint16_t requested) noexcept {
    if(!requested||contains(known,requested))return false;
    for(std::size_t i=0;i<source.offers.size();++i) {
        const auto o=source.offers[i];
        if(o.level>level||o.move!=requested)continue;
        bool earlier=false;
        for(std::size_t j=0;j<i;++j)
            if(source.offers[j].level<=level && source.offers[j].move==requested)
                {earlier=true;break;}
        if(!earlier)return true;
    }
    return false;
}
constexpr Result evaluate(const Catalog& source,const History& path) noexcept {
    Result out{};
    if(!pinnedCatalog(source)||!path.candidateId||path.species!=source.species||
       path.game!=source.game||!path.initialSnapshotVerified||
       !path.samePokemonAndGameVerified||!path.allPlayerActionsVerified||
       !path.initialLevel||path.initialLevel>100||
       path.observedLevel<path.initialLevel||path.observedLevel>100||
       path.stepCount>path.steps.size()||!structural(path.initialMoves)||
       !structural(path.observedMoves))return out;
    // Preflight the *whole* packet. Unsupported/unattested later services must
    // not allow an earlier guessed path conflict to become a hard rejection.
    for(std::uint8_t i=0;i<path.stepCount;++i) {
        const auto& s=path.steps[i];
        if(!s.originalActionReceiptVerified||s.game!=path.game||
           s.level<path.initialLevel||s.level>path.observedLevel||
           s.move>354||s.action==Action::UnknownService)return out;
        if((s.action==Action::TeachTM||s.action==Action::TeachHM) &&
           (!s.machineNumber||s.machineNumber>(s.action==Action::TeachTM?50:8)))return out;
        if(s.action==Action::Relearn && !source.originalRelearnerFunctionVerified)
            return out;
    }
    out.rebuilt=path.initialMoves;
    std::size_t cursor=0;
    while(cursor<source.offers.size()&&source.offers[cursor].level<=path.initialLevel)
        ++cursor;
    std::uint8_t level=path.initialLevel;
    for(std::uint8_t i=0;i<path.stepCount;++i) {
        const Step s=path.steps[i];
        if(s.level<level)return out; // impossible chronological order -> no proof
        const bool isOffer=s.action==Action::LevelLearn||
                           s.action==Action::LevelDecline||
                           s.action==Action::LevelAlreadyKnown;
        if(isOffer) {
            if(cursor>=source.offers.size()||
               source.offers[cursor].level!=s.level||
               source.offers[cursor].move!=s.move||s.machineNumber) {
                out.status=Status::CandidateContradicted;return out;
            }
            ++cursor;++out.mandatoryOffers;
            const bool already=contains(out.rebuilt,s.move);
            const auto free=firstFree(out.rebuilt);
            if(s.action==Action::LevelAlreadyKnown) {
                if(!already){out.status=Status::CandidateContradicted;return out;}
            } else if(s.action==Action::LevelDecline) {
                if(already||free<4){out.status=Status::CandidateContradicted;return out;}
            } else {
                if(already||s.slot>=4||(free<4&&s.slot!=free)){
                    out.status=Status::CandidateContradicted;return out;
                }
                out.rebuilt[s.slot]=s.move;
            }
        } else {
            if(cursor<source.offers.size()&&source.offers[cursor].level<=s.level) {
                out.status=Status::CandidateContradicted;return out;
            }
            if(s.action==Action::DeleteMove) {
                if(s.move||s.machineNumber||s.slot>=4||!out.rebuilt[s.slot]||
                   !out.rebuilt[1]){out.status=Status::CandidateContradicted;return out;}
                for(std::uint8_t j=s.slot;j<3;++j)out.rebuilt[j]=out.rebuilt[j+1];
                out.rebuilt[3]=0;++out.deletions;
                level=s.level;continue;
            }
            if(!s.move||s.slot>=4||contains(out.rebuilt,s.move)){
                out.status=Status::CandidateContradicted;return out;
            }
            if(s.action==Action::TeachTM||s.action==Action::TeachHM) {
                const std::size_t index=s.action==Action::TeachTM?
                    static_cast<std::size_t>(s.machineNumber-1):
                    static_cast<std::size_t>(s.machineNumber+49);
                if(index>=source.machineMoves.size())return out;
                if(source.machineMoves[index]!=s.move ||
                   !(source.machineMask&(std::uint64_t{1}<<index))) {
                    out.status=Status::CandidateContradicted;return out;
                }
                ++out.machines;
            } else if(s.action==Action::Relearn) {
                if(s.machineNumber)return out;
                if(!inRelearnerMenu(source,s.level,out.rebuilt,s.move)) {
                    out.status=Status::CandidateContradicted;return out;
                }
                ++out.reminders;
            } else return out;
            const auto free=firstFree(out.rebuilt);
            if(free<4&&s.slot!=free) {
                out.status=Status::CandidateContradicted;return out;
            }
            out.rebuilt[s.slot]=s.move;
        }
        level=s.level;
    }
    if(cursor<source.offers.size()&&
       source.offers[cursor].level<=path.observedLevel) {
        out.status=Status::CandidateContradicted;return out;
    }
    out.status=out.rebuilt==path.observedMoves?
        Status::CandidateConsistent:Status::CandidateContradicted;
    return out;
}
// A failed candidate never proves all retail routes impossible. Incomplete
// enumeration and any Unknown route prevent even candidate-set contradiction.
struct Alternatives {
    std::array<History,8> paths{};
    std::uint8_t count=0;
    bool allActualHistoriesAuthenticatedAndEnumerated=false;
};
constexpr Status evaluate(const Catalog& source,const Alternatives& options) noexcept {
    if(!options.count||options.count>options.paths.size()||
       !options.paths[0].candidateId)return Status::Unknown;
    bool unknown=false;
    for(std::uint8_t i=0;i<options.count;++i) {
        if(options.paths[i].candidateId!=options.paths[0].candidateId)
            return Status::Unknown;
        const Status state=evaluate(source,options.paths[i]).status;
        if(state==Status::CandidateConsistent)return state;
        if(state==Status::Unknown)unknown=true;
    }
    return unknown||!options.allActualHistoriesAuthenticatedAndEnumerated?
        Status::Unknown:Status::CandidateContradicted;
}
} // namespace Legality::Gen3GbaJointMethodHistoryCore
