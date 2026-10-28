#include "UI/NatureStatEffect.h"
#include <array>
#include <cassert>
#include <iostream>

int main() {
    namespace N = PokeBank::UIModel::NatureStatEffect;
    // Independent native nature order, HP=0, Atk=1, Def=2, Spe=3, SpA=4, SpD=5.
    constexpr std::array<std::array<int,2>,25> pairs{{
        {0,0},{1,2},{1,3},{1,4},{1,5}, {2,1},{0,0},{2,3},{2,4},{2,5},
        {3,1},{3,2},{0,0},{3,4},{3,5}, {4,1},{4,2},{4,3},{0,0},{4,5},
        {5,1},{5,2},{5,3},{5,4},{0,0}}};
    int neutrals = 0;
    for (unsigned n = 0; n < pairs.size(); ++n) {
        neutrals += N::neutral(n);
        assert(N::modifier(n,0) == 0);
        for (unsigned s = 1; s <= 5; ++s)
            assert(N::modifier(n,s) == (pairs[n][0] == static_cast<int>(s) ? 1 : pairs[n][1] == static_cast<int>(s) ? -1 : 0));
    }
    assert(neutrals == 5);
    assert(N::modifier(25,1) == 0 && N::modifier(255,1) == 0 && N::modifier(3,6) == 0);
    std::cout << "All 25 native natures: five neutral, correct raised/lowered stat, HP unchanged PASS\n";
}
