// Feature C (owner Oct 28; PKSE f4fd79a / c671aa4, hardened): favourite and remembered folders.
#include "Utils/FolderPreferences.h"
#include "Utils/SettingsFile.h"

#include <cassert>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

namespace P = Utils::FolderPreferences;

int main() {
    // Canonical SD-card folders only.
    assert(P::canonical("sdmc:/") == "sdmc:/");
    assert(P::canonical("sdmc:/roms/gba/") == "sdmc:/roms/gba");
    assert(P::canonical("sdmc://roms//gba") == "sdmc:/roms/gba");
    for (const char* bad : {"", "sdmc:", "/roms", "romfs:/a", "sdmc:/a/../b", "sdmc:/./a", "sdmc:/a\\b",
                            "sdmc:/a:b", "save:/x"})
        assert(!P::canonical(bad));
    assert(!P::canonical(std::string("sdmc:/a\x01", 8)));
    assert(!P::canonical("sdmc:/" + std::string(Utils::SettingsFile::kMaxValue, 'a')));

    // Favourites: canonical, de-duplicated case-insensitively (FAT), toggled, bounded.
    P::Preferences p;
    assert(p.toggleFavorite("sdmc:/Roms/GBA/") && p.favorites.size() == 1 && p.favorites[0] == "sdmc:/Roms/GBA");
    assert(p.isFavorite("sdmc:/roms/gba") && p.isFavorite("sdmc://ROMS/gba/"));
    assert(p.toggleFavorite("sdmc:/roms/gba") && p.favorites.empty());       // same folder: removed
    assert(!p.toggleFavorite("") && !p.toggleFavorite("sdmc:/a/.."));
    for (int i = 0; i < 25; ++i) {
        const bool added = p.toggleFavorite("sdmc:/f" + std::to_string(i));
        assert(added == (i < static_cast<int>(P::kMaxFavorites)));
    }
    assert(p.favorites.size() == P::kMaxFavorites);

    // Remembered last folder per operation; a bad path never replaces a good one.
    assert(p.remember(P::Operation::ImportPokemonFile, "sdmc:/pk/"));
    assert(!p.remember(P::Operation::ImportPokemonFile, "") && p.lastImport == "sdmc:/pk");
    assert(p.lastExport.empty() && p.lastSaveCopy.empty());

    // Settings round trip; hand-edited duplicates and junk are dropped on load.
    const auto text = P::toSettingsText(p);
    const auto back = P::fromSettings(Utils::SettingsFile::parse(text));
    assert(back.favorites == p.favorites && back.lastImport == p.lastImport);
    const auto messy = P::fromSettings(Utils::SettingsFile::parse(
        "favoriteFolder=sdmc:/a/\nfavoriteFolder=SDMC:/a\nfavoriteFolder=sdmc:/A\nfavoriteFolder=\n"
        "favoriteFolder=../x\nlastImportFolder=sdmc:/../etc\nlastExportFolder=sdmc:/out//\n"));
    assert(messy.favorites.size() == 1 && messy.favorites[0] == "sdmc:/a");
    assert(messy.lastImport.empty() && messy.lastExport == "sdmc:/out");

    // A missing folder is reported, never removed; an empty one is missing (c671aa4).
    const auto exists = [](const std::string& f) { return f == "sdmc:/present"; };
    assert(P::status("sdmc:/present", exists) == P::FolderStatus::Present);
    assert(P::status("sdmc:/gone", exists) == P::FolderStatus::Missing);
    assert(P::status("", exists) == P::FolderStatus::Missing);

    // Wiring: settings load/save carry the preferences through the existing atomic writer.
    std::ifstream in("src/Utils/Settings.cpp");
    const std::string src{std::istreambuf_iterator<char>(in), {}};
    assert(src.find("g_folderPreferences = FolderPreferences::fromSettings(entries);") != std::string::npos);
    assert(src.find("text += FolderPreferences::toSettingsText(g_folderPreferences);") != std::string::npos);
    assert(src.find("AtomicTextFile::replace(settingsPath(), text)") != std::string::npos);
    std::cout << "Favourite and remembered folders: PASS\n";
    return 0;
}
