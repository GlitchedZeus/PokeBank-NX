// Independent synthetic save geometry probes, not authentic distribution specimens.
#include "Legality/Gen3BerryContext.h"
#include <algorithm>
#include <cassert>
#include <iostream>
#include <vector>
namespace D=PokeVault::Integration::Gen3::Detail;
void put16(std::vector<uint8_t>& b,size_t p,uint16_t v){b[p]=v;b[p+1]=v>>8;}
void put32(std::vector<uint8_t>& b,size_t p,uint32_t v){put16(b,p,v);put16(b,p+2,v>>16);}
std::vector<uint8_t> makeSource(std::string_view game,int rotate,bool absent=false,bool badBerry=false,bool japanese=false) {
    const bool rs=game=="ruby_gba"||game=="sapphire_gba",frlg=game=="firered_gba"||game=="leafgreen_gba";
    const size_t berry=rs?0x3160:frlg?0x30EC:0x31F8,length=rs?0x530:0x34;
    std::vector<uint8_t> b(0x20000,0xFF);
    const auto phys=[&](size_t logical){return ((logical+rotate)%14)*0x1000;};
    for(size_t id=0;id<14;++id){const auto p=phys(id);std::fill_n(b.begin()+p,0x1000,0);put16(b,p+0xFF4,id);put32(b,p+0xFF8,0x08012025);put32(b,p+0xFFC,11);}
    put32(b,phys(0)+0xAC,rs?0:frlg?1:0x12345678);
    const auto at=[&](size_t p){return phys(1+p/0xF80)+p%0xF80;};
    if(!absent) {
        for(size_t i=0;i<6;++i)b[at(berry+i)]=static_cast<uint8_t>(0xBB+std::string_view("PUMKIN")[i]-'A');
        b[at(berry+6)]=0xFF;b[at(berry+0xA)]=2;b[at(berry+0x14)]=3;
        if(japanese){b[at(berry)]=0x56;b[at(berry+1)]=0x61;b[at(berry+2)]=0x84;b[at(berry+3)]=0xFF;}
        // R/S pointer bytes are deliberately nonzero; native checksum omits them.
        if(rs)for(size_t i=0xC;i<0x14;++i)b[at(berry+i)]=0xAC;
        uint32_t sum=0;for(size_t i=0;i<length-4;++i)if(!rs||i<0xC||i>=0x14)sum+=b[at(berry+i)];
        if(badBerry)++sum;
        for(size_t i=0;i<4;++i)b[at(berry+length-4+i)]=sum>>(8*i);
    }
    for(size_t id=0;id<14;++id)put16(b,phys(id)+0xFF6,D::sectorChecksum(std::span<const uint8_t>(b).subspan(phys(id),D::kChunkLengths[id])));
    return b;
}
int main(){
    using C=Legality::Gen3BerryContext;size_t checks=0;
    for(const auto game:{"ruby_gba","sapphire_gba","emerald_gba","firered_gba","leafgreen_gba"})for(int rotation=0;rotation<14;++rotation) {
        auto b=makeSource(game,rotation);const auto original=b;
        auto c=C::fromSave(b,game,2);assert(c&&c->match(game)==C::Match::Recognized);
        for(uint8_t lang:{0,6,8,255}){c=C::fromSave(b,game,lang);assert(c&&c->match(game)==C::Match::Unknown);}
        // Japanese byte decoding yields full-width Latin, not the USA ASCII name
        // (PKHeX StringConverter3.G3_JP); this is NoMatch, not InvalidUSA.
        c=C::fromSave(b,game,1);assert(c&&c->match(game)==C::Match::Unrecognized);
        assert(b==original);
        b=makeSource(game,rotation,false,false,true);c=C::fromSave(b,game,1);
        assert(c&&c->match(game)==C::Match::Recognized);
        b=makeSource(game,rotation,false,true);c=C::fromSave(b,game,2);assert(c&&c->match(game)==C::Match::Unknown);
        b=makeSource(game,rotation,true);c=C::fromSave(b,game,2);assert(c&&c->match(game)==C::Match::NoData);
        // A newer valid backup with no data overrides the older berry-bearing copy.
        const auto older=makeSource(game,rotation);
        std::copy_n(older.begin(),0xE000,b.begin());
        auto backup=makeSource(game,rotation,true);
        for(size_t sector=0;sector<14;++sector)put32(backup,sector*0x1000+0xFFC,12);
        std::copy_n(backup.begin(),0xE000,b.begin()+0xE000);
        c=C::fromSave(b,game,2);assert(c&&c->match(game)==C::Match::NoData);
        ++checks;
    }
    std::cout<<"Gen III berry source: "<<checks<<" title/rotation combinations; charset, native checksum, backup selection and immutability PASS\n";
}
