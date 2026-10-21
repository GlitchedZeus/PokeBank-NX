// Independent synthetic save geometry probes, not authentic distribution specimens.
#include "Legality/Gen3BerryContext.h"
#include <algorithm>
#include <cassert>
#include <iostream>
#include <vector>
#include "fixtures/gen3_berry_save.h"
using namespace Gen3BerryFixture;
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
        for(size_t footer:{7u,12u,48u}) {auto emulator=b;emulator.resize(b.size()+footer,0xFF);
            c=C::fromSave(emulator,game,2);assert(c&&c->match(game)==C::Match::Recognized);}
        auto malformedSize=b;malformedSize.push_back(0);assert(!C::fromSave(malformedSize,game,2));
        b=makeSource(game,rotation,false,false,true);c=C::fromSave(b,game,1);
        assert(c&&c->match(game)==C::Match::Recognized);
        b=makeSource(game,rotation,false,true);c=C::fromSave(b,game,2);assert(c&&c->match(game)==C::Match::Malformed);
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
