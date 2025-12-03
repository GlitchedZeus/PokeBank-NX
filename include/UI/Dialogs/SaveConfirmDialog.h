#ifndef UI_DIALOGS_SAVE_CONFIRM_DIALOG_H
#define UI_DIALOGS_SAVE_CONFIRM_DIALOG_H

// Forward declarations
namespace UI {
    class PKSEFramebuffer;
    class TrainerViewScreen;
}

namespace UI {
namespace Dialogs {
    void drawSaveConfirmDialog(UI::TrainerViewScreen& screen, UI::PKSEFramebuffer& fb);
}
}

#endif
