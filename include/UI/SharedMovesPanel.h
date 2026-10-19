#pragma once

// The ONE right-side MOVES panel and the ONE compact Move chooser row used by the Gen I-V
// Pokemon editor (Create, Edit and View). The generation adapter supplies only native data:
// four move ids, current PP, PP Ups, a compatibility status, and its game version. Layout is a
// constexpr model so host tests can assert that nothing clips or overlaps.
//
// Visual basis: the accepted Gen I MOVES panel (Gen1PokemonDetailsPresentation.cpp): name on
// the left, PP right-aligned in its own column, status right-aligned, compact focus outline,
// no PP noise on an empty slot. Improved with fixed PP cur/max and PP Ups columns and a second
// dim line with type / category / power / accuracy where the generation has them.

#include "Names/MoveNames.h"
#include "UI/Common.h"
#include "UI/PKSEFramebuffer.h"
#include "UI/SharedMenuGeometry.h"
#include "UI/SharedMoveFacts.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <string>

namespace UI::SharedMovesPanel {

namespace Facts = PokeBank::UIModel::SharedMoveFacts;

// ---------------------------------------------------------------------------------------------
// Panel layout (all values relative to the panel's x/y; w = panel width)
// ---------------------------------------------------------------------------------------------
struct Layout {
    int x = 0, y = 0, w = 0;
    // Column captions ("PP", "Ups") baseline.
    int captionY = 0;
    int firstRowY = 0;
    static constexpr int kRowStep = 44;
    static constexpr int kRowHeight = 42;   // focus outline
    static constexpr int kInsetX = 8;       // focus outline inset from the panel edge
    static constexpr int kTextX = 18;       // name / detail line
    static constexpr int kStatusW = 84;     // widest status: "Preserved"
    // S01: each right-aligned column is as wide as its own caption, measured with the bundled
    // font (SharedMenuGeometry::textWidth), plus a fixed gap, so "PP" and "PP Ups" never run into
    // each other. Before, the 56 px "PP Ups" caption sat in a 40 px column and ended ~3 px from "PP".
    static constexpr int kUpsW = PokeBank::UIModel::SharedMenuGeometry::textWidth(
        "PP Ups", PokeBank::UIModel::SharedMenuGeometry::Text::Caption);
    static constexpr int kPpW = 64;         // "40/64"
    static constexpr int kGap = 12;
    static constexpr int kColumnGap = 20;   // between the PP and PP Ups columns
    // Right edges of the right-aligned columns.
    int statusRight() const noexcept { return x + w - 16; }
    int upsRight() const noexcept { return statusRight() - kStatusW - kGap; }
    int ppRight() const noexcept { return upsRight() - kUpsW - kColumnGap; }
    int nameX() const noexcept { return x + kTextX; }
    int nameMaxW() const noexcept { return ppRight() - kPpW - kGap - nameX(); }
    int detailMaxW() const noexcept { return statusRight() - nameX(); }
    int rowTop(int slot) const noexcept { return firstRowY + slot * kRowStep; }
    int focusX() const noexcept { return x + kInsetX; }
    int focusW() const noexcept { return w - 2 * kInsetX; }
    int line1Y(int slot) const noexcept { return rowTop(slot) + 3; }
    int line2Y(int slot) const noexcept { return rowTop(slot) + 21; }
    // Bottom of the four rows; everything below belongs to the caller (hint / data pane).
    int bottom() const noexcept { return rowTop(4) - (kRowStep - kRowHeight); }
};

// The shared shell puts the MOVES title at y+10 and the data/graph pane at y+244.
constexpr Layout layout(int x, int y, int w) noexcept {
    Layout l;
    l.x = x; l.y = y; l.w = w;
    l.captionY = y + 36;
    l.firstRowY = y + 56;
    return l;
}
inline constexpr int kDataPaneOffset = 244;

// Caption spacing contract (S01): the visible gap between the "PP" caption's right edge and the
// "PP Ups" caption's left edge is at least kColumnGap, and both values fit under their captions.
namespace detail {
namespace Geo = PokeBank::UIModel::SharedMenuGeometry;
constexpr int captionGap() noexcept {
    return Layout::kUpsW + Layout::kColumnGap - Geo::textWidth("PP Ups", Geo::Text::Caption);
}
} // namespace detail
static_assert(detail::captionGap() >= 20, "PP and PP Ups captions are clearly apart");
static_assert(detail::Geo::textWidth("+3", detail::Geo::Text::Caption) <= Layout::kUpsW &&
              detail::Geo::textWidth("63/63", detail::Geo::Text::Caption) <= Layout::kPpW,
              "values fit their columns");

// ---------------------------------------------------------------------------------------------
// Slot data supplied by the native adapter
// ---------------------------------------------------------------------------------------------
struct Slot {
    uint16_t move = 0;
    uint8_t pp = 0;
    uint8_t ppUps = 0;
    int maxPP = -1;            // -1: derive from the era's base PP and PP Ups
    const char* status = "";   // "OK", "Preserved", "Transfer", "Invalid"... or "" for none
    Color statusColor = Colors::TextDim;
};

inline std::string ppText(const Slot& slot, Enums::GameVersion game) {
    const auto f = Facts::facts(slot.move, game);
    const int maximum = slot.maxPP >= 0 ? slot.maxPP
        : Facts::maxPP(f.basePP, slot.ppUps, Facts::eraFor(game));
    return std::to_string(slot.pp) + "/" + std::to_string(maximum);
}

inline std::string upsText(const Slot& slot) {
    return "+" + std::to_string(std::min<int>(slot.ppUps, 3));
}

// Bound text without splitting a UTF-8 code point when adding an ellipsis.
inline std::string fit(PKSEFramebuffer& fb, const std::string& text, int available,
                       TextStyle style = TextStyle::Caption) {
    int width = 0, height = 0;
    fb.measureText(text, width, height, style);
    if (width <= available) return text;
    const std::string suffix = "...";
    std::string prefix = text;
    while (!prefix.empty()) {
        std::size_t end = prefix.size() - 1;
        while (end > 0 && (static_cast<unsigned char>(prefix[end]) & 0xc0) == 0x80) --end;
        prefix.resize(end);
        fb.measureText(prefix + suffix, width, height, style);
        if (width <= available) return prefix + suffix;
    }
    return {};
}

inline void drawRight(PKSEFramebuffer& fb, int right, int y, const std::string& text, Color color) {
    int w = 0, h = 0;
    fb.measureText(text, w, h, TextStyle::Caption);
    fb.drawText(right - w, y, text, color, TextStyle::Caption);
}

// Draw the four slots and column captions. `focusedSlot` < 0 draws no focus outline.
// The caller draws the panel surface + "MOVES" title and publishes touch targets from
// `layout.rowTop(slot)` / `Layout::kRowHeight`.
inline void draw(PKSEFramebuffer& fb, const Layout& l, const std::array<Slot, 4>& slots,
                 Enums::GameVersion game, int focusedSlot) {
    drawRight(fb, l.ppRight(), l.captionY, "PP", Colors::TextDim);
    drawRight(fb, l.upsRight(), l.captionY, "PP Ups", Colors::TextDim);
    for (int slot = 0; slot < 4; ++slot) {
        const auto& s = slots[static_cast<std::size_t>(slot)];
        if (slot == focusedSlot)
            fb.drawRoundedRect(l.focusX(), l.rowTop(slot), l.focusW(), Layout::kRowHeight, 6,
                               Colors::FocusBorder, 2);
        const bool empty = s.move == 0;
        const std::string name = empty ? std::string("Empty") : std::string(Names::getMoveName(s.move));
        fb.drawText(l.nameX(), l.line1Y(slot), fit(fb, name, l.nameMaxW()),
                    empty ? Colors::TextDim : Colors::Text, TextStyle::Caption);
        if (!empty) {
            drawRight(fb, l.ppRight(), l.line1Y(slot), ppText(s, game), Colors::Text);
            drawRight(fb, l.upsRight(), l.line1Y(slot), upsText(s),
                      s.ppUps ? Colors::Accent : Colors::TextDim);
            const std::string detail = Facts::detailLine(Facts::facts(s.move, game));
            if (!detail.empty())
                fb.drawText(l.nameX(), l.line2Y(slot), fit(fb, detail, l.detailMaxW()),
                            Colors::TextDim, TextStyle::Caption);
        }
        if (s.status && *s.status)
            drawRight(fb, l.statusRight(), l.line1Y(slot),
                      fit(fb, s.status, Layout::kStatusW), s.statusColor);
    }
}

// ---------------------------------------------------------------------------------------------
// Compact Move chooser rows (number + name, type, category, power, accuracy, PP)
// ---------------------------------------------------------------------------------------------
struct ChooserColumns {
    int x = 0, w = 0;
    int numberX() const noexcept { return x; }
    int nameX() const noexcept { return x + 50; }
    int nameMaxW() const noexcept { return typeX() - 12 - nameX(); }
    int typeX() const noexcept { return x + w - 380; }
    int categoryX() const noexcept { return x + w - 290; }
    int powerRight() const noexcept { return x + w - 132; }
    int accuracyRight() const noexcept { return x + w - 66; }
    int ppRight() const noexcept { return x + w; }
};

// Row text spans [x, x+w]; 712 = compact picker (760) minus its 24px insets.
constexpr ChooserColumns chooserColumns(int x, int w) noexcept { return {x, w}; }

inline std::string moveNumber(uint16_t move) {
    std::string n = std::to_string(move);
    while (n.size() < 3) n = "0" + n;
    return n;
}

// Column captions for the chooser; Category is only shown where the generation has it.
inline void drawChooserCaptions(PKSEFramebuffer& fb, const ChooserColumns& c, int y,
                                Enums::GameVersion game) {
    fb.drawText(c.numberX(), y, "No.", Colors::TextDim, TextStyle::Caption);
    fb.drawText(c.nameX(), y, "Move", Colors::TextDim, TextStyle::Caption);
    fb.drawText(c.typeX(), y, "Type", Colors::TextDim, TextStyle::Caption);
    if (Facts::categoryRule(Facts::eraFor(game)) == Facts::CategoryRule::PerMove)
        fb.drawText(c.categoryX(), y, "Category", Colors::TextDim, TextStyle::Caption);
    drawRight(fb, c.powerRight(), y, "Pwr", Colors::TextDim);
    drawRight(fb, c.accuracyRight(), y, "Acc", Colors::TextDim);
    drawRight(fb, c.ppRight(), y, "PP", Colors::TextDim);
}

// One chooser row. `y` is the row's text baseline top; move 0 is the explicit Empty choice.
inline void drawChooserRow(PKSEFramebuffer& fb, const ChooserColumns& c, int y,
                           uint16_t move, Enums::GameVersion game, bool selected) {
    const Color ink = selected ? Colors::Text : Colors::TextSecondary;
    if (move == 0) {
        fb.drawText(c.numberX(), y, "---", Colors::TextDim, TextStyle::Body);
        fb.drawText(c.nameX(), y, "Empty move slot", ink, TextStyle::Body);
        return;
    }
    const auto f = Facts::facts(move, game);
    fb.drawText(c.numberX(), y + 2, moveNumber(move), Colors::TextDim, TextStyle::Caption);
    fb.drawText(c.nameX(), y, fit(fb, Names::getMoveName(move), c.nameMaxW(), TextStyle::Body),
                ink, TextStyle::Body);
    fb.drawText(c.typeX(), y + 2, Facts::typeLabel(f), ink, TextStyle::Caption);
    if (const char* category = Facts::categoryLabel(f); *category)
        fb.drawText(c.categoryX(), y + 2, category, ink, TextStyle::Caption);
    drawRight(fb, c.powerRight(), y + 2, Facts::powerLabel(f), ink);
    drawRight(fb, c.accuracyRight(), y + 2, Facts::accuracyLabel(f), ink);
    drawRight(fb, c.ppRight(), y + 2, f.basePP > 0 ? std::to_string(f.basePP) : "\xE2\x80\x94", ink);
}

} // namespace UI::SharedMovesPanel
