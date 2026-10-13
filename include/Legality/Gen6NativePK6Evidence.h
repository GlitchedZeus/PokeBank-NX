#pragma once
// SPDX-License-Identifier: GPL-3.0-or-later
// Generation VI detached PK6 encrypted/unencrypted RECORD inspection only.
// Minimal native adaptation of PKHeX.Core PKM/Util/PokeCrypto.cs
//   Decrypt67, CryptArray, Shuffle67, BlockPosition
// and PKM/PK6.cs (source SHA-256 pinned in PKHEX_USER_ARCHIVE_SOURCE_MAP.json).
// The caller explicitly selects ciphertext vs decrypted data. A game/provider
// label is NOT authority to infer the format, nor is a checksum proof of a
// historical encounter, transfer, event, or breeding provenance.
// This file has no source-save output, mutation, I/O, allocations, or throwing.
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace Legality::Gen6NativePK6 {
inline constexpr std::size_t StoredBytes=232;
inline constexpr std::size_t PartyBytes=260;
inline constexpr std::size_t BlockBytes=56;
inline constexpr std::size_t DataOffset=8;
inline constexpr std::size_t StoredPayload=224;
inline constexpr std::uint16_t SpeciesCeiling=721;

enum class Layout : std::uint8_t { Unknown, Stored, Party };
enum class Status : std::uint8_t {
    UnknownLength, InvalidSanity, InvalidChecksum, EmptySlot,
    InvalidSpecies, InvalidNature, InvalidEVPerStat, InvalidEVTotal, Parsed
};
struct Evidence {
    Status status=Status::UnknownLength;
    Layout layout=Layout::Unknown;
    std::uint32_t encryptionConstant=0;
    std::uint32_t pid=0;
    std::uint16_t species=0;
    std::uint16_t evTotal=0;
    std::uint8_t nature=0;
    bool checksumVerified=false;
    bool originalSourceAuthenticated=false;
};
// Same 24 indexed permutations used by PKHeX.Core.PokeCrypto.BlockPosition.
inline constexpr std::array<std::array<std::uint8_t,4>,24> BlockPosition{{
    {{0,1,2,3}},{{0,1,3,2}},{{0,2,1,3}},{{0,3,1,2}},
    {{0,2,3,1}},{{0,3,2,1}},{{1,0,2,3}},{{1,0,3,2}},
    {{2,0,1,3}},{{3,0,1,2}},{{2,0,3,1}},{{3,0,2,1}},
    {{1,2,0,3}},{{1,3,0,2}},{{2,1,0,3}},{{3,1,0,2}},
    {{2,3,0,1}},{{3,2,0,1}},{{1,2,3,0}},{{1,3,2,0}},
    {{2,1,3,0}},{{3,1,2,0}},{{2,3,1,0}},{{3,2,1,0}}
}};
constexpr std::uint16_t read16(std::span<const std::uint8_t> input,
                               std::size_t offset) noexcept {
    return static_cast<std::uint16_t>(input[offset]) |
           static_cast<std::uint16_t>(static_cast<std::uint16_t>(input[offset+1])<<8u);
}
constexpr std::uint32_t read32(std::span<const std::uint8_t> input,
                               std::size_t offset) noexcept {
    return static_cast<std::uint32_t>(read16(input,offset)) |
           (static_cast<std::uint32_t>(read16(input,offset+2))<<16u);
}
// PKHeX PokeCrypto.CryptArray: in-place on a PRIVATE local copy only.
inline void xorWords(std::span<std::uint8_t> bytes,std::uint32_t seed) noexcept {
    for(std::size_t p=0;p<bytes.size();p+=2) {
        seed=seed*0x41C64E6Du+0x6073u;
        const auto word=static_cast<std::uint16_t>(
            read16(bytes,p)^static_cast<std::uint16_t>(seed>>16));
        bytes[p]=static_cast<std::uint8_t>(word);
        bytes[p+1]=static_cast<std::uint8_t>(word>>8);
    }
}
// Return value is evidence about this byte sequence, not global legality.
// Internal shared Gen VI/VII envelope. Only generation-specific named wrappers
// should select a species bound. Not evidence of acquisition or origin.
inline Evidence inspectWithSpeciesCeiling(std::span<const std::uint8_t> input,
                                          bool encrypted,
                                          std::uint16_t ceiling) noexcept {
    Evidence out{};
    const std::size_t length=input.size();
    if(length!=StoredBytes && length!=PartyBytes)return out;
    out.layout=length==StoredBytes?Layout::Stored:Layout::Party;
    std::array<std::uint8_t,PartyBytes> decrypted{};
    for(std::size_t i=0;i<length;++i)decrypted[i]=input[i];
    auto decoded=std::span<std::uint8_t>(decrypted.data(),length);
    out.encryptionConstant=read32(input,0);
    if(encrypted) {
        xorWords(decoded.subspan(DataOffset,StoredPayload),out.encryptionConstant);
        if(length==PartyBytes)
            xorWords(decoded.subspan(StoredBytes,PartyBytes-StoredBytes),out.encryptionConstant);
        // The PKHeX shuffle maps position i to canonical block ID order[i].
        // Preserve decrypted intermediate bytes until all block copies end.
        const auto original=decrypted;
        const auto& order=BlockPosition[((out.encryptionConstant >> 13u) & 31u) % 24u];
        for(std::size_t slot=0;slot<4;++slot)
            for(std::size_t j=0;j<BlockBytes;++j)
                decoded[DataOffset+slot*BlockBytes+j]=
                    original[DataOffset+static_cast<std::size_t>(order[slot])*BlockBytes+j];
    }
    if(read16(decoded,4)!=0){out.status=Status::InvalidSanity;return out;}
    std::uint32_t sum=0;
    for(std::size_t i=DataOffset;i<StoredBytes;i+=2)sum+=read16(decoded,i);
    if(static_cast<std::uint16_t>(sum)!=read16(decoded,6)) {
        out.status=Status::InvalidChecksum;return out;
    }
    out.checksumVerified=true;
    out.species=read16(decoded,8);
    if(!out.species) {out.status=Status::EmptySlot;return out;}
    if(out.species>ceiling) {out.status=Status::InvalidSpecies;return out;}
    out.pid=read32(decoded,0x18);
    out.nature=decoded[0x1c];
    if(out.nature>24) {out.status=Status::InvalidNature;return out;}
    for(std::size_t i=0x1e;i<=0x23;++i) {
        const auto ev=decoded[i];
        out.evTotal+=ev;
        if(ev>252) {out.status=Status::InvalidEVPerStat;return out;}
    }
    if(out.evTotal>510) {out.status=Status::InvalidEVTotal;return out;}
    out.status=Status::Parsed;
    return out;
}
inline Evidence inspect(std::span<const std::uint8_t> input,
                        bool encrypted) noexcept {
    return inspectWithSpeciesCeiling(input,encrypted,SpeciesCeiling);
}
static_assert(BlockPosition[0][0]==0);
static_assert(BlockPosition[23][0]==3);
static_assert(StoredBytes==8+4*BlockBytes);
} // namespace Legality::Gen6NativePK6
