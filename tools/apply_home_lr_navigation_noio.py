#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def replace_once(path: str, old: str, new: str, label: str) -> None:
    p = ROOT / path
    text = p.read_text(encoding="utf-8")
    count = text.count(old)
    if count != 1:
        raise SystemExit(f"{label}: expected exactly one match, got {count}")
    p.write_text(text.replace(old, new, 1), encoding="utf-8")


# Declare a cached-only selection refresh next to the existing full preview refresh.
replace_once(
    "include/UI/SaveSelectScreen.h",
    """        void setUser(int idx);\n        void refreshHubPreview();\n        bool launchCurrentTitle();""",
    """        void setUser(int idx);\n        void refreshHubSelectionFromCache();\n        void refreshHubPreview();\n        bool launchCurrentTitle();""",
    "SaveSelectScreen cached refresh declaration",
)

# Product Home shoulder navigation must never mount/parse saves, scan ROM folders, or enumerate HOME
# applications. It copies already-discovered card metadata only and clears the previous game's launch
# target so the UI can repaint immediately. The real target is resolved when Launch is requested.
replace_once(
    "src/UI/SaveSelectScreen.cpp",
    """    void SaveSelectScreen::refreshHubPreview() {\n        partyPreview = {};""",
    """    void SaveSelectScreen::refreshHubSelectionFromCache() {\n        partyPreview = {};\n        partyPreviewStatus.clear();\n        previewTrainerName.clear();\n        previewTrainerGender = 0;\n        previewTrainerGenderKnown = false;\n        previewDexSeen = 0;\n        previewDexCaught = 0;\n        previewDexTotal = 0;\n        hubNotice.clear();\n        launchDescriptor = {};\n\n        const UserEntry* user = currentUser();\n        if (!user || titleIndex < 0 || titleIndex >= static_cast<int>(user->titles.size())) {\n            partyPreviewStatus = \"No game selected\";\n            return;\n        }\n\n        const auto& title = user->titles[static_cast<size_t>(titleIndex)];\n        previewTrainerName = title.trainerName;\n        previewTrainerGender = title.trainerGender;\n        previewTrainerGenderKnown = title.trainerGenderKnown;\n        previewDexSeen = title.dexSeen;\n        previewDexCaught = title.dexCaught;\n        previewDexTotal = title.dexTotal;\n\n        // Keep the selected card responsive. This is deliberately presentation-only: no save mount,\n        // parser, provider discovery, HOME application enumeration, or ROM filesystem scan belongs on\n        // the L/R input frame. ZR/A perform exact validation when the user actually requests work.\n        if (title.sourceKind == SelectedSourceKind::RetroArchFRLG &&\n            title.legacyInstances.size() > 1) {\n            launchDescriptor.backend = GameLaunchBackend::HomebrewNro;\n            launchDescriptor.state = GameLaunchState::ChooseSource;\n            launchDescriptor.providerId = \"source-choice\";\n            launchDescriptor.detail = \"Choose the exact validated save/source to launch.\";\n            partyPreviewStatus = \"Choose a Save Instance to refresh the active party.\";\n        } else {\n            launchDescriptor.state = GameLaunchState::Ready;\n            launchDescriptor.detail = \"Launch target resolves when requested.\";\n            partyPreviewStatus = \"Open or launch to refresh the active party.\";\n        }\n    }\n\n    void SaveSelectScreen::refreshHubPreview() {\n        partyPreview = {};""",
    "cached Product Home selection refresh",
)

replace_once(
    "src/UI/SaveSelectScreen.cpp",
    """            if (titleIndex != before) {\n                scrollSelectionIntoView();\n                refreshHubPreview();\n            }\n        }\n\n        if (headerActionIndex >= 0) {""",
    """            if (titleIndex != before) {\n                scrollSelectionIntoView();\n                refreshHubSelectionFromCache();\n            }\n        }\n\n        if (headerActionIndex >= 0) {""",
    "Product Home L/R cached refresh",
)

# A cached placeholder must never be trusted for an actual launch. Re-run the exact preview/launch
# resolution only after the deliberate ZR action, where the user expects I/O and validation.
replace_once(
    "src/UI/SaveSelectScreen.cpp",
    """        const auto& title = user->titles[static_cast<size_t>(titleIndex)];\n\n        if (title.sourceKind == SelectedSourceKind::RetroArchFRLG &&\n            title.legacyInstances.size() > 1) {\n            launchLegacyMode = true;""",
    """        const auto& title = user->titles[static_cast<size_t>(titleIndex)];\n\n        // L/R keeps only cached presentation state. Resolve and validate the exact launch target now,\n        // on the explicit launch action, before consulting launchDescriptor.\n        refreshHubPreview();\n\n        if (title.sourceKind == SelectedSourceKind::RetroArchFRLG &&\n            title.legacyInstances.size() > 1) {\n            launchLegacyMode = true;""",
    "explicit launch revalidation",
)

# Lock the navigation boundary into the product contract so no future performance/launch patch can
# quietly put save parsing or launcher discovery back onto the L/R input path.
test_path = ROOT / "tests/test_game_hub_contract.py"
test = test_path.read_text(encoding="utf-8")
anchor = '''require("HidNpadButton_L" in source and "HidNpadButton_R" in source,\n        "L/R must switch the selected game")\n'''
if anchor not in test:
    raise SystemExit("game hub contract: L/R anchor missing")
extra = '''require("refreshHubSelectionFromCache" in header and "refreshHubSelectionFromCache" in source,\n        "Product Home must have an explicit cached-only game-selection refresh")\nlr_start = source.index("// L/R changes the selected game from anywhere on Product Home")\nlr_end = source.index("if (headerActionIndex >= 0)", lr_start)\nlr_block = source[lr_start:lr_end]\nrequire("refreshHubSelectionFromCache();" in lr_block and "refreshHubPreview();" not in lr_block,\n        "Product Home L/R must not run the full save/launch preview on the input frame")\ncache_start = source.index("void SaveSelectScreen::refreshHubSelectionFromCache()")\ncache_end = source.index("void SaveSelectScreen::refreshHubPreview()", cache_start)\ncache_block = source[cache_start:cache_end]\nfor forbidden in ("resolveGameLaunch", "fsdevMountSaveData", "openAssignedSource",\n                  "readTrainerInfoFRLG", "discoverConfiguredLegacySaves"):\n    require(forbidden not in cache_block,\n            f"cached Product Home selection refresh must not perform I/O: {forbidden}")\nlaunch_start = source.index("bool SaveSelectScreen::launchCurrentTitle()")\nlaunch_end = source.index("void SaveSelectScreen::openGen4Setup", launch_start)\nlaunch_block = source[launch_start:launch_end]\nrequire("refreshHubPreview();" in launch_block and\n        launch_block.index("refreshHubPreview();") < launch_block.index("launchDescriptor.state"),\n        "explicit Launch must resolve the exact target before using launchDescriptor")\n'''
if extra.strip() not in test:
    test = test.replace(anchor, anchor + extra, 1)
test_path.write_text(test, encoding="utf-8")

print("Applied Product Home L/R no-I/O navigation fix.")
