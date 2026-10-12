#pragma once
// L4 Stage 2: strict *read-only* encrypted PK3 -> original gift field view.
// The native record is factual; receipt/template/source are separate claims.
#include "Legality/Gen3DistributionWitnessEvidence.h"
#include "Encryption/Encryption3FRLG.h"
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>

namespace Legality::Gen3DistributionNativePK3Evidence {
namespace W = Gen3DistributionWitnessEvidence;
constexpr uint8_t read8(std::span<const std::byte> b,std::size_t o) noexcept {
    return std::to_integer<uint8_t>(b[o]);
}
constexpr uint16_t read16(std::span<const std::byte> b,std::size_t o) noexcept {
    return static_cast<uint16_t>(read8(b,o)|(uint16_t(read8(b,o+1))<<8));
}
constexpr uint32_t read32(std::span<const std::byte> b,std::size_t o) noexcept {
    return uint32_t(read16(b,o))|(uint32_t(read16(b,o+2))<<16);
}
// This takes canonical, externally checksum-verified records; the public
// inspectEncrypted entrypoint below performs the validation itself.
constexpr W::Native projectCanonical(std::span<const std::byte> b,
                                    bool independentlyVerified) noexcept {
    W::Native n{};
    if(!independentlyVerified || (b.size()!=0x50 && b.size()!=0x64) ||
       read16(b,0x20)==0 || (read8(b,0x13)&1u))return n;
    n.available=true;
    n.pid=read32(b,0);
    n.speciesInternal=read16(b,0x20);
    n.ids={read16(b,0x04),read16(b,0x06)};
    n.language=read8(b,0x12);
    for(std::size_t i=0;i<10;++i)n.nickname[i]=b[0x08+i];
    for(std::size_t i=0;i<7;++i)n.otName[i]=b[0x14+i];
    n.metLocation=read8(b,0x45);
    const uint16_t origins=read16(b,0x46);
    n.metLevel=static_cast<uint8_t>(origins&0x7fu);
    n.originGame=static_cast<uint8_t>((origins>>7)&0xfu);
    n.ball=static_cast<uint8_t>((origins>>11)&0xfu);
    n.trainerGender=static_cast<uint8_t>((origins>>15)&1u);
    for(std::size_t i=0;i<4;++i)n.moves[i]=read16(b,0x2c+2*i);
    n.heldItem=read16(b,0x22);
    n.egg=(read32(b,0x48)&(1u<<30))!=0;
    n.ribbonBits=read32(b,0x4c);
    n.fateful=(n.ribbonBits&(1u<<31))!=0;
    return n;
}
inline W::Native inspectEncrypted(std::span<const std::byte> encrypted) noexcept {
    if(encrypted.size()!=Encryption::SIZE_STORED3_FRLG &&
       encrypted.size()!=Encryption::SIZE_PARTY3_FRLG)return {};
    if((read8(encrypted,0x13)&1u)!=0)return {}; // bad egg
    const std::unique_ptr<std::byte[]> data(Encryption::decryptArray3FRLG(encrypted));
    if(!data)return {};
    const std::span<const std::byte> canonical(data.get(),encrypted.size());
    if(read16(canonical,0x1c)!=Encryption::checksum3FRLG(canonical))return {};
    return projectCanonical(canonical,true);
}
inline W::Result evaluateEncrypted(std::span<const std::byte> encrypted,
                                   const W::OriginalCandidate& source={}) noexcept {
    return W::evaluate(inspectEncrypted(encrypted),source);
}
static_assert(read16(std::array<std::byte,2>{std::byte{0x34},std::byte{0x12}},0)==0x1234u);
} // namespace Legality::Gen3DistributionNativePK3Evidence
