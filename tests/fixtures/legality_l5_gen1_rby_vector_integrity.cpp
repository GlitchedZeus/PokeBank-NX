// L5 independent Gen I raw DV and Stat Experience input integrity.
// Source-independent native-format coverage; NOT an executable PKHeX oracle,
// NOT Legality::analyze, NOT authenticated retail acquisition evidence.
#include <array>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string_view>
#include <unordered_set>
#include "legality_l5_gen1_rby_oracle_cases.inc"
#ifdef NDEBUG
#error "L5 Gen I audit native-format assertions must never be disabled"
#endif
namespace {
std::uint64_t assertions=0;
#define CHECK(x) do {++assertions;if(!(x)) {std::cerr<<"L5 Gen I DV/statExp invariant failure at line "<<__LINE__<<'\n';std::abort();}}while(false)
constexpr unsigned hpDV(unsigned dv) noexcept {
    return (((dv >> 12) & 1u) << 3) |
           (((dv >>  8) & 1u) << 2) |
           (((dv >>  4) & 1u) << 1) |
           (( dv        & 1u) << 0);
}
constexpr unsigned encodeBE16(unsigned value) noexcept {
    const std::array<std::uint8_t,2> raw{
        static_cast<std::uint8_t>(value>>8),
        static_cast<std::uint8_t>(value)};
    return (static_cast<unsigned>(raw[0])<<8)|raw[1];
}
void checkAllNativeDVAndStatExp() {
    for(unsigned dv=0;dv<=65535;++dv) {
        CHECK(encodeBE16(dv)==dv);
        const unsigned attack=dv>>12&15u,defense=dv>>8&15u;
        const unsigned speed=dv>>4&15u,special=dv&15u;
        CHECK(hpDV(dv)==(((attack&1u)<<3)|((defense&1u)<<2)|
                         ((speed&1u)<<1)|(special&1u)));
        CHECK(hpDV(dv)<=15u);
        for(unsigned statExp:{0u,32768u,65535u}) {
            CHECK(encodeBE16(statExp)==statExp);
            // The original five independent Gen I 16-bit Stat Experience
            // fields may each reach 65535. The 510/252 modern EV caps
            // DO NOT constrain this stored field.
            std::array<std::uint16_t,5> values{
                static_cast<std::uint16_t>(statExp),
                static_cast<std::uint16_t>(statExp),
                static_cast<std::uint16_t>(statExp),
                static_cast<std::uint16_t>(statExp),
                static_cast<std::uint16_t>(statExp)};
            for(auto value:values)CHECK(value==statExp);
        }
    }
}
void checkSourceIndependentVectors() {
    unsigned count=0;
    std::array<unsigned,6> sources{};
    std::array<unsigned,2> species{},moves{};
    std::array<unsigned,3> dvPatterns{},statExpPatterns{};
    std::unordered_set<std::string_view> ids;
    constexpr std::array<std::string_view,6> allowedSources{
        "", "red_gb", "blue_gb", "yellow_gb", "platinum_nds", "unknown_source"};
    for(const auto& c:kL5Gen1Cases) {
        ++count;
        CHECK(ids.emplace(c.id).second);
        CHECK(c.species==1u || c.species==81u);
        CHECK(c.move==33u || c.move==200u);
        CHECK(c.level==5u && c.experience==125u);
        const int speciesIndex=c.species==1u?0:1;
        const int moveIndex=c.move==33u?0:1;
        ++species[speciesIndex];++moves[moveIndex];
        CHECK(c.dv16==0u || c.dv16==21845u || c.dv16==65535u);
        const int dvIndex=c.dv16==0u?0:c.dv16==21845u?1:2;
        ++dvPatterns[dvIndex];
        const unsigned expectedStatExp=c.dv16==0u?0u:c.dv16==21845u?32768u:65535u;
        CHECK(c.stat_experience==expectedStatExp);
        ++statExpPatterns[expectedStatExp==0u?0:expectedStatExp==32768u?1:2];
        CHECK(hpDV(c.dv16)==(c.dv16==0u?0u:15u));
        CHECK(encodeBE16(c.stat_experience)==c.stat_experience);
        bool recognized=false;
        for(std::size_t i=0;i<allowedSources.size();++i){
            if(c.source==allowedSources[i]){recognized=true;++sources[i];break;}
        }
        CHECK(recognized);
        // The string is supplied external context; none of these 72 synthetic
        // PK1 records contains authenticated native Red/Blue/Yellow metadata.
        CHECK(c.species<=151u && c.move<=255u);
    }
    CHECK(count==72u);
    for(unsigned n:sources)CHECK(n==12u);
    for(unsigned n:species)CHECK(n==36u);
    for(unsigned n:moves)CHECK(n==36u);
    for(unsigned n:dvPatterns)CHECK(n==24u);
    for(unsigned n:statExpPatterns)CHECK(n==24u);
}
}
int main(){
    static_assert(hpDV(0)==0);
    static_assert(hpDV(0xffff)==15);
    static_assert(hpDV(0x5555)==15);
    checkAllNativeDVAndStatExp();
    checkSourceIndependentVectors();
    std::cout<<"L5 Gen I raw 65,536 DV patterns x 3 Stat Experience boundaries PASS "
             <<assertions<<" independent assertions; 72 exact source-neutral vectors (NOT PKHeX runtime)\n";
}
