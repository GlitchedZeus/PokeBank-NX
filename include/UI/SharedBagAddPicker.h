#pragma once

// The ONE Bag "Add Item" picker and Remove-confirm presentation for the Gen I-V surfaces. The
// picker is the shared compact grid (UI/SharedItemGridPicker.h, R2-BUG-006); the confirmations are
// laid out by UI/SharedMenuGeometry.h (MenuType::RemoveConfirm / KeyItemWarning). The decision of
// what A means lives in UI/SharedBagAddFlow.h; this header only draws and hit-tests.
//
// One rectangle per visible cell is BOTH the focus highlight and the touch target
// (SharedEditorChrome::drawMenuCell), and input hit-tests through the SAME grid
// (SharedItemGridPicker::hitIndex), so what is highlighted is exactly what a tap selects.
// The frame is content-driven (a short pocket draws a short box) and never covers the footer.

#include "UI/ItemPickerArtwork.h"
#include "UI/SharedBagAddFlow.h"
#include "UI/SharedEditorChrome.h"
#include "UI/SharedItemGridPicker.h"
#include "UI/SharedMenuGeometry.h"
#include "UI/SharedScrollbar.h"

#include <string>
#include <string_view>

namespace UI::SharedBagAddPicker {

namespace MG = PokeBank::UIModel::SharedMenuGeometry;
namespace Grid = UI::SharedItemGridPicker;

// R2-BUG-006: the Add picker is the shared compact grid (UI/SharedItemGridPicker.h): up to 4
// columns x 10 rows of sprite + name + held quantity, never clipping a name. Input and draw both
// build the grid from the same labels (pickerGrid), so the cell a tap lands on is the cell drawn.
// The decision of what A means stays in UI/SharedBagAddFlow.h (Add stages x1, no keypad).
template <class LabelAt>
[[nodiscard]] inline Grid::Grid pickerGrid(int count, LabelAt&& labelAt) {
    return Grid::gridFor(count, labelAt, /*heldColumn=*/true);
}

// Trailing column of a cell: the quantity already held ("x12"), empty when not held. A held
// item stays choosable (A focuses its stack, UI/SharedBagAddFlow.h).
inline constexpr int kHeldColumnW = Grid::kHeldColumnW;
[[nodiscard]] inline std::string heldLabel(uint16_t held) {
    return held == 0 ? std::string() : "x" + std::to_string(held);
}

// Draw the picker for a cursor-driven owner (the window centres the selection). labelAt(index)
// -> std::string; itemAt(index) -> native item id; heldAt(index) -> quantity already in the
// pocket (0 = none); machineMoveAt(index) -> the move this generation's TM/HM teaches (0 = not a
// machine) for the R2-BUG-005 detail line. Each visible cell publishes touch id touchBase + index;
// the draggable scrollbar publishes into `scrollbar`.
template <class Buttons, class LabelAt, class ItemAt, class HeldAt, class MachineMoveAt>
inline void drawPicker(PKSEFramebuffer& fb, Buttons& touchButtons, int touchBase,
                       Enums::GameVersion game, std::string_view title, const std::string& subtitle,
                       int count, int selected, LabelAt&& labelAt, ItemAt&& itemAt,
                       HeldAt&& heldAt, MachineMoveAt&& machineMoveAt, SharedScrollbar::State& scrollbar) {
    SharedEditorChrome::beginModal(touchButtons);
    const auto g = pickerGrid(count, labelAt);
    Grid::drawFrame(fb, g, title, subtitle, selected);
    if (selected >= 0 && selected < count)
        Grid::drawMachineDetail(fb, g.frame.x + Grid::kRule.padX, g.frame.y + Grid::kDetailTop,
                                g.frame.w - 2 * Grid::kRule.padX, game, machineMoveAt(selected));
    const int firstRow = Grid::firstRowFor(g, selected);
    Grid::drawCells(fb, g, selected, firstRow, 0,
        [&](const MG::Rect& cell, int index, bool focused) {
            SharedEditorChrome::drawMenuCell(fb, touchButtons, touchBase + index, cell, focused);
        },
        labelAt,
        [&](PKSEFramebuffer& fb, const MG::Rect& sprite, int index) {
            ItemPickerArtwork::drawNativeItem(fb, sprite.right(), sprite.y, sprite.w, game,
                                              itemAt(index), labelAt(index));
        },
        heldAt);
    const auto track = Grid::scrollbarTrack(g, SharedScrollbar::kTrackWidth);
    SharedScrollbar::drawAndPublish(fb, scrollbar, track.x, track.y, SharedScrollbar::kTrackWidth, track.h,
                                    Grid::scrollModel(g, firstRow));
}

// ---- Bag confirmations (Remove, Key Item warning) ------------------------------------------
// One presentation for every bag confirmation: SharedMenuGeometry Confirm layout (title, body
// lines, in-frame glyph-button row). A confirms the explicit Bag Remove / Key Item question;
// B always cancels. The modal owns touch and its choices are not repeated in the footer.
struct ConfirmGlyph {
    const char* glyph;   // "Y", "A", "B"
    const char* label;   // "Remove", "Confirm", "Cancel"
    bool commits;        // drawn with the commit outline
};

// Owns the frame's touch (SharedEditorChrome::beginModal: the covered footer is never tappable)
// and clears the covered footer, leaving its own glyph buttons as the visible choices.
template <class Buttons>
inline void drawConfirm(PKSEFramebuffer& fb, Buttons& touchButtons, MG::MenuType type,
                        std::string_view title, const std::string& line0, const std::string& line1,
                        ConfirmGlyph commit, ConfirmGlyph cancel, Enums::GameVersion game,
                        uint16_t itemId, const std::string& itemName) {
    SharedEditorChrome::beginModal(touchButtons);
    const auto l = MG::layout(type, 0);
    const auto& f = l.frame;
    fb.drawFilledRect(0, 0, fb.getWidth(), fb.getHeight() - kNavBarH, Color(0, 0, 0, 160));
    fb.drawFilledRoundedRect(f.x, f.y, f.w, f.h, 18, Colors::Panel);
    fb.drawRoundedRect(f.x, f.y, f.w, f.h, 18, Colors::Divider, 1);
    fb.drawText(f.x + l.rule.padX, f.y + 14, std::string(title), Colors::Text, TextStyle::Heading);
    const int top0 = MG::bodyLineTop(l, 0);
    if (itemId != 0)
        ItemPickerArtwork::drawNativeItem(fb, f.right() - l.rule.padX, top0 - 3, 30, game, itemId, itemName);
    fb.drawText(f.x + l.rule.padX, top0, line0, Colors::Text, TextStyle::Body);
    fb.drawText(f.x + l.rule.padX, MG::bodyLineTop(l, 1) + 4, line1, Colors::TextSecondary,
                TextStyle::Caption);
    const ConfirmGlyph glyphs[2] = {commit, cancel};
    for (int i = 0; i < 2; ++i) {
        const auto r = MG::buttonRect(l, i, 2);
        fb.drawRoundedRect(r.x, r.y, r.w, r.h, 12,
                           glyphs[i].commits ? (type == MG::MenuType::RemoveConfirm ? Colors::Error : Colors::Accent)
                                             : Colors::Divider,
                           glyphs[i].commits ? 2 : 1);
        drawGlyphButton(fb,r.x,r.y,r.w,r.h,glyphs[i].glyph,glyphs[i].label);
    }
    drawNavBar(fb,std::vector<ControllerHint>{}); // Visible choices live inside this modal.
}

// Remove confirmation: the exact stack and the staged-only note; A commits, B cancels.
template <class Buttons>
inline void drawRemoveConfirm(PKSEFramebuffer& fb, Buttons& touchButtons, Enums::GameVersion game,
                              std::string_view title, uint16_t itemId, const std::string& itemName,
                              uint16_t count, const std::string& note) {
    drawConfirm(fb, touchButtons, MG::MenuType::RemoveConfirm, title, itemName + "  |  Count " + std::to_string(count),
                note, {"A", "Remove", true}, {"B", "Cancel", false}, game, itemId, itemName);
}

// Key Item warning (UI/SharedBagAddFlow.h needsKeyItemWarning): shown before a Key Item is added
// to or removed from the staged bag; A confirms, B cancels with nothing staged.
template <class Buttons>
inline void drawKeyItemWarning(PKSEFramebuffer& fb, Buttons& touchButtons, Enums::GameVersion game,
                               uint16_t itemId, const std::string& itemName,
                               std::string_view pocketName, bool removing) {
    drawConfirm(fb, touchButtons, MG::MenuType::KeyItemWarning, "Key Item Warning",
                PokeBank::UIModel::BagAdd::keyItemWarningQuestion(itemName, pocketName, removing),
                "Story/event state is not replayed or undone; no event flags change.",
                {"A", "Confirm", true}, {"B", "Cancel", false}, game, itemId, itemName);
}

} // namespace UI::SharedBagAddPicker
