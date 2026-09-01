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
        const char* titles[2] = { "Working backup", "New backup..." };
        const std::string subs[2] = {
            backupName,
            "name it with the keyboard",
        };

        screen.touchButtons.clear();
        const int rx = x + 20, rw = w - 40;
        int ry = cy + 34;
        for (int i = 0; i < rows; ++i) {
            const int d = static_cast<int>(screen.saveDestAt(i));
            const bool sel = (screen.saveDestIndex == i);
            if (sel) fb.drawSelectionHighlight(rx, ry, rw, rowH);
            else     fb.drawFilledRoundedRect(rx, ry, rw, rowH, 10, Colors::PanelAlt);

            fb.drawText(rx + 18, ry + 7, titles[d], Colors::Text, TextStyle::Body);
            fb.drawText(rx + 18, ry + 32, subs[d], Colors::TextDim, TextStyle::Caption);

            screen.touchButtons.push_back({ i, rx, ry, rw, rowH });
            ry += rowH + rowGap;
        }

        drawDialogFooter(fb, x, y, w, h, "Up/Down: Choose  |  A: Save  |  B: Cancel");
    }

}
}
