#include <algorithm>
#include <array>
#include <cassert>
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <span>
#include <string>
#include <vector>

#include "Legacy/RetroArchRBYDiscovery.h"

namespace fs = std::filesystem;
using namespace PokeVault::Integration::Gen1;
using namespace PokeVault::Legacy;

namespace {
constexpr std::size_t kIntlMainStart = 0x2598;
constexpr std::size_t kIntlMoney = 0x25F3;
constexpr std::size_t kIntlTid = 0x2605;
constexpr std::size_t kIntlCurrentBoxIndex = 0x284C;
constexpr std::size_t kIntlStarter = 0x29C3;
constexpr std::size_t kIntlFriendship = 0x271C;
constexpr std::size_t kIntlParty = 0x2F2C;
constexpr std::size_t kIntlCurrentBox = 0x30C0;
constexpr std::size_t kIntlChecksum = 0x3523;
constexpr std::size_t kIntlMainLength = 0x0F8B;

uint8_t diff8(std::span<const uint8_t> bytes) {
    uint8_t value = 0xFF;
    for (uint8_t b : bytes) value = static_cast<uint8_t>(value - b);
    return value;
}

void putBE16(std::vector<uint8_t>& d, std::size_t o, uint16_t v) {
    d[o] = static_cast<uint8_t>(v >> 8);
    d[o + 1] = static_cast<uint8_t>(v);
}

void putMoney(std::vector<uint8_t>& d, uint32_t money) {
    assert(money <= 999999);
    const std::array<unsigned, 6> digit{{
        (money / 100000) % 10, (money / 10000) % 10, (money / 1000) % 10,
        (money / 100) % 10, (money / 10) % 10, money % 10,
    }};
    for (int i = 0; i < 3; ++i)
        d[kIntlMoney + i] = static_cast<uint8_t>((digit[i * 2] << 4) | digit[i * 2 + 1]);
}

std::vector<uint8_t> makeInternationalFixture(SourceGame game, uint32_t money = 123456) {
    std::vector<uint8_t> d(kRawSaveSize, 0);
    // WILL, terminated, using the Gen I international character table.
    d[kIntlMainStart + 0] = 0x96;
    d[kIntlMainStart + 1] = 0x88;
    d[kIntlMainStart + 2] = 0x8B;
    d[kIntlMainStart + 3] = 0x8B;
    d[kIntlMainStart + 4] = 0x50;
    putMoney(d, money);
    putBE16(d, kIntlTid, 0x1234);
    d[kIntlStarter] = game == SourceGame::Yellow ? 0x54 : 0x99;
    d[kIntlFriendship] = game == SourceGame::Yellow ? 90 : 0;
    d[kIntlCurrentBoxIndex] = 2; // uninitialized stored box banks are intentionally ignored.
    d[kIntlParty] = 0;
    d[kIntlParty + 1] = 0xFF;
    d[kIntlCurrentBox] = 0;
    d[kIntlCurrentBox + 1] = 0xFF;

    // Prevent pinned PKSM-Core's intentionally loose Japanese-list heuristic from interpreting
    // this mostly-empty synthetic international fixture as Japanese. This byte is outside the
    // international party/current-box structures and is included in the legitimate INT checksum.
    d[0x2ED5] = 0xFF;
    d[kIntlChecksum] = diff8(std::span<const uint8_t>(d).subspan(kIntlMainStart, kIntlMainLength));
    return d;
}

void writeBytes(const fs::path& path, const std::vector<uint8_t>& bytes) {
    fs::create_directories(path.parent_path());
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    assert(out.good());
    out.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    assert(out.good());
}

std::vector<uint8_t> readBytes(const fs::path& path) {
    std::ifstream in(path, std::ios::binary);
    assert(in.good());
    return std::vector<uint8_t>((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
}

const RBYSource* findByGame(const RBYDiscoveryResult& result, const std::string& gameId) {
    for (const auto& source : result.sources)
        if (source.gameId == gameId && source.ready()) return &source;
    return nullptr;
}

const RBYSource* findByStatus(const RBYDiscoveryResult& result, RBYSourceStatus status) {
    for (const auto& source : result.sources)
        if (source.status == status) return &source;
    return nullptr;
}

struct TempTree {
    fs::path root;
    TempTree() {
        root = fs::temp_directory_path() /
            ("pokebank-rby-discovery-" + std::to_string(static_cast<unsigned long long>(::getpid())));
        fs::remove_all(root);
        fs::create_directories(root);
    }
    ~TempTree() { fs::remove_all(root); }
};
} // namespace

int main() {
    TempTree temp;
    const fs::path root = temp.root / "savefiles";
    fs::create_directories(root);

    const auto red = makeInternationalFixture(SourceGame::Red);
    const auto blue = makeInternationalFixture(SourceGame::Blue);
    const auto yellow = makeInternationalFixture(SourceGame::Yellow);

    const fs::path redPath = root / "Pokemon Red.srm";
    const fs::path bluePath = root / "Pokemon Blue.sav";
    const fs::path yellowPath = root / "Pokemon Yellow.srm";
    writeBytes(redPath, red);
    writeBytes(bluePath, blue);
    writeBytes(yellowPath, yellow);

    // Valid but unhinted Gen I is deliberately surfaced as identity-ambiguous.
    const fs::path mysteryPath = root / "mystery.srm";
    writeBytes(mysteryPath, red);

    // Named malformed candidates remain diagnostic entries instead of silently disappearing.
    const fs::path wrongSizePath = root / "Pokemon Red wrong-size.sav";
    writeBytes(wrongSizePath, std::vector<uint8_t>(123, 0));
    auto checksumBad = red;
    checksumBad[0x2608] ^= 1;
    writeBytes(root / "Pokemon Blue checksum-bad.sav", checksumBad);
    auto malformed = red;
    malformed[kIntlParty] = 7;
    malformed[kIntlChecksum] = diff8(std::span<const uint8_t>(malformed).subspan(kIntlMainStart, kIntlMainLength));
    writeBytes(root / "Pokemon Red malformed.sav", malformed);

    // Arbitrary RetroArch savestates are outside the source contract even if bytes look save-like.
    writeBytes(root / "Pokemon Red.state", red);

    const std::array<std::string, 1> roots{{root.string()}};
    auto result = discoverRBYSaves(roots);
    assert(result.activeRoot == root.string());
    const RBYSource* redSource = findByGame(result, "red_gb");
    const RBYSource* blueSource = findByGame(result, "blue_gb");
    const RBYSource* yellowSource = findByGame(result, "yellow_gb");
    assert(redSource && blueSource && yellowSource);
    assert(redSource->save->trainer().money == 123456);
    assert(redSource->save->trainer().trainerId == 0x1234);
    assert(findByStatus(result, RBYSourceStatus::AmbiguousIdentity));

    bool sawWrongSize = false, sawChecksum = false, sawMalformed = false, sawState = false;
    for (const auto& source : result.sources) {
        if (source.path.find("wrong-size") != std::string::npos) {
            sawWrongSize = true;
            assert(source.status == RBYSourceStatus::InvalidSave);
            assert(source.parseError == SaveError::WrongSize);
        }
        if (source.path.find("checksum-bad") != std::string::npos) {
            sawChecksum = true;
            assert(source.status == RBYSourceStatus::InvalidSave);
            assert(source.parseError == SaveError::ChecksumMismatch);
        }
        if (source.path.find("malformed") != std::string::npos) {
            sawMalformed = true;
            assert(source.status == RBYSourceStatus::InvalidSave);
        }
        if (source.path.find(".state") != std::string::npos) sawState = true;
    }
    assert(sawWrongSize && sawChecksum && sawMalformed && !sawState);

    // The scanner never mutates the source file.
    assert(readBytes(redPath) == red);

    // A hard-link alias is one physical file and must not become a duplicate source.
    const fs::path aliasRoot = temp.root / "aliases";
    fs::create_directories(aliasRoot);
    const fs::path physical = aliasRoot / "Pokemon Red.srm";
    writeBytes(physical, red);
    const fs::path alias = aliasRoot / "Red Alias.srm";
    std::error_code linkError;
    fs::create_hard_link(physical, alias, linkError);
    assert(!linkError);
    const std::array<std::string, 1> aliasRoots{{aliasRoot.string()}};
    auto aliasResult = discoverRBYSaves(aliasRoots);
    std::size_t readyRed = 0;
    for (const auto& source : aliasResult.sources)
        if (source.ready() && source.gameId == "red_gb") ++readyRed;
    assert(readyRed == 1);

    // Byte-identical but physically separate files remain separate source instances.
    const fs::path copiesRoot = temp.root / "copies";
    fs::create_directories(copiesRoot);
    writeBytes(copiesRoot / "Pokemon Red A.srm", red);
    writeBytes(copiesRoot / "Pokemon Red B.srm", red);
    const std::array<std::string, 1> copyRoots{{copiesRoot.string()}};
    auto copies = discoverRBYSaves(copyRoots);
    std::vector<const RBYSource*> copySources;
    for (const auto& source : copies.sources)
        if (source.ready() && source.gameId == "red_gb") copySources.push_back(&source);
    assert(copySources.size() == 2);
    assert(copySources[0]->contentFingerprint == copySources[1]->contentFingerprint);
    assert(copySources[0]->canonicalPath != copySources[1]->canonicalPath);
    assert(copySources[0]->sourceIdentity != copySources[1]->sourceIdentity);

    // Refresh is a real reread: stable source identity, new fingerprint/model after RetroArch saves.
    const std::string oldIdentity = redSource->sourceIdentity;
    const std::string oldFingerprint = redSource->contentFingerprint;
    const auto refreshedBytes = makeInternationalFixture(SourceGame::Red, 654321);
    writeBytes(redPath, refreshedBytes);
    result = discoverRBYSaves(roots);
    redSource = findByGame(result, "red_gb");
    assert(redSource);
    assert(redSource->sourceIdentity == oldIdentity);
    assert(redSource->contentFingerprint != oldFingerprint);
    assert(redSource->save->trainer().money == 654321);
    assert(readBytes(redPath) == refreshedBytes);

    // Configured savefile_directory has precedence over the conventional fallback.
    const fs::path configRoot = temp.root / "configured";
    const fs::path fallbackRoot = temp.root / "fallback";
    fs::create_directories(configRoot);
    fs::create_directories(fallbackRoot);
    writeBytes(configRoot / "Pokemon Blue.srm", blue);
    writeBytes(fallbackRoot / "Pokemon Yellow.srm", yellow);
    const fs::path config = temp.root / "retroarch.cfg";
    {
        std::ofstream out(config);
        assert(out.good());
        out << "savefile_directory = \"" << configRoot.string() << "\"\n";
    }
    auto configured = discoverConfiguredRetroArchRBYSaves({}, config.string(), fallbackRoot.string());
    assert(configured.activeRootKind == RBYDiscoveryResult::RootKind::Configured);
    assert(findByGame(configured, "blue_gb"));
    assert(!findByGame(configured, "yellow_gb"));

    const fs::path missingConfig = temp.root / "missing.cfg";
    auto fallback = discoverConfiguredRetroArchRBYSaves({}, missingConfig.string(), fallbackRoot.string());
    assert(fallback.activeRootKind == RBYDiscoveryResult::RootKind::ConventionalFallback);
    assert(findByGame(fallback, "yellow_gb"));

    // Depth 2 is admitted, depth 3 is not.
    const fs::path depthRoot = temp.root / "depth";
    writeBytes(depthRoot / "one" / "two" / "Pokemon Red.srm", red);
    writeBytes(depthRoot / "one" / "two" / "three" / "Pokemon Blue.srm", blue);
    const std::array<std::string, 1> depthRoots{{depthRoot.string()}};
    auto bounded = discoverRBYSaves(depthRoots);
    assert(findByGame(bounded, "red_gb"));
    assert(!findByGame(bounded, "blue_gb"));

    // Candidate limit is deterministic and prevents unbounded emulator-tree traversal.
    const fs::path limitRoot = temp.root / "limit";
    fs::create_directories(limitRoot);
    writeBytes(limitRoot / "Pokemon Red 1.srm", red);
    writeBytes(limitRoot / "Pokemon Red 2.srm", red);
    writeBytes(limitRoot / "Pokemon Red 3.srm", red);
    const std::array<std::string, 1> limitRoots{{limitRoot.string()}};
    auto limited = discoverRBYSaves(limitRoots, RBYScanLimits{2, 2});
    assert(limited.filesExamined == 2);
    assert(limited.limitReached);

    std::cout << "RetroArch RBY bounded read-only discovery tests passed\n";
    return 0;
}
