#include "Legality/Legality.h"
#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <span>
#ifdef NDEBUG
#error assertions required
#endif
using namespace Legality;
static void put16(std::array<uint8_t,232>& b,size_t p,uint16_t v){b[p]=uint8_t(v);b[p+1]=uint8_t(v>>8);}
static std::array<uint8_t,232> make(uint16_t species,uint8_t nature=0,uint8_t ev=0){
    std::array<uint8_t,232> raw{};put16(raw,8,species);raw[0x1c]=nature;raw[0x1e]=ev;
    uint32_t sum=0;for(size_t k=8;k<raw.size();k+=2)sum+=uint16_t(raw[k])|(uint16_t(raw[k+1])<<8u);
    put16(raw,6,uint16_t(sum));return raw;
}
int main(){
    const auto src=make(807),copy=src;
    auto good=analyzePk7Record(src,false);
    assert(good.verdict()==Verdict::Incomplete && good.ok() && !good.hasInvalid());
    assert(good.coverage.structure==CoverageLevel::Complete && src==copy);
    const auto pk6=analyzePk6Record(src,false);
    assert(pk6.verdict()==Verdict::Invalid); // different native ceiling, same crypto
    assert(analyzePk7Record(make(808),false).verdict()==Verdict::Invalid);
    assert(analyzePk7Record(make(25,25),false).verdict()==Verdict::Invalid);
    assert(analyzePk7Record(make(25,0,253),false).verdict()==Verdict::Invalid);
    auto tampered=src;tampered[20]^=1;
    auto framing=analyzePk7Record(tampered,false);
    assert(framing.verdict()==Verdict::Incomplete && !framing.hasInvalid());
    assert(analyzePk7Record(std::span<const uint8_t>(src.data(),31),false).verdict()==Verdict::Incomplete);
    assert(analyzePk7Record(make(0),false).verdict()==Verdict::Incomplete);
    std::cout<<"PK7 report: good, invalid, cross-format ceiling, untrusted and empty PASS\n";
}
