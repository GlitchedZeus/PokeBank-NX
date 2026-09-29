#!/usr/bin/env python3
"""Static architecture gates for provider-neutral Save Instances.

These checks complement parser/fixture tests by pinning routing and bounded-provider invariants that
live in Switch UI code and therefore are not part of the ordinary host C++ link graph.
"""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

def text(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")

def require(haystack: str, needle: str, message: str) -> None:
    if needle not in haystack:
        raise AssertionError(message)

shared = text("include/Source/SaveInstance.h")
ui = text("src/UI/SaveSelectScreen.cpp")
legacy = text("src/Legacy/RetroArchFRLGDiscovery.cpp")
gen4 = text("src/Integration/Gen4/Gen4SourceDiscovery.cpp")
ui_manager = text("src/UI/UI.cpp")

for field in (
    "gameId", "generation", "providerId", "providerLabel", "sourcePath",
    "normalizedPath", "sourceIdentity", "physicalIdentity", "containerType",
    "modifiedTime", "fileSize", "trainerName", "partyCount", "validation",
    "access", "recoveredOlderCopy", "contentFingerprint", "claimedProfile",
    "rememberedSource", "sourceIndex",
):
    require(shared, field, f"shared SaveInstance lost required field: {field}")
require(shared, "appendDeduplicated", "shared physical-source dedupe helper missing")
require(shared, "sortNewestFirst", "shared newest-first ordering helper missing")
require(shared, "visibleToProfile", "shared profile visibility helper missing")

# Both classic and Gen IV render through one provider-neutral row component.
require(ui, "std::vector<PokeVault::Source::SaveInstance>& instances",
        "Save Instances row renderer is not provider-neutral")
require(ui, "drawSaveInstanceRows(fb, parent.legacyInstances",
        "classic Save Instances no longer use the shared row renderer")
require(ui, "drawSaveInstanceRows(fb, gen4Instances",
        "Gen IV Save Instances no longer use the shared row renderer")
require(ui, "appendDeduplicated(gen4Instances",
        "Gen IV chooser bypasses shared physical-source dedupe")
require(ui, "sortNewestFirst(gen4Instances)",
        "Gen IV chooser bypasses shared newest-first ordering")
require(ui, "visibleToProfile(instance, currentProfileIdentity())",
        "Gen IV chooser bypasses shared profile visibility")

# Refresh must re-run the complete configured provider set for the selected generation.
require(ui, "discoverConfiguredLegacySaves()",
        "classic refresh is no longer provider-complete")
require(ui, "discoverKnownSources()",
        "Gen IV refresh is no longer provider-complete")

# A remembered Gen IV binding names one exact source and may open directly only after strict
# read-only revalidation. Save Instances remains the provider-neutral source-management chooser for
# discovery, replacement, ambiguity, and manually chosen sources outside known roots.
select_start = ui.index("void SaveSelectScreen::selectCurrentTitle()")
select_end = ui.index("void SaveSelectScreen::selectCurrentLegacyInstance()", select_start)
select_body = ui[select_start:select_end]
require(select_body, "openAssignedSource(",
        "remembered Gen IV open no longer revalidates the exact assigned source")
require(select_body, "OpenStatus::Ready",
        "remembered Gen IV open no longer fails closed on validation status")
require(select_body, "openGen4Setup(",
        "invalid remembered Gen IV source no longer routes to safe source setup")
if "discoverGen4Candidates();" in select_body:
    raise AssertionError("normal remembered Gen IV open regressed into the candidate grid")
require(ui, "remembered.binding.sourcePath",
        "remembered/manual Gen IV source outside known roots is no longer folded into chooser")
require(ui, "appendReady(std::move(candidate), true);",
        "remembered/manual Gen IV source is not tagged in shared metadata")
require(ui, "drawSaveInstanceRows(fb, gen4Instances",
        "Gen IV source-management chooser lost provider-neutral Save Instances")

# Provider boundaries: configured mGBA only, exact Tico classic children only, no invented DS path.
require(legacy, '"savegamePath"', "mGBA discovery is no longer tied to configured battery storage")
require(legacy, 'for (const char* slug : {"gb", "gbc", "gba"})',
        "Tico classic discovery roots changed")
if "tico/saves/nds" in legacy.lower() or "tico/saves/nds" in gen4.lower():
    raise AssertionError("invented Tico DS save path detected")

# DraStic cartridge backups remain read-only candidates; .dss is explicitly a savestate.
require(gen4, 'extension(path) == ".dss"', "DraStic .dss rejection gate missing")
require(gen4, "UnsupportedSavestate", "DraStic savestate diagnostic state missing")
require(gen4, 'sdmc:/switch/drastic/user/backup', "verified DraStic backup root missing")

# UIManager keeps generation-specific strict open bridges. The shared layer is metadata/presentation,
# not a giant parser, and every external trainer view remains an external/read-only source kind.
require(ui_manager, "handleLegacyFRLGView", "classic strict open bridge missing")
require(ui_manager, "handleGen4View", "Gen IV strict open bridge missing")
require(ui_manager, "SourceKind::RetroArchLegacy", "classic external-source read-only kind missing")
require(ui_manager, "SourceKind::ExternalLegacy", "Gen IV external-source read-only kind missing")

for adapter in ("FRLG", "RBY", "GSC"):
    body = text(f"src/Legacy/{adapter}SourceBrowser.cpp")
    require(body, "appendDeduplicated", f"{adapter} bypasses shared dedupe")
    require(body, "applyClaims(instance)", f"{adapter} drops alias ownership")
require(ui, "sameValidatedSnapshot(shownInstance, freshInstance)", "stale source snapshot opens silently")
require(ui, "claimInstanceAndSave(entry.instance, profile)", "classic claim drops source aliases")
require(text(".github/workflows/audit-hardening-native.yml"), "include/Source/**", "shared source model changes skip native CI")

print("Provider-neutral Save Instances architecture contract: PASS")
