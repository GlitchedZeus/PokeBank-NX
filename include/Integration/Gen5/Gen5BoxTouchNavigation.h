#ifndef POKEBANK_GEN5_BOX_TOUCH_NAVIGATION_H
#define POKEBANK_GEN5_BOX_TOUCH_NAVIGATION_H

#include <cstdint>

namespace PokeVault::Integration::Gen5 {

// Pure input routing for the EXISTING shared 6x5 boxed-Pokemon grid.
// IDs are emitted by Panels::drawBoxPokemon, not invented Gen V coordinates.
// Never pass header-rename (1002), Storage/Bank, or other inherited touch
// actions to the generic handler on an external Gen V source.
enum class BoxTouchKind5 : uint8_t {
    Ignore, PreviousBox, NextBox, FocusSlot, OpenSlot, OpenSummary
};
struct BoxTouchAction5 {
    BoxTouchKind5 kind = BoxTouchKind5::Ignore;
    int slot = -1;
};
[[nodiscard]] constexpr BoxTouchAction5 decodeGen5BoxTouch(
    int hitId,int selectedSlot,int capacity) noexcept {
    if(capacity<=0 || capacity>30)return {};
    if(hitId==1000)return {BoxTouchKind5::PreviousBox,-1};
    if(hitId==1001)return {BoxTouchKind5::NextBox,-1};
    if(hitId==2000)
        return selectedSlot>=0 && selectedSlot<capacity
            ? BoxTouchAction5{BoxTouchKind5::OpenSummary,selectedSlot}
            : BoxTouchAction5{};
    if(hitId>=0 && hitId<capacity)
        return {selectedSlot==hitId?BoxTouchKind5::OpenSlot:
                                    BoxTouchKind5::FocusSlot,hitId};
    return {};
}
// ZL/ZR jump 5 boxes, while L/R step 1; reject empty geometry.
[[nodiscard]] constexpr int advanceGen5BoxIndex(
    int selectedBox,int count,int delta) noexcept {
    if(count<=0 || selectedBox<0 || selectedBox>=count)return selectedBox;
    const int step=delta%count;
    return (selectedBox+step+count)%count;
}
static_assert(decodeGen5BoxTouch(0,-1,30).kind==BoxTouchKind5::FocusSlot);
static_assert(decodeGen5BoxTouch(0,0,30).kind==BoxTouchKind5::OpenSlot);
static_assert(decodeGen5BoxTouch(1002,0,30).kind==BoxTouchKind5::Ignore);
static_assert(decodeGen5BoxTouch(2000,-1,30).kind==BoxTouchKind5::Ignore);
static_assert(advanceGen5BoxIndex(0,24,-5)==19);
static_assert(advanceGen5BoxIndex(23,24,5)==4);

} // namespace PokeVault::Integration::Gen5
#endif
