#include "UI/Dialogs/EditDialog.h"
#include "UI/TrainerViewScreen.h"
#include "UI/Common.h"
#include "UI/PKSEFramebuffer.h"
#include "Trainer/Trainer.h"
#include "Utils/Utilities.h"

namespace Dialogs {

void drawEditDialog(TrainerViewScreen& screen, PKSEFramebuffer& fb) {
    // Draw semi-transparent overlay (simulate with dark panel)
    int dialogWidth = 500;
    int dialogHeight = 200;
    int dialogX = (fb.getWidth() - dialogWidth) / 2;
    int dialogY = (fb.getHeight() - dialogHeight) / 2;

    // Draw dialog background
    fb.drawFilledRect(dialogX, dialogY, dialogWidth, dialogHeight, Colors::Panel);
    fb.drawRect(dialogX, dialogY, dialogWidth, dialogHeight, Colors::Border);

    // Draw title
    fb.drawText(dialogX + 20, dialogY + 20, "Edit Item Amount", Colors::Text);
    fb.drawFilledRect(dialogX + 20, dialogY + 45, dialogWidth - 40, 2, Colors::Border);

    // Get item name
    std::string itemName = "Unknown Item";
    if (screen.selectedCategory >= 0 && screen.selectedCategory < static_cast<int>(screen.trainer.items.size())) {
        const auto& pouch = screen.trainer.items[screen.selectedCategory];
        if (screen.selectedItemIndex >= 0 && screen.selectedItemIndex < static_cast<int>(pouch.size())) {
            itemName = getItemName(pouch[screen.selectedItemIndex].itemId);
        }
    }

    // Draw item name
    fb.drawText(dialogX + 20, dialogY + 65, "Item: " + itemName, Colors::Text);

    // Draw current value (large and centered)
    char valueText[32];
    snprintf(valueText, sizeof(valueText), "Amount: %d", screen.editDialogValue);
    fb.drawText(dialogX + 150, dialogY + 95, valueText, Colors::Yellow);

    // Draw instructions
    fb.drawText(dialogX + 20, dialogY + 125, "Left/Right: +/-1  |  Up/Down: +/-10", Colors::TextDim);
    fb.drawText(dialogX + 20, dialogY + 145, "ZL/ZR: +/-100", Colors::TextDim);
    fb.drawText(dialogX + 20, dialogY + 165, "A: Confirm  |  B: Cancel", Colors::TextDim);
}

}
