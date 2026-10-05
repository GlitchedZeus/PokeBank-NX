from pathlib import Path


def replace_once(text: str, old: str, new: str, label: str) -> str:
    count = text.count(old)
    if count != 1:
        raise SystemExit(f"{label}: expected exactly one anchor, found {count}")
    return text.replace(old, new, 1)


# 1) SaveSelectScreen public resume hook + preview-only refresh option.
p = Path("include/UI/SaveSelectScreen.h")
s = p.read_text(encoding="utf-8")
s = replace_once(
    s,
    "        [[nodiscard]] NavigationState navigationState() const;\n\n",
    "        [[nodiscard]] NavigationState navigationState() const;\n"
    "        // Resume Product Home after an editor without rebuilding the whole catalog.\n"
    "        void resumeAfterEditor();\n\n",
    "SaveSelectScreen resume hook",
)
s = replace_once(
    s,
    "        void refreshHubPreview();\n",
    "        void refreshHubPreview(bool resolveLaunchTarget = true);\n",
    "SaveSelectScreen preview signature",
)
p.write_text(s, encoding="utf-8")


# 2) Product Home: real trainer/dex/party preview on navigation, but no launch discovery on that frame.
p = Path("src/UI/SaveSelectScreen.cpp")
s = p.read_text(encoding="utf-8")
s = replace_once(
    s,
    "    void SaveSelectScreen::refreshHubPreview() {\n",
    "    void SaveSelectScreen::refreshHubPreview(bool resolveLaunchTarget) {\n",
    "refreshHubPreview definition",
)

old_resolve = '''        launchDescriptor = resolveGameLaunch(
            title.titleId, title.gameId, providerId, sourcePath, bindingKey);

'''
new_resolve = '''        if (resolveLaunchTarget) {
            launchDescriptor = resolveGameLaunch(
                title.titleId, title.gameId, providerId, sourcePath, bindingKey);
        } else {
            // Presentation-only refresh: trainer/dex/party still come from the real save, but
            // navigation must never enumerate HOME applications, playlists, or ROM libraries.
            launchDescriptor = {};
            if (title.sourceKind == SelectedSourceKind::RetroArchFRLG &&
                title.legacyInstances.size() > 1) {
                launchDescriptor.backend = GameLaunchBackend::HomebrewNro;
                launchDescriptor.state = GameLaunchState::ChooseSource;
                launchDescriptor.providerId = "source-choice";
                launchDescriptor.detail = "Choose the exact validated save/source to launch.";
            } else {
                launchDescriptor.state = GameLaunchState::Ready;
                launchDescriptor.detail = "Launch target resolves when requested.";
            }
        }

'''
s = replace_once(s, old_resolve, new_resolve, "preview launch-resolution boundary")

# Constructor: restore the dee2ad47 visible-data behavior without launch discovery.
ctor_start = s.index("    SaveSelectScreen::SaveSelectScreen(")
ctor_end = s.index("    void SaveSelectScreen::loadLegacySources(", ctor_start)
ctor = s[ctor_start:ctor_end]
if not ctor.rstrip().endswith("refreshHubPreview();\n    }"):
    raise SystemExit("constructor final preview anchor changed")
ctor = ctor.replace("        refreshHubPreview();\n    }", "        refreshHubPreview(false);\n    }", 1)
s = s[:ctor_start] + ctor + s[ctor_end:]

# L/R: load the selected save's actual presentation, while the false flag keeps launch I/O out.
lr_start = s.index("// L/R changes the selected game from anywhere on Product Home")
lr_end = s.index("if (headerActionIndex >= 0)", lr_start)
lr = s[lr_start:lr_end]
if "refreshHubSelectionFromCache();" not in lr:
    raise SystemExit("Product Home L/R cached-refresh anchor changed")
lr = lr.replace("refreshHubSelectionFromCache();", "refreshHubPreview(false);", 1)
s = s[:lr_start] + lr + s[lr_end:]

# Profile switching and leaving Full Games should refresh visible save data, not launch targets.
set_user_start = s.index("    void SaveSelectScreen::setUser(")
set_user_end = s.index("    void SaveSelectScreen::refreshHubSelectionFromCache()", set_user_start)
set_user = s[set_user_start:set_user_end]
set_user = replace_once(
    set_user,
    "        if (!classicGamesActive) refreshHubPreview();\n",
    "        if (!classicGamesActive) refreshHubPreview(false);\n",
    "setUser preview",
)
s = s[:set_user_start] + set_user + s[set_user_end:]

classic_old = '''            if (kDown & HidNpadButton_B) {
                classicGamesActive = false;
                scrollRow = 0;
                refreshHubPreview();
                return;
            }
'''
classic_new = '''            if (kDown & HidNpadButton_B) {
                classicGamesActive = false;
                scrollRow = 0;
                refreshHubPreview(false);
                return;
            }
'''
s = replace_once(s, classic_old, classic_new, "Full Games return preview")

# Reuse the exact Product Home object after editor exit. Only refresh the selected card presentation.
insert_anchor = "    void SaveSelectScreen::loadLegacySources(\n"
if "void SaveSelectScreen::resumeAfterEditor()" in s:
    raise SystemExit("resumeAfterEditor already exists")
