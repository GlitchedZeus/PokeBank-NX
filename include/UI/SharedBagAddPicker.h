#pragma once

// The ONE Bag "Add Item" picker and Remove-confirm presentation for the Gen IV / Gen V surfaces,
// laid out by UI/SharedMenuGeometry.h (MenuType::ItemPicker / MenuType::RemoveConfirm). The
// decision of what A means lives in UI/SharedBagAddFlow.h; this header only draws and hit-tests.
//
// One rectangle per visible row is BOTH the focus highlight and the touch target
// (SharedEditorChrome::drawMenuCell), and input hit-tests through the SAME layout
// (hitChoice), so what is highlighted is exactly what a tap selects. Rows are content-driven
// (a short pocket draws a short box) and never cover the screen footer.

#include "UI/ItemPickerArtwork.h"
#include "UI/SharedEditorChrome.h"
#include "UI/SharedMenuGeometry.h"

#include <string>
#include <string_view>

namespace UI::SharedBagAddPicker {

namespace MG = PokeBank::UIModel::SharedMenuGeometry;

[[nodiscard]] constexpr MG::Layout pickerLayout(int count) noexcept {
    return MG::layout(MG::MenuType::ItemPicker, count);
}
// Rows the picker shows at once: the page size of L/R and the scroll window.
[[nodiscard]] constexpr int visibleRows(int count) noexcept { return pickerLayout(count).visibleRows; }
[[nodiscard]] constexpr int firstVisible(int selected, int count) noexcept {
    return MG::firstVisibleRow(pickerLayout(count), selected);
}
// The choice a touch at (x, y) lands on, or -1. Uses the drawn cell rectangles.
[[nodiscard]] constexpr int hitChoice(int x, int y, int selected, int count) noexcept {
    const auto l = pickerLayout(count);
    const int cell = MG::hitCell(l, x, y);
    if (cell < 0) return -1;
    const int index = MG::firstVisibleRow(l, selected) + cell;
    return index < count ? index : -1;
}
// The scrollbar track (Lane P's SharedScrollbar) sits in the frame's right padding beside the
// list rows, the same height as the list.
[[nodiscard]] constexpr MG::Rect scrollbarTrack(int count, int trackWidth) noexcept {
    const auto l = pickerLayout(count);
    return {l.list.right() + (l.rule.padX - trackWidth) / 2, l.list.y, trackWidth, l.list.h};
}
// True when (x, y) is on the row list (swipes that start here scroll the picker).
[[nodiscard]] constexpr bool onList(int x, int y, int count) noexcept {
    return pickerLayout(count).list.contains(x, y);
}

// Trailing column of a row: the quantity already held ("x12"), empty when not held. A held
// item stays choosable (A focuses its stack, UI/SharedBagAddFlow.h).
inline constexpr int kHeldColumnW = 56;
[[nodiscard]] inline std::string heldLabel(uint16_t held) {
    return held == 0 ? std::string() : "x" + std::to_string(held);
}

// Draw the picker. labelAt(index) -> std::string; itemAt(index) -> native item id;
// heldAt(index) -> quantity already in the pocket (0 = none).
// Each visible row publishes touch id touchBase + index.
template <class Buttons, class LabelAt, class ItemAt, class HeldAt>
inline void drawPicker(PKSEFramebuffer& fb, Buttons& touchButtons, int touchBase,
                       Enums::GameVersion game, std::string_view title, const std::string& subtitle,
                       int count, int selected, LabelAt&& labelAt, ItemAt&& itemAt,
                       HeldAt&& heldAt) {
    SharedEditorChrome::beginModal(touchButtons);
    const auto l = pickerLayout(count);
    const auto& f = l.frame;
    fb.drawFilledRect(0, 0, fb.getWidth(), fb.getHeight() - kNavBarH, Color(0, 0, 0, 160));
    fb.drawFilledRoundedRect(f.x, f.y, f.w, f.h, 18, Colors::Panel);
    fb.drawRoundedRect(f.x, f.y, f.w, f.h, 18, Colors::Divider, 1);
    fb.drawText(f.x + l.rule.padX, f.y + 8, std::string(title), Colors::Text, TextStyle::Heading);
    const std::string position = count > 0
        ? std::to_string(selected + 1) + " / " + std::to_string(count) : std::string("0 / 0");
    fb.drawText(f.x + l.rule.padX, f.y + 40, subtitle + "  |  " + position,
                Colors::TextSecondary, TextStyle::Caption);
    const int first = MG::firstVisibleRow(l, selected);
    for (int row = 0; row < l.visibleRows && first + row < count; ++row) {
        const int index = first + row;
        const auto cell = MG::cellRect(l, row);
        SharedEditorChrome::drawMenuCell(fb, touchButtons, touchBase + index, cell, index == selected);
        const std::string label = labelAt(index);
        const auto sprite = MG::spriteRect(l, cell);
        ItemPickerArtwork::drawNativeItem(fb, sprite.right(), sprite.y, sprite.w, game,
                                          itemAt(index), label);
        const auto text = MG::labelRect(l, cell, kHeldColumnW);
        fb.drawText(text.x, MG::textTop(cell, l.rule.label), label,
                    index == selected ? Colors::Text : Colors::TextSecondary, TextStyle::Body);
        const std::string held = heldLabel(heldAt(index));
        if (!held.empty())
            fb.drawText(text.right() + 8, MG::textTop(cell, MG::Text::Caption), held, Colors::Accent,
                        TextStyle::Caption);
    }
}

// Remove confirmation: title, the exact stack, and the staged-only note; Y commits (frozen rule),
// B cancels. The in-frame glyph row repeats the screen footer; it is not a touch target.
inline void drawRemoveConfirm(PKSEFramebuffer& fb, Enums::GameVersion game, std::string_view title,
                              uint16_t itemId, const std::string& itemName, uint16_t count,
                              const std::string& note) {
    const auto l = MG::layout(MG::MenuType::RemoveConfirm, 0);
    const auto& f = l.frame;
    fb.drawFilledRect(0, 0, fb.getWidth(), fb.getHeight() - kNavBarH, Color(0, 0, 0, 160));
    fb.drawFilledRoundedRect(f.x, f.y, f.w, f.h, 18, Colors::Panel);
    fb.drawRoundedRect(f.x, f.y, f.w, f.h, 18, Colors::Divider, 1);
    fb.drawText(f.x + l.rule.padX, f.y + 14, std::string(title), Colors::Text, TextStyle::Heading);
    const int line0 = MG::bodyLineTop(l, 0);
    ItemPickerArtwork::drawNativeItem(fb, f.right() - l.rule.padX, line0 - 3, 30, game, itemId, itemName);
    fb.drawText(f.x + l.rule.padX, line0, itemName + "  |  Count " + std::to_string(count),
                Colors::Text, TextStyle::Body);
    fb.drawText(f.x + l.rule.padX, MG::bodyLineTop(l, 1) + 4, note, Colors::TextSecondary,
                TextStyle::Caption);
    const auto remove = MG::buttonRect(l, 0, 2);
    const auto cancel = MG::buttonRect(l, 1, 2);
    fb.drawRoundedRect(remove.x, remove.y, remove.w, remove.h, 12, Colors::Error, 2);
    fb.drawText(remove.x + 18, MG::textTop(remove, MG::Text::Body), "Y  Remove", Colors::Text,
                TextStyle::Body);
    fb.drawRoundedRect(cancel.x, cancel.y, cancel.w, cancel.h, 12, Colors::Divider, 1);
    fb.drawText(cancel.x + 18, MG::textTop(cancel, MG::Text::Body), "B  Cancel", Colors::Text,
                TextStyle::Body);
}

} // namespace UI::SharedBagAddPicker
