#include "UI/GameLauncher.h"

#include <algorithm>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <dirent.h>
#include <map>
#include <set>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

#include "Utils/PokeBankPaths.h"

#ifdef __SWITCH__
#include <switch.h>
#endif

namespace UI {
namespace {

struct StoredLaunchBinding {
    std::string providerId;
    std::string launcherPath;
    std::string contentPath;
    std::string corePath;
};

using BindingMap = std::map<std::string, StoredLaunchBinding>;
constexpr std::string_view kBindingHeader = "POKEBANK_GAME_LAUNCH_BINDINGS_V1\n";

bool regularFile(const std::string& path) {
    struct stat st{};
    return !path.empty() && ::stat(path.c_str(), &st) == 0 && S_ISREG(st.st_mode);
}

bool directory(const std::string& path) {
    struct stat st{};
    return !path.empty() && ::stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode);
}

std::string switchPath(std::string path) {
    if (!path.empty() && path.front() == '/' && path.find(":/") == std::string::npos)
        return "sdmc:" + path;
    return path;
}

std::string parentPath(std::string path) {
    while (path.size() > 6 && path.back() == '/') path.pop_back();
    const size_t slash = path.find_last_of("/\\");
    if (slash == std::string::npos) return {};
    if (path.rfind("sdmc:/", 0) == 0 && slash <= 5) return "sdmc:/";
    return path.substr(0, slash);
}

bool hasExtension(std::string_view path, std::string_view ext) {
    if (path.size() < ext.size()) return false;
    auto tail = path.substr(path.size() - ext.size());
    for (size_t i = 0; i < ext.size(); ++i)
        if (std::tolower(static_cast<unsigned char>(tail[i])) !=
            std::tolower(static_cast<unsigned char>(ext[i]))) return false;
    return true;
}

bool readTextFile(const std::string& path, std::string& out, size_t maxSize = 4 * 1024 * 1024) {
    FILE* f = std::fopen(path.c_str(), "rb");
    if (!f) return false;
    if (std::fseek(f, 0, SEEK_END) != 0) { std::fclose(f); return false; }
    const long size = std::ftell(f);
    if (size < 0 || static_cast<size_t>(size) > maxSize) { std::fclose(f); return false; }
    std::rewind(f);
    out.resize(static_cast<size_t>(size));
    const size_t got = out.empty() ? 0 : std::fread(out.data(), 1, out.size(), f);
    const bool closeOk = std::fclose(f) == 0;
    if (got != out.size() || !closeOk) {
        out.clear();
        return false;
    }
    return true;
}

bool extractJsonStringAfter(const std::string& text, size_t keyPos,
                            std::string_view key, std::string& out) {
    const size_t colon = text.find(':', keyPos + key.size());
    if (colon == std::string::npos) return false;
    const size_t quote = text.find('"', colon + 1);
    if (quote == std::string::npos) return false;
    out.clear();
    bool escape = false;
    for (size_t i = quote + 1; i < text.size(); ++i) {
        const char c = text[i];
        if (escape) {
            if (c == '"' || c == '\\' || c == '/') out.push_back(c);
            else if (c == 'n') out.push_back('\n');
            else if (c == 't') out.push_back('\t');
            else out.push_back(c);
            escape = false;
            continue;
        }
        if (c == '\\') { escape = true; continue; }
        if (c == '"') return true;
        out.push_back(c);
    }
    out.clear();
    return false;
}

std::string hexEncode(std::string_view value) {
    static constexpr char digits[] = "0123456789abcdef";
    std::string out;
    out.reserve(value.size() * 2);
    for (unsigned char c : value) {
        out.push_back(digits[c >> 4]);
        out.push_back(digits[c & 0x0f]);
    }
    return out;
}

bool hexDecode(std::string_view value, std::string& out) {
    if ((value.size() & 1u) != 0) return false;
    auto nibble = [](char c) -> int {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        return -1;
    };
    out.clear();
    out.reserve(value.size() / 2);
    for (size_t i = 0; i < value.size(); i += 2) {
        const int hi = nibble(value[i]);
        const int lo = nibble(value[i + 1]);
        if (hi < 0 || lo < 0) { out.clear(); return false; }
        out.push_back(static_cast<char>((hi << 4) | lo));
    }
    return out.find('\0') == std::string::npos;
}

bool parseBindings(std::string_view text, BindingMap& bindings) {
    bindings.clear();
    if (text.substr(0, kBindingHeader.size()) != kBindingHeader) return false;
    size_t start = kBindingHeader.size();
    while (start < text.size()) {
        const size_t end = text.find('\n', start);
        const std::string_view row = text.substr(
            start, (end == std::string_view::npos ? text.size() : end) - start);
        if (!row.empty()) {
            std::array<std::string, 5> fields{};
            size_t pos = 0;
            for (size_t field = 0; field < fields.size(); ++field) {
                const size_t tab = row.find('\t', pos);
                if (field + 1 < fields.size()) {
                    if (tab == std::string_view::npos) return false;
                    if (!hexDecode(row.substr(pos, tab - pos), fields[field])) return false;
                    pos = tab + 1;
                } else {
                    // The fifth field must consume the rest of the row. A sixth tab/field is
                    // malformed rather than something we silently ignore.
                    if (tab != std::string_view::npos) return false;
                    if (!hexDecode(row.substr(pos), fields[field])) return false;
                }
            }
            if (fields[0].empty() || fields[1].empty() || fields[3].empty()) return false;
            StoredLaunchBinding binding;
            binding.providerId = std::move(fields[1]);
            binding.launcherPath = std::move(fields[2]);
            binding.contentPath = std::move(fields[3]);
            binding.corePath = std::move(fields[4]);
            if (!bindings.emplace(std::move(fields[0]), std::move(binding)).second) return false;
        }
        if (end == std::string_view::npos) break;
        start = end + 1;
    }
    return true;
}

std::string serializeBindings(const BindingMap& bindings) {
    std::string out(kBindingHeader);
    for (const auto& [key, binding] : bindings) {
        out += hexEncode(key) + "\t" + hexEncode(binding.providerId) + "\t" +
               hexEncode(binding.launcherPath) + "\t" + hexEncode(binding.contentPath) + "\t" +
               hexEncode(binding.corePath) + "\n";
    }
    return out;
}

bool loadBindings(BindingMap& bindings) {
    const std::string path = PokeBank::Paths::gameLaunchBindingsFile();
    std::string text;
    if (readTextFile(path, text, 512 * 1024) && parseBindings(text, bindings)) return true;
    text.clear();
    if (readTextFile(path + ".bak", text, 512 * 1024) && parseBindings(text, bindings)) return true;
    bindings.clear();
    return false;
}

bool writeBindings(const BindingMap& bindings, std::string& error) {
    error.clear();
    std::string dirError;
    if (!PokeBank::Paths::ensureConfigRoot(&dirError)) {
        error = dirError.empty() ? "Could not create the PokeBank NX config directory." : dirError;
        return false;
    }

    const std::string target = PokeBank::Paths::gameLaunchBindingsFile();
    const std::string temp = target + ".tmp";
    const std::string backup = target + ".bak";
    const std::string payload = serializeBindings(bindings);

    FILE* f = std::fopen(temp.c_str(), "wb");
    if (!f) { error = "Could not create temporary launch bindings."; return false; }
    bool ok = std::fwrite(payload.data(), 1, payload.size(), f) == payload.size();
    if (ok) ok = std::fflush(f) == 0;
    if (ok) ok = ::fsync(::fileno(f)) == 0;
    if (std::fclose(f) != 0) ok = false;
    if (!ok) {
        error = "Could not safely write launch bindings.";
        std::remove(temp.c_str());
        return false;
    }

    BindingMap checked;
    std::string checkedBytes;
    if (!readTextFile(temp, checkedBytes, 512 * 1024) ||
        checkedBytes != payload || !parseBindings(checkedBytes, checked)) {
        error = "Launch binding verification failed.";
        std::remove(temp.c_str());
        return false;
    }

    struct stat st{};
    const bool hadTarget = ::stat(target.c_str(), &st) == 0;
    std::remove(backup.c_str());
    if (hadTarget && std::rename(target.c_str(), backup.c_str()) != 0) {
        error = "Could not preserve the previous launch bindings.";
        std::remove(temp.c_str());
        return false;
    }
    if (std::rename(temp.c_str(), target.c_str()) != 0) {
        if (hadTarget) std::rename(backup.c_str(), target.c_str());
        error = "Could not install the new launch bindings.";
        return false;
    }
    return true;
}

std::string firstExisting(std::initializer_list<const char*> paths) {
    for (const char* path : paths)
        if (regularFile(path)) return path;
    return paths.size() ? *paths.begin() : std::string{};
}

std::string defaultLauncherPath(GameLaunchProviderKind kind,
                                std::string_view gameId,
                                std::string_view sourcePath) {
    (void)sourcePath;
    switch (kind) {
        case GameLaunchProviderKind::RetroArch:
            return firstExisting({"sdmc:/retroarch/retroarch_switch.nro",
                                  "sdmc:/switch/retroarch_switch.nro"});
        case GameLaunchProviderKind::MGBA:
            return firstExisting({"sdmc:/switch/mgba.nro",
                                  "sdmc:/switch/mgba/mgba.nro"});
        case GameLaunchProviderKind::DraStic:
            return firstExisting({"sdmc:/switch/DrasticDS.nro",
                                  "sdmc:/switch/drastic/DrasticDS.nro"});
        case GameLaunchProviderKind::MelonDS:
            return firstExisting({"sdmc:/switch/melonds/melonDS.nro",
                                  "sdmc:/switch/melonDS.nro"});
        case GameLaunchProviderKind::Tico:
            if (gameId.size() >= 4 && gameId.substr(gameId.size() - 4) == "_gba")
                return "sdmc:/tico/cores/tico-mgba.nro";
            if ((gameId.size() >= 3 && gameId.substr(gameId.size() - 3) == "_gb") ||
                (gameId.size() >= 4 && gameId.substr(gameId.size() - 4) == "_gbc"))
                return "sdmc:/tico/cores/tico-gambatte.nro";
            return {};
        case GameLaunchProviderKind::Unknown:
            return {};
    }
    return {};
}

std::string defaultRetroArchCore(std::string_view gameId) {
    if (gameId.size() >= 4 && gameId.substr(gameId.size() - 4) == "_gba")
        return firstExisting({"sdmc:/retroarch/cores/mgba_libretro_libnx.nro"});
    if ((gameId.size() >= 3 && gameId.substr(gameId.size() - 3) == "_gb") ||
        (gameId.size() >= 4 && gameId.substr(gameId.size() - 4) == "_gbc"))
        return firstExisting({"sdmc:/retroarch/cores/gambatte_libretro_libnx.nro",
                              "sdmc:/retroarch/cores/mgba_libretro_libnx.nro"});
    return {};
}

bool loadStoredBinding(std::string_view key, std::string_view providerId,
                       StoredLaunchBinding& binding) {
    if (key.empty()) return false;
    BindingMap bindings;
    loadBindings(bindings);
    const auto found = bindings.find(std::string(key));
    if (found == bindings.end()) return false;
    if (compactLaunchProvider(found->second.providerId) != compactLaunchProvider(providerId))
        return false;
    binding = found->second;
    return true;
}

void collectContentMatches(const std::string& root, std::string_view wantedStem,
                           std::string_view gameId, size_t depth, size_t maxDepth,
                           size_t& examined, size_t maxFiles, std::set<std::string>& visited,
                           std::vector<std::string>& matches) {
    if (root.empty() || depth > maxDepth || examined >= maxFiles || !directory(root)) return;
    struct stat rootInfo{};
    if (::stat(root.c_str(), &rootInfo) != 0) return;
    const std::string key = rootInfo.st_ino
        ? std::to_string(static_cast<unsigned long long>(rootInfo.st_dev)) + ":" +
          std::to_string(static_cast<unsigned long long>(rootInfo.st_ino))
        : root;
    if (!visited.insert(key).second) return;

    DIR* dir = ::opendir(root.c_str());
    if (!dir) return;
    std::vector<std::string> names;
    while (const dirent* entry = ::readdir(dir)) {
        const std::string name(entry->d_name);
        if (name != "." && name != "..") names.push_back(name);
    }
    ::closedir(dir);
    std::sort(names.begin(), names.end());

    for (const auto& name : names) {
        if (examined >= maxFiles) break;
        const std::string path = root.back() == '/' ? root + name : root + "/" + name;
        struct stat st{};
        if (::stat(path.c_str(), &st) != 0) continue;
        if (S_ISDIR(st.st_mode)) {
            if (depth < maxDepth)
                collectContentMatches(path, wantedStem, gameId, depth + 1, maxDepth,
                                      examined, maxFiles, visited, matches);
            continue;
        }
        if (!S_ISREG(st.st_mode) || !gameLaunchContentSupported(gameId, path)) continue;
        ++examined;
        if (normalizedLaunchStem(path) == wantedStem)
            matches.push_back(path);
    }
}

std::vector<std::string> findContentMatches(std::initializer_list<std::string> roots,
                                            std::string_view wantedStem,
                                            std::string_view gameId,
                                            size_t maxDepth = 3,
                                            size_t maxFiles = 512) {
    std::vector<std::string> matches;
    std::set<std::string> visited;
    size_t examined = 0;
    for (const auto& root : roots) {
        collectContentMatches(root, wantedStem, gameId, 0, maxDepth,
                              examined, maxFiles, visited, matches);
        if (examined >= maxFiles) break;
    }
    std::sort(matches.begin(), matches.end());
    matches.erase(std::unique(matches.begin(), matches.end()), matches.end());
    return matches;
}

GameLaunchDescriptor directDescriptor(GameLaunchProviderKind kind,
                                      std::string_view gameId,
                                      std::string_view providerId,
                                      std::string launcher,
                                      std::string content,
                                      std::string detail) {
    GameLaunchDescriptor result;
    result.backend = kind == GameLaunchProviderKind::RetroArch
        ? GameLaunchBackend::RetroArch : GameLaunchBackend::HomebrewNro;
    result.providerId = std::string(providerId);
    result.launcherPath = std::move(launcher);
    result.contentPath = std::move(content);
    if (kind == GameLaunchProviderKind::RetroArch)
        result.corePath = defaultRetroArchCore(gameId);

    if (result.launcherPath.empty() || !regularFile(result.launcherPath)) {
        result.state = GameLaunchState::LauncherMissing;
        result.detail = "The emulator/launcher NRO is not installed at a known path.";
        return result;
    }
    if (!regularFile(result.contentPath)) {
        result.state = GameLaunchState::NeedsContentLink;
        result.detail = "The linked game file is missing; link it again.";
        return result;
    }
    if (kind == GameLaunchProviderKind::RetroArch &&
        (result.corePath.empty() || !regularFile(result.corePath))) {
        result.state = GameLaunchState::LauncherMissing;
        result.detail = "A compatible RetroArch core is not installed.";
        return result;
    }
    result.state = GameLaunchState::Ready;
    result.detail = std::move(detail);
    return result;
}

GameLaunchDescriptor descriptorFromStored(std::string_view gameId,
                                          std::string_view providerId,
                                          const StoredLaunchBinding& stored) {
    const GameLaunchProviderKind kind = gameLaunchProviderKind(providerId);
    GameLaunchDescriptor result;
    result.backend = kind == GameLaunchProviderKind::RetroArch
        ? GameLaunchBackend::RetroArch : GameLaunchBackend::HomebrewNro;
    result.providerId = std::string(providerId);

    // A launch binding may remember CONTENT, but it never gets to choose executable code.
    // Recompute the NRO/core from the provider adapter every time so a hand-edited/stale config
    // cannot redirect PokeBank NX to an arbitrary executable path.
    result.launcherPath = defaultLauncherPath(kind, gameId, stored.contentPath);
    result.contentPath = stored.contentPath;
    if (kind == GameLaunchProviderKind::RetroArch)
        result.corePath = defaultRetroArchCore(gameId);

    if (result.launcherPath.empty() || !regularFile(result.launcherPath)) {
        result.state = GameLaunchState::LauncherMissing;
        result.detail = "The linked emulator/launcher NRO is missing.";
    } else if (!regularFile(result.contentPath)) {
        result.state = GameLaunchState::NeedsContentLink;
        result.detail = "The linked game file is missing; link it again.";
    } else if (kind == GameLaunchProviderKind::RetroArch &&
               (result.corePath.empty() || !regularFile(result.corePath))) {
        result.state = GameLaunchState::LauncherMissing;
        result.detail = "The linked RetroArch core is missing.";
    } else {
        result.state = GameLaunchState::Ready;
        result.detail = "Linked game file is ready to launch.";
    }
    return result;
}

GameLaunchDescriptor resolveRetroArch(std::string_view gameId,
                                      std::string_view sourcePath,
                                      std::string_view bindingKey) {
    GameLaunchDescriptor result;
    result.backend = GameLaunchBackend::RetroArch;
    result.providerId = "retroarch";
    result.launcherPath = defaultLauncherPath(
        GameLaunchProviderKind::RetroArch, gameId, sourcePath);

    if (result.launcherPath.empty() || !regularFile(result.launcherPath)) {
        result.state = GameLaunchState::LauncherMissing;
        result.detail = "RetroArch is not installed at a known path.";
        return result;
    }

    StoredLaunchBinding stored;
    if (loadStoredBinding(bindingKey, "retroarch", stored))
        return descriptorFromStored(gameId, "retroarch", stored);

    const std::string wanted = normalizedLaunchStem(sourcePath);
    if (wanted.empty()) {
        result.state = GameLaunchState::NeedsContentLink;
        result.detail = "This save has no stable content basename to match.";
        return result;
    }

    DIR* dir = ::opendir("sdmc:/retroarch/playlists");
    if (dir) {
        bool sawContent = false;
        size_t filesRead = 0;
        while (const dirent* entry = ::readdir(dir)) {
            if (++filesRead > 128) break;
            const std::string name(entry->d_name);
            if (name == "." || name == ".." || !hasExtension(name, ".lpl")) continue;
            const std::string playlist = "sdmc:/retroarch/playlists/" + name;
            std::string text;
            if (!readTextFile(playlist, text)) continue;

            size_t pos = 0;
            while ((pos = text.find("\"path\"", pos)) != std::string::npos) {
                std::string content;
                if (!extractJsonStringAfter(text, pos, "\"path\"", content)) { pos += 6; continue; }
                const size_t nextPath = text.find("\"path\"", pos + 6);
                if (normalizedLaunchStem(content) != wanted) { pos += 6; continue; }

                content = switchPath(content);
                if (!regularFile(content)) { pos += 6; continue; }
                sawContent = true;

                const size_t corePos = text.find("\"core_path\"", pos + 6);
                if (corePos == std::string::npos ||
                    (nextPath != std::string::npos && corePos > nextPath)) {
                    pos += 6;
                    continue;
                }
                std::string core;
                if (!extractJsonStringAfter(text, corePos, "\"core_path\"", core)) {
                    pos += 6;
                    continue;
                }
                core = switchPath(core);
                if (core == "DETECT" || core.empty() || !regularFile(core)) {
                    pos += 6;
                    continue;
                }

                result.contentPath = std::move(content);
                result.corePath = std::move(core);
                result.state = GameLaunchState::Ready;
                result.detail = "RetroArch playlist matched this save to its game file and core.";
                ::closedir(dir);
                return result;
            }
        }
        ::closedir(dir);
        result.detail = sawContent
            ? "The matching playlist has no usable core; link the game file once."
            : "No RetroArch playlist matches this save; link the game file once.";
    } else {
        result.detail = "No RetroArch playlist could be read; link the game file once.";
    }

    result.state = GameLaunchState::NeedsContentLink;
    return result;
}

GameLaunchDescriptor resolveKnownHomebrew(GameLaunchProviderKind kind,
                                          std::string_view gameId,
                                          std::string_view providerId,
                                          std::string_view sourcePath,
                                          std::string_view bindingKey) {
    StoredLaunchBinding stored;
    if (loadStoredBinding(bindingKey, providerId, stored))
        return descriptorFromStored(gameId, providerId, stored);

    const std::string launcher = defaultLauncherPath(kind, gameId, sourcePath);
    if (launcher.empty() || !regularFile(launcher)) {
        GameLaunchDescriptor result;
        result.backend = GameLaunchBackend::HomebrewNro;
        result.providerId = std::string(providerId);
        result.launcherPath = launcher;
        result.state = GameLaunchState::LauncherMissing;
        result.detail = "The emulator/launcher NRO is not installed at a known path.";
        return result;
    }

    const std::string wanted = normalizedLaunchStem(sourcePath);
    std::vector<std::string> matches;
    if (!wanted.empty()) {
        if (kind == GameLaunchProviderKind::Tico) {
            std::string root;
            if (gameId.size() >= 4 && gameId.substr(gameId.size() - 4) == "_gba")
                root = "sdmc:/tico/roms/gba";
            else if (gameId.size() >= 4 && gameId.substr(gameId.size() - 4) == "_gbc")
                root = "sdmc:/tico/roms/gbc";
            else if (gameId.size() >= 3 && gameId.substr(gameId.size() - 3) == "_gb")
                root = "sdmc:/tico/roms/gb";
            if (!root.empty())
                matches = findContentMatches({root}, wanted, gameId, 4, 512);
        } else if (kind == GameLaunchProviderKind::DraStic) {
            matches = findContentMatches({"sdmc:/switch/drastic/games"},
                                         wanted, gameId, 4, 512);
        } else if (kind == GameLaunchProviderKind::MelonDS) {
            matches = findContentMatches({"sdmc:/switch/melonds", "sdmc:/melonds"},
                                         wanted, gameId, 3, 512);
        } else if (kind == GameLaunchProviderKind::MGBA) {
            const std::string saveDir = parentPath(std::string(sourcePath));
            const std::string nearby = parentPath(saveDir);
            matches = findContentMatches({saveDir, nearby}, wanted, gameId, 2, 384);
        }
    }

    if (matches.size() == 1)
        return directDescriptor(kind, gameId, providerId, launcher, matches.front(),
                                "Matched the validated save to its game file.");

    GameLaunchDescriptor result;
    result.backend = GameLaunchBackend::HomebrewNro;
    result.providerId = std::string(providerId);
    result.launcherPath = launcher;
    result.state = GameLaunchState::NeedsContentLink;
    result.detail = matches.size() > 1
        ? "More than one matching game file was found; choose the exact one."
        : "This save is valid, but its game file is not linked yet.";
    return result;
}

std::string quoted(std::string_view value) {
    std::string out = "\"";
    for (char c : value) {
        if (c == '"' || c == '\\') out.push_back('\\');
        out.push_back(c);
    }
    out.push_back('"');
    return out;
}

} // namespace

