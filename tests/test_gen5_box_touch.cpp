#include "Integration/Gen5/Gen5BoxTouchNavigation.h"

#include <cassert>
#include <iostream>

namespace G=PokeVault::Integration::Gen5;

int main() {
    using K=G::BoxTouchKind5;
    // Native full 6x5 box hit IDs. One tap focuses; tapping the already
    // selected cell opens the SAME guarded Gen V Pokémon Actions.
    for(int selected=-1;selected<30;++selected) {
        for(int hit=0;hit<30;++hit) {
            const auto result=G::decodeGen5BoxTouch(hit,selected,30);
            assert(result.slot==hit);
            assert(result.kind==(selected==hit?K::OpenSlot:K::FocusSlot));
        }
    }
    // Inherited box arrows work without opening the generic source writer.
    assert(G::decodeGen5BoxTouch(1000,0,30).kind==K::PreviousBox);
    assert(G::decodeGen5BoxTouch(1001,0,30).kind==K::NextBox);
    assert(G::decodeGen5BoxTouch(2000,29,30).kind==K::OpenSummary);
    assert(G::decodeGen5BoxTouch(2000,29,30).slot==29);
    // Header rename, storage actions, invalid/empty/out-of-range and the
    // Gen V modal touch namespace never trigger any inherited mutation.
    for(int hit:{-1,-9,30,31,1002,1003,1100,2001,77000,80000}) {
        assert(G::decodeGen5BoxTouch(hit,0,30).kind==K::Ignore);
    }
    for(int selected:{-1,30,31}) {
        assert(G::decodeGen5BoxTouch(2000,selected,30).kind==K::Ignore);
    }
    for(int capacity:{-3,0,31,100}) {
        for(int hit:{0,20,1000,1001,2000})
            assert(G::decodeGen5BoxTouch(hit,0,capacity).kind==K::Ignore);
    }
    for(int capacity=1;capacity<=30;++capacity) {
        assert(G::decodeGen5BoxTouch(capacity,0,capacity).kind==K::Ignore);
        assert(G::decodeGen5BoxTouch(capacity-1,0,capacity).slot==capacity-1);
    }
    for(int count=1;count<=30;++count) {
        for(int index=0;index<count;++index) {
            for(int delta:{-30,-5,-1,0,1,5,30}) {
                const int expected=((index+delta)%count+count)%count;
                assert(G::advanceGen5BoxIndex(index,count,delta)==expected);
            }
        }
    }
    assert(G::advanceGen5BoxIndex(0,0,-5)==0);
    assert(G::advanceGen5BoxIndex(-1,24,5)==-1);
    assert(G::advanceGen5BoxIndex(24,24,5)==24);
    std::cout<<"Gen V shared box touch: focus/second tap, source-safe IDs,"
                " 24-box jumps and invalid geometry PASS\n";
}
