#ifndef UI_UI_H
#define UI_UI_H

#include <switch.h>

#include "UI/Common.h"
#include "UI/PKSEFramebuffer.h"
#include "UI/UIScreen.h"
#include "UI/TouchInput.h"
#include "UI/SaveSelectScreen.h"
#include "UI/BackupSelectionScreen.h"
#include "UI/TrainerViewScreen.h"

namespace Trainer {
    class Trainer;
}

namespace Pokemon {
    struct Pokemon8SWSH;
}

namespace UI {

    class UIManager {
    public:
        UIManager();
        ~UIManager();

        void run();

    private:
        PKSEFramebuffer fb;
        PadState pad;
        TouchInput touch;
        bool running;

        void handleSaveSelection();
        void handleBackupSelection(AccountUid userUid, u64 titleId, const std::string& titleName);
        // loadedFromCart records source provenance only. Installed game saves remain read-only;
        // edits are written to the automatically created working backup or a new named backup.
        void handleTrainerView(AccountUid userUid, u64 titleId, const std::string& titleName,
                               const std::string& backupDir, bool loadedFromCart);
    };
}

#endif
