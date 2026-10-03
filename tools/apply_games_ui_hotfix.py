#!/usr/bin/env python3
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parent.parent
SOURCE = ROOT / "src/UI/SaveSelectScreen.cpp"
HEADER = ROOT / "include/UI/SaveSelectScreen.h"
TEST = ROOT / "tests/test_game_hub_contract.py"
WORKFLOW = ROOT / ".github/workflows/product-ui-native.yml"


def replace_once(text: str, old: str, new: str, label: str) -> str:
    count = text.count(old)
    if count != 1:
        raise SystemExit(f"{label}: expected exactly one match, found {count}")
    return text.replace(old, new, 1)


def sub_once(text: str, pattern: str, repl: str, label: str) -> str:
    result, count = re.subn(pattern, repl, text, count=1, flags=re.S)
    if count != 1:
        raise SystemExit(f"{label}: expected exactly one regex match, found {count}")
    return result


source = SOURCE.read_text(encoding="utf-8")
header = HEADER.read_text(encoding="utf-8")
test = TEST.read_text(encoding="utf-8")
workflow = WORKFLOW.read_text(encoding="utf-8")

# -----------------------------------------------------------------------------
# Header state: source assignment must know where to return instead of opening
# the editor accidentally. Home also needs a Gen IV source-setup return target.
# -----------------------------------------------------------------------------
header = replace_once(
    header,
    """        std::string legacyNotice;\n        struct LegacyAssignmentEntry {""",
    """        std::string legacyNotice;\n        bool legacySourceSetupFromGamesDrawer = false;\n        bool legacySourceSetupFromClassicGames = false;\n        bool legacySourceSetupFromHome = false;\n        struct LegacyAssignmentEntry {""",
    "legacy source return state",
)
header = replace_once(
    header,
    """        bool gen4SetupFromGamesDrawer = false;\n        bool gen4SetupFromClassicGames = false;""",
    """        bool gen4SetupFromGamesDrawer = false;\n        bool gen4SetupFromClassicGames = false;\n        bool gen4SetupFromHome = false;""",
    "Gen IV Home return state",
)
header = replace_once(
    header,
    """        void openGen4Setup(const std::string& gameId, std::string notice = {},\n                           bool returnToGamesDrawer = false, bool returnToClassicGames = false);""",
    """        void openGen4Setup(const std::string& gameId, std::string notice = {},\n                           bool returnToGamesDrawer = false, bool returnToClassicGames = false,\n                           bool returnToHome = false);""",
    "Gen IV setup declaration",
)

# -----------------------------------------------------------------------------
# Sort: keep the current profile pinned and reset focus/navigation so Y-sort can
# never leave the browser in profile focus or make the next D-pad press vanish.
# -----------------------------------------------------------------------------
source = replace_once(
    source,
    """    void SaveSelectScreen::cycleGameSortMode() {\n        const int next = (static_cast<int>(gameSortMode) + 1) % 5;\n        gameSortMode = static_cast<GameSortMode>(next);\n        saveGameHubState();\n        sortGamesPreservingSelection();\n        hubNotice = std::string(\"Sorted by \") + gameSortModeLabel() + \".\";\n    }""",
    """    void SaveSelectScreen::cycleGameSortMode() {\n        // Sorting changes game order only. Pin the exact Switch profile so a sort can never\n        // look like (or become) a profile switch even if another input edge arrives nearby.\n        const std::string pinnedProfile = currentProfileIdentity();\n        const int pinnedUserIndex = userIndex;\n\n        const int next = (static_cast<int>(gameSortMode) + 1) % 5;\n        gameSortMode = static_cast<GameSortMode>(next);\n        saveGameHubState();\n        sortGamesPreservingSelection();\n\n        if (!pinnedProfile.empty() &&\n            (userIndex < 0 || userIndex >= static_cast<int>(users.size()) ||\n             profileIdentity(users[static_cast<size_t>(userIndex)].uid) != pinnedProfile)) {\n            const auto found = std::find_if(users.begin(), users.end(), [&](const UserEntry& user) {\n                return profileIdentity(user.uid) == pinnedProfile;\n            });\n            if (found != users.end())\n                userIndex = static_cast<int>(std::distance(users.begin(), found));\n            else if (!users.empty())\n                userIndex = std::clamp(pinnedUserIndex, 0, static_cast<int>(users.size()) - 1);\n        }\n\n        headerActionIndex = -1;\n        controllerNavigation.reset();\n        gamesDrawerIndex = titleIndex;\n        if (classicGamesActive) scrollClassicSelectionIntoView();\n        hubNotice = std::string(\"Sorted by \") + gameSortModeLabel() + \".\";\n    }""",
    "profile-stable sort",
)

