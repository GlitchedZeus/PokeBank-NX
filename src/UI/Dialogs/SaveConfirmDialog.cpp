#include "UI/Dialogs/SaveConfirmDialog.h"
#include "UI/Dialogs/DialogFrame.h"
#include "UI/Dialogs/EditControls.h"   // drawEditChoiceButton -- on-button controller glyphs
#include "UI/TrainerViewScreen.h"
#include "UI/Common.h"
#include "UI/PKSEFramebuffer.h"
#include "Globals.h"

namespace UI {
namespace Dialogs {
    namespace {
        // Leaf folder name of a backup path -- that IS the backup's name in the picker list, the
        // same way it is in the backup selection screen.
        std::string leafOf(const std::string& path) {
            const size_t slash = path.find_last_of('/');
            return (slash == std::string::npos) ? path : path.substr(slash + 1);
        }
    }

    void drawSaveConfirmDialog(TrainerViewScreen& screen, PKSEFramebuffer& fb) {
        constexpr int w = 560;

        // Exiting with unsaved changes is a different question ("throw these away?"), not a
        // destination choice, so it keeps the plain two-button form.
        if (screen.exitingWithUnsavedChanges) {
            constexpr int h = 248;
            const int x = (fb.getWidth() - w) / 2, y = (fb.getHeight() - h) / 2;
            int cy = drawDialogFrame(fb, x, y, w, h, "Unsaved Changes", Colors::Warning);
            fb.drawText(x + 24, cy,      "You have unsaved changes.", Colors::Text);
            fb.drawText(x + 24, cy + 28, "Changes will be lost if you continue.", Colors::TextDim);

            // Buttons carry their glyph (id 0 = Cancel/B, id 1 = Discard & Exit/A), no guide line.
            screen.touchButtons.clear();
            const int cbh = TouchTargetMin, cby = y + h - cbh - 16;
            const int cbw = (w - 48 - 16) / 2;
            drawEditChoiceButton(screen, fb, x + 24,           cby, cbw, cbh, "B", "Cancel",         0);
            drawEditChoiceButton(screen, fb, x + w - 24 - cbw, cby, cbw, cbh, "A", "Discard & Exit", 1);
            return;
        }

        // --- Destination picker ---------------------------------------------------------
        const int rows = screen.saveDestCount();
        constexpr int rowH = 58, rowGap = 8;
        // The notice is a LINE here, not an extra dialog. This dialog is already a confirm/cancel,
        // so folding a warning in tells the user without adding a step.
        const int warnH = (screen.illegalDataWritten ? 26 : 0);
        const int h = 168 + warnH + rows * (rowH + rowGap);
        const int x = (fb.getWidth() - w) / 2, y = (fb.getHeight() - h) / 2;

        int cy = drawDialogFrame(fb, x, y, w, h, "Save Changes", Colors::Text);
        fb.drawText(x + 24, cy,
                    screen.hasUnsavedChanges ? "Where should this be written?"
                                             : "No changes made. Write anyway?",
                    screen.hasUnsavedChanges ? Colors::Text : Colors::TextDim);
        if (screen.illegalDataWritten) {
            fb.drawText(x + 24, cy + 24,
                        "Contains values the games treat as illegal.",
                        Color(235, 120, 120), TextStyle::Caption);
            cy += 26;
        }
        // There is deliberately no "this save contains DLC content" warning. Owning a DLC gates the
        // AREAS, not the Pokemon -- the patch ships the data to every copy, so a player without the
        // Expansion Pass can hold a Crown Tundra species traded to them and it works normally.

        const std::string backupName = leafOf(screen.backupDir);
        const char* titles[3] = { "This backup", "New backup...", "Game save" };
        const std::string subs[3] = {
            backupName,
            "name it with the keyboard",
            // Same destination, very different act depending on where this session came from.
            screen.loadedFromCart ? "write back to " + screen.titleName
                                  : "REPLACES your live save with this backup",
        };

        screen.touchButtons.clear();
        const int rx = x + 20, rw = w - 40;
        int ry = cy + 34;
        for (int i = 0; i < rows; ++i) {
            // A row is not its destination: a title session shows Game save / New backup, so the
            // row index has to be mapped rather than used to index the arrays directly.
            const int d = static_cast<int>(screen.saveDestAt(i));
            const bool sel = (screen.saveDestIndex == i);
            if (sel) fb.drawSelectionHighlight(rx, ry, rw, rowH);
            else     fb.drawFilledRoundedRect(rx, ry, rw, rowH, 10, Colors::PanelAlt);

            // Red only when writing to the game would DESTROY something: a backup-sourced session
            // overwriting live progress. Saving a cart session back to its own cart is routine and
            // shouldn't be dressed up as a hazard.
            const bool danger = (d == TrainerViewScreen::DestGameSave) && !screen.loadedFromCart;
            fb.drawText(rx + 18, ry + 7, titles[d],
                        danger ? Color(235, 120, 120) : Colors::Text, TextStyle::Body);
            fb.drawText(rx + 18, ry + 32, subs[d], Colors::TextDim, TextStyle::Caption);

            screen.touchButtons.push_back({ i, rx, ry, rw, rowH });
            ry += rowH + rowGap;
        }

        drawDialogFooter(fb, x, y, w, h, "Up/Down: Choose  |  A: Save  |  B: Cancel");
    }

    void drawSaveInjectConfirm(TrainerViewScreen& screen, PKSEFramebuffer& fb) {
        constexpr int w = 620, h = 268;
        const int x = (fb.getWidth() - w) / 2, y = (fb.getHeight() - h) / 2;

        int cy = drawDialogFrame(fb, x, y, w, h, "Write to the game save?", Color(235, 120, 120));
        fb.drawText(x + 24, cy,
                    "This replaces " + screen.titleName + "'s save data", Colors::Text);
        fb.drawText(x + 24, cy + 26,
                    "with backup \"" + leafOf(screen.backupDir) + "\" plus your edits.", Colors::Text);
        // Say plainly what is at risk. The user may have loaded a backup from weeks ago, in which
        // case this rolls their game back -- and the dialog is the only place that can warn them.
        fb.drawText(x + 24, cy + 60,
                    "Any progress made since that backup will be lost.", Color(235, 120, 120));
        fb.drawText(x + 24, cy + 86,
                    "The backup itself is written either way.", Colors::TextDim, TextStyle::Caption);

        screen.touchButtons.clear();
        const int bw = (w - 60) / 2, bh = 52, by = y + h - 48 - bh - 8;
        // Glyph ON each button (B: Cancel, A: Write to game); the destructive action stays red.
        drawEditChoiceButton(screen, fb, x + 20,      by, bw, bh, "B", "Cancel",        0);
        drawEditChoiceButton(screen, fb, x + 40 + bw, by, bw, bh, "A", "Write to game", 1,
                             Color(160, 60, 60), Colors::White);
    }
}
}
