#pragma once
// L3 Stage 4. Read-only, candidate-local original Gen III -> Pal Park ->
// native Gen IV four-slot witness. All source decisions must be independently
// attested. A contradicted candidate is never global Pokemon illegality.
#include "Legality/Gen14CandidateMoveJointEvolutionHistory.h"
#include <array>
#include <cstdint>

namespace Legality::Gen14CandidateMovePalParkNativeJoint {
namespace Pre=Gen14CandidateMoveJointEvolutionHistory;
enum class Game : uint8_t {Unknown,Diamond,Pearl,Platinum,HeartGold,SoulSilver};
enum class Action : uint8_t {OfferLearn,OfferDecline,OfferKnown,Machine,Tutor,Reminder,DeleteMove,VisitGame,Unsupported};
enum class Status : uint8_t {Unresolved,Consistent,Contradicted};
struct Offer {uint8_t level=0;uint16_t move=0;};
struct Step {
    Game game=Game::Unknown;
    uint8_t level=0;
    Action action=Action::Unsupported;
    uint16_t move=0;
    uint8_t slot=0;
    bool hidden=false;
    uint8_t machineNumber=0;
    uint32_t itemReceipt=0;
    bool independentlyVerified=false;
    bool methodSourceVerified=false;
};
struct Input {
    Pre::Path departure{};
    uint32_t candidateId=0;
    Game arrivalGame=Game::Unknown,observedGame=Game::Unknown;
    uint8_t arrivalLevel=0,observedLevel=0;
    uint16_t species=0;
    uint8_t nativeForm=0; // Explicit Platinum Deoxys base form only.
    bool nativeFormVerified=false;
    std::array<uint16_t,4> recordedDeparture{},recordedArrival{},observedMoves{};
    bool samePokemonThroughTransferVerified=false;
    bool preTransferSnapshotVerified=false;
    bool immediateArrivalSnapshotVerified=false;
    bool nativeActionsCompleteVerified=false;
    std::array<Step,48> steps{};
    uint8_t count=0;
};
struct Result {
    Status status=Status::Unresolved;
    std::array<uint16_t,4> rebuilt{};
    uint16_t species=0;
    Game finalGame=Game::Unknown;
    uint8_t verifiedOffers=0,verifiedMachines=0,verifiedTutors=0;
    uint8_t verifiedReminders=0,verifiedDeletions=0,visits=0;
};
constexpr bool validGame(Game g) noexcept {return g>=Game::Diamond && g<=Game::SoulSilver;}
constexpr bool isGen3HM(uint16_t m) noexcept {
    return m==15||m==19||m==57||m==70||m==148||m==249||m==127||m==291;
}
constexpr bool validSlots(const std::array<uint16_t,4>& a,uint16_t max) noexcept {
    bool empty=false;
    for(uint8_t i=0;i<4;++i) {
        if(!a[i]){empty=true;continue;}
        if(empty||a[i]>max)return false;
        for(uint8_t j=0;j<i;++j)if(a[i]==a[j])return false;
    }
    return true;
}
constexpr bool has(const std::array<uint16_t,4>& a,uint16_t move) noexcept {
    for(const auto x:a)if(x==move)return true;
    return false;
}
constexpr uint8_t openSlot(const std::array<uint16_t,4>& a) noexcept {
    for(uint8_t i=0;i<4;++i)if(!a[i])return i;
    return 4;
}
// GBA must provide complete level offers and original TM receipts, as in
// Stage 3. Native provides supported(game,species), offerCount, offerAt,
// machineKnown(game,hm,no), machineCompatible(game,species,hm,no,move),
// tutorCompatible(game,species,move). Unmodeled is Unknown.
template<class Gba,class Native> constexpr Result evaluate(const Input& in) noexcept {
    Result out{};
    if(!in.candidateId||!in.samePokemonThroughTransferVerified||
       !in.preTransferSnapshotVerified||!in.immediateArrivalSnapshotVerified||
       !in.nativeActionsCompleteVerified||!validGame(in.arrivalGame)||
       !validGame(in.observedGame)||!in.species||in.species>386||
       !in.arrivalLevel||in.observedLevel<in.arrivalLevel||
       in.observedLevel>100||in.count>in.steps.size()||
       (in.species==386 && (!in.nativeFormVerified || in.nativeForm!=0))||
       !validSlots(in.recordedDeparture,354)||
       !validSlots(in.recordedArrival,467)||
       !validSlots(in.observedMoves,467))return out;
    if(in.departure.candidateId!=in.candidateId||
       in.departure.observedSpecies!=in.species||
       in.departure.observedLevel!=in.arrivalLevel)return out;
    // Unknown future method or unsupported source must not accidentally
    // turn a shorter plausible prefix into a global false negative.
    for(uint8_t i=0;i<in.count;++i) {
        const auto& s=in.steps[i];
        if(!s.independentlyVerified||!validGame(s.game)||
           s.level<in.arrivalLevel||s.level>in.observedLevel||
           s.action==Action::Unsupported)return out;
        if(s.action!=Action::VisitGame && s.action!=Action::DeleteMove) {
            if(!s.methodSourceVerified||!s.move||s.move>467)return out;
            if(!Native::supported(s.game,in.species))return out;
            if(s.action==Action::Machine &&
               (!s.machineNumber||!Native::machineKnown(s.game,s.hidden,s.machineNumber)||
               (!s.hidden && !s.itemReceipt)))return out;
        }
        if(s.action==Action::DeleteMove && !s.methodSourceVerified)return out;
    }
    const auto pre=Pre::evaluate<Gba>(in.departure);
    if(pre.status==Pre::Status::Unresolved)return out;
    if(pre.status==Pre::Status::CandidateContradicted) {
        out.status=Status::Contradicted;return out;
    }
    if(pre.rebuilt!=in.recordedDeparture) {
        out.status=Status::Contradicted;return out;
    }
    for(const auto move:in.recordedDeparture)if(isGen3HM(move)) {
        out.status=Status::Contradicted;return out;
    }
    if(in.recordedDeparture!=in.recordedArrival) {
        out.status=Status::Contradicted;return out;
    }
    Game game=in.arrivalGame;uint8_t level=in.arrivalLevel;
    out.rebuilt=in.recordedArrival;
    std::array<uint32_t,48> consumed{};uint8_t used=0;
    // Native offers are prompted only on real level-ups (not automatically
    // on cartridge arrivals or game visits). Preserve same-level order.
    auto nextAfter=[&](Game g,uint8_t lv) constexpr {
        uint16_t i=0;
        if(!Native::supported(g,in.species))return i;
        while(i<Native::offerCount(g,in.species) &&
              Native::offerAt(g,in.species,i).level<=lv)++i;
        return i;
    };
    uint16_t next=nextAfter(game,level);
    for(uint8_t i=0;i<in.count;++i) {
        const auto& s=in.steps[i];
        if(s.level<level)return out;
        const bool offer=s.action==Action::OfferLearn||
                         s.action==Action::OfferDecline||
                         s.action==Action::OfferKnown;
        if(offer) {
            if(s.game!=game)return out;
            const auto expected=next<Native::offerCount(game,in.species)
                ?Native::offerAt(game,in.species,next):Offer{};
            if(expected.level!=s.level||expected.move!=s.move) {
                out.status=Status::Contradicted;return out;
            }
            const auto free=openSlot(out.rebuilt);
            const bool known=has(out.rebuilt,s.move);
            if(s.action==Action::OfferKnown) {
                if(!known){out.status=Status::Contradicted;return out;}
            } else if(s.action==Action::OfferDecline) {
                if(known||free<4){out.status=Status::Contradicted;return out;}
            } else {
                if(known||s.slot>=4||(free<4&&s.slot!=free)) {
                    out.status=Status::Contradicted;return out;
                }
                out.rebuilt[s.slot]=s.move;
            }
            ++next;++out.verifiedOffers;
        } else {
            if(Native::supported(game,in.species) &&
               next<Native::offerCount(game,in.species) &&
               Native::offerAt(game,in.species,next).level<=s.level) {
                out.status=Status::Contradicted;return out;
            }
            if(s.action==Action::VisitGame) {
                if(s.game==game||s.move||s.machineNumber||s.itemReceipt)return out;
                game=s.game;next=nextAfter(game,s.level);++out.visits;
            } else if(s.action==Action::DeleteMove) {
                if(s.game!=game||s.slot>=4||s.move!=out.rebuilt[s.slot]||
                   !out.rebuilt[1]||s.machineNumber||s.itemReceipt)return out;
                for(uint8_t k=s.slot;k<3;++k)out.rebuilt[k]=out.rebuilt[k+1];
                out.rebuilt[3]=0;++out.verifiedDeletions;
            } else if(s.action==Action::Machine||s.action==Action::Tutor||
                      s.action==Action::Reminder) {
                if(s.game!=game)return out;
                if(s.action==Action::Machine) {
                    if(!Native::machineCompatible(game,in.species,s.hidden,
                                                  s.machineNumber,s.move)) {
                        out.status=Status::Contradicted;return out;
                    }
                    if(!s.hidden) {
                        for(uint8_t j=0;j<used;++j)if(consumed[j]==s.itemReceipt) {
                            out.status=Status::Contradicted;return out;
                        }
                        consumed[used++]=s.itemReceipt;
                    }
                    ++out.verifiedMachines;
                } else if(s.action==Action::Tutor) {
                    if(!Native::tutorCompatible(game,in.species,s.move)) {
                        out.status=Status::Contradicted;return out;
                    }
                    ++out.verifiedTutors;
                } else {
                    bool seen=false;
                    for(uint16_t j=0;j<Native::offerCount(game,in.species);++j) {
                        const auto o=Native::offerAt(game,in.species,j);
                        if(o.move==s.move && o.level<=s.level)seen=true;
                    }
                    if(!seen){out.status=Status::Contradicted;return out;}
                    ++out.verifiedReminders;
                }
                const auto free=openSlot(out.rebuilt);
                if(has(out.rebuilt,s.move)||s.slot>=4||
                   (free<4&&s.slot!=free)) {
                    out.status=Status::Contradicted;return out;
                }
                out.rebuilt[s.slot]=s.move;
            } else return out;
        }
        level=s.level;
    }
    if(Native::supported(game,in.species)) {
        if(next<Native::offerCount(game,in.species) &&
           Native::offerAt(game,in.species,next).level<=in.observedLevel) {
            out.status=Status::Contradicted;return out;
        }
    } else if(in.observedLevel>level)return out;
    out.species=in.species;out.finalGame=game;
    out.status=(game==in.observedGame && out.rebuilt==in.observedMoves)
        ?Status::Consistent:Status::Contradicted;
    return out;
}
} // namespace Legality::Gen14CandidateMovePalParkNativeJoint
