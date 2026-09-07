#include "Games/GameIdentity.h"
#include "Legacy/RetroArchFRLGDiscovery.h"

#include <algorithm>
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

#include <unistd.h>

namespace fs = std::filesystem;

namespace {
    std::vector<uint8_t> readFile(const fs::path& path) {
        std::ifstream input(path, std::ios::binary);
        return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
    }

    void writeFile(const fs::path& path, const std::vector<uint8_t>& bytes) {
        std::ofstream output(path, std::ios::binary | std::ios::trunc);
        output.write(reinterpret_cast<const char*>(bytes.data()),
                     static_cast<std::streamsize>(bytes.size()));
        assert(output.good());
    }
}

int main() {
    char tempTemplate[] = "/tmp/pokebank-retroarch-XXXXXX";
    const char* created = mkdtemp(tempTemplate);
    assert(created != nullptr);
    const fs::path temp(created);
    const fs::path saves = temp / "savefiles";
    const fs::path core = saves / "mGBA";
    fs::create_directories(core);

    const auto fixture = readFile("build-host/frlg_fixture.sav");
    assert(fixture.size() == 0x20000);
    const fs::path fireRedPath = core / "Pokemon FireRed.sav";
    const fs::path leafGreenPath = saves / "Pokemon LeafGreen.srm";
    const fs::path ambiguousPath = saves / "FRLG Mystery.sav";
    const fs::path brokenPath = saves / "FireRed Broken.sav";
    writeFile(fireRedPath, fixture);
    writeFile(leafGreenPath, fixture);
    writeFile(ambiguousPath, fixture);
    writeFile(brokenPath, {1, 2, 3, 4});
    writeFile(saves / "unrelated.sav", {9, 8, 7});

    const fs::path config = temp / "retroarch.cfg";
    {
        std::ofstream output(config);
        output << "video_driver = \"gl\"\n";
        output << "savefile_directory = \"savefiles\"\n";
    }
    const auto roots = PokeVault::Legacy::retroArchSaveRootsFromConfig(config.string());
    assert(roots.size() == 1 && roots.front() == saves.string());
    assert(PokeVault::Legacy::retroArchSaveRootsFromConfig(
        (temp / "missing.cfg").string()).empty());
    const std::vector<std::string> missingRoots{(temp / "missing-save-root").string()};
    const auto missing = PokeVault::Legacy::discoverFRLGSaves(missingRoots);
    assert(missing.sources.empty() && missing.filesExamined == 0 && !missing.limitReached);

    const auto fireRedBefore = readFile(fireRedPath);
    const auto leafGreenBefore = readFile(leafGreenPath);
    auto result = PokeVault::Legacy::discoverFRLGSaves(roots);
    assert(result.filesExamined == 5);
    assert(!result.limitReached);
    assert(readFile(fireRedPath) == fireRedBefore);
    assert(readFile(leafGreenPath) == leafGreenBefore);

    auto findReady = [&](std::string_view gameId) {
        return std::find_if(result.sources.begin(), result.sources.end(), [&](const auto& source) {
            return source.ready() && source.gameId == gameId;
        });
    };
    const auto fireRed = findReady("firered_gba");
    const auto leafGreen = findReady("leafgreen_gba");
    assert(fireRed != result.sources.end());
    assert(leafGreen != result.sources.end());
    assert(fireRed->save->party().size() == 1);
    assert(fireRed->save->boxes().size() == 2);
    assert(leafGreen->save->party().size() == 1);
    assert(leafGreen->save->boxes().size() == 2);

    const auto ambiguous = std::find_if(result.sources.begin(), result.sources.end(),
        [&](const auto& source) { return source.path == ambiguousPath.string(); });
    assert(ambiguous != result.sources.end());
    assert(ambiguous->status == PokeVault::Legacy::LegacySourceStatus::AmbiguousIdentity);
    assert(!ambiguous->save);

    const auto broken = std::find_if(result.sources.begin(), result.sources.end(),
        [&](const auto& source) { return source.path == brokenPath.string(); });
    assert(broken != result.sources.end());
    assert(broken->status == PokeVault::Legacy::LegacySourceStatus::InvalidSave);
    assert(broken->parseError == PokeVault::Integration::Gen3::SaveError::WrongSize);
    assert(result.sources.size() == 4); // unrelated invalid save is deliberately suppressed

    const auto* fireRedGba = PokeVault::Games::findGame("firered_gba");
    const auto* fireRedSwitch = PokeVault::Games::findGame("firered_switch");
    const auto* leafGreenGba = PokeVault::Games::findGame("leafgreen_gba");
    const auto* leafGreenSwitch = PokeVault::Games::findGame("leafgreen_switch");
    assert(fireRedGba && fireRedSwitch && leafGreenGba && leafGreenSwitch);
    assert(fireRedGba->id != fireRedSwitch->id);
    assert(leafGreenGba->id != leafGreenSwitch->id);
    assert(fireRedGba->support == PokeVault::Games::SourceSupport::ReadOnly);
    assert(leafGreenGba->support == PokeVault::Games::SourceSupport::ReadOnly);

    auto limited = PokeVault::Legacy::discoverFRLGSaves(roots, {.maxDepth = 2, .maxFiles = 1});
    assert(limited.filesExamined == 1 && limited.limitReached);

    const fs::path depthRoot = temp / "depth-root";
    const fs::path depthThree = depthRoot / "one" / "two" / "three";
    fs::create_directories(depthThree);
    writeFile(depthThree / "Pokemon FireRed.sav", fixture);
    const std::vector<std::string> depthRoots{depthRoot.string()};
    const auto depthLimited = PokeVault::Legacy::discoverFRLGSaves(
        depthRoots, {.maxDepth = 2, .maxFiles = 256});
    assert(depthLimited.sources.empty() && depthLimited.filesExamined == 0);
    const auto depthAllowed = PokeVault::Legacy::discoverFRLGSaves(
        depthRoots, {.maxDepth = 3, .maxFiles = 256});
    assert(depthAllowed.sources.size() == 1 && depthAllowed.sources.front().ready());

    fs::remove_all(temp);
    std::cout << "RetroArch FRLG read-only discovery tests passed\n";
    return 0;
}
