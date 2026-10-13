#pragma once
// L2 Phase A original Gen V/VI daycare evidence alternatives.
// PKHeX reference: EncounterEgg5/6 + AbilityBreedLegality + Breeding.
// Conditional original history, NOT inferred from detached PK5/PK6.
// Reuses Gen56PKHeXEggTemplate; no duplicate native PK5 codec.
#include "Legality/Gen56PKHeXEggTemplate.h"
#include <array>
#include <cstdint>
namespace Legality::Gen56OriginalEggHistories {
namespace Egg=Gen56PKHeXEggTemplate;
enum class Sex : std::uint8_t { Male, Female, Genderless, Unknown };
enum class Verdict : std::uint8_t { NoRoutes, CapacityUnknown, OneConditionalMatch,
    UnknownOriginalSource, UnresolvedLocalHistories, UnknownExhaustiveness,
    AllOriginalLocalHistoriesContradicted };
struct Parent {
    std::uint64_t snapshotId=0;
    std::uint16_t originalSpecies=0;
    Sex sex=Sex::Unknown;
    bool originalSnapshotIndependentlyVerified=false;
    bool originallyHiddenAbility=false;
    bool originalAbilityIndependentlyVerified=false;
    // Original held item is a historical source datum, never copied from
    // a post-hatch specimen. No original IV/nature RNG claim is inferred.
    std::uint16_t originalHeldItem=0;
    bool originalHeldItemIndependentlyVerified=false;
};
struct Route {
    std::uint64_t routeId=0,originalEggId=0,originalEventId=0,originalPairId=0;
    Egg::Game sourceGame=Egg::Game::Unknown;
    std::uint16_t originalEggSpecies=0;
    bool originalEventIndependentlyAttested=false;
    bool originalPhysicalPairIndependentlyAttested=false;
    bool originalPairEligibilityIndependentlyVerified=false;
    bool eligibleOriginalPair=false;
    bool originalHiddenTransmissionMethodIndependentlyVerified=false;
    bool allLocalMethodsIndependentlyCovered=false;
    std::array<Parent,2> slots{};
};
struct Input {
    Egg::Claim claimedEgg{};
    std::uint64_t originalEggId=0;
    bool sameOriginalUnhatchedEggIndependentlyAttested=false;
    std::array<Route,8> routes{};
    std::uint8_t count=0;
    bool allOriginalRoutesIndependentlyEnumerated=false;
};
struct Result {
    Verdict verdict=Verdict::NoRoutes;
    std::uint8_t matched=0,contradicted=0,unknown=0,unverified=0;
    std::array<Egg::Status,8> eggChecks{};
};
constexpr Result evaluate(const Input& in) noexcept {
    Result out{};
    if(!in.count)return out;
    if(in.count>in.routes.size()) {out.verdict=Verdict::CapacityUnknown;return out;}
    if(!in.originalEggId || !in.sameOriginalUnhatchedEggIndependentlyAttested ||
       Egg::generation(in.claimedEgg.originGame)==0){
        out.verdict=Verdict::UnknownOriginalSource;return out;
    }
    bool duplicate=false;
    for(std::uint8_t i=0;i<in.count;++i){
        const auto& r=in.routes[i];
        if(!r.routeId) {duplicate=true;++out.unverified;continue;}
        bool repeated=false;
        for(std::uint8_t j=0;j<i;++j)
            if(r.routeId==in.routes[j].routeId)repeated=true;
        if(repeated)duplicate=true;
        if(!r.originalEventIndependentlyAttested ||
           !r.originalPhysicalPairIndependentlyAttested ||
           !r.originalEventId || !r.originalPairId ||
           !r.originalEggId || r.originalEggId!=in.originalEggId ||
           r.originalEggSpecies!=in.claimedEgg.originalEggSpecies ||
           r.sourceGame!=in.claimedEgg.originGame ||
           !r.slots[0].originalSnapshotIndependentlyVerified ||
           !r.slots[1].originalSnapshotIndependentlyVerified ||
           !r.slots[0].originalHeldItemIndependentlyVerified ||
           !r.slots[1].originalHeldItemIndependentlyVerified ||
           !r.slots[0].originalAbilityIndependentlyVerified ||
           !r.slots[1].originalAbilityIndependentlyVerified ||
           !r.slots[0].snapshotId || !r.slots[1].snapshotId ||
           r.slots[0].snapshotId==r.slots[1].snapshotId ||
           !r.slots[0].originalSpecies || !r.slots[1].originalSpecies ||
           r.slots[0].sex==Sex::Unknown || r.slots[1].sex==Sex::Unknown ||
           !r.originalPairEligibilityIndependentlyVerified){
            ++out.unverified;continue;
        }
        const bool ditto0=r.slots[0].originalSpecies==132;
        const bool ditto1=r.slots[1].originalSpecies==132;
        // Physical daycare order remains distinct from donor sex roles.
        const bool complementary=(r.slots[0].sex==Sex::Male && r.slots[1].sex==Sex::Female) ||
                                 (r.slots[0].sex==Sex::Female && r.slots[1].sex==Sex::Male);
        if(!r.eligibleOriginalPair || (ditto0 && ditto1) ||
           (!ditto0 && !ditto1 && !complementary)){
            if(r.allLocalMethodsIndependentlyCovered && !repeated)++out.contradicted;
            else ++out.unknown;
            continue;
        }
        out.eggChecks[i]=Egg::evaluate(in.claimedEgg).status;
        if(out.eggChecks[i]==Egg::Status::LocalContradiction){
            if(r.allLocalMethodsIndependentlyCovered && !repeated)++out.contradicted;
            else ++out.unknown;
            continue;
        }
        if(out.eggChecks[i]!=Egg::Status::ConditionallyCompatible){++out.unknown;continue;}
        if(in.claimedEgg.originalAbilityNumber==4){
            // PKHeX only establishes species/form Hidden Ability permission;
            // an erased original maternal/Ditto transmission event cannot be
            // reconstructed or positively verified from the child alone.
            if(!r.originalHiddenTransmissionMethodIndependentlyVerified){
                ++out.unknown;continue;
            }
            bool sourceHasHidden=false;
            for(const auto& p:r.slots)sourceHasHidden|=p.originallyHiddenAbility && p.originalSpecies!=132;
            if(!sourceHasHidden){
                if(r.allLocalMethodsIndependentlyCovered && !repeated)++out.contradicted;
                else ++out.unknown;
                continue;
            }
        }
        ++out.matched;
    }
    if(out.matched)out.verdict=Verdict::OneConditionalMatch;
    else if(duplicate || out.unverified)out.verdict=Verdict::UnknownOriginalSource;
    else if(out.unknown)out.verdict=Verdict::UnresolvedLocalHistories;
    else if(!in.allOriginalRoutesIndependentlyEnumerated)
        out.verdict=Verdict::UnknownExhaustiveness;
    else if(out.contradicted==in.count)
        out.verdict=Verdict::AllOriginalLocalHistoriesContradicted;
    else out.verdict=Verdict::UnresolvedLocalHistories;
    return out;
}
} // namespace Legality::Gen56OriginalEggHistories
