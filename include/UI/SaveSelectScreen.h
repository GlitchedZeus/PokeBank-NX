#ifndef UI_SAVE_SELECT_SCREEN_H
#define UI_SAVE_SELECT_SCREEN_H

#include <array>
#include <cstdint>
#include <map>
#include <vector>
#include <string>

#include <switch.h>

#include "UI/UIScreen.h"
#include "UI/NavigationRepeat.h"
#include "UI/TouchScroll.h"
#include "UI/PKSEFramebuffer.h"
#include "UI/GameLaunchModel.h"
#include "Legacy/FRLGSourceBrowser.h"
#include "Legacy/LegacySourceBindings.h"
#include "Integration/Gen4/Gen4SourceDiscovery.h"

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
        size_t getSelectedLegacySourceIndex() const { return selectedLegacySourceIndex; }
        OpenIntent getOpenIntent() const { return openIntent; }

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
            std::vector<PokeVault::Legacy::FRLGSaveInstance> legacyInstances;
            SelectedSourceKind sourceKind = SelectedSourceKind::SwitchTitle;
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
        std::string hubNotice;
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
        bool appExitRequested = false;  // explicit Options -> Exit PokeBank NX
        MainMenuDestination requestedMainMenuDestination = MainMenuDestination::None;
        bool hubDockFocused = false;
        int headerActionIndex = -1; // -1 = none, 0 = profile, 1 = settings
        int hubDockIndex = 0;
        int gamesDrawerIndex = 0;
        int gamesDrawerScroll = 0;
        TouchScrollState gamesDrawerTouchScroll;
        int profilePickerIndex = 0;
        int profilePickerScroll = 0; // independent visible first row; never scroll the focus index
        TouchScrollState profileTouchScroll;
        // Main product-home focus outside the persistent dock:
        // -1 = selected game card, 0 = Master Vault, 1 = Pokédex.
        int hubFeatureIndex = -1;
        enum class Overlay { None, Options, Help, GamesDrawer, ProfilePicker, GameWorkspace,
                             LegacyInstances, LegacyAssignment, LegacyDetails, Gen4Setup,
                             Gen4Candidates, GameFilePicker };
        Overlay overlay = Overlay::None;
        Overlay helpReturnOverlay = Overlay::None;
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
        void refreshHubPreview(bool resolveLaunchTarget = true);
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

        const UserEntry* currentUser() const;
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
