#ifndef UI_BACKUP_SELECTION_SCREEN_H
#define UI_BACKUP_SELECTION_SCREEN_H

#include <vector>
#include <string>

#include <switch.h>

#include "UI/UIScreen.h"
#include "UI/NavigationRepeat.h"
#include "UI/PKSEFramebuffer.h"
#include "UI/TouchScroll.h"
#include "UI/SharedScrollbar.h"
#include "UI/BackupBrowserModel.h"
#include "UI/SourceBackupClassification.h"

namespace UI {
    // The full-screen PKSE Save Backups picker. One screen for installed Switch titles and, since
    // round 5 (#322), for Gen I-V emulator games: current save, "Create backup now", then every
    // copy newest first (BackupBrowserModel.h). A on a copy opens THAT copy through an isolated
    // staged session (UIManager); the copy and the live original are never written.
    class BackupSelectionScreen : public UIScreen {
    public:
        // Installed Switch title: profile/exact-game PKSE folders plus read-only legacy folders.
        BackupSelectionScreen(AccountUid userUid, u64 titleId, const std::string& titleName);

        // Emulator game: the exact-source listings Product Home built (SourceBackups::list).
        struct EmulatorGame {
            std::string gameId;           // exact release, e.g. black_nds
            std::string titleName;        // "Pokemon Black"
            std::string providerLabel;    // raw provider as discovery recorded it
            std::string providerDisplay;  // "DraStic"
            std::vector<PokeBank::UIModel::SourceBackups::Listing> listings;
        };
        BackupSelectionScreen(AccountUid userUid, EmulatorGame game);

        void update(const PadState& pad, const TouchInput& touch) override;
        void draw(PKSEFramebuffer& fb) override;
        bool shouldExit() const override { return goBack; }

        bool hasSelectedBackup() const { return backupSelected; }
        // One-shot: the user pressed A on "Create backup now" for an installed title (the only way
        // a Switch backup is made here). Emulator backups are made by this screen itself.
        bool takeCreateBackupRequest() {
            const bool requested = createBackupRequested;
            createBackupRequested = false;
            return requested;
        }
        // After a backup was made: list it and say so, keeping the screen open.
        void reloadWithNotice(const std::string& message) {
            loadBackups();
            statusMessage = message;
            statusFrames = 300;
        }
        // True for A on the current save (Browse), false for A on a copy (Open).
        bool shouldCreateNewBackup() const { return createNewBackup; }
        const std::string& getSelectedBackupPath() const { return selectedBackupPath; }
        // The row A acted on: its kind, date and path (for the editor's provenance banner).
        const PokeBank::UIModel::BackupBrowser::Row& selectedRow() const { return selectedRowValue; }
        bool isEmulator() const { return emulator; }
        const EmulatorGame& emulatorGame() const { return emulatorGameValue; }

        /**
         * Report that acting on the selection failed, so the screen can say so and take the choice
         * back. Without this the caller's only option is to bail out of the whole flow, which
         * drops the user back at the save picker with no indication that anything went wrong — the
         * failure is indistinguishable from having pressed B.
         */
        void reportFailure(const std::string& message) {
            statusMessage = message;
            statusFrames = 480;
            backupSelected = false;      // hand the choice back rather than re-firing it every frame
            createNewBackup = false;
        }

    private:
        PokeBank::UIModel::ControllerNavigation controllerNavigation;
        TouchScrollState backupScroll;
        SharedScrollbar::State backupScrollbar;   // draggable thumb beside the backup list
        struct BackupInfo {
            PokeBank::UIModel::BackupBrowser::Row row;
            std::string displayName;     // primary line (date / name / action)
            std::string detail;          // secondary line (what it is)
            std::string path;
            bool legacyUnscoped = false; // pre-A08 title-only folder: read-only, never deleted
            bool createAction = false;   // the "Create backup now" row
        };

        AccountUid userUid{};
        u64 titleId = 0;
        std::string titleName;
        std::string exactGameId;
        std::string gameDirectory;
        std::string legacyGameDirectory;
        bool namespaceReady = false;
        bool emulator = false;
        EmulatorGame emulatorGameValue;
        std::vector<BackupInfo> backups;
        int selectedIndex = 0;
        int backupFirstRow = 0;  // Physical viewport; never reuse the action cursor as scroll position.
        bool backupSelected = false;
        bool createNewBackup = false;  // True if user wants to browse the current save
        bool createBackupRequested = false;
        bool goBack = false;
        std::string selectedBackupPath;
        PokeBank::UIModel::BackupBrowser::Row selectedRowValue;
        bool showDeleteConfirmation = false;
        int deleteConfirmationIndex = -1;
        // Delete-confirm button rects, captured during draw and hit-tested next frame (this screen
        // has no touchButtons framework; same one-frame-late contract as TrainerViewScreen).
        struct DlgBtn { int x = 0, y = 0, w = 0, h = 0; };
        DlgBtn deleteCancelBtn, deleteDeleteBtn;
        std::string statusMessage;   // transient failure notice, shown just above the nav bar
        int statusFrames = 0;

        void loadBackups();
        void loadSwitchRows(std::vector<PokeBank::UIModel::BackupBrowser::Row>& rows);
        void createEmulatorBackup(int listing);
        void drawBackupList(PKSEFramebuffer& fb);
        void drawDeleteConfirmation(PKSEFramebuffer& fb);
        std::string formatTimestamp(const std::string& timestamp);
        std::string saveName() const;
        void deleteBackup(int index);
    };
}

#endif
