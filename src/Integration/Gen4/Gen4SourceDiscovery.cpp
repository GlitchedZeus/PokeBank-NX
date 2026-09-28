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

std::string leafName(std::string_view path) {
    const size_t slash = path.find_last_of("/\\");
    return std::string(slash == std::string_view::npos ? path : path.substr(slash + 1));
}

bool discoveryExtension(std::string_view path) {
    const std::string ext = extension(path);
    return ext == ".sav" || ext == ".srm" || ext == ".dsv" || ext == ".dss";
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

uint32_t readLe32(const uint8_t* p) noexcept {
    return static_cast<uint32_t>(p[0]) |
           (static_cast<uint32_t>(p[1]) << 8) |
           (static_cast<uint32_t>(p[2]) << 16) |
           (static_cast<uint32_t>(p[3]) << 24);
}

bool hasDesmumeFooterMarker(const std::string& path, uint64_t size) {
    if (size < 16) return false;
    FILE* file = std::fopen(path.c_str(), "rb");
    if (!file) return false;
    if (std::fseek(file, static_cast<long>(size - 16), SEEK_SET) != 0) {
        std::fclose(file);
        return false;
    }
    std::array<char,16> cookie{};
    const bool readOk = std::fread(cookie.data(), 1, cookie.size(), file) == cookie.size();
    const bool closeOk = std::fclose(file) == 0;
    const bool ok = readOk && closeOk;
    static constexpr std::array<char,16> marker{
        '|','-','D','E','S','M','U','M','E',' ','S','A','V','E','-','|'
    };
    return ok && cookie == marker;
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
            candidate.status == CandidateStatus::UnsupportedSavestate ||
            root.sourceType != "RetroArch")
            state.result.candidates.push_back(std::move(candidate));
    }
}

}

SourcePayloadRead readSourcePayloadReadOnly(const std::string& path) {
    SourcePayloadRead out;
    struct stat st{};
    if (!isRegular(path, &st)) {
        out.status = SourcePayloadStatus::ReadError;
        out.diagnostic = "source is missing or is not a regular file";
        return out;
    }
    const uint64_t size = static_cast<uint64_t>(st.st_size);

    if (size == SAVE_SIZE) {
        if (!readExactly(path, SAVE_SIZE, out.bytes)) {
            out.status = SourcePayloadStatus::ReadError;
            out.diagnostic = "raw save could not be read completely";
            return out;
        }
        out.status = SourcePayloadStatus::Ready;
        out.kind = SourceContainerKind::Raw;
        out.diagnostic = "exact raw 0x80000-byte save";
        return out;
    }

    if (extension(path) != ".dsv") {
        out.status = SourcePayloadStatus::InvalidSize;
        out.diagnostic = "source is not an exact 0x80000-byte Gen IV save";
        return out;
    }

    // DeSmuME documents a fixed 40-byte footer at EOF:
    // six little-endian u32 values followed by the 16-byte "|-DESMUME SAVE-|" cookie.
    // DraStic's normal .dsv is compatible with this container contract on supported files.
    constexpr size_t footerSize = 40;
    if (size < SAVE_SIZE + footerSize || !hasDesmumeFooterMarker(path, size)) {
        out.status = SourcePayloadStatus::UnsupportedWrapper;
        out.diagnostic = ".dsv wrapper is not the documented footer format";
        return out;
    }

    FILE* file = std::fopen(path.c_str(), "rb");
    if (!file) {
        out.status = SourcePayloadStatus::ReadError;
        out.diagnostic = "could not open .dsv read-only";
        return out;
    }
    std::array<uint8_t, footerSize> footer{};
    bool ok = std::fseek(file, static_cast<long>(size - footerSize), SEEK_SET) == 0 &&
              std::fread(footer.data(), 1, footer.size(), file) == footer.size();
    const uint32_t actuallyWritten = ok ? readLe32(footer.data() + 0) : 0;
    const uint32_t paddedSize      = ok ? readLe32(footer.data() + 4) : 0;
    const uint32_t version         = ok ? readLe32(footer.data() + 20) : 0xFFFFFFFFu;

    if (!ok || paddedSize != SAVE_SIZE || actuallyWritten > paddedSize || version != 0 ||
        size < static_cast<uint64_t>(paddedSize) + footerSize) {
        std::fclose(file);
        out.status = SourcePayloadStatus::UnsupportedWrapper;
        out.diagnostic =
            ".dsv footer is present but its padded size/version is not a supported Gen IV container";
        return out;
    }

    if (std::fseek(file, 0, SEEK_SET) != 0) {
        std::fclose(file);
        out.status = SourcePayloadStatus::ReadError;
        out.diagnostic = "could not seek to the .dsv raw payload";
        return out;
    }
    out.bytes.resize(SAVE_SIZE);
    const size_t read = std::fread(out.bytes.data(), 1, out.bytes.size(), file);
    const bool readError = std::ferror(file) != 0;
    const bool closed = std::fclose(file) == 0;
    if (read != out.bytes.size() || readError || !closed) {
        out.bytes.clear();
        out.status = SourcePayloadStatus::ReadError;
        out.diagnostic = "could not read the .dsv raw payload";
        return out;
    }

    out.status = SourcePayloadStatus::Ready;
    out.kind = SourceContainerKind::DsvFooter;
    out.diagnostic = "validated read-only .dsv container; raw payload ends at padded_size 0x80000";
    return out;
}

