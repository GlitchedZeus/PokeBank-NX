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
void w16(std::vector<std::byte>& b,std::size_t off,uint16_t value) {
    b[off]=static_cast<std::byte>(value&255u);
    b[off+1]=static_cast<std::byte>(value>>8);
}
void w32(std::vector<std::byte>& b,std::size_t off,uint32_t value) {
    w16(b,off,static_cast<uint16_t>(value));
    w16(b,off+2,static_cast<uint16_t>(value>>16));
}
int main() {
    auto unknown=source;
    unknown.originalRecipientTrainerKnown=false;
    assert(R::evaluate(true,egg,unknown).status==Status::UnknownRecipientHistory);
    auto badOrigin=egg;badOrigin.originVersion=8;
    assert(R::evaluate(true,badOrigin,source).status==Status::UnknownSourceGame);
    auto invalidPID=egg;invalidPID.pid=0x12345678u;
    assert(R::evaluate(true,invalidPID,source).status==Status::CandidatePidIvMismatch);
    auto hatched=egg;hatched.observed.isEgg=false;
    hatched.observed.metLocation=1;
    auto s=source;s.hatchGameHistoryKnown=true;s.hatchGameVersion=10;
    assert(R::evaluate(true,hatched,s).status==Status::ConditionalGiftCandidate);
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
    auto corrupt=encrypted;corrupt[0x32]^=std::byte{0x21};
    Pokemon::Pokemon4ReadOnly broken(corrupt,Enums::GameVersion::DP);
    assert(R::evaluateNative(broken,source).status==Status::UnavailableNative);
    const auto after=record.originalEncryptedBytes();
    assert(after.size()==saved.size());
    for(std::size_t i=0;i<saved.size();++i)assert(saved[i]==after[i]);
    std::cout<<"L4 V09 native encrypted Ranger Manaphy gift history: PASS\n";
}
