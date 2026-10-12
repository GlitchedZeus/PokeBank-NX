#pragma once
// L3 Stage 4 retail adapter: candidate-local source-specific Pal Park and
// native Platinum. Original Ruby/Sapphire/Emerald/FRLG sources are joined by
// the exact Stage 3 source tables. Platinum methods: pret/pokeplatinum
// @fe1ea418a7364c93daf0d07ae55e195452d8803c, selected 12 National forms
// and 12 numbered machine sources. For other Gen IV games/species the
// native move method remains Unresolved. No changes to production legality.
#include "Legality/Gen14CandidateMovePalParkNativeJoint.h"
#include "Legality/Gen14CandidateMoveJointEvolutionRetail.h"
#include "Legality/Gen4PlatinumNativeMethodData.h"
#include "Legality/Gen4PlatinumPinnedMachines.h"
namespace Legality::Gen14CandidateMovePalParkNativeRetail {
namespace Joint=Gen14CandidateMovePalParkNativeJoint;
namespace G4=Gen4PlatinumNativeMethodData;
namespace Pin=Gen4PlatinumPinnedMachines;
using Status=Joint::Status;
struct PlatinumSource {
    static constexpr bool supported(Joint::Game game,uint16_t species) noexcept {
        return game==Joint::Game::Platinum && G4::rowIndex(species)<G4::kRows.size();
    }
    static constexpr uint16_t offerCount(Joint::Game game,uint16_t species) noexcept {
        return supported(game,species)?G4::offerCount(species):0;
    }
    static constexpr Joint::Offer offerAt(Joint::Game game,uint16_t species,
                                           uint16_t index) noexcept {
        const uint16_t packed=offerCount(game,species)>index?
            G4::packedOffer(species,index):0;
        return {static_cast<uint8_t>(packed>>9),static_cast<uint16_t>(packed&511u)};
    }
    static constexpr bool machineKnown(Joint::Game game,bool hidden,uint8_t number) noexcept {
        return game==Joint::Game::Platinum && Pin::known(hidden,number);
    }
    static constexpr bool machineCompatible(Joint::Game game,uint16_t species,
                            bool hidden,uint8_t number,uint16_t move) noexcept {
        return supported(game,species) && Pin::compatible(species,hidden,number,move);
    }
    static constexpr bool tutorCompatible(Joint::Game game,uint16_t species,uint16_t move) noexcept {
        return supported(game,species) && G4::tutorCompatible(species,move);
    }
};
using GbaSource=Gen14CandidateMoveJointEvolutionRetail::OriginalSource;
using Input=Joint::Input;
using Result=Joint::Result;
constexpr Result evaluate(const Input& witness) noexcept {
    return Joint::evaluate<GbaSource,PlatinumSource>(witness);
}
static_assert(PlatinumSource::supported(Joint::Game::Platinum,1));
static_assert(!PlatinumSource::supported(Joint::Game::Diamond,1));
static_assert(!PlatinumSource::supported(Joint::Game::Platinum,152));
static_assert(PlatinumSource::offerAt(Joint::Game::Platinum,1,8).level==21);
static_assert(PlatinumSource::offerAt(Joint::Game::Platinum,1,8).move==230);
static_assert(PlatinumSource::machineKnown(Joint::Game::Platinum,false,53));
static_assert(PlatinumSource::machineCompatible(Joint::Game::Platinum,1,false,53,412));
static_assert(!PlatinumSource::machineCompatible(Joint::Game::Platinum,1,true,2,19));
} // namespace Legality::Gen14CandidateMovePalParkNativeRetail
