#pragma once
// L3 M07/M11/M12: a verified Pal Park boundary must join the *same* bounded
// source-Pokemon four-slot replay, original exact game, immediate-arrival
// snapshot, and separately cited teaching methods. No save writes or broad
// species-pool inference. A single contradicted candidate is never global
// illegality; incomplete original or native methods remain Unresolved.
#include "Legality/Gen14CandidateMoveChronology.h"
#include "Legality/Gen14CandidateMoveMachineEvidence.h"
#include "Legality/Gen34PalParkSnapshotTransition.h"
#include <array>
#include <cstdint>
#include <string_view>

namespace Legality::Gen14CandidateMovePalParkJointStage {
namespace C=Gen14CandidateMoveChronology;
namespace M=Gen14CandidateMoveMachineEvidence;
namespace P=Gen34PalParkSnapshotTransition;
using Game=C::Game;
enum class Status : uint8_t { Unresolved, CandidateConsistent, CandidateContradicted, Unsupported };
struct MachineWitness {
    uint8_t stepIndex=0;
    M::Stage exactItem{};
};
struct LaterMethodWitness {
    uint8_t stepIndex=0;
    Game sourceGame=Game::Unknown;
    C::Method method=C::Method::LevelUp;
    uint16_t move=0;
    uint8_t level=0;
    // A method-specific *original retail* packet, not a species pool.
    uint32_t independentSourceRecord=0;
    bool independentlyVerified=false;
};
struct Input {
    C::Path path{};
    P::Input crossing{};
    bool sameOriginalPokemonAcrossBothWitnessesVerified=false;
    std::array<MachineWitness,32> gbaMachines{};
    uint8_t gbaMachineCount=0;
    std::array<LaterMethodWitness,32> laterMethods{};
    uint8_t laterMethodCount=0;
};
struct Result {
    Status status=Status::Unresolved;
    C::Status history=C::Status::Unresolved;
    P::Status transfer=P::Status::UnknownStageWitness;
    uint8_t palParkIndex=0;
    uint8_t verifiedGbaMachines=0, verifiedNativeMethods=0;
    std::array<uint16_t,4> departureMoves{}, arrivalMoves{}, observedMoves{};
};
constexpr std::string_view gameName(Game game) noexcept {
    switch(game) {
    case Game::Ruby:return "ruby_gba";
    case Game::Sapphire:return "sapphire_gba";
    case Game::Emerald:return "emerald_gba";
    case Game::FireRed:return "firered_gba";
    case Game::LeafGreen:return "leafgreen_gba";
    case Game::Diamond:return "diamond_nds";
    case Game::Pearl:return "pearl_nds";
    case Game::Platinum:return "platinum_nds";
    case Game::HeartGold:return "heartgold_nds";
    case Game::SoulSilver:return "soulsilver_nds";
    default:return {};
    }
}
constexpr Result evaluate(const Input& in) noexcept {
    Result out{};
    if(C::generation(in.path.origin)!=3 || !in.path.candidateId ||
       !in.path.species || in.path.species>386 ||
       in.path.stepCount>in.path.steps.size() ||
       in.gbaMachineCount>in.gbaMachines.size() ||
       in.laterMethodCount>in.laterMethods.size())return out;
    if(!in.sameOriginalPokemonAcrossBothWitnessesVerified)return out;
    out.transfer=P::evaluate(in.crossing).status;
    if(out.transfer==P::Status::UnsupportedGames ||
       out.transfer==P::Status::MalformedCandidateSnapshot) {
        out.status=Status::Unsupported;return out;
    }
    if(out.transfer==P::Status::UnknownStageWitness)return out;
    if(out.transfer==P::Status::CandidateContradicted) {
        out.status=Status::CandidateContradicted;return out;
    }
    if(in.path.species!=in.crossing.species)return out;
    // Exactly one verified transfer edge. Path origin and departure are
    // distinct: a Gen III cartridge visit may precede Pal Park.
    uint8_t edges=0,edge=0;
    Game departure=in.path.origin;
    for(uint8_t i=0;i<in.path.stepCount;++i) {
        const auto& step=in.path.steps[i];
        if(step.action==C::Action::PalPark) {
            ++edges;edge=i;
            if(edges>1)return out;
            if(C::generation(departure)!=3 ||
               in.crossing.sourceGame!=gameName(departure) ||
               in.crossing.destinationGame!=gameName(step.game) ||
               !step.evidenceVerified || !step.level ||
               step.move || step.methodSource!=Game::Unknown)return out;
            departure=step.game;
        } else if(step.action==C::Action::VisitGame) {
            // The full chronology verifies each visit separately; record
            // the explicit source game so the crossing cannot be relabeled.
            departure=step.game;
        }
    }
    if(edges!=1)return out;
    out.palParkIndex=edge;
    // Re-evaluate the *exact prefix*, immediately before Pal Park, with
    // its witnessed departure slots (not final post-arrival slots).
    C::Path prefix=in.path;
    prefix.stepCount=edge;
    prefix.observedLevel=in.path.steps[edge].level;
    prefix.observedMoves=in.crossing.outboundGbaMoves;
    const auto before=C::evaluate(prefix);
    if(before.status==C::Status::Unresolved ||
       before.status==C::Status::Unsupported)return out;
    if(before.status==C::Status::CandidateContradicted) {
        out.status=Status::CandidateContradicted;return out;
    }
    out.departureMoves=before.rebuilt;
    out.arrivalMoves=in.crossing.immediateNdsMoves;
    // Every exact TM/HM action before transfer requires a distinct cited
    // source-game/item witness, and no disconnected extra witnesses may
    // turn a different timeline into a jointly certified history.
    std::array<bool,32> usedGba{};
    std::array<bool,32> usedNative{};
    for(uint8_t i=0;i<in.path.stepCount;++i) {
        const auto& step=in.path.steps[i];
        if(i<edge && step.action==C::Action::Learn &&
           (step.method==C::Method::TM || step.method==C::Method::HM)) {
            uint8_t matched=0;
            for(uint8_t j=0;j<in.gbaMachineCount;++j) {
                if(in.gbaMachines[j].stepIndex!=i)continue;
                ++matched;usedGba[j]=true;
                const auto status=M::classifyChronologyStep(
                    in.gbaMachines[j].exactItem,step);
                if(status==M::Status::CandidateContradicted) {
                    out.status=Status::CandidateContradicted;return out;
                }
                if(status!=M::Status::CandidateCompatible)return out;
                ++out.verifiedGbaMachines;
            }
            if(matched!=1)return out;
        }
        if(i>edge && step.action==C::Action::Learn) {
            uint8_t matched=0;
            for(uint8_t j=0;j<in.laterMethodCount;++j) {
                const auto& witness=in.laterMethods[j];
                if(witness.stepIndex!=i)continue;
                ++matched;usedNative[j]=true;
                if(!witness.independentlyVerified || !witness.independentSourceRecord ||
                   witness.sourceGame!=step.game || witness.method!=step.method ||
                   witness.move!=step.move || witness.level!=step.level ||
                   !step.evidenceVerified || step.methodSource!=step.game)return out;
                ++out.verifiedNativeMethods;
            }
            if(matched!=1)return out;
        }
    }
    for(uint8_t j=0;j<in.gbaMachineCount;++j)if(!usedGba[j])return out;
    for(uint8_t j=0;j<in.laterMethodCount;++j)if(!usedNative[j])return out;
    const auto after=C::evaluate(in.path);
    out.history=after.status;
    if(after.status==C::Status::CandidateContradicted) {
        out.status=Status::CandidateContradicted;return out;
    }
    if(after.status!=C::Status::CandidateConsistent)return out;
    out.observedMoves=after.rebuilt;
    out.status=Status::CandidateConsistent;
    return out;
}
static_assert(gameName(Game::Ruby)=="ruby_gba");
static_assert(gameName(Game::LeafGreen)=="leafgreen_gba");
static_assert(gameName(Game::Platinum)=="platinum_nds");
static_assert(gameName(Game::HeartGold)=="heartgold_nds");
static_assert(gameName(Game::Unknown).empty());
} // namespace Legality::Gen14CandidateMovePalParkJointStage
