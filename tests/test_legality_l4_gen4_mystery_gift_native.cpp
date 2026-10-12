#include "Legality/Gen4MysteryGiftNativeEvidence.h"
#include "Encryption/Encryption4.h"
#include "Enums/GameVersion.h"
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <vector>
namespace C=Legality::Gen4MysteryGiftContainerEvidence;
namespace H=Legality::Gen4MysteryGiftHistoryEvidence;
namespace N=Legality::Gen4MysteryGiftNativeEvidence;
void w16(std::vector<std::byte>& p,std::size_t i,uint16_t n) {
    p[i]=static_cast<std::byte>(n&255u);p[i+1]=static_cast<std::byte>(n>>8u);
}
void w32(std::vector<std::byte>& p,std::size_t i,uint32_t n) {
    w16(p,i,static_cast<uint16_t>(n));w16(p,i+2,static_cast<uint16_t>(n>>16));
}
C::Attestation hypothetical() {
    C::Attestation a{};
    a.grade=C::ArchiveGrade::OriginalPCD;
    a.exactSourceIndependentlyIdentified=true;
    a.digestIndependentlyVerified=true;
    a.sourceDigest[0]=std::byte{0x6c};
    a.originalFieldsIndependentlyExtracted=true;
    a.releasedIndependentlyCorroborated=true;
    a.recipientHistoryIndependentlyKnown=true;
    return a;
}
H::Source hypotheticalHistory(const H::Observed& n,C::GiftType type) {
    H::Source s{};
    s.originalFieldsIndependentlyVerified=true;
    s.giftType=type;
    s.originalSpecies={true,n.species};
    s.originalTid={true,n.tid};s.originalSid={true,n.sid};
    s.originalLanguage={true,n.language};
    s.originalOriginVersion={true,n.originVersion};
    s.originalBall={true,n.ball};s.originalEgg={true,n.egg};
    s.originalFateful={true,n.fateful};s.originalMoves={true,n.moves};
    s.requiredAtReceiptRibbons={true,{0,0,0}};
    s.originalRawOT={true,n.rawOT};s.originalRawNickname={true,n.rawNickname};
    s.speciesHistory=s.trainerHistory=s.languageHistory=s.originHistory=
    s.eggHistory=s.ballHistory=s.movesHistory=s.rawOTHistory=
    s.nicknameHistory=H::History::Unchanged;
    s.ribbonRetentionIndependentlyVerified=true;
    return s;
}
int main() {
    // Synthetic PK4, not a real historical event/receipt. Use the accepted
    // original project's real encrypted PK4 implementation and parser.
    std::vector<std::byte> canonical(Encryption::SIZE_STORED4);
    w32(canonical,0,0x0f199b50u);
    w16(canonical,0x08,4);w16(canonical,0x0c,7200);
    w16(canonical,0x0e,58364);
    w16(canonical,0x28,10);w16(canonical,0x2a,52);
    w16(canonical,0x2c,53);w32(canonical,0x24,1);
    canonical[0x17]=std::byte{1};canonical[0x40]=std::byte{1};
    canonical[0x5f]=std::byte{7};canonical[0x84]=std::byte{40};
    canonical[0x86]=std::byte{16};w16(canonical,0x46,3053);
    canonical[0x48]=std::byte{0x41};w16(canonical,0x4a,0xffffu);
    canonical[0x68]=std::byte{0x52};w16(canonical,0x6a,0xffffu);
    w16(canonical,0x06,Encryption::checksum4(canonical));
    const auto encrypted=Encryption::encryptArray4(canonical);
    Pokemon::Pokemon4ReadOnly record(encrypted,Enums::GameVersion::HGSS);
    assert(record.valid());
    const std::vector<std::byte> preserved(record.originalEncryptedBytes().begin(),
                                          record.originalEncryptedBytes().end());
    std::vector<std::byte> pcd(C::PCDSize),pgt(C::PGTSize);
    w16(pcd,0,1);w16(pgt,0,1);w16(pcd,C::PCDCardId,67);
    pgt[2]=std::byte{5};
    auto preliminary=N::evaluateNative(record,pcd,{},hypothetical(),{});
    assert(preliminary.observed.nativeValid);
    auto original=hypotheticalHistory(preliminary.observed,C::GiftType::Pokemon);
    auto yes=N::evaluateNative(record,pcd,{},hypothetical(),original);
    assert(yes.history.status==H::Status::ConditionalCandidate);
    assert(yes.observed.moves[0]==10 && yes.observed.moves[1]==52);
    assert(yes.card.cardIdPresent && yes.card.cardId==67);
    for(std::size_t i=0;i<C::PGTSize;++i)pgt[i]=pcd[i];
    pgt[2]=std::byte{8};
    yes=N::evaluateNative(record,pcd,pgt,hypothetical(),original);
    assert(yes.card.status==C::Status::PairedPayloadMatches);
    assert(yes.history.status==H::Status::ConditionalCandidate);
    auto altered=pgt;altered[0x81]^=std::byte{1};
    assert(N::evaluateNative(record,pcd,altered,hypothetical(),original)
           .history.status==H::Status::UnknownOriginalSource);
    auto tampered=encrypted;tampered[0x32]^=std::byte{1};
    Pokemon::Pokemon4ReadOnly invalid(tampered,Enums::GameVersion::HGSS);
    assert(!invalid.valid());
    assert(N::evaluateNative(invalid,pcd,pgt,hypothetical(),original)
           .history.status==H::Status::UnavailableNative);
    auto wrongOT=original;wrongOT.originalTid.value++;
    assert(N::evaluateNative(record,pcd,pgt,hypothetical(),wrongOT)
           .history.status==H::Status::CandidateContradicted);
    auto absent=hypothetical();absent.recipientHistoryIndependentlyKnown=false;
    assert(N::evaluateNative(record,pcd,pgt,absent,original)
           .history.status==H::Status::UnknownOriginalSource);
    // A forged assertion of Ranger authenticity must not bypass the
    // existing native Ranger PID/IV / recipient witness requirements.
    std::vector<std::byte> manaphyCard(C::PCDSize);
    w16(manaphyCard,0,7);
    auto claimed=original;claimed.giftType=C::GiftType::ManaphyEgg;
    claimed.rangerGiftProofIndependentlyVerified=true; // adversarial caller
    const auto ranger=N::evaluateNative(record,manaphyCard,{},hypothetical(),claimed);
    assert(ranger.rangerEvaluated);
    assert(ranger.history.status==H::Status::UnknownRangerReceipt);
    assert(ranger.ranger.status!=Legality::Gen4EventRangerGiftHistory::Status::ConditionalGiftCandidate);
    // All 32 Gen IV block-order selector values, both real PK4 record
    // sizes, through the *existing* cipher and immutable native parser.
    unsigned nativeCases=0;
    for(const auto size:{Encryption::SIZE_STORED4,Encryption::SIZE_PARTY4})
    for(uint32_t sv=0;sv<32;++sv) {
        auto raw=canonical;
        raw.resize(size);
        w32(raw,0,(sv<<13)|0x1u);
        w16(raw,0x06,Encryption::checksum4(raw));
        const auto cipher=Encryption::encryptArray4(raw);
        assert(cipher.size()==size);
        Pokemon::Pokemon4ReadOnly current(cipher,Enums::GameVersion::HGSS);
        assert(current.valid() && !current.empty());
        const auto first=N::evaluateNative(current,pcd,{},hypothetical(),{});
        assert(first.observed.nativeValid);
        const auto src=hypotheticalHistory(first.observed,C::GiftType::Pokemon);
        assert(N::evaluateNative(current,pcd,pgt,hypothetical(),src)
               .history.status==H::Status::ConditionalCandidate);
        const auto currentEncrypted=current.originalEncryptedBytes();
        assert(currentEncrypted.size()==cipher.size());
        for(size_t i=0;i<cipher.size();++i)assert(currentEncrypted[i]==cipher[i]);
        ++nativeCases;
    }
    assert(nativeCases==64);
    const auto bytes=record.originalEncryptedBytes();
    assert(bytes.size()==preserved.size());
    for(std::size_t i=0;i<bytes.size();++i) assert(bytes[i]==preserved[i]);
    std::cout<<"L4 PCD/PGT -> strict native encrypted PK4, Ranger guard + 64 PK4 native layouts: PASS\n";
}