GameLaunchDescriptor resolveGameLaunch(uint64_t titleId,
                                       std::string_view gameId,
                                       std::string_view providerId,
                                       std::string_view sourcePath,
                                       std::string_view bindingKey) {
    if (titleId != 0) {
        GameLaunchDescriptor result;
        result.backend = GameLaunchBackend::SwitchTitle;
        result.titleId = titleId;
        result.providerId = "switch";

#ifdef __SWITCH__
        auto* ctl = static_cast<NsApplicationControlData*>(
            std::malloc(sizeof(NsApplicationControlData)));
        if (!ctl) {
            result.state = GameLaunchState::Unavailable;
            result.detail = "Could not check whether the game is currently launchable.";
            return result;
        }
        u64 outSize = 0;
        const Result rc = nsGetApplicationControlData(
            NsApplicationControlSource_Storage, titleId, ctl,
            sizeof(NsApplicationControlData), &outSize);
        std::free(ctl);
        if (R_FAILED(rc) || outSize == 0) {
            result.state = GameLaunchState::Unavailable;
            result.detail = "The save exists, but the game is not currently installed or inserted.";
            return result;
        }
#endif
        result.state = GameLaunchState::Ready;
        result.detail = "Launch installed Nintendo Switch title.";
        return result;
    }

    const GameLaunchProviderKind kind = gameLaunchProviderKind(providerId);
    if (kind == GameLaunchProviderKind::RetroArch)
        return resolveRetroArch(gameId, sourcePath, bindingKey);
    if (kind == GameLaunchProviderKind::MGBA ||
        kind == GameLaunchProviderKind::Tico ||
        kind == GameLaunchProviderKind::DraStic ||
        kind == GameLaunchProviderKind::MelonDS)
        return resolveKnownHomebrew(kind, gameId, providerId, sourcePath, bindingKey);

    GameLaunchDescriptor result;
    result.backend = GameLaunchBackend::HomebrewNro;
    result.providerId = std::string(providerId);
    result.state = GameLaunchState::Unavailable;
    result.detail = providerId.empty()
        ? "Choose a validated save source before launching."
        : "This source provider does not have a launch adapter yet.";
    return result;
}

