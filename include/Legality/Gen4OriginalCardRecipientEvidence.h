#pragma once
// L4 Stage 4: bounded original Gen IV PCD/PGT card -> individual recipient
// save -> native PK4 candidate. Never infer a card or actual receipt from
// matching PGT bytes alone. Every candidate exclusion is local; no authenticity.
#include "Legality/Gen4MysteryGiftContainerEvidence.h"
#include "Legality/Gen4EventClockReceiptEvidence.h"
#include <cstddef>
#include <cstdint>

namespace Legality::Gen4OriginalCardRecipientEvidence {
namespace C=Gen4MysteryGiftContainerEvidence;
namespace Clock=Gen4EventClockReceiptEvidence;
enum class Status : uint8_t {
    UnknownNative, UnknownContainer, NonPokemonDelivery, UnknownOriginalArchive,
    UnknownCardIdentity, CandidateCardMismatch, CandidateGiftTypeMismatch,
    UnknownRecipientSave,
    UnknownReceiptCardLink, CandidateReceiptMismatch, UnknownNativeLink,
    UnknownRangerRecipient, UnknownReceiptClock, CandidateClockMismatch,
    ConditionalCandidate
};
struct CardSource {
    C::Attestation archive{};
    // SHA-256 is computed/verified by an independent archival intake,
    // NEVER calculated from truncated card ID or the matching PGT body.
    C::Digest exactCardArchiveDigest{};
    bool exactCardDigestIndependentlyVerified=false;
    bool originalGiftTypeIndependentlyVerified=false;
    C::GiftType originalGiftType=C::GiftType::None;
    bool cardIdIndependentlyVerified=false;
    uint16_t originalCardId=0;
    bool compatibilityBytesIndependentlyVerified=false;
    uint16_t originalCompatibility=0;
    bool giftBodyLinkedToExactCardIndependentlyVerified=false;
};
struct RecipientEvidence {
    C::Digest originalRecipientSaveDigest{};
    bool originalSaveDigestIndependentlyVerified=false;
    bool originalSaveIdentityIndependentlyVerified=false;
    bool receiptRecordIndependentlyVerified=false;
    bool receiptRecordBoundToOriginalCardIndependentlyVerified=false;
    uint16_t originalReceiptCardId=0;
    bool originalTrainerIdentityIndependentlyVerified=false;
    // This is the recipient save's independently observed slot; it is not
    // the PGT/PCD linkedSlot field and must NEVER be equated with it.
    bool recipientSlotIndependentlyVerified=false;
    bool individualNativePK4LinkedToReceiptIndependentlyVerified=false;
    // Ranger PID/IV and anti-shiny recipient history must have been verified
    // by the separate native Ranger evidence evaluator, not by a PGT type.
    bool originalRangerRecipientHistoryIndependentlyVerified=false;
    bool rangerNativePidIvHistoryIndependentlyVerified=false;
    // A retail DS-clock field is NEVER a server-attested receipt timestamp.
    bool originalReceiptDateIndependentlyLinked=false;
    bool nativeDateAvailable=false;
    Clock::Date observedEgg{},observedMet{};
    Clock::Evidence clock{};
};
struct Result {
    Status status=Status::UnknownNative;
    C::ArchiveStatus archive=C::ArchiveStatus::UnknownSource;
    Clock::Status clock=Clock::Status::UnknownNative;
    uint16_t independentlyPinnedCardId=0;
    bool candidateOnly=true;
    bool authenticated=false;
    bool globallyInvalid=false;
};
constexpr bool structural(const C::Projection& p) noexcept {
    return p.status==C::Status::StructuralPCD ||
           p.status==C::Status::PairedPayloadMatches ||
           p.status==C::Status::StructuralPGT;
}
constexpr Result evaluate(bool nativeValid,const C::Projection& card,
                          const CardSource& source,
                          const RecipientEvidence& recipient) noexcept {
    Result out{};
    if(!nativeValid)return out;
    if(!structural(card)) {out.status=Status::UnknownContainer;return out;}
    if(card.delivery!=C::Delivery::DirectPokemon &&
       card.delivery!=C::Delivery::RangerManaphyEgg) {
        out.status=Status::NonPokemonDelivery;return out;
    }
    out.archive=C::attest(card,source.archive);
    if(out.archive!=C::ArchiveStatus::ConditionalOriginalSource) {
        out.status=Status::UnknownOriginalArchive;return out;
    }
    // PGT contains NO WC4 card ID, region/redeem context, original card
    // metadata or verified source digest. External guesses cannot add them.
    if(card.format!=C::Format::PCD || !card.cardIdPresent ||
       !source.exactCardDigestIndependentlyVerified ||
       !C::nonzero(source.exactCardArchiveDigest) ||
       source.exactCardArchiveDigest!=source.archive.sourceDigest ||
       !source.originalGiftTypeIndependentlyVerified ||
       !source.cardIdIndependentlyVerified ||
       !source.compatibilityBytesIndependentlyVerified ||
       !source.giftBodyLinkedToExactCardIndependentlyVerified) {
        out.status=Status::UnknownCardIdentity;return out;
    }
    out.independentlyPinnedCardId=source.originalCardId;
    if(card.type!=source.originalGiftType) {
        out.status=Status::CandidateGiftTypeMismatch;return out;
    }
    if(card.cardId!=source.originalCardId ||
       card.rawCompatibility!=source.originalCompatibility) {
        out.status=Status::CandidateCardMismatch;return out;
    }
    if(!recipient.originalSaveDigestIndependentlyVerified ||
       !C::nonzero(recipient.originalRecipientSaveDigest) ||
       !recipient.originalSaveIdentityIndependentlyVerified ||
       !recipient.originalTrainerIdentityIndependentlyVerified ||
       !recipient.recipientSlotIndependentlyVerified) {
        out.status=Status::UnknownRecipientSave;return out;
    }
    if(!recipient.receiptRecordIndependentlyVerified ||
       !recipient.receiptRecordBoundToOriginalCardIndependentlyVerified) {
        out.status=Status::UnknownReceiptCardLink;return out;
    }
    if(recipient.originalReceiptCardId!=source.originalCardId) {
        out.status=Status::CandidateReceiptMismatch;return out;
    }
    if(!recipient.individualNativePK4LinkedToReceiptIndependentlyVerified) {
        out.status=Status::UnknownNativeLink;return out;
    }
    if(card.delivery==C::Delivery::RangerManaphyEgg &&
       (!recipient.originalRangerRecipientHistoryIndependentlyVerified ||
        !recipient.rangerNativePidIvHistoryIndependentlyVerified)) {
        out.status=Status::UnknownRangerRecipient;return out;
    }
    if(!recipient.originalReceiptDateIndependentlyLinked ||
       !recipient.nativeDateAvailable) {
        out.status=Status::UnknownReceiptClock;return out;
    }
    const auto time=Clock::evaluate(true,recipient.observedEgg,
                                     recipient.observedMet,recipient.clock);
    out.clock=time.status;
    if(time.status==Clock::Status::CandidateOutsideWindow) {
        out.status=Status::CandidateClockMismatch;return out;
    }
    if(time.status!=Clock::Status::CandidateWithinWindow) {
        out.status=Status::UnknownReceiptClock;return out;
    }
    out.status=Status::ConditionalCandidate;
    return out;
}
static_assert(!Result{}.globallyInvalid && !Result{}.authenticated);
} // namespace Legality::Gen4OriginalCardRecipientEvidence
