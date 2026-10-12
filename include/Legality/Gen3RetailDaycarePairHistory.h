#pragma once
// L2 Stage 1: conditional ORIGINAL Gen III deposited daycare pair witness.
// Verified against:
// pret/pokeruby@5784633c src/daycare.c GetDaycareCompatibilityScore,
// _GetEggSpecies/GetEggSpeciesAndParentSlots;
// pret/pokeemerald@731ad5bf src/daycare.c analogous source paths;
// pret/pokefirered@037335f4 src/daycare.c analogous source paths.
// Independently supplied ORIGINAL parent snapshots only. Never derive a
// historical deposited pair, OT, personality or egg group from a child PK3.
// Incompatibility is ONLY candidate-local, never whole-Pokemon Invalid.
#include <array>
#include <cstdint>
#include <string_view>

namespace Legality::Gen3RetailDaycarePairHistory {
enum class Status : uint8_t {
    UnsupportedGame, UnknownOriginalStage, UnknownOriginalSnapshots,
    UnknownPersonalData, IncompatiblePair, ConditionallyCompatiblePair
};
enum class Gender : uint8_t { Male, Female, Genderless };
struct OriginalParent {
    uint64_t snapshotIdentity=0;
    uint16_t species=0; // national original source species 1..386
    uint32_t personality=0;
    uint32_t originalFullOTId=0; // original complete 32-bit OT ID
    uint8_t eggGroup1=0,eggGroup2=0;
    uint8_t genderRatio=255; // retail species personal gender threshold
    bool independentlyAttestedSnapshot=false;
    bool independentlyAttestedPersonalData=false;
    bool independentlyAttestedOTId=false;
};
struct Input {
    std::string_view game{};
    uint64_t originalDaycareEventIdentity=0;
    bool eventIdentityIndependentlyVerified=false;
    bool physicalDepositOrderIndependentlyVerified=false;
    std::array<OriginalParent,2> physicalSlots{};
};
struct Result {
    Status status=Status::UnknownOriginalStage;
    uint8_t compatibilityScore=0; // 0, 20, 50, 70; only when known
    bool scoreKnown=false;
    bool physicalRolesKnown=false;
    uint64_t originalFatherSnapshot=0,originalMotherSnapshot=0;
    // For Ditto + male or genderless, egg species derives from the
    // NON-Ditto parent even when the egg move "mother" slot is Ditto.
    uint64_t originalEggSpeciesParentSnapshot=0;
};
constexpr bool supported(std::string_view game) noexcept {
    return game=="ruby_gba" || game=="sapphire_gba" ||
           game=="emerald_gba" || game=="firered_gba" ||
           game=="leafgreen_gba";
}
constexpr bool validGroup(uint8_t group) noexcept {
    return group>=1 && group<=15;
}
constexpr Gender gender(const OriginalParent& parent) noexcept {
    if(parent.genderRatio==255) return Gender::Genderless;
    if(parent.genderRatio==254) return Gender::Female;
    if(parent.genderRatio==0) return Gender::Male;
    return static_cast<uint8_t>(parent.personality & 0xFFu) <
           parent.genderRatio ? Gender::Female : Gender::Male;
}
constexpr bool overlap(const OriginalParent& a,
                       const OriginalParent& b) noexcept {
    return a.eggGroup1==b.eggGroup1 ||
           a.eggGroup1==b.eggGroup2 ||
           a.eggGroup2==b.eggGroup1 ||
           a.eggGroup2==b.eggGroup2;
}
constexpr Result evaluate(const Input& in) noexcept {
    Result out{};
    if(!supported(in.game)){out.status=Status::UnsupportedGame;return out;}
    if(!in.originalDaycareEventIdentity ||
       !in.eventIdentityIndependentlyVerified ||
       !in.physicalDepositOrderIndependentlyVerified)
        return out;
    const auto& a=in.physicalSlots[0];
    const auto& b=in.physicalSlots[1];
    if(!a.independentlyAttestedSnapshot || !b.independentlyAttestedSnapshot ||
       !a.snapshotIdentity || !b.snapshotIdentity ||
       a.snapshotIdentity==b.snapshotIdentity || !a.species || !b.species) {
        out.status=Status::UnknownOriginalSnapshots;return out;
    }
    if(!a.independentlyAttestedPersonalData ||
       !b.independentlyAttestedPersonalData ||
       !validGroup(a.eggGroup1) || !validGroup(a.eggGroup2) ||
       !validGroup(b.eggGroup1) || !validGroup(b.eggGroup2) ||
       // Contradictory claimed personal snapshots are NOT retail proof.
       (a.species==132)!=(a.eggGroup1==13) ||
       (b.species==132)!=(b.eggGroup1==13)) {
        out.status=Status::UnknownPersonalData;return out;
    }
    // Source-native species ceiling is checked on the original pair,
    // not on the detached descendant. Preserve candidate-local rejection.
    if(a.species>386 || b.species>386 ||
       a.eggGroup1==15 || b.eggGroup1==15 ||
       (a.eggGroup1==13 && b.eggGroup1==13)) {
        out.status=Status::IncompatiblePair;
        out.scoreKnown=true;return out;
    }
    const bool aDitto=a.eggGroup1==13,bDitto=b.eggGroup1==13;
    if(!aDitto && !bDitto) {
        if(gender(a)==gender(b) || gender(a)==Gender::Genderless ||
           gender(b)==Gender::Genderless || !overlap(a,b)) {
            out.status=Status::IncompatiblePair;
            out.scoreKnown=true;return out;
        }
        out.originalFatherSnapshot=gender(a)==Gender::Male ?
            a.snapshotIdentity:b.snapshotIdentity;
        out.originalMotherSnapshot=gender(a)==Gender::Female ?
            a.snapshotIdentity:b.snapshotIdentity;
        out.originalEggSpeciesParentSnapshot=out.originalMotherSnapshot;
    } else {
        const auto& normal=aDitto?b:a;
        const auto& ditto=aDitto?a:b;
        if(gender(normal)==Gender::Female) {
            out.originalMotherSnapshot=normal.snapshotIdentity;
            out.originalFatherSnapshot=ditto.snapshotIdentity;
        } else {
            out.originalMotherSnapshot=ditto.snapshotIdentity;
            out.originalFatherSnapshot=normal.snapshotIdentity;
        }
        out.originalEggSpeciesParentSnapshot=normal.snapshotIdentity;
    }
    out.physicalRolesKnown=true;
    out.status=Status::ConditionallyCompatiblePair;
    // OT identity is a source-history witness, not a guess from the child.
    if(a.independentlyAttestedOTId && b.independentlyAttestedOTId) {
        const bool diffOT=a.originalFullOTId!=b.originalFullOTId;
        out.compatibilityScore=(aDitto || bDitto) ?
            (diffOT?50:20) :
            a.species==b.species ? (diffOT?70:50) : (diffOT?50:20);
        out.scoreKnown=true;
    }
    return out;
}
static_assert(supported("sapphire_gba"));
static_assert(supported("leafgreen_gba"));
static_assert(!supported("diamond_nds"));
} // namespace Legality::Gen3RetailDaycarePairHistory