# -----------------------------------------------------------------------------
# Home preview: if a classic game has several saves, use the explicitly preferred
# source for trainer/party/launch preview instead of treating the game as forever
# ambiguous after the user already selected a save.
# -----------------------------------------------------------------------------
source = replace_once(
    source,
    """        std::string providerId;\n        std::string sourcePath;\n        std::string bindingKey;\n        if (title.sourceKind == SelectedSourceKind::RetroArchFRLG) {\n            if (title.legacyInstances.size() == 1) {\n                const auto& instance = title.legacyInstances.front();\n                providerId = instance.providerId.empty()\n                    ? PokeVault::Source::providerIdFor(instance.providerLabel)\n                    : instance.providerId;\n                sourcePath = instance.path();\n                bindingKey = gameLaunchBindingKey(\n                    currentProfileIdentity(), title.gameId, instance.sourceIdentity);\n                previewTrainerName = instance.trainerName;\n            }\n        } else if (title.sourceKind == SelectedSourceKind::Gen4AssignedFile && legacyBindings) {""",
    """        std::string providerId;\n        std::string sourcePath;\n        std::string bindingKey;\n        int activeLegacyIndex = -1;\n        if (title.sourceKind == SelectedSourceKind::RetroArchFRLG) {\n            if (title.legacyInstances.size() == 1)\n                activeLegacyIndex = 0;\n            else if (title.legacyInstances.size() > 1)\n                activeLegacyIndex = preferredLegacySourceIndex(\n                    title.legacyInstances, legacyBindings, currentProfileIdentity(), title.gameId);\n\n            if (activeLegacyIndex >= 0 &&\n                activeLegacyIndex < static_cast<int>(title.legacyInstances.size())) {\n                const auto& instance = title.legacyInstances[static_cast<size_t>(activeLegacyIndex)];\n                providerId = instance.providerId.empty()\n                    ? PokeVault::Source::providerIdFor(instance.providerLabel)\n                    : instance.providerId;\n                sourcePath = instance.path();\n                bindingKey = gameLaunchBindingKey(\n                    currentProfileIdentity(), title.gameId, instance.sourceIdentity);\n                previewTrainerName = instance.trainerName;\n            }\n        } else if (title.sourceKind == SelectedSourceKind::Gen4AssignedFile && legacyBindings) {""",
    "preferred legacy preview source",
)

legacy_preview_pattern = r'''        if \(title\.sourceKind == SelectedSourceKind::RetroArchFRLG\) \{\n            if \(title\.legacyInstances\.size\(\) != 1\) \{.*?            partyPreviewStatus = "Validated read-only source party";\n            return;\n        \}'''
legacy_preview_repl = '''        if (title.sourceKind == SelectedSourceKind::RetroArchFRLG) {\n            if (title.legacyInstances.empty()) {\n                partyPreviewStatus = "No validated save instance.";\n                return;\n            }\n            if (activeLegacyIndex < 0 ||\n                activeLegacyIndex >= static_cast<int>(title.legacyInstances.size())) {\n                partyPreviewStatus = "Choose a Save Instance to preview its active party.";\n                launchDescriptor.backend = GameLaunchBackend::HomebrewNro;\n                launchDescriptor.state = GameLaunchState::ChooseSource;\n                launchDescriptor.providerId = "source-choice";\n                launchDescriptor.detail = "Choose the exact validated save/source to launch.";\n                return;\n            }\n\n            const auto& activeInstance =\n                title.legacyInstances[static_cast<size_t>(activeLegacyIndex)];\n            previewTrainerName = activeInstance.trainerName;\n            const size_t handle = activeInstance.sourceIndex;\n            if (!legacyCatalog || handle >= legacyCatalog->sources.size()) {\n                partyPreviewStatus = "Party preview source is stale.";\n                return;\n            }\n            const auto& source = legacyCatalog->sources[handle];\n            if (!source.ready() || source.gameId != title.gameId) {\n                partyPreviewStatus = "Party preview source no longer validates.";\n                return;\n            }\n            if (source.isGen1()) {\n                const auto dex = source.gen1Save->dexProgress();\n                previewDexSeen = dex.seen;\n                previewDexCaught = dex.caught;\n                previewDexTotal = dex.total;\n                previewTrainerGender = 0;\n                previewTrainerGenderKnown = true;\n                const auto& party = source.gen1Save->party();\n                for (size_t i = 0; i < std::min(party.size(), partyPreview.size()); ++i)\n                    addParty(i, party[i].species, party[i].level, 0, false);\n            } else if (source.isGen2()) {\n                const auto dex = source.gen2Save->dexProgress();\n                previewDexSeen = dex.seen;\n                previewDexCaught = dex.caught;\n                previewDexTotal = dex.total;\n                const auto gender = source.gen2Save->trainer().gender;\n                if (gender) {\n                    previewTrainerGender = *gender;\n                    previewTrainerGenderKnown = true;\n                } else if (title.gameId == "gold_gbc" || title.gameId == "silver_gbc") {\n                    previewTrainerGender = 0;\n                    previewTrainerGenderKnown = true;\n                }\n                const auto& party = source.gen2Save->party();\n                for (size_t i = 0; i < std::min(party.size(), partyPreview.size()); ++i)\n                    addParty(i, party[i].species, party[i].level, 0, false);\n            } else if (source.isGen3()) {\n                const auto dex = source.save->dexProgress();\n                previewDexSeen = dex.seen;\n                previewDexCaught = dex.caught;\n                previewDexTotal = dex.total;\n                previewTrainerGender = source.save->trainer().gender;\n                previewTrainerGenderKnown = true;\n                const auto party = source.save->party();\n                for (size_t i = 0; i < std::min(party.size(), partyPreview.size()); ++i)\n                    addParty(i, party[i].species, 0, 0, false);\n            }\n            partyPreviewStatus = "Validated read-only source party";\n            return;\n        }'''
source = sub_once(source, legacy_preview_pattern, legacy_preview_repl, "preferred legacy Home preview")

