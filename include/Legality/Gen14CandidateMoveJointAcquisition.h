#pragma once
// L3 M06/M07/M12: verify ONE full same-species, same-cartridge history
// against source-ordered level-up *and* numbered machine acquisitions.
// Four individually obtainable moves do not prove one feasible sequence.
// Positive/contradicted results refer solely to this witnessed candidate;
// unmodeled egg, event, tutor, reminder, evolution, trade or Gen IV routes
// remain UNRESOLVED, never a global Invalid judgment.
// Sources: pinned pokeruby@63a8cbf, pokeemerald@5eff786,
// pokefirered@c75f352; accepted Gen3EmeraldLevelUpData.inc.
#include "Legality/Gen14CandidateMoveGbaLevelUpTimeline.h"
#include "Legality/Gen14CandidateMoveMachineEvidence.h"
#include <array>
#include <cstdint>

namespace Legality::Gen14CandidateMoveJointAcquisition {
namespace C=Gen14CandidateMoveChronology;
namespace L=Gen3GbaVersionedLevelUpTimeline;
namespace M=Gen14CandidateMoveMachineEvidence;
using Status=C::Status;
struct MachineWitness {
    uint8_t stepIndex=0;
    M::Stage numberedItem{};
};
struct Input {
    C::Path path{};
    // The witness is for an uninterrupted species/game/level-up stage;
    // any game visit, evolution, egg, event or unrecorded service calls
    // must be analyzed as a separate candidate, never silently excluded.
    bool uninterruptedSameSpeciesGameStageVerified=false;
    bool allOfferedLevelUpDecisionsRecorded=false;
    std::array<MachineWitness,32> machines{};
    uint8_t machineCount=0;
};
struct Result {
    Status status=Status::Unresolved;
    uint8_t requiredOffers=0, verifiedMachines=0;
    C::Result fourSlot{};
};
constexpr Result evaluate(const Input& in) noexcept {
    Result out{};
    const auto& p=in.path;
    if(C::generation(p.origin)!=3 || !p.species || p.species>386 ||
       p.stepCount>p.steps.size() || in.machineCount>in.machines.size())
        return out;
    if(!in.uninterruptedSameSpeciesGameStageVerified ||
       !in.allOfferedLevelUpDecisionsRecorded ||
       !L::supported(p.origin,p.species))return out;
    // Unsupported alternative route types MUST be classified before the
    // generic four-slot replay. Otherwise an unrelated tentative trade,
    // Pal Park, reminder, tutor or move deletion could generate a local
    // contradiction which this narrower same-game verifier has no
    // source authority to assert. These paths require their own packets.
    for(uint8_t i=0;i<p.stepCount;++i) {
        const auto& step=p.steps[i];
        if(step.action==C::Action::VisitGame ||
           step.action==C::Action::PalPark ||
           step.action==C::Action::DeleteMove ||
           (step.action==C::Action::Learn &&
            step.method!=C::Method::LevelUp &&
            step.method!=C::Method::TM &&
            step.method!=C::Method::HM))
            return out;
    }
    // Stage evidence is checked FIRST. Never convert absent source into
    // a contradiction even if a different guessed history fails.
    out.fourSlot=C::evaluate(p);
    if(out.fourSlot.status!=Status::CandidateConsistent) {
        out.status=out.fourSlot.status;return out;
    }
    uint16_t next=0;
    const uint16_t count=L::offerCount(p.origin,p.species);
    while(next<count && L::offerAt(p.origin,p.species,next).level<=p.initialLevel)
        ++next;
    std::array<bool,32> used{};
    for(uint8_t i=0;i<p.stepCount;++i) {
        const auto& step=p.steps[i];
        // Trades, Pal Park, evolving, egg inheritance and event routes need
        // separate joint witnesses; absence of a modeled edge is Unknown.
        if(step.action==C::Action::VisitGame ||
           step.action==C::Action::PalPark)return out;
        if(step.game!=p.origin)return out;
        const bool levelOffer=step.method==C::Method::LevelUp &&
            (step.action==C::Action::Learn ||
             step.action==C::Action::Decline ||
             step.action==C::Action::AlreadyKnown);
        if(levelOffer) {
            if(next>=count) {
                out.status=Status::CandidateContradicted;return out;
            }
            const auto source=L::offerAt(p.origin,p.species,next);
            // Each evolution-stage offer is indivisible and ordered;
            // a fake lv45 SolarBeam or a skipped lv7 Leech Seed can
            // look compatible in a species-wide single-move pool.
            if(source.level!=step.level || source.move!=step.move) {
                out.status=Status::CandidateContradicted;return out;
            }
            ++next;++out.requiredOffers;continue;
        }
        // A source level-up offer at/before the present stage cannot be
        // silently bypassed by subsequently teaching a machine move.
        if(next<count && L::offerAt(p.origin,p.species,next).level<=step.level) {
            out.status=Status::CandidateContradicted;return out;
        }
        if(step.action!=C::Action::Learn ||
           (step.method!=C::Method::TM && step.method!=C::Method::HM))
            return out; // Reminder, tutor, deletion: other evidence packets
        uint8_t matching=0;
        for(uint8_t j=0;j<in.machineCount;++j) {
            if(in.machines[j].stepIndex!=i)continue;
            ++matching;used[j]=true;
            // A source-backed TM/HM row is species-specific evidence.
            // Matching the item and move is not sufficient if its
            // independently attested species differs from this Pokemon.
            // Do not transmute Bulbasaur's pinned machines into proof for
            // a Charmander, traded sibling or evolved-stage specimen.
            if(in.machines[j].numberedItem.nationalSpecies!=p.species)
                return out; // Inconsistent witness => Unknown.
            const auto item=M::classifyChronologyStep(
                in.machines[j].numberedItem,step);
            if(item==M::Status::CandidateContradicted) {
                out.status=Status::CandidateContradicted;return out;
            }
            if(item!=M::Status::CandidateCompatible)return out;
        }
        if(matching!=1)return out;
        ++out.verifiedMachines;
    }
    // No unrecorded final offer, and no extra/cross-candidate item receipt
    // used to manufacture joint method evidence.
    if(next<count && L::offerAt(p.origin,p.species,next).level<=p.observedLevel) {
        out.status=Status::CandidateContradicted;return out;
    }
    for(uint8_t i=0;i<in.machineCount;++i)if(!used[i])return out;
    out.status=Status::CandidateConsistent;
    return out;
}
static_assert(L::offerAt(C::Game::Ruby,1,2).move==73);
static_assert(L::offerAt(C::Game::Ruby,1,3).move==22);
static_assert(L::offerAt(C::Game::Ruby,1,10).level==46);
static_assert(!L::supported(C::Game::Ruby,133));
// These source-scoped tests compile in the already registered host suite
// through the owned Gen14CandidateMoveHistory.h include path, independent
// of coordinator registration of the standalone L3 runner.
constexpr Input smoke(bool includeRequiredOffer) noexcept {
    Input in{};
    auto& p=in.path;
    p.candidateId=901;p.species=1;p.origin=C::Game::Ruby;
    p.initialLevel=5;p.observedLevel=7;
    p.initialMoves={33,45,0,0};p.observedMoves=p.initialMoves;
    p.initialSnapshotVerified=true;p.samePokemonAndSpeciesVerified=true;
    p.fullChronologyVerified=true;
    if(includeRequiredOffer) {
        p.stepCount=1;p.observedMoves={33,45,73,0};
        p.steps[0]={C::Game::Ruby,7,C::Action::Learn,
                    C::Method::LevelUp,73,2,true,C::Game::Ruby};
    }
    in.uninterruptedSameSpeciesGameStageVerified=true;
    in.allOfferedLevelUpDecisionsRecorded=true;
    return in;
}
static_assert(C::evaluate(smoke(false).path).status==Status::CandidateConsistent);
static_assert(evaluate(smoke(false)).status==Status::CandidateContradicted);
static_assert(evaluate(smoke(true)).status==Status::CandidateConsistent);
static_assert([]{auto a=smoke(true);
    a.allOfferedLevelUpDecisionsRecorded=false;
    return evaluate(a).status==Status::Unresolved;}());
// Compile a real source-numbered TM into the SAME ordered history; moving
// that TM ahead of the mandatory level-up is not a valid alternative even
// though both final moves are individually permitted for Bulbasaur.
constexpr Input smokeInterleaved(bool skipLevelOffer) noexcept {
    Input a=smoke(true);auto& p=a.path;
    if(skipLevelOffer) {
        p.stepCount=1;
        p.observedMoves={33,45,92,0};
        p.steps[0]={C::Game::Ruby,7,C::Action::Learn,
                    C::Method::TM,92,2,true,C::Game::Ruby};
        a.machines[0]={0,{C::Game::Ruby,1,M::Machine::TM,6,92,true,true}};
    } else {
        p.stepCount=2;p.observedMoves={33,45,73,92};
        p.steps[1]={C::Game::Ruby,7,C::Action::Learn,
                    C::Method::TM,92,3,true,C::Game::Ruby};
        a.machines[0]={1,{C::Game::Ruby,1,M::Machine::TM,6,92,true,true}};
    }
    a.machineCount=1;return a;
}
static_assert(evaluate(smokeInterleaved(false)).status==Status::CandidateConsistent);
static_assert(C::evaluate(smokeInterleaved(true).path).status==Status::CandidateConsistent);
static_assert(evaluate(smokeInterleaved(true)).status==Status::CandidateContradicted);
static_assert([]{auto a=smokeInterleaved(false);
    a.machines[0].numberedItem.itemIdentityVerified=false;
    return evaluate(a).status==Status::Unresolved;}());

// Regression: a Bulbasaur retail machine row must not "verify" another
// Pokemon's identical item/move/level teaching. The generic slot replay
// succeeds, but the species-bound source record is not this species.
static_assert([]{auto a=smokeInterleaved(false);
    auto& p=a.path;
    p.species=4;p.initialLevel=70;p.observedLevel=70;
    p.initialMoves={10,45,52,108};
    p.observedMoves={10,45,92,108};
    p.stepCount=1;
    p.steps[0]={C::Game::Ruby,70,C::Action::Learn,
                C::Method::TM,92,2,true,C::Game::Ruby};
    a.machines[0].stepIndex=0;
    return C::evaluate(p).status==Status::CandidateConsistent &&
           evaluate(a).status==Status::Unresolved;
}());


} // namespace Legality::Gen14CandidateMoveJointAcquisition
