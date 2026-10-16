#pragma once
// L2 Stage 6: independent ORIGINAL biological parent provenance.
// A traded, evolved or Pal Park-migrated parent may be a daycare parent
// only when its identity and chronological source chain are verified.
// No lineage is guessed from the detached child's OT/met/current species.
// Every contradiction is for THIS proposed history, never global Invalid.
#include "Legality/Gen34OriginalSameRouteProof.h"
#include <array>
#include <cstdint>

namespace Legality::Gen34OriginalParentLineage {
namespace Route=Gen34OriginalSameRouteProof;
enum class TransitionKind : std::uint8_t { Trade, Evolution, PalPark };
enum class Status : std::uint8_t {
    NotSupplied, UnknownOriginalHistory, CrossedBiologicalIdentity,
    ContradictedOriginalRoute, ConditionallyReplayed
};
constexpr bool gen3(Route::Game g) noexcept {
    return g>=Route::Game::Ruby && g<=Route::Game::LeafGreen;
}
constexpr bool gen4(Route::Game g) noexcept {
    return g>=Route::Game::Diamond && g<=Route::Game::SoulSilver;
}
struct Snapshot {
    std::uint64_t identity=0;
    Route::Game game=Route::Game::Unknown;
    std::uint16_t species=0;
};
struct Transition {
    bool independentlyAttested=false;
    bool sameIndividualIndependentlyAttested=false;
    // For evolution, proof of an actual source-game legal evolution is
    // external. Species IDs alone do NOT establish an evolution family.
    bool evolutionMethodIndependentlyVerified=false;
    TransitionKind kind=TransitionKind::Trade;
    std::uint64_t originalSequence=0;
    Snapshot before{},after{};
};
struct Parent {
    bool supplied=false;
    bool originalOriginIndependentlyAttested=false;
    bool originalDepositIndependentlyAttested=false;
    Snapshot originalOrigin{},deposited{};
    std::uint8_t transitions=0;
    std::array<Transition,4> history{};
};
struct Input {
    bool supplied=false;
    bool originalEventOrderingIndependentlyAttested=false;
    std::uint64_t originalEggEventSequence=0;
    std::array<Parent,2> physicalSlots{};
};
struct Expected {
    Route::Game originalDaycareGame=Route::Game::Unknown;
    std::array<std::uint64_t,2> originalDepositIdentity{};
    std::array<std::uint16_t,2> originalDepositSpecies{};
};
struct Result {
    Status status=Status::NotSupplied;
    std::array<Status,2> parentChecks{Status::NotSupplied,Status::NotSupplied};
};
constexpr Status checkParent(const Parent& p,const Expected& e,
                             std::uint8_t slot,std::uint64_t eggSequence) noexcept {
    if(!p.supplied || !p.originalOriginIndependentlyAttested ||
       !p.originalDepositIndependentlyAttested ||
       !p.originalOrigin.identity || !p.deposited.identity ||
       !Route::supported(p.originalOrigin.game) ||
       !Route::supported(p.deposited.game) ||
       !p.originalOrigin.species || !p.deposited.species ||
       p.originalOrigin.species>493 || p.deposited.species>493 ||
       (gen3(p.originalOrigin.game) && p.originalOrigin.species>386) ||
       (gen3(p.deposited.game) && p.deposited.species>386) ||
       p.transitions>p.history.size())
        return Status::UnknownOriginalHistory;
    Snapshot current=p.originalOrigin;
    std::uint64_t sequence=0;
    for(std::uint8_t i=0;i<p.transitions;++i) {
        const auto& t=p.history[i];
        if(!t.independentlyAttested || !t.sameIndividualIndependentlyAttested ||
           !t.originalSequence || t.originalSequence<=sequence)
            return Status::UnknownOriginalHistory;
        if(t.originalSequence>=eggSequence)
            return Status::ContradictedOriginalRoute;
        if(t.before.identity!=current.identity ||
           t.before.game!=current.game ||
           t.before.species!=current.species)
            return Status::CrossedBiologicalIdentity;
        if(!t.after.identity || !Route::supported(t.after.game) ||
           !t.after.species || t.after.species>493 ||
           (gen3(t.after.game) && t.after.species>386))
            return Status::UnknownOriginalHistory;
        if(t.kind==TransitionKind::Trade) {
            // Trade alone changes custody/game, never biological species.
            if(t.after.species!=current.species ||
               (gen3(current.game)!=gen3(t.after.game)))
                return Status::ContradictedOriginalRoute;
        }else if(t.kind==TransitionKind::Evolution) {
            if(!t.evolutionMethodIndependentlyVerified)
                return Status::UnknownOriginalHistory;
            if(t.after.game!=current.game ||
               t.after.species==current.species)
                return Status::ContradictedOriginalRoute;
        }else if(t.kind==TransitionKind::PalPark) {
            // Transfer is irreversible III -> IV, and originals <=386.
            if(!gen3(current.game) || !gen4(t.after.game) ||
               current.species>386 || t.after.species!=current.species)
                return Status::ContradictedOriginalRoute;
        }else return Status::UnknownOriginalHistory;
        current=t.after;
        sequence=t.originalSequence;
    }
    if(current.identity!=p.deposited.identity ||
       current.species!=p.deposited.species ||
       current.game!=p.deposited.game)
        return Status::CrossedBiologicalIdentity;
    if(current.game!=e.originalDaycareGame ||
       current.identity!=e.originalDepositIdentity[slot] ||
       current.species!=e.originalDepositSpecies[slot])
        return Status::CrossedBiologicalIdentity;
    return Status::ConditionallyReplayed;
}
constexpr Result evaluate(const Input& in,const Expected& e) noexcept {
    Result out{};
    if(!in.supplied)return out;
    if(!in.originalEventOrderingIndependentlyAttested ||
       !in.originalEggEventSequence || !Route::supported(e.originalDaycareGame) ||
       !e.originalDepositIdentity[0] || !e.originalDepositIdentity[1] ||
       e.originalDepositIdentity[0]==e.originalDepositIdentity[1] ||
       !e.originalDepositSpecies[0] || !e.originalDepositSpecies[1]) {
        out.status=Status::UnknownOriginalHistory;return out;
    }
    out.parentChecks[0]=checkParent(in.physicalSlots[0],e,0,in.originalEggEventSequence);
    out.parentChecks[1]=checkParent(in.physicalSlots[1],e,1,in.originalEggEventSequence);
    if(out.parentChecks[0]==Status::UnknownOriginalHistory ||
       out.parentChecks[1]==Status::UnknownOriginalHistory)
        out.status=Status::UnknownOriginalHistory;
    else if(out.parentChecks[0]==Status::CrossedBiologicalIdentity ||
            out.parentChecks[1]==Status::CrossedBiologicalIdentity)
        out.status=Status::CrossedBiologicalIdentity;
    else if(out.parentChecks[0]==Status::ContradictedOriginalRoute ||
            out.parentChecks[1]==Status::ContradictedOriginalRoute)
        out.status=Status::ContradictedOriginalRoute;
    else {
        // Two physical daycare slots cannot represent the SAME archived
        // biological individual disguised by two independent custody paths.
        // Compare independently attested source snapshots within the same
        // source-game namespace, including intermediate trade/evolution IDs.
        const auto& left=in.physicalSlots[0];
        const auto& right=in.physicalSlots[1];
        bool alias=false;
        for(std::uint8_t a=0;a<=left.transitions;++a) {
            const auto& l=a==0?left.originalOrigin:left.history[a-1].after;
            for(std::uint8_t b=0;b<=right.transitions;++b) {
                const auto& r=b==0?right.originalOrigin:right.history[b-1].after;
                if(l.identity==r.identity && l.game==r.game)alias=true;
            }
        }
        out.status=alias?Status::CrossedBiologicalIdentity:
                         Status::ConditionallyReplayed;
    }
    return out;
}
static_assert(evaluate({},{}).status==Status::NotSupplied);
} // namespace Legality::Gen34OriginalParentLineage
