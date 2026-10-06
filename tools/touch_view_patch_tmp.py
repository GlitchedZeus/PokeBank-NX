from pathlib import Path
import re


def once(path, old, new):
    p = Path(path)
    s = p.read_text()
    n = s.count(old)
    if n != 1:
        raise SystemExit(f"{path}: expected 1 anchor, got {n}: {old[:120]!r}")
    p.write_text(s.replace(old, new, 1))

# ---------------------------------------------------------------------------
# Shared details state: allow finger-owned scrolling and independent reports.
# ---------------------------------------------------------------------------
once('include/UI/TrainerViewScreenBase.h',
'''            int  leftScroll = 0;                         // left-pane vertical scroll (px); auto-follows selection, reset on (re)open
            std::vector<int> leftOrder;                  // left-pane editable field ids in DRAW order (rebuilt each draw)
''',
'''            int  leftScroll = 0;                         // left/info pane vertical scroll (px)
            bool leftScrollManual = false;               // finger drag owns scroll until direct/controller selection resumes
            int legalityScroll = 0;                      // first visible legality-report row
            int ribbonScroll = 0;                        // first visible ribbon/mark row
            std::vector<int> leftOrder;                  // left-pane editable field ids in DRAW order (rebuilt each draw)
''')

# ---------------------------------------------------------------------------
# Save destination dialog: real tappable Cancel/Save controls, not text only.
# ---------------------------------------------------------------------------
once('src/UI/Dialogs/SaveConfirmDialog.cpp',
'''        const int h = 168 + warnH + rows * (rowH + rowGap);
''',
'''        const int h = 226 + warnH + rows * (rowH + rowGap);
''')
once('src/UI/Dialogs/SaveConfirmDialog.cpp',
'''        drawDialogFooter(fb, x, y, w, h, "D-pad/Stick: Choose  |  A: Save  |  B: Cancel");
''',
'''        const int cbh = TouchTargetMin;
        const int cby = y + h - cbh - 16;
        const int cbw = (w - 48 - 16) / 2;
        drawEditChoiceButton(screen, fb, x + 24, cby, cbw, cbh, "B", "Cancel", 90);
        drawEditChoiceButton(screen, fb, x + w - 24 - cbw, cby, cbw, cbh, "A", "Save", 91);
''')

# ---------------------------------------------------------------------------
# Details input: report scrolling, native pane drags, safe save buttons.
# ---------------------------------------------------------------------------
p = Path('src/UI/TrainerViewScreenBase.inc')
s = p.read_text()
old = '''                const int td = touchedButtonId(touch);
                if (td >= 0 && td < nDest) { saveDestIndex = td; kDown |= HidNpadButton_A; }
'''
new = '''                const int td = touchedButtonId(touch);
                if (td == 90) kDown |= HidNpadButton_B;
                else if (td == 91) kDown |= HidNpadButton_A;
                else if (td >= 0 && td < nDest) { saveDestIndex = td; kDown |= HidNpadButton_A; }
'''
if s.count(old) != 1: raise SystemExit('save destination touch anchor moved')
s = s.replace(old, new, 1)

