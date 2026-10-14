#include "Integration/Gen6/Gen6SaveSources.h"

#include <cstdio>
#include <dirent.h>
#include <sys/stat.h>

#include <algorithm>

namespace PokeVault::Integration::Gen6::Sources {
namespace {

constexpr std::array<std::string_view, 4> GameIds = {"x_3ds", "y_3ds", "omegaruby_3ds", "alphasapphire_3ds"};

uint32_t uniqueId(uint64_t titleId) noexcept { return static_cast<uint32_t>((titleId >> 8) & 0xFFFFF); }

std::string join(const std::string& parent, const std::string& child) {
    if (parent.empty()) return child;
    return parent.back() == '/' ? parent + child : parent + "/" + child;
}

bool isDirectory(const std::string& path) {
    struct stat st{};
    return ::stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode);
}
bool isRegularFile(const std::string& path) {
    struct stat st{};
    return ::lstat(path.c_str(), &st) == 0 && S_ISREG(st.st_mode);
}

std::vector<std::string> listDirectories(const std::string& path) {
    std::vector<std::string> out;
    DIR* dir = ::opendir(path.c_str());
    if (!dir) return out;
    while (const dirent* entry = ::readdir(dir)) {
        const std::string name = entry->d_name;
        if (name == "." || name == "..") continue;
        if (isDirectory(join(path, name))) out.push_back(name);
    }
    ::closedir(dir);
    std::sort(out.begin(), out.end());
    return out;
}

std::string stripSpaces(std::string_view s) {
    const auto first = s.find_first_not_of(" \t\r\n");
    if (first == std::string_view::npos) return {};
    const auto last = s.find_last_not_of(" \t\r\n");
    return std::string(s.substr(first, last - first + 1));
}

std::string_view platformFor(ProviderId provider) {
    return provider == ProviderId::DekoponEmulator || provider == ProviderId::AzaharEmulator
               ? "Nintendo 3DS (emulated)" : "Nintendo 3DS";
}

} // namespace

std::string checkpointFolderPrefix(std::string_view gameId) {
    const uint64_t title = Layout::titleIdForGameId(gameId);
    if (title == 0) return {};
    char buffer[16] = {};
    std::snprintf(buffer, sizeof(buffer), "0x%05X ", static_cast<unsigned>(uniqueId(title)));
    return buffer;
}

std::string_view gameIdFromCheckpointFolder(std::string_view folderName) {
    for (const auto id : GameIds) {
        const std::string prefix = checkpointFolderPrefix(id);
        if (folderName.size() > prefix.size() && folderName.substr(0, prefix.size()) == prefix) return id;
    }
    return {};
}

std::optional<std::string> dekoponUserDir(std::optional<std::string_view> pointerContents) {
    if (!pointerContents) return std::string(DekoponDefaultUserDir);
    std::string path = stripSpaces(*pointerContents);
    if (!path.empty() && path.back() != '/') path.push_back('/');
    if (path.empty() || path == DekoponDefaultUserDir) return std::string(DekoponDefaultUserDir);
    if (path.rfind("sdmc:/", 0) != 0) return std::nullopt;  // USB or other device: unreachable here
    return path;
}

std::string emulatorSaveFile(std::string_view userDir, std::string_view gameId) {
    const uint64_t title = Layout::titleIdForGameId(gameId);
    if (title == 0 || userDir.empty()) return {};
    char tail[64] = {};
    std::snprintf(tail, sizeof(tail), "title/%08x/%08x/data/00000001/main",
                  static_cast<unsigned>(title >> 32), static_cast<unsigned>(title & 0xFFFFFFFFu));
    std::string out(userDir);
    if (out.back() != '/') out.push_back('/');
    out += "sdmc/Nintendo 3DS/00000000000000000000000000000000/00000000000000000000000000000000/";
    out += tail;
    return out;
}