const char* candidateStatusName(CandidateStatus status) noexcept {
    switch (status) {
        case CandidateStatus::Ready: return "Ready";
        case CandidateStatus::InvalidSave: return "Invalid";
        case CandidateStatus::UnsupportedWrapper: return "Unsupported wrapper";
        case CandidateStatus::UnsupportedSavestate: return "Unsupported savestate";
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

PokeVault::Source::SaveInstance toSaveInstance(
    const SourceCandidate& candidate, std::string_view gameId,
    size_t validationHandle, bool rememberedSource, std::string_view claimedProfile) {
    using PokeVault::Source::AccessMode;
    using PokeVault::Source::DiagnosticState;
    using PokeVault::Source::SaveInstance;
    using PokeVault::Source::SaveInstanceKind;
    using PokeVault::Source::ValidationStatus;

    SaveInstance instance;
    instance.sourceIndex = validationHandle;
    instance.label = leafName(candidate.path);
    instance.providerLabel = candidate.sourceType.empty() ? "Source" : candidate.sourceType;
    instance.location = candidate.path;
    instance.normalizedPath = candidate.normalizedPath;
    instance.sourceIdentity = candidate.sourceIdentity;
    instance.trainerName = candidate.trainerName;
    instance.fileSize = candidate.fileSize;
    instance.modifiedTime = candidate.modifiedTime;
    instance.partyCount = candidate.partyCount;

    instance.gameId = std::string(gameId);
    instance.generation = 4;
    instance.platformLabel = "Nintendo DS";
    instance.providerId = PokeVault::Source::providerIdFor(instance.providerLabel);
    instance.sourcePath = candidate.path;
    // Gen IV sourceIdentity is deliberately provider-neutral and path-stable, so it is also the
    // correct shared dedupe key for overlapping known roots.
    instance.physicalIdentity = candidate.sourceIdentity;
    const std::string ext = extension(candidate.path);
    if (ext == ".dss") {
        instance.kind = SaveInstanceKind::SaveState;
        instance.containerType = "Savestate";
    } else if (ext == ".dsv") {
        instance.kind = SaveInstanceKind::Backup;
        instance.containerType = "DSV cartridge backup";
    } else if (rememberedSource || instance.providerLabel == "Manual" ||
               instance.providerLabel == "Remembered") {
        instance.kind = SaveInstanceKind::ManualImport;
        instance.containerType = "Manual cartridge save";
    } else {
        instance.kind = SaveInstanceKind::BatterySave;
        instance.containerType = "Raw cartridge save";
    }

    switch (candidate.status) {
        case CandidateStatus::Ready:
            instance.validation = candidateMatchesGame(candidate, gameId)
                ? ValidationStatus::Ready : ValidationStatus::AssignmentMismatch;
            break;
        case CandidateStatus::InvalidSave:
            instance.validation = ValidationStatus::Invalid;
            break;
        case CandidateStatus::UnsupportedWrapper:
        case CandidateStatus::UnsupportedSavestate:
            instance.validation = ValidationStatus::Unsupported;
            break;
        case CandidateStatus::ReadError:
            instance.validation = ValidationStatus::ReadError;
            break;
        case CandidateStatus::AssignmentMismatch:
            instance.validation = ValidationStatus::AssignmentMismatch;
            break;
    }
    instance.access = AccessMode::ReadOnly;
    instance.recoveredOlderCopy = candidate.recoveredOlderCopy;
    instance.diagnosticState = candidate.recoveredOlderCopy
        ? DiagnosticState::RecoveredOlderCopy : DiagnosticState::None;
    instance.rememberedSource = rememberedSource;
    instance.claimedProfile = std::string(claimedProfile);
    instance.diagnostic = candidate.diagnostic;

    std::string detail = candidate.expectedRawFamily.empty()
        ? std::string("Validated Gen IV save") : candidate.expectedRawFamily;
    if (!candidate.trainerName.empty()) detail += " / " + candidate.trainerName;
    detail += " / Party " + std::to_string(candidate.partyCount);
    if (candidate.recoveredOlderCopy) detail += " / RECOVERED OLDER COPY";
    instance.sourceLabel = std::move(detail);
    return instance;
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

    // DraStic .dss is a savestate snapshot, not the cartridge/in-game backup file. Never try to
    // reinterpret it based on size or embedded bytes; savestate extraction is outside G4-02H.
    if (extension(path) == ".dss") {
        result.status = CandidateStatus::UnsupportedSavestate;
        result.diagnostic =
            "DraStic .dss savestate detected. Savestates are not assignable; use the cartridge save under /switch/drastic/user/backup/.";
        return result;
    }

    const auto payload = readSourcePayloadReadOnly(path);
    if (!payload.ready()) {
        if (payload.status == SourcePayloadStatus::UnsupportedWrapper)
            result.status = CandidateStatus::UnsupportedWrapper;
        else if (payload.status == SourcePayloadStatus::ReadError)
            result.status = CandidateStatus::ReadError;
        else
            result.status = CandidateStatus::InvalidSave;
        result.diagnostic = payload.diagnostic;
        return result;
    }
    const auto& bytes = payload.bytes;

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
    const std::string container =
        payload.kind == SourceContainerKind::DsvFooter ? " / .dsv container" : "";
    result.diagnostic = result.recoveredOlderCopy
        ? "validated read-only Gen IV source" + container + "; RecoveredOlderCopy"
        : "validated read-only Gen IV source" + container;
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

std::vector<DiscoveryRoot> defaultDraSticRoots() {
    return {
        {"sdmc:/switch/drastic/user/backup", "DraStic", 1},
        {"sdmc:/switch/drastic/backup", "DraStic", 1},
        {"sdmc:/switch/drastic/user/savestates", "DraStic Savestate", 1},
        {"sdmc:/switch/drastic/savestates", "DraStic Savestate", 1},
    };
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
    for (const auto& root : defaultDraSticRoots())
        if (isDirectory(root.path)) roots.push_back(root);
    for (const auto& root : {
            DiscoveryRoot{"sdmc:/switch/melonds", "melonDS", 2},
            DiscoveryRoot{"sdmc:/melonds", "melonDS", 2},
        }) {
        if (isDirectory(root.path)) roots.push_back(root);
    }
    return discoverSources(roots, limits);
}

}
