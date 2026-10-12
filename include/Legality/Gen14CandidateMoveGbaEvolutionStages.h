#pragma once
// L3 M06/M13: read-only, candidate-local original-species transition chain.
// Source-pinned starter evolution levels, ALL original five GBA retail games:
// pokeruby@63a8cbf0016b351a4e68f7036fa0b77e23d2f2c1,
// pokeemerald@5eff78649e7170a877b961ef0b3da13b81a16038,
// pokefirered@c75f352304d529f6ba92d4f74b9cf8b5c3810788;
//   src/data/pokemon/evolution.h (EVO_LEVEL rows).
// Per-stage offers are independently checked by GbaLevelUpTimeline.
// An EVOLUTION is not a second encounter, fresh egg, or move reset.
// Historical evolution may occur after the minimum level (B cancellation).
#include "Legality/Gen14CandidateMoveGbaLevelUpTimeline.h"
#include <array>
#include <cstdint>

namespace Legality::Gen14CandidateMoveGbaEvolutionStages {
namespace L=Gen3GbaVersionedLevelUpTimeline;
namespace C=Gen14CandidateMoveChronology;
using Game=C::Game;
enum class Status : uint8_t {
    Unresolved, CandidateConsistent, CandidateContradicted, Unsupported
};
struct Evolution {
    uint16_t fromSpecies=0, toSpecies=0;
    uint8_t level=0;
    bool independentlyVerifiedSourceEvolution=false;
    bool noExtraMoveMutationOnEvolutionVerified=false;
};
struct Input {
    uint32_t candidateId=0;
    bool sameOriginalPokemonAcrossStagesVerified=false;
    bool exactEvolutionSequenceVerified=false;
    std::array<L::Input,3> phases{};
    std::array<Evolution,2> evolutions{};
    uint8_t phaseCount=0;
};
struct Result {
    Status status=Status::Unresolved;
    std::array<uint16_t,4> observedMoves{};
    uint8_t verifiedPhases=0, verifiedEvolutions=0;
    uint16_t finalSpecies=0;
};
constexpr uint8_t requiredMinimumLevel(uint16_t before,
                                       uint16_t after) noexcept {
    if(before==1 && after==2)return 16;
    if(before==2 && after==3)return 32;
    if(before==4 && after==5)return 16;
    if(before==5 && after==6)return 36;
    if(before==7 && after==8)return 16;
    if(before==8 && after==9)return 36;
    return 0;
}
constexpr bool offersAtLevel(Game game,uint16_t species,
                             uint8_t level) noexcept {
    for(uint16_t i=0;i<L::offerCount(game,species);++i)
        if(L::offerAt(game,species,i).level==level)return true;
    return false;
}
constexpr Result evaluate(const Input& in) noexcept {
    Result out{};
    if(!in.candidateId || !in.phaseCount ||
       in.phaseCount>in.phases.size())return out;
    if(!in.sameOriginalPokemonAcrossStagesVerified ||
       !in.exactEvolutionSequenceVerified)return out;
    const Game game=in.phases[0].game;
    if(C::generation(game)!=3) {
        out.status=Status::Unsupported;return out;
    }
    for(uint8_t i=0;i<in.phaseCount;++i) {
        const auto& phase=in.phases[i];
        if(phase.game!=game)return out; // Trade visits require another model.
        if(i) {
            const auto& prior=in.phases[i-1];
            const auto& edge=in.evolutions[i-1];
            const auto minimum=requiredMinimumLevel(
                prior.nationalSpecies,phase.nationalSpecies);
            if(!edge.independentlyVerifiedSourceEvolution ||
               !edge.noExtraMoveMutationOnEvolutionVerified ||
               !minimum || edge.fromSpecies!=prior.nationalSpecies ||
               edge.toSpecies!=phase.nationalSpecies ||
               edge.level!=prior.observedLevel ||
               edge.level!=phase.initialLevel ||
               edge.level<minimum ||
               phase.initialMoves!=out.observedMoves)return out;
            // The pinned *table* proves an offer exists for the target
            // at the exact evolution level, but not whether that offer
            // was actually prompted by the retail evolution service.
            // Do not silently skip an evolution-on-level move opportunity.
            if(offersAtLevel(game,phase.nationalSpecies,edge.level))
                return out;
            ++out.verifiedEvolutions;
        }
        const auto stage=L::evaluate(phase);
        if(stage.status==L::Status::CandidateContradicted) {
            out.status=Status::CandidateContradicted;return out;
        }
        if(stage.status!=L::Status::CandidateConsistent)return out;
        out.observedMoves=stage.rebuilt;
        out.finalSpecies=phase.nationalSpecies;
        ++out.verifiedPhases;
    }
    out.status=Status::CandidateConsistent;
    return out;
}
static_assert(requiredMinimumLevel(1,2)==16);
static_assert(requiredMinimumLevel(2,3)==32);
static_assert(requiredMinimumLevel(4,5)==16);
static_assert(requiredMinimumLevel(5,6)==36);
static_assert(requiredMinimumLevel(7,8)==16);
static_assert(requiredMinimumLevel(8,9)==36);
static_assert(requiredMinimumLevel(1,3)==0);
static_assert(offersAtLevel(Game::FireRed,6,36)); // Wing Attack at evolution
static_assert(!offersAtLevel(Game::Ruby,3,32)); // Venusaur no offer at 32
} // namespace Legality::Gen14CandidateMoveGbaEvolutionStages