# -----------------------------------------------------------------------------
# Save/source selection from Home, Quick Games, or full Games is management, not
# an implicit editor open. Persist the exact source then return to the caller.
# -----------------------------------------------------------------------------
source = replace_once(
    source,
    """        if (!legacyBindings || !legacyBindings->preferGameSourceAndSave(\n                freshInstance, currentProfileIdentity(), refreshedTitle.gameId)) {\n            legacyNotice = \"Couldn't remember that exact save choice. Nothing was opened.\";\n            overlay = Overlay::LegacyInstances;\n            return;\n        }\n        selectedUserUid = u->uid;""",
    """        if (!legacyBindings || !legacyBindings->preferGameSourceAndSave(\n                freshInstance, currentProfileIdentity(), refreshedTitle.gameId)) {\n            legacyNotice = \"Couldn't remember that exact save choice. Nothing was opened.\";\n            overlay = Overlay::LegacyInstances;\n            return;\n        }\n\n        if (legacySourceSetupFromGamesDrawer || legacySourceSetupFromClassicGames ||\n            legacySourceSetupFromHome) {\n            const bool returnToDrawer = legacySourceSetupFromGamesDrawer;\n            const bool returnToHome = legacySourceSetupFromHome;\n            selectedLegacySourceIndex = freshInstance.sourceIndex;\n            hubNotice = \"Active save changed for Pokémon \" + refreshedTitle.label + \".\";\n            legacyNotice.clear();\n            launchLegacyMode = false;\n            legacySourceSetupFromGamesDrawer = false;\n            legacySourceSetupFromClassicGames = false;\n            legacySourceSetupFromHome = false;\n            overlay = returnToDrawer ? Overlay::GamesDrawer : Overlay::None;\n            if (returnToDrawer) {\n                gamesDrawerIndex = titleIndex;\n                gamesDrawerScroll = std::max(0, gamesDrawerIndex / 3 - 1);\n            }\n            if (returnToHome) refreshHubPreview();\n            controllerNavigation.reset();\n            return;\n        }\n\n        selectedUserUid = u->uid;""",
    "source management return after legacy selection",
)

# Full Games only needs cover assets. Trainer thumbnails and preview parsing were both unnecessary
# for a save-assignment browser and made the large grid heavier than it needed to be.
source = replace_once(
    source,
    """            // Warm the small set of game-card / trainer assets before the full grid becomes\n            // interactive. The caches make later entries effectively free, and row-to-row\n            // navigation no longer stalls on first-use PNG/control-icon decoding.""",
    """            // Warm only game-card artwork before the full grid becomes interactive. Trainer\n            // presentation belongs on Product Home, not in this save-assignment browser.""",
    "Games prewarm comment",
)
source = replace_once(
    source,
    """                    (void)SystemIcons::gameCardIcon(artKey, title.titleId);\n                    const auto portrait = trainerPortraitForGame(\n                        title.gameId, title.trainerGenderKnown, title.trainerGender);\n                    if (portrait.assetKey && portrait.assetKey[0] != '\\0')\n                        (void)SystemIcons::trainerPortrait(portrait.assetKey);""",
    """                    (void)SystemIcons::gameCardIcon(artKey, title.titleId);""",
    "remove Games trainer prewarm",
)
source = replace_once(
    source,
    """            headerActionIndex = -1;\n            scrollClassicSelectionIntoView();""",
    """            headerActionIndex = -1;\n            controllerNavigation.reset();\n            scrollClassicSelectionIntoView();""",
    "reset Games navigation on entry",
)

