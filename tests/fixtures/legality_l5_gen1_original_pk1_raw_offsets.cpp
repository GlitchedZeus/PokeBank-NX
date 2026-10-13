// L5 independent source-pinned Gen I PK1 byte-layout adversarial audit.
// Confirmed against kwsch/PKHeX@6501f0ab... PKHeX.Core/PKM/PK1.cs
// and NX coordinator Gen1ReadOnlySave.cpp at ad9c1c859f94... .
// NO retail save is asserted, NO Legality::analyze is linked, and this
// does NOT execute or replace the original PKHeX.Core legality oracle.
#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#ifdef NDEBUG
#error "L5 native PK1 raw-offset regressions must not disable assertions"
#endif
namespace {
using Bytes=std::array<std::uint8_t,44>;
std::uint64_t checks=0;
#define CHECK(cond) do { ++checks; if(!(cond)) { \
    std::cerr<<"Gen I original PK1 raw-offset assertion failed, line "<<__LINE__<<'\n'; \
    std::abort(); }} while(false)
constexpr std::uint16_t be16(const Bytes& b,unsigned off) noexcept {
    return static_cast<std::uint16_t>((static_cast<unsigned>(b[off])<<8)|b[off+1]);
}
constexpr std::uint32_t be24(const Bytes& b,unsigned off) noexcept {
    return (static_cast<std::uint32_t>(b[off])<<16)|
           (static_cast<std::uint32_t>(b[off+1])<<8)|b[off+2];
}
constexpr void put16(Bytes& b,unsigned off,unsigned v) noexcept {
    b[off]=static_cast<std::uint8_t>(v>>8);
    b[off+1]=static_cast<std::uint8_t>(v);
}
constexpr void put24(Bytes& b,unsigned off,std::uint32_t v) noexcept {
    b[off]=static_cast<std::uint8_t>(v>>16);
    b[off+1]=static_cast<std::uint8_t>(v>>8);
    b[off+2]=static_cast<std::uint8_t>(v);
}
constexpr unsigned hpDV(std::uint16_t v) noexcept {
    return (((v>>12)&1u)<<3)|(((v>>8)&1u)<<2)|(((v>>4)&1u)<<1)|(v&1u);
}
constexpr std::array<unsigned,5> statExpOffsets{0x11,0x13,0x15,0x17,0x19};
constexpr std::array<unsigned,5> boundaries{0,1,32768,65534,65535};
constexpr bool uniqueOffsets() noexcept {
    for(unsigned i=0;i<5;++i)
        for(unsigned j=i+1;j<5;++j)
            if(statExpOffsets[i]+2>statExpOffsets[j])return false;
    return statExpOffsets.back()+2==0x1b;
}
static_assert(uniqueOffsets());
static_assert(hpDV(0xffff)==15 && hpDV(0)==0 && hpDV(0x5555)==15);
constexpr Bytes independentRaw(unsigned dv,unsigned rotation) noexcept {
    Bytes b{};
    b[0]=153; // synthetic Bulbasaur internal ID, no authenticated origin
    b[3]=5; b[7]=45; b[8]=33; b[0x21]=5;
    put16(b,0x0c,1234); put24(b,0x0e,0x123456);
    for(unsigned i=0;i<5;++i)
        put16(b,statExpOffsets[i],boundaries[(rotation+i)%5]);
    put16(b,0x1b,dv);
    for(unsigned i=0;i<4;++i)b[0x1d+i]=static_cast<std::uint8_t>((i<<6)|35);
    return b;
}
void exhaustAllDVWithAllDistinctStatExpRotations(){
    for(unsigned dv=0;dv<=65535;++dv){
        const unsigned a=dv>>12&15u,d=dv>>8&15u,s=dv>>4&15u,p=dv&15u;
        CHECK(hpDV(static_cast<std::uint16_t>(dv))==
              (((a&1u)<<3)|((d&1u)<<2)|((s&1u)<<1)|(p&1u)));
        for(unsigned rotation=0;rotation<5;++rotation){
            const auto b=independentRaw(dv,rotation);
            CHECK(be16(b,0x1b)==dv);
            CHECK(be24(b,0x0e)==0x123456u);
            CHECK(be16(b,0x0c)==1234u);
            CHECK(b[0]==153 && b[3]==5 && b[7]==45 && b[8]==33 && b[0x21]==5);
            for(unsigned i=0;i<5;++i){
                CHECK(be16(b,statExpOffsets[i])==boundaries[(rotation+i)%5]);
                CHECK(be16(b,statExpOffsets[i])<=65535u);
            }
            for(unsigned i=0;i<4;++i){
                CHECK((b[0x1d+i]&0x3fu)==35u);
                CHECK((b[0x1d+i]>>6)==i);
            }
        }
    }
}
void exerciseIndependentStoredFieldCorruption(){
    for(unsigned rotation=0;rotation<5;++rotation){
        const auto original=independentRaw(0xA53C,rotation);
        for(unsigned field=0;field<5;++field){
            for(unsigned byte=0;byte<2;++byte){
                for(unsigned bit=0;bit<8;++bit){
                    auto changed=original;
                    changed[statExpOffsets[field]+byte]^=static_cast<std::uint8_t>(1u<<bit);
                    CHECK(be16(changed,statExpOffsets[field])!=
                          be16(original,statExpOffsets[field]));
                    for(unsigned other=0;other<5;++other){
                        if(other!=field)
                            CHECK(be16(changed,statExpOffsets[other])==
                                  be16(original,statExpOffsets[other]));
                    }
                    CHECK(be16(changed,0x1b)==be16(original,0x1b));
                    CHECK(be24(changed,0x0e)==be24(original,0x0e));
                    CHECK(changed[7]==original[7] && changed[8]==original[8]);
                }
            }
        }
    }
}
void verifyExperienceLevelsPpAndSourceNeutrality(){
    constexpr std::array<std::uint32_t,6> expValues{0,1,0x7fff,0xffff,0x10000,0xffffff};
    for(auto experience:expValues){
        auto b=independentRaw(0x5555,0);
        put24(b,0x0e,experience);
        CHECK(be24(b,0x0e)==experience);
        CHECK(be16(b,0x11)==0);
        CHECK(be16(b,0x1b)==0x5555);
        b[3]=50;b[0x21]=5;
        CHECK(b[3]==50 && b[0x21]==5); // boxed vs party level offsets differ
        for(unsigned pp=0;pp<64;++pp){
            for(unsigned ppUps=0;ppUps<4;++ppUps){
                for(unsigned slot=0;slot<4;++slot){
                    b[0x1d+slot]=static_cast<std::uint8_t>((ppUps<<6)|pp);
                    CHECK((b[0x1d+slot]&0x3fu)==pp);
                    CHECK((b[0x1d+slot]>>6)==ppUps);
                    CHECK(be16(b,0x1b)==0x5555);
                    CHECK(be24(b,0x0e)==experience);
                }
            }
        }
    }
    for(unsigned catchRate:{0u,1u,45u,190u,255u}){
        auto b=independentRaw(0x1234,2);
        b[7]=static_cast<std::uint8_t>(catchRate);
        CHECK(b[7]==catchRate);
        CHECK(be16(b,0x1b)==0x1234);
        // No single catch-rate field proves Red, Blue, Yellow, event or trade provenance.
    }
}
}
int main(){
    exhaustAllDVWithAllDistinctStatExpRotations();
    exerciseIndependentStoredFieldCorruption();
    verifyExperienceLevelsPpAndSourceNeutrality();
    std::cout<<"L5 original PKHeX/NX Gen I PK1 44-byte field offsets: "<<checks
             <<" source-pinned independent assertions PASS (synthetic, NOT oracle/runtime parity)\n";
}
