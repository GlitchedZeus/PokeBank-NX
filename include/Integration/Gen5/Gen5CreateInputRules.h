#ifndef POKEBANK_GEN5_CREATE_INPUT_RULES_H
#define POKEBANK_GEN5_CREATE_INPUT_RULES_H

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>

namespace PokeVault::Integration::Gen5 {

// Reused by native Create validation and the existing shared Add UI.
// These are input constraints, not origin/encounter conclusions.
[[nodiscard]] constexpr bool createLanguageAllowed5(uint32_t value) noexcept {
    return value==2 || value==3 || value==4 || value==5 || value==7;
}
[[nodiscard]] constexpr uint32_t createLastCalendarDay5(
    uint32_t year,uint32_t month) noexcept {
    if(year<2000 || year>2099 || month<1 || month>12)return 0;
    constexpr std::array<uint32_t,12> lengths{
        31,28,31,30,31,30,31,31,30,31,30,31};
    return lengths[month-1]+(month==2 && year%4==0?1u:0u);
}
// Fail fast on a gap or duplicate; leaving move slots unfilled is valid
// only AFTER at least one occupied move.
[[nodiscard]] constexpr bool createMoveChoiceAllowed5(
    const std::array<uint32_t,4>& already,
    size_t index,uint32_t proposed) noexcept {
    if(index>=already.size() || proposed>559)return false;
    if(index==0)return proposed!=0;
    if(already[0]==0 || (already[index-1]==0 && proposed!=0))return false;
    if(proposed==0)return true;
    for(size_t i=0;i<index;++i)if(already[i]==proposed)return false;
    return true;
}
// Bound every future EV prompt to the native 510 total. The six stats
// follow the same HP/Atk/Def/Spe/SpA/SpD order as the shared PK5 editor.
[[nodiscard]] constexpr uint32_t createRemainingEVMax5(
    const std::array<uint32_t,6>& entered,size_t index) noexcept {
    if(index>=entered.size())return 0;
    uint32_t used=0;
    for(size_t i=0;i<index;++i) {
        if(entered[i]>255 || used>510-entered[i])return 0;
        used+=entered[i];
    }
    return std::min(uint32_t{255},uint32_t{510}-used);
}
static_assert(createLanguageAllowed5(2) && createLanguageAllowed5(7));
static_assert(!createLanguageAllowed5(6));
static_assert(createLastCalendarDay5(2024,2)==29);
static_assert(createLastCalendarDay5(2025,2)==28);
static_assert(createLastCalendarDay5(2100,2)==0);
static_assert(createMoveChoiceAllowed5({1,0,0,0},1,85));
static_assert(!createMoveChoiceAllowed5({1,0,0,0},1,1));
static_assert(!createMoveChoiceAllowed5({1,0,0,0},2,85));
static_assert(createRemainingEVMax5({252,252,0,0,0,0},2)==6);
static_assert(createRemainingEVMax5({255,255,0,0,0,0},2)==0);
} // namespace PokeVault::Integration::Gen5
#endif
