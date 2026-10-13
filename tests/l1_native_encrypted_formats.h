#pragma once
// Test-only independent byte-level PK3 and PK4 native ciphertext reference.
// PK3 0x50/0x64, four 12-byte shuffle blocks + XOR PID^OT32.
// PK4 0x88/0xEC, four 32-byte shuffle blocks, checksum-keyed LCRNG XOR.
// Source crosschecks: production Encryption3FRLG.cpp and Encryption4.cpp;
// PKHeX PokeCrypto, documented original handheld PK3/PK4 layouts.
// This utility NEVER overwrites source save data.
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstddef>
#include <span>
#include <vector>
namespace L1NativeEncryptedFormats {
inline constexpr std::array<std::array<uint8_t,4>,24> kPosition{{
    {{0,1,2,3}},{{0,1,3,2}},{{0,2,1,3}},{{0,3,1,2}},{{0,2,3,1}},{{0,3,2,1}},
    {{1,0,2,3}},{{1,0,3,2}},{{2,0,1,3}},{{3,0,1,2}},{{2,0,3,1}},{{3,0,2,1}},
    {{1,2,0,3}},{{1,3,0,2}},{{2,1,0,3}},{{3,1,0,2}},{{2,3,0,1}},{{3,2,0,1}},
    {{1,2,3,0}},{{1,3,2,0}},{{2,1,3,0}},{{3,1,2,0}},{{2,3,1,0}},{{3,2,1,0}}
}};
inline uint16_t rd16(std::span<const uint8_t> x,std::size_t off) noexcept {
    return static_cast<uint16_t>(x[off] | (uint16_t(x[off+1])<<8));
}
inline uint32_t rd32(std::span<const uint8_t> x,std::size_t off) noexcept {
    return uint32_t(rd16(x,off)) | (uint32_t(rd16(x,off+2))<<16);
}
inline void wr16(std::span<uint8_t> x,std::size_t off,uint16_t value) noexcept {
    x[off]=static_cast<uint8_t>(value);x[off+1]=static_cast<uint8_t>(value>>8);
}
inline void wr32(std::span<uint8_t> x,std::size_t off,uint32_t value) noexcept {
    wr16(x,off,static_cast<uint16_t>(value));wr16(x,off+2,static_cast<uint16_t>(value>>16));
}
inline uint16_t checksum(std::span<const uint8_t> x,std::size_t start,std::size_t stop) noexcept {
    uint32_t sum=0;
    for(auto o=start;o+1<stop;o+=2)sum+=rd16(x,o);
    return static_cast<uint16_t>(sum);
}
inline std::vector<uint8_t> shuffle(std::span<const uint8_t> data,std::size_t off,
                                    std::size_t blockSize,uint8_t permutation,bool encrypt) {
    std::vector<uint8_t> result(data.begin(),data.end());
    const auto& positions=kPosition[permutation%24];
    for(std::size_t canonical=0;canonical<4;++canonical){
        auto from=encrypt ? canonical : positions[canonical];
        auto to=encrypt ? positions[canonical] : canonical;
        std::copy_n(data.begin()+static_cast<std::ptrdiff_t>(off+from*blockSize),
                    static_cast<std::ptrdiff_t>(blockSize),
                    result.begin()+static_cast<std::ptrdiff_t>(off+to*blockSize));
    }
    return result;
}
inline void xorPk3(std::vector<uint8_t>& x) noexcept {
    const auto key=rd32(x,0)^rd32(x,4);
    for(std::size_t off=0x20;off<0x50;off+=4)wr32(x,off,rd32(x,off)^key);
}
inline std::vector<uint8_t> encrypt3(std::span<const uint8_t> canonical) {
    if(canonical.size()!=0x50 && canonical.size()!=0x64)return {};
    auto x=shuffle(canonical,0x20,12,static_cast<uint8_t>(rd32(canonical,0)%24),true);
    wr16(x,0x1C,checksum(canonical,0x20,0x50));
    xorPk3(x);return x;
}
inline std::vector<uint8_t> decrypt3(std::span<const uint8_t> encrypted) {
    if(encrypted.size()!=0x50 && encrypted.size()!=0x64)return {};
    std::vector<uint8_t> x(encrypted.begin(),encrypted.end());xorPk3(x);
    return shuffle(x,0x20,12,static_cast<uint8_t>(rd32(x,0)%24),false);
}
inline void cryptLcrng(std::vector<uint8_t>& x,std::size_t lo,std::size_t hi,uint32_t seed) noexcept {
    for(auto i=lo;i+1<hi;i+=2){
        seed=seed*0x41C64E6Du+0x6073u;
        wr16(x,i,rd16(x,i)^static_cast<uint16_t>(seed>>16));
    }
}
inline std::vector<uint8_t> encrypt4(std::span<const uint8_t> canonical) {
    if(canonical.size()!=0x88 && canonical.size()!=0xEC)return {};
    const auto pid=rd32(canonical,0);
    std::vector<uint8_t> filled(canonical.begin(),canonical.end());
    const auto sum=checksum(filled,8,0x88);wr16(filled,6,sum);
    auto x=shuffle(filled,8,32,static_cast<uint8_t>(((pid>>13)&31u)%24u),true);
    cryptLcrng(x,8,0x88,sum);
    if(x.size()==0xEC)cryptLcrng(x,0x88,x.size(),pid);
    return x;
}
inline std::vector<uint8_t> decrypt4(std::span<const uint8_t> encrypted) {
    if(encrypted.size()!=0x88 && encrypted.size()!=0xEC)return {};
    std::vector<uint8_t> x(encrypted.begin(),encrypted.end());
    const auto pid=rd32(x,0);const auto sum=rd16(x,6);
    cryptLcrng(x,8,0x88,sum);
    if(x.size()==0xEC)cryptLcrng(x,0x88,x.size(),pid);
    return shuffle(x,8,32,static_cast<uint8_t>(((pid>>13)&31u)%24u),false);
}
inline bool valid3(std::span<const uint8_t> raw) {
    auto dec=decrypt3(raw);return !dec.empty() && rd16(dec,0x1C)==checksum(dec,0x20,0x50);
}
inline bool valid4(std::span<const uint8_t> raw) {
    auto dec=decrypt4(raw);return !dec.empty() && rd16(dec,6)==checksum(dec,8,0x88);
}
}
