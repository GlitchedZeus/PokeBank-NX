#include <algorithm>
#include <cstring>
#include <cstdio>
#include <iomanip>
#include <sstream>

#include "Globals.h"
#include "UI/SaveSelectScreen.h"
#include "UI/Common.h"
#include "UI/ScreenChrome.h"
#include "UI/SystemIcons.h"
#include "UI/TouchInput.h"
#include "Enums/GameVersion.h"
#include "Games/GameIdentity.h"
#include "Integration/Gen4/Gen4AssignedSource.h"
#include "Integration/Gen4/Gen4SourceDiscovery.h"
#include "Utils/Keyboard.h"
#include "Utils/Logger.h"
#include "Utils/Settings.h"

using namespace Utils;
using namespace Enums;

namespace UI {
    namespace {
        std::string sourceLeafName(const std::string& path) {
            const size_t slash = path.find_last_of("/\\");
            std::string leaf = slash == std::string::npos ? path : path.substr(slash + 1);
            constexpr size_t maxChars = 22;
            if (leaf.size() > maxChars) leaf = leaf.substr(0, maxChars - 3) + "...";
            return leaf;
        }

        std::string profileIdentity(AccountUid uid) {
            std::ostringstream output;
            output << std::hex << std::setfill('0')
                   << std::setw(16) << static_cast<unsigned long long>(uid.uid[0])
                   << std::setw(16) << static_cast<unsigned long long>(uid.uid[1]);
            return output.str();
        }

        std::string shortValue(const std::string& value, size_t length = 12) {
            return value.substr(0, std::min(length, value.size()));
        }

        std::string providerSummary(
            const std::vector<PokeVault::Legacy::FRLGSaveInstance>& instances) {
            std::vector<std::string> providers;
            for (const auto& instance : instances) {
                const std::string provider =
                    instance.providerLabel.empty() ? std::string("Source") : instance.providerLabel;
                if (std::find(providers.begin(), providers.end(), provider) == providers.end())
                    providers.push_back(provider);
            }
            if (providers.empty()) return "No validated providers";
            if (providers.size() == 1) return "Provider: " + providers.front();

            std::string summary = "Providers: ";
            for (size_t i = 0; i < providers.size(); ++i) {
                if (i != 0) summary += " + ";
                summary += providers[i];
            }
            return summary;
        }

        void drawSaveInstanceRows(
            PKSEFramebuffer& fb,
            const std::vector<PokeVault::Source::SaveInstance>& instances,
            int selectedIndex, int first, int x, int rowY, int width,
            int rowHeight, int visibleRows, bool showOlderLabel) {
            const int last = std::min<int>(
                static_cast<int>(instances.size()), first + visibleRows);
            for (int i = first; i < last; ++i) {
                const auto& instance = instances[static_cast<size_t>(i)];
                drawFocusedCard(fb, x + 24, rowY, width - 48, rowHeight - 6,
                                i == selectedIndex, 10);
                fb.drawText(x + 44, rowY + 8, instance.label,
                            i == selectedIndex ? Colors::TextPrimary : Colors::TextSecondary,
                            TextStyle::Body);
                if (instance.mostRecentlyModified || showOlderLabel) {
                    const std::string recency = instance.mostRecentlyModified
                        ? "MOST RECENTLY MODIFIED" : "OLDER FILE";
                    int fw = 0, fh = 0;
                    fb.measureText(recency, fw, fh, TextStyle::Caption);
                    fb.drawText(x + width - 44 - fw, rowY + 11, recency, Colors::TextMuted,
                                TextStyle::Caption);
                }
                const std::string sourceLine =
                    (instance.providerLabel.empty() ? std::string("Source") : instance.providerLabel) +
                    " / " + instance.sourceLabel;
                fb.drawText(x + 44, rowY + 34, sourceLine, Colors::TextMuted,
                            TextStyle::Caption);
                rowY += rowHeight;
            }
        }
    }

    // Layout (1280x720).
    constexpr int HEADER_Y   = 82;    // top of the user header card
    constexpr int HEADER_H   = 104;
    constexpr int AVATAR     = 84;
    constexpr int CHIP       = 56;    // small per-user switcher avatar
    constexpr int TILE_W     = 184;
    constexpr int TILE_H     = 208;
    constexpr int ICON       = 126;
    constexpr int GAP        = 16;
    constexpr int MAX_COLS   = 5;
    constexpr int GRID_Y     = 216;
    // Rows of tiles that fit between the grid's top and the nav bar: (720 - 56 - 208) = 456px of
    // band against a 224px row pitch. Anything past this scrolls rather than being drawn off the
    // bottom edge, which is how titles eleven and twelve used to vanish.
    constexpr int VISIBLE_ROWS = 2;

    SaveSelectScreen::SaveSelectScreen(
        PokeVault::Legacy::FRLGDiscoveryResult& legacySources,
        PokeVault::Legacy::LegacySourceBindings& bindings)
        : legacyCatalog(&legacySources), legacyBindings(&bindings) {
        loadUsers();
        loadLegacySources(legacySources);
        loadGen4Cards();
    }

    void SaveSelectScreen::loadLegacySources(
        const PokeVault::Legacy::FRLGDiscoveryResult& legacySources) {
        for (auto& user : users) {
            user.titles.erase(std::remove_if(user.titles.begin(), user.titles.end(),
                [](const auto& title) {
                    return title.sourceKind == SelectedSourceKind::RetroArchFRLG;
                }), user.titles.end());
        }
        if (users.size() == 1 && users.front().titles.empty() &&
            users.front().name == "No users found") {
            users.front().name = "Game Sources";
        }

        // Discovery is app-global; normal visibility is not. A filesystem source has no intrinsic
        // Nintendo-account owner, so it appears only after an explicit persistent assignment.
        for (auto& user : users) {
            const auto cards = legacyBindings
                ? PokeVault::Legacy::buildFRLGSourceCardsForProfile(
                    legacySources, *legacyBindings, profileIdentity(user.uid))
                : std::vector<PokeVault::Legacy::FRLGSourceCard>{};
            user.titles.reserve(user.titles.size() + cards.size());
            for (const auto& card : cards) {
                TitleEntry entry;
                entry.name = "Pokemon " + card.title;
                entry.label = card.title;
                entry.gameId = card.gameId;
                entry.platformLabel = card.platformLabel;
                entry.sourceLabel = card.sourceLabel;
                entry.locationLabel = std::to_string(card.instances.size()) +
                    (card.instances.size() == 1 ? " SAVE" : " SAVES");
                entry.artworkKey = card.artworkKey;
                entry.legacyInstances = card.instances;
                entry.sourceKind = SelectedSourceKind::RetroArchFRLG;
                user.titles.push_back(std::move(entry));
            }
        }
        rebuildUnassignedLegacySources();
    }

