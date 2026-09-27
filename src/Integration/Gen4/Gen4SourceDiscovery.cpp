#include "Integration/Gen4/Gen4SourceDiscovery.h"

#include "Utils/SHA256.h"
#include "Utils/StringHelpers.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdio>
#include <dirent.h>
#include <set>
#include <sys/stat.h>
#include <vector>
#include <string_view>

namespace PokeVault::Integration::Gen4 {
namespace {
constexpr size_t SAVE_SIZE = 0x80000;

std::string trim(std::string value) {
    const auto notSpace=[](unsigned char c){ return !std::isspace(c); };
    value.erase(value.begin(),std::find_if(value.begin(),value.end(),notSpace));
    value.erase(std::find_if(value.rbegin(),value.rend(),notSpace).base(),value.end());
    return value;
}

std::vector<std::string> retroArchRootsFromConfig(const std::string& configPath) {
    std::vector<std::string> roots;
    FILE* file=std::fopen(configPath.c_str(),"rb");
    if(!file) return roots;
    char line[2048];
    while(std::fgets(line,sizeof(line),file)) {
        std::string value(line);
        const size_t equals=value.find('=');
        if(equals==std::string::npos || trim(value.substr(0,equals))!="savefile_directory") continue;
        value=trim(value.substr(equals+1));
        if(value.size()>=2 && value.front()=='"' && value.back()=='"')
            value=value.substr(1,value.size()-2);
        if(!value.empty() && value!="default") {
            if(value.front()=='/' || value.find(":/")!=std::string::npos) roots.push_back(value);
            else {
                const size_t slash=configPath.find_last_of("/\\");
                roots.push_back((slash==std::string::npos?std::string("."):configPath.substr(0,slash))+"/"+value);
            }
        }
        break;
    }
    std::fclose(file);
    return roots;
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
        } else slash = false;
        result.push_back(c);
    }
    while (result.size() > 1 && result.back() == '/') result.pop_back();
    return result;
}

std::string join(const std::string& root, const std::string& leaf) {
    if (root.empty() || root.back() == '/') return root + leaf;
    return root + "/" + leaf;
}

bool isDirectory(const std::string& path) {
    struct stat st{};
    return ::stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode);
}

bool isRegular(const std::string& path, struct stat* out = nullptr) {
    struct stat st{};
    if (::stat(path.c_str(), &st) != 0 || !S_ISREG(st.st_mode)) return false;
    if (out) *out = st;
    return true;
}

