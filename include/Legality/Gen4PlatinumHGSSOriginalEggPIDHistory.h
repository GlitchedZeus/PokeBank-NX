#pragma once
// L2 Stage 3: ORIGINAL Platinum and HGSS ordinary-daycare PID chronology.
// Platinum: pret/pokeplatinum@fe1ea418a7364c93daf0d07ae55e195452d8803c
//   src/overlay005/daycare.c, Daycare_GetParentToInheritNature,
//   Daycare_SetInheritedNature, Egg_SetInitialData.
// HGSS: pret/pokeheartgold@9d8b7591 src/get_egg.c,
//   Daycare_EverstoneCheck, GenerateEggPID, SetBreedEggStats.
// A detached PK4 NEVER attests historical parent items, LCRNG, MT trace,
// trainer OT, or source-parent languages; contradictions are local only.
#include <array>
#include <cstdint>
#include <span>
#include <string_view>
namespace Legality::Gen4PlatinumHGSSOriginalEggPIDHistory {
enum class Family : uint8_t { Unknown, Platinum, HGSS };
constexpr Family family(std::string_view game) noexcept {
    if(game=="platinum_nds")return Family::Platinum;
    if(game=="heartgold_nds"||game=="soulsilver_nds")return Family::HGSS;
    return Family::Unknown;
}
enum class Status : uint8_t {
    UnsupportedGame, UnknownOriginalEgg, UnknownOriginalPair,
    IncompatibleOrdinaryPair, UnknownOriginalParentSnapshots,
    UnknownParentOrder, UnknownCreationItemHistory,
    UnknownSourceChronology, IncoherentSourceChronology,
    UnknownEverstoneLCRNG, UnknownEverstoneParentNature,
    UnknownMTTranscript, OversizedMTTranscript,
    IncompleteMTTranscript, ContradictoryMTConsumption,
    UnknownCollectionLanguages, UnknownCollectionTrainer,
    OriginalEggPIDContradicted, ConditionalOriginalEggPIDMatched
};
struct Parent {
    uint64_t snapshotId=0;
    uint16_t species=0,heldItem=0;
    uint8_t gender=2; // 0 male, 1 female, 2 genderless
    uint32_t originalPID=0;
    bool originalSnapshotIndependentlyVerified=false;
    bool creationItemIndependentlyVerified=false;
    bool originalNaturePIDIndependentlyVerified=false;
};
struct Input {
    std::string_view game{};
    uint64_t originalEggEventId=0;
    bool originalEggIdentityIndependentlyVerified=false;
    uint32_t originalUnhatchedEggPID=0;
    bool ordinaryPairEligibilityIndependentlyVerified=false;
    bool eligibleOrdinaryPair=false;
    bool physicalDepositOrderIndependentlyVerified=false;
    std::array<Parent,2> parents{};
    uint64_t parentStageToken=0,pendingStageToken=0,collectionStageToken=0;
    bool creationAndCollectionStagesIndependentlyVerified=false;
    bool lcrngEntryIndependentlyVerified=false;
    uint32_t lcrngEntryState=0;
    bool consecutiveMTTranscriptIndependentlyVerified=false;
    bool exactMTConsumptionIndependentlyVerified=false;
    std::span<const uint32_t> consecutiveMT{};
    bool parentLanguagesAtCollectionIndependentlyVerified=false;
    std::array<uint8_t,2> originalParentLanguages{};
    bool collectingTrainerFullOTIndependentlyVerified=false;
    uint32_t collectingTrainerFullOT=0;
};
struct Result {
    Status status=Status::UnknownOriginalEgg;
    uint32_t originalPendingPID=0,originalFinalPID=0,finalLCState=0;
    uint16_t mtDrawCount=0;
    uint8_t lcDrawCount=0,internationalRerolls=0;
    int8_t originalEverstoneDonor=-1;
    bool everstoneActivated=false,retailNatureAttemptLimitReached=false;
};
constexpr uint16_t drawLC(uint32_t& s,Result& out) noexcept {
    s=s*0x41C64E6Du+0x6073u;
    ++out.lcDrawCount;
    return static_cast<uint16_t>(s>>16);
}
constexpr uint32_t prandom(uint32_t p) noexcept {
    return static_cast<uint32_t>(p*1812433253u+1u);
}
constexpr bool shiny(uint32_t ot,uint32_t pid) noexcept {
    return ((ot&0xFFFFu)^(ot>>16)^(pid&0xFFFFu)^(pid>>16))<8u;
}
constexpr Result evaluate(const Input& in) noexcept {
    Result out{};
    const Family f=family(in.game);
    if(f==Family::Unknown){out.status=Status::UnsupportedGame;return out;}
    if(!in.originalEggIdentityIndependentlyVerified || !in.originalEggEventId)
        return out;
    if(!in.ordinaryPairEligibilityIndependentlyVerified){
        out.status=Status::UnknownOriginalPair;return out;
    }
    if(!in.eligibleOrdinaryPair){
        out.status=Status::IncompatibleOrdinaryPair;return out;
    }
    if(!in.physicalDepositOrderIndependentlyVerified){
        out.status=Status::UnknownParentOrder;return out;
    }
    for(const auto& p:in.parents) {
        if(!p.originalSnapshotIndependentlyVerified || !p.snapshotId ||
           !p.species || p.species>493 || p.gender>2){
            out.status=Status::UnknownOriginalParentSnapshots;return out;
        }
        if(!p.creationItemIndependentlyVerified){
            out.status=Status::UnknownCreationItemHistory;return out;
        }
    }
    if(in.parents[0].snapshotId==in.parents[1].snapshotId){
        out.status=Status::UnknownOriginalParentSnapshots;return out;
    }
    const bool ditto0=in.parents[0].species==132;
    const bool ditto1=in.parents[1].species==132;
    if((ditto0 && ditto1) ||
       (!ditto0 && !ditto1 &&
        !((in.parents[0].gender==0 && in.parents[1].gender==1) ||
          (in.parents[0].gender==1 && in.parents[1].gender==0)))){
        out.status=Status::IncompatibleOrdinaryPair;return out;
    }
    if(!in.creationAndCollectionStagesIndependentlyVerified ||
       !in.parentStageToken || !in.pendingStageToken || !in.collectionStageToken){
        out.status=Status::UnknownSourceChronology;return out;
    }
    if(in.parentStageToken!=in.pendingStageToken ||
       in.parentStageToken!=in.collectionStageToken ||
       in.parentStageToken!=in.originalEggEventId){
        out.status=Status::IncoherentSourceChronology;return out;
    }
    uint32_t lc=in.lcrngEntryState;
    int8_t donor=-1;
    if(f==Family::Platinum){
        // The LAST female is selected, then the LAST Ditto overrides her.
        for(uint8_t i=0;i<2;++i)
            if(in.parents[i].gender==1)donor=static_cast<int8_t>(i);
        for(uint8_t i=0;i<2;++i)
            if(in.parents[i].species==132)donor=static_cast<int8_t>(i);
        if(donor<0){out.status=Status::IncompatibleOrdinaryPair;return out;}
        if(in.parents[static_cast<uint8_t>(donor)].heldItem==229){
            if(!in.lcrngEntryIndependentlyVerified){
                out.status=Status::UnknownEverstoneLCRNG;return out;
            }
            if(drawLC(lc,out)<0x7FFFu){
                out.originalEverstoneDonor=donor;
                out.everstoneActivated=true;
            }
        }
    } else {
        // HGSS: either original parent may donate an Everstone nature.
        const bool stone0=in.parents[0].heldItem==229;
        const bool stone1=in.parents[1].heldItem==229;
        if(stone0||stone1){
            if(!in.lcrngEntryIndependentlyVerified){
                out.status=Status::UnknownEverstoneLCRNG;return out;
            }
            if(stone0 && stone1)
                donor=static_cast<int8_t>((drawLC(lc,out)&1u)?0:1);
            else donor=stone0?0:1;
            if(drawLC(lc,out)<0x7FFFu){
                out.originalEverstoneDonor=donor;
                out.everstoneActivated=true;
            }
        }
    }
    out.finalLCState=lc;
    if(out.everstoneActivated &&
       !in.parents[static_cast<uint8_t>(out.originalEverstoneDonor)]
          .originalNaturePIDIndependentlyVerified){
        out.status=Status::UnknownEverstoneParentNature;return out;
    }
    if(!in.consecutiveMTTranscriptIndependentlyVerified ||
       !in.exactMTConsumptionIndependentlyVerified ||
       in.consecutiveMT.empty()){
        out.status=Status::UnknownMTTranscript;return out;
    }
    if(in.consecutiveMT.size()>2401){
        out.status=Status::OversizedMTTranscript;return out;
    }
    bool accepted=false;
    for(const uint32_t candidate:in.consecutiveMT){
        ++out.mtDrawCount;
        const bool natureMatches=!out.everstoneActivated ||
            (candidate && candidate%25u==
             in.parents[static_cast<uint8_t>(out.originalEverstoneDonor)].originalPID%25u);
        if(natureMatches || out.mtDrawCount==2401){
            accepted=true;
            out.retailNatureAttemptLimitReached=!natureMatches;
            out.originalPendingPID=candidate;
            if(out.mtDrawCount!=in.consecutiveMT.size()){
                out.status=Status::ContradictoryMTConsumption;return out;
            }
            break;
        }
    }
    if(!accepted){out.status=Status::IncompleteMTTranscript;return out;}
    if(!in.parentLanguagesAtCollectionIndependentlyVerified){
        out.status=Status::UnknownCollectionLanguages;return out;
    }
    if(!in.collectingTrainerFullOTIndependentlyVerified){
        out.status=Status::UnknownCollectionTrainer;return out;
    }
    uint32_t final=out.originalPendingPID;
    if(in.originalParentLanguages[0]!=in.originalParentLanguages[1] &&
       !shiny(in.collectingTrainerFullOT,final)){
        for(uint8_t k=0;k<4;++k){
            final=prandom(final);
            ++out.internationalRerolls;
            if(shiny(in.collectingTrainerFullOT,final))break;
        }
    }
    out.originalFinalPID=final;
    out.status=final==in.originalUnhatchedEggPID ?
        Status::ConditionalOriginalEggPIDMatched:Status::OriginalEggPIDContradicted;
    return out;
}
static_assert(family("platinum_nds")==Family::Platinum);
static_assert(family("soulsilver_nds")==Family::HGSS);
static_assert(family("pearl_nds")==Family::Unknown);
} // namespace Legality::Gen4PlatinumHGSSOriginalEggPIDHistory