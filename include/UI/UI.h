#ifndef UI_UI_H
#define UI_UI_H

#include <switch.h>

#include "UI/Common.h"
#include "UI/PKSEFramebuffer.h"
#include "UI/UIScreen.h"
#include "UI/UserSelectionScreen.h"
#include "UI/TitleSelectionScreen.h"
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
        bool running;

        void handleUserSelection();
        void handleTitleSelection(AccountUid userUid);
        void handleBackupSelection(AccountUid userUid, u64 titleId, const std::string& titleName);
        void handleTrainerView(AccountUid userUid, u64 titleId, const std::string& titleName, const std::string& backupDir);
    };
}

#endif
