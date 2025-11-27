#ifndef STAT_EDIT_DIALOG_H
#define STAT_EDIT_DIALOG_H

// Forward declarations
class PKSEFramebuffer;
class TrainerViewScreen;

namespace Dialogs {
    // Stat edit mode
    enum class StatEditMode {
        IV,  // Individual Values (0-31)
        EV   // Effort Values (0-252, total max 510)
    };

    void drawStatEditDialog(TrainerViewScreen& screen, PKSEFramebuffer& fb);
}

#endif // STAT_EDIT_DIALOG_H