    void SaveSelectScreen::loadGen4Cards() {
        for (auto& user : users) {
            user.titles.erase(std::remove_if(user.titles.begin(), user.titles.end(),
                [](const auto& title) {
                    return title.sourceKind == SelectedSourceKind::Gen4AssignedFile;
                }), user.titles.end());

            const std::string profile = profileIdentity(user.uid);
            for (const auto& game : PokeVault::Games::allGameDescriptors()) {
                if (game.platform != PokeVault::Games::Platform::NintendoDS ||
                    game.dataGeneration != 4 ||
                    game.support != PokeVault::Games::SourceSupport::ReadOnly) continue;

                TitleEntry entry;
                entry.name = "Pokemon " + std::string(game.title);
                entry.label = std::string(game.title);
                entry.gameId = std::string(game.id);
                entry.platformLabel = std::string(PokeVault::Games::platformName(game.platform));
                entry.artworkKey = entry.gameId;
                entry.sourceKind = SelectedSourceKind::Gen4AssignedFile;

                if (!legacyBindings) {
                    entry.sourceLabel = "CHOOSE SAVE";
                } else {
                    const auto assigned = legacyBindings->resolveFileForGame(profile, game.id);
                    switch (assigned.status) {
                        case PokeVault::Legacy::AssignedFileStatus::Ready:
                            entry.sourceLabel = "REMEMBERED";
                            entry.locationLabel = sourceLeafName(assigned.binding.sourcePath);
                            break;
                        case PokeVault::Legacy::AssignedFileStatus::Missing:
                            entry.sourceLabel = "MISSING";
                            entry.locationLabel = sourceLeafName(assigned.binding.sourcePath);
                            break;
                        case PokeVault::Legacy::AssignedFileStatus::Unreadable:
                            entry.sourceLabel = "INVALID";
                            entry.locationLabel = sourceLeafName(assigned.binding.sourcePath);
                            break;
                        case PokeVault::Legacy::AssignedFileStatus::Ambiguous:
                            entry.sourceLabel = "AMBIGUOUS";
                            break;
                        case PokeVault::Legacy::AssignedFileStatus::Unassigned:
                            entry.sourceLabel = "CHOOSE SAVE";
                            break;
                    }
                }
                user.titles.push_back(std::move(entry));
            }
        }
    }

    void SaveSelectScreen::rebuildUnassignedLegacySources() {
        unassignedLegacySources.clear();
        if (!legacyCatalog || !legacyBindings) return;
        for (const auto& card : PokeVault::Legacy::buildFRLGSourceCards(*legacyCatalog)) {
            for (auto instance : card.instances) {
                legacyBindings->applyClaims(instance);
                if (instance.claimConflict || !instance.claimedProfile.empty()) continue;
                unassignedLegacySources.push_back({card.gameId, card.title, instance});
            }
        }
        if (legacyAssignmentIndex >= static_cast<int>(unassignedLegacySources.size()))
            legacyAssignmentIndex = std::max(0,
                static_cast<int>(unassignedLegacySources.size()) - 1);
    }

    std::string SaveSelectScreen::currentProfileIdentity() const {
        const UserEntry* user = currentUser();
        return user ? profileIdentity(user->uid) : std::string{};
    }

    const PokeVault::Legacy::FRLGSaveInstance* SaveSelectScreen::currentLegacyInstance() const {
        const UserEntry* user = currentUser();
        if (!user || titleIndex < 0 || titleIndex >= static_cast<int>(user->titles.size()))
            return nullptr;
        const auto& title = user->titles[titleIndex];
        if (title.sourceKind != SelectedSourceKind::RetroArchFRLG || legacyInstanceIndex < 0 ||
            legacyInstanceIndex >= static_cast<int>(title.legacyInstances.size())) return nullptr;
        return &title.legacyInstances[static_cast<size_t>(legacyInstanceIndex)];
    }

    bool SaveSelectScreen::assignCurrentLegacySource() {
        if (!legacyBindings || legacyAssignmentIndex < 0 ||
            legacyAssignmentIndex >= static_cast<int>(unassignedLegacySources.size())) return false;
        const std::string profile = currentProfileIdentity();
        if (profile.empty()) return false;
        const auto& entry = unassignedLegacySources[static_cast<size_t>(legacyAssignmentIndex)];
        if (!legacyBindings->claimInstanceAndSave(entry.instance, profile)) {
            logErrorToFile("Legacy binding assignment failed", legacyBindings->lastError().c_str());
            legacyNotice = "Assignment could not be saved; source remains unassigned.";
            return false;
        }
        legacyNotice = sourceLeafName(entry.instance.location) + " assigned to this profile.";
        loadLegacySources(*legacyCatalog);
        overlay = Overlay::None;
        titleIndex = 0;
        scrollSelectionIntoView();
        return true;
    }

    bool SaveSelectScreen::refreshLegacySources(
        const std::string& gameId, const std::string& preferredSourceIdentity,
        bool requirePreferred) {
        if (!legacyCatalog) return false;
        auto refreshed = PokeVault::Legacy::discoverConfiguredLegacySaves();
        *legacyCatalog = std::move(refreshed);
        loadLegacySources(*legacyCatalog);

        logInfoToFile("Legacy active battery-save root",
            legacyCatalog->activeRoot.empty() ? "(none)" : legacyCatalog->activeRoot.c_str());
        const UserEntry* user = currentUser();
        if (!user) return false;
        const auto parent = std::find_if(user->titles.begin(), user->titles.end(),
            [&](const auto& title) {
                return title.sourceKind == SelectedSourceKind::RetroArchFRLG &&
                       title.gameId == gameId;
            });
        if (parent == user->titles.end()) {
            overlay = Overlay::None;
            legacyNotice = "No validated saves remain for this game.";
            titleIndex = std::min<int>(titleIndex,
                std::max<int>(0, static_cast<int>(user->titles.size()) - 1));
            scrollSelectionIntoView();
            return false;
        }

        titleIndex = static_cast<int>(std::distance(user->titles.begin(), parent));
        legacyInstanceIndex = 0;
        if (!preferredSourceIdentity.empty()) {
            const auto instance = std::find_if(parent->legacyInstances.begin(),
                parent->legacyInstances.end(), [&](const auto& candidate) {
                    return candidate.sourceIdentity == preferredSourceIdentity;
                });
            if (instance == parent->legacyInstances.end()) {
                legacyNotice = "That save changed location or was removed; nothing was opened.";
                overlay = Overlay::LegacyInstances;
                legacyInstanceScroll = 0;
                return !requirePreferred;
            }
            legacyInstanceIndex = static_cast<int>(
                std::distance(parent->legacyInstances.begin(), instance));
        }
        legacyInstanceScroll = std::max(0, legacyInstanceIndex - 5);
        legacyNotice = "Save list refreshed from configured emulator roots.";
        overlay = Overlay::LegacyInstances;
        scrollSelectionIntoView();
        return true;
    }

    void SaveSelectScreen::loadUsers() {
        users.clear();

        AccountUid userIds[ACC_USER_LIST_SIZE];
        s32 userCount = 0;
        Result rc = accountListAllUsers(userIds, ACC_USER_LIST_SIZE, &userCount);
        if (R_FAILED(rc)) {
            logErrorToFile("SaveSelect: failed to list users");
            userCount = 0;
        }

        for (s32 i = 0; i < userCount; i++) {
            UserEntry entry;
            entry.uid = userIds[i];

            AccountProfile profile;
            AccountProfileBase base;
            if (R_SUCCEEDED(accountGetProfile(&profile, userIds[i]))) {
                if (R_SUCCEEDED(accountProfileGet(&profile, NULL, &base))) {
                    entry.name = std::string(base.nickname);
                } else {
                    entry.name = "Unknown User";
                }
                accountProfileClose(&profile);
            } else {
                entry.name = "Unknown User";
            }

            loadTitlesForUser(entry);
            users.push_back(std::move(entry));
        }

        if (users.empty()) {
            UserEntry def;
            def.name = "No users found";
            memset(&def.uid, 0, sizeof(AccountUid));
            users.push_back(std::move(def));
        }
    }

