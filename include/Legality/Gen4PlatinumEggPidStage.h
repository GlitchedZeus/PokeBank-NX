#pragma once
// RNG-005 / BREED-005: source-attested *Platinum-only* ordinary daycare
// PID-creation candidate stage (MT output selection + conditional
// Everstone nature + four bounded Masuda ARNG transformations).
// Detached PK4 does not retain original MT state, daycare LCRNG flip,
// parent item history, or language provenance. This helper NEVER
// reconstructs those inputs from the child's final nature or PID.
// No single candidate rejection is global Invalid.
#include "Pokemon/Pokemon4ReadOnly.h"
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace Legality::Gen4PlatinumEggPidStage {
constexpr uint16_t kEverstone=229; // retail Gen IV ITEM_EVERSTONE
constexpr uint16_t kDitto=132;
constexpr uint8_t kHistoryBound=64;
enum class Status : uint8_t {
    UnsupportedGame, UnknownOriginalEgg, UnknownSourceGame,
    UnknownHistoricalParents, UnknownPairCompatibility,
    UnknownHistoricalStage, IncoherentHistoricalStage,
    UnknownNatureDonor, UnknownHeldItemHistory,
    UnknownEverstoneProc, UnknownParentLanguageHistory,
    UnknownMTDrawHistory, SearchBoundExceededUnknown,
    CandidatePidContradicted, KnownConditionalPidHistory
};
struct Witness {
    std::string_view game{};
    const Pokemon::Pokemon4ReadOnly* physicalSlot0=nullptr;
    const Pokemon::Pokemon4ReadOnly* physicalSlot1=nullptr;
    const Pokemon::Pokemon4ReadOnly* originalUnhatchedEgg=nullptr;
    // These independent IDs all name the SAME archived daycare PID
    // creation event, not merely similarly dated current saves.
    uint64_t parentPairToken=0,mtCreationToken=0,eggCreationToken=0;
    bool parentSnapshotsIndependentlyVerified=false;
    bool parentalItemHistoryIndependentlyVerified=false;
    bool physicalDaycareOrderIndependentlyVerified=false;
    bool pairCompatibilityIndependentlyVerified=false;
    bool originalEggSourceIndependentlyVerified=false;
    bool originalEggPidUnchangedIndependentlyVerified=false;
    bool originalDaycareTrainerIdIndependentlyVerified=false;
    uint32_t originalDaycareTrainerId32=0;
    bool parentsLanguageHistoryIndependentlyVerified=false;
    bool parentsHaveDifferentLanguages=false;
    // Needed ONLY when the selected nature donor held Everstone.
    bool creationLCRngRollIndependentlyVerified=false;
    uint16_t creationLCRngRoll=0;
    // Outputs MUST be the exact consecutive MT values, beginning at
    // the original daycare PID creation boundary; never unrelated
    // random samples or brute-forced nature-compatible numbers.
    bool consecutiveMTOutputsIndependentlyVerified=false;
    bool exactConsumedMTCountIndependentlyVerified=false;
    std::array<uint32_t,kHistoryBound> mtDraws{};
    uint8_t mtCount=0;
};
struct Result {
    Status status=Status::UnknownOriginalEgg;
    uint32_t selectedUnadjustedPid=0,retainedEggPid=0;
    uint8_t consumedMTDraws=0,internationalArngSteps=0;
    int8_t selectedEverstoneSlot=-1;
    bool everstoneApplied=false;
};
constexpr uint32_t nextArng(uint32_t pid) noexcept {
    return static_cast<uint32_t>(pid*0x6C078965u+1u);
}
constexpr bool shinyForTrainer(uint32_t pid,uint32_t trainerId32) noexcept {
    return (((pid>>16)^(pid&0xFFFFu)^(trainerId32>>16)^
             (trainerId32&0xFFFFu))<8u);
}
struct InternationalPid {
    uint32_t pid=0;
    uint8_t steps=0;
};
constexpr InternationalPid languageAdjustedPid(uint32_t initialPid,
                                                uint32_t trainerId32,
                                                bool differingLanguages) noexcept {
    InternationalPid out{initialPid,0};
    if(!differingLanguages || shinyForTrainer(initialPid,trainerId32))
        return out;
    // Platinum's Egg_SetInitialData: four applications at most.
    for(uint8_t i=0;i<4;++i) {
        out.pid=nextArng(out.pid);++out.steps;
        if(shinyForTrainer(out.pid,trainerId32))break;
    }
    return out;
}
constexpr int8_t natureDonor(uint16_t species0,uint8_t gender0,
                             uint16_t species1,uint8_t gender1) noexcept {
    // Retail Platinum Daycare_GetParentToInheritNature scans both
    // physical slots for a female (last selected), then Ditto (last
    // selected), overriding female. Ditto+Ditto cannot breed.
    if(species0==kDitto && species1==kDitto)return -1;
    if(species1==kDitto)return 1;
    if(species0==kDitto)return 0;
    if(gender1==1)return 1;
    if(gender0==1)return 0;
    return -1;
}
inline Result evaluate(const Witness& w) noexcept {
    Result out{};
    if(w.game!="platinum_nds") {
        out.status=Status::UnsupportedGame;return out;
    }
    if(!w.originalUnhatchedEgg ||
       !w.originalEggSourceIndependentlyVerified ||
       !w.originalEggPidUnchangedIndependentlyVerified ||
       !w.originalUnhatchedEgg->valid() ||
       w.originalUnhatchedEgg->empty() ||
       !w.originalUnhatchedEgg->isEgg())return out;
    if(w.originalUnhatchedEgg->sourceGroup()!=Enums::GameVersion::PT) {
        out.status=Status::UnknownSourceGame;return out;
    }
    if(!w.physicalSlot0 || !w.physicalSlot1 ||
       !w.parentSnapshotsIndependentlyVerified ||
       !w.physicalDaycareOrderIndependentlyVerified ||
       !w.physicalSlot0->valid() || !w.physicalSlot1->valid() ||
       w.physicalSlot0->empty() || w.physicalSlot1->empty() ||
       w.physicalSlot0->isEgg() || w.physicalSlot1->isEgg() ||
       w.physicalSlot0->sourceGroup()!=Enums::GameVersion::PT ||
       w.physicalSlot1->sourceGroup()!=Enums::GameVersion::PT) {
        out.status=Status::UnknownHistoricalParents;return out;
    }
    if(!w.pairCompatibilityIndependentlyVerified) {
        out.status=Status::UnknownPairCompatibility;return out;
    }
    if(!w.parentPairToken || !w.mtCreationToken || !w.eggCreationToken) {
        out.status=Status::UnknownHistoricalStage;return out;
    }
    if(w.parentPairToken!=w.mtCreationToken ||
       w.parentPairToken!=w.eggCreationToken) {
        out.status=Status::IncoherentHistoricalStage;return out;
    }
    const auto& slot0=*w.physicalSlot0;
    const auto& slot1=*w.physicalSlot1;
    const int8_t selected=natureDonor(slot0.species(),slot0.gender(),
                                     slot1.species(),slot1.gender());
    if(selected<0) {
        out.status=Status::UnknownNatureDonor;return out;
    }
    out.selectedEverstoneSlot=selected;
    if(!w.parentalItemHistoryIndependentlyVerified) {
        out.status=Status::UnknownHeldItemHistory;return out;
    }
    const bool isHolder=(selected==0?slot0.heldItem():slot1.heldItem())==kEverstone;
    bool inherited=false;
    if(isHolder) {
        if(!w.creationLCRngRollIndependentlyVerified) {
            out.status=Status::UnknownEverstoneProc;return out;
        }
        // Retail compares the full LCRNG_Next() 16-bit output with
        // (0xffff/2)=32767, so 32767 belongs to the FAILED half.
        inherited=w.creationLCRngRoll<0x7FFFu;
    }
    out.everstoneApplied=inherited;
    if(!w.parentsLanguageHistoryIndependentlyVerified ||
       !w.originalDaycareTrainerIdIndependentlyVerified) {
        out.status=Status::UnknownParentLanguageHistory;return out;
    }
    if(!w.consecutiveMTOutputsIndependentlyVerified ||
       !w.exactConsumedMTCountIndependentlyVerified ||
       !w.mtCount || w.mtCount>kHistoryBound) {
        out.status=Status::UnknownMTDrawHistory;return out;
    }
    uint8_t expected=1;
    if(inherited) {
        const uint32_t parentPid=selected==0?slot0.pid():slot1.pid();
        const uint8_t nature=static_cast<uint8_t>(parentPid%25u);
        bool found=false;
        for(uint8_t i=0;i<w.mtCount;++i) {
            const auto pid=w.mtDraws[i];
            if(pid && pid%25u==nature) {
                expected=static_cast<uint8_t>(i+1u);
                found=true;break;
            }
        }
        if(!found) {
            // Platinum can continue up to 2401 draws. Exhausting
            // our 64-value audit budget cannot refute an original PID.
            out.status=Status::SearchBoundExceededUnknown;return out;
        }
    }
    out.consumedMTDraws=expected;
    if(w.mtCount!=expected) {
        out.status=Status::CandidatePidContradicted;return out;
    }
    out.selectedUnadjustedPid=w.mtDraws[expected-1u];
    const auto adjusted=languageAdjustedPid(
        out.selectedUnadjustedPid,w.originalDaycareTrainerId32,
        w.parentsHaveDifferentLanguages);
    out.retainedEggPid=adjusted.pid;
    out.internationalArngSteps=adjusted.steps;
    out.status=adjusted.pid==w.originalUnhatchedEgg->pid()?
        Status::KnownConditionalPidHistory:Status::CandidatePidContradicted;
    return out;
}
// At most eight independently source-identified daycare PID histories
// competing to explain one exact native encrypted UNHATCHED PK4. A
// contradiction is limited to a fully-attested original MT/LCRNG/PID
// hypothesis; it never eliminates non-daycare original events or other
// unenumerated parental histories.
enum class UniverseStatus : uint8_t {
    NoCandidates, CapacityExceededUnknown, UnknownRootEgg,
    UnknownCandidateIdentity, OneConditionalPidHistory,
    UnknownCandidateProvenance, UnknownPidStageEvidence,
    UnknownCandidateEnumeration, AllAttestedPlatinumPidHistoriesContradicted
};
struct Alternative {
    uint64_t identity=0;
    bool sourceIndependentlyGrounded=false;
    bool fullNegativePidMethodCoverageIndependentlyVerified=false;
    Witness stage{};
};
struct UniverseInput {
    const Pokemon::Pokemon4ReadOnly* originalEgg=nullptr;
    bool originalEggSourceIndependentlyVerified=false;
    bool allPlatinumDaycarePidRoutesIndependentlyEnumerated=false;
    std::array<Alternative,8> alternatives{};
    uint8_t count=0;
};
struct UniverseResult {
    UniverseStatus status=UniverseStatus::NoCandidates;
    uint8_t considered=0,consistent=0,contradicted=0,unresolved=0,unverified=0;
    std::array<Status,8> individual{};
};
inline UniverseResult evaluateUniverse(const UniverseInput& in) noexcept {
    UniverseResult out{};
    if(!in.count)return out;
    if(in.count>in.alternatives.size()) {
        out.status=UniverseStatus::CapacityExceededUnknown;return out;
    }
    if(!in.originalEgg || !in.originalEggSourceIndependentlyVerified ||
       !in.originalEgg->valid() || in.originalEgg->empty() ||
       !in.originalEgg->isEgg() ||
       in.originalEgg->sourceGroup()!=Enums::GameVersion::PT) {
        out.status=UniverseStatus::UnknownRootEgg;return out;
    }
    for(uint8_t i=0;i<in.count;++i) {
        const auto id=in.alternatives[i].identity;
        if(!id) {
            out.status=UniverseStatus::UnknownCandidateIdentity;return out;
        }
        for(uint8_t j=0;j<i;++j)
            if(id==in.alternatives[j].identity) {
                out.status=UniverseStatus::UnknownCandidateIdentity;return out;
            }
    }
    const auto root=in.originalEgg->originalEncryptedBytes();
    for(uint8_t i=0;i<in.count;++i) {
        const auto& alt=in.alternatives[i];
        ++out.considered;
        const auto* candidateEgg=alt.stage.originalUnhatchedEgg;
        if(!alt.sourceIndependentlyGrounded || !candidateEgg ||
           !candidateEgg->valid() || candidateEgg->sourceGroup()!=
           Enums::GameVersion::PT) {
            ++out.unverified;continue;
        }
        const auto bytes=candidateEgg->originalEncryptedBytes();
        if(bytes.size()!=root.size() ||
           !std::equal(bytes.begin(),bytes.end(),root.begin())) {
            ++out.unverified;continue;
        }
        const auto evidence=evaluate(alt.stage);
        out.individual[i]=evidence.status;
        if(evidence.status==Status::KnownConditionalPidHistory) {
            ++out.consistent;continue;
        }
        if(evidence.status==Status::CandidatePidContradicted &&
           alt.fullNegativePidMethodCoverageIndependentlyVerified) {
            ++out.contradicted;continue;
        }
        ++out.unresolved;
    }
    if(out.consistent)out.status=UniverseStatus::OneConditionalPidHistory;
    else if(out.unverified)out.status=UniverseStatus::UnknownCandidateProvenance;
    else if(out.unresolved)out.status=UniverseStatus::UnknownPidStageEvidence;
    else if(!in.allPlatinumDaycarePidRoutesIndependentlyEnumerated)
        out.status=UniverseStatus::UnknownCandidateEnumeration;
    else if(out.contradicted==out.considered)
        out.status=UniverseStatus::AllAttestedPlatinumPidHistoriesContradicted;
    else out.status=UniverseStatus::UnknownPidStageEvidence;
    return out;
}
} // namespace Legality::Gen4PlatinumEggPidStage
