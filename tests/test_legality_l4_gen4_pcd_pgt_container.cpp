#include "Legality/Gen4MysteryGiftContainerEvidence.h"
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <vector>
namespace C=Legality::Gen4MysteryGiftContainerEvidence;
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
        ++pairs;
    }
    assert(pairs==15 && mutated==15*259*8);
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
        const bool expected=(grade==1u||grade==3u) && mask==63u;
        assert((actual==C::ArchiveStatus::ConditionalOriginalSource)==expected);
        ++archives;
    }
    assert(archives==448);
    auto a=verified();a.grade=C::ArchiveGrade::OriginalPGT;
    assert(C::attest(projected,a)==C::ArchiveStatus::UnknownSource);
    auto gift=pgt;w16(gift,0,1);
    a=verified();a.grade=C::ArchiveGrade::OriginalPGT;
    assert(C::attest(C::inspect(gift),a)==C::ArchiveStatus::ConditionalOriginalSource);
    std::cout<<"L4 Gen IV PCD/PGT: "<<pairs<<" delivery families, "<<mutated
             <<" bitwise pair negatives, "<<archives<<" provenance states: PASS\n";
}
