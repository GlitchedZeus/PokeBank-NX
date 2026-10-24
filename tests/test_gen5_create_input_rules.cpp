#include "Integration/Gen5/Gen5CreateInputRules.h"

#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>

namespace G=PokeVault::Integration::Gen5;
int main() {
    for(uint32_t n=0;n<10;++n)
        assert(G::createLanguageAllowed5(n)==
            (n==2 || n==3 || n==4 || n==5 || n==7));
    for(uint32_t y=2000;y<=2099;++y)for(uint32_t m=1;m<=12;++m) {
        const auto day=G::createLastCalendarDay5(y,m);
        assert(day>=28 && day<=31);
        if(m==2)assert(day==(y%4==0?29u:28u));
    }
    assert(G::createLastCalendarDay5(1999,1)==0);
    assert(G::createLastCalendarDay5(2100,1)==0);
    assert(G::createLastCalendarDay5(2024,0)==0);
    assert(G::createLastCalendarDay5(2024,13)==0);
    const std::array<uint32_t,4> ids{1,85,0,0};
    assert(!G::createMoveChoiceAllowed5(ids,0,0));
    assert(G::createMoveChoiceAllowed5(ids,0,1));
    assert(!G::createMoveChoiceAllowed5(ids,2,1));
    assert(!G::createMoveChoiceAllowed5(ids,2,85));
    assert(G::createMoveChoiceAllowed5(ids,2,0));
    assert(G::createMoveChoiceAllowed5(ids,2,559));
    assert(!G::createMoveChoiceAllowed5(ids,3,559));
    assert(!G::createMoveChoiceAllowed5(ids,4,1));
    assert(!G::createMoveChoiceAllowed5(ids,1,560));
    assert(!G::createMoveChoiceAllowed5({1,0,0,0},2,85));
    for(uint32_t value=0;value<=510;++value) {
        std::array<uint32_t,6> ev{value,0,0,0,0,0};
        assert(G::createRemainingEVMax5(ev,1)==
            (value>255?0u:std::min(255u,510u-value)));
    }
    assert(G::createRemainingEVMax5({252,252,6,0,0,0},3)==0);
    assert(G::createRemainingEVMax5({0,0,0,0,0,0},6)==0);
    assert(G::createRemainingEVMax5({300,0,0,0,0,0},1)==0);
    std::cout<<"Gen V shared Create input bounds PASS\n";
}
