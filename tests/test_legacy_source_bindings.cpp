#include "Legacy/LegacySourceBindings.h"
#include <cassert>
#include <cerrno>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <unistd.h>

namespace fs = std::filesystem;
using PokeVault::Legacy::LegacySourceBindings;
namespace {
    std::string faultStage;
    std::string corruptStage;
    fs::path corruptPath;
    bool rejectRollback = false;
    int renameAttemptsOverExisting = 0;
    int renameNoReplace(const char* from, const char* to) {
        if (fs::exists(to)) {
            ++renameAttemptsOverExisting;
            errno = EEXIST;
            return -1;
        }
        if (rejectRollback && std::string(from).ends_with(".bak")) {
            errno = EBUSY;
            return -1;
        }
        return std::rename(from, to);
    }
    int fault(const char* stage) {
        if (faultStage == stage) { errno = EBUSY; return -1; }
        if (corruptStage == stage) {
            std::ofstream file(corruptPath, std::ios::binary);
            file << "truncated database";
        }
        return 0;
    }
    const PokeVault::Legacy::BindingFileOps ops{renameNoReplace, fault};
    std::string contents(const fs::path& path) {
        std::ifstream file(path, std::ios::binary);
        return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
    }
    void write(const fs::path& path, const std::string& bytes) {
        std::ofstream file(path, std::ios::binary); file << bytes; assert(file.good());
    }
    void verify(const fs::path& db, const std::string& leafOwner) {
        LegacySourceBindings loaded(db.string(), ops);
        assert(loaded.load());
        assert(loaded.size() == 2);
        assert(loaded.isVisibleTo("firered_gba:save-a", "will"));
        assert(!loaded.isVisibleTo("firered_gba:save-a", "niece"));
        assert(loaded.isVisibleTo("leafgreen_gba:save-b", leafOwner));
        assert(!loaded.isVisibleTo("leafgreen_gba:save-b",
                                   leafOwner == "will" ? "niece" : "will"));
    }
}
int main() {
    char temp[] = "/tmp/pokebank-bindings-XXXXXX";
    assert(mkdtemp(temp));
    const fs::path root(temp);
    for (const std::string leafOwner : {"will", "niece"}) {
        const fs::path db = root / (leafOwner + ".cfg");
        LegacySourceBindings bindings(db.string(), ops);
        assert(bindings.load() && bindings.size() == 0);
        assert(!bindings.assignAndSave("", "will"));
        assert(bindings.assignAndSave("firered_gba:save-a", "will"));
        const std::string first = contents(db);
        assert(bindings.assignAndSave("leafgreen_gba:save-b", leafOwner));
        assert(contents(db.string() + ".bak") == first);
        verify(db, leafOwner);
        // Same stable alias is one row; genuinely separate physical saves are separate rows.
        assert(bindings.assignAndSave("leafgreen_gba:save-b", leafOwner));
        assert(bindings.size() == 2);
        assert(bindings.assignAndSave("leafgreen_gba:independent-save", leafOwner));
        assert(bindings.size() == 3);
        assert(bindings.unassign("leafgreen_gba:independent-save") && bindings.save());
        verify(db, leafOwner);

        for (const char* stage : {"write-temp", "validate-temp", "retire-backup",
                                 "old-to-backup", "temp-to-target", "validate-target"}) {
            const std::string before = contents(db);
            faultStage = stage;
            assert(!bindings.assignAndSave("third-source", "will"));
            assert(!bindings.isAssigned("third-source"));
            assert(bindings.lastError().find(stage) != std::string::npos);
            assert(bindings.lastError().find("errno=") != std::string::npos);
            assert(contents(db) == before);
            faultStage.clear();
            verify(db, leafOwner);
            // Reassignment failure restores the prior owner, not merely "unassigned".
            faultStage = stage;
            assert(!bindings.assignAndSave("leafgreen_gba:save-b", "replacement"));
            assert(bindings.assignedProfile("leafgreen_gba:save-b") == leafOwner);
            faultStage.clear();
        }
        // If rollback itself is denied (e.g. busy handle), .bak survives and reload recovers.
        faultStage = "temp-to-target";
        rejectRollback = true;
        assert(!bindings.assignAndSave("third-source", "will"));
        assert(bindings.lastError().find("rollback") != std::string::npos);
        assert(fs::exists(db.string() + ".bak") && !fs::exists(db));
        faultStage.clear();
        rejectRollback = false;
        verify(db, leafOwner);
        // Next write recovers the backup before rotating it.
        assert(bindings.assignAndSave("leafgreen_gba:save-b", leafOwner));
        verify(db, leafOwner);

        const std::string valid = contents(db);
        for (const char* stage : {"validate-temp", "validate-target"}) {
            corruptStage = stage;
            corruptPath = std::string(stage) == "validate-temp" ? fs::path(db.string() + ".tmp") : db;
            assert(!bindings.assignAndSave("third-source", "will"));
            assert(!bindings.isAssigned("third-source"));
            assert(contents(db) == valid);
            assert(bindings.lastError().find(stage) != std::string::npos);
            corruptStage.clear();
            verify(db, leafOwner);
        }
        // Stale temp is never preferred over committed primary.
        write(db.string() + ".tmp", "partial");
        verify(db, leafOwner);
        // Interruption after old-to-backup: load previous DB, never speculative tmp.
        fs::remove(db.string() + ".bak");
        fs::rename(db, db.string() + ".bak");
        verify(db, leafOwner);
        assert(bindings.save());
        verify(db, leafOwner);
        // Corrupt primary + valid backup: read backup, refuse to overwrite the suspect primary.
        write(db.string() + ".bak", valid);
        write(db, "corrupt");
        LegacySourceBindings recovered(db.string(), ops);
        assert(recovered.load() && recovered.size() == 2);
        assert(!recovered.assignAndSave("third-source", "will"));
        assert(contents(db) == "corrupt");
        assert(contents(db.string() + ".bak") == valid);
        // An unreadable/malformed DB without backup cannot become an empty successful database.
        fs::remove(db.string() + ".bak");
        assert(!recovered.load());
        assert(recovered.size() == 2);
        assert(!recovered.save());
    }
    // First-save failure never claims a binding, even after restart.
    const fs::path firstDb = root / "first.cfg";
    LegacySourceBindings first(firstDb.string(), ops);
    faultStage = "validate-target";
    assert(!first.assignAndSave("firered_gba:save-a", "will"));
    assert(first.size() == 0);
    faultStage.clear();
    LegacySourceBindings empty(firstDb.string(), ops);
    assert(empty.load() && empty.size() == 0);
    // Actual I/O open failure, independent of the checkpoint seam.
    LegacySourceBindings noDirectory((root / "absent" / "cfg").string(), ops);
    assert(!noDirectory.assignAndSave("source", "will"));
    assert(noDirectory.size() == 0);
    assert(noDirectory.lastError().find("open-temp") != std::string::npos);
    LegacySourceBindings ordinary((root / "ordinary.cfg").string());
    assert(ordinary.assignAndSave("firered_gba:save-a", "will"));
    assert(ordinary.assignAndSave("leafgreen_gba:save-b", "niece"));
    verify(root / "ordinary.cfg", "niece");
    assert(renameAttemptsOverExisting == 0);
    fs::remove_all(root);
    std::cout << "Legacy binding transactions: first/second, isolation, rollback and recovery PASS\n";
}