std::string extension(std::string_view path) {
    const size_t dot = path.find_last_of('.');
    if (dot == std::string_view::npos) return {};
    std::string ext(path.substr(dot));
    std::transform(ext.begin(), ext.end(), ext.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return ext;
}

bool discoveryExtension(std::string_view path) {
    const std::string ext = extension(path);
    return ext == ".sav" || ext == ".srm" || ext == ".dsv";
}

std::string sha256Hex(std::string_view value) {
    Utils::SHA256 hash;
    hash.update(reinterpret_cast<const uint8_t*>(value.data()), value.size());
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

std::string sourceIdentityForPath(const std::string& path) {
    const std::string key = "gen4-file:" + normalizedPath(path);
    return sha256Hex(key);
}

bool readExactly(const std::string& path, size_t expected, std::vector<uint8_t>& bytes) {
    FILE* file = std::fopen(path.c_str(), "rb");
    if (!file) return false;
    bytes.resize(expected);
    const size_t read = std::fread(bytes.data(), 1, expected, file);
    const int trailing = std::fgetc(file);
    const bool closeOk = std::fclose(file) == 0;
    if (read != expected || trailing != EOF || !closeOk) {
        bytes.clear();
        return false;
    }
    return true;
}

bool hasDesmumeFooterMarker(const std::string& path, uint64_t size) {
    if (size <= SAVE_SIZE) return false;
    FILE* file = std::fopen(path.c_str(), "rb");
    if (!file) return false;
    constexpr size_t tailSize = 512;
    const size_t want = static_cast<size_t>(std::min<uint64_t>(size, tailSize));
    if (std::fseek(file, static_cast<long>(size - want), SEEK_SET) != 0) {
        std::fclose(file);
        return false;
    }
    std::vector<char> tail(want);
    const size_t got = std::fread(tail.data(), 1, tail.size(), file);
    std::fclose(file);
    static constexpr std::string_view marker = "|-DESMUME SAVE-|";
    return std::search(tail.begin(), tail.begin() + static_cast<std::ptrdiff_t>(got),
                       marker.begin(), marker.end()) != tail.begin() + static_cast<std::ptrdiff_t>(got);
}

const char* familyName(Layout layout) noexcept {
    switch (layout) {
        case Layout::DiamondPearl: return "DP";
        case Layout::Platinum: return "PT";
        case Layout::HeartGoldSoulSilver: return "HGSS";
    }
    return "";
}

std::string sourceTypeForPath(std::string_view rootType, const std::string& path) {
    if (rootType == "RetroArch" && extension(path) == ".dsv") return "RetroArch/DeSmuME";
    return std::string(rootType);
}

struct ScanState {
    DiscoveryLimits limits;
    DiscoveryResult result;
    std::set<std::string> visitedDirectories;
    std::set<std::string> visitedFiles;
};

void scanDirectory(const DiscoveryRoot& root, const std::string& path, size_t depth, ScanState& state) {
    if (state.result.limitReached || depth > root.maxDepth) return;

    struct stat directoryInfo{};
    if (::stat(path.c_str(), &directoryInfo) != 0 || !S_ISDIR(directoryInfo.st_mode)) return;
    const std::string directoryKey = directoryInfo.st_ino
        ? std::to_string(static_cast<unsigned long long>(directoryInfo.st_dev)) + ":" +
          std::to_string(static_cast<unsigned long long>(directoryInfo.st_ino))
        : normalizedPath(path);
    if (!state.visitedDirectories.insert(directoryKey).second) return;

    DIR* directory = opendir(path.c_str());
    if (!directory) return;
    std::vector<std::string> names;
    while (const dirent* entry = readdir(directory)) {
        const std::string name(entry->d_name);
        if (name != "." && name != "..") names.push_back(name);
    }
    closedir(directory);
    std::sort(names.begin(), names.end());

    for (const auto& name : names) {
        if (state.result.limitReached) break;
        const std::string child = join(path, name);
        struct stat st{};
        if (::stat(child.c_str(), &st) != 0) continue;
        if (S_ISDIR(st.st_mode)) {
            if (depth < root.maxDepth) scanDirectory(root, child, depth + 1, state);
            continue;
        }
        if (!S_ISREG(st.st_mode) || !discoveryExtension(name)) continue;
        const std::string fileKey = st.st_ino
            ? std::to_string(static_cast<unsigned long long>(st.st_dev)) + ":" +
              std::to_string(static_cast<unsigned long long>(st.st_ino))
            : normalizedPath(child);
        if (!state.visitedFiles.insert(fileKey).second) continue;
        if (state.result.filesExamined >= state.limits.maxFiles) {
            state.result.limitReached = true;
            break;
        }
        ++state.result.filesExamined;
        SourceCandidate candidate = inspectSourceFile(
            child, sourceTypeForPath(root.sourceType, child));
        // Keep ready Gen IV saves and explicit wrapper/invalid candidates that came from
        // a known DS provider. This gives setup diagnostics without surfacing unrelated files.
        if (candidate.ready() || candidate.status == CandidateStatus::UnsupportedWrapper ||
            root.sourceType != "RetroArch")
            state.result.candidates.push_back(std::move(candidate));
    }
}

}

const char* candidateStatusName(CandidateStatus status) noexcept {
    switch (status) {
        case CandidateStatus::Ready: return "Ready";
        case CandidateStatus::InvalidSave: return "Invalid";
        case CandidateStatus::UnsupportedWrapper: return "Unsupported wrapper";
        case CandidateStatus::ReadError: return "Read error";
        case CandidateStatus::AssignmentMismatch: return "Assignment mismatch";
    }
    return "Unknown";
}

bool candidateMatchesGame(const SourceCandidate& candidate, std::string_view gameId) noexcept {
    if (!candidate.ready()) return false;
    Enums::GameVersion assigned = Enums::GameVersion::Invalid;
    return Gen4ReadOnlySave::validateAssignment(
        candidate.layout, candidate.exactGameFromSave, gameId, &assigned) !=
        AssignmentStatus::Mismatch;
}

SourceCandidate inspectSourceFile(
    const std::string& path, std::string_view sourceType, std::string_view assignedGameId) {
    SourceCandidate result;
    result.path = path;
    result.normalizedPath = normalizedPath(path);
    result.sourceIdentity = sourceIdentityForPath(path);
    result.sourceType = sourceType.empty() ? "Manual" : std::string(sourceType);

    struct stat st{};
    if (!isRegular(path, &st)) {
        result.status = CandidateStatus::ReadError;
        result.diagnostic = "candidate is missing or is not a regular file";
        return result;
    }
    result.fileSize = static_cast<uint64_t>(st.st_size);
    result.modifiedTime = static_cast<int64_t>(st.st_mtime);

    if (result.fileSize != SAVE_SIZE) {
        if (extension(path) == ".dsv" && hasDesmumeFooterMarker(path, result.fileSize)) {
            result.status = CandidateStatus::UnsupportedWrapper;
            result.diagnostic =
                "DeSmuME .dsv footer wrapper detected; G4-02 does not trim wrapper bytes";
        } else {
            result.status = CandidateStatus::InvalidSave;
            result.diagnostic = "candidate is not an exact 0x80000-byte Generation IV save";
        }
        return result;
    }

    std::vector<uint8_t> bytes;
    if (!readExactly(path, SAVE_SIZE, bytes)) {
        result.status = CandidateStatus::ReadError;
        result.diagnostic = "candidate could not be read completely without trailing bytes";
        return result;
    }

    std::optional<Gen4ReadOnlySave> parsed;
    size_t validLayouts = 0;
    for (Layout layout : {Layout::DiamondPearl, Layout::Platinum, Layout::HeartGoldSoulSilver}) {
        auto candidate = Gen4ReadOnlySave::parse(bytes, layout);
        if (!candidate) continue;
        ++validLayouts;
        if (validLayouts == 1) {
            result.layout = layout;
            parsed = std::move(candidate);
        }
    }
    if (validLayouts != 1 || !parsed) {
        result.status = CandidateStatus::InvalidSave;
        result.diagnostic = validLayouts == 0
            ? "strict Gen IV parser rejected every supported layout"
            : "candidate validated as more than one Gen IV layout";
        return result;
    }

    result.expectedRawFamily = familyName(result.layout);
    result.exactGameFromSave = parsed->exactGameFromSave();
    result.recoveredOlderCopy = parsed->recovered();
    result.trainerName = Utils::utf16ToUtf8(parsed->trainer().name);
    result.partyCount = parsed->partyCount();

    if (!assignedGameId.empty()) {
        Enums::GameVersion assigned = Enums::GameVersion::Invalid;
        if (Gen4ReadOnlySave::validateAssignment(
                result.layout, result.exactGameFromSave, assignedGameId, &assigned) ==
            AssignmentStatus::Mismatch) {
            result.status = CandidateStatus::AssignmentMismatch;
            result.diagnostic = "validated save family does not match the selected game card";
            return result;
        }
    }

    result.status = CandidateStatus::Ready;
    result.diagnostic = result.recoveredOlderCopy
        ? "validated read-only Gen IV source; RecoveredOlderCopy"
        : "validated read-only Gen IV source";
    return result;
}

DiscoveryResult discoverSources(std::span<const DiscoveryRoot> roots, DiscoveryLimits limits) {
    if (limits.maxFiles == 0) limits.maxFiles = 1;
    ScanState state{limits, {}, {}, {}};
    for (const auto& root : roots) {
        if (state.result.limitReached) break;
        if (!root.path.empty() && isDirectory(root.path))
            scanDirectory(root, root.path, 0, state);
    }
    return std::move(state.result);
}

DiscoveryResult discoverKnownSources(
    DiscoveryLimits limits, const std::string& retroArchConfig,
    const std::string& retroArchFallback) {
    std::vector<DiscoveryRoot> roots;

    const auto configured = retroArchRootsFromConfig(retroArchConfig);
    if (!configured.empty() && isDirectory(configured.front()))
        roots.push_back({configured.front(), "RetroArch", 2});
    else if (isDirectory(retroArchFallback))
        roots.push_back({retroArchFallback, "RetroArch", 2});

    // Switch-native emulator roots only. No sdmc:/ root scan is ever performed.
    for (const auto& root : {
            DiscoveryRoot{"sdmc:/switch/drastic/user/backup", "DraStic", 1},
            DiscoveryRoot{"sdmc:/switch/drastic/backup", "DraStic", 1},
            DiscoveryRoot{"sdmc:/switch/melonds", "melonDS", 2},
            DiscoveryRoot{"sdmc:/melonds", "melonDS", 2},
        }) {
        if (isDirectory(root.path)) roots.push_back(root);
    }
    return discoverSources(roots, limits);
}

}