old = '''            // Legality overlay (full issue list) intercepts input while open: B or any tap closes it.
            if (details.legalityOverlay) {
                if ((kDown & HidNpadButton_B) || tb >= 0) details.legalityOverlay = false;
                return;
            }
            // Ribbon overlay (full ribbon/mark list) does the same.
            if (details.ribbonOverlay) {
                if ((kDown & HidNpadButton_B) || tb >= 0) details.ribbonOverlay = false;
                return;
            }
            // Open the ribbon list: Y, or tapping the Ribbons row (id 94).
            if ((kDown & HidNpadButton_Y) || tb == 94) {
                details.ribbonOverlay = true;
                return;
            }
            // Open the legality issue list: R, or tapping the legality summary (id 95) -- but ONLY
            // when the mon actually has issues. A clean mon has nothing to show, so the button is
            // disabled (greyed in the guide) and this does nothing.
            if ((kDown & HidNpadButton_R) || tb == 95) {
                if (pokemon && !Legality::analyze(*pokemon, pokemon->getGameGroup()).ok())
                    details.legalityOverlay = true;
                return;
            }
'''
new = '''            // Report overlays are real scroll surfaces. A drag scrolls; only B or the explicit
            // Close target exits, so reading a long report cannot accidentally dismiss it.
            if (details.legalityOverlay) {
                if ((kDown & HidNpadButton_B) || tb == 96) {
                    details.legalityOverlay = false; details.legalityScroll = 0; return;
                }
                if (kDown & HidNpadButton_Up)   details.legalityScroll = std::max(0, details.legalityScroll - 1);
                if (kDown & HidNpadButton_Down) ++details.legalityScroll;
                if (kDown & HidNpadButton_L)    details.legalityScroll = std::max(0, details.legalityScroll - 8);
                if (kDown & HidNpadButton_R)    details.legalityScroll += 8;
                if (touch.justReleased() && touch.dragged()) {
                    const int dy = touch.deltaY();
                    const int steps = std::max(1, std::abs(dy) / 30);
                    if (dy < 0) details.legalityScroll += steps;
                    else if (dy > 0) details.legalityScroll = std::max(0, details.legalityScroll - steps);
                }
                return;
            }
            if (details.ribbonOverlay) {
                if ((kDown & HidNpadButton_B) || tb == 96) {
                    details.ribbonOverlay = false; details.ribbonScroll = 0; return;
                }
                if (kDown & HidNpadButton_Up)   details.ribbonScroll = std::max(0, details.ribbonScroll - 1);
                if (kDown & HidNpadButton_Down) ++details.ribbonScroll;
                if (kDown & HidNpadButton_L)    details.ribbonScroll = std::max(0, details.ribbonScroll - 8);
                if (kDown & HidNpadButton_R)    details.ribbonScroll += 8;
                if (touch.justReleased() && touch.dragged()) {
                    const int dy = touch.deltaY();
                    const int steps = std::max(1, std::abs(dy) / 28);
                    if (dy < 0) details.ribbonScroll += steps;
                    else if (dy > 0) details.ribbonScroll = std::max(0, details.ribbonScroll - steps);
                }
                return;
            }
            // Open reports at the top every time.
            if ((kDown & HidNpadButton_Y) || tb == 94) {
                details.ribbonScroll = 0;
                details.ribbonOverlay = true;
                return;
            }
            if ((kDown & HidNpadButton_R) || tb == 95) {
                if (pokemon && !Legality::analyze(*pokemon, pokemon->getGameGroup()).ok()) {
                    details.legalityScroll = 0;
                    details.legalityOverlay = true;
                }
                return;
            }

            // Finger-owned scrolling for content panes. Gen II's native-data card is on the right;
            // the modern details column is on the left. A drag returns immediately and therefore
            // never activates whatever row happens to be under the release point.
            const bool gen2Passive = sourceGameId == "gold_gbc" || sourceGameId == "silver_gbc" ||
                                     sourceGameId == "crystal_gbc";
            if (touch.justReleased() && touch.dragged()) {
                const bool inGen2Data = gen2Passive && touch.startX() >= 796 && touch.startX() < 1016 &&
                                        touch.startY() >= 393 && touch.startY() < 609;
                const bool inModernInfo = !gen2Passive && touch.startX() >= 24 && touch.startX() < 454 &&
                                          touch.startY() >= 276 && touch.startY() < 650;
                if (inGen2Data || inModernInfo) {
                    details.leftScroll = std::max(0, details.leftScroll - touch.deltaY());
                    details.leftScrollManual = true;
                    return;
                }
            }
'''
if s.count(old) != 1: raise SystemExit('details report block moved')
s = s.replace(old, new, 1)

old = '''            if (tb >= 0 && tb <= 31) { details.selectedField = tb; kDown |= HidNpadButton_A; }
'''
new = '''            if (tb >= 0 && tb <= 31) {
                details.leftScrollManual = false;
                details.selectedField = tb;
                kDown |= HidNpadButton_A;
            }
'''
if s.count(old) != 1: raise SystemExit('details direct-row touch anchor moved')
s = s.replace(old, new, 1)

old = '''            {
                int& f = details.selectedField;
'''
new = '''            {
                if (kDown & (HidNpadButton_Up | HidNpadButton_Down | HidNpadButton_Left | HidNpadButton_Right))
                    details.leftScrollManual = false;
                int& f = details.selectedField;
'''
if s.count(old) != 1: raise SystemExit('details navigation anchor moved')
s = s.replace(old, new, 1)

# Reset manual/report ownership whenever the existing code resets the information scroll.
s = s.replace('details.leftScroll = 0;', 'details.leftScroll = 0; details.leftScrollManual = false;')
p.write_text(s)

