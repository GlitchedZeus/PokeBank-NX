#include "Legacy/RetroArchFRLGDiscovery.h"
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
        using Integration::Gen3::SourceGame;

        enum class IdentityHint {
            None, Ruby, Sapphire, Emerald, FireRed, LeafGreen, Ambiguous
        };

        std::string trim(std::string value) {
            const auto notSpace = [](unsigned char c) { return !std::isspace(c); };
            value.erase(value.begin(), std::find_if(value.begin(), value.end(), notSpace));
            value.erase(std::find_if(value.rbegin(), value.rend(), notSpace).base(), value.end());
            return value;
        }

        std::string dirname(const std::string& path) {
            const size_t slash = path.find_last_of("/\\");
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

        std::string sha256Hex(const uint8_t* bytes, size_t size) {
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
            const size_t dot = name.find_last_of('.');
            if (dot == std::string::npos) return false;
            std::string extension = name.substr(dot);
            std::transform(extension.begin(), extension.end(), extension.begin(),
                [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            return extension == ".sav" || extension == ".srm";
        }

        IdentityHint identityHint(const std::string& path) {
            std::string compact;
            compact.reserve(path.size());
            for (unsigned char c : path)
                if (std::isalnum(c)) compact.push_back(static_cast<char>(std::tolower(c)));

            const std::array<std::pair<std::string_view, IdentityHint>, 5> names{{
                {"firered", IdentityHint::FireRed}, {"leafgreen", IdentityHint::LeafGreen},
                {"sapphire", IdentityHint::Sapphire}, {"emerald", IdentityHint::Emerald},
                {"ruby", IdentityHint::Ruby},
            }};
            IdentityHint result = IdentityHint::None;
            size_t matches = 0;
            for (const auto& [needle, hint] : names) {
                if (compact.find(needle) == std::string::npos) continue;
                result = hint;
                ++matches;
            }
            return matches > 1 ? IdentityHint::Ambiguous : result;
        }

        SourceGame sourceGameForHint(IdentityHint hint) noexcept {
            switch (hint) {
                case IdentityHint::Ruby: return SourceGame::RubyGBA;
                case IdentityHint::Sapphire: return SourceGame::SapphireGBA;
                case IdentityHint::Emerald: return SourceGame::EmeraldGBA;
                case IdentityHint::LeafGreen: return SourceGame::LeafGreenGBA;
                case IdentityHint::FireRed:
                case IdentityHint::None:
                case IdentityHint::Ambiguous:
                    return SourceGame::FireRedGBA;
            }
            return SourceGame::FireRedGBA;
        }

        Integration::Gen3::ParseResult parseAnyFamily(std::span<const uint8_t> bytes) {
            auto parsed = Integration::Gen3::parse(bytes, SourceGame::FireRedGBA);
            if (parsed || parsed.error != Integration::Gen3::SaveError::UnsupportedGame) return parsed;
            parsed = Integration::Gen3::parse(bytes, SourceGame::RubyGBA);
            if (parsed || parsed.error != Integration::Gen3::SaveError::UnsupportedGame) return parsed;
            return Integration::Gen3::parse(bytes, SourceGame::EmeraldGBA);
        }

        bool readExactly(const std::string& path, size_t expected, std::vector<uint8_t>& bytes) {
            FILE* file = std::fopen(path.c_str(), "rb");
            if (!file) return false;
            bytes.resize(expected);
            const size_t count = std::fread(bytes.data(), 1, bytes.size(), file);
            const int trailing = std::fgetc(file);
            const bool closeOk = std::fclose(file) == 0;
            if (count != expected || trailing != EOF || !closeOk) {
                bytes.clear();
                return false;
            }
            return true;
        }

        FRLGSource inspectFile(const std::string& path, const struct stat& metadata,
                               IdentityHint hint) {
            FRLGSource source;
            source.path = path;
            source.normalizedPath = normalizedPath(path);
            source.sourceIdentity = sourceIdentity(path);
            source.fileSize = static_cast<uint64_t>(metadata.st_size);
            source.modifiedTime = static_cast<int64_t>(metadata.st_mtime);
            const size_t size = static_cast<size_t>(metadata.st_size);
            if (size != 0x20000) {
                source.status = LegacySourceStatus::InvalidSave;
                source.parseError = Integration::Gen3::SaveError::WrongSize;
                source.detail = "candidate is not an exact 128 KiB Gen III save";
                return source;
            }

            std::vector<uint8_t> bytes;
            if (!readExactly(path, size, bytes)) {
                source.status = LegacySourceStatus::ReadError;
                source.detail = "candidate could not be read completely";
                return source;
            }
            source.contentFingerprint = sha256Hex(bytes.data(), bytes.size());

            if (hint == IdentityHint::None || hint == IdentityHint::Ambiguous) {
                auto parsed = parseAnyFamily(bytes);
                if (!parsed) {
                    source.status = LegacySourceStatus::InvalidSave;
                    source.parseError = parsed.error;
                    source.detail = parsed.detail;
                    return source;
                }
                source.status = LegacySourceStatus::AmbiguousIdentity;
                source.detail = hint == IdentityHint::Ambiguous ?
                    "valid Gen III save path contains multiple release identity hints" :
                    "valid Gen III save needs a Ruby/Sapphire/Emerald/FireRed/LeafGreen source hint";
                return source;
            }

            const SourceGame assumed = sourceGameForHint(hint);
            auto parsed = Integration::Gen3::parse(bytes, assumed);
            if (!parsed) {
                source.status = LegacySourceStatus::InvalidSave;
                source.parseError = parsed.error;
                source.detail = parsed.detail;
                return source;
            }

            source.status = LegacySourceStatus::Ready;
            source.gameId = std::string(Integration::Gen3::sourceGameId(assumed));
            source.save = std::move(parsed.save);
            source.detail = "validated read-only RetroArch Generation III source";
            return source;
        }

        struct ScanState {
            ScanLimits limits;
            FRLGDiscoveryResult result;
            std::set<std::string> visitedDirectories;
            std::set<std::string> visitedFiles;
        };

        void scanDirectory(const std::string& root, size_t depth, ScanState& state) {
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
                FRLGSource source = inspectFile(path, metadata, hint);
                source.canonicalPath = fileIdentity;
                // Do not surface every unrelated emulator save. Keep valid Gen III-family files
                // and named candidates so useful validation errors remain visible.
                if (source.ready() || source.status == LegacySourceStatus::AmbiguousIdentity ||
                    hint != IdentityHint::None) {
                    state.result.sources.push_back(std::move(source));
                }
            }
        }

        LegacySourceStatus mapRBYStatus(RBYSourceStatus status) noexcept {
            switch (status) {
                case RBYSourceStatus::Ready: return LegacySourceStatus::Ready;
                case RBYSourceStatus::InvalidSave: return LegacySourceStatus::InvalidSave;
                case RBYSourceStatus::AmbiguousIdentity: return LegacySourceStatus::AmbiguousIdentity;
                case RBYSourceStatus::ReadError: return LegacySourceStatus::ReadError;
                case RBYSourceStatus::ScanLimitReached: return LegacySourceStatus::ScanLimitReached;
            }
            return LegacySourceStatus::ReadError;
        }

        FRLGSource importRBYSource(const RBYSource& input) {
            FRLGSource output;
            output.path = input.path;
            output.normalizedPath = input.normalizedPath;
            output.sourceIdentity = input.sourceIdentity;
            output.canonicalPath = input.canonicalPath;
            output.fileSize = input.fileSize;
            output.modifiedTime = input.modifiedTime;
            output.contentFingerprint = input.contentFingerprint;
            output.gameId = input.gameId;
            output.status = mapRBYStatus(input.status);
            output.gen1ParseError = input.parseError;
            output.detail = input.detail;
            output.gen1Save = input.save;
            return output;
        }
    }

    std::vector<std::string> retroArchSaveRootsFromConfig(const std::string& configPath) {
        std::vector<std::string> roots;
        FILE* file = std::fopen(configPath.c_str(), "rb");
        if (!file) return roots;
        char line[2048];
        while (std::fgets(line, sizeof(line), file)) {
            std::string value(line);
            const size_t equals = value.find('=');
            if (equals == std::string::npos || trim(value.substr(0, equals)) != "savefile_directory")
                continue;
            value = trim(value.substr(equals + 1));
            if (value.size() >= 2 && value.front() == '"' && value.back() == '"')
                value = value.substr(1, value.size() - 2);
            if (!value.empty() && value != "default") {
                roots.push_back(isAbsoluteOrDevicePath(value) ? value : join(dirname(configPath), value));
            }
            break;
        }
        std::fclose(file);
        return roots;
    }

    FRLGDiscoveryResult discoverFRLGSaves(
        std::span<const std::string> approvedRoots, ScanLimits limits) {
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

    FRLGDiscoveryResult discoverConfiguredRetroArchFRLGSaves(
        ScanLimits limits, const std::string& configPath,
        const std::string& conventionalRoot) {
        const auto configuredRoots = retroArchSaveRootsFromConfig(configPath);
        std::vector<std::string> selectedRoots;
        FRLGDiscoveryResult::RootKind kind = FRLGDiscoveryResult::RootKind::None;
        if (!configuredRoots.empty() && isDirectory(configuredRoots.front())) {
            selectedRoots.push_back(configuredRoots.front());
            kind = FRLGDiscoveryResult::RootKind::Configured;
        } else if (isDirectory(conventionalRoot)) {
            selectedRoots.push_back(conventionalRoot);
            kind = FRLGDiscoveryResult::RootKind::ConventionalFallback;
        }

        // Keep the already accepted Gen III traversal unchanged, then independently run the strict
        // Gen I scanner over the same approved RetroArch battery-save root. The two parsers never
        // reinterpret each other's data; this merge is presentation/catalog plumbing only.
        auto result = discoverFRLGSaves(selectedRoots, limits);
        result.activeRootKind = kind;

        RBYScanLimits rbyLimits;
        rbyLimits.maxDepth = limits.maxDepth;
        rbyLimits.maxFiles = limits.maxFiles;
        auto rby = discoverConfiguredRetroArchRBYSaves(
            rbyLimits, configPath, conventionalRoot);
        result.filesExamined = std::max(result.filesExamined, rby.filesExamined);
        result.limitReached = result.limitReached || rby.limitReached;
        if (result.activeRoot.empty()) result.activeRoot = rby.activeRoot;
        if (result.activeRootKind == FRLGDiscoveryResult::RootKind::None) {
            switch (rby.activeRootKind) {
                case RBYDiscoveryResult::RootKind::Configured:
                    result.activeRootKind = FRLGDiscoveryResult::RootKind::Configured;
                    break;
                case RBYDiscoveryResult::RootKind::ConventionalFallback:
                    result.activeRootKind = FRLGDiscoveryResult::RootKind::ConventionalFallback;
                    break;
                case RBYDiscoveryResult::RootKind::None:
                    break;
            }
        }
        result.sources.reserve(result.sources.size() + rby.sources.size());
        for (const auto& source : rby.sources)
            result.sources.push_back(importRBYSource(source));
        return result;
    }
}
