#pragma once

#include "UI/Common.h"
#include "UI/ItemPickerArtwork.h"
#include "UI/PKSEFramebuffer.h"
#include "UI/SharedHeldItemPicker.h"
#include "UI/SharedSearchInput.h"

#include <string>

// Round 5 NEW-06 (owner, Oct 24: the Held Item search keyboard hid too many results): the Held Item
// pickers search the way the Species chooser does. Minus opens the shared key grid
// (UI/SharedSearchInput.h) in a right-hand column under the focused item's artwork; the results
// stay visible on the left in Grid::kSearchColumns columns and narrow live as keys are typed.
// D-pad / A / B drive the keys while the grid is open (B or Done closes it and keeps the filter);
// a touch on the keys types, a tap on a result still chooses it, exactly as in the Species chooser.
namespace UI::HeldItemSearchPanel {

namespace Grid = PokeBank::UIModel::SharedHeldItemPicker;

inline constexpr int kPanelX = Grid::gridInsetX + Grid::gridWidth(Grid::kSearchColumns) + 24;   // 544
inline constexpr int kPanelW = Grid::modalWidth - kPanelX - Grid::gridInsetX;                   // 474
inline constexpr int kKeyGridH = 176;                                   // same keys as the Species chooser
inline constexpr int kKeyGridY = Grid::modalHeight - 24 - kKeyGridH;    // 320: bottom of the column
inline constexpr int kArtSize = 96;
inline constexpr int kArtY = Grid::gridInsetY + 36;                     // under the query line
static_assert(kKeyGridY >= kArtY + kArtSize + 40, "artwork and its name stay above the keys");

inline SharedSearchInput::Rect keyGridRect(int modalX, int modalY) noexcept {
    return {modalX + kPanelX, modalY + kKeyGridY, kPanelW, kKeyGridH};
}

struct Result {
    bool consumed = false;       // the input belonged to the search (no browsing / select / cancel)
    bool queryChanged = false;   // re-filter the presentation list
};

// Controller: Minus toggles the key grid; while it is open the D-pad / A / B drive the keys.
inline Result handleKeys(SharedSearchInput::State& s, bool minus, bool up, bool down, bool left,
                         bool right, bool a, bool b) noexcept {
    Result r;
    if (minus) { s.toggleGrid(); r.consumed = true; return r; }
    if (!s.open) return r;
    const auto e = SharedSearchInput::handleKeys(s, {up, down, left, right, a, b});
    r.consumed = e != SharedSearchInput::Event::None;
    r.queryChanged = e == SharedSearchInput::Event::QueryChanged;
    return r;
}

// Touch: a gesture that starts on the keys belongs to them until release.
inline Result handleTouch(SharedSearchInput::State& s, const SharedSearchInput::Touch& t, int modalX,
                          int modalY) noexcept {
    Result r;
    const auto e = SharedSearchInput::handleTouch(s, t, keyGridRect(modalX, modalY));
    r.consumed = e != SharedSearchInput::Event::None;
    r.queryChanged = e == SharedSearchInput::Event::QueryChanged;
    return r;
}

// The right-hand column while searching: the live query, the focused result's artwork and name,
// the match count, and the key grid.
inline void draw(PKSEFramebuffer& fb, int x, int y, const SharedSearchInput::State& s,
                 const std::string& focusedName, const std::string& spriteKey, int matches) {
    const int px = x + kPanelX;
    SharedSearchInput::drawField(fb, px, y + Grid::gridInsetY, s, "Search:", "B / Done closes");
    if (!spriteKey.empty()) ItemPickerArtwork::draw(fb, px + kArtSize, y + kArtY, kArtSize, spriteKey);
    fb.drawText(px + kArtSize + 16, y + kArtY + 24, focusedName, Colors::Text, TextStyle::Body);
    // `matches` 0: the query matches nothing, so the last results stay listed (never empty).
    fb.drawText(px + kArtSize + 16, y + kArtY + 56,
                matches == 0 ? std::string("No match: last results kept")
                             : std::to_string(matches) + (matches == 1 ? " match" : " matches"),
                matches == 0 ? Colors::Warning : Colors::TextSecondary, TextStyle::Caption);
    SharedSearchInput::drawGrid(fb, s, keyGridRect(x, y));
}

} // namespace UI::HeldItemSearchPanel
