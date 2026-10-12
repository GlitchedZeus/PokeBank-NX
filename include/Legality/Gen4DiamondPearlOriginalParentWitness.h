#pragma once
// L2 Stage 3: independently archived original Diamond/Pearl daycare parent
// identity, creation-time item and collection-time language evidence.
// Physical slots are ordered as deposited, NOT inferred from a detached PK4.
// A contradictory archive pairing is Unknown/incoherent candidate provenance,
// never evidence that the child is globally illegal.
#include <array>
#include <cstdint>
namespace Legality::Gen4DiamondPearlOriginalParentWitness {
enum class Status : std::uint8_t {
    NotSupplied, UnknownSourceEvidence, IncoherentPairOrEvent,
    IncoherentOriginalSnapshots, ConditionallyMatched
};
struct Snapshot {
    std::uint64_t snapshotIdentity=0;
    std::uint16_t species=0;
    std::uint8_t gender=2;
    std::uint32_t personality=0;
    std::uint16_t heldItemAtCreation=0;
    std::uint8_t languageAtCollection=0;
};
struct Input {
    bool supplied=false;
    bool originalDaycareEventIndependentlyVerified=false;
    bool physicalDepositOrderIndependentlyVerified=false;
    bool originalPairIdentityIndependentlyVerified=false;
    bool originalParentSnapshotsIndependentlyVerified=false;
    bool creationItemsIndependentlyVerified=false;
    bool collectionLanguagesIndependentlyVerified=false;
    std::uint64_t originalEggEventIdentity=0;
    std::uint64_t originalPhysicalPairIdentity=0;
    std::array<Snapshot,2> physicalSlots{};
};
struct Expected {
    std::uint64_t originalEggEventIdentity=0;
    // Optional independently attested IV/move pair identity; 0 means the
    // source PID evaluator has not established an IV/move pair identity.
    std::uint64_t otherStagePhysicalPairIdentity=0;
    std::array<std::uint16_t,2> species{};
    std::array<std::uint8_t,2> gender{};
    std::array<std::uint32_t,2> personalities{};
    std::array<bool,2> personalitiesIndependentlyVerified{};
    std::array<std::uint16_t,2> creationItems{};
    std::array<std::uint8_t,2> collectionLanguages{};
};
constexpr Status evaluate(const Input& source,const Expected& expected) noexcept {
    if(!source.supplied)return Status::NotSupplied;
    if(!source.originalDaycareEventIndependentlyVerified ||
       !source.physicalDepositOrderIndependentlyVerified ||
       !source.originalPairIdentityIndependentlyVerified ||
       !source.originalParentSnapshotsIndependentlyVerified ||
       !source.creationItemsIndependentlyVerified ||
       !source.collectionLanguagesIndependentlyVerified ||
       !source.originalEggEventIdentity || !source.originalPhysicalPairIdentity ||
       !source.physicalSlots[0].snapshotIdentity ||
       !source.physicalSlots[1].snapshotIdentity ||
       source.physicalSlots[0].snapshotIdentity==
           source.physicalSlots[1].snapshotIdentity ||
       !expected.personalitiesIndependentlyVerified[0] ||
       !expected.personalitiesIndependentlyVerified[1])
        return Status::UnknownSourceEvidence;
    if(!expected.originalEggEventIdentity ||
       source.originalEggEventIdentity!=expected.originalEggEventIdentity ||
       (expected.otherStagePhysicalPairIdentity &&
        source.originalPhysicalPairIdentity!=
            expected.otherStagePhysicalPairIdentity))
        return Status::IncoherentPairOrEvent;
    for(std::uint8_t i=0;i<2;++i) {
        const auto& slot=source.physicalSlots[i];
        if(slot.species!=expected.species[i] ||
           slot.gender!=expected.gender[i] ||
           slot.personality!=expected.personalities[i] ||
           slot.heldItemAtCreation!=expected.creationItems[i] ||
           slot.languageAtCollection!=expected.collectionLanguages[i])
            return Status::IncoherentOriginalSnapshots;
    }
    return Status::ConditionallyMatched;
}
static_assert(evaluate({}, {})==Status::NotSupplied);
} // namespace Legality::Gen4DiamondPearlOriginalParentWitness
