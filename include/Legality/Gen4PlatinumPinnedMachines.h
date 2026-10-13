#pragma once
// L3 Stage 4: exact numbered original Platinum TM/HM item move mapping.
// Source: pret/pokeplatinum@fe1ea418a7364c93daf0d07ae55e195452d8803c, res/items/data/{tmNN,hmNN}.json
// tm06, tm10, tm24, tm30, tm35, tm53, tm75, tm90, hm01, hm02, hm04, hm06
// Unpinned machine number means Unknown, NOT incompatible.
#include "Legality/Gen4PlatinumNativeMethodData.h"
namespace Legality::Gen4PlatinumPinnedMachines {
struct Entry {bool hidden;uint8_t number;uint16_t move;};
inline constexpr std::array<Entry,12> kPinned{{
    {false,6,92},
    {false,10,237},
    {false,24,85},
    {false,30,247},
    {false,35,53},
    {false,53,412},
    {false,75,14},
    {false,90,164},
    {true,1,15},
    {true,2,19},
    {true,4,70},
    {true,6,249}
}};
constexpr bool known(bool hidden,uint8_t number) noexcept {
    for(const auto& row:kPinned)if(row.hidden==hidden&&row.number==number)return true;
    return false;
}
constexpr bool exactMove(bool hidden,uint8_t number,uint16_t move) noexcept {
    for(const auto& row:kPinned)
        if(row.hidden==hidden&&row.number==number)return row.move==move;
    return false;
}
constexpr bool compatible(uint16_t species,bool hidden,uint8_t number,uint16_t move) noexcept {
    return known(hidden,number) && exactMove(hidden,number,move) &&
        Gen4PlatinumNativeMethodData::machineCompatible(species,hidden,number);
}
static_assert(exactMove(false,6,92));
static_assert(exactMove(false,53,412));
static_assert(compatible(1,false,6,92));
static_assert(!compatible(1,true,2,19));
static_assert(!known(false,1));
} // namespace Legality::Gen4PlatinumPinnedMachines
