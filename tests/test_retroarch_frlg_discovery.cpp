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

    uint16_t read16(const std::vector<uint8_t>& bytes, size_t offset) {
        return static_cast<uint16_t>(bytes[offset]) |
               static_cast<uint16_t>(bytes[offset + 1] << 8);
    }

    void write16(std::vector<uint8_t>& bytes, size_t offset, uint16_t value) {
        bytes[offset] = static_cast<uint8_t>(value);
        bytes[offset + 1] = static_cast<uint8_t>(value >> 8);
    }

    uint16_t sectorChecksum(const uint8_t* bytes) {
        uint32_t sum = 0;
        for (size_t offset = 0; offset < 0xF80; offset += 4) {
            sum += static_cast<uint32_t>(bytes[offset]) |
                   (static_cast<uint32_t>(bytes[offset + 1]) << 8) |
                   (static_cast<uint32_t>(bytes[offset + 2]) << 16) |
                   (static_cast<uint32_t>(bytes[offset + 3]) << 24);
        }
        return static_cast<uint16_t>((sum & 0xFFFFu) + (sum >> 16));
    }

    std::vector<uint8_t> withTwoPartyPokemon(const std::vector<uint8_t>& fixture) {
        std::vector<uint8_t> changed = fixture;
        constexpr size_t activeSlot = 0xE000;
        size_t partySector = changed.size();
        for (size_t physical = 0; physical < 14; ++physical) {
            const size_t offset = activeSlot + physical * 0x1000;
            if (read16(changed, offset + 0xFF4) == 1) {
                partySector = offset;
                break;
            }
        }
        assert(partySector < changed.size());
        changed[partySector + 0x34] = 2;
        std::copy_n(changed.begin() + static_cast<std::ptrdiff_t>(partySector + 0x38), 100,
                    changed.begin() + static_cast<std::ptrdiff_t>(partySector + 0x38 + 100));
        write16(changed, partySector + 0xFF6,
                sectorChecksum(changed.data() + partySector));
        return changed;
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
    // Overlapping and repeated approved roots must not surface the same underlying file twice.
    auto overlappingRoots = roots;
    overlappingRoots.push_back(core.string());
    overlappingRoots.push_back(saves.string());
    auto result = PokeVault::Legacy::discoverFRLGSaves(overlappingRoots);
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
    assert(!fireRed->canonicalPath.empty() && !leafGreen->canonicalPath.empty());
    assert(!fireRed->sourceIdentity.empty() && !leafGreen->sourceIdentity.empty());
    assert(fireRed->sourceIdentity != leafGreen->sourceIdentity);
    assert(fireRed->contentFingerprint.size() == 64);
    assert(fireRed->normalizedPath == fireRedPath.string());
    assert(fireRed->fileSize == 0x20000 && leafGreen->fileSize == 0x20000);
    assert(std::count_if(result.sources.begin(), result.sources.end(), [&](const auto& source) {
        return source.ready() && source.gameId == "firered_gba";
    }) == 1);
    assert(std::count_if(result.sources.begin(), result.sources.end(), [&](const auto& source) {
        return source.ready() && source.gameId == "leafgreen_gba";
    }) == 1);
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

    // A usable configured root is authoritative. A conventional fallback that also exists must
    // not be scanned additively, otherwise stale copies RetroArch is not using would reappear.
    const fs::path fallbackRoot = temp / "conventional-savefiles";
    fs::create_directories(fallbackRoot);
    const fs::path staleFallback = fallbackRoot / "Pokemon FireRed Old.sav";
    writeFile(staleFallback, fixture);
    const auto configured = PokeVault::Legacy::discoverConfiguredRetroArchFRLGSaves(
        {}, config.string(), fallbackRoot.string());
    assert(configured.activeRoot == saves.string());
    assert(configured.activeRootKind ==
           PokeVault::Legacy::FRLGDiscoveryResult::RootKind::Configured);
    assert(std::none_of(configured.sources.begin(), configured.sources.end(),
        [&](const auto& source) { return source.path == staleFallback.string(); }));

    const fs::path defaultConfig = temp / "retroarch-default.cfg";
    {
        std::ofstream output(defaultConfig);
        output << "savefile_directory = \"default\"\n";
    }
    const auto fallbackSelected = PokeVault::Legacy::discoverConfiguredRetroArchFRLGSaves(
        {}, defaultConfig.string(), fallbackRoot.string());
    assert(fallbackSelected.activeRoot == fallbackRoot.string());
    assert(fallbackSelected.activeRootKind ==
           PokeVault::Legacy::FRLGDiscoveryResult::RootKind::ConventionalFallback);
    assert(fallbackSelected.sources.size() == 1 && fallbackSelected.sources[0].ready());

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

    // Refresh is a fresh bounded read: same-path valid replacement bytes produce a new strict
    // model and fingerprint instead of leaving the old one-Pokemon parsed object cached.
    const std::string oldFingerprint = fireRed->contentFingerprint;
    const std::string stableIdentity = fireRed->sourceIdentity;
    writeFile(fireRedPath, withTwoPartyPokemon(fixture));
    auto refreshed = PokeVault::Legacy::discoverConfiguredRetroArchFRLGSaves(
        {}, config.string(), fallbackRoot.string());
    const auto currentFireRed = std::find_if(refreshed.sources.begin(), refreshed.sources.end(),
        [&](const auto& source) { return source.normalizedPath == fireRedPath.string(); });
    assert(currentFireRed != refreshed.sources.end() && currentFireRed->ready());
    assert(currentFireRed->sourceIdentity == stableIdentity);
    assert(currentFireRed->contentFingerprint != oldFingerprint);
    assert(currentFireRed->save->party().size() == 2);

    // Invalid and deleted files disappear from selectable sources safely.
    writeFile(leafGreenPath, {1, 2, 3, 4});
    auto changed = PokeVault::Legacy::discoverConfiguredRetroArchFRLGSaves(
        {}, config.string(), fallbackRoot.string());
    assert(std::none_of(changed.sources.begin(), changed.sources.end(), [](const auto& source) {
        return source.ready() && source.gameId == "leafgreen_gba";
    }));
    fs::remove(fireRedPath);
    auto removed = PokeVault::Legacy::discoverConfiguredRetroArchFRLGSaves(
        {}, config.string(), fallbackRoot.string());
    assert(std::none_of(removed.sources.begin(), removed.sources.end(), [&](const auto& source) {
        return source.normalizedPath == fireRedPath.string();
    }));

    fs::remove_all(temp);
    std::cout << "RetroArch FRLG read-only discovery tests passed\n";
    return 0;
}
