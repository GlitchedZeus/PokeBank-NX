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
#include "Legacy/ModernReadOnlyTrainer.h"
#include "Utils/Keyboard.h"

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
        UIScreen* keyboardBackdrop = nullptr;
        PokeVault::Legacy::FRLGDiscoveryResult legacyFRLGSources;
        PokeVault::Legacy::LegacySourceBindings legacySourceBindings;
        SaveSelectScreen::NavigationState productHomeNavigation{};
        bool productHomeNavigationValid = false;
        AppShellScreen::NavigationState appShellNavigation{};
        bool appShellNavigationValid = false;

        Utils::KeyboardResult runKeyboard(const Utils::KeyboardRequest& request);
        void drawKeyboardBackdrop();

        SaveSelectScreen::MainMenuDestination handleSaveSelection();
        void handleBackupSelection(AccountUid userUid, u64 titleId, const std::string& titleName,
                                   SaveSelectScreen::OpenIntent intent);
        bool handleDefaultQuickOpen(AccountUid userUid, u64 titleId,
                                    const std::string& titleName, std::string& error);
        void handleItemsQuickOpen(AccountUid userUid, u64 titleId, const std::string& titleName);
        bool handleTrainerView(AccountUid userUid, u64 titleId, const std::string& titleName,
                               const std::string& backupDir, bool loadedFromCart,
                               SaveSelectScreen::OpenIntent intent, std::string& error,
                               const PokeVault::Source::ImmutableSnapshot::Directory* historical = nullptr,
                               const std::string& provenance = {});
        bool handleLegacyFRLGView(AccountUid userUid, size_t sourceIndex, const std::string& gameId,
                                  SaveSelectScreen::OpenIntent intent, std::string& error);
        bool handleEmulatorBackupView(AccountUid userUid,const std::string& gameId,
            const PokeVault::Source::ImmutableSnapshot::File& file,const std::string& label,
            const std::string& provider,std::string& error);
        bool handleGen4View(AccountUid userUid, const std::string& gameId,
                            SaveSelectScreen::OpenIntent intent, std::string& error);
        bool handleGen5View(AccountUid userUid, const std::string& gameId,
                            SaveSelectScreen::OpenIntent intent,
                            PokeVault::Integration::Gen5::SaveCopySelection selectedCopy,
                            std::string_view selectedFingerprint,
                            std::string& error);
        // Gen VI+ titles served by the shared Modern adapter (Integration/Modern): the ONE
        // TrainerViewScreen over a ModernReadOnlyTrainer; the source save is never written.
        bool handleModernInstalledView(AccountUid userUid, u64 titleId, const std::string& gameId,
                                       const std::string& titleName, SaveSelectScreen::OpenIntent intent,
                                       std::string& error);
        void runModernScreen(PokeVault::Legacy::ModernReadOnlyTrainer& trainer, const std::string& title,
                             const std::string& sourcePath, AccountUid userUid, const std::string& gameId,
                             const std::string& sourceType, SaveSelectScreen::OpenIntent intent);
        bool handleModernView(AccountUid userUid, const std::string& gameId,
                              SaveSelectScreen::OpenIntent intent,
                              std::string_view selectedFingerprint,
                              std::string& error);
    };
}

#endif
