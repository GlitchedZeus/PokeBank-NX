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
//   * A divider, then the read-only identity rows (gold, unselectable), one per native field.
//   * A visible "Y Review Changes" button in its own slot at the bottom of the card.
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
inline constexpr int kInfoH = 42, kInfoGap = 6, kMinInfoH = 30;
inline constexpr int kReviewW = 240, kReviewH = 48, kReviewMargin = 12;
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
    bool fits = true; // every row ends above the Review button
};

inline Page layout(const std::vector<Row>& rows, int x, int y, int w, int h) {
    Page page;
    page.panel = {x, y, w, h};
    page.header = {x, y, w, kHeaderH};
    const int cardX = x + (w - kCardW) / 2;
    // The Review button is right-aligned with the cards, at the bottom of the panel.
    page.review = {cardX + kCardW - kReviewW, y + h - kReviewMargin - kReviewH, kReviewW, kReviewH};
    int cy = y + kHeaderH + kTop;
    int info = 0;
    for (const auto& row : rows) {
        if (row.editable) {
            page.rows.push_back({row, {cardX, cy, kCardW, kEditH}});
            cy += kEditH + kEditGap;
        } else {
            ++info;
        }
    }
    cy += kDividerBefore;
    page.dividerY = cy;
    cy += kDividerAfter;
    // Read-only rows share what is left above the Review button, at most the Gen V height.
    const int available = page.review.y - kReviewMargin - cy;
    const int infoH = info ? std::clamp((available + kInfoGap) / info - kInfoGap, kMinInfoH, kInfoH) : kInfoH;
    for (const auto& row : rows) {
        if (row.editable) continue;
        page.rows.push_back({row, {cardX, cy, kCardW, infoH}});
        cy += infoH + kInfoGap;
    }
    page.fits = cy - kInfoGap <= page.review.y - kReviewMargin;
    return page;
}

// Where the label and the value text of a placed row are drawn, given their measured widths and
// the text height. Editable rows carry the value in a pill; read-only rows right-align it.
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
