#ifndef POKEBANK_PATHS_H
#define POKEBANK_PATHS_H

#include <cerrno>
#include <cctype>
#include <cstdio>
#include <string>
#include <string_view>
#include <sys/stat.h>

#include "Utils/NXTypes.h"

namespace PokeBank::Paths {

inline constexpr std::string_view kRoot = "sdmc:/switch/PokeBank-NX";

inline std::string root() { return std::string(kRoot); }
inline std::string exportsRoot() { return root() + "/exports"; }
inline std::string gen1ExportsRoot() { return exportsRoot() + "/gen1"; }
inline std::string gen2ExportsRoot() { return exportsRoot() + "/gen2"; }
inline std::string backupsRoot() { return root() + "/backups"; }
inline std::string transactionsRoot() { return root() + "/transactions"; }

inline bool hasValidAccountUid(AccountUid uid) noexcept {
    return uid.uid[0] != 0 || uid.uid[1] != 0;
}

inline std::string accountUidHex(AccountUid uid) {
    if (!hasValidAccountUid(uid)) return {};
    char buffer[33];
    std::snprintf(buffer, sizeof(buffer), "%016llx%016llx",
                  static_cast<unsigned long long>(uid.uid[0]),
                  static_cast<unsigned long long>(uid.uid[1]));
    return std::string(buffer);
}

inline std::string backupProfileComponent(AccountUid uid) {
    const std::string id = accountUidHex(uid);
    return id.empty() ? std::string{} : "account-" + id;
}

inline std::string configRoot() { return root() + "/config"; }
inline std::string logsRoot() { return root() + "/logs"; }
inline std::string banksRoot() { return root() + "/banks"; }
inline std::string legacyBankRoot() { return banksRoot() + "/legacy-pkse"; }
inline std::string settingsFile() { return configRoot() + "/settings.cfg"; }
inline std::string legacySourceBindingsFile() { return configRoot() + "/legacy_source_bindings.cfg"; }
inline std::string traceFile() { return logsRoot() + "/trace.log"; }

inline bool isSafeTimestamp(std::string_view value) noexcept {
    if (value.size() != 15 || value[8] != '_') return false;
    for (size_t i = 0; i < value.size(); ++i) {
        if (i == 8) continue;
        if (value[i] < '0' || value[i] > '9') return false;
    }
    return true;
}

inline bool isSafeComponent(std::string_view value) noexcept {
    if (value.empty() || value == "." || value == "..") return false;
    for (const unsigned char c : value) {
        if (!(std::isalnum(c) || c == '-' || c == '_' || c == '.')) return false;
    }
    return true;
}

inline std::string sanitizeComponent(std::string_view value) {
    std::string out;
    out.reserve(value.size());
    for (const unsigned char c : value) {
        out.push_back((std::isalnum(c) || c == '-' || c == '_' || c == '.')
                ? static_cast<char>(c) : '_');
    }
    while (out == "." || out == "..") out.push_back('_');
    if (out.empty()) out = "unnamed";
    if (out.size() > 96) out.resize(96);
    return out;
}

inline std::string child(const std::string& parent, std::string_view component) {
    if (!isSafeComponent(component)) return {};
    return parent + "/" + std::string(component);
}

inline std::string profileBackupsRoot(AccountUid uid) {
    const std::string profile = backupProfileComponent(uid);
    return profile.empty() ? std::string{} : child(backupsRoot(), profile);
}

inline std::string exactGameBackupsRoot(AccountUid uid, std::string_view exactGameId) {
    const std::string profileRoot = profileBackupsRoot(uid);
    if (profileRoot.empty() || !isSafeComponent(exactGameId)) return {};
    return child(profileRoot, exactGameId);
}

inline std::string workspaceBackupPath(AccountUid uid, std::string_view exactGameId,
                                       std::string_view workspace) {
    if (!isSafeComponent(workspace)) return {};
    const std::string gameRoot = exactGameBackupsRoot(uid, exactGameId);
    return gameRoot.empty() ? std::string{} : child(gameRoot, workspace);
}

// Compatibility-only view of the pre-A08 title-based layout. Never use this helper as a
// writable destination: it intentionally carries no account or exact-release ownership.
inline std::string legacyUnscopedGameBackupsRoot(std::string_view titleName) {
    return child(backupsRoot(), sanitizeComponent(titleName));
}

inline bool isOwnedPath(std::string_view path) noexcept {
    if (path == kRoot) return true;
    return path.size() > kRoot.size() && path.substr(0, kRoot.size()) == kRoot &&
 path[kRoot.size()] == '/';
}

inline bool directoryExists(const std::string& path) noexcept {
    struct stat st{};
    return ::stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode);
}

