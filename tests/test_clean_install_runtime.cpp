#include "Utils/PokeBankPaths.h"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <unistd.h>

int main() {
    namespace fs = std::filesystem;
    using namespace PokeBank::Paths;
    const fs::path sd = fs::temp_directory_path() /
        ("pokebank-clean-install-" + std::to_string(static_cast<long long>(getpid())));
    fs::remove_all(sd);
    fs::create_directories(sd);
    assert(!fs::exists(sd / "PKSE"));
    assert(!fs::exists(sd / "PKSM"));
    assert(!fs::exists(sd / "JKSV"));
    assert(!fs::exists(sd / "Checkpoint"));

    std::string error;
    const fs::path owned = sd / "switch/PokeBank-NX";
    assert(ensureDirectoryTree((owned / "config").string(), &error));
    assert(ensureDirectoryTree((owned / "backups").string(), &error));
    assert(ensureDirectoryTree((owned / "exports/gen2").string(), &error));
    assert(fs::is_directory(owned / "config"));
    assert(fs::is_directory(owned / "backups"));
    assert(fs::is_directory(owned / "exports/gen2"));
    assert(!fs::exists(sd / "PKSE"));

    const std::string game = sanitizeComponent("Gold / Test");
    assert(game == "Gold___Test");
    const fs::path exportDir = owned / "exports/gen2" /
        (game + "_20260912_173200");
    assert(ensureDirectoryTree(exportDir.string(), &error));
    for (const char* name : {"original_backup.srm", "edited.srm", "EDIT_MANIFEST.txt"}) {
        std::ofstream f(exportDir / name, std::ios::binary);
        f << "fixture";
    }
    assert(fs::exists(exportDir / "original_backup.srm"));
    assert(fs::exists(exportDir / "edited.srm"));
    assert(fs::exists(exportDir / "EDIT_MANIFEST.txt"));
    assert(!fs::exists(sd / "PKSE"));
    fs::remove_all(sd);
    std::cout << "Clean-install no-PKSE filesystem contract passed\n";
}