bool saveGameLaunchBinding(std::string_view bindingKey,
                           std::string_view gameId,
                           std::string_view providerId,
                           std::string_view contentPath,
                           std::string& error) {
    error.clear();
    if (bindingKey.empty()) {
        error = "No exact source identity is available for this launch link.";
        return false;
    }
    const GameLaunchProviderKind kind = gameLaunchProviderKind(providerId);
    if (kind == GameLaunchProviderKind::Unknown) {
        error = "This source provider does not have a launch adapter.";
        return false;
    }

    const std::string content = switchPath(std::string(contentPath));
    if (!regularFile(content)) {
        error = "That game file does not exist.";
        return false;
    }
    if (!gameLaunchContentSupported(gameId, content)) {
        error = "That file type does not match this Pokémon game.";
        return false;
    }

    StoredLaunchBinding binding;
    binding.providerId = compactLaunchProvider(providerId);
    binding.launcherPath = defaultLauncherPath(kind, gameId, content);
    binding.contentPath = content;
    if (kind == GameLaunchProviderKind::RetroArch) {
        binding.corePath = defaultRetroArchCore(gameId);
        if (binding.corePath.empty() || !regularFile(binding.corePath)) {
            error = "No compatible RetroArch core is installed for this game.";
            return false;
        }
    }

    BindingMap bindings;
    loadBindings(bindings);
    bindings[std::string(bindingKey)] = std::move(binding);
    return writeBindings(bindings, error);
}

