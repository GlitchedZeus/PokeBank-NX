from pathlib import Path

p = Path("tests/test_game_hub_contract.py")
text = p.read_text()


def replace_once(old: str, new: str, label: str) -> None:
    global text
    count = text.count(old)
    if count != 1:
        raise SystemExit(f"{label}: expected exactly one match, found {count}")
    text = text.replace(old, new, 1)


replace_once(
    '''require('\"Multiple saves exist. Open Source / Game File once to choose the exact save.\"' in source and
        '\"That save changed while opening. Nothing was opened.\"' in source,
        "ambiguous or changed legacy sources must fail closed instead of guessing")''',
    '''require('\"Multiple saves exist. Open Source / Game File once to choose the exact save.\"' in source and
        '\"That save changed since discovery. Review the refreshed save list.\"' in source and
        "legacySnapshotStillCurrent" in source,
        "ambiguous or changed legacy sources must fail closed without rescanning every provider")''',
    "changed-save contract",
)

replace_once(
    '''require("const TitleEntry selected =" in stable_open and
        "const std::string selectedGameId = selected.gameId;" in stable_open,
        "game open must snapshot stable game identity before refreshing source discovery")
require("candidate.gameId == selectedGameId" in stable_open and
        "candidate.sourceIdentity == shown.sourceIdentity" in stable_open,
        "legacy open must re-map by gameId + source identity after refresh, never stale list index")''',
    '''require("const TitleEntry selected =" in stable_open and
        "const std::string selectedGameId = selected.gameId;" in stable_open,
        "game open must snapshot stable game identity before validating the exact source")
require("shown.sourceIndex >= legacyCatalog->sources.size()" in stable_open and
        "cachedSource.gameId == selectedGameId" in stable_open and
        "cachedSource.normalizedPath == shown.normalizedPath" in stable_open and
        "cachedSource.contentFingerprint == shown.contentFingerprint" in stable_open and
        "legacySnapshotStillCurrent(shown)" in stable_open and
        "discoverConfiguredLegacySaves()" not in stable_open,
        "legacy open must validate only the exact cached source and avoid full provider rediscovery")''',
    "stable legacy open contract",
)

drawer_anchor = '''require('\"X: Save / Source   •   B: Close\"' not in drawer_draw,
        "Quick Games must rely on the footer for controls instead of repeating X/B instructions inside the drawer")'''
drawer_extra = drawer_anchor + '''
require("Quick Games is selection/navigation only" in drawer_update and
        "refreshHubSelectionFromCache();" in drawer_update and
        "refreshHubPreview();" not in drawer_update,
        "Quick Games A must close from cached state without synchronous save/launch discovery")'''
replace_once(drawer_anchor, drawer_extra, "Quick Games hotpath contract")

p.write_text(text)
print("updated hardware hotpath contracts")
