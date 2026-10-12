#pragma once
// L3 Stage 3 retail adapter: jointly replay one PK3 source candidate against
// the full original Ruby/Sapphire/Emerald/FRLG level-up tables and exact
// numbered machine compatibility. Read-only; not production Legality.cpp.
// Original pin: pret/pokeruby@63a8cbf, pokeemerald@5eff786,
// pokefirered@c75f352. Missing source actions remain Unresolved.
#include "Legality/Gen14CandidateMoveJointEvolutionHistory.h"
#include "Legality/Gen14CandidateMoveGbaLevelUpTimeline.h"
#include "Legality/Gen14CandidateMoveMachineEvidence.h"

namespace Legality::Gen14CandidateMoveJointEvolutionRetail {
namespace H=Gen14CandidateMoveJointEvolutionHistory;
namespace L=Gen3GbaVersionedLevelUpTimeline;
namespace M=Gen14CandidateMoveMachineEvidence;
namespace C=Gen14CandidateMoveChronology;
struct OriginalSource {
    static constexpr C::Game game(H::Game g) noexcept {
        switch(g) {
        case H::Game::Ruby:return C::Game::Ruby;
        case H::Game::Sapphire:return C::Game::Sapphire;
        case H::Game::Emerald:return C::Game::Emerald;
        case H::Game::FireRed:return C::Game::FireRed;
        case H::Game::LeafGreen:return C::Game::LeafGreen;
        default:return C::Game::Unknown;
        }
    }
    static constexpr bool supported(H::Game g,uint16_t species) noexcept {
        return L::supported(game(g),species);
    }
    static constexpr uint16_t offerCount(H::Game g,uint16_t species) noexcept {
        return L::offerCount(game(g),species);
    }
    static constexpr H::Offer offerAt(H::Game g,uint16_t species,
                                      uint16_t index) noexcept {
        const auto o=L::offerAt(game(g),species,index);
        return {o.level,o.move};
    }
    static constexpr bool supportsMachine(H::Game g,uint16_t species,
                         H::Machine type,uint8_t number,uint16_t move) noexcept {
        const auto kind=type==H::Machine::TM?M::Machine::TM:M::Machine::HM;
        // Retail compatibility is ONLY a method-source check. The separate
        // Step receipt and same-individual source facts remain mandatory.
        return M::classify({game(g),species,kind,number,move,true,true})==
               M::Status::CandidateCompatible;
    }
};
using Input=H::Path;
using Result=H::Result;
constexpr Result evaluate(const Input& p) noexcept {
    return H::evaluate<OriginalSource>(p);
}
constexpr H::Status evaluate(const H::Alternatives& a) noexcept {
    return H::evaluate<OriginalSource>(a);
}
static_assert(OriginalSource::supported(H::Game::Ruby,386));
static_assert(OriginalSource::supported(H::Game::FireRed,386));
static_assert(OriginalSource::offerAt(H::Game::Ruby,4,3).move==108);
static_assert(OriginalSource::offerAt(H::Game::FireRed,4,3).move==232);
static_assert(OriginalSource::offerAt(H::Game::Ruby,5,7).move==53);
static_assert(OriginalSource::offerAt(H::Game::Ruby,6,9).move==17);
static_assert(OriginalSource::supportsMachine(H::Game::Ruby,6,H::Machine::HM,2,19));
static_assert(!OriginalSource::supportsMachine(H::Game::Ruby,5,H::Machine::HM,2,19));
static_assert(OriginalSource::supportsMachine(H::Game::LeafGreen,1,H::Machine::TM,10,237));
} // namespace Legality::Gen14CandidateMoveJointEvolutionRetail
