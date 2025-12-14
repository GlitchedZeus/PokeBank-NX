#include "UI/Dialogs/SaveConfirmDialog.h"
#include "UI/TrainerViewScreen.h"
#include "UI/Common.h"
#include "UI/PKSEFramebuffer.h"

namespace UI {
namespace Dialogs {
    void drawSaveConfirmDialog(TrainerViewScreen& screen, PKSEFramebuffer& fb) {
        // Draw dialog
        int dialogWidth = 500;
        int dialogHeight = 180;
        int dialogX = (fb.getWidth() - dialogWidth) / 2;
        int dialogY = (fb.getHeight() - dialogHeight) / 2;

        // Draw dialog background
        fb.drawFilledRect(dialogX, dialogY, dialogWidth, dialogHeight, Colors::Panel);
        fb.drawRect(dialogX, dialogY, dialogWidth, dialogHeight, Colors::Border);

        // Draw title and message
        if (screen.exitingWithUnsavedChanges) {
            fb.drawText(dialogX + 20, dialogY + 20, "Unsaved Changes", Colors::Yellow);
            fb.drawFilledRect(dialogX + 20, dialogY + 45, dialogWidth - 40, 2, Colors::Border);
            fb.drawText(dialogX + 20, dialogY + 70, "You have unsaved changes.", Colors::Text);
            fb.drawText(dialogX + 20, dialogY + 95, "Changes will be lost if you continue.", Colors::Text);
            fb.drawText(dialogX + 20, dialogY + 135, "A: Discard and Exit  |  B: Cancel", Colors::TextDim);
        } else {
            // Regular save dialog (triggered by X button)
            fb.drawText(dialogX + 20, dialogY + 20, "Save Changes", Colors::Text);
            fb.drawFilledRect(dialogX + 20, dialogY + 45, dialogWidth - 40, 2, Colors::Border);
            if (screen.hasUnsavedChanges) {
                fb.drawText(dialogX + 20, dialogY + 70, "You have unsaved changes.", Colors::Yellow);
                fb.drawText(dialogX + 20, dialogY + 95, "Do you want to save them?", Colors::Text);
            } else {
                fb.drawText(dialogX + 20, dialogY + 70, "No changes have been made.", Colors::TextDim);
                fb.drawText(dialogX + 20, dialogY + 95, "Save anyway?", Colors::Text);
            }
            fb.drawText(dialogX + 20, dialogY + 135, "A: Save  |  B: Cancel", Colors::TextDim);
        }
    }
}
}