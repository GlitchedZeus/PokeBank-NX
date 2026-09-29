#ifndef UI_UI_H
#define UI_UI_H

#include <switch.h>

#include "UI/Common.h"
#include "UI/PKSEFramebuffer.h"
#include "UI/UIScreen.h"
#include "UI/TouchInput.h"
#include "UI/SaveSelectScreen.h"
#include "UI/AppShellScreen.h"
#include "UI/BackupSelectionScreen.h"
#include "UI/TrainerViewScreen.h"
#include "Legacy/RetroArchFRLGDiscovery.h"
#include "Legacy/LegacySourceBindings.h"

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
        PokeVault::Legacy::FRLGDiscoveryResult legacyFRLGSources;
        PokeVault::Legacy::LegacySourceBindings legacySourceBindings;
        SaveSelectScreen::NavigationState productHomeNavigation{};
        bool productHomeNavigationValid = false;
        AppShellScreen::NavigationState appShellNavigation{};
        bool appShellNavigationValid = false;

        SaveSelectScreen::MainMenuDestination handleSaveSelection();
        void handleBackupSelection(AccountUid userUid, u64 titleId, const std::string& titleName);
        bool handleTrainerView(AccountUid userUid, u64 titleId, const std::string& titleName,
                               const std::string& backupDir, bool loadedFromCart,
                               std::string& error);
        bool handleLegacyFRLGView(AccountUid userUid, size_t sourceIndex, const std::string& gameId,
                                  std::string& error);
        bool handleGen4View(AccountUid userUid, const std::string& gameId, std::string& error);
    };
}

#endif
