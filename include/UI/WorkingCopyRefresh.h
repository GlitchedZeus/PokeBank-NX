#pragma once

// Round 4 (R2-EMUBACKUP / S05): opening an installed Switch save must not create an unnecessary
// backup. The editor still needs a working copy of the installed save, and PKSE saves its edits
// straight into that folder, so the single reusable "Working" copy is refreshed like this:
//
//   1. an existing Working copy is moved aside (a hidden ".previous." sibling, which
//      listBackupDirectories never shows);
//   2. a fresh Working copy of the installed save is made (the caller's makeFresh, i.e.
//      Utils::backupSaveData(..., false));
//   3. if the old copy is byte-identical to the fresh one it held nothing new: it is deleted and NO
//      backup appears; if it differs it holds edits, so it is kept as a timestamped backup folder
//      (never silently discarded);
//   4. if the fresh copy fails, the old Working copy is put back exactly as it was.
//
// Deliberate backups are made only by the Save Backups "Create backup now" row.
// Header-only POSIX so the same code runs on host tests and libnx.

#include <algorithm>
#include <cstdio>
#include <dirent.h>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

namespace PokeBank::UIModel::WorkingCopy {

namespace detail {

inline bool isDir(const std::string& path) {
    struct stat st{};
    return ::lstat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode);
}
inline bool isFile(const std::string& path) {
    struct stat st{};
    return ::lstat(path.c_str(), &st) == 0 && S_ISREG(st.st_mode);
}
inline bool exists(const std::string& path) {
    struct stat st{};
    return ::lstat(path.c_str(), &st) == 0;
}
inline std::vector<std::string> children(const std::string& dir) {
    std::vector<std::string> names;
    if (DIR* d = ::opendir(dir.c_str())) {
        while (const dirent* e = ::readdir(d)) {
            const std::string name = e->d_name;
            if (name != "." && name != "..") names.push_back(name);
        }
        ::closedir(d);
    }
    std::sort(names.begin(), names.end());
    return names;
}
inline bool sameFile(const std::string& a, const std::string& b) {
    std::FILE* fa = std::fopen(a.c_str(), "rb");
    std::FILE* fb = std::fopen(b.c_str(), "rb");
    bool same = fa && fb;
    unsigned char ba[16 * 1024], bb[16 * 1024];
    while (same) {
        const std::size_t na = std::fread(ba, 1, sizeof(ba), fa);
        const std::size_t nb = std::fread(bb, 1, sizeof(bb), fb);
        if (na != nb || !std::equal(ba, ba + na, bb)) same = false;
        else if (na == 0) break;
    }
    if (fa) { same = same && !std::ferror(fa); std::fclose(fa); }
    if (fb) { same = same && !std::ferror(fb); std::fclose(fb); }
    return same;
}
inline bool removeTree(const std::string& path) {
    if (isDir(path)) {
        for (const std::string& name : children(path))
            if (!removeTree(path + "/" + name)) return false;
        return ::rmdir(path.c_str()) == 0;
    }
    return ::unlink(path.c_str()) == 0;
}

} // namespace detail

// Two directory trees hold exactly the same names and bytes.
[[nodiscard]] inline bool sameTree(const std::string& a, const std::string& b) {
    if (detail::isDir(a) != detail::isDir(b)) return false;
    if (!detail::isDir(a)) return detail::isFile(a) && detail::isFile(b) && detail::sameFile(a, b);
    const auto ca = detail::children(a), cb = detail::children(b);
    if (ca != cb) return false;
    for (const std::string& name : ca)
        if (!sameTree(a + "/" + name, b + "/" + name)) return false;
    return true;
}

enum class Outcome : unsigned char {
    Fresh,            // there was no earlier Working copy
    UnchangedDropped, // the earlier copy held nothing new: removed, no backup made
    EditsPreserved,   // the earlier copy differed: kept as a timestamped backup folder
    Failed,           // no fresh copy; the earlier Working copy (if any) was restored
};

struct Result {
    Outcome outcome = Outcome::Failed;
    std::string workingPath;   // the fresh Working copy to open
    std::string preservedPath; // EditsPreserved: where the earlier copy now lives
    std::string error;
};

// `workingPath` is <game backups>/Working; `stamp` is getTimestamp() (yyyymmdd_hhmmss);
// `makeFresh()` creates a fresh Working copy at workingPath and returns its path ("" on failure).
template <class MakeFresh>
[[nodiscard]] Result refresh(const std::string& workingPath, const std::string& stamp, MakeFresh&& makeFresh) {
    Result out;
    const auto slash = workingPath.find_last_of('/');
    if (workingPath.empty() || slash == std::string::npos || workingPath.substr(slash + 1) != "Working" ||
        stamp.empty() || stamp == "." || stamp == ".." || stamp.find_first_of("/\\\r\n") != std::string::npos) {
        out.error = "No safe working-copy path for this game.";
        return out;
    }
    const std::string gameDir = workingPath.substr(0, slash);

    std::string aside;
    if (detail::exists(workingPath)) {
        if (!detail::isDir(workingPath)) {
            out.error = "The working copy path is not a folder.";
            return out;
        }
        aside = workingPath + ".previous.open." + stamp;
        for (int n = 2; detail::exists(aside); ++n) aside = workingPath + ".previous.open." + stamp + "-" + std::to_string(n);
        if (std::rename(workingPath.c_str(), aside.c_str()) != 0) {
            out.error = "Could not set the earlier working copy aside.";
            return out;
        }
    }

    const std::string fresh = makeFresh();
    if (fresh != workingPath || !detail::isDir(fresh)) {
        // A failed copy may have left a partial Working folder. Remove only that app-owned
        // destination before restoring the old copy; never touch an unexpected returned path.
        const bool cleared = !detail::exists(workingPath) || detail::removeTree(workingPath);
        const bool restored = aside.empty() || (cleared && std::rename(aside.c_str(), workingPath.c_str()) == 0);
        if (!restored) out.preservedPath = aside;
        out.error = "Couldn't read the installed save into a working copy.";
        if (!restored) out.error += " The earlier copy is preserved at " + aside;
        return out;
    }
    out.workingPath = fresh;
    if (aside.empty()) {
        out.outcome = Outcome::Fresh;
        return out;
    }
    if (sameTree(aside, fresh)) {
        detail::removeTree(aside);   // held nothing new: no unnecessary backup
        out.outcome = Outcome::UnchangedDropped;
        return out;
    }
    // The earlier copy holds edits: keep it, visible as a dated backup, never deleted here.
    std::string keep = gameDir + "/" + stamp;
    for (int n = 2; detail::exists(keep); ++n) keep = gameDir + "/" + stamp + "-" + std::to_string(n);
    if (std::rename(aside.c_str(), keep.c_str()) != 0) keep = aside;   // still on the card, just hidden
    out.outcome = Outcome::EditsPreserved;
    out.preservedPath = keep;
    return out;
}

} // namespace PokeBank::UIModel::WorkingCopy