    /**
     * List the Pokemon saves this user has, by enumerating SAVE DATA -- not installed titles.
     *
     * The distinction is the whole point. This used to walk `nsListApplicationRecord` and, for each
     * Pokemon title found, probe whether a save could be mounted. That asks "which games are
     * installed, and do they have saves?" when the only question a save editor cares about is
     * "which saves exist?" -- and the two differ in a case that is not rare at all:
     *
     *   A game played from a CARTRIDGE has no application record when the cart is out. Its save
     *   lives on internal storage and is perfectly editable, but the title vanishes from the picker
     *   the moment the cart is swapped for another game. An archived or partly-uninstalled title
     *   does the same.
     *
     * Enumerating save data finds those, because the OS lists the save whether or not anything is
     * currently installed to play it. It is also cheaper: no mount/unmount probe per title, which
     * was both slow and a devoptab slot churn.
     */
    // Scan one save-data space and append this user's Pokemon saves. Returns false only if the
    // space could not be opened at all; an empty space is a perfectly normal success.
    bool SaveSelectScreen::scanSaveSpace(UserEntry& user, int spaceId, int& scanned, int& forUser) {
        FsSaveDataInfoReader reader;
        Result rc = fsOpenSaveDataInfoReader(&reader, static_cast<FsSaveDataSpaceId>(spaceId));
        if (R_FAILED(rc)) {
            char m[112];
            snprintf(m, sizeof(m), "SaveSelect: cannot read save space %d (rc=0x%08X)", spaceId, (unsigned)rc);
            logInfoToFile(m);
            return false;
        }

        FsSaveDataInfo info[24];
        s64 readCount = 0;
        while (R_SUCCEEDED(fsSaveDataInfoReaderRead(&reader, info, 24, &readCount)) && readCount > 0) {
            for (s64 i = 0; i < readCount; i++) {
                scanned++;
                // Account saves only -- system/temporary/cache entries are not a player's save file.
                if (info[i].save_data_type != FsSaveDataType_Account) continue;
                if (memcmp(&info[i].uid, &user.uid, sizeof(AccountUid)) != 0) continue;
                forUser++;

                const u64 titleId = info[i].application_id;
                GameVersion gv = getGameVersion(titleId);
                if (gv == GameVersion::Invalid) continue;   // not a Pokemon title PKSE knows
                const auto* identity = PokeVault::Games::findSwitchGame(titleId);
                if (!identity) continue;                    // supported parser without a stable release identity

                // One tile per game. A title can report more than one save entry (save_data_index),
                // and scanning two spaces can see the same save twice -- listing a game twice would
                // be worse than useless.
                bool dup = false;
                for (const auto& t : user.titles) {
                    if (t.titleId == titleId) { dup = true; break; }
                }
                if (dup) continue;

                char m[128];
                snprintf(m, sizeof(m), "SaveSelect: %s (%016llX) -> listed",
                         getGameVersionName(gv).c_str(), (unsigned long long)titleId);
                logInfoToFile(m);

                TitleEntry t;
                t.titleId = titleId;
                t.label = getGameVersionName(gv);       // short: "Shield", "Legends: Z-A", ...
                t.name  = "Pokemon " + t.label;         // full name (backup dir + downstream compat)
                t.gameId = std::string(identity->id);
                t.platformLabel = std::string(PokeVault::Games::platformName(identity->platform));
                user.titles.push_back(std::move(t));
            }
        }
        fsSaveDataInfoReaderClose(&reader);
        return true;
    }

    void SaveSelectScreen::loadTitlesForUser(UserEntry& user) {
        int scanned = 0, forUser = 0;

        // `All` is the pseudo-space the header blesses for this reader, and it is what should
        // normally answer. Fall back to the concrete spaces if it is refused, because "found
        // nothing" is precisely the failure this function exists to stop producing.
        if (!scanSaveSpace(user, FsSaveDataSpaceId_All, scanned, forUser)) {
            scanSaveSpace(user, FsSaveDataSpaceId_User,   scanned, forUser);
            scanSaveSpace(user, FsSaveDataSpaceId_SdUser, scanned, forUser);
        }

        // Counts make a missing title diagnosable from the log alone: how many saves the console
        // reported in total, how many belong to this user, and how many were Pokemon titles.
        char summary[192];
        snprintf(summary, sizeof(summary),
                 "SaveSelect: %d save entries on console, %d for %s, %d Pokemon titles listed",
                 scanned, forUser, user.name.c_str(), (int)user.titles.size());
        logInfoToFile(summary);
    }

    const SaveSelectScreen::UserEntry* SaveSelectScreen::currentUser() const {
        if (users.empty()) return nullptr;
        return &users[userIndex];
    }

    int SaveSelectScreen::titleColumns() const {
        const UserEntry* u = currentUser();
        int count = u ? (int)u->titles.size() : 0;
        if (count <= 0) return 1;
        return count < MAX_COLS ? count : MAX_COLS;
    }

    // Rows of tiles, at MAX_COLS per row.
    int SaveSelectScreen::titleRows() const {
        const UserEntry* u = currentUser();
        const int count = u ? (int)u->titles.size() : 0;
        if (count <= 0) return 0;
        const int cols = titleColumns();
        return (count + cols - 1) / cols;
    }

    /**
     * Move the scroll window as LITTLE as possible to keep the selected tile on screen.
     *
     * `scrollRow` is deliberately STATE, not something recomputed from `titleIndex` each frame. The
     * derived version centred the selection -- `first = selRow - VISIBLE_ROWS/2` -- which reads as
     * paging: with three rows, selecting the bottom row shows rows 1-2, and moving back up to the
     * middle row snapped the view to rows 0-1 even though the middle row was *already visible*.
     * Every vertical move repainted the whole grid.
     *
     * Scrolling only when the selection would otherwise fall outside the window means the view
     * holds still while the cursor moves inside it, and shifts by exactly one row at the edges.
     * With three rows that is: nothing at all between the top two rows, one row down when you enter
     * the bottom row, and one row back up only when you leave the top row.
     */
    void SaveSelectScreen::scrollSelectionIntoView() {
        const int rows = titleRows();
        if (rows <= VISIBLE_ROWS) { scrollRow = 0; return; }   // everything fits; never offset

        const int selRow = titleIndex / titleColumns();
        if (selRow < scrollRow)                            scrollRow = selRow;
        else if (selRow >= scrollRow + VISIBLE_ROWS)       scrollRow = selRow - VISIBLE_ROWS + 1;

        // Clamp for the case the row count shrank under us (switching to a user with fewer saves).
        if (scrollRow > rows - VISIBLE_ROWS) scrollRow = rows - VISIBLE_ROWS;
        if (scrollRow < 0)                   scrollRow = 0;
    }

    void SaveSelectScreen::setUser(int idx) {
        if (users.empty()) return;
        userIndex = (idx % (int)users.size() + (int)users.size()) % (int)users.size();
        titleIndex = 0;
        scrollRow  = 0;   // a different user has a different title count; start at the top
    }

    void SaveSelectScreen::openGen4Setup(const std::string& gameId, std::string notice) {
        gen4TargetGameId = gameId;
        gen4Notice = std::move(notice);
        gen4Candidates.clear();
        gen4Instances.clear();
        gen4SetupIndex = 0;
        gen4CandidateIndex = 0;
        gen4CandidateScroll = 0;
        overlay = Overlay::Gen4Setup;
    }

