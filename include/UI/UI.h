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
#include "Legacy/RetroArchFRLGDiscovery.h"

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
        // The application lifecycle owns the read-only legacy-source catalog. Keeping the
        // validated adapter results here both avoids reparsing during one app session and makes
        // the discovered Party/Boxes model available to the existing source browser when its
        // legacy-card routing is added. Nothing in this catalog exposes a write operation.
        PokeVault::Legacy::FRLGDiscoveryResult legacyFRLGSources;

        void handleSaveSelection();
        void handleBackupSelection(AccountUid userUid, u64 titleId, const std::string& titleName);
        // loadedFromCart records source provenance only. Installed game saves remain read-only;
        // edits are written to the automatically created working backup or a new named backup.
        bool handleTrainerView(AccountUid userUid, u64 titleId, const std::string& titleName,
                               const std::string& backupDir, bool loadedFromCart,
                               std::string& error);
        bool handleLegacyFRLGView(size_t sourceIndex, std::string& error);
    };
}

#endif
