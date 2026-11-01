#ifndef UI_SAVE_SELECT_SCREEN_H
#define UI_SAVE_SELECT_SCREEN_H

#include <array>
#include <cstdint>
#include <map>
#include <optional>
#include <vector>
#include "UI/SourceBackupClassification.h"
#include "UI/RecoveryManagementFs.h"
#include <memory>
#include <string>
#include <string_view>

#include <switch.h>

#include "UI/UIScreen.h"
#include "UI/NavigationRepeat.h"
#include "UI/TouchScroll.h"
#include "UI/SharedScrollbar.h"
#include "UI/QuickGamesDrawerMotion.h"
#include "UI/PKSEFramebuffer.h"
#include "UI/GameLaunchModel.h"
#include "Legacy/FRLGSourceBrowser.h"
#include "Legacy/LegacySourceBindings.h"
#include "Integration/Gen4/Gen4SourceDiscovery.h"
#include "Integration/Gen5/Gen5SourceDiscovery.h"
#include "Integration/Gen5/Gen5GameCardPreview.h"
#include "Integration/Gen5/Gen5SourceSelection.h"
#include "UI/ProfileTitleFocus.h"
#include "Source/HomeCardSnapshot.h"

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
            Gen4AssignedFile,
            Gen5AssignedFile,
            // Gen VI+ exact-game cards served by the shared Modern adapter (Integration/Modern):
            // explicit per-profile assignment of a validated external 3DS save archive.
            ModernAssignedFile,
            // The installed Switch title of a Gen VI-IX game card with no external save assigned:
            // its save is mounted read-only, copied into an app-owned snapshot and opened through
            // the same shared adapter (UIManager::handleModernInstalledView). Never written back.
            ModernInstalledSnapshot,
        };

        enum class OpenIntent { Default, Items, Backups };

        enum class MainMenuDestination {
            None,
            MasterVault,
            Pokedex,
            Banks,
            Search,
            More,
            Settings,
        };

        struct NavigationState {
            int userIndex = 0;
            int titleIndex = 0;
            int hubDockIndex = 0;
            int hubFeatureIndex = -1;
            int scrollRow = 0;
            bool hubDockFocused = false;
            bool classicGamesActive = false;

            // Stable identity survives title/source refreshes that can reorder the visible list.
            // Indices remain only as a last-resort fallback when a source genuinely disappears.
            std::string profileIdentity;
            std::string gameId;
            std::string sourceIdentity;
            SelectedSourceKind sourceKind = SelectedSourceKind::None;
            u64 titleId = 0;
        };

        SaveSelectScreen(PokeVault::Legacy::FRLGDiscoveryResult& legacySources,
                         PokeVault::Legacy::LegacySourceBindings& legacyBindings,
                         const NavigationState* resumeState = nullptr);
        void update(const PadState& pad, const TouchInput& touch) override;
        void draw(PKSEFramebuffer& fb) override;
        bool shouldExit() const override { return exitRequested; }
        bool hasRequestedAppExit() const { return appExitRequested; }
        MainMenuDestination getRequestedMainMenuDestination() const {
            return requestedMainMenuDestination;
        }
        [[nodiscard]] NavigationState navigationState() const;
        // Resume Product Home after an editor without rebuilding the whole catalog.
        void resumeAfterEditor();
        // Explicit Open failures stay on the selected card and surface the validation reason.
        void reportOpenFailure(std::string message);

        bool hasSelectedTitle() const { return titleSelected; }
        AccountUid getSelectedUser() const { return selectedUserUid; }
        u64 getSelectedTitleId() const { return selectedTitleId; }
        const std::string& getSelectedTitleName() const { return selectedTitleName; }
        const std::string& getSelectedGameId() const { return selectedGameId; }
        SelectedSourceKind getSelectedSourceKind() const { return selectedSourceKind; }
        PokeVault::Integration::Gen5::SaveCopySelection getSelectedGen5Copy() const {
            return selectedGen5Copy;
        }
        const std::string& getSelectedGen5Fingerprint() const {
            return selectedGen5Fingerprint;
        }
        size_t getSelectedLegacySourceIndex() const { return selectedLegacySourceIndex; }
        OpenIntent getOpenIntent() const { return openIntent; }
        const PokeVault::Source::ImmutableSnapshot::File& getSelectedBackupSnapshot() const {
            return selectedBackupSnapshot;
        }
        const std::string& getSelectedBackupLabel() const { return selectedBackupLabel; }
        // The card's raw provider label for an opened backup copy (top bar names its provider).
        const std::string& getSelectedBackupProvider() const { return selectedBackupProvider; }

        // Read-only inspection used by host UI tests (tests/test_ui_modern_game_cards_host.cpp).
        // Nothing here mutates sources; focusGameForInspection only moves HOME focus.
        struct CardInspection {
            std::string gameId;
            std::string name;
            std::string label;
            std::string sourceLabel;
            SelectedSourceKind sourceKind = SelectedSourceKind::None;
            u64 titleId = 0;
            bool linkedSave = false;
        };
        [[nodiscard]] std::vector<CardInspection> cardsForInspection() const;
        bool focusGameForInspection(std::string_view gameId);
        [[nodiscard]] std::string_view overlayForInspection() const;
        [[nodiscard]] std::string noticeForInspection() const;
        [[nodiscard]] const GameLaunchDescriptor& launchForInspection() const { return launchDescriptor; }
        [[nodiscard]] int focusedIndexForInspection() const { return titleIndex; }

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
            std::string artworkKey;
            std::string trainerName;
            uint8_t trainerGender = 0;
            bool trainerGenderKnown = false;
            uint16_t dexSeen = 0;
            uint16_t dexCaught = 0;
            uint16_t dexTotal = 0;
            // Raw provider recorded by discovery/manual link in Save Instance
            // metadata (binding sourceType for Gen IV/V). Empty while the card
            // is unassigned/missing/invalid/ambiguous.
            std::string providerLabel;
            // An explicit Gen IV/V assignment currently resolves to a file.
            bool linkedSave = false;
            // Presentation state of the validated Home snapshot for this card.
            PokeVault::Source::SnapshotState snapshotState =
                PokeVault::Source::SnapshotState::Unavailable;
            std::vector<PokeVault::Legacy::FRLGSaveInstance> legacyInstances;
            SelectedSourceKind sourceKind = SelectedSourceKind::SwitchTitle;
            // A Gen VI-IX card merged with its installed Switch title keeps the console card's own
            // facts here (titleId != 0 on the card as well), so reassignment can restore them.
            std::shared_ptr<TitleEntry> installedFacts;
        };
        struct UserEntry {
            AccountUid uid;
            std::string name;
            std::vector<TitleEntry> titles;
        };
        struct HitRect { int x, y, w, h, idx; };
        struct PartyPreviewSlot {
            uint16_t species = 0;
            uint8_t level = 0;
            uint8_t form = 0;
            bool shiny = false;
            std::string name;
        };
        struct LaunchFileEntry {
            std::string name;
            std::string path;
            bool directory = false;
        };

        enum class GameSortMode : uint8_t {
            ReleaseDate = 0,
            MostPlayed,
            RecentlyPlayed,
            RecentlyAdded,
            Favorites,
        };
        struct GameHubRecord {
            bool favorite = false;
            uint32_t launchCount = 0;
            int64_t lastPlayed = 0;
            int64_t firstSeen = 0;
        };

        std::vector<UserEntry> users;
        int userIndex = 0;
        int titleIndex = 0;
        std::array<PartyPreviewSlot, 6> partyPreview{};
        std::string partyPreviewStatus;
        std::string previewTrainerName;
        uint8_t previewTrainerGender = 0;
        bool previewTrainerGenderKnown = false;
        uint16_t previewDexSeen = 0;
        uint16_t previewDexCaught = 0;
        uint16_t previewDexTotal = 0;
        GameLaunchDescriptor launchDescriptor;
        // Where the emulator's Quit goes for the ready launch (Thread L's quit-route notice),
        // recomputed only when the resolved launch target changes, never per frame.
        std::string launchQuitNotice;
        std::string launchQuitNoticeKey;
        std::string hubNotice;
        std::string openFailureMessage;
        bool launchLegacyMode = false;
        OpenIntent openIntent = OpenIntent::Default;
        bool classicGamesActive = false;
        bool helpReturnClassicGames = false;
        GameSortMode gameSortMode = GameSortMode::ReleaseDate;
        std::map<std::string, GameHubRecord> gameHubRecords;

        std::vector<LaunchFileEntry> launchFileEntries;
        std::string launchBrowsePath;
        std::string launchBrowseRoot;
        std::string launchLinkBindingKey;
        std::string launchLinkGameId;
        std::string launchLinkProviderId;
        std::string launchLinkSourcePath;
        std::string launchFileNotice;
        int launchFileIndex = 0;
        int launchFileScroll = 0;
        TouchScrollState launchFileTouchScroll;
        bool launchFileReturnToLegacy = false;

        bool titleSelected = false;
        bool exitRequested = false;     // return from Games & Sources to product Home
        bool appExitRequested = false;  // set only by an explicit Y in the leave decision
        // P0 #320: every way out of PokeBank NX (quit, or a launch handoff that closes the app)
        // first opens one visible leave decision; only an explicit Y there commits it.
        enum class LeaveKind : uint8_t { Quit, LaunchTitle, LaunchLegacyInstance };
        LeaveKind leaveKind = LeaveKind::Quit;
        GameLaunchDescriptor pendingLaunch;
        std::string pendingLaunchName;
        std::string pendingLaunchNotice;
        std::string pendingLaunchStarter = "the emulator"; // LeaveDecisionLayout::launchStarter
        void openLeaveDecision(LeaveKind kind, const char* origin);
        void openLaunchDecision(LeaveKind kind, const GameLaunchDescriptor& descriptor,
                                const std::string& gameName, const char* origin);
        MainMenuDestination requestedMainMenuDestination = MainMenuDestination::None;
        bool hubDockFocused = false;
        int headerActionIndex = -1; // -1 = none, 0 = profile, 1 = settings
        int hubDockIndex = 0;
        int gamesDrawerIndex = 0;
        int gamesDrawerScroll = 0;
        TouchScrollState gamesDrawerTouchScroll;
        SharedScrollbar::State gamesDrawerScrollbar;   // draggable thumb beside the Quick Games grid
        // One shared draggable scrollbar per scrolling list on this screen (R2-SCROLLBAR).
        SharedScrollbar::State profileScrollbar;
        SharedScrollbar::State launchFileScrollbar;
        SharedScrollbar::State legacyInstanceScrollbar;
        SharedScrollbar::State legacyAssignmentScrollbar;
        SharedScrollbar::State gen4CandidateScrollbar;
        SharedScrollbar::State gen5CandidateScrollbar;
        SharedScrollbar::State classicGamesScrollbar;
        SharedScrollbar::State emuBackupScrollbar;
        PokeBank::UIModel::QuickGamesDrawerMotion quickDrawerMotion;
        int profilePickerIndex = 0;
        int profilePickerScroll = 0; // independent visible first row; never scroll the focus index
        TouchScrollState profileTouchScroll;
        // Main product-home focus outside the persistent dock:
        // -1 = selected game card, 0 = Master Vault, 1 = Pokédex.
        int hubFeatureIndex = -1;
        enum class Overlay { None, Options, Help, OpenFailure, GamesDrawer, ProfilePicker, GameWorkspace,
                             LegacyInstances, LegacyAssignment, LegacyDetails, Gen4Setup,
                             Gen4Candidates, Gen5Setup, Gen5UnassignConfirm,
                             Gen5Candidates, GameFilePicker, QuitConfirm,
                             EmulatorBackups, EmulatorBackupDeleteConfirm,
                             RecoveryFiles, RecoveryConfirm };
        Overlay overlay = Overlay::None;
        Overlay helpReturnOverlay = Overlay::None;
        Overlay leaveReturnOverlay = Overlay::None; // where A/B "Stay" returns from the leave decision
        // + -> Backups for an emulator game (round 2 phase 7): one listing per exact source file.
        std::vector<PokeBank::UIModel::SourceBackups::Listing> emuBackupListings;
        struct EmuBackupRow { int listing = 0; int entry = 0; };
        std::vector<EmuBackupRow> emuBackupRows;
        int emuBackupIndex = 0;
        int emuBackupScroll = 0;
        std::string emuBackupNotice;
        std::string emuBackupProvider; // provider display name of the card the Backups list belongs to
        std::string emuBackupGameId;   // exact game of the card (names the backup folder)
        void openEmulatorBackups(const std::string& notice = {});
        void createEmulatorBackup(int listing);
        void browseEmulatorSource(const std::string& source);
        // Owner item #10: Backups -> X "Recovery files". Review older copies and DurableFile
        // leftovers (UI/RecoveryManagementModel.h decides what may go), select with A, Y opens a
        // confirmation that lists every file and its size, Y there removes, A/B keep. Nothing is
        // selected by default and nothing is removed on a single press.
        PokeBank::UIModel::RecoveryManagement::Fs::Request recoveryRequest;
        PokeBank::UIModel::RecoveryManagement::Fs::Listing recoveryListing;
        PokeBank::UIModel::RecoveryManagement::Selection recoverySelection;
        PokeBank::UIModel::RecoveryManagement::Plan recoveryPlan;
        std::vector<PokeBank::UIModel::RecoveryManagement::Row> recoveryRows;
        int recoveryIndex = 0;
        int recoveryScroll = 0;
        int recoveryConfirmScroll = 0;   // first file line shown in the confirmation
        std::string recoveryNotice;
        SharedScrollbar::State recoveryScrollbar;
        void openRecoveryFiles(const std::string& notice = {});
        void rebuildRecoveryRows();
        // Touch motion is surface-local. These markers let one centralized fence clear residual
        // drag/coast state whenever an overlay or the full Games browser is entered/exited.
        Overlay touchScrollOverlay = Overlay::None;
        bool touchScrollClassicGamesActive = false;
        int optionsIndex = 0;
        int gameWorkspaceIndex = 0;
        int legacyInstanceIndex = 0;
        int legacyInstanceScroll = 0;
        TouchScrollState legacyInstanceTouchScroll;
        AccountUid selectedUserUid{};
        u64 selectedTitleId = 0;
        std::string selectedTitleName;
        std::string selectedGameId;
        SelectedSourceKind selectedSourceKind = SelectedSourceKind::None;
        PokeVault::Source::ImmutableSnapshot::File selectedBackupSnapshot;
        std::string selectedBackupLabel;
        std::string selectedBackupProvider;
        // Ephemeral, exact-session selection; never guessed from file mtime
        // and never grants emulator/source write authority.
        PokeVault::Integration::Gen5::SaveCopySelection selectedGen5Copy =
            PokeVault::Integration::Gen5::SaveCopySelection::Automatic;
        // Only the normalized exact source bytes selected for THIS editor
        // open, never a permission token or a remembered game preference.
        std::string selectedGen5Fingerprint;
        // Why the last direct open of an assigned Gen V save fell back to the
        // Save Instances picker (empty when nothing was assigned).
        std::string gen5DirectOpenNotice;
        bool openAssignedGen5Directly(const UserEntry& user, const TitleEntry& title,
                                      const std::string& gameId);
        static PokeVault::Integration::Gen5::SaveCopySelection gen5CopyForVariant(
            const std::string& variant) {
            using PokeVault::Integration::Gen5::SaveCopySelection;
            if (variant == "gen5-copy-0-primary") return SaveCopySelection::Primary;
            if (variant == "gen5-copy-1-backup") return SaveCopySelection::Backup;
            return SaveCopySelection::Automatic;
        }
        size_t selectedLegacySourceIndex = 0;
        PokeVault::Legacy::FRLGDiscoveryResult* legacyCatalog = nullptr;
        PokeVault::Legacy::LegacySourceBindings* legacyBindings = nullptr;
        std::string legacyNotice;
        struct LegacyAssignmentEntry {
            std::string gameId;
            std::string title;
            PokeVault::Legacy::FRLGSaveInstance instance;
        };
        std::vector<LegacyAssignmentEntry> unassignedLegacySources;
        int legacyAssignmentIndex = 0;
        int legacyAssignmentScroll = 0;
        TouchScrollState legacyAssignmentTouchScroll;
        PokeVault::Legacy::FRLGSaveInstance legacyDetailsInstance;
        std::string legacyDetailsGameId;

        // Gen IV game cards are always visible. Pressing A discovers/refreshes validated save
        // instances and always shows a chooser before opening. The existing assignment database is
        // retained only as a remembered/manual source so paths outside known emulator roots remain
        // reachable; it must never bypass the Save Instances chooser.
        std::string gen4TargetGameId;
        std::string gen4Notice;
        bool gen4SetupFromGamesDrawer = false;
        bool gen4SetupFromClassicGames = false;
        // Generation-specific candidates stay as opaque validation handles; the chooser itself
        // consumes the same provider-neutral SaveInstance rows used by Gen I-III.
        std::vector<PokeVault::Integration::Gen4::SourceCandidate> gen4Candidates;
        std::vector<PokeVault::Source::SaveInstance> gen4Instances;

        // Isolated Gen V preview-only Game Sources; original SAV stays immutable.
        std::string gen5TargetGameId;
        std::string gen5Notice;
        bool gen5SetupFromGamesDrawer = false;
        // Explicit Source/Game File action only links a validated save to
        // the active Switch profile; A/Open Editor remains a separate route.
        bool gen5AssignOnly = true;
        bool gen5SetupReturnCandidates = false;
        std::vector<PokeVault::Source::SaveInstance> gen5Instances;
        int gen5SetupIndex = 0;
        int gen5CandidateIndex = 0;
        int gen5CandidateScroll = 0;
        TouchScrollState gen5CandidateTouchScroll;
        PokeVault::Integration::Gen5::ProfileGameFocus gen5ProfileFocus;
        PokeBank::UIModel::ProfileTitleFocus profileTitleFocus; // every card kind (Gen V above stays)
        int gen4SetupIndex = 0;
        int gen4CandidateIndex = 0;
        int gen4CandidateScroll = 0;
        TouchScrollState gen4CandidateTouchScroll;

        // Tap targets captured during draw(), hit-tested on the next update().
        std::vector<HitRect> titleRects;
        std::vector<HitRect> userRects;
        std::vector<HitRect> dockRects;
        std::vector<HitRect> headerRects;
        std::vector<HitRect> featureRects;
        std::vector<HitRect> overlayRects;

        void syncTouchScrollSurface();
        void activateHubDock();
        void activateGameWorkspace();
        void openSaveSourceForCurrentTitle(bool fromGamesDrawer, bool fromClassicGames);
        void loadGameHubState();
        void saveGameHubState() const;
        void sortGamesPreservingSelection();
        void cycleGameSortMode();
        void toggleCurrentFavorite();
        void recordCurrentLaunch();
        [[nodiscard]] bool titleFavorite(const UserEntry& user, const TitleEntry& title) const;
        [[nodiscard]] const char* gameSortModeLabel() const;
        void loadUsers();
        void loadLegacySources(const PokeVault::Legacy::FRLGDiscoveryResult& legacySources);
        void loadGen4Cards();
        void loadGen5Cards();
        void loadModernCards();
        void rebuildUnassignedLegacySources();
        bool assignCurrentLegacySource();
        [[nodiscard]] std::string currentProfileIdentity() const;
        [[nodiscard]] std::string currentSourceIdentity() const;
        [[nodiscard]] const PokeVault::Legacy::FRLGSaveInstance* currentLegacyInstance() const;
        bool refreshLegacySources(const std::string& gameId,
                                  const std::string& preferredSourceIdentity = {},
                                  bool requirePreferred = false);
        // Titles come from enumerating SAVE DATA, not installed applications: a game played from a
        // cartridge that is currently out, or one that has been uninstalled, keeps its save on
        // internal storage and must still be editable.
        void loadTitlesForUser(UserEntry& user);
        static bool scanSaveSpace(UserEntry& user, int spaceId, int& scanned, int& forUser);
        void setUser(int idx);
        void refreshHubSelectionFromCache();
        // Fingerprinted, validated Home snapshot of an assigned Gen IV/V save.
        // stat() on every call; the strict parser only when size/mtime or the
        // assignment changed. Never opens an editor or writes a source.
        const PokeVault::Source::HomeCardSnapshot* refreshAssignedSnapshot(
            const std::string& profile, const TitleEntry& title,
            PokeVault::Source::SnapshotRefresh* outcome = nullptr);
        void applyAssignedCardState(TitleEntry& card, const std::string& profile);
        void applySnapshotToHub(const PokeVault::Source::HomeCardSnapshot& snapshot);
        // Re-check only the focused card's save file; reparse only on change.
        void pollFocusedSaveChange();
        bool syncFocusedSaveSnapshot();
        std::string legacyWatchKey;
        PokeVault::Source::HomeSnapshotCache homeSnapshots;
        int saveWatchFrames = 0;
        void refreshHubPreview(bool resolveLaunchTarget = true);
        const std::string& currentLaunchQuitNotice();
        bool launchCurrentTitle();
        bool launchCurrentLegacyInstance();
        bool beginLaunchLinkForCurrentTitle();
        void openGameFilePicker(const std::string& gameId,
                                const std::string& providerId,
                                const std::string& sourcePath,
                                const std::string& bindingKey,
                                bool returnToLegacy);
        void refreshGameFilePicker();
        void activateGameFilePicker();
        void browseGameFileParent();
        void selectCurrentTitle();
        void selectCurrentTitleForItems();
        void selectCurrentLegacyInstance();
        void openGen4Setup(const std::string& gameId, std::string notice = {},
                           bool returnToGamesDrawer = false, bool returnToClassicGames = false);
        void discoverGen4Candidates();
        bool assignGen4Candidate(const PokeVault::Integration::Gen4::SourceCandidate& candidate);
        void chooseGen4ManualFile();
        bool unassignCurrentGen4Game();
        void selectAssignedGen4Title();
        void openGen5Setup(const std::string& gameId, std::string notice = {},
                           bool returnToGamesDrawer = false);
        void discoverGen5Candidates();
        void closeGen5Setup();
        bool assignGen5Candidate(const PokeVault::Source::SaveInstance& chosen);
        void chooseGen5ManualFile();
        bool unassignCurrentGen5Game();
        // Modern-adapter titles reuse the Gen V Save Instances overlays (gen5TargetGameId names the
        // target); these are the family-specific discovery/assign/open steps behind them.
        bool openAssignedModernDirectly(const UserEntry& user, const TitleEntry& title,
                                        const std::string& gameId);
        void discoverModernCandidates();
        bool assignModernCandidate(const PokeVault::Source::SaveInstance& chosen);
        void chooseModernManualFile();
        bool unassignCurrentModernGame();

        const UserEntry* currentUser() const;
        void selectInstalledModernSnapshot(const UserEntry& user, const TitleEntry& title);
        void applyModernLaunchState(const TitleEntry& title);
        void openModernSaveInstances(const std::string& gameId, const std::string& reason);
        [[nodiscard]] std::string modernUnlinkedStatus(const TitleEntry& title) const;
        int titleColumns() const;      // grid columns for the current user's title count (<= 5)
        int titleRows() const;         // rows those tiles occupy
        int classicTitleColumns() const;
        int classicTitleRows() const;
        void drawClassicGameSources(PKSEFramebuffer& fb);

        // Top row of the scroll window. Persistent STATE, not derived from the selection: it moves
        // only when the selected tile would otherwise fall outside the window, so the grid holds
        // still while the cursor moves within it instead of re-centring (which reads as paging).
        int scrollRow = 0;
        TouchScrollState classicGamesTouchScroll;
        void scrollSelectionIntoView();
        void scrollClassicSelectionIntoView();
    };
}

#endif
