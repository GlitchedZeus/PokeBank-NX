#pragma once
// L3 M07/M11: machine-item identity must be bound to an actual source game,
// not inferred from a species' broad editor move compatibility pool.
// Source-verified Bulbasaur rows across all five GBA game identities. Pinned originals:
// pokeruby@63a8cbf, pokeemerald@5eff786, pokefirered@c75f352
//   src/data/pokemon/tmhm_learnsets.h and include/constants/moves.h
// Unknown machine/species/Gen IV routes remain Unresolved; all negative
// outcomes are candidate-local claims about a specified item, not a Pokemon.
#include "Legality/Gen14CandidateMoveChronology.h"
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
constexpr Status classify(const Stage& s) noexcept {
    if(C::generation(s.game)!=3 || s.nationalSpecies!=1 || !s.move ||
       !s.itemIdentityVerified || !s.samePokemonAndGameVerified)
        return Status::Unresolved;
    uint16_t expected=0;
    switch(s.machine) {
    case Machine::TM:
        switch(s.machineNumber) {
        case 6:expected=92;break;   // Toxic
        case 9:expected=331;break;  // Bullet Seed
        case 10:expected=237;break; // Hidden Power
        case 11:expected=241;break; // Sunny Day
        case 17:expected=182;break; // Protect
        case 19:expected=202;break; // Giga Drain
        case 21:expected=218;break; // Frustration
        case 22:expected=76;break;  // SolarBeam
        case 27:expected=216;break; // Return
        case 32:expected=104;break; // Double Team
        case 36:expected=188;break; // Sludge Bomb
        case 42:expected=263;break; // Facade
        case 43:expected=290;break; // Secret Power
        case 44:expected=156;break; // Rest
        case 45:expected=213;break; // Attract
        default:break;
        }
        break;
    case Machine::HM:
        switch(s.machineNumber) {
        case 1:expected=15;break;  // Cut
        case 4:expected=70;break;  // Strength
        case 5:expected=148;break; // Flash
        case 6:expected=249;break; // Rock Smash
        default:break;
        }
        break;
    default:return Status::Unresolved;
    }
    if(!expected)return Status::Unresolved;
    return expected==s.move?Status::CandidateCompatible:
                            Status::CandidateContradicted;
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
