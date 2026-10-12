#pragma once

#include "UI/SharedPokemonEditorContract.h"

#include <array>
#include <cstddef>

namespace PokeBank::UIModel::NarrowPokemonMenu {

// This is a *presentation* of real staged-editor commands, not a second
// mutation/action dispatcher. nativeIndex is the existing generation's action
// index. -1 means a deliberately unavailable, visibly grey and inert option.
struct Row {
    const char* label = "";
    int nativeIndex = -1;
    [[nodiscard]] constexpr bool enabled() const noexcept { return nativeIndex >= 0; }
};

struct Model {
    std::array<Row, 14> rows{};
    int count = 0;

    constexpr void append(const char* label, int nativeIndex = -1) noexcept {
        if (count < static_cast<int>(rows.size()))
            rows[static_cast<std::size_t>(count++)] = {label, nativeIndex};
    }
    [[nodiscard]] constexpr int visualIndex(int nativeIndex) const noexcept {
        // -1 is the disabled-row sentinel, never a selectable native action.
        if (nativeIndex < 0) return -1;
        for (int i = 0; i < count; ++i)
            if (rows[static_cast<std::size_t>(i)].nativeIndex == nativeIndex)
                return i;
        return -1;
    }
};

// Match the ORIGINAL narrow action popup's overall size and subtle disabled
// cards. Show every command at once: no inaccessible bottom row behind a scroll.
// Dense menus adapt their row height to the 664px content area, while smaller
// menus retain large fingertip-sized 54px rows.
struct Geometry {
    int width = 440;
    int headerHeight = 78;
    int rowHeight = 54;
    int bottomHeight = 34;
    int height = 0;
    int x = 0;
    int y = 0;

    [[nodiscard]] constexpr int rowY(int index) const noexcept {
        return y + headerHeight + index * rowHeight;
    }
    [[nodiscard]] constexpr int rowX() const noexcept { return x + 10; }
    [[nodiscard]] constexpr int rowWidth() const noexcept { return width - 20; }
};

[[nodiscard]] constexpr Geometry geometry(int actionCount, int screenW = 1280,
                                          int screenH = 720, int footerH = 56) noexcept {
    Geometry g{};
    g.rowHeight = actionCount <= 9 ? 54 : actionCount == 10 ? 52 :
                  actionCount == 11 ? 48 : 44;
    g.height = g.headerHeight + actionCount * g.rowHeight + g.bottomHeight;
    g.x = (screenW - g.width) / 2;
    g.y = (screenH - footerH - g.height) / 2;
    return g;
}

[[nodiscard]] constexpr int classicIndex(
    const SharedPokemonEditor::ActionSet& native,
    SharedPokemonEditor::Action action) noexcept {
    for (int i = 0; i < static_cast<int>(native.count); ++i)
        if (native[static_cast<std::size_t>(i)] == action) return i;
    return -1;
}

// The same old narrow/greyed-out *look* across Gen I-III, while Edit, Clone,
// Remove, Legality and Review still go through the real existing staged handlers.
// Master Vault, external transfers and direct Make Shiny are not fabricated.
[[nodiscard]] constexpr Model classic(
    const SharedPokemonEditor::ActionSet& native, bool occupied) noexcept {
    using A = SharedPokemonEditor::Action;
    Model result{};
    if (!occupied) {
        result.append("Add Pokemon", classicIndex(native, A::Add));
        const int review = classicIndex(native, A::Review);
        if (review >= 0) result.append("Review Pending Changes", review);
        result.append("Cancel", classicIndex(native, A::Close));
        return result;
    }
    result.append("View Pokemon", classicIndex(native, A::View));
    result.append("Add to Master Vault");
    result.append("Add to Bank...");
    result.append("Transfer to Game...");
    result.append("Edit", classicIndex(native, A::Edit));
    result.append("Clone", classicIndex(native, A::Clone));
    result.append("Release", classicIndex(native, A::Remove));
    result.append("Make Shiny");
    result.append("Legality & Provenance", classicIndex(native, A::LegalityProvenance));
    const int review = classicIndex(native, A::Review);
    if (review >= 0) result.append("Review Pending Changes", review);
    result.append("Cancel", classicIndex(native, A::Close));
    return result;
}

static_assert(geometry(12).height <= 720 - 56);
static_assert(classic(
    SharedPokemonEditor::actionsForSlot(true, {true, true, true}, true), true
).visualIndex(-1) == -1);
static_assert(geometry(9).rowHeight >= 54);
static_assert(geometry(2).width == 440);
static_assert(classic(
    SharedPokemonEditor::actionsForSlot(true, {true, true, true}, true), true).count == 11);
static_assert(classic(
    SharedPokemonEditor::actionsForSlot(false), false).count == 2);

} // namespace PokeBank::UIModel::NarrowPokemonMenu
