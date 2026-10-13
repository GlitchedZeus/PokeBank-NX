#pragma once
// L4 Gen I: strict native read-only gift/trade/event comparison, not L1 wild
// encounters. Original source attestation and historical receipt are external.
// No Gen I ribbons, abilities, gender, nature, eggs, held items or modern ball.
#include "Legality/Gen12OriginalDistributionJoinEvidence.h"
#include "Legality/Gen1CatchRateEvidence.h"
#include "Legality/Gen1OriginalGiftNativePreflight.h"
#include "Legality/Gen1OriginalNPCTradeMetadata.h"
#include <array>
#include <cstdint>
#include <span>

namespace Legality::Gen1OriginalGiftNativeEvidence {
namespace J=Gen12OriginalDistributionJoinEvidence;
namespace G=Gen12GiftHistoryEvidence;
namespace Rate=Gen1CatchRate;
enum class SourceClass : uint8_t { Unknown, OriginalCartridgeMechanic,
    OriginalStadiumMedia, ModernUnverifiedListing };
enum class Status : uint8_t { UnknownNative, UnknownSource, UnknownCartridgeVersion,
    UnknownOriginalNPCTrade, UnknownOriginalNameBytes, UnknownHistoricalRoute,
    UnknownCatchRateHistory, CandidateExcluded, ConditionalCandidate };
enum Field : uint32_t { CatchRate=1u<<20, NPCTradeInput=1u<<21 };
struct NameWitness {
    // Source native save's original game-font name bytes, separately extracted
    // from the read-only save; PokemonRecord's UTF-8 names are not raw bytes.
    bool exactOriginalSaveBytesIndependentlyVerified=false;
    bool rawNamesIndependentlyExtractedFromThisRecord=false;
    bool originalEncodingIndependentlyReconciled=false;
    G::Name rawOT{},rawNickname{};
};
struct CatchHistory {
    bool originalAcquisitionRateIndependentlyAttested=false;
    uint8_t originallyObservedRate=0;
    // Exactly one of preserved or a complete original Time Capsule round-trip.
    bool originalBytePreservedThroughCompleteHistory=false;
    bool originalTimeCapsuleRoundTripIndependentlyVerified=false;
    bool returnedHeldItemByteIndependentlyVerified=false;
    uint8_t observedTradebackHeldItemByte=0;
};
struct Source {
    J::Candidate hypothesis{};  // includes pinned original distribution archives
    SourceClass classification=SourceClass::Unknown;
    bool originalCartridgeScriptIndependentlyPinned=false;
    bool originalStadiumMediaMechanismIndependentlyPinned=false;
    // A historical acquisition record must link this original gift/trade
    // to this individual, not merely document a possible game mechanic.
    bool originalAcquisitionContextIndependentlyVerified=false;
    bool originalIndividualRecordLinkedToNativeIndependentlyVerified=false;
    bool exactOriginalNPCTradeRowIndependentlyPinned=false;
    bool englishRetailTradeTableIndependentlyVerified=false;
    // Gen I NPC trade OTID is generated from Random; it is NOT in TradeMons.
    bool individualRandomNPCTrainerIdReceiptIndependentlyVerified=false;
    uint8_t originalNPCTradeRowIndex=255;
    bool originalOfferedSpeciesIndependentlyVerified=false;
    uint16_t originalOfferedSpecies=0;
    bool observedOfferedSpeciesIndependentlyVerified=false;
    uint16_t observedOfferedSpecies=0;
    bool sourceSaveEditionIndependentlyVerified=false;
    G::Game originalSaveEdition=G::Game::Unknown;
    NameWitness names{};
    CatchHistory catchHistory{};
};
struct Result {
    Status status=Status::UnknownNative;
    J::Status giftStatus=J::Status::UnknownSource;
    Rate::Evidence catchRateClassification=Rate::Evidence::Unknown;
    uint32_t contradicted=0,missing=0;
    bool candidateOnly=true,authenticated=false,globallyInvalid=false;
    bool originalDistributionAuthenticated=false,payloadEmbeddingPermitted=false;
};
constexpr bool originalGen1Kind(G::Kind kind) noexcept {
    return kind==G::Kind::Gift || kind==G::Kind::Static ||
           kind==G::Kind::NPCTrade || kind==G::Kind::StadiumGift;
}
// The native object is the actual already-validated PK1 PokemonRecord from
// Integration/Gen1/Gen1ReadOnlySave.h. The template avoids copying/reparsing
// its native body, and makes no guess about its absent version/name bytes.
template<class NativeRecord>
constexpr Result evaluate(const NativeRecord& native,bool independentlyValidated,
    const Source& source) noexcept {
    Result result{};
    if(!independentlyValidated ||
       !Gen1OriginalGiftNativePreflight::bounded(native.species,native.level,native.moves,native.dvs,native.ppUps))
        return result;
    const auto& hypothesis=source.hypothesis;
    const auto& row=hypothesis.original;
    if(!G::isGen1(row.game) || !originalGen1Kind(row.kind) ||
       source.classification==SourceClass::ModernUnverifiedListing ||
       source.classification==SourceClass::Unknown ||
       !Gen1OriginalGiftNativePreflight::verifiedOriginalMechanism(
           row.kind==G::Kind::StadiumGift,
           source.originalCartridgeScriptIndependentlyPinned,
           source.originalStadiumMediaMechanismIndependentlyPinned) ||
       !source.originalAcquisitionContextIndependentlyVerified ||
       !source.originalIndividualRecordLinkedToNativeIndependentlyVerified ||
       (row.kind==G::Kind::StadiumGift)!=
           (source.classification==SourceClass::OriginalStadiumMedia)) {
        result.status=Status::UnknownSource;return result;
    }
    result.status=Status::UnknownCartridgeVersion;
    if(!source.sourceSaveEditionIndependentlyVerified ||
       !G::isGen1(source.originalSaveEdition))return result;
    if(row.kind==G::Kind::NPCTrade) {
        result.status=Status::UnknownOriginalNPCTrade;
        if(!source.exactOriginalNPCTradeRowIndependentlyPinned ||
           !source.originalOfferedSpeciesIndependentlyVerified ||
           !source.observedOfferedSpeciesIndependentlyVerified ||
           source.originalOfferedSpecies==0 ||source.originalOfferedSpecies>151 ||
           source.observedOfferedSpecies==0 ||source.observedOfferedSpecies>151 ||
           !hypothesis.route.offeredTradeLevel.known ||
           !source.englishRetailTradeTableIndependentlyVerified ||
           (row.trainerId.known &&
            !source.individualRandomNPCTrainerIdReceiptIndependentlyVerified))return result;
        const auto edition=row.game==G::Game::Yellow?
            Gen1OriginalNPCTradeMetadata::Edition::EnglishYellow:
            Gen1OriginalNPCTradeMetadata::Edition::EnglishRedBlue;
        const auto documented=Gen1OriginalNPCTradeMetadata::row(
            edition,source.originalNPCTradeRowIndex);
        if(!documented.reachable || documented.offered!=source.originalOfferedSpecies ||
           documented.received!=row.species.value)return result;
        // A claimed original nickname MUST be the version-specific fixed
        // retail trade name in the independently attested English charset.
        // Never substitute a modern listing's Unicode spelling.
        if(!source.names.originalEncodingIndependentlyReconciled ||
           !row.nickname.known)return result;
        const auto scripted=Gen1OriginalNPCTradeMetadata::encodeEnglishNickname(
            documented.originalNickname);
        if(!scripted.valid || row.nickname.value!=scripted.bytes)return result;
        if(source.originalOfferedSpecies!=source.observedOfferedSpecies)
            result.contradicted|=NPCTradeInput;
    }
    G::Observation observation{};
    observation.game=source.originalSaveEdition;
    observation.species={true,native.species};
    observation.level={true,native.level};
    observation.trainerId={true,native.trainerId};
    for(size_t i=0;i<4;++i)observation.moves.value[i]=native.moves[i];
    observation.moves.known=true;
    auto compared=hypothesis;
    const auto& n=source.names;
    const bool raw=n.exactOriginalSaveBytesIndependentlyVerified &&
       n.rawNamesIndependentlyExtractedFromThisRecord &&
       n.originalEncodingIndependentlyReconciled;
    if(raw) {
        observation.trainerName={true,n.rawOT};
        observation.nickname={true,n.rawNickname};
    }
    compared.originalRawCharsetLinkIndependentlyVerified=raw &&
        hypothesis.originalRawCharsetLinkIndependentlyVerified;
    result.status=Status::UnknownOriginalNameBytes;
    const auto joined=J::evaluate(observation,compared);
    result.giftStatus=joined.status;
    if(joined.status==J::Status::CandidateExcluded) {
        result.contradicted|=joined.gift.contradicted;
    } else if(joined.status!=J::Status::ConditionalCandidate) {
        result.status=Status::UnknownHistoricalRoute;return result;
    }
    // Catch-rate byte in PK1 is ambiguous after a Time Capsule round-trip;
    // species-personal catch rates do not prove this specific gift's origin.
    const auto& history=source.catchHistory;
    result.catchRateClassification=Rate::classify(
        row.game==G::Game::Yellow?"yellow_gb":"red_gb",
        native.species,native.catchRate);
    const Gen1OriginalGiftNativePreflight::RateHistory rate{
        history.originalAcquisitionRateIndependentlyAttested,
        history.originallyObservedRate,
        history.originalBytePreservedThroughCompleteHistory,
        history.originalTimeCapsuleRoundTripIndependentlyVerified,
        history.returnedHeldItemByteIndependentlyVerified,
        compared.route.timeCapsule==G::Crossing::SnapshotVerified &&
            compared.route.crossingSpecies.known &&
            compared.route.crossingMoves.known &&
            G::legalSpeciesAtCrossing(compared.route.crossingSpecies.value) &&
            G::legalMovesAtCrossing(compared.route.crossingMoves.value),
        Rate::isPossibleTimeCapsuleHeldItem(history.observedTradebackHeldItemByte),
        history.observedTradebackHeldItemByte};
    const auto verdict=Gen1OriginalGiftNativePreflight::compareRate(native.catchRate,rate);
    if(verdict==Gen1OriginalGiftNativePreflight::RateStatus::Unknown)
        result.missing|=CatchRate;
    else if(verdict==Gen1OriginalGiftNativePreflight::RateStatus::OriginalRateContradicted ||
            verdict==Gen1OriginalGiftNativePreflight::RateStatus::TradebackItemContradicted)
        result.contradicted|=CatchRate;
    if(result.contradicted)result.status=Status::CandidateExcluded;
    else if(result.missing)result.status=Status::UnknownCatchRateHistory;
    else result.status=Status::ConditionalCandidate;
    return result;
}
static_assert(!originalGen1Kind(G::Kind::PCNYGift));
static_assert(!Result{}.authenticated && !Result{}.globallyInvalid);
} // namespace Legality::Gen1OriginalGiftNativeEvidence
