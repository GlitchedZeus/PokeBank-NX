#pragma once
// L3 Stage 5: candidate-only, read-only Gen III history -> Pal Park -> Gen IV.
// A witnessed evolution, trade visit, or Shedinja sibling must survive into
// the exact outbound PK3 snapshot. Never infer a move receipt from a PK4.
#include "Legality/Gen3GbaExceptionalHistoryCore.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace Legality::Gen34PalParkMultiCartridgeCore {
namespace X=Gen3GbaExceptionalHistoryCore;
namespace H=Gen3GbaJointMethodHistoryCore;
enum class Game : std::uint8_t { Unknown, Diamond, Pearl, Platinum, HeartGold, SoulSilver };
enum class Action : std::uint8_t {
    OfferLearn, OfferDecline, OfferKnown, TeachTM, TeachHM, Tutor,
    Reminder, DeleteMove, VisitGame, Unsupported
};
enum class Status : std::uint8_t { Unknown, CandidateConsistent, CandidateContradicted };
struct Offer { std::uint8_t level=0; std::uint16_t move=0; };
struct Machine { bool hm=false; std::uint8_t number=0; std::uint16_t move=0; bool compatible=false; };
struct Catalog {
    Game game=Game::Unknown;
    std::uint16_t species=0;
    std::span<const Offer> offers{};
    std::span<const Machine> machines{};
    std::span<const std::uint16_t> tutors{};
    bool fullLevelSourceVerified=false;
    bool listedNumberedMachineMethodsVerified=false;
    bool fullTutorSourceVerified=false;
};
struct Step {
    Game game=Game::Unknown;
    std::uint8_t level=0;
    Action action=Action::Unsupported;
    std::uint16_t move=0;
    std::uint8_t slot=0;
    std::uint8_t number=0;
    std::uint32_t independentReceipt=0;
    bool exactActionVerified=false;
    bool methodSourceVerified=false;
};
struct OriginalTmReceipt {
    std::uint8_t phaseIndex=0, stepIndex=0, machineNumber=0;
    X::Game game=X::Game::Unknown;
    std::uint16_t species=0, move=0;
    std::uint32_t singleUseItemReceipt=0;
    bool originalSourceAndItemVerified=false;
};
struct Input {
    X::Witness gba{};
    std::array<OriginalTmReceipt,64> sourceTmReceipts{};
    std::uint8_t sourceTmReceiptCount=0;
    std::uint32_t transferredCandidateId=0;
    std::uint16_t species=0;
    Game arrivalGame=Game::Unknown, observedGame=Game::Unknown;
    std::uint8_t arrivalLevel=0, observedLevel=0;
    std::uint8_t outboundForm=0, immediateForm=0;
    bool formContinuityVerified=false;
    std::array<std::uint16_t,4> outboundGba{}, immediateNds{}, observedMoves{};
    bool witnessedSameIndividualAtTransfer=false;
    bool witnessedOutboundSnapshot=false;
    bool witnessedImmediateArrivalSnapshot=false;
    bool witnessedWholeNativeActionHistory=false;
    std::array<Step,64> steps{};
    std::uint8_t stepCount=0;
};
struct Result {
    Status status=Status::Unknown;
    std::array<std::uint16_t,4> rebuilt{};
    std::uint16_t offers=0, machines=0, tutors=0, reminders=0, deletions=0, visits=0;
    std::uint8_t sourcePhases=0;
    Game finalGame=Game::Unknown;
    bool globalInvalidAuthorized=false;
};
[[nodiscard]] constexpr bool validGame(Game game) noexcept {
    return game>=Game::Diamond && game<=Game::SoulSilver;
}
[[nodiscard]] constexpr bool structured(const std::array<std::uint16_t,4>& a,
                                         std::uint16_t max) noexcept {
    if(!a[0])return false;
    bool empty=false;
    for(std::uint8_t i=0;i<4;++i) {
        if(!a[i]){empty=true;continue;}
        if(empty||a[i]>max)return false;
        for(std::uint8_t j=0;j<i;++j)if(a[j]==a[i])return false;
    }
    return true;
}
[[nodiscard]] constexpr bool oldHM(std::uint16_t m) noexcept {
    return m==15||m==19||m==57||m==70||m==148||m==249||m==127||m==291;
}
[[nodiscard]] constexpr bool contains(const std::array<std::uint16_t,4>& a,
                                       std::uint16_t m) noexcept {
    for(const auto x:a)if(x==m)return true;
    return false;
}
[[nodiscard]] constexpr std::uint8_t firstFree(const std::array<std::uint16_t,4>& a) noexcept {
    for(std::uint8_t i=0;i<4;++i)if(!a[i])return i;
    return 4;
}
[[nodiscard]] constexpr const Catalog* findCatalog(std::span<const Catalog> catalogs,
                                                    Game g,std::uint16_t sp) noexcept {
    for(const auto& c:catalogs)if(c.game==g && c.species==sp)return &c;
    return nullptr;
}
[[nodiscard]] constexpr bool validCatalog(const Catalog& c) noexcept {
    if(!validGame(c.game)||c.species<1||c.species>493||
       !c.fullLevelSourceVerified||c.offers.size()>128||
       c.machines.size()>100||c.tutors.size()>128)return false;
    std::uint8_t last=0;
    for(const auto offer:c.offers) {
        if(!offer.level||offer.level>100||!offer.move||offer.move>467||offer.level<last)
            return false;
        last=offer.level;
    }
    for(const auto m:c.machines)if(!m.number||!m.move||m.move>467||
        m.number>(m.hm?8:92))return false;
    for(const auto m:c.tutors)if(!m||m>467)return false;
    return true;
}
[[nodiscard]] constexpr std::uint16_t afterLevel(const Catalog& c,std::uint8_t level) noexcept {
    std::uint16_t index=0;
    while(index<c.offers.size() && c.offers[index].level<=level)++index;
    return index;
}
// Preflight before any path contradiction: a later unknown source method,
// unsupported cartridge, or lost receipt makes the entire asserted path Unknown.
[[nodiscard]] constexpr bool preflight(const Input& in,
                                       std::span<const Catalog> catalogs) noexcept {
    if(!in.transferredCandidateId||!in.species||in.species>386||
       !validGame(in.arrivalGame)||!validGame(in.observedGame)||
       !in.arrivalLevel||in.arrivalLevel>100||
       in.observedLevel<in.arrivalLevel||in.observedLevel>100||
       in.stepCount>in.steps.size()||
       in.sourceTmReceiptCount>in.sourceTmReceipts.size()||
       !in.witnessedSameIndividualAtTransfer||!in.witnessedOutboundSnapshot||
       !in.witnessedImmediateArrivalSnapshot||!in.witnessedWholeNativeActionHistory||
       !structured(in.outboundGba,354)||!structured(in.immediateNds,467)||
       !structured(in.observedMoves,467)||
       !in.gba.phaseCount||in.gba.phaseCount>in.gba.phases.size())return false;
    // Deoxys and other alternate forms cannot be silently normalized.
    if(!in.formContinuityVerified||in.outboundForm!=in.immediateForm||
       in.outboundForm!=0||in.immediateForm!=0)return false;
    if(!X::preflight(in.gba))return false;
    const auto& last=in.gba.phases[in.gba.phaseCount-1].history;
    if(last.candidateId!=in.transferredCandidateId||last.species!=in.species||
       last.observedLevel!=in.arrivalLevel||
       last.observedMoves!=in.outboundGba)return false;
    // Original TM is single-use. An independently witnessed numbered item
    // must belong to THIS phase, game, species, move and TM number. A move
    // pool, a remembered GBA origin flag, or another cartridge's item does
    // not establish a consumed original TM. One receipt cannot cover two TMs.
    std::array<bool,64> usedReceipts{};
    for(std::uint8_t i=0;i<in.gba.phaseCount;++i) {
        const auto& phase=in.gba.phases[i].history;
        for(std::uint8_t j=0;j<phase.stepCount;++j) {
            const auto& action=phase.steps[j];
            if(action.action!=H::Action::TeachTM)continue;
            unsigned matches=0;
            for(std::uint8_t k=0;k<in.sourceTmReceiptCount;++k) {
                const auto& r=in.sourceTmReceipts[k];
                if(r.phaseIndex!=i||r.stepIndex!=j)continue;
                ++matches;
                if(!r.originalSourceAndItemVerified||!r.singleUseItemReceipt||
                   r.game!=phase.game||r.species!=phase.species||
                   r.move!=action.move||r.machineNumber!=action.machineNumber)
                    return false;
                usedReceipts[k]=true;
            }
            if(matches!=1)return false;
        }
    }
    for(std::uint8_t k=0;k<in.sourceTmReceiptCount;++k) {
        if(!usedReceipts[k])return false;
        for(std::uint8_t m=0;m<k;++m)
            if(in.sourceTmReceipts[k].singleUseItemReceipt==
               in.sourceTmReceipts[m].singleUseItemReceipt)return false;
    }
    Game current=in.arrivalGame;
    std::uint8_t level=in.arrivalLevel;
    // Catalogs are source facts, not certificates for the trainer's decisions.
    // Source completeness is needed for level changes even with no moves learned.
    for(std::uint8_t i=0;i<in.stepCount;++i) {
        const auto& s=in.steps[i];
        if(!s.exactActionVerified||!s.methodSourceVerified||
           !validGame(s.game)||s.level<level||s.level>in.observedLevel||
           s.action==Action::Unsupported)return false;
        const Catalog* c=findCatalog(catalogs,current,in.species);
        if(s.level>level && (!c||!validCatalog(*c)))return false;
        if(s.action==Action::VisitGame) {
            if(s.game==current||s.move||s.slot||s.number||s.independentReceipt)
                return false;
            current=s.game;
            level=s.level;
            continue;
        }
        if(s.game!=current)return false;
        if(!c||!validCatalog(*c))return false;
        const bool teaching=s.action==Action::TeachTM||s.action==Action::TeachHM||
                            s.action==Action::Tutor||s.action==Action::Reminder;
        const bool offer=s.action==Action::OfferLearn||s.action==Action::OfferDecline||
                         s.action==Action::OfferKnown;
        if(teaching||offer) {
            if(!s.move||s.move>467||s.slot>=4)return false;
        } else if(s.action==Action::DeleteMove) {
            if(s.move||s.slot>=4||s.number||s.independentReceipt)return false;
        } else return false;
        if(offer && (s.number||s.independentReceipt))return false;
        if(s.action==Action::TeachTM||s.action==Action::TeachHM) {
            if(!c->listedNumberedMachineMethodsVerified||!s.number||
               s.number>(s.action==Action::TeachTM?92:8)||
               (s.action==Action::TeachTM && !s.independentReceipt))return false;
        } else if(s.action==Action::Tutor) {
            if(!c->fullTutorSourceVerified||s.number||!s.independentReceipt)return false;
        } else if(s.action==Action::Reminder) {
            if(s.number||!s.independentReceipt)return false;
        }
        level=s.level;
    }
    const auto* c=findCatalog(catalogs,current,in.species);
    if(in.observedLevel>level && (!c||!validCatalog(*c)))return false;
    return true;
}
[[nodiscard]] constexpr Result evaluate(const Input& in,
                                        std::span<const Catalog> catalogs) noexcept {
    Result out{};
    if(!preflight(in,catalogs))return out;
    const auto before=X::evaluate(in.gba);
    if(before.status==H::Status::Unknown)return out;
    if(before.status==H::Status::CandidateContradicted) {
        out.status=Status::CandidateContradicted;return out;
    }
    out.sourcePhases=before.phases;
    if(before.observedMoves!=in.outboundGba) {
        out.status=Status::CandidateContradicted;return out;
    }
    for(const auto move:in.outboundGba)if(oldHM(move)) {
        out.status=Status::CandidateContradicted;return out;
    }
    if(in.outboundGba!=in.immediateNds) {
        out.status=Status::CandidateContradicted;return out;
    }
    Game game=in.arrivalGame;
    std::uint8_t level=in.arrivalLevel;
    out.rebuilt=in.immediateNds;
    const Catalog* cat=findCatalog(catalogs,game,in.species);
    std::uint16_t next=cat?afterLevel(*cat,level):0;
    std::array<std::uint32_t,64> usedItems{};
    std::uint8_t usedCount=0;
    for(std::uint8_t i=0;i<in.stepCount;++i) {
        const auto& s=in.steps[i];
        if(s.level<level)return out;
        cat=findCatalog(catalogs,game,in.species);
        if(s.level>level && (!cat||!validCatalog(*cat)))return out;
        const bool offer=s.action==Action::OfferLearn||s.action==Action::OfferDecline||
                         s.action==Action::OfferKnown;
        if(offer) {
            if(!cat||next>=cat->offers.size()||
               cat->offers[next].level!=s.level||cat->offers[next].move!=s.move){
                out.status=Status::CandidateContradicted;return out;
            }
            const auto empty=firstFree(out.rebuilt);
            const bool known=contains(out.rebuilt,s.move);
            if(s.action==Action::OfferKnown) {
                if(!known){out.status=Status::CandidateContradicted;return out;}
            } else if(s.action==Action::OfferDecline) {
                if(known||empty<4){out.status=Status::CandidateContradicted;return out;}
            } else {
                if(known||s.slot>=4||(empty<4&&s.slot!=empty)) {
                    out.status=Status::CandidateContradicted;return out;
                }
                out.rebuilt[s.slot]=s.move;
            }
            ++next;++out.offers;
        } else {
            if(cat && next<cat->offers.size() && cat->offers[next].level<=s.level) {
                out.status=Status::CandidateContradicted;return out;
            }
            if(s.action==Action::VisitGame) {
                game=s.game;
                cat=findCatalog(catalogs,game,in.species);
                next=cat?afterLevel(*cat,s.level):0;
                ++out.visits;
            } else if(s.action==Action::DeleteMove) {
                if(s.slot>=4||!out.rebuilt[s.slot]||!out.rebuilt[1]){
                    out.status=Status::CandidateContradicted;return out;
                }
                for(std::uint8_t k=s.slot;k<3;++k)out.rebuilt[k]=out.rebuilt[k+1];
                out.rebuilt[3]=0;++out.deletions;
            } else {
                if(contains(out.rebuilt,s.move)||s.slot>=4) {
                    out.status=Status::CandidateContradicted;return out;
                }
                if(s.action==Action::TeachTM||s.action==Action::TeachHM) {
                    bool supported=false,known=false;
                    for(const auto m:cat->machines) {
                        if(m.hm!=(s.action==Action::TeachHM)||m.number!=s.number)
                            continue;
                        known=true;
                        if(m.move==s.move && m.compatible)supported=true;
                    }
                    if(!known)return out; // no source rule, never a hard negative
                    if(!supported){out.status=Status::CandidateContradicted;return out;}
                    if(s.action==Action::TeachTM) {
                        for(std::uint8_t j=0;j<usedCount;++j)
                            if(usedItems[j]==s.independentReceipt) {
                                out.status=Status::CandidateContradicted;return out;
                            }
                        usedItems[usedCount++]=s.independentReceipt;
                    }
                    ++out.machines;
                } else if(s.action==Action::Tutor) {
                    bool known=false;
                    for(const auto m:cat->tutors)if(m==s.move)known=true;
                    if(!known){out.status=Status::CandidateContradicted;return out;}
                    ++out.tutors;
                } else if(s.action==Action::Reminder) {
                    bool known=false;
                    for(const auto offer:cat->offers)
                        if(offer.move==s.move&&offer.level<=s.level)known=true;
                    if(!known){out.status=Status::CandidateContradicted;return out;}
                    ++out.reminders;
                } else return out;
                const auto empty=firstFree(out.rebuilt);
                if(empty<4 && s.slot!=empty) {
                    out.status=Status::CandidateContradicted;return out;
                }
                out.rebuilt[s.slot]=s.move;
            }
        }
        level=s.level;
    }
    cat=findCatalog(catalogs,game,in.species);
    if(cat && next<cat->offers.size() && cat->offers[next].level<=in.observedLevel) {
        out.status=Status::CandidateContradicted;return out;
    }
    out.finalGame=game;
    out.status=game==in.observedGame&&out.rebuilt==in.observedMoves?
        Status::CandidateConsistent:Status::CandidateContradicted;
    return out;
}
struct Alternatives {
    std::array<Input,8> candidates{};
    std::uint8_t count=0;
    bool allActualRoutesAttestedAndEnumerated=false;
};
[[nodiscard]] constexpr Status evaluate(const Alternatives& paths,
                                        std::span<const Catalog> catalogs) noexcept {
    if(!paths.count||paths.count>paths.candidates.size())return Status::Unknown;
    const auto id=paths.candidates[0].transferredCandidateId;
    if(!id)return Status::Unknown;
    bool unknown=false;
    for(std::uint8_t i=0;i<paths.count;++i) {
        if(paths.candidates[i].transferredCandidateId!=id)return Status::Unknown;
        const auto status=evaluate(paths.candidates[i],catalogs).status;
        if(status==Status::CandidateConsistent)return status;
        if(status==Status::Unknown)unknown=true;
    }
    return unknown||!paths.allActualRoutesAttestedAndEnumerated?
        Status::Unknown:Status::CandidateContradicted;
}
static_assert(oldHM(15)&&oldHM(19)&&oldHM(57)&&oldHM(70)&&
              oldHM(148)&&oldHM(249)&&oldHM(127)&&oldHM(291));
static_assert(!oldHM(92)&&!oldHM(237)&&!oldHM(412));
} // namespace Legality::Gen34PalParkMultiCartridgeCore
