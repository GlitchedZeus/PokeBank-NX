#pragma once

// Round 6 (7E gap G5): Gen I-III Export Copy proves the emulator's save file is unchanged since it
// was opened, as Gen IV (Gen4SourceSnapshot) and Gen V (staged baseline re-read) already do. Open
// takes a read-only physical snapshot of the exact file (ImmutableSourceSnapshot: no links, stable
// stamp, SHA-256) and checks it is the same bytes discovery strictly parsed; every Gen I-III export
// re-reads the file first and refuses if its stamp or bytes moved (an emulator saved meanwhile).
// The file is only ever read. Host-tested: tests/test_emulator_backup_access.cpp classicSourceGuard.

#include "Source/ImmutableSourceSnapshot.h"

#include <optional>
#include <string>

namespace PokeBank::UIModel::ClassicSourceGuard {

namespace Snap = PokeVault::Source::ImmutableSnapshot;

[[nodiscard]] inline std::string parentOf(const std::string& path) {
    const auto slash = path.find_last_of('/');
    return slash == std::string::npos || slash == 0 ? std::string{} : path.substr(0, slash);
}

// The snapshot taken at Open. `parsedFingerprint` is discovery's SHA-256 of the bytes it parsed;
// a different file now means the save changed between listing and Open, so Open is refused.
[[nodiscard]] inline std::optional<Snap::File> snapshotAtOpen(const std::string& path,
                                                              const std::string& parsedFingerprint,
                                                              std::string& error) {
    Snap::File file;
    const std::string root = parentOf(path);
    if (root.empty() || !Snap::read(path, root, {}, file, error)) {
        if (error.empty()) error = "The save file could not be read for a stable snapshot";
        return std::nullopt;
    }
    if (!parsedFingerprint.empty() && file.sha256 != parsedFingerprint) {
        error = "The save changed since it was listed; refresh its source and open it again";
        return std::nullopt;
    }
    error.clear();
    return file;
}

// Before an export. A session without a snapshot (none was taken) is not refused, as with Gen IV's
// optional Open snapshot; every Gen I-III open path takes one.
[[nodiscard]] inline bool unchangedSinceOpen(const std::optional<Snap::File>& snapshot, std::string& error) {
    if (!snapshot) return true;
    if (Snap::unchanged(*snapshot, error)) return true;
    error = "Not exported: the save file changed since it was opened (" + error +
            "). Reopen it; the staged edits were made against the earlier bytes.";
    return false;
}

} // namespace PokeBank::UIModel::ClassicSourceGuard
