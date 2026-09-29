#include "UI/GameLauncher.h"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace fs = std::filesystem;

namespace {
void touch(const fs::path& path) {
    fs::create_directories(path.parent_path());
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    assert(out);
    out << "fixture";
}

void writePlaylist(const fs::path& path,
                   const std::vector<std::pair<std::string,std::string>>& entries) {
    fs::create_directories(path.parent_path());
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    assert(out);
    out << "{\n  \"items\": [\n";
    for (size_t i = 0; i < entries.size(); ++i) {
        out << "    {\"path\": \"" << entries[i].first
            << "\", \"core_path\": \"" << entries[i].second << "\"}";
        if (i + 1 != entries.size()) out << ",";
        out << "\n";
    }
    out << "  ]\n}\n";
}

void resetTree() {
    fs::remove_all("sdmc:");
    touch("sdmc:/retroarch/retroarch_switch.nro");
    touch("sdmc:/retroarch/cores/desmume_libretro_libnx.nro");
    touch("sdmc:/retroarch/cores/mgba_libretro_libnx.nro");
}
}

int main() {
    using namespace UI;

    // Same stem, wrong family first: .gba must be skipped and the unique .nds entry selected.
    resetTree();
    touch("sdmc:/roms/gba/Pokemon Platinum.gba");
    touch("sdmc:/roms/nds/Pokemon Platinum.nds");
    writePlaylist("sdmc:/retroarch/playlists/test.lpl", {
        {"/roms/gba/Pokemon Platinum.gba", "/retroarch/cores/mgba_libretro_libnx.nro"},
        {"/roms/nds/Pokemon Platinum.nds", "/retroarch/cores/desmume_libretro_libnx.nro"},
    });
    auto d = resolveGameLaunch(0, "platinum_nds", "retroarch",
                               "/saves/Pokemon Platinum.dsv", "");
    assert(d.state == GameLaunchState::Ready);
    assert(d.contentPath == "sdmc:/roms/nds/Pokemon Platinum.nds");

    // Two viable same-stem NDS entries are ambiguous. Directory/playlist order must not choose one.
    resetTree();
    touch("sdmc:/roms/a/Pokemon Platinum.nds");
    touch("sdmc:/roms/b/Pokemon Platinum.nds");
    writePlaylist("sdmc:/retroarch/playlists/test.lpl", {
        {"/roms/a/Pokemon Platinum.nds", "/retroarch/cores/desmume_libretro_libnx.nro"},
        {"/roms/b/Pokemon Platinum.nds", "/retroarch/cores/desmume_libretro_libnx.nro"},
    });
    d = resolveGameLaunch(0, "platinum_nds", "retroarch",
                          "/saves/Pokemon Platinum.dsv", "");
    assert(d.state == GameLaunchState::NeedsContentLink);
    assert(!d.ready());
    assert(d.detail.find("More than one") != std::string::npos);

    // Wrong-family only never becomes launch-ready merely because the stem matches.
    resetTree();
    touch("sdmc:/roms/gba/Pokemon Platinum.gba");
    writePlaylist("sdmc:/retroarch/playlists/test.lpl", {
        {"/roms/gba/Pokemon Platinum.gba", "/retroarch/cores/mgba_libretro_libnx.nro"},
    });
    d = resolveGameLaunch(0, "platinum_nds", "retroarch",
                          "/saves/Pokemon Platinum.dsv", "");
    assert(d.state == GameLaunchState::NeedsContentLink);
    assert(!d.ready());
    assert(d.detail.find("does not match this game family") != std::string::npos);

    fs::remove_all("sdmc:");
    std::cout << "RetroArch launch resolver ambiguity/family contract: PASS\n";
    return 0;
}