    void SaveSelectScreen::discoverGen4Candidates() {
        gen4Candidates.clear();
        gen4Instances.clear();
        auto discovered = PokeVault::Integration::Gen4::discoverKnownSources();
        size_t wrappers = 0;
        size_t savestates = 0;
        std::string rememberedDiagnostic;

        auto appendReady = [&](PokeVault::Integration::Gen4::SourceCandidate candidate,
                               bool rememberedSource = false) {
            if (!candidate.ready() ||
                !PokeVault::Integration::Gen4::candidateMatchesGame(candidate, gen4TargetGameId))
                return;
            auto instance = PokeVault::Integration::Gen4::toSaveInstance(
                candidate, gen4TargetGameId, gen4Candidates.size(), rememberedSource,
                {});
            if (legacyBindings) legacyBindings->applyClaims(instance);
            if (!PokeVault::Source::visibleToProfile(instance, currentProfileIdentity()))
                return;
            if (!PokeVault::Source::appendDeduplicated(gen4Instances, std::move(instance)))
                return;
            gen4Candidates.push_back(std::move(candidate));
        };

        for (auto& candidate : discovered.candidates) {
            if (candidate.status == PokeVault::Integration::Gen4::CandidateStatus::UnsupportedWrapper)
                ++wrappers;
            else if (candidate.status == PokeVault::Integration::Gen4::CandidateStatus::UnsupportedSavestate)
                ++savestates;
            appendReady(std::move(candidate));
        }

        // A manually chosen source may live outside every known emulator root. Keep the remembered
        // assignment as one candidate in the chooser, but never let it skip the chooser.
        if (legacyBindings) {
            const auto remembered = legacyBindings->resolveFileForGame(
                currentProfileIdentity(), gen4TargetGameId);
            if (remembered.status == PokeVault::Legacy::AssignedFileStatus::Ready) {
                auto candidate = PokeVault::Integration::Gen4::inspectSourceFile(
                    remembered.binding.sourcePath,
                    remembered.binding.sourceType.empty() ? "Remembered" : remembered.binding.sourceType,
                    gen4TargetGameId);
                if (!candidate.ready())
                    rememberedDiagnostic = candidate.diagnostic;
                appendReady(std::move(candidate), true);
            } else if (remembered.status == PokeVault::Legacy::AssignedFileStatus::Missing) {
                rememberedDiagnostic = "Remembered save is missing; choose or discover another source.";
            } else if (remembered.status == PokeVault::Legacy::AssignedFileStatus::Unreadable) {
                rememberedDiagnostic = "Remembered save is unreadable; choose or discover another source.";
            } else if (remembered.status == PokeVault::Legacy::AssignedFileStatus::Ambiguous) {
                rememberedDiagnostic = "Remembered source assignment is ambiguous; choose a replacement.";
            }
        }

        // Match the classic chooser through the same provider-neutral ordering rule.
        PokeVault::Source::sortNewestFirst(gen4Instances);
        gen4CandidateIndex = 0;
        gen4CandidateScroll = 0;
        if (gen4Instances.empty()) {
            gen4Notice = !rememberedDiagnostic.empty() ? rememberedDiagnostic
                : discovered.limitReached
                    ? "No compatible save found before the bounded scan limit."
                : savestates > 0
                    ? "DraStic savestate found (.dss). PokeBank needs the cartridge save in /switch/drastic/user/backup/."
                : wrappers > 0
                    ? "Unsupported .dsv wrapper found. Valid footer-declared 0x80000 containers are supported read-only."
                    : "No compatible Gen IV cartridge save found in known emulator locations.";
            overlay = Overlay::Gen4Setup;
        } else {
            gen4Notice = std::to_string(gen4Instances.size()) +
                (gen4Instances.size() == 1 ? " validated save instance." : " validated save instances.");
            overlay = Overlay::Gen4Candidates;
        }
    }

    bool SaveSelectScreen::assignGen4Candidate(
        const PokeVault::Integration::Gen4::SourceCandidate& candidate) {
        if (!legacyBindings || !candidate.ready() ||
            !PokeVault::Integration::Gen4::candidateMatchesGame(candidate, gen4TargetGameId))
            return false;
        const std::string profile = currentProfileIdentity();
        if (profile.empty()) return false;
        const auto fresh = PokeVault::Integration::Gen4::inspectSourceFile(
            candidate.path, candidate.sourceType, gen4TargetGameId);
        const auto shownInstance = PokeVault::Integration::Gen4::toSaveInstance(candidate, gen4TargetGameId);
        auto freshInstance = PokeVault::Integration::Gen4::toSaveInstance(fresh, gen4TargetGameId);
        legacyBindings->applyClaims(freshInstance);
        if (!PokeVault::Source::sameValidatedSnapshot(shownInstance, freshInstance) ||
            !PokeVault::Source::visibleToProfile(freshInstance, profile)) {
            discoverGen4Candidates();
            gen4Notice = "That save changed or is no longer available to this profile. Review the refreshed list.";
            return false;
        }

        PokeVault::Legacy::BindingRecord binding;
        binding.profileIdentity = profile;
        binding.gameIdentity = gen4TargetGameId;
        binding.sourcePath = candidate.path;
        binding.sourceType = candidate.sourceType;
        binding.expectedRawFamily = candidate.expectedRawFamily;
        if (!legacyBindings->replaceFileAssignmentAndSave(candidate.sourceIdentity, std::move(binding))) {
            gen4Notice = legacyBindings->lastError().find("conflict") != std::string::npos
                ? "That physical save is already remembered by another profile or game. Forget it there first."
                : "The save assignment could not be stored safely.";
            overlay = Overlay::Gen4Setup;
            return false;
        }

        loadGen4Cards();
        overlay = Overlay::None;
        gen4Notice.clear();
        selectAssignedGen4Title();
        return titleSelected;
    }

    void SaveSelectScreen::chooseGen4ManualFile() {
        const auto chosen = Utils::promptText(
            "Choose Generation IV Save",
            "Full SD path to a raw cartridge save (.sav/.srm/.dsv; not .dss)",
            "", 240);
        if (!chosen.accepted) return;

        auto candidate = PokeVault::Integration::Gen4::inspectSourceFile(
            chosen.text, "Manual", gen4TargetGameId);
        if (!candidate.ready()) {
            gen4Notice = candidate.diagnostic.empty()
                ? "That file is not a compatible save for this game card."
                : candidate.diagnostic;
            overlay = Overlay::Gen4Setup;
            return;
        }
        assignGen4Candidate(candidate);
    }

    bool SaveSelectScreen::unassignCurrentGen4Game() {
        if (!legacyBindings) return false;
        const std::string profile = currentProfileIdentity();
        if (!legacyBindings->unassignGameAndSave(profile, gen4TargetGameId)) {
            gen4Notice = "No remembered save could be removed for this game.";
            return false;
        }
        loadGen4Cards();
        gen4Notice = "Remembered save removed. The source file itself was not changed.";
        overlay = Overlay::Gen4Setup;
        return true;
    }

    void SaveSelectScreen::selectAssignedGen4Title() {
        const UserEntry* user = currentUser();
        if (!user) return;
        const auto found = std::find_if(user->titles.begin(), user->titles.end(),
            [&](const auto& title) {
                return title.sourceKind == SelectedSourceKind::Gen4AssignedFile &&
                       title.gameId == gen4TargetGameId;
            });
        if (found == user->titles.end()) return;
        titleIndex = static_cast<int>(std::distance(user->titles.begin(), found));
        const auto opened = legacyBindings
            ? PokeVault::Integration::Gen4::openAssignedSource(
                *legacyBindings, currentProfileIdentity(), found->gameId)
            : PokeVault::Integration::Gen4::AssignedSourceReadOnly{};
        if (opened.status != PokeVault::Integration::Gen4::OpenStatus::Ready) {
            openGen4Setup(found->gameId, opened.diagnostic);
            return;
        }
        selectedUserUid = user->uid;
        selectedTitleId = 0;
        selectedTitleName = found->name;
        selectedGameId = found->gameId;
        selectedSourceKind = found->sourceKind;
        titleSelected = true;
    }

    void SaveSelectScreen::selectCurrentTitle() {
        const UserEntry* u = currentUser();
        if (!u || titleIndex < 0 || titleIndex >= (int)u->titles.size()) return;
        const auto& title = u->titles[titleIndex];
        if (title.sourceKind == SelectedSourceKind::RetroArchFRLG) {
            const std::string gameId = title.gameId;
            refreshLegacySources(gameId);
            return;
        }
        if (title.sourceKind == SelectedSourceKind::Gen4AssignedFile) {
            // Never auto-open a remembered adapter path. Gen IV now matches the classic source UX:
            // game identity first, then an explicit validated Save Instances choice every time.
            gen4TargetGameId = title.gameId;
            discoverGen4Candidates();
            return;
        }
        selectedUserUid  = u->uid;
        selectedTitleId  = title.titleId;
        selectedTitleName = title.name;
        selectedGameId = title.gameId;
        selectedSourceKind = title.sourceKind;
        titleSelected = true;
    }

