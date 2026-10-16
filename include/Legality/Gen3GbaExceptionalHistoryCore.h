#pragma once
// L3 Stage 4: independently attested single-history GBA species transitions.
// This is an informational candidate verifier, NOT retail history discovery.
// Nothing here creates item/tutor/egg receipts, changes saves, or yields Invalid.
#include "Legality/Gen3GbaJointMethodHistoryCore.h"
#include <array>
#include <cstddef>
#include <cstdint>

namespace Legality::Gen3GbaExceptionalHistoryCore {
namespace H = Gen3GbaJointMethodHistoryCore;
using Game = H::Game;
using Status = H::Status;
enum class EdgeKind : std::uint8_t { Evolve, CartridgeVisit, ShedinjaSibling };
enum class OfferChoice : std::uint8_t { Learn, Decline, AlreadyKnown };
struct Decision {
    std::uint8_t level=0;
    std::uint16_t move=0;
    OfferChoice choice=OfferChoice::Learn;
    std::uint8_t slot=0;
    bool choiceAuthenticated=false;
};
struct Phase {
    H::Catalog catalog{};
    H::History history{};
};
struct Edge {
    EdgeKind kind=EdgeKind::Evolve;
    std::uint16_t fromSpecies=0, toSpecies=0;
    Game fromGame=Game::Unknown, toGame=Game::Unknown;
    std::uint8_t level=0;
    std::uint32_t fromCandidate=0, toCandidate=0;
    bool originalTransitionAttested=false;
    bool samePokemonAttested=false;
    bool moveSlotsImmutableAcrossEdgeAttested=false;
    bool allImmediateOffersAttested=false;
    // Shedinja is a separately created sibling, NEVER the same Pokemon.
    bool nincadaToNinjaskAlreadyAttested=false;
    bool partyCapacityAndPokeballAttested=false;
    bool copiedAfterNinjaskOffersAttested=false;
    std::array<Decision,16> immediateOffers{};
    std::uint8_t offerCount=0;
};
struct Witness {
    std::array<Phase,5> phases{};
    std::array<Edge,4> edges{};
    std::uint8_t phaseCount=0;
    bool entireClaimedHistoryAttested=false;
};
struct Result {
    Status status=Status::Unknown;
    std::uint8_t phases=0, evolutions=0, visits=0, siblings=0;
    std::uint16_t immediateOffers=0;
    std::array<std::uint16_t,4> observedMoves{};
    bool globalInvalidAuthorized=false;
};
[[nodiscard]] constexpr std::uint8_t minimumLevel(std::uint16_t before,
                                                  std::uint16_t after) noexcept {
    if(before==1&&after==2)return 16;
    if(before==2&&after==3)return 32;
    if(before==4&&after==5)return 16;
    if(before==5&&after==6)return 36;
    if(before==7&&after==8)return 16;
    if(before==8&&after==9)return 36;
    if(before==290&&after==291)return 20;
    return 0; // Unmodeled evolution source, not proof of impossible evolution.
}
[[nodiscard]] constexpr bool preflight(const Witness& w) noexcept {
    if(!w.entireClaimedHistoryAttested||w.phaseCount==0||
       w.phaseCount>w.phases.size())return false;
    for(std::uint8_t i=0;i<w.phaseCount;++i) {
        const auto& p=w.phases[i];
        const auto& h=p.history;
        if(!H::pinnedCatalog(p.catalog)||!h.candidateId||
           h.game!=p.catalog.game||h.species!=p.catalog.species||
           !h.initialSnapshotVerified||!h.samePokemonAndGameVerified||
           !h.allPlayerActionsVerified||h.stepCount>h.steps.size()||
           !H::structural(h.initialMoves)||!H::structural(h.observedMoves)||
           h.initialLevel==0||h.observedLevel<h.initialLevel||
           h.observedLevel>100)return false;
        // Unknown service, tutors, eggs, events and erased receipts require a
        // different source-owning verifier. Preflight before any contradiction.
        for(std::uint8_t j=0;j<h.stepCount;++j) {
            const auto& s=h.steps[j];
            if(!s.originalActionReceiptVerified||s.action==H::Action::UnknownService||
               s.game!=h.game||s.level<h.initialLevel||
               s.level>h.observedLevel)return false;
        }
        if(i==0)continue;
        const auto& e=w.edges[i-1];
        const auto& prev=w.phases[i-1].history;
        if(!e.originalTransitionAttested ||
           !e.moveSlotsImmutableAcrossEdgeAttested ||
           e.fromSpecies!=prev.species ||e.toSpecies!=h.species ||
           e.fromGame!=prev.game||e.toGame!=h.game||
           e.level!=prev.observedLevel||e.level!=h.initialLevel||
           !e.fromCandidate||!e.toCandidate ||
           e.fromCandidate!=prev.candidateId||e.toCandidate!=h.candidateId||
           e.offerCount>e.immediateOffers.size())return false;
        if(e.kind==EdgeKind::Evolve) {
            if(!e.samePokemonAttested ||e.fromCandidate!=e.toCandidate||
               e.fromGame!=e.toGame||
               !minimumLevel(e.fromSpecies,e.toSpecies)||
               e.level<minimumLevel(e.fromSpecies,e.toSpecies)||
               !e.allImmediateOffersAttested)return false;
        } else if(e.kind==EdgeKind::CartridgeVisit) {
            if(!e.samePokemonAttested||e.fromCandidate!=e.toCandidate||
               e.fromSpecies!=e.toSpecies||e.fromGame==e.toGame||
               e.offerCount)return false;
        } else if(e.kind==EdgeKind::ShedinjaSibling) {
            if(e.samePokemonAttested||e.fromCandidate==e.toCandidate||
               e.fromSpecies!=291||e.toSpecies!=292||
               e.fromGame!=e.toGame||e.offerCount||
               !e.nincadaToNinjaskAlreadyAttested||
               !e.partyCapacityAndPokeballAttested||
               !e.copiedAfterNinjaskOffersAttested)return false;
        } else return false;
        for(std::uint8_t j=0;j<e.offerCount;++j) {
            const auto& o=e.immediateOffers[j];
            if(!o.choiceAuthenticated||o.level!=e.level||
               !o.move||o.move>354||o.slot>=4)return false;
        }
    }
    return true;
}
[[nodiscard]] constexpr Result evaluate(const Witness& w) noexcept {
    Result out{};
    if(!preflight(w))return out;
    bool siblingSeen=false;
    for(std::uint8_t i=0;i<w.phaseCount;++i) {
        const auto& phase=w.phases[i];
        if(i) {
            const auto& e=w.edges[i-1];
            const auto& next=phase.history;
            std::array<std::uint16_t,4> rebuilt=out.observedMoves;
            if(e.kind==EdgeKind::Evolve) {
                // The evolved-species offers at EXACT evolution level happen
                // after species change, before subsequent phases, in source order.
                std::uint8_t seen=0;
                for(const auto& offer:phase.catalog.offers) {
                    if(offer.level!=e.level)continue;
                    if(seen>=e.offerCount)return out; // absent decision: Unknown
                    const auto d=e.immediateOffers[seen++];
                    if(d.move!=offer.move||d.level!=offer.level) {
                        out.status=Status::CandidateContradicted;return out;
                    }
                    const auto open=H::firstFree(rebuilt);
                    const bool known=H::contains(rebuilt,d.move);
                    if(d.choice==OfferChoice::AlreadyKnown) {
                        if(!known){out.status=Status::CandidateContradicted;return out;}
                    } else if(d.choice==OfferChoice::Decline) {
                        if(known||open<4) {out.status=Status::CandidateContradicted;return out;}
                    } else if(d.choice==OfferChoice::Learn) {
                        if(known||d.slot>=4||(open<4&&d.slot!=open)) {
                            out.status=Status::CandidateContradicted;return out;
                        }
                        rebuilt[d.slot]=d.move;
                    } else return out;
                }
                if(seen!=e.offerCount)return out;
                out.immediateOffers+=seen;
                ++out.evolutions;
            } else if(e.kind==EdgeKind::CartridgeVisit) {
                // Exact same Pokemon and unchanged slots; game is a *real*
                // claimed visit, not a broad union of game move pools.
                ++out.visits;
            } else if(e.kind==EdgeKind::ShedinjaSibling) {
                if(siblingSeen || i<2 || w.edges[i-2].kind!=EdgeKind::Evolve ||
                   w.edges[i-2].fromSpecies!=290 ||
                   w.edges[i-2].toSpecies!=291 ||
                   e.level!=w.edges[i-2].level)return out;
                siblingSeen=true;++out.siblings;
                // Sibling slots are copied AFTER Ninjask's level-20 offers;
                // this does not extend the original Nincada's identity.
            }
            if(rebuilt!=next.initialMoves) {
                out.status=Status::CandidateContradicted;return out;
            }
        }
        const auto stage=H::evaluate(phase.catalog,phase.history);
        if(stage.status!=Status::CandidateConsistent) {
            out.status=stage.status;return out;
        }
        out.observedMoves=stage.rebuilt;
        ++out.phases;
    }
    out.status=Status::CandidateConsistent;
    return out;
}
// A same-Pokemon candidate set cannot mix a Shedinja sibling's distinct ID
// with a Ninjask's ID, or turn one failed hypothetical path into Invalid.
struct Alternatives {
    std::array<Witness,4> candidates{};
    std::uint8_t count=0;
    bool allGenuineHistoriesEnumerated=false;
};
[[nodiscard]] constexpr Status evaluate(const Alternatives& a) noexcept {
    if(!a.count||a.count>a.candidates.size())return Status::Unknown;
    bool unknown=false;
    const auto originId=a.candidates[0].phases[0].history.candidateId;
    if(!originId)return Status::Unknown;
    for(std::uint8_t i=0;i<a.count;++i) {
        if(a.candidates[i].phaseCount==0||
           a.candidates[i].phases[0].history.candidateId!=originId)
            return Status::Unknown;
        const auto status=evaluate(a.candidates[i]).status;
        if(status==Status::CandidateConsistent)return status;
        if(status==Status::Unknown)unknown=true;
    }
    return unknown||!a.allGenuineHistoriesEnumerated?
        Status::Unknown:Status::CandidateContradicted;
}
static_assert(minimumLevel(1,2)==16);
static_assert(minimumLevel(290,291)==20);
static_assert(minimumLevel(290,292)==0);
} // namespace Legality::Gen3GbaExceptionalHistoryCore
