#pragma once
// L3 M07/M11: machine-item identity must be bound to an actual source game,
// not inferred from a species' broad editor move compatibility pool.
// Source-verified original 386-species rows across all five GBA games. Pinned originals:
// pokeruby@63a8cbf, pokeemerald@5eff786, pokefirered@c75f352
//   src/data/pokemon/tmhm_learnsets.h and include/constants/moves.h
// Unknown machine/species/Gen IV routes remain Unresolved; all negative
// outcomes are candidate-local claims about a specified item, not a Pokemon.
#include "Legality/Gen14CandidateMoveChronology.h"
#include "Legality/Gen3GbaOriginalMachineData.h"
#include <cstdint>
namespace Legality::Gen14CandidateMoveMachineEvidence {
namespace C=Gen14CandidateMoveChronology;
enum class Machine : uint8_t { TM, HM };
enum class Status : uint8_t {
    Unresolved, CandidateCompatible, CandidateContradicted
};
struct Stage {
    C::Game game=C::Game::Unknown;
    uint16_t nationalSpecies=0;
    Machine machine=Machine::TM;
    uint8_t machineNumber=0;
    uint16_t move=0;
    // Verified source and used-item identity must be *supplied*; a PK3
    // does not record its original TM/HM teaching transcript.
    bool itemIdentityVerified=false;
    bool samePokemonAndGameVerified=false;
};
namespace Data=Gen3GbaOriginalMachineData;
constexpr Status classify(const Stage& s) noexcept {
    if(C::generation(s.game)!=3 || s.nationalSpecies<1 ||
       s.nationalSpecies>386 || !s.move || s.move>354 ||
       !s.itemIdentityVerified || !s.samePokemonAndGameVerified)
        return Status::Unresolved;
    uint8_t bit=0;
    if(s.machine==Machine::TM) {
        if(!s.machineNumber || s.machineNumber>50)return Status::Unresolved;
        bit=s.machineNumber-1;
    } else if(s.machine==Machine::HM) {
        if(!s.machineNumber || s.machineNumber>8)return Status::Unresolved;
        bit=s.machineNumber+49;
    } else return Status::Unresolved;
    const uint64_t mask=(s.game==C::Game::FireRed ||
                         s.game==C::Game::LeafGreen)
        ?Data::kFireRedLeafGreenMasks[s.nationalSpecies]
        :Data::kRubySapphireMasks[s.nationalSpecies];
    // A source-attested machine number is not interchangeable with an
    // arbitrary native move ID. This negative is scoped ONLY to the
    // claimed machine method and original species/version candidate.
    if(!(mask & (uint64_t{1}<<bit)) ||
       Data::kMachineMove[bit]!=s.move)
        return Status::CandidateContradicted;
    return Status::CandidateCompatible;
}
// The evidence of one machine-item step may corroborate a witness without
// creating history: the original candidate, its chronological order, moves
// in all four slots and any other game visits are still independently needed.
constexpr Status classifyChronologyStep(const Stage& s,
                                        const C::Step& step) noexcept {
    if(step.action!=C::Action::Learn || step.game!=s.game ||
       step.methodSource!=s.game || step.move!=s.move ||
       !step.evidenceVerified)return Status::Unresolved;
    if((s.machine==Machine::TM && step.method!=C::Method::TM) ||
       (s.machine==Machine::HM && step.method!=C::Method::HM))
        return Status::Unresolved;
    return classify(s);
}
static_assert(classify({C::Game::Ruby,1,Machine::TM,6,92,true,true})==
              Status::CandidateCompatible);
static_assert(classify({C::Game::LeafGreen,1,Machine::HM,1,15,true,true})==
              Status::CandidateCompatible);
static_assert(classify({C::Game::Emerald,1,Machine::TM,6,237,true,true})==
              Status::CandidateContradicted);
static_assert(classify({C::Game::Platinum,1,Machine::HM,1,15,true,true})==
              Status::Unresolved);
}
