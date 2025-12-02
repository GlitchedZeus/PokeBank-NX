#ifndef UI_DIALOGS_STAT_EDIT_DIALOG_H
#define UI_DIALOGS_STAT_EDIT_DIALOG_H

// Forward declarations
namespace UI {
    class PKSEFramebuffer;
    class TrainerViewScreen;
}

namespace UI {
namespace Dialogs {
    // Stat edit mode
    enum class StatEditMode {
        IV,  // Individual Values (0-31)
        EV   // Effort Values (0-252, total max 510)
    };

    void drawStatEditDialog(UI::TrainerViewScreen& screen, UI::PKSEFramebuffer& fb);
}
}

#endif
