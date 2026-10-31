// Round 8 automated QA (owner Oct 28: "Mock localized Switch title discovery without requiring me
// to own those games"): the REAL SaveSelectScreen title discovery (loadUsers -> loadTitlesForUser ->
// scanSaveSpace) runs on the host against a SCRIPTED console: one account and the save-data entries
// the console would report (tests/host-ui/switch.h, HostUi::consoleSaves). Title ids are the twelve
// FireRed/LeafGreen Switch releases (upstream PKSE GameVersion.h, see test_frlg_switch_releases).
//
// Evidence class: SYNTHETIC. No save is mounted or read (fsdevMountSaveData refuses on the host).
// This proves the app's discovery logic lists each release once and rejects everything else; it
// does not prove that a real console reports these entries this way.
#include "HostUi.h"

#include "Utils/NXTypes.h"
#include "Globals.h"
#include "Legacy/LegacySourceBindings.h"
#include "Legacy/RetroArchFRLGDiscovery.h"
#include "UI/SaveSelectScreen.h"

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unistd.h>

namespace fs = std::filesystem;

namespace {
int failures = 0, checks = 0;
void check(bool ok, const std::string& what) {
    ++checks;
    if (!ok) { std::cerr << "FAILED: " << what << '\n'; ++failures; }
}

constexpr u64 kOwner[2] = {0x0123456789ABCDEFULL, 0x0FEDCBA987654321ULL};
constexpr u64 kOther[2] = {0x1111111111111111ULL, 0x2222222222222222ULL};
constexpr u64 kReleases[12] = {
    0x01006FA0233F8000ULL, 0x0100554023408000ULL, 0x01004B3023412000ULL, 0x010092302342A000ULL,
    0x01007F8023416000ULL, 0x0100EB702342C000ULL,                                    // FireRed
    0x0100F1E0233FA000ULL, 0x010034D02340E000ULL, 0x010087C02342E000ULL, 0x01005C7023432000ULL,
    0x0100FD6023430000ULL, 0x01002B5023434000ULL,                                    // LeafGreen
};
constexpr u64 kSword = 0x0100ABF008968000ULL;           // a supported non-FRLG control
constexpr u64 kNotPokemon = 0x0100000000010000ULL;      // any other application
constexpr u64 kNearMiss = 0x0100554023409000ULL;        // English FireRed id + 0x1000: unknown

void add(u64 title, const u64 (&user)[2], u8 type, s32 space) {
    auto& e = HostUi::consoleSaveEntries[HostUi::consoleSaveCount];
    e = {};
    e.application_id = title;
    e.save_data_type = type;
    e.uid[0] = user[0];
    e.uid[1] = user[1];
    HostUi::consoleSaveSpace[HostUi::consoleSaveCount++] = space;
}

std::string hex(u64 v) {
    char b[24];
    std::snprintf(b, sizeof(b), "(%016llX)", static_cast<unsigned long long>(v));
    return b;
}

std::string readLogs() {
    std::string all;
    const fs::path dir = "sdmc:/switch/PokeBank-NX/logs";
    if (!fs::exists(dir)) return all;
    for (const auto& e : fs::directory_iterator(dir)) {
        std::ifstream in(e.path());
        std::stringstream s;
        s << in.rdbuf();
        all += s.str();
    }
    return all;
}

size_t count(const std::string& hay, const std::string& needle) {
    size_t n = 0;
    for (size_t at = hay.find(needle); at != std::string::npos; at = hay.find(needle, at + 1)) ++n;
    return n;
}

// One discovery pass on a fresh private working directory (the app's sdmc:/ root lands there).
std::string discover(const std::string& label) {
    char templ[] = "/tmp/pokebank-discovery-XXXXXX";
    const char* made = ::mkdtemp(templ);
    check(made != nullptr, label + ": temporary directory");
    const fs::path previous = fs::current_path();
    fs::current_path(made);
    fs::create_directory("sdmc:");
    g_debugLogging = true;
    {
        PokeVault::Legacy::FRLGDiscoveryResult legacy;
        PokeVault::Legacy::LegacySourceBindings bindings;
        UI::SaveSelectScreen screen(legacy, bindings);
    }
    g_debugLogging = false;
    const std::string log = readLogs();
    fs::current_path(previous);
    fs::remove_all(made);
    return log;
}

void reset() {
    HostUi::consoleSaves = {};
    HostUi::consoleSaveCount = 0;
}
}  // namespace