    void SaveSelectScreen::selectCurrentLegacyInstance() {
        const UserEntry* u = currentUser();
        if (!u || titleIndex < 0 || titleIndex >= static_cast<int>(u->titles.size())) return;
        const auto& title = u->titles[titleIndex];
        if (title.sourceKind != SelectedSourceKind::RetroArchFRLG ||
            legacyInstanceIndex < 0 ||
            legacyInstanceIndex >= static_cast<int>(title.legacyInstances.size())) return;
        // Reread the active physical root at the selection boundary. If this child was replaced or
        // deleted while the picker was open, the stale instance can no longer resolve to anything.
        const std::string gameId = title.gameId;
        const auto shownInstance = title.legacyInstances[static_cast<size_t>(legacyInstanceIndex)];
        const std::string sourceIdentity = shownInstance.sourceIdentity;
        if (!refreshLegacySources(gameId, sourceIdentity, true)) return;
        u = currentUser();
        if (!u || titleIndex < 0 || titleIndex >= static_cast<int>(u->titles.size())) return;
        const auto& refreshedTitle = u->titles[titleIndex];
        if (legacyInstanceIndex < 0 ||
            legacyInstanceIndex >= static_cast<int>(refreshedTitle.legacyInstances.size())) return;

        const auto& freshInstance = refreshedTitle.legacyInstances[static_cast<size_t>(legacyInstanceIndex)];
        if (!PokeVault::Source::sameValidatedSnapshot(shownInstance, freshInstance)) {
            legacyNotice = "That save changed. Review the refreshed list before opening.";
            overlay = Overlay::LegacyInstances;
            return;
        }
        selectedUserUid = u->uid;
        selectedTitleId = 0;
        selectedTitleName = refreshedTitle.name;
        selectedGameId = refreshedTitle.gameId;
        selectedSourceKind = refreshedTitle.sourceKind;
        selectedLegacySourceIndex =
            refreshedTitle.legacyInstances[static_cast<size_t>(legacyInstanceIndex)].sourceIndex;
        titleSelected = true;
    }

    void SaveSelectScreen::update(const PadState& pad, const TouchInput& touch) {
        // A tap on a nav-bar badge becomes that button's press, so every handler below is
        // reached identically whether the user pressed the button or tapped its on-screen badge.
        const HidAnalogStickState stick = padGetStickPos(&pad, 0);
        u64 kDown = controllerNavigation.apply(
            padGetButtonsDown(&pad), padGetButtons(&pad), stick.x, stick.y,
            HidNpadButton_Up, HidNpadButton_Down, HidNpadButton_Left, HidNpadButton_Right)
            | navTouchButton(touch);

        if (overlay == Overlay::Help) {
            if (kDown & (HidNpadButton_B | HidNpadButton_Minus)) overlay = Overlay::None;
            return;
        }
        if (overlay == Overlay::LegacyDetails) {
            if (kDown & (HidNpadButton_B | HidNpadButton_Y)) overlay = Overlay::LegacyInstances;
            return;
        }
        if (overlay == Overlay::LegacyAssignment) {
            const int count = static_cast<int>(unassignedLegacySources.size());
            if (kDown & HidNpadButton_B) { overlay = Overlay::None; return; }
            if (kDown & HidNpadButton_X) {
                if (legacyCatalog) {
                    *legacyCatalog = PokeVault::Legacy::discoverConfiguredLegacySaves();
                    loadLegacySources(*legacyCatalog);
                    legacyNotice = "Unassigned source list refreshed.";
                }
                if (unassignedLegacySources.empty()) overlay = Overlay::None;
                return;
            }
            if (count == 0) { overlay = Overlay::None; return; }
            if (kDown & HidNpadButton_Up)
                legacyAssignmentIndex = (legacyAssignmentIndex - 1 + count) % count;
            if (kDown & HidNpadButton_Down)
                legacyAssignmentIndex = (legacyAssignmentIndex + 1) % count;
            constexpr int visibleRows = 5;
            if (legacyAssignmentIndex < legacyAssignmentScroll)
                legacyAssignmentScroll = legacyAssignmentIndex;
            else if (legacyAssignmentIndex >= legacyAssignmentScroll + visibleRows)
                legacyAssignmentScroll = legacyAssignmentIndex - visibleRows + 1;
            if (kDown & HidNpadButton_A) assignCurrentLegacySource();
            return;
        }
        if (overlay == Overlay::Gen4Setup) {
            if (kDown & HidNpadButton_B) { overlay = Overlay::None; return; }
            if (kDown & HidNpadButton_Up) gen4SetupIndex = (gen4SetupIndex + 3) % 4;
            if (kDown & HidNpadButton_Down) gen4SetupIndex = (gen4SetupIndex + 1) % 4;
            if (kDown & HidNpadButton_A) {
                if (gen4SetupIndex == 0) discoverGen4Candidates();
                else if (gen4SetupIndex == 1) chooseGen4ManualFile();
                else if (gen4SetupIndex == 2) unassignCurrentGen4Game();
                else overlay = Overlay::None;
            }
            return;
        }
        if (overlay == Overlay::Gen4Candidates) {
            const int count = static_cast<int>(gen4Instances.size());
            if (kDown & HidNpadButton_B) { overlay = Overlay::None; return; }
            if (kDown & HidNpadButton_X) { discoverGen4Candidates(); return; }
            if (kDown & HidNpadButton_Y) {
                openGen4Setup(gen4TargetGameId,
                    "Add, repair, or forget a remembered source. Opening still happens from Save Instances.");
                return;
            }
            if (count == 0) { overlay = Overlay::Gen4Setup; return; }
            if (kDown & HidNpadButton_Up)
                gen4CandidateIndex = (gen4CandidateIndex - 1 + count) % count;
            if (kDown & HidNpadButton_Down)
                gen4CandidateIndex = (gen4CandidateIndex + 1) % count;
            constexpr int visibleRows = 5;
            if (gen4CandidateIndex < gen4CandidateScroll)
                gen4CandidateScroll = gen4CandidateIndex;
            else if (gen4CandidateIndex >= gen4CandidateScroll + visibleRows)
                gen4CandidateScroll = gen4CandidateIndex - visibleRows + 1;
            if (kDown & HidNpadButton_A) {
                const size_t handle =
                    gen4Instances[static_cast<size_t>(gen4CandidateIndex)].sourceIndex;
                if (handle < gen4Candidates.size())
                    assignGen4Candidate(gen4Candidates[handle]);
            }
            return;
        }

        if (overlay == Overlay::Options) {
            if (kDown & HidNpadButton_Up)   optionsIndex = (optionsIndex + 2) % 3;
            if (kDown & HidNpadButton_Down) optionsIndex = (optionsIndex + 1) % 3;
            if (kDown & HidNpadButton_B) { overlay = Overlay::None; return; }
            if (kDown & HidNpadButton_A) {
                if (optionsIndex == 0) {
                    applyTheme(nextThemeMode(g_themeMode));
                    Utils::saveSettings();
                } else if (optionsIndex == 1) {
                    exitRequested = true;
                } else {
                    overlay = Overlay::None;
                }
            }
            return;
        }
        if (overlay == Overlay::LegacyInstances) {
            const UserEntry* u = currentUser();
            if (!u || titleIndex < 0 || titleIndex >= static_cast<int>(u->titles.size())) {
                overlay = Overlay::None;
                return;
            }
            const auto& instances = u->titles[titleIndex].legacyInstances;
            const int count = static_cast<int>(instances.size());
            if (kDown & HidNpadButton_B) {
                overlay = Overlay::None;
                return;
            }
            if (kDown & HidNpadButton_X) {
                const std::string gameId = u->titles[titleIndex].gameId;
                std::string preferred;
                if (legacyInstanceIndex >= 0 && legacyInstanceIndex < count)
                    preferred = instances[static_cast<size_t>(legacyInstanceIndex)].sourceIdentity;
                refreshLegacySources(gameId, preferred, false);
                return;
            }
            if (kDown & HidNpadButton_Y) {
                const auto* instance = currentLegacyInstance();
                if (instance) {
                    legacyDetailsInstance = *instance;
                    legacyDetailsGameId = u->titles[titleIndex].gameId;
                    overlay = Overlay::LegacyDetails;
                }
                return;
            }
            if (count == 0) {
                overlay = Overlay::None;
                return;
            }
            if (kDown & HidNpadButton_Up)
                legacyInstanceIndex = (legacyInstanceIndex - 1 + count) % count;
            if (kDown & HidNpadButton_Down)
                legacyInstanceIndex = (legacyInstanceIndex + 1) % count;
            constexpr int visibleRows = 6;
            if (legacyInstanceIndex < legacyInstanceScroll)
                legacyInstanceScroll = legacyInstanceIndex;
            else if (legacyInstanceIndex >= legacyInstanceScroll + visibleRows)
                legacyInstanceScroll = legacyInstanceIndex - visibleRows + 1;
            if (kDown & HidNpadButton_A) selectCurrentLegacyInstance();
            return;
        }

        if (kDown & HidNpadButton_Plus) {
            overlay = Overlay::Options;
            optionsIndex = 0;
            return;
        }
        if (kDown & HidNpadButton_Minus) {
            overlay = Overlay::Help;
            return;
        }
        {
            const UserEntry* current = currentUser();
            if ((kDown & HidNpadButton_Y) && current && titleIndex >= 0 &&
                titleIndex < static_cast<int>(current->titles.size()) &&
                current->titles[titleIndex].sourceKind == SelectedSourceKind::Gen4AssignedFile) {
                openGen4Setup(current->titles[titleIndex].gameId,
                              "Add, repair, or forget a remembered source for this game.");
                return;
            }
        }
        if ((kDown & HidNpadButton_X) && !unassignedLegacySources.empty()) {
            overlay = Overlay::LegacyAssignment;
            legacyAssignmentIndex = 0;
            legacyAssignmentScroll = 0;
            legacyNotice.clear();
            return;
        }

        // Touch (tap targets were captured last draw()).
        if (touch.justPressed()) {
            const int tx = touch.x(), ty = touch.y();
            bool handled = false;
            for (const auto& r : userRects) {
                if (tx >= r.x && tx < r.x + r.w && ty >= r.y && ty < r.y + r.h) {
                    setUser(r.idx); handled = true; break;
                }
            }
            if (!handled) {
                for (const auto& r : titleRects) {
                    if (tx >= r.x && tx < r.x + r.w && ty >= r.y && ty < r.y + r.h) {
                        titleIndex = r.idx; kDown |= HidNpadButton_A; break;
                    }
                }
            }
        }

        // Switch users with the shoulder buttons (only when there's more than one).
        if (users.size() > 1) {
            if (kDown & HidNpadButton_L) setUser(userIndex - 1);
            if (kDown & HidNpadButton_R) setUser(userIndex + 1);
        }

        // Title grid navigation.
        const UserEntry* u = currentUser();
        int count = u ? (int)u->titles.size() : 0;
        if (count > 0) {
            int cols = titleColumns();
            if (kDown & HidNpadButton_Left)  titleIndex = (titleIndex - 1 + count) % count;
            if (kDown & HidNpadButton_Right) titleIndex = (titleIndex + 1) % count;
            if (kDown & HidNpadButton_Down) {
                // A partial last row still has to be reachable. Straight down when that column
                // exists below; otherwise fall to the final tile rather than refusing to move --
                // with twelve titles the bottom row is just two wide, and Down from the right-hand
                // columns would otherwise do nothing at all.
                if (titleIndex + cols < count)                        titleIndex += cols;
                else if (titleIndex / cols < (count - 1) / cols)      titleIndex = count - 1;
            }
            if ((kDown & HidNpadButton_Up)   && titleIndex - cols >= 0)    titleIndex -= cols;
            if (kDown & HidNpadButton_A) selectCurrentTitle();
            // One place, after every way the selection can move -- stick, D-pad or a tap.
            scrollSelectionIntoView();
        }

    }

