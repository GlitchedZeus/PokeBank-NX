// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include <algorithm>
#include <cstdint>
#include <vector>
namespace Legality::Breeding {
struct BirthMovesData {
    std::vector<uint16_t> initial,levels,eggs,machines;
    bool voltTackle=false;
};
// Forward native move insertion. A known move is skipped; only a genuinely new
// move pushes the oldest slot out. HGSS InheritMoves/TryAppendBoxMonMove and
// Emerald BuildEggMoveset are the original mechanism references. Generation V
// runs its machine phase after shared level-up inheritance.
inline std::vector<uint16_t> inheritBirthMoves(const BirthMovesData& data,uint8_t generation,
        const std::vector<uint16_t>& father,const std::vector<uint16_t>& mother,bool lightBall=false){
    auto result=data.initial;
    const auto has=[](const auto& values,uint16_t move){return std::find(values.begin(),values.end(),move)!=values.end();};
    const auto append=[&](uint16_t move){if(!move||has(result,move))return;if(result.size()==4)result.erase(result.begin());result.push_back(move);};
    for(auto move:father)if(has(data.eggs,move))append(move);
    const auto machine=[&](){for(auto move:father)if(has(data.machines,move))append(move);};
    const auto shared=[&](){for(auto move:father)if(has(mother,move)&&has(data.levels,move))append(move);};
    if(generation==5){shared();machine();}else{machine();shared();}
    if(data.voltTackle&&lightBall)append(344);
    return result;
}
}