Probe probeSave(std::span<const uint8_t> bytes, const SourceContext& context) {
    Probe out;
    auto& row = out.instance;
    const auto& provider = descriptor(context.provider);
    row.sourceIndex = context.validationHandle;
    row.gameId = std::string(context.assignedExactGame);
    row.generation = 6;
    row.platformLabel = std::string(platformFor(context.provider));
    row.kind = context.provider == ProviderId::ManualLink ? Source::SaveInstanceKind::ManualImport
               : context.provider == ProviderId::CheckpointLayout || context.provider == ProviderId::JksmLayout
                   ? Source::SaveInstanceKind::Backup
                   : Source::SaveInstanceKind::BatterySave;
    row.providerId = std::string(provider.providerId);
    row.providerLabel = std::string(provider.label);
    row.sourcePath = std::string(context.sourcePath);
    row.location = row.sourcePath;
    row.normalizedPath = std::string(context.normalizedPath);
    row.physicalIdentity = std::string(context.physicalIdentity);
    row.sourceIdentity = std::string(context.sourceIdentity);
    row.claimedProfile = std::string(context.claimedProfile);
    row.fileSize = bytes.size();
    row.containerType = "3ds-save-archive-main";
    row.access = Source::AccessMode::ReadOnly;

    if (provider.status == ProviderStatus::Unsupported) {
        row.validation = Source::ValidationStatus::Unsupported;
        row.diagnostic = "Gen VI provider has no verified Switch save path";
        return out;
    }
    if (!Layout::isGen6GameId(context.assignedExactGame) || context.sourcePath.empty()) {
        row.validation = Source::ValidationStatus::Unsupported;
        row.diagnostic = "Gen VI requires an explicit exact-game assignment and a source path";
        return out;
    }

    std::string error;
    auto save = Gen6Save::parse(bytes, {}, &error);
    if (!save) {
        row.validation = error.find("unsupported") != std::string::npos ? Source::ValidationStatus::Unsupported
                                                                        : Source::ValidationStatus::Invalid;
        row.diagnostic = error;
        return out;
    }
    if (save->exactGameId() != context.assignedExactGame) {
        row.validation = Source::ValidationStatus::AssignmentMismatch;
        row.diagnostic = "Validated Gen VI save is " + std::string(save->exactGameId()) +
                         "; reassign the source explicitly";
        return out;
    }
    if (!context.evidenceGameId.empty() && context.evidenceGameId != save->exactGameId()) {
        row.validation = Source::ValidationStatus::AssignmentMismatch;
        row.diagnostic = "Gen VI save contents disagree with the provider's title-ID folder";
        return out;
    }

    row.validation = Source::ValidationStatus::Ready;
    row.partyCount = save->partyCount();
    // Only plain printable ASCII becomes a visible label; other text stays unlabelled
    // rather than guessing a glyph mapping.
    std::string name;
    for (const char16_t ch : save->trainer().rawName) {
        if (ch < 0x20 || ch > 0x7E) { name.clear(); break; }
        name.push_back(static_cast<char>(ch));
    }
    row.trainerName = name;
    row.diagnostic = "Gen VI " + std::string(save->family() == Layout::Family::XY ? "X/Y" : "OR/AS") +
                     " save validated (all block checksums); source is read-only";
    if (save->diagnostics().invalidBoxRecords > 0)
        row.diagnostic += "; malformed boxed PK6 entries quarantined: " +
                          std::to_string(save->diagnostics().invalidBoxRecords);
    out.save = std::move(save);
    return out;
}

std::vector<DiscoveredFile> findCheckpointExports(const std::string& checkpointRoot) {
    std::vector<DiscoveredFile> out;
    for (const auto& folder : listDirectories(checkpointRoot)) {
        const auto game = gameIdFromCheckpointFolder(folder);
        if (game.empty()) continue;
        const std::string titleDir = join(checkpointRoot, folder);
        for (const auto& backup : listDirectories(titleDir)) {
            const std::string file = join(join(titleDir, backup), "main");
            if (isRegularFile(file))
                out.push_back({ProviderId::CheckpointLayout, file, std::string(game)});
        }
    }
    return out;
}

std::vector<DiscoveredFile> findEmulatorSaves(ProviderId provider, const std::string& userDir) {
    std::vector<DiscoveredFile> out;
    if (descriptor(provider).status != ProviderStatus::Verified ||
        (provider != ProviderId::DekoponEmulator))
        return out;
    for (const auto id : GameIds) {
        const std::string file = emulatorSaveFile(userDir, id);
        if (isRegularFile(file)) out.push_back({provider, file, std::string(id)});
    }
    return out;
}

std::optional<std::vector<uint8_t>> readCandidate(const std::string& path, std::string* error) {
    const auto fail = [&](const char* why) -> std::optional<std::vector<uint8_t>> {
        if (error) *error = why;
        return std::nullopt;
    };
    struct stat st{};
    if (::lstat(path.c_str(), &st) != 0 || !S_ISREG(st.st_mode)) return fail("Gen VI source is not a regular file");
    const auto size = static_cast<size_t>(st.st_size);
    if (size != Layout::XYSize && size != Layout::ORASSize && size != Layout::ORASDemoSize)
        return fail("Gen VI source size is not a known SAV6 size");
    FILE* file = std::fopen(path.c_str(), "rb");
    if (!file) return fail("Gen VI source cannot be opened for reading");
    std::vector<uint8_t> bytes(size);
    const bool ok = std::fread(bytes.data(), 1, size, file) == size && std::fgetc(file) == EOF;
    std::fclose(file);
    if (!ok) return fail("Gen VI source changed size or could not be read completely");
    if (error) error->clear();
    return bytes;
}

} // namespace PokeVault::Integration::Gen6::Sources