    void SaveSelectScreen::draw(PKSEFramebuffer& fb) {
        titleRects.clear();
        userRects.clear();

        drawAppBackdrop(fb);
        drawTitleBar(fb, "Game Sources  /  v" + VERSION_STRING + "  /  " + BUILD_COMMIT);

        const UserEntry* u = currentUser();

        // ---- User header card ----
        drawPanelSurface(fb, 24, HEADER_Y, fb.getWidth() - 48, HEADER_H, true);

        const int avX = 44, avY = HEADER_Y + (HEADER_H - AVATAR) / 2;
        if (u) {
            const IconImage* av = u->name == "Game Sources" ? nullptr
                                                             : &SystemIcons::userIcon(u->uid);
            if (av && av->valid())
                fb.drawImageScaled(avX, avY, av->width, av->height, AVATAR, AVATAR, av->data, 4);
            else {
                fb.drawFilledRoundedRect(avX, avY, AVATAR, AVATAR, 10, Colors::PanelAlt);
                if (u->name == "Game Sources") {
                    int rw, rh;
                    fb.measureText("PB", rw, rh, TextStyle::Heading);
                    fb.drawText(avX + (AVATAR - rw) / 2, avY + (AVATAR - rh) / 2,
                                "PB", Colors::Accent, TextStyle::Heading);
                }
            }
            fb.drawRoundedRect(avX, avY, AVATAR, AVATAR, 10, Colors::Border, 1);
        }

        const int nameX = avX + AVATAR + 20;
        if (u) {
            int nlh = fb.lineHeight(TextStyle::Title);
            fb.drawText(nameX, avY + 6, u->name, Colors::Text, TextStyle::Title);
            int cnt = (int)u->titles.size();
            std::string sub = std::to_string(cnt) +
                (cnt == 1 ? " available game source" : " available game sources");
            fb.drawText(nameX, avY + 6 + nlh + 4, sub, Colors::TextDim, TextStyle::Caption);
        }

        // Per-user switcher chips (only when more than one account).
        if (users.size() > 1) {
            int n = (int)users.size();
            int totalW = n * CHIP + (n - 1) * 12;
            int startX = fb.getWidth() - 40 - totalW;
            int chipY = HEADER_Y + (HEADER_H - CHIP) / 2;
            for (int i = 0; i < n; i++) {
                int cx = startX + i * (CHIP + 12);
                const IconImage& av = SystemIcons::userIcon(users[i].uid);
                if (av.valid())
                    fb.drawImageScaled(cx, chipY, av.width, av.height, CHIP, CHIP, av.data, 4);
                else
                    fb.drawFilledRoundedRect(cx, chipY, CHIP, CHIP, 8, Colors::PanelAlt);
                if (i == userIndex) fb.drawRoundedRect(cx, chipY, CHIP, CHIP, 8, Colors::FocusBorder, 3);
                else                fb.drawRoundedRect(cx, chipY, CHIP, CHIP, 8, Colors::Border, 1);
                userRects.push_back({cx, chipY, CHIP, CHIP, i});
            }
        }

        // ---- Title grid ----
        int count = u ? (int)u->titles.size() : 0;
        if (count == 0) {
            const char* msg = (!u || u->name == "Game Sources")
                ? "No validated Pokémon game sources found"
                : "No Pokémon saves found for this user";
            int mw, mh; fb.measureText(msg, mw, mh, TextStyle::Body);
            fb.drawText((fb.getWidth() - mw) / 2, GRID_Y + 120, msg, Colors::TextDim, TextStyle::Body);
        } else {
            const int cols  = titleColumns();
            const int rows  = titleRows();
            const int first = scrollRow;
            const int gridW = cols * TILE_W + (cols - 1) * GAP;
            int startX = (fb.getWidth() - gridW) / 2;
            if (startX < 40) startX = 40;

            // Only the rows inside the window are drawn, and only those get tap targets -- an
            // off-screen tile must not be tappable through whatever is covering it.
            const int firstIdx = first * cols;
            const int lastIdx  = (first + VISIBLE_ROWS) * cols;   // exclusive

            for (int i = firstIdx; i < count && i < lastIdx; i++) {
                int col = i % cols, row = (i / cols) - first;
                int tileX = startX + col * (TILE_W + GAP);
                int tileY = GRID_Y + row * (TILE_H + GAP);
                bool sel = (i == titleIndex);

                drawFocusedCard(fb, tileX, tileY, TILE_W, TILE_H, sel, 16);

                int iconX = tileX + (TILE_W - ICON) / 2;
                int iconY = tileY + 16;
                const bool legacy = u->titles[i].sourceKind == SelectedSourceKind::RetroArchFRLG;
                const IconImage& ic = SystemIcons::gameCardIcon(
                    legacy ? u->titles[i].artworkKey : u->titles[i].gameId,
                    u->titles[i].titleId);
                if (ic.valid())
                    fb.drawImageScaled(iconX, iconY, ic.width, ic.height, ICON, ICON, ic.data, 4);
                else {
                    fb.drawFilledRoundedRect(iconX, iconY, ICON, ICON, 10, Colors::PanelAlt);
                    if (legacy) {
                        const std::string mark = "GBA";
                        int mw, mh; fb.measureText(mark, mw, mh, TextStyle::Heading);
                        fb.drawText(iconX + (ICON - mw) / 2, iconY + (ICON - mh) / 2,
                                    mark, Colors::Accent, TextStyle::Heading);
                    }
                }

                // Source/provider details intentionally stay off the cover art. Save Instances is
                // the authoritative place to choose and inspect validated physical sources.

                // Release name and platform are separate lines. A FireRed tile must always say
                // whether it is the GBA or Switch release; display names are never identity.
                const std::string& label = u->titles[i].label;
                int lw, lh; fb.measureText(label, lw, lh, TextStyle::Caption);
                fb.drawText(tileX + (TILE_W - lw) / 2, iconY + ICON + 8, label,
                            sel ? Colors::Text : Colors::TextDim, TextStyle::Caption);
                const std::string& platform = u->titles[i].platformLabel;
                int pw, ph; fb.measureText(platform, pw, ph, TextStyle::Caption);
                fb.drawText(tileX + (TILE_W - pw) / 2, iconY + ICON + 8 + lh + 1, platform,
                            Colors::TextDim, TextStyle::Caption);
                // Per-card source counts/paths are deliberately omitted here so every generation
                // follows one rule. Save Instances owns source count, provider and path metadata.

                titleRects.push_back({tileX, tileY, TILE_W, TILE_H, i});
            }

            // Scrollbar to the right of the grid, drawn only when the rows overflow -- same thumb
            // the backup list and the details editor use.
            constexpr int ROW_PITCH = TILE_H + GAP;
            drawScrollbar(fb, startX + gridW + 12, GRID_Y, VISIBLE_ROWS * ROW_PITCH,
                          rows * ROW_PITCH, first * ROW_PITCH);
        }

        // ---- Footer ----
        auto homeHints = controllerHints(users.size() > 1
            ? PokeBank::UIModel::ControllerContext::SelectGameMultipleUsers
            : PokeBank::UIModel::ControllerContext::SelectGame);
        if (!unassignedLegacySources.empty())
            homeHints.push_back({"X", "Assign Legacy Save"});
        if (u && titleIndex >= 0 && titleIndex < static_cast<int>(u->titles.size()) &&
            u->titles[titleIndex].sourceKind == SelectedSourceKind::Gen4AssignedFile)
            homeHints.push_back({"Y", "Source Setup"});
        drawNavBar(fb, homeHints);

        if (overlay == Overlay::LegacyInstances && u && titleIndex >= 0 &&
            titleIndex < static_cast<int>(u->titles.size())) {
            const auto& parent = u->titles[titleIndex];
            constexpr int w = 780, h = 530, rowH = 66, visibleRows = 5;
            const int x = (fb.getWidth() - w) / 2, y = (fb.getHeight() - h) / 2;
            drawModalSurface(fb, x, y, w, h);
            fb.drawText(x + 28, y + 18,
                        "SAVE INSTANCES / " + parent.platformLabel + " / READ ONLY",
                        Colors::Accent, TextStyle::Caption);
            fb.drawText(x + 28, y + 44, "Pokémon " + parent.label + " — Save Instances",
                        Colors::TextPrimary, TextStyle::Heading);
            fb.drawText(x + 28, y + 76,
                        "Choose a validated battery save. The source file will not be modified.",
                        Colors::TextSecondary, TextStyle::Caption);
            fb.drawText(x + 28, y + 96, providerSummary(parent.legacyInstances),
                        Colors::TextMuted, TextStyle::Caption);

            const int first = legacyInstanceScroll;
            drawSaveInstanceRows(fb, parent.legacyInstances, legacyInstanceIndex, first,
                                 x, y + 122, w, rowH, visibleRows, true);
            if (!legacyNotice.empty())
                fb.drawText(x + 28, y + h - 38, legacyNotice, Colors::TextMuted,
                            TextStyle::Caption);
            drawNavBar(fb, {{"Up/Down", "Choose Save"}, {"A", "Open Read Only"},
                            {"Y", "Source Details"}, {"X", "Refresh Saves"}, {"B", "Back"}});
        } else if (overlay == Overlay::LegacyAssignment && u) {
            constexpr int w = 800, h = 530, rowH = 70, visibleRows = 5;
            const int x = (fb.getWidth() - w) / 2, y = (fb.getHeight() - h) / 2;
            drawModalSurface(fb, x, y, w, h);
            fb.drawText(x + 28, y + 18, "SAVE SOURCE / EXPLICIT PROFILE ASSIGNMENT",
                        Colors::Accent, TextStyle::Caption);
            fb.drawText(x + 28, y + 44, "Assign a Legacy Save to " + u->name,
                        Colors::TextPrimary, TextStyle::Heading);
            fb.drawText(x + 28, y + 76,
                        "Assigning claims this save for this profile and hides it from other profiles.",
                        Colors::TextSecondary, TextStyle::Caption);
            const int first = legacyAssignmentScroll;
            const int last = std::min<int>(static_cast<int>(unassignedLegacySources.size()),
                                           first + visibleRows);
            int rowY = y + 108;
            for (int index = first; index < last; ++index) {
                const auto& entry = unassignedLegacySources[static_cast<size_t>(index)];
                drawFocusedCard(fb, x + 24, rowY, w - 48, rowH - 6,
                                index == legacyAssignmentIndex, 10);
                fb.drawText(x + 44, rowY + 7, "Pokémon " + entry.title + " — " +
                            entry.instance.label,
                            index == legacyAssignmentIndex ? Colors::TextPrimary
                                                           : Colors::TextSecondary,
                            TextStyle::Body);
                fb.drawText(x + 44, rowY + 35,
                            (entry.instance.providerLabel.empty() ? std::string("Source")
                                                                 : entry.instance.providerLabel) +
                                " / " + entry.instance.sourceLabel,
                            Colors::TextMuted, TextStyle::Caption);
                rowY += rowH;
            }
            if (!legacyNotice.empty())
                fb.drawText(x + 28, y + h - 36, legacyNotice, Colors::TextMuted,
                            TextStyle::Caption);
            drawNavBar(fb, {{"Up/Down", "Choose"}, {"A", "Assign to This Profile"},
                            {"X", "Refresh"}, {"B", "Cancel"}});
        } else if (overlay == Overlay::LegacyDetails) {
            constexpr int w = 900, h = 520;
            const int x = (fb.getWidth() - w) / 2, y = (fb.getHeight() - h) / 2;
            drawModalSurface(fb, x, y, w, h);
            const std::string detailsProvider = legacyDetailsInstance.providerLabel.empty()
                ? std::string("SOURCE") : legacyDetailsInstance.providerLabel;
            fb.drawText(x + 28, y + 18,
                        detailsProvider + " / SOURCE DIAGNOSTICS / READ ONLY",
                        Colors::Accent, TextStyle::Caption);
            fb.drawText(x + 28, y + 44, legacyDetailsInstance.label,
                        Colors::TextPrimary, TextStyle::Heading);
            int lineY = y + 88;
            auto drawLine = [&](const std::string& label, const std::string& value) {
                fb.drawText(x + 32, lineY, label, Colors::TextMuted, TextStyle::Caption);
                fb.drawText(x + 214, lineY, value, Colors::TextPrimary, TextStyle::Caption);
                lineY += 30;
            };
            drawLine("Provider", legacyDetailsInstance.providerLabel.empty()
                ? "Source" : legacyDetailsInstance.providerLabel);
            drawLine("Game identity", legacyDetailsGameId);
            drawLine("Trainer", legacyDetailsInstance.trainerName.empty()
                ? "Unknown" : legacyDetailsInstance.trainerName);
            drawLine("Party count", std::to_string(legacyDetailsInstance.partyCount));
            drawLine("File size", std::to_string(legacyDetailsInstance.fileSize) + " bytes");
            drawLine("Modified state", std::to_string(legacyDetailsInstance.modifiedTime));
            drawLine("Source identity", shortValue(legacyDetailsInstance.sourceIdentity, 24));
            drawLine("Content SHA-256", shortValue(legacyDetailsInstance.contentFingerprint, 24));
            const std::string& path = legacyDetailsInstance.normalizedPath;
            drawLine("Physical path", path.substr(0, std::min<size_t>(70, path.size())));
            for (size_t offset = 70; offset < path.size() && offset < 210; offset += 70)
                drawLine("", path.substr(offset, 70));
            drawNavBar(fb, {{"B", "Back to Save List"}});
        } else if (overlay == Overlay::Gen4Setup && u) {
            constexpr int w = 760, h = 430, rowH = 64;
            const int x = (fb.getWidth() - w) / 2, y = (fb.getHeight() - h) / 2;
            drawModalSurface(fb, x, y, w, h);
            const auto* identity = PokeVault::Games::findGame(gen4TargetGameId);
            const std::string title = identity ? std::string(identity->title) : std::string("Generation IV");
            fb.drawText(x + 28, y + 18, "NINTENDO DS / SOURCE SETUP / READ ONLY",
                        Colors::Accent, TextStyle::Caption);
            fb.drawText(x + 28, y + 44, "Pokémon " + title,
                        Colors::TextPrimary, TextStyle::Heading);
            fb.drawText(x + 28, y + 76,
                        "Add or repair sources here. Opening always happens from Save Instances.",
                        Colors::TextSecondary, TextStyle::Caption);
            const std::string rows[4] = {
                "Refresh Known Emulator Saves",
                "Choose Save File Manually",
                "Forget Remembered Save",
                "Cancel"
            };
            int ry = y + 112;
            for (int i = 0; i < 4; ++i) {
                drawFocusedCard(fb, x + 24, ry, w - 48, rowH - 8, i == gen4SetupIndex, 10);
                fb.drawText(x + 44, ry + 15, rows[i],
                            i == gen4SetupIndex ? Colors::TextPrimary : Colors::TextSecondary);
                ry += rowH;
            }
            if (!gen4Notice.empty())
                fb.drawText(x + 28, y + h - 42, gen4Notice.substr(0, 100),
                            Colors::TextMuted, TextStyle::Caption);
            drawNavBar(fb, {{"Up/Down", "Choose"}, {"A", "Select"}, {"B", "Back"}});
        } else if (overlay == Overlay::Gen4Candidates && u) {
            constexpr int w = 900, h = 540, rowH = 76, visibleRows = 5;
            const int x = (fb.getWidth() - w) / 2, y = (fb.getHeight() - h) / 2;
            drawModalSurface(fb, x, y, w, h);
            const auto* identity = PokeVault::Games::findGame(gen4TargetGameId);
            const std::string title = identity ? std::string(identity->title) : std::string("Generation IV");
            fb.drawText(x + 28, y + 18, "NINTENDO DS / SAVE INSTANCES / READ ONLY",
                        Colors::Accent, TextStyle::Caption);
            fb.drawText(x + 28, y + 44, "Pokémon " + title + " — Save Instances",
                        Colors::TextPrimary, TextStyle::Heading);
            fb.drawText(x + 28, y + 76,
                        "Choose a validated cartridge save. The source file will not be modified.",
                        Colors::TextSecondary, TextStyle::Caption);

            const int first = gen4CandidateScroll;
            drawSaveInstanceRows(fb, gen4Instances, gen4CandidateIndex, first,
                                 x, y + 108, w, rowH, visibleRows, false);
            if (!gen4Notice.empty())
                fb.drawText(x + 28, y + h - 34, gen4Notice, Colors::TextMuted, TextStyle::Caption);
            drawNavBar(fb, {{"Up/Down", "Choose Save"}, {"A", "Open Read Only"},
                            {"Y", "Source Setup"}, {"X", "Refresh Saves"}, {"B", "Back"}});
        } else if (overlay == Overlay::Help) {
            drawInfoOverlay(fb, "Game Sources & Controls", {
                "D-pad / Left Stick   Navigate (hold to scroll)",
                "A   Open the focused game and choose a save instance",
                "L / R   Previous or next Switch user",
                "X   Assign an unassigned legacy save to this profile",
                "Y   Add or repair sources for the focused Gen IV game",
                "+   Options and appearance",
                "-   Help for the current screen"
            });
        } else if (overlay == Overlay::Options) {
            constexpr int w = 560, h = 326, rowH = 64;
            const int x = (fb.getWidth() - w) / 2, y = (fb.getHeight() - h) / 2;
            drawModalSurface(fb, x, y, w, h);
            fb.drawText(x + 26, y + 16, "POKEBANK NX  /  OPTIONS", Colors::AccentPrimary,
                        TextStyle::Caption);
            fb.drawText(x + 26, y + 40, "Quick Options", Colors::TextPrimary, TextStyle::Heading);
            const std::string rows[3] = {
                "Theme: " + std::string(themeModeName(g_themeMode)),
                "Exit PokeBank NX",
                "Cancel"
            };
            int ry = y + 92;
            for (int i = 0; i < 3; ++i) {
                drawFocusedCard(fb, x + 22, ry, w - 44, rowH - 8, i == optionsIndex, 12);
                fb.drawText(x + 44, ry + 15, rows[i],
                            i == optionsIndex ? Colors::TextPrimary : Colors::TextSecondary);
                ry += rowH;
            }
            drawNavBar(fb, {{"Up/Down", "Choose"}, {"A", "Select"}, {"B", "Cancel"}});
        }
    }
}