# ---------------------------------------------------------------------------
# Modern View rendering: manual scroll + clipped, scrollable reports.
# ---------------------------------------------------------------------------
p = Path('src/UI/Modals/PokemonDetailsModal.cpp')
s = p.read_text()
old = '''            if (selRowY >= 0) {
                if (selRowY - s < contentTop)                s = selRowY - contentTop;
                else if ((selRowY + RH) - s > contentBottom) s = (selRowY + RH) - contentBottom;
            }
'''
new = '''            if (!screen.details.leftScrollManual && selRowY >= 0) {
                if (selRowY - s < contentTop)                s = selRowY - contentTop;
                else if ((selRowY + RH) - s > contentBottom) s = (selRowY + RH) - contentBottom;
            }
'''
if s.count(old) != 1: raise SystemExit('modern auto-follow anchor moved')
s = s.replace(old, new, 1)

start = s.index('        // Legality issue overlay —')
end = s.index('    }\n}\n}', start)
replacement = r'''        // Legality report: clipped touch-scroll surface with explicit close.
        if (screen.details.legalityOverlay) {
            struct ReportLine { std::string text; Color color; };
            std::vector<ReportLine> lines;
            if (legalityRep.ok()) {
                lines.push_back({"No problems found.", Color(120, 205, 140)});
            } else {
                for (const auto& issue : legalityRep.issues) {
                    if (issue.severity == Legality::Severity::Info) continue;
                    const bool invalid = issue.severity == Legality::Severity::Invalid;
                    lines.push_back({std::string(invalid ? "[illegal]  " : "[warning]  ") + issue.text,
                                     invalid ? Color(235, 100, 100) : Colors::Orange});
                }
            }
            const int ow = 820, oh = H - 100;
            const int ox = (W - ow) / 2, oy = 50;
            const int rowH = 30, listTop = oy + 78, listBottom = oy + oh - 72;
            const int visible = std::max(1, (listBottom - listTop) / rowH);
            const int maxScroll = std::max(0, static_cast<int>(lines.size()) - visible);
            screen.details.legalityScroll = std::clamp(screen.details.legalityScroll, 0, maxScroll);

            fb.drawFilledRect(0, 0, W, H, Color(0, 0, 0, 170));
            fb.drawFilledRoundedRect(ox, oy, ow, oh, 16, Colors::Panel);
            fb.drawRoundedRect(ox, oy, ow, oh, 16, Colors::Border, 1);
            fb.drawText(ox + 24, oy + 20, "Legality", Colors::Text, TextStyle::Heading);
            fb.drawText(ox + 24, oy + 50, "Swipe or use Up/Down to scroll", Colors::TextDim, TextStyle::Caption);
            fb.setClipRect(ox + 20, listTop, ow - 40, listBottom - listTop);
            int ly = listTop;
            for (int i = screen.details.legalityScroll;
                 i < static_cast<int>(lines.size()) && i < screen.details.legalityScroll + visible; ++i) {
                fb.drawText(ox + 28, ly, lines[static_cast<size_t>(i)].text,
                            lines[static_cast<size_t>(i)].color, TextStyle::Caption);
                ly += rowH;
            }
            fb.clearClip();
            drawScrollbar(fb, ox + ow - 12, listTop, listBottom - listTop,
                          std::max(1, static_cast<int>(lines.size()) * rowH),
                          screen.details.legalityScroll * rowH);
            const int closeW = 150, closeH = 44;
            const int closeX = ox + ow - closeW - 22, closeY = oy + oh - closeH - 14;
            drawGlyphButton(fb, closeX, closeY, closeW, closeH, "B", "Close");
            screen.touchButtons.push_back({96, closeX, closeY, closeW, closeH});
        }

        // Ribbon/mark report: the same native scroll contract in two compact columns.
        if (screen.details.ribbonOverlay) {
            const auto rb = Names::getMonRibbons(reinterpret_cast<const uint8_t*>(p->getData().data()),
                                                 p->getGameGroup());
            const int ow = 860, oh = H - 100;
            const int ox = (W - ow) / 2, oy = 50;
            const int rowH = 30, cols = 2, listTop = oy + 78, listBottom = oy + oh - 72;
            const int visibleRows = std::max(1, (listBottom - listTop) / rowH);
            const int totalRows = (static_cast<int>(rb.size()) + cols - 1) / cols;
            const int maxScroll = std::max(0, totalRows - visibleRows);
            screen.details.ribbonScroll = std::clamp(screen.details.ribbonScroll, 0, maxScroll);

            fb.drawFilledRect(0, 0, W, H, Color(0, 0, 0, 170));
            fb.drawFilledRoundedRect(ox, oy, ow, oh, 16, Colors::Panel);
            fb.drawRoundedRect(ox, oy, ow, oh, 16, Colors::Border, 1);
            fb.drawText(ox + 24, oy + 20, "Ribbons & Marks", Colors::Text, TextStyle::Heading);
            fb.drawText(ox + 24, oy + 50, "Swipe or use Up/Down to scroll", Colors::TextDim, TextStyle::Caption);
            const int colW = (ow - 64) / cols;
            fb.setClipRect(ox + 20, listTop, ow - 40, listBottom - listTop);
            for (int r = 0; r < visibleRows; ++r) {
                const int sourceRow = screen.details.ribbonScroll + r;
                for (int c = 0; c < cols; ++c) {
                    const int idx = sourceRow * cols + c;
                    if (idx >= static_cast<int>(rb.size())) continue;
                    fb.drawText(ox + 28 + c * colW, listTop + r * rowH,
                                rb[static_cast<size_t>(idx)], Colors::Text, TextStyle::Caption);
                }
            }
            fb.clearClip();
            drawScrollbar(fb, ox + ow - 12, listTop, listBottom - listTop,
                          std::max(1, totalRows * rowH), screen.details.ribbonScroll * rowH);
            const int closeW = 150, closeH = 44;
            const int closeX = ox + ow - closeW - 22, closeY = oy + oh - closeH - 14;
            drawGlyphButton(fb, closeX, closeY, closeW, closeH, "B", "Close");
            screen.touchButtons.push_back({96, closeX, closeY, closeW, closeH});
        }
'''
s = s[:start] + replacement + s[end:]
p.write_text(s)

