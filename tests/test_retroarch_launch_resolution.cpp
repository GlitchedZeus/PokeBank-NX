#include "UI/GameLauncher.h"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

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

// Minimal cartridge images carrying only the header fields the identity check reads.
void writeRom(const fs::path& path, std::string_view ext, std::string_view id) {
    std::vector<unsigned char> rom(0x200, 0);
    if (ext == ".nds") {
        std::copy(id.begin(), id.end(), rom.begin() + 0x0C);
    } else if (ext == ".gba") {
        std::copy(id.begin(), id.end(), rom.begin() + 0xAC);
        rom[0xB2] = 0x96;
    } else {
        std::copy(id.begin(), id.end(), rom.begin() + 0x134);
        if (ext == ".gbc") rom[0x143] = 0x80;
    }
    fs::create_directories(path.parent_path());
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    out.write(reinterpret_cast<const char*>(rom.data()), static_cast<std::streamsize>(rom.size()));
    assert(out);
}

std::vector<unsigned char> header(std::string_view ext, std::string_view id) {
    writeRom("sdmc:/tmp/h" + std::string(ext), ext, id);
    std::ifstream in("sdmc:/tmp/h" + std::string(ext), std::ios::binary);
    return std::vector<unsigned char>(std::istreambuf_iterator<char>(in), {});
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

    // ROM header identity: pure classification of real header fields.
    resetTree();
    {
        auto id = [](std::string_view gameId, std::string_view path, std::string_view ext,
                     std::string_view code) {
            const auto h = header(ext, code);
            return gameLaunchRomIdentity(gameId, path, h.data(), h.size());
        };
        using R = GameLaunchRomIdentity;
        assert(id("red_gb", "a.gb", ".gb", "POKEMON RED") == R::Matches);
        assert(id("red_gb", "a.gb", ".gb", "POKEMON ROUGE") == R::Matches);
        assert(id("blue_gb", "a.gb", ".gb", "POKEMON BLU") == R::Matches);
        assert(id("blue_gb", "a.gb", ".gb", "POKEMON RED") == R::Contradicts);
        assert(id("yellow_gb", "a.gb", ".gb", "POKEMON YELAPSF") == R::Matches);
        assert(id("gold_gbc", "a.gbc", ".gbc", "POKEMON_GLDAAUE") == R::Matches);
        assert(id("silver_gbc", "a.gbc", ".gbc", "POKEMON_GLDAAUE") == R::Contradicts);
        assert(id("crystal_gbc", "a.gbc", ".gbc", "PM_CRYSTAL") == R::Matches);
        assert(id("emerald_gba", "a.gba", ".gba", "BPEE") == R::Matches);
        assert(id("ruby_gba", "a.gba", ".gba", "AXPE") == R::Contradicts);
        assert(id("leafgreen_gba", "a.gba", ".gba", "BPGD") == R::Matches);
        assert(id("heartgold_nds", "a.nds", ".nds", "IPKE") == R::Matches);
        assert(id("soulsilver_nds", "a.nds", ".nds", "IPKE") == R::Contradicts);
        assert(id("platinum_nds", "a.nds", ".nds", "CPUJ") == R::Matches);
        // Unova: BW and B2W2 cartridge codes are distinct titles.
        assert(id("black_nds", "a.nds", ".nds", "IRBO") == R::Matches);
        assert(id("white_nds", "a.nds", ".nds", "IRAO") == R::Matches);
        assert(id("black2_nds", "a.nds", ".nds", "IREO") == R::Matches);
        assert(id("white2_nds", "a.nds", ".nds", "IRDO") == R::Matches);
        assert(id("black_nds", "a.nds", ".nds", "IREO") == R::Contradicts);
        assert(id("white2_nds", "a.nds", ".nds", "IRAO") == R::Contradicts);
        assert(id("black_nds", "a.nds", ".nds", "ADAE") == R::Contradicts);
        // Unknown header (hack/other) and archives never block a launch.
        assert(id("red_gb", "a.gb", ".gb", "ZELDA") == R::Unknown);
        assert(id("platinum_nds", "a.zip", ".nds", "ADAE") == R::Unknown);
        const unsigned char tiny[4] = {};
        assert(gameLaunchRomIdentity("red_gb", "a.gb", tiny, sizeof tiny) == R::Unknown);
    }

    // A same-stem file whose header is a DIFFERENT game is never launch-ready...
    resetTree();
    writeRom("sdmc:/roms/nds/Pokemon Platinum.nds", ".nds", "ADAE");
    writePlaylist("sdmc:/retroarch/playlists/test.lpl", {
        {"/roms/nds/Pokemon Platinum.nds", "/retroarch/cores/desmume_libretro_libnx.nro"},
    });
    d = resolveGameLaunch(0, "platinum_nds", "retroarch",
                          "/saves/Pokemon Platinum.dsv", "");
    assert(!d.ready());

    // ...and it no longer makes a genuine Platinum dump ambiguous.
    resetTree();
    writeRom("sdmc:/roms/a/Pokemon Platinum.nds", ".nds", "CPUE");
    writeRom("sdmc:/roms/b/Pokemon Platinum.nds", ".nds", "ADAE");
    writePlaylist("sdmc:/retroarch/playlists/test.lpl", {
        {"/roms/a/Pokemon Platinum.nds", "/retroarch/cores/desmume_libretro_libnx.nro"},
        {"/roms/b/Pokemon Platinum.nds", "/retroarch/cores/desmume_libretro_libnx.nro"},
    });
    d = resolveGameLaunch(0, "platinum_nds", "retroarch",
                          "/saves/Pokemon Platinum.dsv", "");
    assert(d.state == GameLaunchState::Ready);
    assert(d.contentPath == "sdmc:/roms/a/Pokemon Platinum.nds");

    // Manual Link Game File refuses a file whose header names another Pokémon game.
    resetTree();
    writeRom("sdmc:/roms/gb/Pokemon Red.gb", ".gb", "POKEMON BLUE");
    std::string linkError;
    assert(!saveGameLaunchBinding("profile|red_gb|src", "red_gb", "retroarch",
                                  "sdmc:/roms/gb/Pokemon Red.gb", linkError));
    assert(linkError.find("different Pok") != std::string::npos);

    fs::remove_all("sdmc:");
    std::cout << "RetroArch launch resolver ambiguity/family contract: PASS\n";
    return 0;
}
