#pragma once

#include "UI/ClassicTrainerSession.h"

#include <algorithm>
#include <string_view>
#include <vector>

// Round 6 P2 (R2-TRAINER / NEW-18): the Gen I-IV Trainer Info page in the Gen V Trainer design.
// One model decides WHICH rows a classic game shows and WHERE each one goes; TrainerViewScreen's
// drawTrainerView paints exactly this, so a layout test sees the same rects the Switch draws.
//
//   * Selectable rows first, as Gen V cards with a value pill: Name, Money, then Play Time and
//     Gender where the generation has a native staged writer (ClassicTrainerSession rows 2 / 3).
//   * A divider, then the read-only identity rows (unselectable, neutral text as on Gen V), one
//     per native field, every row at the Gen V height (no compression).
//   * Round 7 (owner B06 / C06): a compact "Y Review Changes" button in a measured slot directly
//     below the last row, right-aligned with the cards; it never overlaps a row.
// UI/SharedTrainerPanel.h paints exactly these rects, for Gen V and Gen I-IV alike.
//
// Native capability rules (no row is invented):
//   * Gen I stores no trainer gender and no Secret ID: Trainer ID only.
//   * Gold / Silver store no trainer gender (Crystal added it): no Gender row at all.
//   * Crystal shows its stored gender; Gen III / IV show gender and two separate 16-bit IDs.
//   * Play Time is read-only unless the generation's writer is available; Badges and the Pokedex
//     summary are always display-only (Badges carries both Johto and Kanto on GSC / HGSS).
namespace PokeBank::UIModel::ClassicTrainerPage {

enum class Field { Name, Money, PlayTime, Gender, TrainerId, SecretId, Badges, Pokedex };

constexpr std::string_view label(Field field) noexcept {
    switch (field) {
        case Field::Name: return "Name";
        case Field::Money: return "Money";
        case Field::PlayTime: return "Play Time";
        case Field::Gender: return "Gender";
        case Field::TrainerId: return "Trainer ID";
        case Field::SecretId: return "Secret ID";
        case Field::Badges: return "Badges";
        case Field::Pokedex: return "Pokédex";
    }
    return "";
}

struct Row {
    Field field;
    bool editable;
    int touchId; // the ClassicTrainerSession row id for an editable row, -1 for read-only
};

struct Inputs {
    int generation = 0;            // 1-4
    bool crystal = false;          // Gen II: only Crystal stores a trainer gender
    bool playTimeEditable = false; // the generation supplied a native Play Time writer
    bool genderEditable = false;   // the generation supplied a native Gender writer
};

inline std::vector<Row> rows(const Inputs& in) {
    namespace S = ClassicTrainerSession;
    std::vector<Row> out{{Field::Name, true, 0}, {Field::Money, true, 1}};
    const bool genderStored = in.generation >= 3 || (in.generation == 2 && in.crystal);
    const bool genderEditable = in.genderEditable && genderStored;
    if (in.playTimeEditable) out.push_back({Field::PlayTime, true, S::kPlayTimeRow});
    if (genderEditable) out.push_back({Field::Gender, true, S::genderRow(in.playTimeEditable)});
    if (genderStored && !genderEditable) out.push_back({Field::Gender, false, -1});
    out.push_back({Field::TrainerId, false, -1});
    if (in.generation >= 3) out.push_back({Field::SecretId, false, -1});
    if (!in.playTimeEditable) out.push_back({Field::PlayTime, false, -1});
    out.push_back({Field::Badges, false, -1});
    out.push_back({Field::Pokedex, false, -1});
    return out;
}

struct Rect {
    int x = 0, y = 0, w = 0, h = 0;
    constexpr int bottom() const noexcept { return y + h; }
    constexpr int right() const noexcept { return x + w; }
    constexpr bool overlaps(const Rect& o) const noexcept {
        return x < o.right() && o.x < right() && y < o.bottom() && o.y < bottom();
    }
    constexpr bool inside(const Rect& o) const noexcept {
        return x >= o.x && y >= o.y && right() <= o.right() && bottom() <= o.bottom();
    }
};

// Gen V Trainer metrics (Gen5SharedReadOnlySurface drawTrainerEditor).
inline constexpr int kHeaderH = 46;
inline constexpr int kTop = 18;
inline constexpr int kCardW = 680;
inline constexpr int kEditH = 52, kEditGap = 8;
inline constexpr int kDividerBefore = 4, kDividerAfter = 14;
inline constexpr int kInfoH = 42, kInfoGap = 6;
// The Gen I-IV Review button: narrower and shorter than a row card, kReviewGap below the content,
// at least kReviewMargin above the panel's bottom edge.
inline constexpr int kReviewW = 200, kReviewH = 36, kReviewGap = 8, kReviewMargin = 8;
inline constexpr int kPillPad = 44, kPillH = 36, kTextInset = 24, kPillInset = 20;

struct Placed {
    Row row;
    Rect card;
};

struct Page {
    Rect panel;
    Rect header;
    std::vector<Placed> rows;
    int dividerY = 0;
    Rect review;
    bool fits = true; // the rows and the Review button end inside the panel
};

inline Page layout(const std::vector<Row>& rows, int x, int y, int w, int h) {
    Page page;
    page.panel = {x, y, w, h};
    page.header = {x, y, w, kHeaderH};
    const int cardX = x + (w - kCardW) / 2;
    int cy = y + kHeaderH + kTop;
    for (const auto& row : rows) {
        if (!row.editable) continue;
        page.rows.push_back({row, {cardX, cy, kCardW, kEditH}});
        cy += kEditH + kEditGap;
    }
    cy += kDividerBefore;
    page.dividerY = cy;
    cy += kDividerAfter;
    int bottom = page.dividerY;
    for (const auto& row : rows) {
        if (row.editable) continue;
        page.rows.push_back({row, {cardX, cy, kCardW, kInfoH}});
        bottom = cy + kInfoH;
        cy += kInfoH + kInfoGap;
    }
    // Measured from the real content: the slot below the last row, right-aligned with the cards.
    page.review = {cardX + kCardW - kReviewW, bottom + kReviewGap, kReviewW, kReviewH};
    page.fits = page.review.bottom() <= y + h - kReviewMargin;
    return page;
}

// Where the label and the value text of a placed row are drawn (UI/SharedTrainerPanel.h), given
// their measured widths and the text height. Editable rows carry the value in a pill; read-only
// rows right-align it.
struct TextRects {
    Rect label;
    Rect value; // the pill for an editable row
};

constexpr TextRects textRects(const Placed& placed, int labelW, int valueW, int textH) noexcept {
    const Rect& c = placed.card;
    TextRects out;
    out.label = {c.x + kTextInset, c.y + (c.h - textH) / 2, labelW, textH};
    if (placed.row.editable) {
        const int pillW = valueW + kPillPad;
        const int pillH = std::min(kPillH, c.h - 4);
        out.value = {c.right() - kPillInset - pillW, c.y + (c.h - pillH) / 2, pillW, pillH};
    } else {
        out.value = {c.right() - kTextInset - valueW, c.y + (c.h - textH) / 2, valueW, textH};
    }
    return out;
}

} // namespace PokeBank::UIModel::ClassicTrainerPage