int main() {
    // 1. All twelve releases plus noise, answered by the All pseudo-space.
    reset();
    HostUi::consoleSaves.active = true;
    HostUi::consoleSaves.user[0] = kOwner[0];
    HostUi::consoleSaves.user[1] = kOwner[1];
    for (u64 id : kReleases) add(id, kOwner, FsSaveDataType_Account, FsSaveDataSpaceId_User);
    add(kReleases[1], kOwner, FsSaveDataType_Account, FsSaveDataSpaceId_User);  // second save index: one tile
    add(kReleases[0], kOther, FsSaveDataType_Account, FsSaveDataSpaceId_User);  // another profile's save
    add(kReleases[6], kOwner, FsSaveDataType_System, FsSaveDataSpaceId_User);   // not an account save
    add(kNotPokemon, kOwner, FsSaveDataType_Account, FsSaveDataSpaceId_User);
    add(kNearMiss, kOwner, FsSaveDataType_Account, FsSaveDataSpaceId_User);
    add(kSword, kOwner, FsSaveDataType_Account, FsSaveDataSpaceId_SdUser);
    std::string log = discover("all releases");
    for (u64 id : kReleases)
        check(count(log, hex(id) + " -> listed") == 1, "release " + hex(id) + " listed exactly once");
    check(count(log, hex(kSword) + " -> listed") == 1, "Sword control listed");
    check(log.find(hex(kNotPokemon)) == std::string::npos, "non-Pokemon application not listed");
    check(log.find(hex(kNearMiss)) == std::string::npos, "near-miss title id not listed");
    check(log.find("18 save entries on console, 16 for QA Owner, 13 Pokemon titles listed") != std::string::npos,
          "summary: 18 entries, 16 for this profile, 13 titles");
    check(HostUi::consoleSaves.opens == 1, "the All space answered; no fallback scan");

    // 2. The All pseudo-space is refused: discovery falls back to the User and SdUser spaces and
    //    still lists each release once, even when a save is reported by both.
    reset();
    HostUi::consoleSaves.active = true;
    HostUi::consoleSaves.user[0] = kOwner[0];
    HostUi::consoleSaves.user[1] = kOwner[1];
    HostUi::consoleSaves.allSpaceRefused = true;
    for (size_t i = 0; i < 12; ++i)
        add(kReleases[i], kOwner, FsSaveDataType_Account, i % 2 ? FsSaveDataSpaceId_SdUser : FsSaveDataSpaceId_User);
    add(kReleases[3], kOwner, FsSaveDataType_Account, FsSaveDataSpaceId_SdUser);  // seen in both spaces
    log = discover("fallback spaces");
    for (u64 id : kReleases)
        check(count(log, hex(id) + " -> listed") == 1, "fallback: release " + hex(id) + " listed exactly once");
    check(log.find("12 Pokemon titles listed") != std::string::npos, "fallback: 12 titles");
    check(HostUi::consoleSaves.opens == 2, "fallback scanned User and SdUser");

    // 3. Another profile's saves only: nothing is listed for this owner.
    reset();
    HostUi::consoleSaves.active = true;
    HostUi::consoleSaves.user[0] = kOwner[0];
    HostUi::consoleSaves.user[1] = kOwner[1];
    for (u64 id : kReleases) add(id, kOther, FsSaveDataType_Account, FsSaveDataSpaceId_User);
    log = discover("other profile");
    check(log.find("0 Pokemon titles listed") != std::string::npos, "other profile: nothing listed");
    check(log.find(" -> listed") == std::string::npos, "other profile: no tile");

    // 4. The default host console (no account, no saves) is unchanged: nothing is listed.
    reset();
    log = discover("empty console");
    check(log.find(" -> listed") == std::string::npos, "empty console: no tile");

    std::cout << "Switch title discovery (scripted console, real SaveSelectScreen): " << checks
              << " checks, " << failures << " failure(s). SYNTHETIC evidence only.\n";
    return failures == 0 ? 0 : 1;
}
