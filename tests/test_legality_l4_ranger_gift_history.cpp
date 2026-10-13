#include "Legality/Gen4EventRangerGiftHistory.h"
#include "Encryption/Encryption4.h"
#include "Enums/GameVersion.h"
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <span>
#include <vector>
namespace R=Legality::Gen4EventRangerGiftHistory;
namespace G=Legality::Gen4RangerManaphy;
using Status=R::Status;
constexpr std::array<uint8_t,6> iv{16,13,12,2,18,3};
constexpr R::Input egg{{490,2,2,true,3001,0,4,true},
    0x5271E97Eu,iv,10,12345,54321};
static_assert(R::evaluate(true,egg).status==Status::UnknownOriginalGift);
constexpr R::SourceEvidence source={true,true,12345,54321,false,0};
static_assert(R::evaluate(true,egg,source).status==Status::ConditionalGiftCandidate);
static_assert(R::evaluate(false,egg,source).status==Status::UnavailableNative);
constexpr auto traded=[] {
    auto c=egg;c.observed.metLocation=2002;return c;
}();
static_assert(R::evaluate(true,traded,source).status==Status::ConditionalGiftCandidate);
constexpr auto bad=[] {
    auto c=egg;c.observed.ball=16;return c;
}();
static_assert(R::evaluate(true,bad,source).status==Status::CandidateStructuralMismatch);
static_assert(R::evaluate(true,bad).status==Status::UnknownOriginalGift);
static_assert(!R::evaluate(true,bad).candidateStructurallyMatched);
void w16(std::vector<std::byte>& b,std::size_t off,uint16_t value) {
    b[off]=static_cast<std::byte>(value&255u);
    b[off+1]=static_cast<std::byte>(value>>8);
}
void w32(std::vector<std::byte>& b,std::size_t off,uint32_t value) {
    w16(b,off,static_cast<uint16_t>(value));
    w16(b,off+2,static_cast<uint16_t>(value>>16));
}
int main() {
    // Without independent receipt, even a structurally mismatched native
    // source is Unknown; a candidate-local exclusion needs attestation.
    auto withheld=source;
    withheld.rangerGiftOriginIndependentlyAttested=false;
    assert(R::evaluate(true,bad,withheld).status==Status::UnknownOriginalGift);
    assert(!R::evaluate(true,bad,withheld).candidateStructurallyMatched);
    auto unknown=source;
    unknown.originalRecipientTrainerKnown=false;
    assert(R::evaluate(true,egg,unknown).status==Status::UnknownRecipientHistory);
    auto badOrigin=egg;badOrigin.originVersion=8;
    assert(R::evaluate(true,badOrigin,source).status==Status::UnknownSourceGame);
    auto invalidPID=egg;invalidPID.pid=0x12345678u;
    assert(R::evaluate(true,invalidPID,source).status==Status::CandidatePidIvMismatch);
    // One ARNG step from the documented Method-1 fixture changes its
    // PID without changing IVs. The reverse routine can recognize a
    // traded-recipient RNG class but NOT establish its historic TID/SID.
    auto unresolved=egg;
    unresolved.observed.isEgg=false;
    unresolved.observed.eggLocation=G::LocationLinkTrade4;
    unresolved.observed.metLocation=1;
    unresolved.pid=0x07578CB7u; // (0x5271E97E * 0x6C078965 + 1) mod 2^32
    auto independentlyHatched=source;
    independentlyHatched.hatchGameHistoryKnown=true;
    independentlyHatched.hatchGameVersion=10;
    assert(G::analyzePidIv(unresolved.observed,unresolved.pid,
                           unresolved.ivs,12345,54321).evidence==
           G::PidEvidence::TradedRecipientUnresolved);
    const auto unresolvedHistory=R::evaluate(true,unresolved,independentlyHatched);
    assert(unresolvedHistory.status==Status::UnknownRecipientHistory);
    assert(unresolvedHistory.rng.evidence==
           G::PidEvidence::TradedRecipientUnresolved);
    auto hatched=egg;hatched.observed.isEgg=false;
    hatched.observed.metLocation=1;
    auto s=source;s.hatchGameHistoryKnown=true;s.hatchGameVersion=10;
    const auto directHatched=R::evaluate(true,hatched,s);
    assert(directHatched.status==Status::ConditionalGiftCandidate);
    assert(directHatched.hatch==Legality::Gen4Hatch::HatchResult::ValidForOrigin);
    assert(R::evaluate(true,hatched,source).status==Status::UnknownHatchGame);
    hatched.observed.metLocation=0;
    assert(R::evaluate(true,hatched,s).status==Status::CandidateHatchLocationMismatch);
    // Synthetic encrypted native PK4, not a real deposited Ranger gift.
    std::vector<std::byte> bytes(Encryption::SIZE_STORED4);
    w32(bytes,0,0x5271E97Eu);w16(bytes,0x08,490);
    w16(bytes,0x0c,12345);w16(bytes,0x0e,54321);
    w16(bytes,0x44,3001);
    uint32_t ivbits=0x40000000u;
    for(unsigned i=0;i<6;++i)ivbits|=static_cast<uint32_t>(iv[i])<<(5*i);
    w32(bytes,0x38,ivbits);
    bytes[0x17]=std::byte{2};bytes[0x40]=std::byte{5};
    bytes[0x5f]=std::byte{10};bytes[0x83]=std::byte{4};
    w16(bytes,0x06,Encryption::checksum4(bytes));
    const auto encrypted=Encryption::encryptArray4(bytes);
    Pokemon::Pokemon4ReadOnly record(encrypted,Enums::GameVersion::DP);
    assert(record.valid());
    const std::vector<std::byte> saved(
        record.originalEncryptedBytes().begin(),record.originalEncryptedBytes().end());
    assert(R::evaluateNative(record,source).status==Status::ConditionalGiftCandidate);
    assert(R::evaluateNative(record).status==Status::UnknownOriginalGift);
    // Native PK4 Ball fields are format-specific, not a numeric maximum.
    // D/P record: stale HGSS byte 16 cannot overwrite the active Ball 4.
    auto staleHgss=bytes;staleHgss[0x86]=std::byte{16};
    w16(staleHgss,0x06,Encryption::checksum4(staleHgss));
    const auto mismatchedFields=Encryption::encryptArray4(staleHgss);
    Pokemon::Pokemon4ReadOnly diamond(mismatchedFields,Enums::GameVersion::DP);
    Pokemon::Pokemon4ReadOnly heartgold(mismatchedFields,Enums::GameVersion::HGSS);
    assert(R::evaluateNative(diamond,source).status==Status::ConditionalGiftCandidate);
    assert(R::evaluateNative(heartgold,source).status==Status::CandidateStructuralMismatch);
    // HGSS record: stale DPPt byte 16 cannot overwrite its active Ball 4.
    auto staleDp=bytes;staleDp[0x83]=std::byte{16};staleDp[0x86]=std::byte{4};
    w16(staleDp,0x06,Encryption::checksum4(staleDp));
    Pokemon::Pokemon4ReadOnly heartgoldValid(
        Encryption::encryptArray4(staleDp),Enums::GameVersion::HGSS);
    assert(R::evaluateNative(heartgoldValid,source).status==Status::ConditionalGiftCandidate);
    Pokemon::Pokemon4ReadOnly unknownStorage(
        mismatchedFields,Enums::GameVersion::Invalid);
    assert(R::evaluateNative(unknownStorage,source).status==Status::UnknownSourceGame);
    assert(R::evaluateNative(unknownStorage).status==Status::UnknownOriginalGift);
    assert(diamond.originalEncryptedBytes().size()==Encryption::SIZE_STORED4);

    auto corrupt=encrypted;corrupt[0x32]^=std::byte{0x21};
    Pokemon::Pokemon4ReadOnly broken(corrupt,Enums::GameVersion::DP);
    assert(R::evaluateNative(broken,source).status==Status::UnavailableNative);
    const auto after=record.originalEncryptedBytes();
    assert(after.size()==saved.size());
    for(std::size_t i=0;i<saved.size();++i)assert(saved[i]==after[i]);
    std::cout<<"L4 V09 native encrypted Ranger Manaphy gift history: PASS\n";
}