# Make source management remember its caller.
source = replace_once(
    source,
    """        const auto& game = user->titles[static_cast<size_t>(titleIndex)];\n        if (game.sourceKind == SelectedSourceKind::Gen4AssignedFile) {\n            openGen4Setup(game.gameId, \"Assign, repair, or change this game's save source.\",\n                          fromGamesDrawer, fromClassicGames);\n        } else if (game.sourceKind == SelectedSourceKind::RetroArchFRLG) {\n            legacyInstanceIndex = 0;\n            legacyInstanceScroll = 0;\n            legacyNotice.clear();\n            overlay = Overlay::LegacyInstances;""",
    """        const auto& game = user->titles[static_cast<size_t>(titleIndex)];\n        if (game.sourceKind == SelectedSourceKind::Gen4AssignedFile) {\n            openGen4Setup(game.gameId, \"Assign, repair, or change this game's save source.\",\n                          fromGamesDrawer, fromClassicGames,\n                          !fromGamesDrawer && !fromClassicGames);\n        } else if (game.sourceKind == SelectedSourceKind::RetroArchFRLG) {\n            legacySourceSetupFromGamesDrawer = fromGamesDrawer;\n            legacySourceSetupFromClassicGames = fromClassicGames;\n            legacySourceSetupFromHome = !fromGamesDrawer && !fromClassicGames;\n            launchLegacyMode = false;\n            legacyInstanceIndex = 0;\n            const int preferred = preferredLegacySourceIndex(\n                game.legacyInstances, legacyBindings, currentProfileIdentity(), game.gameId);\n            if (preferred >= 0) legacyInstanceIndex = preferred;\n            legacyInstanceScroll = std::max(0, legacyInstanceIndex - 5);\n            legacyNotice.clear();\n            overlay = Overlay::LegacyInstances;""",
    "source-management caller state",
)
source = replace_once(
    source,
    """        } else if (!unassignedLegacySources.empty()) {\n            legacyAssignmentIndex = 0;\n            legacyAssignmentScroll = 0;\n            legacyNotice.clear();\n            overlay = Overlay::LegacyAssignment;""",
    """        } else if (!unassignedLegacySources.empty()) {\n            legacySourceSetupFromGamesDrawer = fromGamesDrawer;\n            legacySourceSetupFromClassicGames = fromClassicGames;\n            legacySourceSetupFromHome = !fromGamesDrawer && !fromClassicGames;\n            legacyAssignmentIndex = 0;\n            legacyAssignmentScroll = 0;\n            legacyNotice.clear();\n            overlay = Overlay::LegacyAssignment;""",
    "legacy assignment caller state",
)

# Gen IV source setup can also return cleanly to Product Home after changing the active save.
source = replace_once(
    source,
    """    void SaveSelectScreen::openGen4Setup(const std::string& gameId, std::string notice,\n                                         bool returnToGamesDrawer, bool returnToClassicGames) {""",
    """    void SaveSelectScreen::openGen4Setup(const std::string& gameId, std::string notice,\n                                         bool returnToGamesDrawer, bool returnToClassicGames,\n                                         bool returnToHome) {""",
    "Gen IV setup definition",
)
source = replace_once(
    source,
    """        gen4SetupFromGamesDrawer = returnToGamesDrawer;\n        gen4SetupFromClassicGames = returnToClassicGames;""",
    """        gen4SetupFromGamesDrawer = returnToGamesDrawer;\n        gen4SetupFromClassicGames = returnToClassicGames;\n        gen4SetupFromHome = returnToHome;""",
    "Gen IV setup caller capture",
)
source = source.replace(
    "if (gen4SetupFromGamesDrawer || gen4SetupFromClassicGames) {",
    "if (gen4SetupFromGamesDrawer || gen4SetupFromClassicGames || gen4SetupFromHome) {",
)
source = source.replace(
    "gen4SetupFromGamesDrawer = false;\n            gen4SetupFromClassicGames = false;",
    "gen4SetupFromGamesDrawer = false;\n            gen4SetupFromClassicGames = false;\n            gen4SetupFromHome = false;",
)
source = source.replace(
    "gen4SetupFromGamesDrawer, gen4SetupFromClassicGames);",
    "gen4SetupFromGamesDrawer, gen4SetupFromClassicGames, gen4SetupFromHome);",
)