resume = '''    void SaveSelectScreen::resumeAfterEditor() {
        titleSelected = false;
        selectedUserUid = {};
        selectedTitleId = 0;
        selectedTitleName.clear();
        selectedGameId.clear();
        selectedSourceKind = SelectedSourceKind::None;
        selectedLegacySourceIndex = 0;
        openIntent = OpenIntent::Default;
        overlay = Overlay::None;
        exitRequested = false;
        appExitRequested = false;
        requestedMainMenuDestination = MainMenuDestination::None;
        hubNotice.clear();

        // Preserve the existing catalog, focus and sort state. Refresh only what the Main Card
        // displays; do not re-enumerate users/saves/providers just because an editor closed.
        refreshHubPreview(false);
    }

'''
s = replace_once(s, insert_anchor, resume + insert_anchor, "resumeAfterEditor insertion")
p.write_text(s, encoding="utf-8")


# 3) UI manager: don't destroy/rebuild Product Home when an editor closes.
p = Path("src/UI/UI.cpp")
s = p.read_text(encoding="utf-8")
s = replace_once(
    s,
    "                    rebuildPicker = true;\n                    break;\n",
    "                    // Return to the same Product Home instance. Rebuilding here re-ran account/save\n"
    "                    // enumeration and source parsing before a frame could draw, which looked like a freeze.\n"
    "                    selectScreen.resumeAfterEditor();\n"
    "                    fb.startFade();\n"
    "                    continue;\n",
    "editor return rebuild",
)
p.write_text(s, encoding="utf-8")


# 4) Launcher: restore the already-working Gen I-III path. Installed-HOME-forwarder probing is
# needed for the unfinished DS direct-launch path only; probing every installed app before Crystal,
# GBA, etc. was a new synchronous stall compared with dee2ad47.
p = Path("src/UI/GameLauncher.cpp")
s = p.read_text(encoding="utf-8")
fn_start = s.index("GameLaunchDescriptor resolveGameLaunch(")
fn_end = s.index("bool saveGameLaunchBinding(", fn_start)
fn = s[fn_start:fn_end]
open_anchor = '''#ifdef __SWITCH__
    if (const uint64_t forwarderTitle = installedGameForwarderTitle(gameId); forwarderTitle != 0) {
'''
if open_anchor not in fn:
    raise SystemExit("installed forwarder resolve anchor changed")
fn = fn.replace(
    open_anchor,
    '''#ifdef __SWITCH__
    // Gen I-III already had a working direct emulator/core path in dee2ad47. Do not put a full
    // installed-application metadata scan in front of those launches. Keep HOME-forwarder probing
    // focused on Nintendo DS, which is the unfinished launch tranche this code was added for.
    if (gameId.ends_with("_nds")) {
        if (const uint64_t forwarderTitle = installedGameForwarderTitle(gameId); forwarderTitle != 0) {
''',
    1,
)
close_anchor = '''        result.detail = "Launch the installed HOME forwarder for this exact game.";
        return result;
    }
#endif

    const GameLaunchProviderKind kind = providerKindForSourcePath(providerId, sourcePath);
'''
if close_anchor not in fn:
    raise SystemExit("installed forwarder close anchor changed")
fn = fn.replace(
    close_anchor,
    '''            result.detail = "Launch the installed HOME forwarder for this exact game.";
            return result;
        }
    }
#endif

    const GameLaunchProviderKind kind = providerKindForSourcePath(providerId, sourcePath);
''',
    1,
)
s = s[:fn_start] + fn + s[fn_end:]
p.write_text(s, encoding="utf-8")


# 5) Update source contract to encode the intended behavior: live save preview, no launch discovery.
p = Path("tests/test_game_hub_contract.py")
s = p.read_text(encoding="utf-8")
old = '''require("refreshHubSelectionFromCache();" in lr_block and "refreshHubPreview();" not in lr_block,
        "Product Home L/R must not run the full save/launch preview on the input frame")
'''
new = '''require("refreshHubPreview(false);" in lr_block and "resolveGameLaunch" not in lr_block,
        "Product Home L/R must restore real trainer/dex/party data without launch discovery")
'''
s = replace_once(s, old, new, "L/R source contract")

# The function signature now has a parameter; keep the cached helper boundary search stable.
s = replace_once(
    s,
    'cache_end = source.index("void SaveSelectScreen::refreshHubPreview()", cache_start)\n',
    'cache_end = source.index("void SaveSelectScreen::refreshHubPreview(bool resolveLaunchTarget)", cache_start)\n',
    "preview signature contract",
)

# Leaving Full Games should restore the real Main Card presentation without resolving launch targets.
s = replace_once(
    s,
    '"refreshHubPreview();" in classic_runtime_block[\n',
    '"refreshHubPreview(false);" in classic_runtime_block[\n',
    "Full Games return source contract",
)

ui_anchor = '''require("const auto destination = handleSaveSelection();" in ui_manager,
        "approved Games/product-home screen must be the app root")
'''
ui_new = ui_anchor + '''require("selectScreen.resumeAfterEditor();" in ui_manager,
        "editor exit must reuse Product Home instead of rebuilding the whole save catalog")
'''
s = replace_once(s, ui_anchor, ui_new, "editor return contract")
p.write_text(s, encoding="utf-8")

print("focused Product Home hardware recovery applied")
