#pragma once
// L1 Stage 2: original G/S/C NPC trade template evaluator.
// The 11 published PKHeX-derived source templates have no independently
// pinned OT/nickname text in this table. Therefore even a complete candidate
// can only corroborate a LOCAL template, never certify retail provenance.
// Other gifts, Time Capsule, wild, egg and later-evolution routes survive.
#include "Legality/Gen2TradeEvidence.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <string_view>

namespace Legality::Gen2OriginalNpcTradeStage {
enum class Status : uint8_t {
    UnknownOriginalGame, UnknownOriginalRow, UnknownOriginalTrade,
    UnknownOriginalSpecies, UnknownOriginalLevel, UnknownOriginalTrainer,
    UnknownOriginalDVs, UnknownOriginalCaughtData, UnknownPresentSpecies,
    UnknownLaterEvolutionOrTimeCapsule, CandidateOriginalSpeciesContradicted,
    CandidateMinimumLevelContradicted, CandidateFixedTrainerContradicted,
    CandidateFixedDVsContradicted, CandidateCaughtDataContradicted,
    ConditionalOriginalTradeTemplateOnly
};
struct Candidate {
    std::string_view exactOriginalGame{};
    std::size_t originalTradeRow=0;
    uint16_t originalSpecies=0, presentSpecies=0;
    uint8_t originalLevel=0;
    uint16_t originalTrainerId=0, originalCrystalCaughtData=0;
    std::array<uint8_t,5> originalDVs{}; // HP, Atk, Def, Spe, Special
    bool originalGameIndependentlyAttested=false;
    bool originalTradeRowIndependentlyAttested=false;
    bool originalReceivedSpeciesAndLevelIndependentlyAttested=false;
    bool originalTradeOtAndTrainerIndependentlyAttested=false;
    bool originalDvsIndependentlyAttested=false;
    bool originalCrystalCaughtDataIndependentlyAttested=false;
    bool noLaterEvolutionOrTimeCapsuleIndependentlyAttested=false;
};
struct Result {
    Status status=Status::UnknownOriginalGame;
    uint16_t templateSpecies=0;
    uint8_t templateMinimumLevel=0;
    bool fixedTrainerAndDvsAvailable=false;
};
inline Result evaluate(const Candidate& c) noexcept {
    if(!c.originalGameIndependentlyAttested ||
       !Gen2Trade::isGen2Game(c.exactOriginalGame))return {};
    if(!c.originalTradeRowIndependentlyAttested ||
       c.originalTradeRow>=std::size(Gen2Trade::kPackedGen2Trades))
        return {Status::UnknownOriginalRow};
    const uint64_t row=Gen2Trade::kPackedGen2Trades[c.originalTradeRow];
    const uint16_t species=Gen2Trade::species(row);
    const uint8_t minimum=Gen2Trade::level(row);
    const bool fixed=Gen2Trade::hasFixedDvs(row);
    const auto result=[=](Status status) {
        return Result{status,species,minimum,fixed};
    };
    if(!c.originalReceivedSpeciesAndLevelIndependentlyAttested)
        return result(Status::UnknownOriginalTrade);
    if(c.originalSpecies==0 || c.originalSpecies>251)
        return result(Status::UnknownOriginalSpecies);
    if(!c.originalLevel || c.originalLevel>100)
        return result(Status::UnknownOriginalLevel);
    if(!c.originalTradeOtAndTrainerIndependentlyAttested)
        return result(Status::UnknownOriginalTrainer);
    // Source fixed DVs and trainer IDs can be negative witnesses only if the
    // actual original received record was separately preserved and decoded.
    if(fixed) {
        if(!c.originalDvsIndependentlyAttested)
            return result(Status::UnknownOriginalDVs);
        for(uint8_t dv:c.originalDVs)
            if(dv>15) return result(Status::UnknownOriginalDVs);
    }
    const bool crystal=c.exactOriginalGame=="crystal_gbc";
    if(crystal && !c.originalCrystalCaughtDataIndependentlyAttested)
        return result(Status::UnknownOriginalCaughtData);
    if(crystal && c.originalCrystalCaughtData==0)
        return result(Status::UnknownOriginalCaughtData);
    if(c.originalSpecies!=species)
        return result(Status::CandidateOriginalSpeciesContradicted);
    if(c.originalLevel<minimum)
        return result(Status::CandidateMinimumLevelContradicted);
    if(fixed) {
        if(c.originalTrainerId!=Gen2Trade::tid(row))
            return result(Status::CandidateFixedTrainerContradicted);
        if(c.originalDVs[1]!=Gen2Trade::atkDv(row) ||
           c.originalDVs[2]!=Gen2Trade::defDv(row) ||
           c.originalDVs[3]!=Gen2Trade::speDv(row) ||
           c.originalDVs[4]!=Gen2Trade::specialDv(row))
            return result(Status::CandidateFixedDVsContradicted);
    }
    // Gold/Silver do not need a surviving Crystal-only caught-data field.
    if(crystal && (Gen2Trade::caughtMetLocation(c.originalCrystalCaughtData)!=
                   Gen2Trade::kNpcTradeLocation ||
                   Gen2Trade::caughtMetLevel(c.originalCrystalCaughtData)!=0))
        return result(Status::CandidateCaughtDataContradicted);
    if(c.presentSpecies==0)return result(Status::UnknownPresentSpecies);
    // Original gen-2 received species, and modern present species, are NOT
    // freely interchangeable. Missing evolution/Time Capsule chains stay
    // Unknown; this helper has no exhaustive chain graph.
    if(c.presentSpecies!=species ||
       !c.noLaterEvolutionOrTimeCapsuleIndependentlyAttested)
        return result(Status::UnknownLaterEvolutionOrTimeCapsule);
    return result(Status::ConditionalOriginalTradeTemplateOnly);
}
static_assert(std::size(Gen2Trade::kPackedGen2Trades)==11);
} // namespace Legality::Gen2OriginalNpcTradeStage
