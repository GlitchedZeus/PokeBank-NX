#pragma once
// L2 B13 — Diamond/Pearl original daycare egg PID creation AND collection.
// Independently derived from pret/pokediamond@b623abef1b4dfe204ca50aab42447ac00c134290
// arm9/overlays/05/asm/ov05_021EC458.s:
//   ov05_021EC8A8  lines 529-610  female/Ditto Everstone coin
//   ov05_021EC944  lines 612-652  pending MT egg PID and 2401 fallback
//   ov05_Daycare_CreateEgg lines 1482-1530, language and PRandom rerolls.
// Source math: arm9/src/math_util.c LCRandom/PRandom/MTRandom.
// This requires independent ORIGINAL-stage snapshots, consecutive MT
// outputs and same-event identity; never infer them from a detached egg.
#include <array>
#include <cstdint>
#include <span>
#include <string_view>

namespace Legality::Gen4DiamondPearlEggPidHistory {
enum class Status : uint8_t {
    UnsupportedGame, UnknownEligibleDaycarePair, IncompatibleDaycarePair,
    UnknownParentCreationSnapshots, UnknownPhysicalSlotOrder,
    UnknownCreationHeldItems, UnknownSourceStage, IncoherentSourceStage,
    UnknownLCRNGState, UnknownParentNature,
    UnknownMTTranscript, IncompleteMTTranscript, MalformedMTTranscript,
    MTConsumptionContradicted, UnknownCollectionLanguages,
    UnknownCollectionTrainer, UnknownOriginalChildPID,
    OriginalEggPIDContradicted, ConditionalOriginalEggPIDMatched
};
struct Parent {
    uint16_t species=0;
    // 0 male, 1 female, 2 genderless, exactly as original gender input.
    uint8_t gender=2;
    uint32_t pid=0;
    uint16_t heldItem=0;
    bool pidIndependentlyVerified=false;
};
struct Input {
    std::string_view game{};
    bool originalDaycarePairEligibilityVerified=false;
    bool eligibleOrdinaryDaycarePair=false;
    bool originalParentSnapshotsVerified=false;
    bool originalDaycareSlotOrderVerified=false;
    bool creationTimeItemsIndependentlyVerified=false;
    std::array<Parent,2> parents{};
    bool creationStageIndependentlyVerified=false;
    bool collectionStageIndependentlyVerified=false;
    uint64_t parentStageToken=0,mtStageToken=0,collectionStageToken=0;
    bool creationLCRNGStateIndependentlyVerified=false;
    uint32_t creationLCRNGState=0;
    // Exhaustive consecutive MT outputs for THIS candidate's branch,
    // not unrelated PID values sampled from a present-day egg.
    bool consecutiveMTOutputsIndependentlyVerified=false;
    bool exactMTConsumptionIndependentlyVerified=false;
    std::span<const uint32_t> mtOutputs{};
    bool originalParentLanguagesIndependentlyVerified=false;
    std::array<uint8_t,2> originalParentLanguages{};
    bool originalCollectionTrainerIdIndependentlyVerified=false;
    uint32_t originalCollectionTrainerId=0; // full TID/SID.
    bool originalUnhatchedEggPidIndependentlyVerified=false;
    uint32_t originalUnhatchedEggPid=0;
};
struct Result {
    Status status=Status::UnknownEligibleDaycarePair;
    uint32_t pendingPid=0,originalFinalPid=0;
    uint32_t finalLCRNGState=0;
    uint16_t mtConsumed=0;
    uint8_t lcConsumed=0,internationalRerolls=0;
    int8_t everstoneDonor=-1;
    bool everstoneNatureProcced=false,retailMTFallback=false;
};
constexpr bool supports(std::string_view game) noexcept {
    return game=="diamond_nds"||game=="pearl_nds";
}
constexpr uint16_t lc(uint32_t& seed,Result& out) noexcept {
    seed=seed*0x41C64E6Du+0x6073u;
    ++out.lcConsumed;
    return static_cast<uint16_t>(seed>>16);
}
constexpr uint32_t prandom(uint32_t personality) noexcept {
    return personality*1812433253u+1u;
}
constexpr bool shiny(uint32_t fullOtId,uint32_t personality) noexcept {
    // Original CalcShininessByOtIdAndPersonality / SHINY_CHECK.
    return ((fullOtId&65535u)^(fullOtId>>16)^
            (personality&65535u)^(personality>>16))<8u;
}
constexpr Result evaluate(const Input& in) noexcept {
    Result out{};
    if(!supports(in.game)){out.status=Status::UnsupportedGame;return out;}
    if(!in.originalDaycarePairEligibilityVerified)return out;
    if(!in.eligibleOrdinaryDaycarePair) {
        out.status=Status::IncompatibleDaycarePair;return out;
    }
    if(!in.originalParentSnapshotsVerified) {
        out.status=Status::UnknownParentCreationSnapshots;return out;
    }
    if(!in.originalDaycareSlotOrderVerified) {
        out.status=Status::UnknownPhysicalSlotOrder;return out;
    }
    if(!in.creationTimeItemsIndependentlyVerified) {
        out.status=Status::UnknownCreationHeldItems;return out;
    }
    if(!in.creationStageIndependentlyVerified ||
       !in.collectionStageIndependentlyVerified ||
       !in.parentStageToken || !in.mtStageToken || !in.collectionStageToken) {
        out.status=Status::UnknownSourceStage;return out;
    }
    if(in.parentStageToken!=in.mtStageToken ||
       in.parentStageToken!=in.collectionStageToken) {
        out.status=Status::IncoherentSourceStage;return out;
    }
    uint32_t rng=in.creationLCRNGState;
    // ov05_021EC8A8: latest female seen (if any), then Ditto takes
    // precedence; double-Ditto coin is retained even though a bona
    // fide eligible ordinary pair must be separately attested.
    int8_t donor=-1;
    uint8_t dittoCount=0;
    for(uint8_t i=0;i<2;++i) {
        if(in.parents[i].gender==1)donor=static_cast<int8_t>(i);
    }
    for(uint8_t i=0;i<2;++i) {
        if(in.parents[i].species==132) {
            donor=static_cast<int8_t>(i);
            ++dittoCount;
        }
    }
    if(dittoCount==2) {
        if(!in.creationLCRNGStateIndependentlyVerified) {
            out.status=Status::UnknownLCRNGState;return out;
        }
        donor=lc(rng,out)<0x7FFFu?1:0;
    }
    if(donor>=0 && in.parents[static_cast<uint8_t>(donor)].heldItem==229) {
        if(!in.creationLCRNGStateIndependentlyVerified) {
            out.status=Status::UnknownLCRNGState;return out;
        }
        if(lc(rng,out)<0x7FFFu) {
            if(!in.parents[static_cast<uint8_t>(donor)].pidIndependentlyVerified) {
                out.status=Status::UnknownParentNature;return out;
            }
            out.everstoneDonor=donor;
            out.everstoneNatureProcced=true;
        }
    }
    out.finalLCRNGState=rng;
    if(!in.consecutiveMTOutputsIndependentlyVerified ||
       !in.exactMTConsumptionIndependentlyVerified ||
       in.mtOutputs.empty()) {
        out.status=Status::UnknownMTTranscript;return out;
    }
    if(in.mtOutputs.size()>2401) {
        out.status=Status::MalformedMTTranscript;return out;
    }
    bool found=false;
    for(uint32_t pid:in.mtOutputs) {
        ++out.mtConsumed;
        const bool accepted=!out.everstoneNatureProcced ||
            (pid!=0 && pid%25u==
                in.parents[static_cast<uint8_t>(out.everstoneDonor)].pid%25u);
        if(accepted || out.mtConsumed==2401u) {
            out.pendingPid=pid;
            out.retailMTFallback=!accepted;
            found=true;
            if(out.mtConsumed!=in.mtOutputs.size()) {
                out.status=Status::MTConsumptionContradicted;return out;
            }
            break;
        }
    }
    if(!found) {
        out.status=Status::IncompleteMTTranscript;return out;
    }
    // ov05_Daycare_CreateEgg compares parent language bytes at
    // COLLECTION, not present-day trainer region labels.
    if(!in.originalParentLanguagesIndependentlyVerified) {
        out.status=Status::UnknownCollectionLanguages;return out;
    }
    if(!in.originalCollectionTrainerIdIndependentlyVerified) {
        out.status=Status::UnknownCollectionTrainer;return out;
    }
    uint32_t final=out.pendingPid;
    if(in.originalParentLanguages[0]!=in.originalParentLanguages[1] &&
       !shiny(in.originalCollectionTrainerId,final)) {
        // Exactly 4 PRandom attempts, stopping early when shiny.
        for(uint8_t j=0;j<4;++j) {
            final=prandom(final);
            ++out.internationalRerolls;
            if(shiny(in.originalCollectionTrainerId,final))break;
        }
    }
    out.originalFinalPid=final;
    if(!in.originalUnhatchedEggPidIndependentlyVerified) {
        out.status=Status::UnknownOriginalChildPID;return out;
    }
    out.status=final==in.originalUnhatchedEggPid?
        Status::ConditionalOriginalEggPIDMatched:
        Status::OriginalEggPIDContradicted;
    return out;
}
} // namespace Legality::Gen4DiamondPearlEggPidHistory
