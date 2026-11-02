#pragma once
// The ONE Gen I-V Bag "Top Up / Add Missing" presentation (owner Oct 28, Feature B). The decisions
// live in UI/BagTopUpModel.h; the plan in Inventory/BulkItemPlan.h; the write in
// Inventory/BulkItemApply.h through each generation's adapter (Inventory/BulkItemAdapters*.h).
// This header maps controller / touch input to the model and draws its two modals with the shared
// conventions: beginModal owns the frame's touch, SharedMenuGeometry lays both out
// (MenuType::BulkSetup / BulkPreview), SETUP rows are SharedBoxedRows boxes that are also their
// touch targets, the PREVIEW list scrolls with the shared scrollbar, and each modal draws its own
// glyph footer (drawNavBar).

#include "UI/BagTopUpModel.h"
#include "UI/SharedBoxedRows.h"
#include "UI/SharedEditorChrome.h"
#include "UI/SharedMenuGeometry.h"
#include "UI/SharedScrollbar.h"
#include "Utils/Keyboard.h"

#include <string>
#include <string_view>

namespace UI::SharedBagTopUp {

namespace Model = PokeBank::UIModel::BagTopUp;
namespace MG = PokeBank::UIModel::SharedMenuGeometry;

struct Ui {
    Model::State model;
    SharedScrollbar::State scrollbar;   // PREVIEW list thumb (published by the draw)
    [[nodiscard]] bool active() const noexcept { return model.active(); }
};

[[nodiscard]] inline MG::Layout setupLayout() noexcept { return MG::layout(MG::MenuType::BulkSetup, Model::kRows); }
[[nodiscard]] inline MG::Layout previewLayout(const Model::State& s) noexcept {
    return MG::layout(MG::MenuType::BulkPreview, static_cast<int>(s.lines.size()));
}

[[nodiscard]] inline Model::Keys keys(u64 down) noexcept {
    Model::Keys k;
    k.up = (down & HidNpadButton_Up) != 0;
    k.down = (down & HidNpadButton_Down) != 0;
    k.left = (down & HidNpadButton_Left) != 0;
    k.right = (down & HidNpadButton_Right) != 0;
    k.a = (down & HidNpadButton_A) != 0;
    k.b = (down & HidNpadButton_B) != 0;
    k.pageUp = (down & (HidNpadButton_L | HidNpadButton_ZL)) != 0;
    k.pageDown = (down & (HidNpadButton_R | HidNpadButton_ZR)) != 0;
    return k;
}

// One frame of input while the flow is open. `touchId` is the screen's tapped touch id this frame
// (SETUP rows publish touchBase + row); the PREVIEW scrollbar takes its touch first.
template <class TouchLike>
[[nodiscard]] inline Model::Action handleInput(Ui& ui, u64 down, int touchId, int touchBase, const TouchLike& touch) {
    auto& s = ui.model;
    if (s.step == Model::Step::Setup) {
        const int tapped = touchId >= touchBase && touchId < touchBase + Model::kRows ? touchId - touchBase : -1;
        return Model::setupInput(s, keys(down), tapped);
    }
    if (s.step == Model::Step::Preview) {
        const auto l = previewLayout(s);
        const auto bar = SharedScrollbar::handleTouch(ui.scrollbar, SharedScrollbar::sample(touch),
            SharedScrollbar::rows(static_cast<int>(s.lines.size()), l.visibleRows, s.scroll));
        if (bar.changed) s.scroll = bar.scroll;
        if (bar.consumed) return Model::Action::None;
        const auto action = Model::previewInput(s, keys(down), l.visibleRows);
        if (s.step != Model::Step::Preview) ui.scrollbar.reset();
        return action;
    }
    return Model::Action::None;
}

// SETUP's Amount row: the shared numpad, 1..999 (each item is then capped natively by the plan).
// It only edits the target; nothing is staged here.
inline bool promptAmount(Ui& ui) {
    const auto r = Utils::promptNumber("Top Up / Add Missing: amount per item",
                                       static_cast<int>(ui.model.quantity), 1, Model::kMaxQuantity);
    if (r.accepted) Model::setAmount(ui.model, r.value);
    return r.accepted;
}

inline Color lineColor(Model::LineKind kind) {
    switch (kind) {
        case Model::LineKind::Heading: return Colors::Text;
        case Model::LineKind::TopUp:
        case Model::LineKind::Add: return Colors::Text;
        case Model::LineKind::Skip: return Colors::TextSecondary;
        case Model::LineKind::Note: return Colors::TextSecondary;
    }
    return Colors::Text;
}

// The SETUP or PREVIEW modal over the dimmed Bag. `pocketName` names the pocket on screen;
// `context` is the game / generation line under the title.
template <class Buttons>
inline void drawTopUpModal(PKSEFramebuffer& fb, Buttons& touchButtons, Ui& ui, int touchBase,
                           const std::string& pocketName, std::string_view context) {
    SharedEditorChrome::beginModal(touchButtons);
    auto& s = ui.model;
    fb.drawFilledRect(0, 0, fb.getWidth(), fb.getHeight() - kNavBarH, Color(0, 0, 0, 160));
    if (s.step == Model::Step::Setup) {
        const auto l = setupLayout();
        const auto& f = l.frame;
        fb.drawFilledRoundedRect(f.x, f.y, f.w, f.h, 18, Colors::Panel);
        fb.drawRoundedRect(f.x, f.y, f.w, f.h, 18, Colors::Divider, 1);
        fb.drawText(f.x + l.rule.padX, f.y + 14, "Top Up / Add Missing", Colors::Text, TextStyle::Heading);
        fb.drawText(f.x + l.rule.padX, f.y + 54, std::string(context) + "  |  Key Items and HMs are never changed",
                    Colors::TextSecondary, TextStyle::Caption);
        for (int i = 0; i < Model::kRows; ++i) {
            const auto cell = MG::cellRect(l, i);
            const auto row = static_cast<Model::Row>(i);
            SharedBoxedRows::drawRow(fb, touchButtons, touchBase + i, cell, Model::rowLabel(row),
                                     {i == s.row}, Model::rowDetail(s, row, pocketName));
        }
        drawNavBar(fb, {{"Up/Down", "Choose"}, {"Left/Right", "Change"}, {"A", "OK"}, {"B", "Close"}});
        return;
    }
    if (s.step != Model::Step::Preview) return;
    const auto l = previewLayout(s);
    const auto& f = l.frame;
    fb.drawFilledRoundedRect(f.x, f.y, f.w, f.h, 18, Colors::Panel);
    fb.drawRoundedRect(f.x, f.y, f.w, f.h, 18, Colors::Divider, 1);
    fb.drawText(f.x + l.rule.padX, f.y + 14, "Preview: Top Up / Add Missing", Colors::Text, TextStyle::Heading);
    fb.drawText(f.x + l.rule.padX, f.y + 56,
                s.totals + "  |  " + Model::modeLabel(s.mode) + ", " +
                    Model::scopeLabel(s.scope, pocketName, s.sharedBagSlots) + ", up to " + std::to_string(s.quantity),
                Colors::TextSecondary, TextStyle::Caption);
    s.scroll = std::clamp(s.scroll, 0, Model::maxScroll(s, l.visibleRows));
    const int textW = l.list.w - SharedScrollbar::kTrackWidth - 12;
    fb.setClipRect(l.list.x, l.list.y, textW, l.list.h);
    for (int i = 0; i < l.visibleRows; ++i) {
        const int index = s.scroll + i;
        if (index >= static_cast<int>(s.lines.size())) break;
        const auto& line = s.lines[static_cast<std::size_t>(index)];
        const auto cell = MG::cellRect(l, i);
        const bool heading = line.kind == Model::LineKind::Heading;
        const int x = cell.x + (heading ? 0 : l.rule.cellPadX + 8);
        fb.drawText(x, MG::textTop(cell, MG::Text::Body), line.text,
                    heading ? Colors::Accent : lineColor(line.kind), TextStyle::Body);
    }
    fb.clearClip();
    SharedScrollbar::drawAndPublish(fb, ui.scrollbar, l.list.right() - SharedScrollbar::kTrackWidth, l.list.y,
                                    SharedScrollbar::kTrackWidth, l.list.h,
                                    SharedScrollbar::rows(static_cast<int>(s.lines.size()), l.visibleRows, s.scroll));
    // A confirmation modal's footer: B backs out first, A (stage everything listed) last.
    if (s.plan.empty())
        drawNavBar(fb, {{"B", "Back"}, {"Up/Down", "Scroll"}});
    else
        SharedEditorChrome::drawFooter(fb, SharedEditorChrome::Presentation::confirmFooter("A", "Stage All", "Up/Down", "Scroll"));
}

} // namespace UI::SharedBagTopUp
