#pragma once

#include "UI/SharedPokemonEditorContract.h"
#include "UI/Gen1PokemonEditorUIContract.h"
#include "UI/SharedMenuGeometry.h"

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

// Phase 2 (owner round 1: "shrink box menu ... not so much blank space"): the sheet takes its
// row pitch, header and padding from the ONE BoxMenu rule (UI/SharedMenuGeometry.h), so every
// generation's box menu is the same compact size: 46 px rows (40 px cards) instead of 54.
[[nodiscard]] constexpr Geometry geometry(int actionCount, int screenW = 1280,
                                          int screenH = 720, int footerH = 56) noexcept {
    constexpr auto rule = SharedMenuGeometry::rule(SharedMenuGeometry::MenuType::BoxMenu);
    Geometry g{};
    g.width = rule.width;
    g.headerHeight = rule.headerH;
    g.bottomHeight = rule.padBottom;
    g.rowHeight = rule.rowHeight;
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

// One action vocabulary and ONE order for every generation's Pokemon Actions sheet:
//   View, Edit, Clone, Move (where supported), Release, Origin & Legality,
//   Review Pending Changes, Cancel   (empty slot: Add Pokemon, Review, Cancel)
// Only commands the staged editor really performs are listed. Shiny is edited inside the
// editor; the old greyed placeholders (Make Shiny, Master Vault, Bank, Transfer) are gone.
// Rows follow the native action set's order and keep its index, so controller order, touch
// ids (touchIdBase + native index) and dispatch are unchanged. Matches Thread C's Gen I/II
// builder (UI/ClassicPokemonActionMenu.h).
inline constexpr const char* kOriginLegalityLabel = "Origin & Legality";

[[nodiscard]] constexpr const char* sharedActionLabel(SharedPokemonEditor::Action action) noexcept {
    using A = SharedPokemonEditor::Action;
    switch (action) {
        case A::View: return "View";
        case A::Edit: return "Edit";
        case A::Clone: return "Clone";
        case A::Move: return "Move";
        case A::Remove: return "Release";
        case A::LegalityProvenance: return kOriginLegalityLabel;
        case A::Add: return "Add Pokemon";
        case A::Review: return "Review Pending Changes";
        case A::Close: return "Cancel";
        case A::None: return nullptr;
    }
    return nullptr;
}

// Canonical rank: a native set listed in any order is presented in the shared order.
[[nodiscard]] constexpr int sharedActionRank(SharedPokemonEditor::Action action) noexcept {
    using A = SharedPokemonEditor::Action;
    switch (action) {
        case A::Add: return 0;
        case A::View: return 1;
        case A::Edit: return 2;
        case A::Clone: return 3;
        case A::Move: return 4;
        case A::Remove: return 5;
        case A::LegalityProvenance: return 6;
        case A::Review: return 7;
        case A::Close: return 8;
        case A::None: return 9;
    }
    return 9;
}

[[nodiscard]] constexpr Model classic(
    const SharedPokemonEditor::ActionSet& native, bool /*occupied*/ = true) noexcept {
    Model result{};
    for (int rank = 0; rank < 9; ++rank)
        for (int i = 0; i < static_cast<int>(native.count); ++i) {
            const auto action = native[static_cast<std::size_t>(i)];
            if (sharedActionRank(action) != rank) continue;
            if (const char* label = sharedActionLabel(action)) result.append(label, i);
        }
    return result;
}

[[nodiscard]] constexpr const char* gen1ActionLabel(Gen1Editor::Action action) noexcept {
    using A = Gen1Editor::Action;
    switch (action) {
        case A::View: return "View";
        case A::Edit: return "Edit";
        case A::Clone: return "Clone";
        case A::Move: return "Move";
        case A::Remove: return "Release";
        case A::LegalityProvenance: return kOriginLegalityLabel;
        case A::AddPokemon: return "Add Pokemon";
        case A::ReviewPendingChanges: return "Review Pending Changes";
        case A::Cancel: return "Cancel";
        default: return nullptr;
    }
}

// Gen I's table pads unused entries with Cancel; list each real command once.
[[nodiscard]] constexpr Model gen1(
    const Gen1Editor::ActionSet& native, bool /*occupied*/ = true) noexcept {
    Model result{};
    bool cancelListed = false;
    for (int i = 0; i < static_cast<int>(native.count); ++i) {
        const auto action = native[static_cast<std::size_t>(i)];
        if (action == Gen1Editor::Action::Cancel) {
            if (cancelListed) continue;
            cancelListed = true;
        }
        if (const char* label = gen1ActionLabel(action)) result.append(label, i);
    }
    return result;
}

namespace detail {
constexpr bool hasLabel(const Model& m, const char* label) noexcept {
    for (int i = 0; i < m.count; ++i) {
        const char* a = m.rows[static_cast<std::size_t>(i)].label;
        const char* b = label;
        while (*a && *a == *b) { ++a; ++b; }
        if (*a == *b) return true;
    }
    return false;
}
} // namespace detail
// Every row of the shared sheet is a real command from the shared vocabulary (no placeholders).
namespace detail {
constexpr bool onlySharedLabels(const Model& m) noexcept {
    constexpr const char* allowed[] = {"View", "Edit", "Clone", "Move", "Release", kOriginLegalityLabel,
                                       "Add Pokemon", "Review Pending Changes", "Cancel"};
    for (int i = 0; i < m.count; ++i) {
        bool ok = false;
        for (const char* label : allowed) {
            const char* a = m.rows[static_cast<std::size_t>(i)].label;
            const char* b = label;
            while (*a && *a == *b) { ++a; ++b; }
            ok = ok || *a == *b;
        }
        if (!ok) return false;
    }
    return true;
}
} // namespace detail
static_assert(detail::onlySharedLabels(classic(SharedPokemonEditor::actionsForSlot(true, {true, true, true}, true))));
static_assert(detail::onlySharedLabels(gen1(Gen1Editor::actionsForSlot(true, true), true)));
static_assert(classic(SharedPokemonEditor::actionsForSlot(true, {true, true, true}, true)).count == 7);
static_assert(classic(SharedPokemonEditor::actionsForSlot(false), false).count == 2);
static_assert(classic(SharedPokemonEditor::actionsForSlot(true, {true, true, true}, true)).visualIndex(-1) == -1);
static_assert(gen1(Gen1Editor::actionsForSlot(true), true).visualIndex(-1) == -1);
static_assert(geometry(12).height <= 720 - 56);
static_assert(geometry(9).rowHeight == 46 && geometry(8).height == 78 + 8 * 46 + 34,
              "box menu: compact shared rows, content-driven height");
static_assert(geometry(2).width == 440);

} // namespace PokeBank::UIModel::NarrowPokemonMenu
