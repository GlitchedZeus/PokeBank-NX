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

    // ---- RetroArch core is chosen from the save's own evidence (round 3 defect 1) ----
    const std::string desmume = "sdmc:/retroarch/cores/desmume_libretro_libnx.nro";
    const std::string melonds = "sdmc:/retroarch/cores/melonds_libretro_libnx.nro";
    const std::string gambatte = "sdmc:/retroarch/cores/gambatte_libretro_libnx.nro";
    const std::string mgba = "sdmc:/retroarch/cores/mgba_libretro_libnx.nro";
    auto ndsFixture = [&]() {
        resetTree();
        touch(melonds);
        writeRom("sdmc:/roms/nds/Pokemon Platinum.nds", ".nds", "CPUE");
        // RetroArch last ran this ROM with DeSmuME; that must not override the save's core.
        writePlaylist("sdmc:/retroarch/playlists/Nintendo - Nintendo DS.lpl", {
            {"/roms/nds/Pokemon Platinum.nds", "/retroarch/cores/desmume_libretro_libnx.nro"},
        });
    };

    // melonDS per-core save folder: launch melonDS even though DeSmuME is the ID default.
    ndsFixture();
    d = resolveGameLaunch(0, "platinum_nds", "retroarch",
                          "sdmc:/retroarch/saves/melonDS/Pokemon Platinum.sav", "");
    assert(d.state == GameLaunchState::Ready);
    assert(d.corePath == melonds && d.launcherPath == melonds);
    assert(d.contentPath == "sdmc:/roms/nds/Pokemon Platinum.nds");
    assert(d.launchNotice.empty());

    // Unsorted saves: the melonDS core writes <game>.sav, DeSmuME writes <game>.dsv.
    ndsFixture();
    d = resolveGameLaunch(0, "platinum_nds", "retroarch",
                          "sdmc:/retroarch/saves/Pokemon Platinum.sav", "");
    assert(d.state == GameLaunchState::Ready && d.corePath == melonds);
    d = resolveGameLaunch(0, "platinum_nds", "retroarch",
                          "sdmc:/retroarch/saves/Pokemon Platinum.dsv", "");
    assert(d.state == GameLaunchState::Ready && d.corePath == desmume);

    // Proven core not installed: say so, never fall back to a core that cannot load the save.
    ndsFixture();
    fs::remove(melonds);
    d = resolveGameLaunch(0, "platinum_nds", "retroarch",
                          "sdmc:/retroarch/saves/melonDS/Pokemon Platinum.sav", "");
    assert(d.state == GameLaunchState::LauncherMissing);
    assert(d.detail.find("melonDS") != std::string::npos);
    assert(d.corePath != desmume);

    // Contradictory evidence keeps the previous choice (the playlist core) and says so.
    ndsFixture();
    d = resolveGameLaunch(0, "platinum_nds", "retroarch",
                          "sdmc:/retroarch/saves/melonDS/Pokemon Platinum.dsv", "");
    assert(d.state == GameLaunchState::Ready && d.corePath == desmume);
    assert(d.launchNotice.find("Core unproven") != std::string::npos);
    assert(gameLaunchDialogNotice(d).find("Core unproven") != std::string::npos);
    assert(gameLaunchDialogNotice(d).find("Homebrew Menu") != std::string::npos);

    // A remembered game-file link recomputes the core from the same evidence.
    ndsFixture();
    {
        std::string bindError;
        assert(saveGameLaunchBinding("profile|platinum_nds|src", "platinum_nds", "retroarch",
                                     "sdmc:/roms/nds/Pokemon Platinum.nds", bindError));
        d = resolveGameLaunch(0, "platinum_nds", "retroarch",
                              "sdmc:/retroarch/saves/melonDS/Pokemon Platinum.sav",
                              "profile|platinum_nds|src");
        assert(d.state == GameLaunchState::Ready && d.corePath == melonds);
        d = resolveGameLaunch(0, "platinum_nds", "retroarch",
                              "sdmc:/retroarch/saves/DeSmuME/Pokemon Platinum.dsv",
                              "profile|platinum_nds|src");
        assert(d.state == GameLaunchState::Ready && d.corePath == desmume);
    }

    // Game Boy: an mGBA per-core folder beats the Gambatte default; no evidence keeps it.
    resetTree();
    touch(gambatte);
    writeRom("sdmc:/roms/gb/Pokemon Red.gb", ".gb", "POKEMON RED");
    writePlaylist("sdmc:/retroarch/playlists/Nintendo - Game Boy.lpl", {
        {"/roms/gb/Pokemon Red.gb", "DETECT"},
    });
    d = resolveGameLaunch(0, "red_gb", "retroarch",
                          "sdmc:/retroarch/saves/mGBA/Pokemon Red.srm", "");
    assert(d.state == GameLaunchState::Ready && d.corePath == mgba);
    d = resolveGameLaunch(0, "red_gb", "retroarch", "sdmc:/retroarch/saves/Pokemon Red.srm", "");
    assert(d.state == GameLaunchState::Ready && d.corePath == gambatte && d.launchNotice.empty());

    // ---- Quit-route preview always matches the real handoff (round 3 defect 2) ----
    resetTree();
    fs::remove_all("romfs:");
    {
        GameLaunchDescriptor retro;
        retro.backend = GameLaunchBackend::RetroArch;
        retro.state = GameLaunchState::Ready;
        retro.providerId = "retroarch";
        retro.corePath = mgba;
        retro.launcherPath = mgba;
        retro.contentPath = "sdmc:/roms/gba/Pokemon Emerald.gba";
        touch("sdmc:/switch/PokeBank-NX.nro");
        setGameLaunchReturnPath("sdmc:/switch/PokeBank-NX.nro");
        GameLaunchChainPlan plan;
        std::string planError;

        // No opt-in: direct, and the preview says so.
        assert(previewGameLaunchQuitRoute(retro) == GameLaunchQuitRoute::LoaderDefault);
        assert(planGameLaunchHandoff(retro, plan, planError) && !plan.viaReturnHost);

        // Opted in but the bundled host cannot be installed: never promise a return.
        touch("sdmc:/switch/PokeBank-NX/config/experimental_return_host.enable");
        assert(previewGameLaunchQuitRoute(retro) == GameLaunchQuitRoute::LoaderDefault);
        assert(planGameLaunchHandoff(retro, plan, planError) && !plan.viaReturnHost);
        assert(gameLaunchDialogNotice(retro).find("returns to PokeBank") == std::string::npos);

        // Host payload present: the preview promises the return and the plan takes it, using
        // the verified installed copy.
        const std::string kHostPayload = "NRO0-return-host-fixture";
        {
            fs::create_directories("romfs:/runtime");
            std::ofstream payload("romfs:/runtime/PokeBankReturnHost.nro", std::ios::binary);
            payload << kHostPayload;
        }
        setGameLaunchReturnPath("sdmc:/switch/PokeBank-NX.nro"); // re-decide readiness
        assert(previewGameLaunchQuitRoute(retro) == GameLaunchQuitRoute::ReturnsToPokeBank);
        assert(gameLaunchDialogNotice(retro).find("returns to PokeBank NX") != std::string::npos);
        assert(planGameLaunchHandoff(retro, plan, planError) && plan.viaReturnHost);
        const std::string installed = "sdmc:/switch/PokeBank-NX/runtime/PokeBankReturnHost.nro";
        assert(plan.nroPath == installed && fs::file_size(installed) == kHostPayload.size());
        assert(!fs::exists(installed + ".tmp"));

        // Promised a return, then the host vanished and cannot be reinstalled: the launch is
        // refused with the new route instead of silently going direct; the next try is direct.
        assert(previewGameLaunchQuitRoute(retro) == GameLaunchQuitRoute::ReturnsToPokeBank);
        fs::remove(installed);
        fs::remove_all("romfs:");
        assert(!planGameLaunchHandoff(retro, plan, planError));
        assert(planError.find("has changed") != std::string::npos);
        assert(planError.find("Homebrew Menu") != std::string::npos);
        assert(planGameLaunchHandoff(retro, plan, planError) && !plan.viaReturnHost);
        assert(previewGameLaunchQuitRoute(retro) == GameLaunchQuitRoute::LoaderDefault);
        setGameLaunchReturnPath("");
    }

    // Launch/return: Product Home focus is recorded at handoff and restored exactly once.
    resetTree();
    {
        GameLaunchReturnSelection selection;
        selection.profileIdentity = "profile";
        selection.gameId = "red_gb";
        selection.sourceIdentity = "retroarch|sdmc:/retroarch/saves/Pokemon Red.srm";
        selection.sourceKind = 2;
        selection.titleIndex = 3;
        selection.savedAt = 1'760'000'000;
        std::string error;
        assert(saveGameLaunchReturnSelection(selection, error) && error.empty());
        const fs::path record = "sdmc:/switch/PokeBank-NX/config/launch_return_selection.cfg";
        assert(fs::exists(record));
        assert(!fs::exists(record.string() + ".tmp"));
        GameLaunchReturnSelection restored;
        assert(consumeGameLaunchReturnSelection(selection.savedAt + 120, restored));
        assert(restored.gameId == "red_gb" && restored.titleIndex == 3 &&
               restored.sourceIdentity == selection.sourceIdentity);
        assert(!fs::exists(record));                         // consumed
        assert(!consumeGameLaunchReturnSelection(selection.savedAt + 121, restored));
        // A corrupt record is still removed so it can never pin focus on later starts.
        {
            std::ofstream out(record, std::ios::binary | std::ios::trunc);
            out << "garbage";
        }
        assert(!consumeGameLaunchReturnSelection(selection.savedAt, restored));
        assert(!fs::exists(record));
        // Overwrite keeps only the newest handoff.
        assert(saveGameLaunchReturnSelection(selection, error));
        selection.gameId = "blue_gb";
        assert(saveGameLaunchReturnSelection(selection, error));
        assert(consumeGameLaunchReturnSelection(selection.savedAt, restored) &&
               restored.gameId == "blue_gb");
        GameLaunchReturnSelection unsafe = selection;
        unsafe.gameId = "bad\ngame";
        assert(!saveGameLaunchReturnSelection(unsafe, error) && !error.empty());
        assert(!fs::exists(record));
    }

    // Quit route preview for the resolved descriptors (no return-host opt-in on host builds).
    resetTree();
    {
        GameLaunchDescriptor drastic;
        drastic.backend = GameLaunchBackend::HomebrewNro;
        drastic.state = GameLaunchState::Ready;
        drastic.providerId = "drastic";
        drastic.launcherPath = "sdmc:/switch/DrasticDS.nro";
        drastic.contentPath = "sdmc:/switch/drastic/games/Pokemon Platinum.nds";
        assert(previewGameLaunchQuitRoute(drastic) == GameLaunchQuitRoute::EmulatorLauncher);
        GameLaunchDescriptor retro;
        retro.backend = GameLaunchBackend::RetroArch;
        retro.state = GameLaunchState::Ready;
        retro.corePath = "sdmc:/retroarch/cores/mgba_libretro_libnx.nro";
        retro.contentPath = "sdmc:/roms/gba/Pokemon Emerald.gba";
        assert(previewGameLaunchQuitRoute(retro) == GameLaunchQuitRoute::LoaderDefault);
        GameLaunchDescriptor title;
        title.backend = GameLaunchBackend::SwitchTitle;
        title.state = GameLaunchState::Ready;
        assert(previewGameLaunchQuitRoute(title) == GameLaunchQuitRoute::None);
        // Host builds never hand off, so no return selection may be recorded by UI.cpp.
        std::string launchError;
        assert(!requestGameLaunch(retro, launchError) && !launchError.empty());
        assert(!gameLaunchHandoffRequested());
    }

    fs::remove_all("sdmc:");
    fs::remove_all("romfs:");
    std::cout << "RetroArch launch resolver ambiguity/family contract: PASS\n";
    return 0;
}
