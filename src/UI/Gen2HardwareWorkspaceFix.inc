// Hardware-retest presentation/input adapter for the Generation II shared workspace.
// Keep the proven shared editor implementation underneath; normalize only the
// hardware-visible focus/presentation regressions found on dc6bf22b.

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

} // namespace

bool handleUnifiedGen2SurfaceInput(TrainerViewScreen& screen, uint64_t down, uint64_t held,
                                   int stickX, int stickY, const TouchInput& touch) {
    if (!isGSC(screen)) return false;
    if (stateFor(screen).mode != Mode::None) {
        normalizeHardwareMoveRowFocus(screen);
        normalizeHardwareDerivedHpDvFocus(screen);
        const auto previous = unifiedStateFor(screen).focus;
        const bool handled = handleUnifiedWorkspaceInput(screen, down, held, stickX, stickY, touch);
        normalizeHardwareMoveRowFocus(screen);
        normalizeHardwareDerivedHpDvFocus(screen, &previous);
        return handled;
    }
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
