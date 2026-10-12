#pragma once
// L2 B07: read-only source-game-specific Gen III ordinary-daycare PID history.
// Original retail pins: pret/pokeruby@5784633c src/daycare.c,
// pret/pokeemerald@731ad5bf src/daycare.c, and
// pret/pokefirered@037335f4 src/daycare.c (commented FRLG nature code
// is NOT retail functionality). RNG: each game's src/random.c.
// A historical RNG, pair or creation-time held item cannot be recovered
// merely from one detached native PK3. Contradictions are candidate-local.
#include "Legality/Gen3RetailDaycarePairHistory.h"
#include <array>
#include <cstdint>
#include <string_view>

namespace Legality::Gen3EggPIDCreationHistory {
enum class Family : uint8_t { Unsupported, RubySapphire, Emerald, FireRedLeafGreen };
constexpr Family family(std::string_view g) noexcept {
    if(g=="ruby_gba"||g=="sapphire_gba")return Family::RubySapphire;
    if(g=="emerald_gba")return Family::Emerald;
    if(g=="firered_gba"||g=="leafgreen_gba")return Family::FireRedLeafGreen;
    return Family::Unsupported;
}
enum class Status : uint8_t {
    UnsupportedGame, UnknownOriginalEgg, UnknownOriginalParentPair,
    IncompatibleOriginalDaycarePair, UnknownParentSnapshots,
    UnknownCreationItems, UnknownOriginalStage,
    IncoherentOriginalStage, UnknownPendingRNG, UnknownCollectionRNG,
    UnknownEverstoneVBlankSeed, OriginalEggPIDContradicted,
    ConditionallyMatchedOriginalEggPID
};
struct Parent {
    uint32_t snapshotIdentity=0;
    uint16_t species=0;
    uint8_t gender=2; // original GetBoxMonGender: 0 male, 1 female, 2 genderless
    uint32_t originalPersonality=0;
    bool heldEverstoneAtCreation=false;
    bool originalSnapshotIndependentlyVerified=false;
    bool originalHeldItemIndependentlyVerified=false;
};
struct Input {
    std::string_view game{};
    bool originalUnhatchedEggIndependentlyVerified=false;
    uint32_t originalUnhatchedEggPID=0;
    bool ordinaryDaycarePairIndependentlyVerified=false;
    bool ordinaryDaycarePairEligible=false;
    bool originalPhysicalDepositOrderIndependentlyVerified=false;
    std::array<Parent,2> physicalSlots{};
    // Optional stronger, independently attested ORIGINAL parent witness.
    // Detached PK3s cannot synthesize this provenance. Legacy direct
    // source-stage replay remains available without the opt-in witness.
    bool originalRetailPairWitnessSupplied=false;
    Gen3RetailDaycarePairHistory::Input originalRetailPair{};
    bool originalCreationStageIndependentlyVerified=false;
    bool originalCollectionStageIndependentlyVerified=false;
    uint64_t originalParentStageIdentity=0;
    uint64_t originalCreationStageIdentity=0;
    uint64_t originalCollectionStageIdentity=0;
    bool pendingLCRNGStateIndependentlyVerified=false;
    uint32_t pendingLCRNGState=0;
    // Only Ruby/Sapphire and FireRed/LeafGreen consume another Random()
    // to supply the upper half of the stored pending 16-bit value.
    bool collectionLCRNGStateIndependentlyVerified=false;
    uint32_t collectionLCRNGState=0;
    // Emerald reseeds the SECOND RNG from gMain.vblankCounter2 (u16)
    // before choosing its PID; detached PK3s never preserve this count.
    bool originalEmeraldVBlankCounterIndependentlyVerified=false;
    uint16_t originalEmeraldVBlankCounter=0;
};
struct Result {
    Status status=Status::UnknownOriginalEgg;
    uint32_t pendingPersonality=0, finalPersonality=0;
    uint32_t finalPendingLCRNGState=0,finalCollectionLCRNGState=0;
    uint32_t finalSecondRNGState=0;
    uint16_t originalNatureAttempts=0;
    // Emerald can draw 2401 full PID candidates, plus Everstone coin.
    uint16_t pendingRNGDraws=0;
    uint8_t collectionRNGDraws=0;
    int8_t originalEverstoneDonor=-1;
    bool natureInheritanceActivated=false,natureSearchExhausted=false;
};
constexpr uint16_t nextRNG(uint32_t& s) noexcept {
    s=s*1103515245u+24691u;
    return static_cast<uint16_t>(s>>16);
}
constexpr uint8_t nature(uint32_t pid) noexcept {
    return static_cast<uint8_t>(pid%25u);
}
constexpr Result evaluate(const Input& in) noexcept {
    Result out{};
    const Family f=family(in.game);
    if(f==Family::Unsupported) {
        out.status=Status::UnsupportedGame;return out;
    }
    if(!in.originalUnhatchedEggIndependentlyVerified)return out;
    if(!in.ordinaryDaycarePairIndependentlyVerified ||
       !in.originalPhysicalDepositOrderIndependentlyVerified) {
        out.status=Status::UnknownOriginalParentPair;return out;
    }
    if(!in.ordinaryDaycarePairEligible) {
        out.status=Status::IncompatibleOriginalDaycarePair;return out;
    }
    for(const Parent& p:in.physicalSlots) {
        if(!p.originalSnapshotIndependentlyVerified || !p.snapshotIdentity ||
           p.species==0 || p.gender>2) {
            out.status=Status::UnknownParentSnapshots;return out;
        }
    }
    const Parent& a=in.physicalSlots[0],&b=in.physicalSlots[1];
    if(a.snapshotIdentity==b.snapshotIdentity) {
        out.status=Status::UnknownParentSnapshots;return out;
    }
    // Even independently claimed eligibility cannot create a two-Ditto
    // ordinary egg. Do not replay an unreachable original nature branch.
    if(a.species==132u && b.species==132u) {
        out.status=Status::IncompatibleOriginalDaycarePair;return out;
    }
    if(!in.originalCreationStageIndependentlyVerified ||
       !in.originalCollectionStageIndependentlyVerified ||
       !in.originalParentStageIdentity ||
       !in.originalCreationStageIdentity ||
       !in.originalCollectionStageIdentity) {
        out.status=Status::UnknownOriginalStage;return out;
    }
    if(in.originalParentStageIdentity!=in.originalCreationStageIdentity ||
       in.originalCreationStageIdentity!=in.originalCollectionStageIdentity) {
        out.status=Status::IncoherentOriginalStage;return out;
    }
    if(in.originalRetailPairWitnessSupplied) {
        namespace R=Gen3RetailDaycarePairHistory;
        const auto& pair=in.originalRetailPair;
        if(pair.game!=in.game ||
           pair.originalDaycareEventIdentity!=in.originalParentStageIdentity) {
            out.status=Status::IncoherentOriginalStage;return out;
        }
        const auto original=R::evaluate(pair);
        if(original.status==R::Status::IncompatiblePair) {
            out.status=Status::IncompatibleOriginalDaycarePair;return out;
        }
        if(original.status!=R::Status::ConditionallyCompatiblePair ||
           !original.physicalRolesKnown) {
            out.status=Status::UnknownOriginalParentPair;return out;
        }
        // Do not combine compatibility for one deposited pair with PID
        // replay for other independently claimed original parents.
        for(uint8_t i=0;i<2;++i) {
            const auto& witness=pair.physicalSlots[i];
            const auto& parent=in.physicalSlots[i];
            if(parent.snapshotIdentity!=witness.snapshotIdentity ||
               parent.species!=witness.species ||
               parent.originalPersonality!=witness.personality ||
               parent.gender!=static_cast<uint8_t>(R::gender(witness))) {
                out.status=Status::UnknownOriginalParentPair;return out;
            }
        }
    }
    if(!in.pendingLCRNGStateIndependentlyVerified) {
        out.status=Status::UnknownPendingRNG;return out;
    }
    uint32_t regular=in.pendingLCRNGState;
    if(f==Family::RubySapphire || f==Family::FireRedLeafGreen) {
        // RS _TriggerPendingDaycareEgg and FRLG
        // _TriggerPendingDaycareEgg: (Random() % 0xfffe) + 1.
        const uint16_t low=static_cast<uint16_t>(
            (nextRNG(regular)%0xFFFEu)+1u);
        out.pendingRNGDraws=1;
        out.finalPendingLCRNGState=regular;
        out.pendingPersonality=low;
        // RS and FRLG SetInitialEggData: saved pending LOW 16 bits
        // OR'd with one independent, collection-stage Random() << 16.
        if(!in.collectionLCRNGStateIndependentlyVerified) {
            out.status=Status::UnknownCollectionRNG;return out;
        }
        uint32_t collection=in.collectionLCRNGState;
        const uint32_t high=nextRNG(collection);
        out.collectionRNGDraws=1;
        out.finalCollectionLCRNGState=collection;
        out.finalPersonality=(high<<16)|low;
    } else {
        // Emerald original is categorically different from RS/FRLG:
        // SeedRng2(gMain.vblankCounter2); female, then Ditto preferred.
        // Everstone checked ONLY for this original nature donor.
        int8_t donor=-1;
        for(uint8_t i=0;i<2;++i)
            if(in.physicalSlots[i].gender==1)donor=static_cast<int8_t>(i);
        for(uint8_t i=0;i<2;++i)
            if(in.physicalSlots[i].species==132u)donor=static_cast<int8_t>(i);
        if(donor<0) {
            out.status=Status::IncompatibleOriginalDaycarePair;return out;
        }
        const Parent& d=in.physicalSlots[static_cast<uint8_t>(donor)];
        if(!d.originalHeldItemIndependentlyVerified) {
            out.status=Status::UnknownCreationItems;return out;
        }
        if(!in.originalEmeraldVBlankCounterIndependentlyVerified) {
            out.status=Status::UnknownEverstoneVBlankSeed;return out;
        }
        uint32_t secondary=in.originalEmeraldVBlankCounter;
        // Original Emerald short-circuits the RNG if no Everstone.
        const bool activated=d.heldEverstoneAtCreation &&
                             nextRNG(regular)<(65535u/2u);
        if(d.heldEverstoneAtCreation)out.pendingRNGDraws=1;
        out.natureInheritanceActivated=activated;
        out.originalEverstoneDonor=donor;
        if(!activated) {
            // Original pending creation no-inheritance:
            // (Random2()<<16) | ((Random()%0xfffe)+1).
            const uint32_t high=nextRNG(secondary);
            const uint32_t low=(nextRNG(regular)%0xFFFEu)+1u;
            ++out.pendingRNGDraws;
            out.finalPersonality=(high<<16)|low;
            out.originalNatureAttempts=1;
        } else {
            const uint8_t desired=nature(d.originalPersonality);
            // Source do/while attempts 2401 times maximum; on last
            // failure the last candidate is retained, not Invalid.
            for(uint16_t attempt=1;attempt<=2401;++attempt) {
                const uint32_t high=nextRNG(secondary);
                const uint32_t low=nextRNG(regular);
                const uint32_t pid=(high<<16)|low;
                ++out.pendingRNGDraws;
                out.finalPersonality=pid;
                out.originalNatureAttempts=attempt;
                if(pid && nature(pid)==desired)break;
                if(attempt==2401)out.natureSearchExhausted=true;
            }
        }
        out.pendingPersonality=out.finalPersonality;
        out.finalPendingLCRNGState=regular;
        out.finalSecondRNGState=secondary;
        // Emerald collection SetInitialEggData simply uses the
        // original FULL saved pending PID; no new high-half draw.
    }
    out.status=out.finalPersonality==in.originalUnhatchedEggPID?
        Status::ConditionallyMatchedOriginalEggPID:
        Status::OriginalEggPIDContradicted;
    return out;
}
static_assert(family("ruby_gba")==Family::RubySapphire);
static_assert(family("emerald_gba")==Family::Emerald);
static_assert(family("leafgreen_gba")==Family::FireRedLeafGreen);
} // namespace Legality::Gen3EggPIDCreationHistory
