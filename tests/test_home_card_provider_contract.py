#!/usr/bin/env python3
"""Static gates for Home card providers and the current-save snapshot (Gen I-V).

The C++ host tests pin the provider table, label fitting and the snapshot cache.
These checks pin how the Switch-only SaveSelectScreen wires them, because that
file is not part of the host link graph.
"""
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]


def text(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


def require(haystack: str, *needles: str) -> None:
    for needle in needles:
        if needle not in haystack:
            raise AssertionError(f"missing: {needle}")


def forbid(haystack: str, *needles: str) -> None:
    for needle in needles:
        if needle in haystack:
            raise AssertionError(f"forbidden: {needle}")


ui = text("src/UI/SaveSelectScreen.cpp")
header = text("include/UI/SaveSelectScreen.h")
identity = text("include/Source/SaveProviderIdentity.h")
snapshot = text("include/Source/HomeCardSnapshot.h")

# P6: the generic "Linked save" label is gone; providers come from metadata.
forbid(ui, '"Linked save"', 'raw == "REMEMBERED"')
require(ui, "PokeVault::Source::homeCardSourceText(raw, provider)",
        "card.providerLabel = assigned.binding.sourceType;",
        "entry.providerLabel = PokeVault::Source::cardProviderLabel(card.instances);",
        "installed.providerLabel = PokeVault::Source::cardProviderLabel(card->instances);",
        "PokeVault::Source::saveInstanceSourceLine(instance)",
        "PokeVault::Source::homeSourceLine(")
require(header, "std::string providerLabel;", "bool linkedSave = false;")

# Every provider label any discovery/manual-link path writes is recognised;
# nothing falls through to a generation default.
known = set(re.findall(r'equalsIgnoreCase\(raw, "([^"]+)"\)', identity))
written = set()
for path in ("src/Integration/Gen4/Gen4SourceDiscovery.cpp",
             "src/Integration/Gen5/Gen5SourceDiscovery.cpp"):
    written |= set(re.findall(r'\{\s*"sdmc:[^"]*"\s*,\s*"([^"]+)"', text(path)))
    written |= set(re.findall(r'DiscoveryRoot\{"[^"]*",\s*"([^"]+)"', text(path)))
    written |= set(re.findall(r'roots\.push_back\(\{[^,]+,\s*"([^"]+)"', text(path)))
legacy = text("src/Legacy/RetroArchFRLGDiscovery.cpp")
written |= set(re.findall(r'appendProvider\([^;]*?,\s*"([^"]+)"', legacy))
written |= set(re.findall(r'std::string_view provider = "([^"]+)"', legacy))
for browser in ("src/Legacy/RBYSourceBrowser.cpp", "src/Legacy/GSCSourceBrowser.cpp"):
    written |= set(re.findall(r'instance\.providerLabel = "([^"]+)"', text(browser)))
written |= set(re.findall(r'inspectSource(?:File|Choices)\(\s*chosen\.text,\s*"([^"]+)"', ui))
written.add("RetroArch/DeSmuME")  # Gen IV sourceTypeForPath for .dsv under RetroArch
assert {"DraStic", "melonDS", "RetroArch", "mGBA", "Tico", "Manual"} <= written, written
unknown = sorted(w for w in written if w.lower() not in {k.lower() for k in known})
assert not unknown, f"discovery writes provider labels the Home card cannot name: {unknown}"
require(identity, 'return "Unknown provider";', 'return "Manual file";')

# Labels are measured and ellipsized to the space they own.
require(ui, "std::string fitTextToWidth(PKSEFramebuffer& fb",
        "fitTextToWidth(fb, sourceLine, 326, TextStyle::Body)",
        "fitTextToWidth(fb, meta, tileTextW, TextStyle::Caption)",
        "fitTextToWidth(fb, title.label, tileTextW, TextStyle::Caption)",
        "fitTextToWidth(fb, sourceLine, width - 88, TextStyle::Caption)",
        "const std::string status = fitTextToWidth(fb, partyPreviewStatus,",
        "tileW - 10, TextStyle::Caption);")
forbid(ui, "if (meta.size() > 24)", "if (sourceLine.size() > 38)",
       "if (status.size() > 62)", "if (label.size() > 18)")

# P5: current validated snapshot for Gen IV/V, never a "cached party".
forbid(ui, "cached party", "title.gen5Preview", "card.gen5Preview")
forbid(header, "gen5Preview")
require(ui, "homeSnapshots.refresh(request, statSaveFile,",
        "PokeVault::Integration::Gen5::homeCardSnapshot(",
        "PokeVault::Integration::Gen4::homeCardSnapshot(",
        "applyAssignedCardState(entry, profile);",
        "applyAssignedCardState(card,profile);")
# The fast L/R path is a lookup only: no stat, no parse.
fast = ui.split("void SaveSelectScreen::refreshHubSelectionFromCache()", 1)[1]
fast = fast.split("void SaveSelectScreen::refreshHubPreview(bool resolveLaunchTarget)", 1)[0]
require(fast, "homeSnapshots.find(")
forbid(fast, "openAssignedSource(", "statSaveFile(", "homeSnapshots.refresh(",
       "syncFocusedSaveSnapshot(")
# Every full refresh (start, profile, assignment, editor return) first
# re-checks the focused save; the hub then only reads the cache.
full = ui.split("void SaveSelectScreen::refreshHubPreview(bool resolveLaunchTarget)", 1)[1]
assert full.lstrip("{ \n").split("\n", 3)[2].strip() == "syncFocusedSaveSnapshot();", \
    "refreshHubPreview must re-check the focused save first"
hub = full.split("void SaveSelectScreen::openGameFilePicker(", 1)[0]
forbid(hub, "Gen4::openAssignedSource(", "Gen5::openAssignedSource(")
# Emulator save watch: periodic stat of the focused card on Product Home only.
require(ui, "constexpr int SAVE_WATCH_INTERVAL_FRAMES = 90;",
        "if (overlay == Overlay::None && !classicGamesActive && !titleSelected) {",
        "pollFocusedSaveChange();")
watch = ui.split("bool SaveSelectScreen::syncFocusedSaveSnapshot()", 1)[1]
watch = watch.split("void SaveSelectScreen::rebuildUnassignedLegacySources()", 1)[0]
require(watch, "refreshAssignedSnapshot(profile, title, &outcome);",
        "discoverConfiguredLegacySaves()", "if (watchKey == legacyWatchKey) return false;")
# Missing / invalid / ambiguous / unassigned stay distinct; no stale snapshot
# survives them.
cards = ui.split("void SaveSelectScreen::applyAssignedCardState(", 1)[1]
cards = cards.split("void SaveSelectScreen::applySnapshotToHub(", 1)[0]
require(cards, 'card.sourceLabel = "MISSING";', 'card.sourceLabel = "INVALID";',
        'card.sourceLabel = "AMBIGUOUS";', "homeSnapshots.invalidate(",
        '"REMEMBERED / CHOOSE COPY"', '"REMEMBERED / NEW-GAME SAVE, NO TRAINER YET"',
        '"REMEMBERED / STALE"')
require(snapshot, "SnapshotState::Stale", '"Linked save file is missing."',
        '"Current save party"')
forbid(snapshot, "cached")

# Gen I-III hub output stays on its accepted path (only provider text changed).
require(ui, ': "Choose a Save Instance to view its party.";',
        "addParty(i, party[i].species, party[i].level, 0, false);",
        '? "Current save party" : "No active party Pokémon.";')

print("Home card providers/snapshot contract: PASS")