inline bool ensureOneDirectory(const std::string& path, std::string* error = nullptr) {
    if (directoryExists(path)) return true;
    if (::mkdir(path.c_str(), 0777) == 0) return true;
    if (errno == EEXIST && directoryExists(path)) return true;
    if (error) *error = "Could not create directory: " + path;
    return false;
}

inline bool ensureDirectoryTree(const std::string& path, std::string* error = nullptr) {
    if (path.empty()) {
        if (error) *error = "Directory path is empty";
        return false;
    }

    std::string current;
    size_t pos = 0;
    if (path.rfind("sdmc:/", 0) == 0) {
        current = "sdmc:";
        pos = 6;
    } else if (path[0] == '/') {
        pos = 1;
    }

    while (pos <= path.size()) {
        const size_t slash = path.find('/', pos);
        const size_t end = slash == std::string::npos ? path.size() : slash;
        const std::string_view part(path.data() + pos, end - pos);
        if (!part.empty()) {
  if (part == "." || part == "..") {
      if (error) *error = "Refusing traversal component in directory path";
      return false;
  }
  if (current.empty()) current = path[0] == '/' ? "/" + std::string(part) : std::string(part);
  else current += "/" + std::string(part);
  if (!ensureOneDirectory(current, error)) return false;
        }
        if (slash == std::string::npos) break;
        pos = slash + 1;
    }
    return true;
}

inline bool ensureRuntimeRoot(std::string* error = nullptr) {
    return ensureDirectoryTree(root(), error);
}
inline bool ensureConfigRoot(std::string* error = nullptr) {
    return ensureDirectoryTree(configRoot(), error);
}
inline bool ensureLogsRoot(std::string* error = nullptr) {
    return ensureDirectoryTree(logsRoot(), error);
}
inline bool ensureBackupsRoot(std::string* error = nullptr) {
    return ensureDirectoryTree(backupsRoot(), error);
}
inline bool ensureTransactionsRoot(std::string* error = nullptr) {
    return ensureDirectoryTree(transactionsRoot(), error);
}
inline bool ensureLegacyBankRoot(std::string* error = nullptr) {
    return ensureDirectoryTree(legacyBankRoot(), error);
}
inline bool ensureGen1ExportsRoot(std::string* error = nullptr) {
    return ensureDirectoryTree(gen1ExportsRoot(), error);
}
inline bool ensureGen2ExportsRoot(std::string* error = nullptr) {
    return ensureDirectoryTree(gen2ExportsRoot(), error);
}

inline std::string gen1ExportDirectory(std::string_view gameId, std::string_view timestamp) {
    if (!isSafeTimestamp(timestamp)) return {};
    const std::string safeGame = sanitizeComponent(gameId);
    return gen1ExportsRoot() + "/" + safeGame + "_" + std::string(timestamp);
}

inline std::string gen2ExportDirectory(std::string_view gameId, std::string_view timestamp) {
    if (!isSafeTimestamp(timestamp)) return {};
    const std::string safeGame = sanitizeComponent(gameId);
    return gen2ExportsRoot() + "/" + safeGame + "_" + std::string(timestamp);
}

} // namespace PokeBank::Paths

#endif
