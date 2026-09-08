#include "Legacy/LegacySourceBindings.h"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

#include <unistd.h>

namespace fs = std::filesystem;

int main() {
    char tempTemplate[] = "/tmp/pokebank-bindings-XXXXXX";
    const char* created = mkdtemp(tempTemplate);
    assert(created != nullptr);
    const fs::path temp(created);
    const fs::path database = temp / "legacy_source_bindings.cfg";

    PokeVault::Legacy::LegacySourceBindings bindings(database.string());
    assert(bindings.load());
    assert(bindings.size() == 0);
    assert(!bindings.isVisibleTo("source-a", "profile-a"));
    assert(bindings.assign("source-a", "profile-a"));
    assert(bindings.assign("source-b", "profile-b"));
    assert(!bindings.assign("", "profile-a"));
    assert(bindings.isVisibleTo("source-a", "profile-a"));
    assert(!bindings.isVisibleTo("source-a", "profile-b"));
    assert(bindings.save());

    // A fresh object models an application restart. Ownership remains private and exact.
    PokeVault::Legacy::LegacySourceBindings reloaded(database.string());
    assert(reloaded.load());
    assert(reloaded.size() == 2);
    assert(reloaded.assignedProfile("source-a") == "profile-a");
    assert(reloaded.isVisibleTo("source-a", "profile-a"));
    assert(!reloaded.isVisibleTo("source-a", "profile-b"));
    assert(reloaded.isVisibleTo("source-b", "profile-b"));

    // Reassigning the same stable physical identity updates one binding; an alias must not create
    // another row merely because discovery reached it through a second path.
    assert(reloaded.assign("source-a", "profile-b"));
    assert(reloaded.size() == 2);
    assert(!reloaded.isVisibleTo("source-a", "profile-a"));
    assert(reloaded.isVisibleTo("source-a", "profile-b"));
    assert(reloaded.unassign("source-a"));
    assert(!reloaded.isAssigned("source-a"));
    assert(!reloaded.unassign("source-a"));

    fs::remove_all(temp);
    std::cout << "Legacy source profile binding tests passed\n";
    return 0;
}
