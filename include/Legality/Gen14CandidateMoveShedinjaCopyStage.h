#pragma once
// L3 M08/M13: bounded source-witnessed Nincada -> Ninjask -> Shedinja
// four-slot copy, no inference from a detached PK3 or move family unions.
// Source versions: pret/pokeruby@63a8cbf, pokeemerald@5eff786,
// pokefirered@c75f352; src/evolution_scene.c CreateShedinja and
// evolve -> post-evolution MonTryLearningNewMove -> CreateShedinja order;
// src/data/pokemon/evolution.h EVO_LEVEL_NINJASK (20).
// A Shedinja is a newly created *sibling copy*, not the same Pokemon as
// Ninjask. Its subsequent level-up source is species 292, not species 291.
#include "Legality/Gen14CandidateMoveGbaLevelUpTimeline.h"
#include <array>
#include <cstdint>

namespace Legality::Gen14CandidateMoveShedinjaCopyStage {
namespace L=Gen3GbaVersionedLevelUpTimeline;
namespace C=Gen14CandidateMoveChronology;
using Game=C::Game;
enum class Status : uint8_t { Unresolved, CandidateConsistent, CandidateContradicted, Unsupported };
struct Choice {
    uint8_t level=0;
    uint16_t offeredMove=0;
    L::Decision decision=L::Decision::Decline;
    uint8_t slot=0;
};
struct Input {
    uint32_t originalNincadaCandidateId=0;
    Game game=Game::Unknown;
    L::Input originalNincada{};
    uint8_t evolutionLevel=0;
    bool originalNincadaToNinjaskEvolutionVerified=false;
    bool freePartySlotAtCreationVerified=false;
    bool allPostEvolutionNinjaskOffersVerified=false;
    bool createdAfterNinjaskMoveDecisionsVerified=false;
    bool originalShedinjaSiblingCopyVerified=false;
    std::array<Choice,8> evolutionDecisions{};
    uint8_t evolutionDecisionCount=0;
    std::array<uint16_t,4> verifiedNinjaskAfterEvolution{};
    std::array<uint16_t,4> verifiedShedinjaAtCreation{};
    L::Input laterShedinja{};
};
struct Result {
    Status status=Status::Unresolved;
    uint8_t evolutionOffers=0,learned=0,declined=0,alreadyKnown=0;
    std::array<uint16_t,4> ninjaskAfterEvolution{},shedinjaAtCreation{},observedShedinja{};
};
constexpr Result evaluate(const Input& in) noexcept {
    Result out{};
    if(!in.originalNincadaCandidateId || C::generation(in.game)!=3 ||
       in.evolutionLevel<20 || in.evolutionLevel>100 ||
       in.originalNincada.game!=in.game ||
       in.originalNincada.nationalSpecies!=290 ||
       in.originalNincada.observedLevel!=in.evolutionLevel) {
        out.status=Status::Unsupported;return out;
    }
    if(!in.originalNincadaToNinjaskEvolutionVerified ||
       !in.freePartySlotAtCreationVerified ||
       !in.allPostEvolutionNinjaskOffersVerified ||
       !in.createdAfterNinjaskMoveDecisionsVerified ||
       !in.originalShedinjaSiblingCopyVerified ||
       in.evolutionDecisionCount>in.evolutionDecisions.size())
        return out;
    const auto original=L::evaluate(in.originalNincada);
    if(original.status==L::Status::CandidateContradicted) {
        out.status=Status::CandidateContradicted;return out;
    }
    if(original.status!=L::Status::CandidateConsistent)return out;
    out.ninjaskAfterEvolution=original.rebuilt;
    // The source copy occurs at the END of the evolution scene, after
    // ALL Ninjask offers for the exact evolution level. Later Ninjask
    // offers (38 Agility, 45 Baton Pass) cannot be pulled in from another
    // stage or added via a union of unrelated sibling histories.
    for(uint16_t i=0;i<L::offerCount(in.game,291);++i) {
        const auto offer=L::offerAt(in.game,291,i);
        if(offer.level!=in.evolutionLevel)continue;
        if(out.evolutionOffers>=in.evolutionDecisionCount ||
           out.evolutionOffers>=in.evolutionDecisions.size())return out;
        const Choice step=in.evolutionDecisions[out.evolutionOffers++];
        if(step.level!=offer.level || step.offeredMove!=offer.move) {
            out.status=Status::CandidateContradicted;return out;
        }
        const bool known=C::contains(out.ninjaskAfterEvolution,offer.move);
        const uint8_t empty=C::freeSlot(out.ninjaskAfterEvolution);
        switch(step.decision) {
        case L::Decision::AlreadyKnown:
            if(!known){out.status=Status::CandidateContradicted;return out;}
            ++out.alreadyKnown;break;
        case L::Decision::AutoLearn:
            if(known || empty>=4 || step.slot!=empty) {
                out.status=Status::CandidateContradicted;return out;
            }
            out.ninjaskAfterEvolution[empty]=offer.move;++out.learned;break;
        case L::Decision::ReplaceSlot:
            if(known || empty<4 || step.slot>=4) {
                out.status=Status::CandidateContradicted;return out;
            }
            out.ninjaskAfterEvolution[step.slot]=offer.move;++out.learned;break;
        case L::Decision::Decline:
            if(known || empty<4) {
                out.status=Status::CandidateContradicted;return out;
            }
            ++out.declined;break;
        }
    }
    if(out.evolutionOffers!=in.evolutionDecisionCount)return out;
    if(out.ninjaskAfterEvolution!=in.verifiedNinjaskAfterEvolution ||
       out.ninjaskAfterEvolution!=in.verifiedShedinjaAtCreation) {
        out.status=Status::CandidateContradicted;return out;
    }
    out.shedinjaAtCreation=out.ninjaskAfterEvolution;
    if(in.laterShedinja.game!=in.game ||
       in.laterShedinja.nationalSpecies!=292 ||
       in.laterShedinja.initialLevel!=in.evolutionLevel ||
       in.laterShedinja.initialMoves!=out.shedinjaAtCreation)return out;
    const auto later=L::evaluate(in.laterShedinja);
    if(later.status==L::Status::CandidateContradicted) {
        out.status=Status::CandidateContradicted;return out;
    }
    if(later.status!=L::Status::CandidateConsistent)return out;
    out.observedShedinja=later.rebuilt;
    out.status=Status::CandidateConsistent;
    return out;
}
static_assert(L::offerCount(Game::Emerald,290)==10);
static_assert(L::offerCount(Game::Ruby,291)==15);
static_assert(L::offerCount(Game::FireRed,292)==10);
static_assert(L::offerAt(Game::Sapphire,291,8).move==104);
static_assert(L::offerAt(Game::LeafGreen,291,9).move==210);
static_assert(L::offerAt(Game::Emerald,291,14).move==226);
} // namespace Legality::Gen14CandidateMoveShedinjaCopyStage
