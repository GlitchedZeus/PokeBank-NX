#pragma once

#include "UI/SharedPokemonEditorContract.h"

#include <algorithm>
#include <cstdint>

// Generation-neutral focus model of the ONE shared Pokemon editor workspace
// (DETAILS | STATS | MOVES). It was extracted unchanged from the accepted
// Generation IV workspace and now drives every generation that uses the shared
// surface (src/UI/SharedPokemonEditorSurface.inc). It never mutates a Pokemon:
// navigation only moves focus, and only an explicit activation (A, or a
// completed tap on a field) is reported to the caller, which then opens the
// field's picker or keypad. Generations supply only their row count and which
// rows are editable.
namespace PokeBank::UIModel::SharedPokemonWorkspace {

namespace Shared = SharedPokemonEditor;

// STATS rows 0..5 are IV / EV cells; rows 6..10 are the PID-linked rows.
inline constexpr int kStatRows = 6;
inline constexpr int kNatureRow = 6;
inline constexpr int kGenderRow = 7;
inline constexpr int kShinyRow = 8;
inline constexpr int kAbilityRow = 9;
inline constexpr int kPidRow = 10;
inline constexpr int kMoveRows = 4;
inline constexpr int kVisibleDetailRows = 8;

// Touch ids published by the shared workspace frame.
inline constexpr int kDetailTouchBase = 41000;
inline constexpr int kIvTouchBase = 41100;
inline constexpr int kEvTouchBase = 41120;
inline constexpr int kPidTouchBase = 41200;
inline constexpr int kMoveTouchBase = 41300;
inline constexpr int kMoveEditorTouchBase = 41400;
inline constexpr int kActionTouchBase = 41600;
inline constexpr int kPickerTouchBase = 6000;

struct Keys {
    bool l = false, r = false, up = false, down = false, left = false, right = false;
};

struct NavigationResult {
    Shared::Focus focus{};
    int verticalDirection = 0;
    bool panelChanged = false;
};

// Exactly the Gen IV editable-focus normalization: skip read-only DETAILS
// rows in the direction of travel, keep IV/EV cells, cycle the PID-linked rows
// 6..10 and fall back to the first stat cell when none is editable.
template <class DetailEditable, class PidEditable>
constexpr Shared::Focus normalizeEditable(Shared::Generation generation, Shared::Focus focus,
                                          bool editable, int detailRows,
                                          DetailEditable&& detailEditable,
                                          PidEditable&& pidEditable,
                                          int verticalDirection = 0,
                                          bool panelChanged = false) {
    focus = Shared::normalizeMoveRowFocus(generation, focus);
    if (!editable) return focus;

    if (focus.panel == Shared::Panel::Details) {
        focus.column = 0;
        if (detailEditable(static_cast<int>(focus.row))) return focus;
        int row = focus.row;
        const int direction = verticalDirection < 0 ? -1 : 1;
        for (int step = 0; step < detailRows; ++step) {
            row = (row + direction + detailRows) % detailRows;
            if (detailEditable(row)) {
                focus.row = static_cast<uint8_t>(row);
                return focus;
            }
        }
        return focus;
    }

    if (focus.panel == Shared::Panel::Values) {
        if (focus.row < kStatRows) {
            if (focus.column > 1) focus.column = 1;
            return focus;
        }
        focus.column = 0;
        if (pidEditable(static_cast<int>(focus.row))) return focus;
        int row = focus.row;
        const int direction = verticalDirection < 0 ? -1 : 1;
        for (int step = 0; step < 5; ++step) {
            row += direction;
            if (row < kNatureRow) row = kPidRow;
            if (row > kPidRow) row = kNatureRow;
            if (pidEditable(row)) {
                focus.row = static_cast<uint8_t>(row);
                return focus;
            }
        }
        if (panelChanged) focus = {Shared::Panel::Values, 0, 0};
        return focus;
    }

    focus.row = static_cast<uint8_t>(focus.row % kMoveRows);
    focus.column = 0;
    return focus;
}

// One frame of controller navigation, in the accepted Gen IV order: L/R switch
// panels, Up/Down move within a panel, Left/Right move across cells and cross
// DETAILS -> STATS -> MOVES at the panel edges. Nothing here changes a value.
template <class DetailEditable, class PidEditable>
constexpr NavigationResult navigate(Shared::Generation generation, Shared::Focus focus,
                                    const Keys& keys, bool passive, bool editable,
                                    int detailRows, DetailEditable&& detailEditable,
                                    PidEditable&& pidEditable) {
    NavigationResult result{};
    const auto panelBefore = focus.panel;
    if (keys.l)
        focus = passive ? Shared::passiveViewSwitchPanel(generation, focus, -1)
                        : Shared::switchPanel(generation, focus, -1);
    if (keys.r)
        focus = passive ? Shared::passiveViewSwitchPanel(generation, focus, 1)
                        : Shared::switchPanel(generation, focus, 1);
    if (keys.up) {
        focus = passive ? Shared::passiveViewMoveVertical(generation, focus, -1)
                        : Shared::moveVertical(generation, focus, -1);
        result.verticalDirection = -1;
    }
    if (keys.down) {
        focus = passive ? Shared::passiveViewMoveVertical(generation, focus, 1)
                        : Shared::moveVertical(generation, focus, 1);
        result.verticalDirection = 1;
    }
    if (keys.left)
        focus = passive ? Shared::passiveViewMoveColumn(generation, focus, -1)
                        : Shared::moveRowColumn(generation, focus, -1);
    if (keys.right)
        focus = passive ? Shared::passiveViewMoveColumn(generation, focus, 1)
                        : Shared::moveRowColumn(generation, focus, 1);
    result.panelChanged = focus.panel != panelBefore;
    result.focus = passive
        ? Shared::normalizePassiveViewFocus(generation, focus)
        : normalizeEditable(generation, focus, editable, detailRows, detailEditable,
                            pidEditable, result.verticalDirection, result.panelChanged);
    return result;
}

[[nodiscard]] constexpr int detailsScrollRow(Shared::Focus focus, int previous,
                                             int detailRows) noexcept {
    const int row = static_cast<int>(Shared::detailsScrollFocus(
        focus, static_cast<std::size_t>(previous < 0 ? 0 : previous)));
    return std::clamp(row, 0, std::max(0, detailRows - 1));
}

// A completed tap on a published field id focuses that field (and, in an
// editable session, activates it exactly like A).
[[nodiscard]] constexpr bool focusForTouch(int id, int detailRows, Shared::Focus& focus) noexcept {
    if (id >= kDetailTouchBase && id < kDetailTouchBase + detailRows) {
        focus = {Shared::Panel::Details, static_cast<uint8_t>(id - kDetailTouchBase), 0};
        return true;
    }
    if (id >= kIvTouchBase && id < kIvTouchBase + kStatRows) {
        focus = {Shared::Panel::Values, static_cast<uint8_t>(id - kIvTouchBase), 0};
        return true;
    }
    if (id >= kEvTouchBase && id < kEvTouchBase + kStatRows) {
        focus = {Shared::Panel::Values, static_cast<uint8_t>(id - kEvTouchBase), 1};
        return true;
    }
    if (id >= kPidTouchBase + kNatureRow && id <= kPidTouchBase + kPidRow) {
        focus = {Shared::Panel::Values, static_cast<uint8_t>(id - kPidTouchBase), 0};
        return true;
    }
    if (id >= kMoveTouchBase && id < kMoveTouchBase + kMoveRows) {
        focus = {Shared::Panel::Moves, static_cast<uint8_t>(id - kMoveTouchBase), 0};
        return true;
    }
    return false;
}

// What an explicit activation of the focused field asks the caller to open.
enum class ActivationKind : uint8_t { DetailField, IvCell, EvCell, PidLinkedRow, MoveRow };
struct Activation {
    ActivationKind kind = ActivationKind::DetailField;
    int row = 0;
};
[[nodiscard]] constexpr Activation activationFor(Shared::Focus focus) noexcept {
    if (focus.panel == Shared::Panel::Details)
        return {ActivationKind::DetailField, focus.row};
    if (focus.panel == Shared::Panel::Moves)
        return {ActivationKind::MoveRow, focus.row % kMoveRows};
    if (focus.row < kStatRows)
        return {focus.column == 0 ? ActivationKind::IvCell : ActivationKind::EvCell, focus.row};
    return {ActivationKind::PidLinkedRow, focus.row};
}

// The contextual move editor (Move / PP / PP Ups / Apply / Cancel).
inline constexpr int kMoveEditorRows = 5;
[[nodiscard]] constexpr int moveEditorStep(int row, int direction) noexcept {
    return (row + (direction < 0 ? kMoveEditorRows - 1 : 1)) % kMoveEditorRows;
}

} // namespace PokeBank::UIModel::SharedPokemonWorkspace
