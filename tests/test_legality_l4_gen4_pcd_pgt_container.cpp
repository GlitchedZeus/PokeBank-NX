#include "Legality/Gen4MysteryGiftContainerEvidence.h"
#include "Legality/Gen4OriginalCardRecipientEvidence.h"
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <vector>
namespace C=Legality::Gen4MysteryGiftContainerEvidence;
namespace J=Legality::Gen4OriginalCardRecipientEvidence;
void w16(std::vector<std::byte>& x,size_t i,uint16_t v) {
    x[i]=static_cast<std::byte>(v);x[i+1]=static_cast<std::byte>(v>>8);
}
void w32(std::vector<std::byte>& x,size_t i,uint32_t v) {
    w16(x,i,static_cast<uint16_t>(v));w16(x,i+2,static_cast<uint16_t>(v>>16));
}
C::Attestation verified() {
    C::Attestation a{};a.grade=C::ArchiveGrade::OriginalPCD;
    a.sourceDigest[0]=std::byte{0xa5};a.exactSourceIndependentlyIdentified=true;
    a.digestIndependentlyVerified=true;a.originalFieldsIndependentlyExtracted=true;
    a.releasedIndependentlyCorroborated=true;a.recipientHistoryIndependentlyKnown=true;
    return a;
}
int main() {
    uint64_t pairs=0, mutated=0, archives=0;
    for(unsigned type=1;type<=15;++type) {
        std::vector<std::byte> pcd(C::PCDSize),pgt(C::PGTSize);
        w16(pcd,0,static_cast<uint16_t>(type));
        w16(pgt,0,static_cast<uint16_t>(type));
        w16(pcd,C::PCDCardId,67);
        w16(pcd,C::PCDCompatibility,0xf101u);
        for(std::size_t i=4;i<C::PGTSize;++i)
            pcd[i]=pgt[i]=static_cast<std::byte>((i*13+type)&255u);
        pgt[2]=std::byte{42};pcd[2]=std::byte{7};
        const auto solo=C::inspect(pgt), card=C::inspect(pcd);
        assert(solo.status==C::Status::StructuralPGT && !solo.cardIdPresent);
        assert(!solo.pgtCanRepresentFullCard);
        assert(card.status==C::Status::StructuralPCD && card.cardIdPresent);
        assert(card.cardId==67 && card.rawCompatibility==0xf101u);
        assert(C::inspectPair(pcd,pgt).status==C::Status::PairedPayloadMatches);
        assert(C::inspectPair(pcd,pgt).linkedSlot==7);
        // A non-card PGT cannot determine which of the reused card IDs is
        // its source. Modifying ONLY card metadata retains the same gift.
        auto another=pcd;w16(another,C::PCDCardId,0xabcd);
        assert(C::inspectPair(another,pgt).status==C::Status::PairedPayloadMatches);
        for(std::size_t off=0;off<C::PGTSize;++off) {
            if(off==2)continue;
            for(unsigned bit=0;bit<8;++bit) {
                auto bad=pgt;bad[off]^=static_cast<std::byte>(1u<<bit);
                // Nonzero unknown gift-type mutations can cause strict
                // UnknownGiftType instead of pair mismatch; still fail.
                assert(C::inspectPair(pcd,bad).status!=C::Status::PairedPayloadMatches);
                ++mutated;
            }
        }
        // Symmetric full PCD gift-body fault sweep: corrupting the PCD's
        // embedded PGT must fail just as a standalone-PGT mutation does.
        // The linked-slot byte is the sole intentional difference.
        for(std::size_t off=0;off<C::PGTSize;++off) {
            for(unsigned bit=0;bit<8;++bit) {
                auto altered=pcd;
                altered[off]^=static_cast<std::byte>(1u<<bit);
                const auto outcome=C::inspectPair(altered,pgt);
                if(off==2) {
                    assert(outcome.status==C::Status::PairedPayloadMatches);
                } else {
                    assert(outcome.status!=C::Status::PairedPayloadMatches);
                    assert(C::attest(outcome,verified())!=
                           C::ArchiveStatus::ConditionalOriginalSource);
                    ++mutated;
                }
            }
        }
        // Card-only metadata cannot be inferred from an isolated PGT, even
        // when two cards have the same gift bytes or reused numeric card ID.
        for(const std::size_t offset:{C::PCDCardId,C::PCDCardId+1u,
                                       C::PCDCompatibility,C::PCDCompatibility+1u}) {
            auto changed=pcd;
            changed[offset]^=std::byte{0x80};
            assert(C::inspectPair(changed,pgt).status==C::Status::PairedPayloadMatches);
        }
        ++pairs;
    }
    assert(pairs==15 && mutated==2*15*259*8);
    std::vector<std::byte> card(C::PCDSize),pgt(C::PGTSize);
    w16(card,0,3);w32(card,4,533);
    assert(!C::inspect(card).pgtExtractionAllowed); // Lock Capsule
    w32(card,4,532);assert(C::inspect(card).pgtExtractionAllowed);
    w16(card,0,7);assert(C::inspect(card).delivery==C::Delivery::RangerManaphyEgg);
    assert(!C::inspect(card).giftPayloadMayBeEncrypted);
    w16(card,0,8);assert(C::inspect(card).delivery==C::Delivery::KeyItemUnlock);
    w16(card,0,9);assert(C::inspect(card).delivery==C::Delivery::KeyItemUnlock);
    w16(card,0,10);assert(C::inspect(card).delivery==C::Delivery::KeyItemUnlock);
    w16(card,0,16);assert(C::inspect(card).status==C::Status::UnknownGiftType);
    w16(card,0,0);assert(C::inspect(card).status==C::Status::EmptyGift);
    std::array<std::byte,0> empty{};
    std::array<std::byte,C::PGTSize-1> shortGift{};
    std::array<std::byte,C::PCDSize+1> oversized{};
    assert(C::inspect(empty).status==C::Status::MissingOrWrongSize);
    assert(C::inspect(shortGift).status==C::Status::MissingOrWrongSize);
    assert(C::inspect(oversized).status==C::Status::MissingOrWrongSize);
    w16(card,0,1);
    const auto projected=C::inspect(card);
    assert(C::attest(projected,verified())==C::ArchiveStatus::ConditionalOriginalSource);
    for(unsigned grade=0;grade<=6;++grade)
    for(unsigned mask=0;mask<64;++mask) {
        auto a=verified();a.grade=static_cast<C::ArchiveGrade>(grade);
        a.exactSourceIndependentlyIdentified=(mask&1u)!=0;
        a.digestIndependentlyVerified=(mask&2u)!=0;
        a.originalFieldsIndependentlyExtracted=(mask&4u)!=0;
        a.releasedIndependentlyCorroborated=(mask&8u)!=0;
        a.recipientHistoryIndependentlyKnown=(mask&16u)!=0;
        if(!(mask&32u))a.sourceDigest={};
        const auto actual=C::attest(projected,a);
        const bool expected=grade==1u && mask==63u;
        assert((actual==C::ArchiveStatus::ConditionalOriginalSource)==expected);
        ++archives;
    }
    assert(archives==448);
    // Exhaust native grade-tag space. A malformed or future archive grade
    // cannot be promoted merely because accompanying claim flags are true.
    for(unsigned rawGrade=0;rawGrade<256;++rawGrade) {
        auto gradeSource=verified();
        gradeSource.grade=static_cast<C::ArchiveGrade>(rawGrade);
        const auto status=C::attest(projected,gradeSource);
        if(rawGrade==1u)
            assert(status==C::ArchiveStatus::ConditionalOriginalSource);
        else if(rawGrade>=4u && rawGrade<=6u)
            assert(status==C::ArchiveStatus::UntrustedSource);
        else if(rawGrade==3u)
            assert(status==C::ArchiveStatus::UnknownConversionHistory);
        else
            assert(status==C::ArchiveStatus::UnknownSource);
    }
    // Converted original is never authenticated by its converted digest
    // alone. Independently pin ORIGINAL archive digest, transformation chain
    // and field preservation; exhaust every missing-proof combination.
    unsigned conversionStates=0;
    for(unsigned mask=0;mask<16;++mask) {
        auto converted=verified();
        converted.grade=C::ArchiveGrade::ConvertedOriginal;
        converted.originalArchiveDigest[0]=std::byte{0x32};
        converted.originalArchiveDigestIndependentlyVerified=(mask&1u)!=0;
        converted.originalToConvertedChainIndependentlyVerified=(mask&2u)!=0;
        converted.originalFieldsPreservedIndependentlyVerified=(mask&4u)!=0;
        if(!(mask&8u))converted.originalArchiveDigest={};
        const auto status=C::attest(projected,converted);
        assert(status==(mask==15u ? C::ArchiveStatus::ConditionalOriginalSource :
                                     C::ArchiveStatus::UnknownConversionHistory));
        ++conversionStates;
    }
    // Source-file type compatibility still matters with complete conversion
    // history. Paired bytes alone do not identify a recipient or archive.
    auto converted=verified();converted.grade=C::ArchiveGrade::ConvertedOriginal;
    converted.originalArchiveDigest[0]=std::byte{0x32};
    converted.originalArchiveDigestIndependentlyVerified=true;
    converted.originalToConvertedChainIndependentlyVerified=true;
    converted.originalFieldsPreservedIndependentlyVerified=true;
    assert(C::attest(projected,converted)==C::ArchiveStatus::ConditionalOriginalSource);
    converted.grade=C::ArchiveGrade::OriginalPCD;
    assert(C::attest(projected,converted)==C::ArchiveStatus::ConditionalOriginalSource);
    // The same absent conversion provenance must be quarantined on an
    // isolated PGT. A standalone PGT supplies NO original WC4 card identity.
    std::vector<std::byte> isolated(C::PGTSize);
    w16(isolated,0,static_cast<uint16_t>(C::GiftType::Pokemon));
    const auto isolatedProjection=C::inspect(isolated);
    assert(isolatedProjection.status==C::Status::StructuralPGT);
    assert(!isolatedProjection.cardIdPresent);
    for(unsigned mask=0;mask<16;++mask) {
        auto c=converted;c.grade=C::ArchiveGrade::ConvertedOriginal;
        c.originalArchiveDigestIndependentlyVerified=(mask&1u)!=0;
        c.originalToConvertedChainIndependentlyVerified=(mask&2u)!=0;
        c.originalFieldsPreservedIndependentlyVerified=(mask&4u)!=0;
        if(!(mask&8u))c.originalArchiveDigest={};
        assert(C::attest(isolatedProjection,c)==
               (mask==15u?C::ArchiveStatus::ConditionalOriginalSource:
                           C::ArchiveStatus::UnknownConversionHistory));
        ++conversionStates;
    }
    // The source-grade claim is not a substitute for the other attested
    // release and individual recipient-history requirements.
    auto dropped=converted;dropped.grade=C::ArchiveGrade::ConvertedOriginal;
    dropped.originalToConvertedChainIndependentlyVerified=true;
    dropped.originalArchiveDigestIndependentlyVerified=true;
    dropped.originalFieldsPreservedIndependentlyVerified=true;
    dropped.recipientHistoryIndependentlyKnown=false;
    assert(C::attest(projected,dropped)==C::ArchiveStatus::UnknownReceipt);
    dropped=converted;dropped.grade=C::ArchiveGrade::ConvertedOriginal;
    dropped.originalToConvertedChainIndependentlyVerified=true;
    dropped.originalArchiveDigestIndependentlyVerified=true;
    dropped.originalFieldsPreservedIndependentlyVerified=true;
    dropped.releasedIndependentlyCorroborated=false;
    assert(C::attest(isolatedProjection,dropped)==C::ArchiveStatus::UnknownRelease);
    auto a=verified();a.grade=C::ArchiveGrade::OriginalPGT;
    assert(C::attest(projected,a)==C::ArchiveStatus::UnknownSource);
    auto gift=pgt;w16(gift,0,1);
    a=verified();a.grade=C::ArchiveGrade::OriginalPGT;
    assert(C::attest(C::inspect(gift),a)==C::ArchiveStatus::ConditionalOriginalSource);
    // Original card identity is not determined by PGT gift-body equality.
    // Independently proven source and recipient-save card links are separate.
    auto originalCard=card;
    w16(originalCard,C::PCDCardId,66);
    w16(originalCard,C::PCDCompatibility,0x30);
    auto identity=J::CardSource{};
    identity.archive=verified();
    identity.exactCardArchiveDigest=identity.archive.sourceDigest;
    identity.exactCardDigestIndependentlyVerified=true;
    identity.originalGiftTypeIndependentlyVerified=true;
    identity.originalGiftType=C::GiftType::Pokemon;
    identity.cardIdIndependentlyVerified=true;identity.originalCardId=66;
    identity.compatibilityBytesIndependentlyVerified=true;
    identity.originalCompatibility=0x30;
    identity.giftBodyLinkedToExactCardIndependentlyVerified=true;
    auto recipient=J::RecipientEvidence{};
    recipient.originalRecipientSaveDigest[0]=std::byte{1};
    recipient.originalSaveDigestIndependentlyVerified=true;
    recipient.originalSaveIdentityIndependentlyVerified=true;
    recipient.receiptRecordIndependentlyVerified=true;
    recipient.receiptRecordBoundToOriginalCardIndependentlyVerified=true;
    recipient.originalReceiptCardId=66;
    recipient.originalTrainerIdentityIndependentlyVerified=true;
    recipient.recipientSlotIndependentlyVerified=true;
    recipient.individualNativePK4LinkedToReceiptIndependentlyVerified=true;
    recipient.originalReceiptDateIndependentlyLinked=true;
    recipient.nativeDateAvailable=true;
    recipient.observedMet={2010,3,9};
    recipient.clock.releaseWindowIndependentlyVerified=true;
    recipient.clock.first={2010,3,1};recipient.clock.last={2010,3,31};
    recipient.clock.originalReceiptField=J::Clock::Field::Met;
    recipient.clock.originalFieldUnchangedIndependentlyVerified=true;
    recipient.clock.clockAccurateAtOriginalReceiptIndependentlyVerified=true;
    recipient.clock.noLaterDateRewriteIndependentlyVerified=true;
    auto joined=J::evaluate(true,C::inspect(originalCard),identity,recipient);
    assert(joined.status==J::Status::ConditionalCandidate);
    assert(!joined.authenticated && !joined.globallyInvalid);
    w16(originalCard,C::PCDCardId,67);
    assert(J::evaluate(true,C::inspect(originalCard),identity,recipient).status==
           J::Status::CandidateCardMismatch);
    recipient.receiptRecordBoundToOriginalCardIndependentlyVerified=false;
    w16(originalCard,C::PCDCardId,66);
    assert(J::evaluate(true,C::inspect(originalCard),identity,recipient).status==
           J::Status::UnknownReceiptCardLink);
    // A Ranger-shaped card is NOT independently verified PID/IV and the
    // historical recipient; these are distinct native evidence checks.
    recipient.receiptRecordBoundToOriginalCardIndependentlyVerified=true;
    identity.originalGiftType=C::GiftType::ManaphyEgg;
    w16(originalCard,0,7);
    assert(J::evaluate(true,C::inspect(originalCard),identity,recipient).status==
           J::Status::UnknownRangerRecipient);
    std::cout<<"L4 Gen IV PCD/PGT: "<<pairs<<" delivery families, "<<mutated
             <<" bitwise pair negatives, "<<archives<<" provenance states, "
             <<conversionStates<<" conversion-proof states: PASS\n";
}
