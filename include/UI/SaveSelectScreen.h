#ifndef UI_SAVE_SELECT_SCREEN_H
#define UI_SAVE_SELECT_SCREEN_H

#include <vector>
#include <string>

#include <switch.h>

#include "UI/UIScreen.h"
#include "UI/NavigationRepeat.h"
#include "UI/PKSEFramebuffer.h"
#include "Legacy/FRLGSourceBrowser.h"

namespace UI {
    // JKSV-style combined user + title picker. Shows the selected user's avatar + name at the top
    // and a grid of that user's supported Pokemon game icons below. Replaces the old two-step
    // UserSelection -> TitleSelection flow. Switch users with the L/R shoulders (or by tapping a
    // user chip) when more than one account exists. Only titles PKSE supports, and only ones the
    // user actually has a save for, appear.
    class SaveSelectScreen : public UIScreen {
    public:
        enum class SelectedSourceKind {
            None,
            SwitchTitle,
            RetroArchFRLG,
        };

        explicit SaveSelectScreen(const PokeVault::Legacy::FRLGDiscoveryResult& legacySources);
        void update(const PadState& pad, const TouchInput& touch) override;
        void draw(PKSEFramebuffer& fb) override;
        bool shouldExit() const override { return exitRequested; }

        bool hasSelectedTitle() const { return titleSelected; }
        AccountUid getSelectedUser() const { return selectedUserUid; }
        u64 getSelectedTitleId() const { return selectedTitleId; }
        const std::string& getSelectedTitleName() const { return selectedTitleName; }
        const std::string& getSelectedGameId() const { return selectedGameId; }
        SelectedSourceKind getSelectedSourceKind() const { return selectedSourceKind; }
        size_t getSelectedLegacySourceIndex() const { return selectedLegacySourceIndex; }

    private:
        PokeBank::UIModel::ControllerNavigation controllerNavigation;
        struct TitleEntry {
            u64 titleId = 0;
            std::string name;    // full "Pokemon X" name (used for backup dir + downstream)
            std::string label;   // short display name under the icon (e.g. "Shield")
            std::string gameId;  // stable release + platform identity (e.g. firered_switch)
            std::string platformLabel;
            std::string sourceLabel = "LOCAL SAVE";
            std::string locationLabel;
            SelectedSourceKind sourceKind = SelectedSourceKind::SwitchTitle;
            size_t legacySourceIndex = 0;
        };
        struct UserEntry {
            AccountUid uid;
            std::string name;
            std::vector<TitleEntry> titles;
            bool retroArchGroup = false;
        };
        struct HitRect { int x, y, w, h, idx; };

        std::vector<UserEntry> users;
        int userIndex = 0;
        int titleIndex = 0;

        bool titleSelected = false;
        bool exitRequested = false;
        enum class Overlay { None, Options, Help };
        Overlay overlay = Overlay::None;
        int optionsIndex = 0;
        AccountUid selectedUserUid{};
        u64 selectedTitleId = 0;
        std::string selectedTitleName;
        std::string selectedGameId;
        SelectedSourceKind selectedSourceKind = SelectedSourceKind::None;
        size_t selectedLegacySourceIndex = 0;

        // Tap targets captured during draw(), hit-tested on the next update().
        std::vector<HitRect> titleRects;
        std::vector<HitRect> userRects;

        void loadUsers();
        void loadLegacySources(const PokeVault::Legacy::FRLGDiscoveryResult& legacySources);
        // Titles come from enumerating SAVE DATA, not installed applications: a game played from a
        // cartridge that is currently out, or one that has been uninstalled, keeps its save on
        // internal storage and must still be editable.
        void loadTitlesForUser(UserEntry& user);
        static bool scanSaveSpace(UserEntry& user, int spaceId, int& scanned, int& forUser);
        void setUser(int idx);
        void selectCurrentTitle();

        const UserEntry* currentUser() const;
        int titleColumns() const;      // grid columns for the current user's title count (<= 5)
        int titleRows() const;         // rows those tiles occupy

        // Top row of the scroll window. Persistent STATE, not derived from the selection: it moves
        // only when the selected tile would otherwise fall outside the window, so the grid holds
        // still while the cursor moves within it instead of re-centring (which reads as paging).
        int scrollRow = 0;
        void scrollSelectionIntoView();
    };
}

#endif
