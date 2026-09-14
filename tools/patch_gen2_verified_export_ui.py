#!/usr/bin/env python3
from pathlib import Path

OVERLAY = Path("src/UI/TrainerViewScreenGSCOverlay.inc")
text = OVERLAY.read_text(encoding="utf-8")

include_anchor = '#include "Integration/Gen2/Gen2StagedEditor.h"\n'
verified_include = '#include "Integration/Gen2/Gen2ExportTransaction.h"\n'
if verified_include not in text:
    if include_anchor not in text:
        raise SystemExit("missing Gen2StagedEditor include anchor")
    text = text.replace(include_anchor, include_anchor + verified_include, 1)

# These headers existed solely for the retired direct-write exporter.
for line in ("#include <cerrno>\n", "#include <cstdio>\n", "#include <sys/stat.h>\n"):
    text = text.replace(line, "", 1)

start_marker = "bool mkdirIfNeeded(const std::string& path) {"
end_marker = "\nvoid openStagedEditor(TrainerViewScreen& screen) {"
start = text.find(start_marker)
end = text.find(end_marker, start if start >= 0 else 0)
if start < 0 or end < 0:
    if "publishVerifiedStagedEditorExport" not in text:
        raise SystemExit("legacy export block not found and verified replacement absent")
else:
    replacement = r'''std::string exportStagedCopy(TrainerViewScreen& screen, Gen2Editor& editor, std::string& error) {
    error.clear();
    if (!editor.hasPendingChanges()) {
        error = "No pending changes to export";
        return {};
    }

    // Path construction remains centralized in PokeBankPaths, but all save finalization and I/O are
    // owned by Gen2ExportTransaction. This wrapper deliberately contains no fopen/fwrite sequence.
    const std::string gen2Root = PokeBank::Paths::gen2ExportsRoot();
    if (!PokeBank::Paths::ensureGen2ExportsRoot(&error)) {
        if (error.empty()) error = "Could not create PokeBank NX export directory";
        return {};
    }

    const std::string timestamp = Utils::getTimestamp();
    const std::string proposed = PokeBank::Paths::gen2ExportDirectory(screen.sourceGameId, timestamp);
    const std::string rootPrefix = gen2Root + "/";
    if (proposed.empty() || proposed.rfind(rootPrefix, 0) != 0 || proposed.size() <= rootPrefix.size()) {
        error = "Could not construct safe PokeBank NX export path";
        return {};
    }

    PokeVault::Integration::Gen2::StagedEditorExportRequest request;
    request.rootDirectory = gen2Root;
    request.directoryStem = proposed.substr(rootPrefix.size());
    request.sourcePath = screen.backupDir;
    request.gameId = screen.sourceGameId;
    request.timestamp = timestamp;
#ifdef POKEVAULT_GIT_COMMIT
    request.applicationSha = POKEVAULT_GIT_COMMIT;
#else
    request.applicationSha = "unknown";
#endif

    const auto result = PokeVault::Integration::Gen2::publishVerifiedStagedEditorExport(editor, request);
    if (!result.success) {
        error = result.error.empty() ? "Verified staged export failed" : result.error;
        return {};
    }
    return result.directory;
}
'''
    text = text[:start] + replacement + text[end:]

old_ui = '''            if (exportDir.empty()) {
                screen.postStatus(error.empty() ? "Staged export failed" : error);
            } else {
                state.lastExportDirectory = exportDir;
                screen.postStatus("Exported edited copy + original backup to PokeBank NX exports/gen2");
            }
'''
new_ui = '''            if (exportDir.empty()) {
                const std::string reason = error.empty() ? "Verified staged export failed" : error;
                screen.postStatus(reason + "; source remains unchanged", 360);
            } else {
                state.lastExportDirectory = exportDir;
                screen.postStatus("Verified staged export: " + exportDir, 360);
            }
'''
if old_ui in text:
    text = text.replace(old_ui, new_ui, 1)
elif new_ui not in text:
    raise SystemExit("review export status block not found")

# Permanent safety assertions for the transformation itself.
required = (
    '#include "Integration/Gen2/Gen2ExportTransaction.h"',
    "publishVerifiedStagedEditorExport(editor, request)",
    "Verified staged export: ",
    "source remains unchanged",
)
for needle in required:
    if needle not in text:
        raise SystemExit(f"missing required verified-export UI marker: {needle}")
for forbidden in ("std::fopen", "std::fwrite", "writeBytes(", "writeText(", "pathExists(", "mkdirIfNeeded("):
    if forbidden in text:
        raise SystemExit(f"legacy direct-write marker still present in overlay: {forbidden}")

OVERLAY.write_text(text, encoding="utf-8")
print("Gen II UI export path now delegates exclusively to verified transaction workflow")