# ---------------------------------------------------------------------------
# Gen II passive native-data pane: clip, drag scroll, scrollbar, no frame reset.
# ---------------------------------------------------------------------------
p = Path('src/UI/Modals/Gen2PokemonDetailsModal.cpp')
s = p.read_text()
old = '''    fb.drawText(leftPaneX + 10, splitY + 8, "GEN II DATA", Colors::Accent, TextStyle::Caption);
    int dataY = splitY + 29;
    auto nativeRow = [&](const std::string& label, const std::string& value) {
        compactRow(fb, leftPaneX + 10, dataY, label, value, 80);
        dataY += 16;
    };
'''
new = '''    fb.drawText(leftPaneX + 10, splitY + 8, "GEN II DATA", Colors::Accent, TextStyle::Caption);
    const int nativeTop = splitY + 28;
    const int nativeBottom = splitY + splitH - 8;
    const int nativeScroll = std::max(0, screen.details.leftScroll);
    fb.setClipRect(leftPaneX + 1, nativeTop, leftPaneW - 2, nativeBottom - nativeTop);
    int dataY = splitY + 29 - nativeScroll;
    auto nativeRow = [&](const std::string& label, const std::string& value) {
        compactRow(fb, leftPaneX + 10, dataY, label, value, 80);
        dataY += 16;
    };
'''
if s.count(old) != 1: raise SystemExit('Gen II native-data start anchor moved')
s = s.replace(old, new, 1)
old = '''    fb.drawText(rightPaneX + 10, splitY + 8, "BATTLE STATS", Colors::Accent, TextStyle::Caption);
    StatsRadar::drawGen2Labeled(fb, rightPaneX + 6, splitY + 30, rightPaneW - 12, splitH - 38, battleStats);

    screen.details.leftOrder.clear();
    screen.details.leftScroll = 0;
    drawNavBar(fb, {{"B", "Back"}});
'''
new = '''    const int nativeContentH = std::max(1, dataY + nativeScroll - (splitY + 29));
    const int nativeViewH = nativeBottom - nativeTop;
    const int nativeMax = std::max(0, nativeContentH - nativeViewH);
    screen.details.leftScroll = std::clamp(screen.details.leftScroll, 0, nativeMax);
    fb.clearClip();
    drawScrollbar(fb, leftPaneX + leftPaneW - 6, nativeTop, nativeViewH,
                  nativeContentH, screen.details.leftScroll);

    fb.drawText(rightPaneX + 10, splitY + 8, "BATTLE STATS", Colors::Accent, TextStyle::Caption);
    StatsRadar::drawGen2Labeled(fb, rightPaneX + 6, splitY + 30, rightPaneW - 12, splitH - 38, battleStats);

    screen.details.leftOrder.clear();
    drawNavBar(fb, {{"B", "Back"}});
'''
if s.count(old) != 1: raise SystemExit('Gen II native-data end anchor moved')
s = s.replace(old, new, 1)
p.write_text(s)

print('View/save professional touch patch staged')
