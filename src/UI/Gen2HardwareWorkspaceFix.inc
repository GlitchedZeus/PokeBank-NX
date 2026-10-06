// Hardware-retest adapter for the Generation II shared workspace.
// Keep the proven shared editor implementation underneath; normalize only the
// hardware-visible focus/presentation regressions found on dc6bf22b.

#include "UI/SharedPokemonShell.h"

namespace UI::Gen2PokemonEditor {
namespace {

void normalizeHardwareMoveRowFocus(TrainerViewScreen& screen) {
    auto& unified = unifiedStateFor(screen);
    unified.focus = Unified::normalizeMoveRowFocus(Unified::Generation::Gen2, unified.focus, unified.crystal);
}

void normalizeHardwareDerivedHpDvFocus(TrainerViewScreen& screen,
                                       const Unified::Focus* previous = nullptr) {
    auto& unified = unifiedStateFor(screen);
    auto& focus = unified.focus;
    if (focus.panel != Unified::Panel::Values || focus.row != 0 || focus.column != 0) return;

    // Preserve vertical DV-column navigation while skipping the derived HP DV.
    if (previous && previous->panel == Unified::Panel::Values && previous->column == 0) {
        if (previous->row == 1) {
            focus.row = 6;
            return;
        }
        if (previous->row == 6) {
            focus.row = 1;
            return;
        }
    }

    // HP DV is derived/non-focusable. Moving left from HP Stat Exp must stay
    // inside STATS and land on the next real editable DV: Attack DV.
    if (previous && previous->panel == Unified::Panel::Values &&
        previous->row == 0 && previous->column == 1) {
        focus.panel = Unified::Panel::Values;
        focus.row = 1;
        focus.column = 0;
        return;
    }

    // Entering STATS on the HP row defaults to the first real editable value.
    focus.column = 1;
}

void clearHardwarePanelHeading(PKSEFramebuffer& fb, int x, int y, int width,
                               const char* text, TextStyle style) {
    fb.drawFilledRect(x + 10, y + 6, width, 34, Colors::Panel);
    fb.drawText(x + 14, y + 10, text, Colors::Text, style);
}

void drawHardwareWorkspaceCorrections(TrainerViewScreen& screen, PKSEFramebuffer& fb) {
    const auto& state = stateFor(screen);
    const auto g = Unified::geometry720p();
    const int contentY = g.y + 74;
    const int contentH = g.height - 94;
    const int leftX = g.x + 18, leftW = 300;
    const int midX = leftX + leftW + 14, midW = 398;
    const int rightX = midX + midW + 14;

    // Gen I is the accepted focus model: headings remain neutral and the compact
    // outline on the actual field/cell communicates focus.
    clearHardwarePanelHeading(fb, leftX, contentY, 120, "DETAILS", TextStyle::Caption);
    clearHardwarePanelHeading(fb, midX, contentY, 120, "STATS", TextStyle::Heading);
    clearHardwarePanelHeading(fb, rightX, contentY, 120, "MOVES", TextStyle::Heading);

    // Keep DETAILS scrolling behavior available for future/Crystal rows but remove
    // the stray bottom glyph until the product intentionally exposes a scroll cue.
    fb.drawFilledRect(leftX + leftW - 50, contentY + contentH - 34, 38, 26, Colors::Panel);

    // Empty move rows have no meaningful PP or PP-Up value. Preserve the Empty row
    // and compatibility status, but remove numeric noise and any stale aux-cell outline.
    for (int slot = 0; slot < 4; ++slot) {
        if (state.working.moves[static_cast<std::size_t>(slot)] != 0) continue;
        const int yy = contentY + 62 + slot * 32;
        fb.drawFilledRect(rightX + 184, yy - 6, 104, 30, Colors::Panel);
    }
}

bool pointInRect(int px, int py, int x, int y, int w, int h) noexcept {
    return px >= x && px < x + w && py >= y && py < y + h;
}

bool focusUnifiedTouchPoint(TrainerViewScreen& screen, int px, int py, bool activate,
                            uint64_t& down) {
    auto& state = stateFor(screen);
    auto& unified = unifiedStateFor(screen);

    // Child dialogs own the whole gesture while visible. Their rows map directly to the
    // already-proven controller action path; release activation remains the only mutation point.
    if (unified.moveEditor) {
        constexpr int w = 650, h = 390, navH = 48;
        const int x = (1280 - w) / 2;
        const int y = (720 - navH - h) / 2;
        for (int row = 0; row < 5; ++row) {
            const int rowY = y + 88 + row * 48;
            if (!pointInRect(px, py, x + 22, rowY, w - 44, 42)) continue;
            unified.moveEditorRow = row;
            if (activate) down |= HidNpadButton_A;
            return true;
        }
        return true;
    }
    if (unified.progressionEditor) {
        constexpr int x = 320, y = 168, w = 640;
        for (int row = 0; row < 4; ++row) {
            const int rowY = y + 96 + row * 48;
            if (!pointInRect(px, py, x + 22, rowY - 5, w - 44, 40)) continue;
            unified.progressionRow = row;
            if (activate) down |= HidNpadButton_A;
            return true;
        }
        return true;
    }
    if (state.itemPicker || state.confirmExit || pickerStateFor(screen).model.active()) return false;

    // The final hardware surface is the SharedPokemonShell fullscreen layout, not the older
    // centered workspace geometry. Hit-test exactly that 1280x720 Switch surface.
    const SharedPokemonShell::Geometry geometry(1280, 720, kNavBarH);
    const int contentY = geometry.y;
    const int leftX = geometry.leftX, leftW = geometry.leftW;
    const int midX = geometry.midX, midW = geometry.midW;
    const int rightX = geometry.rightX, rightW = geometry.rightW;

    const std::size_t detailCount = unified.crystal ? 13u : 9u;
    const auto window = Unified::scrollWindow(detailCount, 8, unified.detailsFocus);
    for (std::size_t i = 0; i < window.count; ++i) {
        const int yy = contentY + 224 + static_cast<int>(i) * 38;
        if (!pointInRect(px, py, leftX + 8, yy + 1, leftW - 36, 34)) continue;
        const auto row = static_cast<uint8_t>(window.first + i);
        unified.focus = {Unified::Panel::Details, row, 0};
        unified.detailsFocus = Unified::detailsScrollFocus(unified.focus, unified.detailsFocus);
        if (activate && state.mode != Mode::View) down |= HidNpadButton_A;
        return true;
    }

    for (int row = 0; row < 5; ++row) {
        const int yy = contentY + 70 + row * 34;
        for (int col = 0; col < 2; ++col) {
            if (row == 0 && col == 0) continue; // HP DV is derived/read-only.
            Unified::Focus candidate{Unified::Panel::Values,
                                     static_cast<uint8_t>(row),
                                     static_cast<uint8_t>(col)};
            const auto cell = Unified::cellFocus(candidate);
            if (!pointInRect(px, py, midX + cell.x, yy - 5, cell.width, 28)) continue;
            unified.focus = candidate;
            if (activate && state.mode != Mode::View) down |= HidNpadButton_A;
            return true;
        }
    }

    const std::array<int, 2> capabilityY{contentY + 366, contentY + 408};
    for (int i = 0; i < 2; ++i) {
        if (!pointInRect(px, py, midX + 8, capabilityY[static_cast<std::size_t>(i)] - 5,
                         midW - 16, 30)) continue;
        unified.focus = {Unified::Panel::Values, static_cast<uint8_t>(5 + i), 0};
        if (activate && state.mode != Mode::View) down |= HidNpadButton_A;
        return true;
    }

    const auto moveCell = Unified::moveRowFocus(rightW);
    for (int slot = 0; slot < 4; ++slot) {
        const int yy = contentY + 62 + slot * 32;
        if (!pointInRect(px, py, rightX + moveCell.x, yy - 5, moveCell.width, 28)) continue;
        unified.focus = {Unified::Panel::Moves, static_cast<uint8_t>(slot), 0};
        if (activate && state.mode != Mode::View) down |= HidNpadButton_A;
        return true;
    }
    return false;
}

void handleUnifiedTouch(TrainerViewScreen& screen, const TouchInput& touch, uint64_t& down) {
    auto& state = stateFor(screen);
    auto& unified = unifiedStateFor(screen);
    if (state.mode == Mode::None) return;
    if (state.itemPicker || state.confirmExit || pickerStateFor(screen).model.active()) return;

    if (touch.justTouchedDown())
        focusUnifiedTouchPoint(screen, touch.x(), touch.y(), false, down);

    // Once TouchInput classifies a gesture as a drag it can never become a tap again. Vertical
    // drags beginning in DETAILS therefore browse/scroll without activating the row under release.
    if (touch.justReleased() && touch.dragged() && !unified.moveEditor && !unified.progressionEditor) {
        const SharedPokemonShell::Geometry geometry(1280, 720, kNavBarH);
        if (pointInRect(touch.startX(), touch.startY(), geometry.leftX, geometry.y,
                        geometry.leftW, geometry.h)) {
            const int detailCount = unified.crystal ? 13 : 9;
            const int steps = std::max(1, std::abs(touch.deltaY()) / 38);
            const int direction = touch.deltaY() < 0 ? 1 : -1;
            const int next = std::clamp(static_cast<int>(unified.focus.row) + direction * steps,
                                        0, detailCount - 1);
            unified.focus = {Unified::Panel::Details, static_cast<uint8_t>(next), 0};
            unified.detailsFocus = Unified::detailsScrollFocus(unified.focus, unified.detailsFocus);
        }
        return;
    }

    if (touch.justTapped())
        focusUnifiedTouchPoint(screen, touch.startX(), touch.startY(), true, down);
}

void handleParityTouch(TrainerViewScreen& screen, const TouchInput& touch, uint64_t& down) {
    auto& shared = sharedSurfaceStateFor(screen);
    auto& legacy = editorOverlayState(screen);

    // Clone has one truthful writable target: the highlighted first empty slot in the selected
    // destination box. Tapping any other cell is inert; the existing A backend remains authoritative.
    if (shared.cloneActive) {
        auto* editor = stagedEditor(screen);
        if (!editor) return;
        constexpr int w = 930, h = 540;
        constexpr int x = (1280 - w) / 2;
        constexpr int y = (672 - h) / 2;
        constexpr int cols = 5, cellW = 164, cellH = 72;
        constexpr int startX = x + 48, startY = y + 96;
        if (touch.justTapped()) {
            const int destination = firstEmptyGen2Slot(*editor, shared.cloneBox);
            if (destination >= 0) {
                const int col = destination % cols, row = destination / cols;
                if (pointInRect(touch.startX(), touch.startY(), startX + col * cellW,
                                startY + row * cellH, cellW - 10, cellH - 10))
                    down |= HidNpadButton_A;
            }
        }
        if (touch.justReleased() && touch.dragged() &&
            pointInRect(touch.startX(), touch.startY(), x, y, w, h) &&
            std::abs(touch.deltaX()) > std::abs(touch.deltaY()) &&
            std::abs(touch.deltaX()) >= 56) {
            down |= touch.deltaX() < 0 ? HidNpadButton_R : HidNpadButton_L;
        }
        return;
    }

    // Review rows are informational. Touch changes/browses the highlighted row only; it never
    // aliases A because A exports the complete staged copy rather than acting on one row.
    if (legacy.active && legacy.review) {
        auto* editor = stagedEditor(screen);
        if (!editor) return;
        const int changeCount = static_cast<int>(editor->pendingChanges().size());
        if (changeCount <= 0) return;
        constexpr int w = 980, h = 540;
        constexpr int x = (1280 - w) / 2;
        constexpr int y = (672 - h) / 2;
        const int first = std::clamp(legacy.reviewRow - 4, 0, std::max(0, changeCount - 9));
        const int last = std::min(changeCount, first + 9);
        if (touch.justTouchedDown() || touch.justTapped()) {
            const int px = touch.justTouchedDown() ? touch.x() : touch.startX();
            const int py = touch.justTouchedDown() ? touch.y() : touch.startY();
            for (int i = first; i < last; ++i) {
                const int rowY = y + 88 + (i - first) * 45;
                if (!pointInRect(px, py, x + 22, rowY, w - 44, 42)) continue;
                legacy.reviewRow = i;
                break;
            }
        }
        if (touch.justReleased() && touch.dragged() &&
            pointInRect(touch.startX(), touch.startY(), x, y, w, h)) {
            const int steps = std::max(1, std::abs(touch.deltaY()) / 45);
            legacy.reviewRow = std::clamp(
                legacy.reviewRow + (touch.deltaY() < 0 ? steps : -steps), 0, changeCount - 1);
        }
    }
}

} // namespace

bool handleUnifiedGen2SurfaceInput(TrainerViewScreen& screen, uint64_t down, uint64_t held,
                                   int stickX, int stickY, const TouchInput& touch) {
    if (!isGSC(screen)) return false;
    if (stateFor(screen).mode != Mode::None) {
        handleUnifiedTouch(screen, touch, down);
        normalizeHardwareMoveRowFocus(screen);
        normalizeHardwareDerivedHpDvFocus(screen);
        const auto previous = unifiedStateFor(screen).focus;
        const bool handled = handleUnifiedWorkspaceInput(screen, down, held, stickX, stickY, touch);
        normalizeHardwareMoveRowFocus(screen);
        normalizeHardwareDerivedHpDvFocus(screen, &previous);
        return handled;
    }
    handleParityTouch(screen, touch, down);
    return handleUnifiedGen2SurfaceInputBase(screen, down, held, stickX, stickY, touch);
}

bool drawUnifiedGen2Surface(TrainerViewScreen& screen, PKSEFramebuffer& fb) {
    if (!isGSC(screen)) return false;
    if (stateFor(screen).mode != Mode::None) {
        normalizeHardwareDerivedHpDvFocus(screen);
        drawUnifiedWorkspace(screen, fb);
        drawHardwareWorkspaceCorrections(screen, fb);
        return true;
    }
    return drawUnifiedGen2SurfaceBase(screen, fb);
}

} // namespace UI::Gen2PokemonEditor