# Legacy assignment must also return to the exact surface that invoked X Save / Source.
source = replace_once(
    source,
    """        gamesDrawerIndex = titleIndex;\n        gamesDrawerScroll = std::max(0, gamesDrawerIndex / 3 - 1);\n        legacyNotice = assignedLeaf + \" assigned to this profile.\";\n        hubNotice = legacyNotice;\n        overlay = classicGamesActive ? Overlay::None : Overlay::GamesDrawer;\n        scrollSelectionIntoView();\n        refreshHubPreview();\n        return true;""",
    """        gamesDrawerIndex = titleIndex;\n        gamesDrawerScroll = std::max(0, gamesDrawerIndex / 3 - 1);\n        legacyNotice = assignedLeaf + \" assigned to this profile.\";\n        hubNotice = legacyNotice;\n        const bool returnToDrawer = legacySourceSetupFromGamesDrawer;\n        const bool returnToHome = legacySourceSetupFromHome;\n        overlay = returnToDrawer ? Overlay::GamesDrawer : Overlay::None;\n        legacySourceSetupFromGamesDrawer = false;\n        legacySourceSetupFromClassicGames = false;\n        legacySourceSetupFromHome = false;\n        scrollSelectionIntoView();\n        if (returnToHome) refreshHubPreview();\n        return true;""",
    "legacy assignment return target",
)

# Overlay cancellation should not throw Home into Quick Games or lose the full Games context.
legacy_assignment_old = '''            if (kDown & HidNpadButton_B) {\n                overlay = classicGamesActive ? Overlay::None : Overlay::GamesDrawer;\n                return;\n            }'''
legacy_assignment_new = '''            if (kDown & HidNpadButton_B) {\n                overlay = legacySourceSetupFromGamesDrawer ? Overlay::GamesDrawer : Overlay::None;\n                legacySourceSetupFromGamesDrawer = false;\n                legacySourceSetupFromClassicGames = false;\n                legacySourceSetupFromHome = false;\n                return;\n            }'''
source = replace_once(source, legacy_assignment_old, legacy_assignment_new, "legacy assignment cancel")
source = source.replace(
    "overlay = classicGamesActive ? Overlay::None : Overlay::GamesDrawer;\n                return;",
    "overlay = legacySourceSetupFromGamesDrawer ? Overlay::GamesDrawer : Overlay::None;\n                legacySourceSetupFromGamesDrawer = false;\n                legacySourceSetupFromClassicGames = false;\n                legacySourceSetupFromHome = false;\n                return;",
    2,
)

# Legacy Save Instances B respects its caller too.
source = replace_once(
    source,
    """            if (kDown & HidNpadButton_B) {\n                overlay = Overlay::None;\n                launchLegacyMode = false;\n                return;\n            }""",
    """            if (kDown & HidNpadButton_B) {\n                overlay = legacySourceSetupFromGamesDrawer ? Overlay::GamesDrawer : Overlay::None;\n                if (overlay == Overlay::GamesDrawer) {\n                    gamesDrawerIndex = titleIndex;\n                    gamesDrawerScroll = std::max(0, gamesDrawerIndex / 3 - 1);\n                }\n                launchLegacyMode = false;\n                legacySourceSetupFromGamesDrawer = false;\n                legacySourceSetupFromClassicGames = false;\n                legacySourceSetupFromHome = false;\n                return;\n            }""",
    "legacy instance cancel return",
)

# Full Games D-pad/stick motion is one directional decision per frame. This fixes the hardware case
# where Red could fail to move right to Blue when stale/opposite direction bits coexisted.
classic_move_old = '''                const int cols = classicTitleColumns();\n                const int row = titleIndex / cols;\n                const int col = titleIndex % cols;\n                // Six-column browser navigation is spatial, not a flat cyclic list. Left/Right\n                // stay inside the current row; Up/Down preserve the column and clamp only when\n                // the final row is incomplete.\n                if ((kDown & HidNpadButton_Left) && col > 0)\n                    --titleIndex;\n                if ((kDown & HidNpadButton_Right) && col + 1 < cols &&\n                    titleIndex + 1 < classicCount)\n                    ++titleIndex;\n                if (kDown & HidNpadButton_Up) {\n                    if (row > 0)\n                        titleIndex = std::min(classicCount - 1, (row - 1) * cols + col);\n                    else {\n                        headerActionIndex = 0;\n                        return;\n                    }\n                }\n                if (kDown & HidNpadButton_Down) {\n                    const int nextRow = (row + 1) * cols;\n                    if (nextRow < classicCount)\n                        titleIndex = std::min(classicCount - 1, nextRow + col);\n                }'''
classic_move_new = '''                const int cols = classicTitleColumns();\n                const int row = titleIndex / cols;\n                const int col = titleIndex % cols;\n                // Six-column browser navigation is spatial and mutually exclusive per frame.\n                // Right is intentionally resolved before Left so an opposite/stale bit can never\n                // cancel the common Red -> Blue move seen on hardware.\n                const u64 direction = kDown & (HidNpadButton_Left | HidNpadButton_Right |\n                                                HidNpadButton_Up | HidNpadButton_Down);\n                if ((direction & HidNpadButton_Right) && col + 1 < cols &&\n                    titleIndex + 1 < classicCount) {\n                    ++titleIndex;\n                } else if ((direction & HidNpadButton_Left) && col > 0) {\n                    --titleIndex;\n                } else if (direction & HidNpadButton_Up) {\n                    if (row > 0)\n                        titleIndex = std::min(classicCount - 1, (row - 1) * cols + col);\n                    else {\n                        headerActionIndex = 0;\n                        controllerNavigation.reset();\n                        return;\n                    }\n                } else if (direction & HidNpadButton_Down) {\n                    const int nextRow = (row + 1) * cols;\n                    if (nextRow < classicCount)\n                        titleIndex = std::min(classicCount - 1, nextRow + col);\n                }'''
source = replace_once(source, classic_move_old, classic_move_new, "classic grid directional movement")

