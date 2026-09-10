#include "Legacy/RetroArchRBYDiscovery.h"

#include "Utils/SHA256.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdio>
#include <dirent.h>
#include <set>
#include <sys/stat.h>

namespace PokeVault::Legacy {
namespace {
using Integration::Gen1::SourceGame;

enum class IdentityHint { None, Red, Blue, Yellow, Ambiguous };

std::string trim(std::string value) {
    const auto notSpace = [](unsigned char c) { return !std::isspace(c); };
    value.erase(value.begin(), std::find_if(value.begin(), value.end(), notSpace));
    value.erase(std::find_if(value.rbegin(), value.rend(), notSpace).base(), value.end());
    return value;
}

std::string dirname(const std::string& path) {
    const std::size_t slash = path.find_last_of("/\\");
    return slash == std::string::npos ? std::string(".") : path.substr(0, slash);
}

std::string join(const std::string& root, const std::string& name) {
    if (root.empty() || root.back() == '/' || root.back() == '\\') return root + name;
    return root + "/" + name;
}

bool isAbsoluteOrDevicePath(const std::string& path) {
    return !path.empty() && (path.front() == '/' || path.find(":/") != std::string::npos);
}

bool isRegularFile(const std::string& path, struct stat* metadata = nullptr) {
    struct stat info{};
    if (stat(path.c_str(), &info) != 0 || !S_ISREG(info.st_mode)) return false;
    if (metadata) *metadata = info;
    return true;
}

bool isDirectory(const std::string& path) {
    struct stat info{};
    return stat(path.c_str(), &info) == 0 && S_ISDIR(info.st_mode);
}

std::string normalizedPath(std::string path) {
    std::replace(path.begin(), path.end(), '\\', '/');
    std::string result;
    result.reserve(path.size());
    bool slash = false;
    for (char c : path) {
        if (c == '/') {
            if (slash) continue;
            slash = true;
        } else {
            slash = false;
        }
        result.push_back(c);
    }
    while (result.size() > 1 && result.back() == '/') result.pop_back();
    return result;
}

std::string sha256Hex(const uint8_t* bytes, std::size_t size) {
    Utils::SHA256 hash;
    hash.update(bytes, size);
    std::array<uint8_t, Utils::PKSE_SHA256_HASH_SIZE> digest{};
    hash.finalize(digest.data());
    constexpr char digits[] = "0123456789abcdef";
    std::string result;
    result.reserve(digest.size() * 2);
    for (uint8_t byte : digest) {
        result.push_back(digits[byte >> 4]);
        result.push_back(digits[byte & 0x0F]);
    }
    return result;
}

std::string filesystemIdentity(const std::string& path) {
    struct stat info{};
    if (stat(path.c_str(), &info) == 0 && info.st_ino != 0) {
        return "inode:" + std::to_string(static_cast<unsigned long long>(info.st_dev)) +
            ":" + std::to_string(static_cast<unsigned long long>(info.st_ino));
    }
    return normalizedPath(path);
}

std::string sourceIdentity(const std::string& path) {
    const std::string identity = "retroarch:" + normalizedPath(path);
    return sha256Hex(reinterpret_cast<const uint8_t*>(identity.data()), identity.size());
}

bool supportedExtension(const std::string& name) {
    const std::size_t dot = name.find_last_of('.');
    if (dot == std::string::npos) return false;
    std::string ext = name.substr(dot);
    std::transform(ext.begin(), ext.end(), ext.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return ext == ".sav" || ext == ".srm";
}

std::string compactLower(const std::string& path) {
    std::string compact;
    compact.reserve(path.size());
    for (unsigned char c : path)
        if (std::isalnum(c)) compact.push_back(static_cast<char>(std::tolower(c)));
    return compact;
}

IdentityHint identityHint(const std::string& path) {
    const std::string compact = compactLower(path);
    // Explicitly avoid treating accepted Gen III release names as Gen I hints merely because
    // "firered" contains "red". The 32 KiB size + Gen I parser are still the actual authority.
    if (compact.find("firered") != std::string::npos ||
        compact.find("leafgreen") != std::string::npos)
        return IdentityHint::None;

    const bool red = compact.find("pokemonred") != std::string::npos ||
                     compact.find("pokered") != std::string::npos ||
                     compact == "redsav" || compact == "redsrm";
    const bool blue = compact.find("pokemonblue") != std::string::npos ||
                      compact.find("pokeblue") != std::string::npos ||
                      compact == "bluesav" || compact == "bluesrm";
    const bool yellow = compact.find("pokemonyellow") != std::string::npos ||
                        compact.find("pokeyellow") != std::string::npos ||
                        compact == "yellowsav" || compact == "yellowsrm";
    const int matches = static_cast<int>(red) + static_cast<int>(blue) + static_cast<int>(yellow);
    if (matches > 1) return IdentityHint::Ambiguous;
    if (red) return IdentityHint::Red;
    if (blue) return IdentityHint::Blue;
    if (yellow) return IdentityHint::Yellow;

    // After the title-specific forms above, allow an isolated directory/basename token. This is
    // intentionally conservative: e.g. /savefiles/red/game.srm can work, while "redesign.srm"
    // does not accidentally claim a release.
    std::string token;
    for (std::size_t i = 0; i <= path.size(); ++i) {
        const unsigned char c = i < path.size() ? static_cast<unsigned char>(path[i]) : '/';
        if (std::isalnum(c)) {
            token.push_back(static_cast<char>(std::tolower(c)));
            continue;
        }
        if (token == "red") return IdentityHint::Red;
        if (token == "blue") return IdentityHint::Blue;
        if (token == "yellow") return IdentityHint::Yellow;
        token.clear();
    }
    return IdentityHint::None;
}

SourceGame gameForHint(IdentityHint hint) {
    switch (hint) {
        case IdentityHint::Blue: return SourceGame::Blue;
        case IdentityHint::Yellow: return SourceGame::Yellow;
        case IdentityHint::Red:
        case IdentityHint::None:
        case IdentityHint::Ambiguous:
            return SourceGame::Red;
    }
    return SourceGame::Red;
}

bool readExactly(const std::string& path, std::size_t expected, std::vector<uint8_t>& bytes) {
    FILE* file = std::fopen(path.c_str(), "rb");
    if (!file) return false;
    bytes.resize(expected);
    const std::size_t count = std::fread(bytes.data(), 1, bytes.size(), file);
    const int trailing = std::fgetc(file);
    const bool closeOk = std::fclose(file) == 0;
    if (count != expected || trailing != EOF || !closeOk) {
        bytes.clear();
        return false;
    }
    return true;
}

// For an unhinted candidate, validate the Gen I family without pretending Red vs Blue is encoded.
// Try an RB-family hint and Yellow. A strong version contradiction makes one fail; unknown evidence
// may make both succeed, which is still a valid Gen I save but remains identity-ambiguous.
bool validatesAsAnyGen1(std::span<const uint8_t> bytes,
                        Integration::Gen1::SaveError& error,
                        std::string& detail) {
    auto rb = Integration::Gen1::parse(bytes, SourceGame::Red);
    if (rb) return true;
    auto yellow = Integration::Gen1::parse(bytes, SourceGame::Yellow);
    if (yellow) return true;
    // Prefer a non-version structural error for diagnostics when available.
    if (rb.error != Integration::Gen1::SaveError::GameHintMismatch) {
        error = rb.error; detail = rb.detail;
    } else {
        error = yellow.error; detail = yellow.detail;
    }
    return false;
}

RBYSource inspectFile(const std::string& path, const struct stat& metadata, IdentityHint hint) {
    RBYSource source;
    source.path = path;
    source.normalizedPath = normalizedPath(path);
    source.sourceIdentity = sourceIdentity(path);
    source.fileSize = static_cast<uint64_t>(metadata.st_size);
    source.modifiedTime = static_cast<int64_t>(metadata.st_mtime);

    if (metadata.st_size != static_cast<off_t>(Integration::Gen1::kRawSaveSize)) {
        source.status = RBYSourceStatus::InvalidSave;
        source.parseError = Integration::Gen1::SaveError::WrongSize;
        source.detail = "candidate is not an exact 32 KiB Gen I battery save";
        return source;
    }

    std::vector<uint8_t> bytes;
    if (!readExactly(path, Integration::Gen1::kRawSaveSize, bytes)) {
        source.status = RBYSourceStatus::ReadError;
        source.detail = "candidate could not be read completely";
        return source;
    }
    source.contentFingerprint = sha256Hex(bytes.data(), bytes.size());

    if (hint == IdentityHint::None || hint == IdentityHint::Ambiguous) {
        if (!validatesAsAnyGen1(bytes, source.parseError, source.detail)) {
            source.status = RBYSourceStatus::InvalidSave;
            return source;
        }
        source.status = RBYSourceStatus::AmbiguousIdentity;
        source.detail = hint == IdentityHint::Ambiguous
            ? "valid Gen I save path contains multiple Red/Blue/Yellow identity hints"
            : "valid Gen I save needs an explicit Red/Blue/Yellow source hint; Red vs Blue is not stored in the save";
        return source;
    }

    const SourceGame assumed = gameForHint(hint);
    auto parsed = Integration::Gen1::parse(bytes, assumed);
    if (!parsed) {
        source.status = parsed.error == Integration::Gen1::SaveError::GameHintMismatch
            ? RBYSourceStatus::AmbiguousIdentity : RBYSourceStatus::InvalidSave;
        source.parseError = parsed.error;
        source.detail = parsed.detail;
        return source;
    }
    source.status = RBYSourceStatus::Ready;
    source.gameId = Integration::Gen1::sourceGameId(assumed);
    source.save = std::move(parsed.save);
    source.detail = std::string("validated read-only RetroArch Generation I source (") +
        Integration::Gen1::regionName(source.save->metadata().region) + ")";
    return source;
}

struct ScanState {
    RBYScanLimits limits;
    RBYDiscoveryResult result;
    std::set<std::string> visitedDirectories;
    std::set<std::string> visitedFiles;
};

void scanDirectory(const std::string& root, std::size_t depth, ScanState& state) {
    const std::string directoryIdentity = filesystemIdentity(root);
    if (state.result.limitReached || depth > state.limits.maxDepth ||
        !state.visitedDirectories.insert(directoryIdentity).second) return;
    DIR* directory = opendir(root.c_str());
    if (!directory) return;

    std::vector<std::string> names;
    while (const dirent* entry = readdir(directory)) {
        if (std::string(entry->d_name) != "." && std::string(entry->d_name) != "..")
            names.emplace_back(entry->d_name);
    }
    closedir(directory);
    std::sort(names.begin(), names.end());

    for (const auto& name : names) {
        if (state.result.limitReached) break;
        const std::string path = join(root, name);
        if (isDirectory(path)) {
            if (depth < state.limits.maxDepth) scanDirectory(path, depth + 1, state);
            continue;
        }
        if (!supportedExtension(name)) continue;
        struct stat metadata{};
        if (!isRegularFile(path, &metadata)) continue;
        const std::string fileIdentity = filesystemIdentity(path);
        if (!state.visitedFiles.insert(fileIdentity).second) continue;
        if (state.result.filesExamined >= state.limits.maxFiles) {
            state.result.limitReached = true;
            break;
        }
        ++state.result.filesExamined;
        const IdentityHint hint = identityHint(path);
        RBYSource source = inspectFile(path, metadata, hint);
        source.canonicalPath = fileIdentity;
        // Avoid surfacing every unrelated emulator save. Keep valid Gen I-family files and named
        // candidates so useful validation failures remain diagnosable.
        if (source.ready() || source.status == RBYSourceStatus::AmbiguousIdentity ||
            hint != IdentityHint::None)
            state.result.sources.push_back(std::move(source));
    }
}
} // namespace

std::vector<std::string> retroArchRBYSaveRootsFromConfig(const std::string& configPath) {
    std::vector<std::string> roots;
    FILE* file = std::fopen(configPath.c_str(), "rb");
    if (!file) return roots;
    char line[2048];
    while (std::fgets(line, sizeof(line), file)) {
        std::string value(line);
        const std::size_t equals = value.find('=');
        if (equals == std::string::npos || trim(value.substr(0, equals)) != "savefile_directory")
            continue;
        value = trim(value.substr(equals + 1));
        if (value.size() >= 2 && value.front() == '"' && value.back() == '"')
            value = value.substr(1, value.size() - 2);
        if (!value.empty() && value != "default")
            roots.push_back(isAbsoluteOrDevicePath(value) ? value : join(dirname(configPath), value));
        break;
    }
    std::fclose(file);
    return roots;
}

RBYDiscoveryResult discoverRBYSaves(std::span<const std::string> approvedRoots, RBYScanLimits limits) {
    if (limits.maxFiles == 0) limits.maxFiles = 1;
    ScanState state{limits, {}, {}, {}};
    for (const auto& root : approvedRoots) {
        if (state.result.limitReached) break;
        if (isDirectory(root)) {
            if (state.result.activeRoot.empty()) state.result.activeRoot = normalizedPath(root);
            scanDirectory(root, 0, state);
        }
    }
    return std::move(state.result);
}

RBYDiscoveryResult discoverConfiguredRetroArchRBYSaves(
    RBYScanLimits limits, const std::string& configPath, const std::string& conventionalRoot) {
    const auto configuredRoots = retroArchRBYSaveRootsFromConfig(configPath);
    std::vector<std::string> selectedRoots;
    RBYDiscoveryResult::RootKind kind = RBYDiscoveryResult::RootKind::None;
    if (!configuredRoots.empty() && isDirectory(configuredRoots.front())) {
        selectedRoots.push_back(configuredRoots.front());
        kind = RBYDiscoveryResult::RootKind::Configured;
    } else if (isDirectory(conventionalRoot)) {
        selectedRoots.push_back(conventionalRoot);
        kind = RBYDiscoveryResult::RootKind::ConventionalFallback;
    }
    auto result = discoverRBYSaves(selectedRoots, limits);
    result.activeRootKind = kind;
    return result;
}

} // namespace PokeVault::Legacy
