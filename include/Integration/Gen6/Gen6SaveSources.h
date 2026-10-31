#ifndef POKEBANK_GEN6_SAVE_SOURCES_H
#define POKEBANK_GEN6_SAVE_SOURCES_H

#include "Integration/Gen6/Gen6Save.h"
#include "Source/SaveInstance.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

// Gen VI save sources: data model, provider descriptors and validation only.
// Registration in the Save Instances UI is requested from Thread 2
// (docs/gen6/integration-requests/). Every path below is cited in
// docs/gen6/EMULATOR_SAVE_PROVIDERS.md with the project's pinned commit.
// Identity always comes from validated save CONTENTS; a path/title-ID is only
// corroborating evidence and a disagreement is reported, never resolved.
namespace PokeVault::Integration::Gen6::Sources {

enum class ProviderId : uint8_t {
    CheckpointLayout,  // "/3ds/Checkpoint/saves/0x<unique> <title>/<backup>/main"
    JksmLayout,        // 3DS JKSM "/JKSM/User Saves/<title>/<backup>/main" (manual link only)
    ManualLink,        // explicit user-selected file + explicit exact-game assignment
    DekoponEmulator,   // Switch homebrew 3DS emulator (Azahar fork)
    AzaharEmulator,    // upstream Azahar (no Switch build)
};

enum class ProviderStatus : uint8_t {
    Verified,          // Switch-side path verified from the project's own source
    ManualLinkOnly,    // layout verified, but nothing writes it to the Switch SD card
    Unsupported,       // no verified Switch path; never scanned
};

struct ProviderDescriptor {
    ProviderId id;
    std::string_view providerId;      // Source::SaveInstance::providerId
    std::string_view label;
    ProviderStatus status;
    std::string_view switchRoot;      // empty unless Verified
    std::string_view reference;       // repository @ pinned commit
    std::string_view ownerCheck;      // what the owner should confirm on the SD card
};

inline constexpr std::array<ProviderDescriptor, 5> Providers = {{
    {ProviderId::CheckpointLayout, "checkpoint-3ds", "Checkpoint-layout 3DS export",
     ProviderStatus::Verified, "sdmc:/3ds/Checkpoint/saves/",
     "FlagBrew/Checkpoint@740f221a 3ds/source/paths.cpp; "
     "PalindromicBreadLoaf/dekopon@dfba28f5 src/citra_switch/save_manager.cpp",
     "A Dekopon or copied Checkpoint backup appears as "
     "sdmc:/3ds/Checkpoint/saves/0x0055D <title>/<backup>/main (X; Y 0x0055E, OR 0x011C4, AS 0x011C5)"},
    {ProviderId::JksmLayout, "jksm-3ds", "JKSM 3DS export (manual link)",
     ProviderStatus::ManualLinkOnly, "",
     "J-D-K/JKSM@f883dff8 JKSM/source/FS/FS.cpp, AppStates/BackupMenuState.cpp",
     "JKSM runs on a 3DS: copy an uncompressed <backup>/main to the Switch SD and link it "
     "manually. ZIP exports (ExportToZip) are unsupported."},
    {ProviderId::ManualLink, "manual", "Manually linked save file",
     ProviderStatus::Verified, "", "PokeBank NX (no external layout)",
     "Pick the save file and assign X, Y, Omega Ruby or Alpha Sapphire explicitly."},
    {ProviderId::DekoponEmulator, "dekopon", "Dekopon (3DS emulator)",
     ProviderStatus::Verified, "sdmc:/switch/dekopon/",
     "PalindromicBreadLoaf/dekopon@dfba28f5 (tag v2.1.0-RC1 = 9b0f15f8): "
     "src/common/file_util.cpp, src/citra_switch/config.cpp, "
     "src/core/file_sys/archive_source_sd_savedata.cpp, src/core/hle/service/fs/archive.h",
     "Confirm sdmc:/switch/dekopon/sdmc/Nintendo 3DS/00000000000000000000000000000000/"
     "00000000000000000000000000000000/title/00040000/00055d00/data/00000001/main exists after "
     "an in-game save; report sdmc:/switch/dekopon/user_dir.txt if present."},
    {ProviderId::AzaharEmulator, "azahar", "Azahar (3DS emulator)",
     ProviderStatus::Unsupported, "",
     "azahar-emu/azahar@68202bc4 (no Switch frontend; libretro.yml builds android, linux, "
     "windows, macos, ios, tvos only)",
     "If a Switch Azahar build is in use, report its NRO name/version and the full path of "
     "the folder that contains 'Nintendo 3DS/'."},
}};

[[nodiscard]] constexpr const ProviderDescriptor& descriptor(ProviderId id) noexcept {
    return Providers[static_cast<size_t>(id)];
}

// ---- Pure path model (no filesystem access) ----

// Checkpoint/Dekopon folder prefix: "0x%05X " of (titleId >> 8) & 0xFFFFF.
[[nodiscard]] std::string checkpointFolderPrefix(std::string_view gameId);
// Parses a Checkpoint folder name's leading "0xNNNNN " into the Gen VI game it
// names, or empty. Corroborating evidence only.
[[nodiscard]] std::string_view gameIdFromCheckpointFolder(std::string_view folderName);

// Dekopon user directory: default sdmc:/switch/dekopon/, or the content of
// sdmc:/switch/dekopon/user_dir.txt (StripSpaces + trailing '/', equal-to-
// default means default), exactly as citra_switch/config.cpp ReadUserDirPointer.
// A pointer outside "sdmc:/" (e.g. USB storage) is not reachable -> nullopt.
inline constexpr std::string_view DekoponDefaultUserDir = "sdmc:/switch/dekopon/";
inline constexpr std::string_view DekoponUserDirPointer = "sdmc:/switch/dekopon/user_dir.txt";
[[nodiscard]] std::optional<std::string> dekoponUserDir(std::optional<std::string_view> pointerContents);

// <userDir>sdmc/Nintendo 3DS/<ID0>/<ID1>/title/00040000/<low8>/data/00000001/main
// (ID0 = ID1 = 32 zeros: SYSTEM_ID/SDCARD_ID in core/hle/service/fs/archive.h).
[[nodiscard]] std::string emulatorSaveFile(std::string_view userDir, std::string_view gameId);

// ---- Validation into the provider-neutral Save Instance model ----

struct SourceContext {
    ProviderId provider = ProviderId::ManualLink;
    std::string_view assignedExactGame;  // explicit user/profile assignment (required)
    std::string_view evidenceGameId;     // from title-ID path evidence, may be empty
    std::string_view sourcePath;
    std::string_view normalizedPath;
    std::string_view physicalIdentity;
    std::string_view sourceIdentity;
    std::string_view claimedProfile;
    size_t validationHandle = 0;
};

struct Probe {
    Source::SaveInstance instance;
    std::optional<Gen6Save> save;
    [[nodiscard]] bool ready() const noexcept { return instance.ready() && save.has_value(); }
};

// In-memory and read-only. The game is taken from the validated bytes; the
// assignment and any path evidence must agree with it or the row is not Ready.
[[nodiscard]] Probe probeSave(std::span<const uint8_t> bytes, const SourceContext& context);

// ---- Read-only discovery (host + Switch). Never writes, never guesses. ----

struct DiscoveredFile {
    ProviderId provider;
    std::string path;
    std::string evidenceGameId;  // from the folder's title-ID, if any
};

// Scans <checkpointRoot>/0x0055D */<backup>/main etc. for the four Gen VI
// unique IDs only. `checkpointRoot` must be the verified saves root.
[[nodiscard]] std::vector<DiscoveredFile> findCheckpointExports(const std::string& checkpointRoot);
// Looks up only the four exact Gen VI emulator save files under a user dir.
[[nodiscard]] std::vector<DiscoveredFile> findEmulatorSaves(ProviderId provider, const std::string& userDir);
// Reads one regular file read-only, refusing anything but the two SAV6 sizes.
[[nodiscard]] std::optional<std::vector<uint8_t>> readCandidate(const std::string& path, std::string* error = nullptr);

} // namespace PokeVault::Integration::Gen6::Sources

#endif