# Product Home gets an explicit X Save / Source action so games with several saves can switch the
# active save without going through the full Games browser.
source = replace_once(
    source,
    """        if (kDown & HidNpadButton_Y) {\n            const UserEntry* quickUser = currentUser();""",
    """        if (kDown & HidNpadButton_X) {\n            openSaveSourceForCurrentTitle(false, false);\n            return;\n        }\n        if (kDown & HidNpadButton_Y) {\n            const UserEntry* quickUser = currentUser();""",
    "Home X save/source action",
)

# Full Games tiles are now save/source cards: cover + title + source state, no trainer portrait or
# Pokédex/trainer metadata. This both matches the intended purpose and removes extra work per frame.
classic_card_old = '''            const auto portrait = trainerPortraitForGame(\n                title.gameId, title.trainerGenderKnown, title.trainerGender);\n            drawTrainerPortrait(fb, x + CLASSIC_TILE_W - 58, y + 18, 48, 58,\n                                portrait, false);\n\n            std::string label = title.label;\n            if (label.size() > 18) label = label.substr(0, 17) + "…";\n            int lw=0,lh=0; fb.measureText(label,lw,lh,TextStyle::Caption);\n            fb.drawText(x + (CLASSIC_TILE_W-lw)/2, y + 146, label,\n                        focused ? Colors::SelectedText : Colors::TextPrimary, TextStyle::Caption);\n\n            std::string meta = title.trainerName.empty()\n                ? productSourceLabel(title.sourceLabel) : title.trainerName;\n            if (title.dexTotal > 0)\n                meta += "  •  " + std::to_string(title.dexCaught) + "/" + std::to_string(title.dexTotal);\n            if (meta.size() > 24) meta = meta.substr(0, 23) + "…";\n            int mw=0,mh=0; fb.measureText(meta,mw,mh,TextStyle::Caption);\n            fb.drawText(x + (CLASSIC_TILE_W-mw)/2, y + 172, meta,\n                        Colors::TextMuted, TextStyle::Caption);'''
classic_card_new = '''            std::string label = title.label;\n            if (label.size() > 18) label = label.substr(0, 17) + "…";\n            int lw=0,lh=0; fb.measureText(label,lw,lh,TextStyle::Caption);\n            fb.drawText(x + (CLASSIC_TILE_W-lw)/2, y + 146, label,\n                        focused ? Colors::SelectedText : Colors::TextPrimary, TextStyle::Caption);\n\n            std::string meta;\n            if (title.sourceKind == SelectedSourceKind::RetroArchFRLG) {\n                if (title.legacyInstances.empty())\n                    meta = "No save assigned";\n                else if (title.legacyInstances.size() == 1)\n                    meta = "1 save • " + productSourceLabel(title.sourceLabel);\n                else\n                    meta = std::to_string(title.legacyInstances.size()) + " saves";\n            } else if (title.sourceKind == SelectedSourceKind::Gen4AssignedFile) {\n                meta = productSourceLabel(title.sourceLabel);\n                if (!title.locationLabel.empty()) meta += " • " + title.locationLabel;\n            } else {\n                meta = "System save";\n            }\n            if (meta.size() > 24) meta = meta.substr(0, 23) + "…";\n            int mw=0,mh=0; fb.measureText(meta,mw,mh,TextStyle::Caption);\n            fb.drawText(x + (CLASSIC_TILE_W-mw)/2, y + 172, meta,\n                        Colors::TextMuted, TextStyle::Caption);'''
source = replace_once(source, classic_card_old, classic_card_new, "remove Games trainer metadata")

