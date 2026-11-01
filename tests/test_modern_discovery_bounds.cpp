// QA D3: bounded, symlink-safe Save Instances discovery for Modern-adapter titles
// (src/Integration/Modern/ModernSources.cpp). Everything runs in a temporary tree.
//   * a symlinked Checkpoint root, title folder, backup folder or `main` is never followed, and a
//     save reachable only through such a link (outside the root) is never listed;
//   * readSourceFileReadOnly refuses a symlinked file and any symlinked ancestor (manual paths too);
//   * a large backup tree (408 backups) inspects only the newest `maxInspected`, newest-first and
//     deterministically, and reports exactly what was not inspected;
//   * directory/candidate limits are reported; the UI notice text names them;
//   * deduplication: one physical file seen twice merges; identical copies in two folders stay two;
//   * an explicitly assigned file older than the inspected window still opens and still validates
//     through the caller's explicit inspection (limits never invalidate an assignment).

#include "Integration/Modern/ModernSources.h"
#include "fixtures/gen6/Gen6SyntheticSave.h"

#include <chrono>
#include <cstdio>
#include <ctime>
#include <fstream>
#include <iostream>
#include <string>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <vector>

namespace Modern = PokeVault::Integration::Modern;
using PokeVault::Legacy::LegacySourceBindings;

namespace {
int failures = 0, checks = 0;
void check(bool ok, const std::string& what) {
    ++checks;
    if (!ok) {
        std::cerr << "FAILED: " << what << '\n';
        ++failures;
    }
}
std::string makeTempDir() {
    char pattern[] = "/tmp/modern-discovery-XXXXXX";
    const char* dir = ::mkdtemp(pattern);
    return dir ? dir : "";
}
bool writeFile(const std::string& path, const std::vector<uint8_t>& bytes) {
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    out.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    return static_cast<bool>(out);
}
void setMtime(const std::string& path, std::time_t when) {
    struct timeval tv[2] = {{when, 0}, {when, 0}};
    ::utimes(path.c_str(), tv);
}
void mkdirs(const std::string& path) {
    for (size_t i = 1; i <= path.size(); ++i)
        if (i == path.size() || path[i] == '/') ::mkdir(path.substr(0, i).c_str(), 0755);
}
Modern::DiscoveryRoots rootsFor(const std::string& checkpoint, const std::string& root) {
    Modern::DiscoveryRoots r;
    r.checkpointRoot = checkpoint;
    r.dekoponPointerFile = root + "/no-pointer.txt";
    r.dekoponDefaultUserDir = root + "/no-dekopon/";
    return r;
}
constexpr const char* XTitle = "0x0055D Pokemon X";
} // namespace

