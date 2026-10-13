// L5 Q12 independent source-contract differential. Does not compile/use production NX or PKHeX.
// Pure bounded rule replay: L5 has independently pinned both sides to exact source revisions.
#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>

struct Eval { bool source_hard_cap_reject; bool total_exceeds_510; };
constexpr Eval nxCurrent(const std::array<int,6>& ev) {
    int total=0;bool bad=false;
    for(int x:ev){total+=x;if(x>252)bad=true;}
    return {bad,total>510};
}
constexpr Eval pkhexPinnedEVCap(int generation,const std::array<int,6>& ev) {
    int total=0;bool bad=false;
    for(int x:ev){total+=x;if(generation>=6&&x>252)bad=true;}
    return {bad,total>510};
}
constexpr bool controls() {
    for(int g: {3,4}) {
        for(int value: {0,1,100,251,252}) {
            std::array<int,6> ev{value,0,0,0,0,0};
            if(nxCurrent(ev).source_hard_cap_reject||pkhexPinnedEVCap(g,ev).source_hard_cap_reject)return false;
        }
        for(int value: {253,254,255}) {
            std::array<int,6> ev{value,0,0,0,0,0};
            if(!nxCurrent(ev).source_hard_cap_reject)return false;
            if(pkhexPinnedEVCap(g,ev).source_hard_cap_reject)return false;
        }
        std::array<int,6> two{255,255,0,0,0,0};
        if(nxCurrent(two).total_exceeds_510||pkhexPinnedEVCap(g,two).total_exceeds_510)return false;
        std::array<int,6> above{255,255,1,0,0,0};
        if(!nxCurrent(above).total_exceeds_510||!pkhexPinnedEVCap(g,above).total_exceeds_510)return false;
    }
    std::array<int,6> modern{253,0,0,0,0,0};
    if(!pkhexPinnedEVCap(6,modern).source_hard_cap_reject)return false;
    return true;
}
static_assert(controls(), "Source-anchored EV cap boundary controls");
int main() {
    std::uint64_t compared=0,disagreement=0;
    for(int g:{3,4}) {
        std::uint64_t perGen=0;
        for(int a=0;a<256;++a)for(int b=0;b<256;++b){
            std::array<int,6> ev{a,b,0,0,0,0};
            const auto nx=nxCurrent(ev),pk=pkhexPinnedEVCap(g,ev);
            assert(!nx.total_exceeds_510&&!pk.total_exceeds_510);
            bool diff=nx.source_hard_cap_reject&&!pk.source_hard_cap_reject;
            if(diff)++perGen;
            ++compared;
        }
        assert(perGen==1527);
        disagreement+=perGen;
    }
    assert(compared==131072 && disagreement==3054);
    std::cout<<"L5 Q12 source-contract EV differential: "<<compared
             <<" bounded cases, "<<disagreement
             <<" NX-hardcap/PKHeX-reference disagreements (NOT whole-Pokemon validity)\n";
}