# Region art should stay visible: lighten the text glass while retaining high-contrast text.
source = replace_once(source, "Color(3, 10, 24, 184)", "Color(3, 10, 24, 132)", "Home glass opacity")

# Trainer art is transparent artwork, not a mini card. Use a double outline/shadow frame that reads
# on light and dark themes without painting a white/black rectangle behind the character.
trainer_pattern = r'''    void drawTrainerPortrait\(PKSEFramebuffer& fb, int x, int y, int w, int h,\n                             const TrainerPortraitPresentation& portrait,\n                             bool showLabel\) \{.*?\n    \}\n\n    // Approved product-home layout'''
trainer_repl = '''    void drawTrainerPortrait(PKSEFramebuffer& fb, int x, int y, int w, int h,\n                             const TrainerPortraitPresentation& portrait,\n                             bool showLabel) {\n        const Color accent = Colors::Info;\n        const IconImage& art = SystemIcons::trainerPortrait(portrait.assetKey);\n        const int labelReserve = showLabel ? 24 : 4;\n        const int frameH = std::max(20, h - labelReserve);\n\n        // No filled card behind trainer art. A dark outer stroke plus a pale inner stroke stays\n        // visible over every region image and every app theme without hiding the PNG transparency.\n        fb.drawRoundedRect(x, y, w, frameH, 12, Color(0, 0, 0, 176), 4);\n        fb.drawRoundedRect(x + 2, y + 2, std::max(1, w - 4), std::max(1, frameH - 4),\n                           10, Color(245, 250, 255, 188), 2);\n\n        if (art.valid()) {\n            const auto rect = PokeBank::UIModel::containSprite(\n                x + 5, y + 4, w - 10, frameH - 8, art.width, art.height);\n            if (rect.width > 0 && rect.height > 0)\n                fb.drawImageScaled(rect.x, rect.y, art.width, art.height,\n                                   rect.width, rect.height, art.data, 4);\n        } else {\n            // Truthful fallback: a Poké Ball identity badge, never a fake "character portrait".\n            const int cx = x + w / 2;\n            const int cy = y + frameH / 2;\n            const int r = std::max(13, std::min(w, frameH) / 4);\n            fb.drawCircle(cx, cy, r, withAlpha(accent, 220), 3);\n            fb.drawFilledRect(cx - r, cy - 2, r * 2, 4, withAlpha(accent, 170));\n            fb.drawFilledCircle(cx, cy, std::max(4, r / 4), accent);\n        }\n\n        if (showLabel) {\n            std::string label = portrait.label;\n            if (label.size() > 14) label = label.substr(0, 13) + "…";\n            int tw = 0, th = 0;\n            fb.measureText(label, tw, th, TextStyle::Caption);\n            const int tx = x + std::max(4, (w - tw) / 2);\n            const int ty = y + h - 20;\n            fb.drawText(tx + 1, ty + 1, label, Color(0, 0, 0, 220), TextStyle::Caption);\n            fb.drawText(tx, ty, label, Color(248, 251, 255, 255), TextStyle::Caption);\n        }\n    }\n\n    // Approved product-home layout'''
source = sub_once(source, trainer_pattern, trainer_repl, "transparent trainer presentation")

# Footer/help explain the new Home save/source action.
source = replace_once(
    source,
    '''        drawNavBar(fb, {{"L/R", "Change Game"}, {"A", "Open"}, {"Y", "Quick Games"},\n                        {"+", "Current Game"}, {"-", "Help"}, {"B", "Exit"}});''',
    '''        drawNavBar(fb, {{"L/R", "Change Game"}, {"A", "Open"}, {"X", "Save / Source"},\n                        {"Y", "Quick Games"}, {"+", "Current Game"}, {"-", "Help"}, {"B", "Exit"}});''',
    "Home footer save/source",
)
source = replace_once(
    source,
    '''                    {"L/R", "Previous / next game"},\n                    {"Y", "Open the quick right-side artwork browser"},''',
    '''                    {"L/R", "Previous / next game"},\n                    {"X", "Assign / change the selected game's save source"},\n                    {"Y", "Open the quick right-side artwork browser"},''',
    "Home help save/source",
)

# The Save Instances footer says what A actually does in source-management mode.
source = replace_once(
    source,
    '''            drawNavBar(fb, {{"D-pad/Stick", "Choose Save"},\n                            {"A", launchLegacyMode ? "Launch / Link" : "Open Read Only"},\n                            {"Y", "Source Details"}, {"X", "Refresh Saves"}, {"B", "Back"}});''',
    '''            const bool sourceSetup = legacySourceSetupFromGamesDrawer ||\n                legacySourceSetupFromClassicGames || legacySourceSetupFromHome;\n            drawNavBar(fb, {{"D-pad/Stick", "Choose Save"},\n                            {"A", launchLegacyMode ? "Launch / Link"\n                                                   : sourceSetup ? "Use This Save" : "Open Read Only"},\n                            {"Y", "Source Details"}, {"X", "Refresh Saves"}, {"B", "Back"}});''',
    "legacy source-management footer",
)