int main() {
    const auto xSave = Gen6Fixture::makeSave({});

    // ---- symlinks are never followed ----
    {
        const std::string root = makeTempDir();
        const std::string outside = root + "/outside";            // NOT under the Checkpoint root
        mkdirs(outside + "/" + XTitle + "/escape");
        check(writeFile(outside + "/" + XTitle + "/escape/main", xSave), "outside save written");
        mkdirs(outside + "/loose");
        check(writeFile(outside + "/loose/main", xSave), "loose outside save written");

        const std::string checkpoint = root + "/Checkpoint/saves";
        mkdirs(checkpoint + "/" + XTitle + "/real");
        check(writeFile(checkpoint + "/" + XTitle + "/real/main", xSave), "real save written");
        // symlinked backup folder and symlinked `main` inside the real title folder
        check(::symlink((outside + "/loose").c_str(), (checkpoint + "/" + XTitle + "/linked-backup").c_str()) == 0, "backup link");
        mkdirs(checkpoint + "/" + XTitle + "/linked-file");
        check(::symlink((outside + "/loose/main").c_str(), (checkpoint + "/" + XTitle + "/linked-file/main").c_str()) == 0, "file link");
        // symlinked title folder pointing outside the root
        check(::symlink((outside + "/" + XTitle).c_str(), (checkpoint + "/0x0055D Pokemon X (linked)").c_str()) == 0, "title link");

        const auto found = Modern::discoverKnownSources("x_3ds", rootsFor(checkpoint, root));
        check(found.instances.size() == 1 && found.instances.front().path() == checkpoint + "/" + XTitle + "/real/main",
              "only the real in-root save is listed");
        check(found.symlinksSkipped == 3, "three links skipped (title folder, backup folder, main): " +
                                              std::to_string(found.symlinksSkipped));
        for (const auto& i : found.instances)
            check(i.path().find("/outside/") == std::string::npos, "no out-of-root save exposed");

        // A symlinked root is not opened at all.
        check(::symlink(checkpoint.c_str(), (root + "/linked-root").c_str()) == 0, "root link");
        const auto viaLink = Modern::discoverKnownSources("x_3ds", rootsFor(root + "/linked-root", root));
        check(viaLink.rootsOpened == 0 && viaLink.instances.empty() && viaLink.symlinksSkipped == 1, "symlinked root never opened");
        // A real root under a symlinked ancestor is not opened either.
        check(::symlink((root + "/Checkpoint").c_str(), (root + "/linked-parent").c_str()) == 0, "parent link");
        const auto viaParent = Modern::discoverKnownSources("x_3ds", rootsFor(root + "/linked-parent/saves", root));
        check(viaParent.rootsOpened == 0 && viaParent.instances.empty(), "root under a symlinked parent never opened");

        // Direct reads (manual paths, remembered paths) refuse links too.
        std::vector<uint8_t> bytes;
        std::string why;
        check(!Modern::readSourceFileReadOnly(checkpoint + "/" + XTitle + "/linked-file/main", bytes, why) &&
                  why.find("symbolic link") != std::string::npos,
              "a symlinked file is refused: " + why);
        check(!Modern::readSourceFileReadOnly(checkpoint + "/" + XTitle + "/linked-backup/main", bytes, why) &&
                  why.find("symbolic link") != std::string::npos,
              "a file under a symlinked folder is refused: " + why);
        check(!Modern::readSourceFileReadOnly(root + "/linked-parent/saves/" + XTitle + "/real/main", bytes, why),
              "a file under a symlinked ancestor is refused");
        check(Modern::readSourceFileReadOnly(checkpoint + "/" + XTitle + "/real/main", bytes, why) && bytes == xSave,
              "the real file still reads byte-exactly");
        check(!Modern::inspectSourceFile(checkpoint + "/" + XTitle + "/linked-file/main", "Manual", "x_3ds").ready(),
              "a manual link through a symlink never validates");
        check(!Modern::pathHasNoSymlinkAncestors("sdmc:/no-such-dir/x/main") &&
                  Modern::pathHasNoSymlinkAncestors(checkpoint + "/" + XTitle + "/real/main") &&
                  !Modern::pathHasNoSymlinkAncestors(checkpoint + "/" + XTitle + "/linked-backup/main"),
              "ancestor check: device prefix skipped, missing/linked ancestors refused, real path accepted");
        std::remove((root + "/linked-root").c_str());
    }

    // ---- 408 backups: bounded, deterministic newest-first, reported ----
    {
        const std::string root = makeTempDir();
        const std::string checkpoint = root + "/Checkpoint/saves";
        const std::string title = checkpoint + "/" + XTitle;
        mkdirs(title);
        const std::string seed = root + "/seed-main";
        check(writeFile(seed, xSave), "seed written");
        const std::time_t base = 1700000000;
        // 30 distinct copies with distinct mtimes; 378 hard links of one old file (cheap, shared mtime).
        int linkFailures = 0;
        for (int i = 0; i < 408; ++i) {
            char name[32];
            std::snprintf(name, sizeof name, "backup-%03d", i);
            mkdirs(title + "/" + name);
            const std::string file = title + "/" + name + "/main";
            if (i < 30) {
                writeFile(file, xSave);
                setMtime(file, base + 1000 + i);   // backup-029 is the newest
            } else {
                if (::link(seed.c_str(), file.c_str()) != 0) ++linkFailures;
            }
        }
        check(linkFailures == 0, "378 hard links created");
        setMtime(seed, base);
        Modern::DiscoveryLimits limits;
        limits.maxInspected = 24;
        const auto t0 = std::chrono::steady_clock::now();
        const auto found = Modern::discoverKnownSources("x_3ds", rootsFor(checkpoint, root), limits);
        const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - t0).count();
        std::cout << "408-backup bounded discovery: " << ms << " ms (" << found.filesExamined << " inspected)\n";
        check(found.candidatesFound == 408 && found.filesExamined == 24 && found.candidatesNotInspected == 384 && found.truncated(),
              "only the newest 24 of 408 are read; 384 reported as not inspected");
        check(found.instances.size() == 24, "24 validated rows");
        bool newestFirst = !found.instances.empty() && found.instances.front().path() == title + "/backup-029/main";
        for (size_t i = 1; i < found.instances.size(); ++i)
            newestFirst = newestFirst && found.instances[i - 1].modifiedTime >= found.instances[i].modifiedTime;
        check(newestFirst, "rows are newest-first, backup-029 first");
        for (const auto& r : found.instances)
            check(r.modifiedTime >= base + 1000 + 6, "every inspected row is among the 24 newest copies");
        const auto again = Modern::discoverKnownSources("x_3ds", rootsFor(checkpoint, root), limits);
        bool same = again.instances.size() == found.instances.size();
        for (size_t i = 0; same && i < again.instances.size(); ++i) same = again.instances[i].path() == found.instances[i].path();
        check(same, "a second scan selects the same rows in the same order (deterministic)");
        const auto notice = Modern::discoveryLimitNotice(found, "Gen VI");
        check(notice.find("newest 24 of 408") != std::string::npos && notice.find("384 older") != std::string::npos &&
                  notice.find("Assign Save File Manually") != std::string::npos,
              "UI notice names the truncation and the manual route: " + notice);

        // Directory entry limit.
        Modern::DiscoveryLimits tight;
        tight.maxEntriesPerDirectory = 100;
        const auto capped = Modern::discoverKnownSources("x_3ds", rootsFor(checkpoint, root), tight);
        check(capped.directoryLimitHit && capped.truncated() && capped.candidatesFound <= 100,
              "a folder larger than the entry limit is reported, never fully scanned");
        check(Modern::discoveryLimitNotice(capped, "Gen VI").find("folder size limit") != std::string::npos, "limit notice text");
        Modern::DiscoveryLimits fewCandidates;
        fewCandidates.maxCandidates = 50;
        const auto c2 = Modern::discoverKnownSources("x_3ds", rootsFor(checkpoint, root), fewCandidates);
        check(c2.candidateLimitHit && c2.candidatesFound == 50, "candidate limit reported");
        check(Modern::discoveryLimitNotice(Modern::DiscoveryResult{}, "Gen VI").empty(), "no notice for an unbounded result");

        // ---- an explicit assignment outside the inspected window stays valid ----
        LegacySourceBindings bindings(root + "/bindings.cfg");
        const std::string old = title + "/backup-000/main";  // copy with the oldest distinct mtime: not inspected
        bool inWindow = false;
        for (const auto& r : found.instances) inWindow |= r.path() == old;
        check(!inWindow, "the old backup is outside the inspected window");
        const auto oldRow = Modern::inspectSourceFile(old, Modern::kProviderCheckpoint, "x_3ds");
        check(oldRow.ready(), "explicit inspection of the old backup validates: " + oldRow.diagnostic);
        auto catalog = Modern::forGameAndProfile(std::vector<PokeVault::Source::SaveInstance>{oldRow}, bindings, "p1", "x_3ds");
        const auto assigned = catalog.rows.empty() ? Modern::SourceAssignmentResult{}
                                                   : Modern::assignValidatedSource(bindings, catalog.rows.front(), "p1", "x_3ds");
        check(assigned.success, "old backup assigned explicitly: " + assigned.diagnostic);
        check(Modern::openAssignedSource(bindings, "p1", "x_3ds").ready(), "the assignment opens although discovery would not list it");
        // The UI re-adds the remembered file to the discovered rows: it is listed, once.
        auto rows = found.instances;
        rows.push_back(Modern::inspectSourceChoices(old, Modern::kProviderCheckpoint, "x_3ds").front());
        rows.push_back(Modern::inspectSourceChoices(old, Modern::kProviderCheckpoint, "x_3ds").front());
        const auto withRemembered = Modern::forGameAndProfile(rows, bindings, "p1", "x_3ds");
        size_t oldRows = 0;
        for (const auto& r : withRemembered.rows) oldRows += r.path() == old ? 1 : 0;
        check(oldRows == 1 && withRemembered.rows.size() == 25, "deduplicated: the remembered file is listed exactly once");
        bool remembered = false;
        for (const auto& r : withRemembered.rows) remembered |= r.path() == old && r.rememberedSource;
        check(remembered, "the remembered row is marked as the assignment");
        // Identical bytes in two different backup folders remain two physical choices.
        size_t distinct = 0;
        for (const auto& r : found.instances) distinct += r.contentFingerprint == found.instances.front().contentFingerprint ? 1 : 0;
        check(distinct == 24, "identical-content copies in different folders stay separate choices");
    }

    std::cout << "test_modern_discovery_bounds: " << (failures ? "FAILED " : "all ") << checks << " checks"
              << (failures ? " (" + std::to_string(failures) + " failed)" : " passed") << '\n';
    return failures ? 1 : 0;
}
