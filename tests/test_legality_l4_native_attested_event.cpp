#include "Legality/Gen4EventNativeAttestedEvidence.h"
#include "Encryption/Encryption4.h"
#include "Enums/GameVersion.h"
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <span>
#include <vector>
namespace B=Legality::Gen4EventNativeAttestedEvidence;
namespace U=Legality::Gen4UnifiedEventCandidateEvidence;
namespace A=Legality::Gen4EventArchiveAttestation;
namespace H=Legality::Gen14CandidateMoveHistory;
void w16(std::vector<std::byte>& p,std::size_t i,uint16_t n) {
    p[i]=static_cast<std::byte>(n&255u);
    p[i+1]=static_cast<std::byte>(n>>8u);
}
void w32(std::vector<std::byte>& p,std::size_t i,uint32_t n) {
    w16(p,i,static_cast<uint16_t>(n));
    w16(p,i+2,static_cast<uint16_t>(n>>16u));
}
int main() {
    // Synthetic encrypted PK4 using real pinned structural WC4 template
    // row #0 (species 4, card 67). No distribution archive is claimed.
    std::vector<std::byte> raw(Encryption::SIZE_STORED4);
    w32(raw,0,0x0f199b50u);w16(raw,0x08,4);
    w16(raw,0x0c,7200);w16(raw,0x0e,58364);
    w16(raw,0x28,10);w16(raw,0x2a,52);w16(raw,0x2c,53);
    w32(raw,0x24,1u);w32(raw,0x3c,2u);w32(raw,0x60,4u);
    raw[0x17]=std::byte{1};raw[0x40]=std::byte{1};
    raw[0x5f]=std::byte{7};raw[0x84]=std::byte{40};
    raw[0x86]=std::byte{16};w16(raw,0x46,53);
    raw[0x48]=std::byte{0x41};w16(raw,0x4a,0xffffu);
    raw[0x68]=std::byte{0x52};w16(raw,0x6a,0xffffu);
    w16(raw,0x06,Encryption::checksum4(raw));
    const auto encrypted=Encryption::encryptArray4(raw);
    Pokemon::Pokemon4ReadOnly native(encrypted,Enums::GameVersion::HGSS);
    assert(native.valid());
    const std::vector<std::byte> preserved(
        native.originalEncryptedBytes().begin(),native.originalEncryptedBytes().end());
    U::Input claimed{};
    claimed.moves.claimedCardId=67;
    claimed.moves.history={7001,60,40,true,5};
    claimed.moves.initialMovesIndependentlyVerified=true;
    claimed.moves.initialMoves={10,52,0,0};
    claimed.moves.laterRoutes[2].move=53;
    claimed.moves.laterRoutes[2].count=1;
    claimed.moves.laterRoutes[2].exhaustive=true;
    claimed.moves.laterRoutes[2].options[0]=
        {53,7001,H::Method::Tutor,40,60,2,0,0,true};
    claimed.originalEventRibbonRules={true,{1u,2u,4u},{8u,16u,32u}};
    claimed.originalEventRawTextVerified=true;
    claimed.originalNicknameRawBytesStillComparable=true;
    claimed.originalOTRawBytesStillComparable=true;
    claimed.originalEventNickname[0]=std::byte{0x41};
    claimed.originalEventNickname[2]=std::byte{0xff};
    claimed.originalEventNickname[3]=std::byte{0xff};
    claimed.originalEventOT[0]=std::byte{0x52};
    claimed.originalEventOT[2]=std::byte{0xff};
    claimed.originalEventOT[3]=std::byte{0xff};
    claimed.archiveDigestsIndependentlyVerified=true;
    claimed.moveSourceArchiveSha256[0]=std::byte{0x4a};
    claimed.ribbonSourceArchiveSha256=claimed.moveSourceArchiveSha256;
    claimed.rawTextSourceArchiveSha256=claimed.moveSourceArchiveSha256;
    A::Source s{};
    s.projectionIndex=0;s.cardId=67;s.species=4;
    s.grade=A::ArchiveGrade::OriginalPCD;
    s.originalPayloadSha256=claimed.moveSourceArchiveSha256;
    s.digestIndependentlyChecked=true;
    s.releaseIndependentlyCorroborated=true;
    for(auto& f:s.fields) {
        f.origin=A::FieldOrigin::OriginalPayload;
        f.payloadSha256=s.originalPayloadSha256;
    }
    // Fully injected original-stage "verified" flags are hypothetical
    // contracts, not licensed original source packet attestations.
    const auto compatible=B::evaluate(native,claimed,s);
    assert(compatible.status==B::Status::ConditionalWitness);
    assert(compatible.observed.observedMoves[2]==53);
    assert(compatible.observed.nativeRibbonWords[1]==2u);
    // Fake final-stage fields in the caller MUST NOT change the result.
    auto faked=claimed;
    faked.moves.stored.species=25;
    faked.moves.observedMoves={1,2,3,4};
    faked.observedRibbonWords={0,0,0};
    faked.rawNickname={};
    faked.nativeRawTextCapability=false;
    faked.nativeRibbonCapability=false;
    assert(B::evaluate(native,faked,s).status==B::Status::ConditionalWitness);
    s.grade=A::ArchiveGrade::ReconstructedFromPGT;
    assert(B::evaluate(native,claimed,s).status==B::Status::UntrustedArchive);
    s.grade=A::ArchiveGrade::OriginalPCD;
    s.digestIndependentlyChecked=false;
    assert(B::evaluate(native,claimed,s).status==B::Status::UnknownArchive);
    s.digestIndependentlyChecked=true;
    s.projectionIndex=4;
    assert(B::evaluate(native,claimed,s).status==B::Status::IdentityMismatch);
    auto broken=encrypted;
    broken[0x30]^=std::byte{0x20};
    Pokemon::Pokemon4ReadOnly invalid(broken,Enums::GameVersion::HGSS);
    assert(!invalid.valid());
    assert(B::evaluate(invalid,claimed,s).status==B::Status::UnavailableNative);
    const auto after=native.originalEncryptedBytes();
    assert(after.size()==preserved.size());
    for(std::size_t i=0;i<after.size();++i)assert(after[i]==preserved[i]);
    std::cout<<"L4 immutable native PK4 + original-event candidate joint contract: PASS\n";
}
