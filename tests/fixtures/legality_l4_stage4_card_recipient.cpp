#include "Legality/Gen4OriginalCardRecipientEvidence.h"
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <vector>
namespace J=Legality::Gen4OriginalCardRecipientEvidence;
namespace C=J::C;
namespace T=J::Clock;
static void w16(std::vector<std::byte>& v,std::size_t off,uint16_t n) {
    v[off]=std::byte(n);v[off+1]=std::byte(n>>8);
}
struct Fixture {
    std::vector<std::byte> pcd=std::vector<std::byte>(C::PCDSize);
    std::vector<std::byte> pgt=std::vector<std::byte>(C::PGTSize);
    J::CardSource source{};
    J::RecipientEvidence receipt{};
    Fixture() {
        w16(pcd,0,1);w16(pgt,0,1);w16(pcd,C::PCDCardId,321);
        w16(pcd,C::PCDCompatibility,0x0030);
        pcd[2]=std::byte{8};pgt[2]=std::byte{99};
        source.archive.grade=C::ArchiveGrade::OriginalPCD;
        source.archive.sourceDigest[0]=std::byte{0x35};
        source.archive.exactSourceIndependentlyIdentified=true;
        source.archive.digestIndependentlyVerified=true;
        source.archive.originalFieldsIndependentlyExtracted=true;
        source.archive.releasedIndependentlyCorroborated=true;
        source.archive.recipientHistoryIndependentlyKnown=true;
        source.exactCardArchiveDigest=source.archive.sourceDigest;
        source.exactCardDigestIndependentlyVerified=true;
        source.originalGiftTypeIndependentlyVerified=true;
        source.originalGiftType=C::GiftType::Pokemon;
        source.cardIdIndependentlyVerified=true;
        source.originalCardId=321;
        source.compatibilityBytesIndependentlyVerified=true;
        source.originalCompatibility=0x30;
        source.giftBodyLinkedToExactCardIndependentlyVerified=true;
        receipt.originalRecipientSaveDigest[0]=std::byte{0x88};
        receipt.originalSaveDigestIndependentlyVerified=true;
        receipt.originalSaveIdentityIndependentlyVerified=true;
        receipt.receiptRecordIndependentlyVerified=true;
        receipt.receiptRecordBoundToOriginalCardIndependentlyVerified=true;
        receipt.originalReceiptCardId=321;
        receipt.originalTrainerIdentityIndependentlyVerified=true;
        receipt.recipientSlotIndependentlyVerified=true;
        receipt.individualNativePK4LinkedToReceiptIndependentlyVerified=true;
        receipt.originalReceiptDateIndependentlyLinked=true;
        receipt.nativeDateAvailable=true;
        receipt.observedMet={2010,3,9};
        receipt.clock.releaseWindowIndependentlyVerified=true;
        receipt.clock.first={2010,3,1};receipt.clock.last={2010,3,31};
        receipt.clock.originalReceiptField=T::Field::Met;
        receipt.clock.originalFieldUnchangedIndependentlyVerified=true;
        receipt.clock.clockAccurateAtOriginalReceiptIndependentlyVerified=true;
        receipt.clock.noLaterDateRewriteIndependentlyVerified=true;
    }
};
int main() {
    using S=J::Status;
    Fixture f{};const auto card=C::inspectPair(f.pcd,f.pgt);
    assert(card.status==C::Status::PairedPayloadMatches);
    assert(J::evaluate(true,card,f.source,f.receipt).status==S::ConditionalCandidate);
    assert(!J::evaluate(true,card,f.source,f.receipt).authenticated);
    // PGT is not a card, even with a matching gift body and every positive
    // externally claimed proof. WC4 metadata is intentionally missing.
    assert(J::evaluate(true,C::inspect(f.pgt),f.source,f.receipt).status==S::UnknownOriginalArchive);
    auto giftSource=f.source;giftSource.archive.grade=C::ArchiveGrade::OriginalPGT;
    assert(J::evaluate(true,C::inspect(f.pgt),giftSource,f.receipt).status==S::UnknownCardIdentity);
    // The source-archive gift type must be separately pinned: bytes that
    // still structurally parse as another gift type do not inherit the
    // original card's source identity.
    auto wrongGift=f.pcd;w16(wrongGift,0,2);
    assert(J::evaluate(true,C::inspect(wrongGift),f.source,f.receipt).status==
           S::CandidateGiftTypeMismatch);
    // Different PCDs can contain the SAME 260-byte gift while their card
    // IDs or compatibility metadata differ. No invented identity bridge.
    auto different=f.pcd;w16(different,C::PCDCardId,322);
    assert(C::inspectPair(different,f.pgt).status==C::Status::PairedPayloadMatches);
    assert(J::evaluate(true,C::inspectPair(different,f.pgt),f.source,f.receipt).status==S::CandidateCardMismatch);
    different=f.pcd;w16(different,C::PCDCompatibility,0x40);
    assert(C::inspectPair(different,f.pgt).status==C::Status::PairedPayloadMatches);
    assert(J::evaluate(true,C::inspectPair(different,f.pgt),f.source,f.receipt).status==S::CandidateCardMismatch);
    auto wrongPair=f.pgt;wrongPair[0x44]^=std::byte{1};
    assert(J::evaluate(true,C::inspectPair(f.pcd,wrongPair),f.source,f.receipt).status==S::UnknownContainer);
    // Every independently attested source/save/recipient proof gate must be
    // present. Missing one MUST NOT become a candidate contradiction.
    uint64_t cardMasks=0,recipientMasks=0;
    for(unsigned mask=0;mask<256;++mask) {
        auto s=f.source;
        s.exactCardDigestIndependentlyVerified=bool(mask&1);
        s.cardIdIndependentlyVerified=bool(mask&2);
        s.compatibilityBytesIndependentlyVerified=bool(mask&4);
        s.giftBodyLinkedToExactCardIndependentlyVerified=bool(mask&8);
        s.archive.exactSourceIndependentlyIdentified=bool(mask&16);
        s.archive.digestIndependentlyVerified=bool(mask&32);
        s.archive.originalFieldsIndependentlyExtracted=bool(mask&64);
        s.archive.releasedIndependentlyCorroborated=bool(mask&128);
        const auto r=J::evaluate(true,card,s,f.receipt);
        assert((r.status==S::ConditionalCandidate)==(mask==255));
        assert(!r.authenticated && !r.globallyInvalid);
        ++cardMasks;
    }
    for(unsigned mask=0;mask<512;++mask) {
        auto s=f.receipt;
        s.originalSaveDigestIndependentlyVerified=bool(mask&1);
        s.originalSaveIdentityIndependentlyVerified=bool(mask&2);
        s.receiptRecordIndependentlyVerified=bool(mask&4);
        s.receiptRecordBoundToOriginalCardIndependentlyVerified=bool(mask&8);
        s.originalTrainerIdentityIndependentlyVerified=bool(mask&16);
        s.recipientSlotIndependentlyVerified=bool(mask&32);
        s.individualNativePK4LinkedToReceiptIndependentlyVerified=bool(mask&64);
        s.originalReceiptDateIndependentlyLinked=bool(mask&128);
        s.nativeDateAvailable=bool(mask&256);
        const auto r=J::evaluate(true,card,f.source,s);
        assert((r.status==S::ConditionalCandidate)==(mask==511));
        assert(!r.authenticated && !r.globallyInvalid);
        ++recipientMasks;
    }
    auto receipt=f.receipt;
    receipt.originalReceiptCardId=322;
    assert(J::evaluate(true,card,f.source,receipt).status==S::CandidateReceiptMismatch);
    receipt=f.receipt;receipt.receiptRecordBoundToOriginalCardIndependentlyVerified=false;
    receipt.originalReceiptCardId=322;
    assert(J::evaluate(true,card,f.source,receipt).status==S::UnknownReceiptCardLink);
    // A DS displayed date cannot prove distribution receipt: every clock
    // ambiguity must be independently resolved before a date-window test.
    uint64_t clockMasks=0;
    for(unsigned mask=0;mask<64;++mask) {
        auto s=f.receipt;
        s.clock.releaseWindowIndependentlyVerified=bool(mask&1);
        s.clock.originalFieldUnchangedIndependentlyVerified=bool(mask&2);
        s.clock.clockAccurateAtOriginalReceiptIndependentlyVerified=bool(mask&4);
        s.clock.noLaterDateRewriteIndependentlyVerified=bool(mask&8);
        s.clock.eggOrHatchHistoryComplete=bool(mask&16);
        s.clock.dateFieldHasEggOrHatchAmbiguity=bool(mask&32);
        const auto r=J::evaluate(true,card,f.source,s);
        const bool complete=(mask&15)==15 && ((mask&32)==0 || (mask&16)!=0);
        assert((r.status==S::ConditionalCandidate)==complete);
        ++clockMasks;
    }
    receipt=f.receipt;receipt.observedMet={2010,4,1};
    assert(J::evaluate(true,card,f.source,receipt).status==S::CandidateClockMismatch);
    receipt.clock.clockAccurateAtOriginalReceiptIndependentlyVerified=false;
    assert(J::evaluate(true,card,f.source,receipt).status==S::UnknownReceiptClock);
    // Ranger is a separate native PID/IV + original-recipient problem.
    auto ranger=f.pcd;w16(ranger,0,7);auto rangerPgt=f.pgt;w16(rangerPgt,0,7);
    auto projection=C::inspectPair(ranger,rangerPgt);
    auto rangerSource=f.source;rangerSource.originalGiftType=C::GiftType::ManaphyEgg;
    assert(J::evaluate(true,projection,rangerSource,f.receipt).status==S::UnknownRangerRecipient);
    receipt=f.receipt;receipt.originalRangerRecipientHistoryIndependentlyVerified=true;
    assert(J::evaluate(true,projection,rangerSource,receipt).status==S::UnknownRangerRecipient);
    receipt.rangerNativePidIvHistoryIndependentlyVerified=true;
    assert(J::evaluate(true,projection,rangerSource,receipt).status==S::ConditionalCandidate);
    for(unsigned raw=0;raw<256;++raw) {
        auto bad=f.pcd;w16(bad,0,raw);
        const auto p=C::inspect(bad);
        const auto r=J::evaluate(true,p,f.source,f.receipt);
        if(raw==1)assert(r.status==S::ConditionalCandidate);
        else if(raw==2 || raw==13 || raw==7)assert(r.status==S::CandidateGiftTypeMismatch);
        else if(raw==0 || raw>15)assert(r.status==S::UnknownContainer);
        else assert(r.status==S::NonPokemonDelivery);
        assert(!r.authenticated && !r.globallyInvalid);
    }
    assert(J::evaluate(false,card,f.source,f.receipt).status==S::UnknownNative);
    // Exhaust both matching and different paired bodies over every byte;
    // a mismatch may never attach the original gift to a receipt.
    uint64_t mutated=0;
    for(std::size_t off=0;off<C::PGTSize;++off) {
        if(off==2)continue;
        for(unsigned bit=0;bit<8;++bit) {
            auto x=f.pgt;x[off]^=std::byte{static_cast<uint8_t>(1u<<bit)};
            const auto r=J::evaluate(true,C::inspectPair(f.pcd,x),f.source,f.receipt);
            assert(r.status==S::UnknownContainer);
            ++mutated;
        }
    }
    assert(mutated==(C::PGTSize-1)*8);
    std::cout<<"L4 Stage4 original PCD/PGT recipient witness: "<<cardMasks
             <<" source masks, "<<recipientMasks<<" recipient masks, "
             <<clockMasks<<" clock masks, "<<mutated
             <<" paired-body mutations: PASS\n";
}
