#include "Legality/Gen4EventNativeFieldEvidence.h"
#include "Encryption/Encryption4.h"
#include "Enums/GameVersion.h"
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <span>
#include <vector>
namespace N=Legality::Gen4EventNativeFieldEvidence;
namespace T=Legality::Gen4EventTemplate;
namespace X=Legality::Gen4RawTextEvidence;
void w16(std::vector<std::byte>& d,std::size_t p,uint16_t n) {
    d[p]=static_cast<std::byte>(n&255u);
    d[p+1]=static_cast<std::byte>(n>>8u);
}
void w32(std::vector<std::byte>& d,std::size_t p,uint32_t n) {
    w16(d,p,static_cast<uint16_t>(n));
    w16(d,p+2,static_cast<uint16_t>(n>>16u));
}
int main() {
    // Synthetic valid native encrypted Gen IV PK4, NOT a real archived
    // retail event Pokemon. Uses projected Charmander index 0 identity.
    std::vector<std::byte> original(Encryption::SIZE_STORED4,std::byte{0});
    w32(original,0,0x0f199b50u);
    w16(original,0x08,4);w16(original,0x0c,7200);
    w16(original,0x0e,58364);
    w32(original,0x24,0x00100001u);
    w16(original,0x28,10);w16(original,0x2a,52);
    w16(original,0x2c,53);
    w32(original,0x3c,0x00000002u);
    w32(original,0x60,0x00000004u);
    w16(original,0x46,53); // extended retail met position: 3053
    original[0x40]=std::byte{1}; // fateful
    original[0x17]=std::byte{1};
    original[0x5f]=std::byte{7};
    original[0x84]=std::byte{40};
    original[0x86]=std::byte{16};
    original[0x48]=std::byte{0x41};
    w16(original,0x4a,0xffffu);
    original[0x68]=std::byte{0x52};
    w16(original,0x6a,0xffffu);
    w16(original,0x06,Encryption::checksum4(original));
    const auto encrypted=Encryption::encryptArray4(original);
    assert(encrypted.size()==0x88);
    Pokemon::Pokemon4ReadOnly p(encrypted,Enums::GameVersion::HGSS);
    assert(p.valid() && !p.empty());
    const auto bytesBefore=p.originalEncryptedBytes();
    const std::vector<std::byte> preserved(bytesBefore.begin(),bytesBefore.end());
    const auto snap=N::project(p);
    assert(snap.available && snap.text.status==X::Status::Known);
    assert(snap.text.fullyTerminated());
    assert(T::matches(T::kGen4EventTemplates[0],snap.identity));
    assert(snap.observedMoves[0]==10 && snap.observedMoves[1]==52 &&
           snap.observedMoves[2]==53 && snap.observedMoves[3]==0);
    assert(snap.nativeRibbonWords[0]==0x00100001u);
    assert(snap.nativeRibbonWords[1]==2u && snap.nativeRibbonWords[2]==4u);
    assert(snap.rawNickname[0]==std::byte{0x41});
    assert(snap.rawOT[0]==std::byte{0x52});

    // Original-stage archive moves/ribbons/raw OT remain unavailable even
    // when native-final fields were decoded completely.
    auto unavailableSource=Legality::Gen4EventArchiveAttestation::Source{};
    assert(unavailableSource.fields[0].origin==
           Legality::Gen4EventArchiveAttestation::FieldOrigin::Unavailable);
    auto corrupted=encrypted;
    corrupted[0x20]^=std::byte{0x01};
    Pokemon::Pokemon4ReadOnly invalid(corrupted,Enums::GameVersion::HGSS);
    assert(!invalid.valid());
    assert(!N::project(invalid).available);
    std::array<std::byte,8> shortBytes{};
    Pokemon::Pokemon4ReadOnly truncated(shortBytes,Enums::GameVersion::HGSS);
    assert(!N::project(truncated).available);
    const auto after=p.originalEncryptedBytes();
    assert(after.size()==preserved.size());
    for(std::size_t i=0;i<after.size();++i)assert(after[i]==preserved[i]);
    std::cout<<"L4 native encrypted PK4 final-field projection: PASS\n";
}