bool forgetGameLaunchBinding(std::string_view bindingKey, std::string& error) {
    error.clear();
    if (bindingKey.empty()) return true;
    BindingMap bindings;
    if (!loadBindings(bindings)) return true;
    if (bindings.erase(std::string(bindingKey)) == 0) return true;
    return writeBindings(bindings, error);
}

std::string suggestedGameLaunchBrowseRoot(std::string_view gameId,
                                          std::string_view providerId,
                                          std::string_view sourcePath) {
    const GameLaunchProviderKind kind = gameLaunchProviderKind(providerId);
    std::string candidate;
    switch (kind) {
        case GameLaunchProviderKind::Tico:
            if (gameId.size() >= 4 && gameId.substr(gameId.size() - 4) == "_gba")
                candidate = "sdmc:/tico/roms/gba";
            else if (gameId.size() >= 4 && gameId.substr(gameId.size() - 4) == "_gbc")
                candidate = "sdmc:/tico/roms/gbc";
            else if (gameId.size() >= 3 && gameId.substr(gameId.size() - 3) == "_gb")
                candidate = "sdmc:/tico/roms/gb";
            break;
        case GameLaunchProviderKind::DraStic:
            candidate = "sdmc:/switch/drastic/games";
            break;
        case GameLaunchProviderKind::MelonDS:
            candidate = "sdmc:/switch/melonds";
            break;
        case GameLaunchProviderKind::RetroArch:
            candidate = "sdmc:/retroarch/downloads";
            break;
        case GameLaunchProviderKind::MGBA: {
            const std::string saveDir = parentPath(std::string(sourcePath));
            candidate = parentPath(saveDir);
            break;
        }
        case GameLaunchProviderKind::Unknown:
            break;
    }
    if (!candidate.empty() && directory(candidate)) return candidate;
    const std::string sourceDir = parentPath(std::string(sourcePath));
    if (!sourceDir.empty() && directory(sourceDir)) return sourceDir;
    return "sdmc:/";
}

