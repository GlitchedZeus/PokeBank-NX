#pragma once
// L2 Stage 4: independently preserved Platinum/HGSS original daycare
// acquisition. Never derive source parents, held items, collection languages,
// nature PID or event identities from a detached modern PK4. A mismatched
// source is an unverified LOCAL candidate, not whole-Pokemon Invalid.
#include <array>
#include <cstdint>
#include <string_view>

namespace Legality::Gen4PlatinumHGSSOriginalDaycareSource {
enum class Status : std::uint8_t {
    NotSupplied, UnknownOriginalSource, CrossedOriginalRoute,
    ContradictoryOriginalSnapshots, ConditionallyAttested
};
struct Parent {
    std::uint64_t snapshotId=0;
    std::uint16_t species=0,heldAtCreation=0;
    std::uint8_t originalGender=2,languageAtCollection=0;
    std::uint32_t originalPersonality=0;
};
struct Input {
    bool supplied=false;
    bool originalEventIndependentlyAttested=false;
    bool originalPairIdentityIndependentlyAttested=false;
    bool physicalDepositOrderIndependentlyAttested=false;
    bool bothParentSnapshotsIndependentlyAttested=false;
    bool creationHeldItemsIndependentlyAttested=false;
    bool collectionLanguagesIndependentlyAttested=false;
    bool parentPersonalitiesIndependentlyAttested=false;
    std::string_view game{};
    std::uint64_t originalEggEvent=0,physicalPairIdentity=0;
    std::array<Parent,2> originalPhysicalSlots{};
};
struct Expected {
    std::string_view game{};
    std::uint64_t originalEggEvent=0;
    // Nonzero only where the IV/move route has independently named
    // the physical parent pair. PID replay alone passes zero.
    std::uint64_t independentlyNamedPhysicalPair=0;
    std::array<Parent,2> sourceStageParents{};
    bool collectionLanguagesKnown=false;
    bool originalPersonalitiesKnown=false;
};
constexpr Status evaluate(const Input& in,const Expected& expected) noexcept {
    if(!in.supplied)return Status::NotSupplied;
    if(!in.originalEventIndependentlyAttested ||
       !in.originalPairIdentityIndependentlyAttested ||
       !in.physicalDepositOrderIndependentlyAttested ||
       !in.bothParentSnapshotsIndependentlyAttested ||
       !in.creationHeldItemsIndependentlyAttested ||
       !in.collectionLanguagesIndependentlyAttested ||
       !in.parentPersonalitiesIndependentlyAttested ||
       !expected.collectionLanguagesKnown ||
       !expected.originalPersonalitiesKnown ||
       in.game.empty() || !in.originalEggEvent || !in.physicalPairIdentity ||
       !in.originalPhysicalSlots[0].snapshotId ||
       !in.originalPhysicalSlots[1].snapshotId ||
       in.originalPhysicalSlots[0].snapshotId==
           in.originalPhysicalSlots[1].snapshotId)
        return Status::UnknownOriginalSource;
    if(in.game!=expected.game ||
       !expected.originalEggEvent ||
       in.originalEggEvent!=expected.originalEggEvent ||
       (expected.independentlyNamedPhysicalPair &&
        in.physicalPairIdentity!=expected.independentlyNamedPhysicalPair))
        return Status::CrossedOriginalRoute;
    for(std::uint8_t i=0;i<2;++i) {
        const auto& a=in.originalPhysicalSlots[i];
        const auto& b=expected.sourceStageParents[i];
        if(a.snapshotId!=b.snapshotId || a.species!=b.species ||
           a.heldAtCreation!=b.heldAtCreation ||
           a.originalGender!=b.originalGender ||
           a.languageAtCollection!=b.languageAtCollection ||
           a.originalPersonality!=b.originalPersonality)
            return Status::ContradictoryOriginalSnapshots;
    }
    return Status::ConditionallyAttested;
}
static_assert(evaluate({}, {})==Status::NotSupplied);
} // namespace Legality::Gen4PlatinumHGSSOriginalDaycareSource
