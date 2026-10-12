#pragma once
// BREED-003: conditional source-versioned daycare pair eligibility and
// move-donor role derivation. Parent personal/gender/OT fields MUST come
// from independently verified version-correct snapshots. This is not a
// reconstructed breeding history from the offspring's native PK3/PK4.
#include "Legality/Gen34SameParentEggHistory.h"
#include <cstdint>
#include <string_view>

namespace Legality::Gen34BreedingPairEligibility {
namespace Move=Gen34SameParentEggHistory;
enum class Gender : uint8_t { Unknown, Male, Female, Genderless };
enum class Status : uint8_t {
    UnsupportedGame, UnknownParents, IncoherentParents,
    UnknownPersonalData, UnknownGender, Incompatible, Compatible
};
struct Parent {
    uint32_t snapshotId=0;
    uint16_t species=0;
    uint8_t eggGroup1=0,eggGroup2=0; // version-correct retail group IDs
    Gender gender=Gender::Unknown;
    uint32_t fullOTId=0;             // complete 32-bit OT ID, not TID alone
    bool personalDataIndependentlyVerified=false;
    bool genderIndependentlyVerified=false;
    bool fullOTIdKnown=false;
};
struct Input {
    std::string_view game{};
    Parent slot0{},slot1{};
    // Source-attested physical deposit order, separate from all roles.
    bool depositOrderIndependentlyVerified=false;
};
struct Result {
    Status status=Status::UnknownParents;
    uint8_t compatibilityScore=0;  // 0, 20, 50, or 70 when scoreKnown
    bool scoreKnown=false;
    bool donorRolesKnown=false;
    uint32_t fatherId=0,motherId=0;
    bool depositOrderKnown=false;
    uint32_t slot0Id=0,slot1Id=0;
};
constexpr bool supportedGame(std::string_view game) noexcept {
    // Exact directly checked source families, rather than an assumption
    // that all games implement the same parent and donor algorithms.
    return game=="emerald_gba" || game=="platinum_nds" ||
           game=="heartgold_nds" || game=="soulsilver_nds";
}
constexpr bool validGroup(uint8_t group) noexcept {
    return group>=1 && group<=15;
}
constexpr bool shareGroup(const Parent& a,const Parent& b) noexcept {
    return a.eggGroup1==b.eggGroup1 ||
           (b.eggGroup2 && a.eggGroup1==b.eggGroup2) ||
           (a.eggGroup2 && a.eggGroup2==b.eggGroup1) ||
           (a.eggGroup2 && b.eggGroup2 && a.eggGroup2==b.eggGroup2);
}
constexpr Result evaluate(const Input& in) noexcept {
    Result out{};
    if(!supportedGame(in.game)) {
        out.status=Status::UnsupportedGame;return out;
    }
    const Parent& a=in.slot0;
    const Parent& b=in.slot1;
    if(!a.snapshotId || !b.snapshotId || !a.species || !b.species) return out;
    if(a.snapshotId==b.snapshotId) {
        out.status=Status::IncoherentParents;return out;
    }
    // Version-specific native parent species ceiling. A known impossible
    // source-species pair contradicts this candidate, not the child.
    const uint16_t maxSpecies=in.game=="emerald_gba"?386u:493u;
    if(a.species>maxSpecies || b.species>maxSpecies) {
        out.status=Status::Incompatible;out.scoreKnown=true;return out;
    }
    out.depositOrderKnown=in.depositOrderIndependentlyVerified;
    if(out.depositOrderKnown) {
        out.slot0Id=a.snapshotId;out.slot1Id=b.snapshotId;
    }
    if(!a.personalDataIndependentlyVerified ||
       !b.personalDataIndependentlyVerified ||
       !validGroup(a.eggGroup1) || !validGroup(b.eggGroup1) ||
       (a.eggGroup2 && !validGroup(a.eggGroup2)) ||
       (b.eggGroup2 && !validGroup(b.eggGroup2))) {
        out.status=Status::UnknownPersonalData;return out;
    }
    // Retail first-group terminal cases; do not infer source personal data.
    constexpr uint8_t ditto=13, undiscovered=15;
    const bool aditto=a.eggGroup1==ditto;
    const bool bditto=b.eggGroup1==ditto;
    if(a.eggGroup1==undiscovered || b.eggGroup1==undiscovered ||
       (aditto && bditto)) {
        out.status=Status::Incompatible;
        out.scoreKnown=true;return out;
    }
    const bool oneDitto=aditto!=bditto;
    if(!oneDitto) {
        if(!a.genderIndependentlyVerified || !b.genderIndependentlyVerified ||
           a.gender==Gender::Unknown || b.gender==Gender::Unknown) {
            out.status=Status::UnknownGender;return out;
        }
        const bool opposite=(a.gender==Gender::Female && b.gender==Gender::Male) ||
                            (a.gender==Gender::Male && b.gender==Gender::Female);
        if(!opposite || !shareGroup(a,b)) {
            out.status=Status::Incompatible;
            out.scoreKnown=true;return out;
        }
        out.fatherId=a.gender==Gender::Male?a.snapshotId:b.snapshotId;
        out.motherId=a.gender==Gender::Female?a.snapshotId:b.snapshotId;
        out.donorRolesKnown=true;
    } else {
        const Parent& normal=aditto?b:a;
        const Parent& d=aditto?a:b;
        if(normal.genderIndependentlyVerified &&
           normal.gender!=Gender::Unknown) {
            if(normal.gender==Gender::Female) {
                out.fatherId=d.snapshotId;out.motherId=normal.snapshotId;
            } else {
                // Male OR genderless with Ditto uses the non-Ditto donor.
                out.fatherId=normal.snapshotId;out.motherId=d.snapshotId;
            }
            out.donorRolesKnown=true;
        }
        // Pair eligibility with Ditto does not depend on gender, but
        // unknown role semantics must not turn into verified move evidence.
    }
    out.status=Status::Compatible;
    if(a.fullOTIdKnown && b.fullOTIdKnown) {
        const bool differentOT=a.fullOTId!=b.fullOTId;
        out.compatibilityScore=oneDitto ? (differentOT?50:20) :
            (a.species==b.species ? (differentOT?70:50) : (differentOT?50:20));
        out.scoreKnown=true;
    }
    return out;
}
// Explicit bridge to the existing four-slot parent-hypothesis proof.
// Never turn unsupported, lost data or mismatched donor identity into a
// compatible move-parent witness. Incompatibility is candidate-local.
constexpr Move::PairProof proofFor(const Result& r,
                                   uint32_t candidateFatherId,
                                   uint32_t candidateMotherId) noexcept {
    if(!candidateFatherId || !candidateMotherId ||
       candidateFatherId==candidateMotherId)
        return Move::PairProof::Unknown;
    const bool samePair=(r.depositOrderKnown &&
        ((r.slot0Id==candidateFatherId && r.slot1Id==candidateMotherId) ||
         (r.slot0Id==candidateMotherId && r.slot1Id==candidateFatherId)));
    // If deposit order is not attested, eligibility may still be known,
    // but its parent IDs must be tied to the role snapshots independently.
    if(r.status==Status::Compatible && r.donorRolesKnown &&
       r.fatherId==candidateFatherId && r.motherId==candidateMotherId)
        return Move::PairProof::IndependentlyCompatible;
    if(r.status==Status::Incompatible && samePair)
        return Move::PairProof::IndependentlyIncompatible;
    return Move::PairProof::Unknown;
}
} // namespace Legality::Gen34BreedingPairEligibility