bool requestGameLaunch(const GameLaunchDescriptor& descriptor, std::string& error) {
    error.clear();
    if (!descriptor.ready()) {
        error = descriptor.detail.empty() ? "This game is not ready to launch." : descriptor.detail;
        return false;
    }

#ifdef __SWITCH__
    if (descriptor.backend == GameLaunchBackend::SwitchTitle) {
        const Result rc = appletRequestLaunchApplication(descriptor.titleId, nullptr);
        if (R_FAILED(rc)) {
            char buf[96];
            std::snprintf(buf, sizeof(buf), "Switch launch request failed (0x%08X).", static_cast<unsigned>(rc));
            error = buf;
            return false;
        }
        return true;
    }

    if (descriptor.backend == GameLaunchBackend::RetroArch ||
        descriptor.backend == GameLaunchBackend::HomebrewNro) {
        if (!envHasNextLoad()) {
            error = "The current homebrew loader does not support chaining to another NRO.";
            return false;
        }
        if (!regularFile(descriptor.launcherPath)) {
            error = "The configured emulator/launcher NRO is missing.";
            return false;
        }

        std::string argv = quoted(descriptor.launcherPath);
        if (!descriptor.corePath.empty())
            argv += " -L " + quoted(descriptor.corePath);
        if (!descriptor.contentPath.empty())
            argv += " " + quoted(descriptor.contentPath);

        const Result rc = envSetNextLoad(descriptor.launcherPath.c_str(), argv.c_str());
        if (R_FAILED(rc)) {
            char buf[96];
            std::snprintf(buf, sizeof(buf), "Homebrew launch request failed (0x%08X).", static_cast<unsigned>(rc));
            error = buf;
            return false;
        }
        return true;
    }
#else
    (void)descriptor;
#endif

    error = "Launch is unavailable on this build.";
    return false;
}

} // namespace UI