# -----------------------------------------------------------------------------
# Contracts for the hardware regressions fixed in this pass.
# -----------------------------------------------------------------------------
test = replace_once(test, '"Color(3, 10, 24, 184)" in source', '"Color(3, 10, 24, 132)" in source',
                    "glass contract")
test = replace_once(
    test,
    '''require("Horizontal movement never spills into" in source and\n        "Six-column browser navigation is spatial" in source and\n        "col + 1 < cols" in source and\n        "nextRow < classicCount" in source,\n        "Quick Games and full Games must use row-bounded spatial grid navigation")''',
    '''require("Horizontal movement never spills into" in source and\n        "Six-column browser navigation is spatial and mutually exclusive per frame" in source and\n        "direction & HidNpadButton_Right" in source and\n        "col + 1 < cols" in source and\n        "nextRow < classicCount" in source,\n        "Quick Games and full Games must use row-bounded spatial grid navigation with reliable Right movement")''',
    "grid navigation contract",
)
test = replace_once(
    test,
    '''require("Warm the small set of game-card / trainer assets" in activate_block and\n        "SystemIcons::gameCardIcon" in activate_block and\n        "SystemIcons::trainerPortrait" in activate_block,\n        "full Games must prewarm artwork caches before interactive scrolling")''',
    '''require("Warm only game-card artwork" in activate_block and\n        "SystemIcons::gameCardIcon" in activate_block and\n        "SystemIcons::trainerPortrait" not in activate_block,\n        "full Games must prewarm only cover artwork and keep trainer rendering out of the scrolling hot path")''',
    "Games prewarm contract",
)
insert_after = '''require('std::string("Order: ") + gameSortModeLabel()' in source,\n        "Product Home must show the active game order so L/R navigation never looks random")'''
addition = insert_after + '''\nrequire("pinnedProfile = currentProfileIdentity()" in source and\n        "headerActionIndex = -1;" in source and "controllerNavigation.reset();" in source,\n        "sorting must preserve the exact profile and leave grid navigation ready for the next input")\nrequire("legacySourceSetupFromHome" in header and "gen4SetupFromHome" in header and\n        'openSaveSourceForCurrentTitle(false, false);' in source and\n        '{"X", "Save / Source"}' in source,\n        "Product Home must expose explicit active-save/source selection and return Home after management")\nrequire("No save assigned" in classic_draw and "System save" in classic_draw and\n        "trainerPortraitForGame(" not in classic_draw and "title.trainerName" not in classic_draw,\n        "full Games must stay a lightweight save/source assignment browser without trainer metadata")\nrequire("No filled card behind trainer art" in source and\n        "Color(0, 0, 0, 176)" in source and "Color(245, 250, 255, 188)" in source,\n        "Product Home trainer art must remain transparent with a theme-safe double outline")'''
test = replace_once(test, insert_after, addition, "new Games/Home contracts")

# -----------------------------------------------------------------------------
# Remove the one-time CI bootstrap from the workflow before this script's commit
# is pushed. The next exact-head run therefore uses the normal read-only workflow.
# -----------------------------------------------------------------------------
workflow = workflow.replace("permissions:\n  contents: write  # ONE_TIME_GAMES_UI_HOTFIX\n",
                            "permissions:\n  contents: read\n")
workflow = re.sub(
    r'''\n      # ONE_TIME_GAMES_UI_HOTFIX_START\n      - name: Apply one-time Games UI hotfix\n        run: \|\n.*?          exit 1\n      # ONE_TIME_GAMES_UI_HOTFIX_END\n''',
    "\n",
    workflow,
    count=1,
    flags=re.S,
)
if "ONE_TIME_GAMES_UI_HOTFIX" in workflow:
    raise SystemExit("workflow bootstrap markers were not fully removed")

SOURCE.write_text(source, encoding="utf-8")
HEADER.write_text(header, encoding="utf-8")
TEST.write_text(test, encoding="utf-8")
WORKFLOW.write_text(workflow, encoding="utf-8")

# This file exists only to let the authenticated Actions runner apply a narrow patch without
# replacing the entire 3,000+ line source through the Contents API. Delete it in the final commit.
Path(__file__).unlink()
print("Games UI hotfix applied; bootstrap workflow restored and helper removed.")
