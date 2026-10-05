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
#include "Games/GameIdentity.h"

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


#ifdef __SWITCH__
std::string compactGameTitle(std::string_view value) {
    std::string out;
    out.reserve(value.size());
    for (size_t i = 0; i < value.size(); ++i) {
        const unsigned char c = static_cast<unsigned char>(value[i]);
        // HOME forwarders commonly use the official “Pokémon” spelling. Normalize UTF-8 é/É
        // instead of dropping it and turning the identity word into “pokmon”.
        if (c == 0xC3 && i + 1 < value.size()) {
            const unsigned char next = static_cast<unsigned char>(value[i + 1]);
            if (next == 0xA9 || next == 0x89) {
                out.push_back('e');
                ++i;
                continue;
            }
        }
        if (c < 0x80 && std::isalnum(c))
            out.push_back(static_cast<char>(std::tolower(c)));
    }
    return out;
}

bool installedForwarderNameMatches(std::string_view gameId, std::string_view normalizedName) {
    auto shaped = [&](std::string_view token) {
        const std::string plain(token);
        const std::string version = plain + "version";
        const std::string pokemon = "pokemon" + plain;
        const std::string pokemonVersion = pokemon + "version";
        return normalizedName == plain || normalizedName == version ||
               normalizedName == pokemon || normalizedName == pokemonVersion ||
               normalizedName.rfind(pokemon, 0) == 0;
    };

    // Release-specific matching avoids collisions such as Red/FireRed, Gold/HeartGold and
    // Diamond/Brilliant Diamond while accepting both plain and official Pokémon forwarder names.
    if (gameId == "red_gb") return shaped("red");
    if (gameId == "blue_gb") return shaped("blue");
    if (gameId == "yellow_gb") return shaped("yellow");
    if (gameId == "gold_gbc") return shaped("gold");
    if (gameId == "silver_gbc") return shaped("silver");
    if (gameId == "crystal_gbc") return shaped("crystal");
    if (gameId == "ruby_gba") return shaped("ruby");
    if (gameId == "sapphire_gba") return shaped("sapphire");
    if (gameId == "firered_gba") return shaped("firered");
    if (gameId == "leafgreen_gba") return shaped("leafgreen");
    if (gameId == "emerald_gba") return shaped("emerald");
    if (gameId == "diamond_nds") return shaped("diamond");
    if (gameId == "pearl_nds") return shaped("pearl");
    if (gameId == "platinum_nds") return shaped("platinum");
    if (gameId == "heartgold_nds") return shaped("heartgold");
    if (gameId == "soulsilver_nds") return shaped("soulsilver");
    return false;
}

struct InstalledApplicationName {
    uint64_t titleId = 0;
    std::string normalizedName;
};

const std::vector<InstalledApplicationName>& installedApplicationNames() {
    // HOME application metadata is console-global for the lifetime of this process. Build it
    // once, then let every game identity match in memory instead of re-querying the same control
    // records for FireRed, Emerald, Platinum and every other launch candidate.
    static const std::vector<InstalledApplicationName> applications = [] {
        std::vector<InstalledApplicationName> found;
        s32 offset = 0;
        while (offset < 2048) {
            NsApplicationRecord records[32]{};
            s32 count = 0;
            if (R_FAILED(nsListApplicationRecord(records, 32, offset, &count)) || count <= 0)
                break;
            for (s32 i = 0; i < count; ++i) {
                auto* control = static_cast<NsApplicationControlData*>(
                    std::malloc(sizeof(NsApplicationControlData)));
                if (!control) continue;
                u64 outSize = 0;
                const Result rc = nsGetApplicationControlData(
                    NsApplicationControlSource_Storage, records[i].application_id,
                    control, sizeof(NsApplicationControlData), &outSize);
                if (R_SUCCEEDED(rc) && outSize != 0) {
                    NacpLanguageEntry* language = nullptr;
                    if (R_SUCCEEDED(nacpGetLanguageEntry(&control->nacp, &language)) && language) {
                        const std::string name = compactGameTitle(language->name);
                        if (!name.empty()) found.push_back({records[i].application_id, name});
                    }
                }
                std::free(control);
            }
            if (count < 32) break;
            offset += count;
        }
        return found;
    }();
    return applications;
}

uint64_t installedGameForwarderTitle(std::string_view gameId) {
    static std::map<std::string, uint64_t> cache;
    const std::string key(gameId);
    if (const auto found = cache.find(key); found != cache.end()) return found->second;

    if (!PokeVault::Games::findGame(gameId)) { cache[key] = 0; return 0; }

    uint64_t unique = 0;
    int matches = 0;
    for (const auto& application : installedApplicationNames()) {
        if (!installedForwarderNameMatches(gameId, application.normalizedName)) continue;
        unique = application.titleId;
        if (++matches > 1) break;
    }
    if (matches != 1) unique = 0;
    cache[key] = unique;
    return unique;
}
#endif

std::vector<std::string> configuredDraSticLibraryRoots() {
    std::vector<std::string> roots{
        "sdmc:/switch/drastic/games",
        "sdmc:/switch/drastic/roms",
        "sdmc:/roms/nds",
        "sdmc:/roms/NDS"
    };
    std::string ini;
    if (readTextFile("sdmc:/switch/drastic/launcher.ini", ini, 512 * 1024)) {
        size_t start = 0;
        while (start < ini.size()) {
            const size_t end = ini.find('\n', start);
            std::string line = ini.substr(start,
                (end == std::string::npos ? ini.size() : end) - start);
            while (!line.empty() && (line.back() == '\r' || line.back() == ' ' || line.back() == '\t'))
                line.pop_back();
            const size_t eq = line.find('=');
            if (eq != std::string::npos) {
                std::string value = line.substr(eq + 1);
                const size_t first = value.find_first_not_of(" \t\"");
                if (first != std::string::npos) value.erase(0, first); else value.clear();
                while (!value.empty() &&
                       (value.back() == ' ' || value.back() == '\t' || value.back() == '\"'))
                    value.pop_back();
                value = switchPath(value);
                if (value.rfind("sdmc:/", 0) == 0 && directory(value))
                    roots.push_back(std::move(value));
            }
            if (end == std::string::npos) break;
            start = end + 1;
        }
    }
    std::sort(roots.begin(), roots.end());
    roots.erase(std::unique(roots.begin(), roots.end()), roots.end());
    return roots;
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
                                  "sdmc:/switch/drastic/DrasticDS.nro",
                                  "sdmc:/switch/DrasticDS_nx.nro",
                                  "sdmc:/switch/drastic/DrasticDS_nx.nro",
                                  "sdmc:/switch/drastic/drastic.nro"});
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
    if (gameId.size() >= 4 && gameId.substr(gameId.size() - 4) == "_nds")
        return firstExisting({"sdmc:/retroarch/cores/desmume_libretro_libnx.nro",
                              "sdmc:/retroarch/cores/melonds_libretro_libnx.nro"});
    return {};
}

GameLaunchProviderKind providerKindForSourcePath(std::string_view providerId,
                                                std::string_view sourcePath) {
    GameLaunchProviderKind kind = gameLaunchProviderKind(providerId);
    if (kind != GameLaunchProviderKind::Unknown) return kind;

    std::string path(sourcePath);
    std::transform(path.begin(), path.end(), path.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    if (path.find("/switch/drastic/") != std::string::npos)
        return GameLaunchProviderKind::DraStic;
    if (path.find("/switch/melonds/") != std::string::npos ||
        path.find("/melonds/") != std::string::npos)
        return GameLaunchProviderKind::MelonDS;
    if (path.find("/retroarch/") != std::string::npos)
        return GameLaunchProviderKind::RetroArch;
    if (path.find("/mgba/") != std::string::npos)
        return GameLaunchProviderKind::MGBA;
    if (path.find("/tico/") != std::string::npos)
        return GameLaunchProviderKind::Tico;
    return GameLaunchProviderKind::Unknown;
}

std::string providerIdForKind(GameLaunchProviderKind kind, std::string_view fallback) {
    switch (kind) {
        case GameLaunchProviderKind::RetroArch: return "retroarch";
        case GameLaunchProviderKind::MGBA:      return "mgba";
        case GameLaunchProviderKind::Tico:      return "tico";
        case GameLaunchProviderKind::DraStic:   return "drastic";
        case GameLaunchProviderKind::MelonDS:   return "melonds";
        case GameLaunchProviderKind::Unknown:   return std::string(fallback);
    }
    return std::string(fallback);
}

bool loadStoredBinding(std::string_view key, std::string_view providerId,
                       StoredLaunchBinding& binding) {
    if (key.empty()) return false;
    BindingMap bindings;
    loadBindings(bindings);

    const std::string expectedProvider = compactLaunchProvider(providerId);
    const auto found = bindings.find(std::string(key));
    if (found != bindings.end()) {
        if (compactLaunchProvider(found->second.providerId) != expectedProvider)
            return false;
        binding = found->second;
        return true;
    }

    // Source identities have been tightened over time. Preserve a user's already-linked ROM when
    // the profile + game are unchanged and exactly one valid binding from that family/provider
    // remains. Ambiguity still fails closed and returns to the explicit link flow.
    const size_t sourceSeparator = key.rfind('|');
    if (sourceSeparator == std::string_view::npos) return false;
    const std::string familyPrefix(key.substr(0, sourceSeparator + 1));

    const StoredLaunchBinding* unique = nullptr;
    for (const auto& [storedKey, stored] : bindings) {
        if (storedKey.rfind(familyPrefix, 0) != 0) continue;
        if (compactLaunchProvider(stored.providerId) != expectedProvider) continue;
        if (!regularFile(stored.contentPath)) continue;
        if (unique) return false;
        unique = &stored;
    }
    if (!unique) return false;
    binding = *unique;
    return true;
}

void collectContentMatches(const std::string& root, std::string_view sourcePath,
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
                collectContentMatches(path, sourcePath, gameId, depth + 1, maxDepth,
                                      examined, maxFiles, visited, matches);
            continue;
        }
        if (!S_ISREG(st.st_mode) || !gameLaunchContentSupported(gameId, path)) continue;
        ++examined;
        if (gameLaunchCandidateStemMatches(gameId, sourcePath, path))
            matches.push_back(path);
    }
}

std::vector<std::string> findContentMatches(std::initializer_list<std::string> roots,
                                            std::string_view sourcePath,
                                            std::string_view gameId,
                                            size_t maxDepth = 3,
                                            size_t maxFiles = 512) {
    std::vector<std::string> matches;
    std::set<std::string> visited;
    size_t examined = 0;
    for (const auto& root : roots) {
        collectContentMatches(root, sourcePath, gameId, 0, maxDepth,
                              examined, maxFiles, visited, matches);
        if (examined >= maxFiles) break;
    }
    std::sort(matches.begin(), matches.end());
    matches.erase(std::unique(matches.begin(), matches.end()), matches.end());
    return matches;
}


std::vector<std::string> findContentMatches(const std::vector<std::string>& roots,
                                            std::string_view sourcePath,
                                            std::string_view gameId,
                                            size_t maxDepth,
                                            size_t maxFiles) {
    std::vector<std::string> matches;
    std::set<std::string> visited;
    size_t examined = 0;
    for (const auto& root : roots) {
        collectContentMatches(root, sourcePath, gameId, 0, maxDepth,
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
    result.contentPath = std::move(content);
    if (kind == GameLaunchProviderKind::RetroArch) {
        result.corePath = defaultRetroArchCore(gameId);
        result.launcherPath = std::move(launcher);
    } else {
        result.launcherPath = std::move(launcher);
    }

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
    result.contentPath = stored.contentPath;
    if (kind == GameLaunchProviderKind::RetroArch) {
        result.corePath = defaultRetroArchCore(gameId);
        result.launcherPath = defaultLauncherPath(kind, gameId, stored.contentPath);
    } else {
        result.launcherPath = defaultLauncherPath(kind, gameId, stored.contentPath);
    }

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
    result.launcherPath = defaultLauncherPath(GameLaunchProviderKind::RetroArch, gameId, sourcePath);
    result.corePath = defaultRetroArchCore(gameId);

    if (result.launcherPath.empty() || !regularFile(result.launcherPath)) {
        result.state = GameLaunchState::LauncherMissing;
        result.detail = "The RetroArch frontend NRO is not installed at a known path.";
        return result;
    }
    if (result.corePath.empty() || !regularFile(result.corePath)) {
        result.state = GameLaunchState::LauncherMissing;
        result.detail = "A compatible RetroArch core NRO is not installed.";
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

    struct PlaylistMatch {
        std::string content;
        std::string core;

        bool operator<(const PlaylistMatch& other) const noexcept {
            return content < other.content || (content == other.content && core < other.core);
        }
        bool operator==(const PlaylistMatch& other) const noexcept {
            return content == other.content && core == other.core;
        }
    };
    std::vector<PlaylistMatch> matches;
    bool sawStemMatch = false;
    bool sawFamilyCompatibleContent = false;

    DIR* dir = ::opendir("sdmc:/retroarch/playlists");
    if (dir) {
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
                // Use the same bounded stem matcher as the filesystem adapters. This keeps exact
                // matches first-class but also handles common RetroArch naming differences such as
                // "Pokemon Red.srm" vs "Pokemon - Red Version (USA, Europe).gb". The resolver still
                // requires exactly one usable playlist entry, so Red cannot silently become FireRed.
                if (!gameLaunchCandidateStemMatches(gameId, sourcePath, content)) {
                    pos += 6;
                    continue;
                }
                sawStemMatch = true;

                content = switchPath(content);
                if (!gameLaunchContentSupported(gameId, content)) { pos += 6; continue; }
                if (!regularFile(content)) { pos += 6; continue; }
                sawFamilyCompatibleContent = true;

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
                if (core == "DETECT" || core.empty() || !regularFile(core))
                    core = defaultRetroArchCore(gameId);
                if (core.empty() || !regularFile(core)) {
                    pos += 6;
                    continue;
                }

                matches.push_back({std::move(content), std::move(core)});
                pos += 6;
            }
        }
        ::closedir(dir);

        std::sort(matches.begin(), matches.end());
        matches.erase(std::unique(matches.begin(), matches.end()), matches.end());
        if (matches.size() == 1) {
            result.contentPath = std::move(matches.front().content);
            result.corePath = std::move(matches.front().core);
            result.state = GameLaunchState::Ready;
            result.detail = "RetroArch playlist matched this save to its game file and core.";
            return result;
        }
        if (matches.size() > 1) {
            result.detail =
                "More than one matching RetroArch game/core entry was found; link the exact game file.";
        } else if (sawFamilyCompatibleContent) {
            result.detail = "The matching playlist has no usable core; link the game file once.";
        } else if (sawStemMatch) {
            result.detail = "Same-name playlist content does not match this game family; link the exact game file.";
        } else {
            result.detail = "No RetroArch playlist matches this save; link the game file once.";
        }
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
                matches = findContentMatches({root}, sourcePath, gameId, 4, 512);
        } else if (kind == GameLaunchProviderKind::DraStic) {
            const auto roots = configuredDraSticLibraryRoots();
            // Never synchronously crawl the whole SD card on the ZR input frame.
            matches = findContentMatches(roots, sourcePath, gameId, 1, 512);
        } else if (kind == GameLaunchProviderKind::MelonDS) {
            matches = findContentMatches({
                                             "sdmc:/switch/melonds",
                                             "sdmc:/melonds",
                                             "sdmc:/roms/nds",
                                             "sdmc:/roms/NDS"
                                         },
                                         sourcePath, gameId, 5, 1536);
        } else if (kind == GameLaunchProviderKind::MGBA) {
            const std::string saveDir = parentPath(std::string(sourcePath));
            const std::string nearby = parentPath(saveDir);
            matches = findContentMatches({saveDir, nearby}, sourcePath, gameId, 2, 384);
        }
    }

    if (matches.size() == 1)
        return directDescriptor(kind, gameId, providerId, launcher, matches.front(),
                                "Matched the validated save to its game file.");

    GameLaunchDescriptor result;
    result.backend = GameLaunchBackend::HomebrewNro;
    result.providerId = std::string(providerId);
    result.launcherPath = launcher;
    if (kind == GameLaunchProviderKind::DraStic && matches.empty()) {
        result.state = GameLaunchState::LauncherOnly;
        result.detail = "DraStic is ready; no unique ROM path was resolved, so open the emulator directly.";
        return result;
    }
    result.state = GameLaunchState::NeedsContentLink;
    result.detail = matches.size() > 1
        ? "More than one matching game file was found; choose the exact one."
        : "This save is valid, but its game file is not linked yet.";
    return result;
}

#ifdef __SWITCH__
std::string quoted(std::string_view value) {
    std::string out = "\"";
    for (char c : value) {
        if (c == '"' || c == '\\') out.push_back('\\');
        out.push_back(c);
    }
    out.push_back('"');
    return out;
}
#endif

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

    // A validated emulator/provider is already the strongest launch hint. Resolve it first so an
    // ordinary DraStic/melonDS launch never blocks the input frame on a console-wide HOME metadata
    // scan. Installed HOME forwarders remain a fallback only when there is no provider adapter.
    const GameLaunchProviderKind kind = providerKindForSourcePath(providerId, sourcePath);
    const std::string resolvedProvider = providerIdForKind(kind, providerId);
    if (kind == GameLaunchProviderKind::RetroArch)
        return resolveRetroArch(gameId, sourcePath, bindingKey);
    if (kind == GameLaunchProviderKind::MGBA ||
        kind == GameLaunchProviderKind::Tico ||
        kind == GameLaunchProviderKind::DraStic ||
        kind == GameLaunchProviderKind::MelonDS)
        return resolveKnownHomebrew(kind, gameId, resolvedProvider, sourcePath, bindingKey);

#ifdef __SWITCH__
    if (gameId.ends_with("_nds")) {
        if (const uint64_t forwarderTitle = installedGameForwarderTitle(gameId); forwarderTitle != 0) {
            GameLaunchDescriptor result;
            result.backend = GameLaunchBackend::SwitchTitle;
            result.titleId = forwarderTitle;
            result.providerId = std::string(providerId);
            result.state = GameLaunchState::Ready;
            result.detail = "Launch the installed HOME forwarder for this exact game.";
            return result;
        }
    }
#endif

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
    const GameLaunchProviderKind kind = providerKindForSourcePath(providerId, contentPath);
    const std::string resolvedProvider = providerIdForKind(kind, providerId);
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
    binding.providerId = compactLaunchProvider(resolvedProvider);
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
    const GameLaunchProviderKind kind = providerKindForSourcePath(providerId, sourcePath);
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

    if (descriptor.backend == GameLaunchBackend::RetroArch) {
        if (!envHasNextLoad()) {
            error = "The current homebrew loader does not support chaining to another NRO.";
            return false;
        }
        if (!regularFile(descriptor.launcherPath)) {
            error = "The RetroArch frontend NRO is missing.";
            return false;
        }
        if (descriptor.state != GameLaunchState::LauncherOnly &&
            (descriptor.corePath.empty() || !regularFile(descriptor.corePath))) {
            error = "The RetroArch core NRO for this game is missing.";
            return false;
        }
        if (descriptor.state != GameLaunchState::LauncherOnly &&
            !regularFile(descriptor.contentPath)) {
            error = "The linked game ROM is missing.";
            return false;
        }
        std::string argv = quoted(descriptor.launcherPath);
        if (descriptor.state != GameLaunchState::LauncherOnly && !descriptor.corePath.empty())
            argv += " -L " + quoted(descriptor.corePath);
        if (descriptor.state != GameLaunchState::LauncherOnly && !descriptor.contentPath.empty())
            argv += " " + quoted(descriptor.contentPath);
        const Result rc = envSetNextLoad(descriptor.launcherPath.c_str(), argv.c_str());
        if (R_FAILED(rc)) {
            char buf[96];
            std::snprintf(buf, sizeof(buf), "RetroArch game launch failed (0x%08X).", static_cast<unsigned>(rc));
            error = buf;
            return false;
        }
        return true;
    }

    if (descriptor.backend == GameLaunchBackend::HomebrewNro) {
        if (!envHasNextLoad()) {
            error = "The current homebrew loader does not support chaining to another NRO.";
            return false;
        }
        if (!regularFile(descriptor.launcherPath)) {
            error = "The configured emulator/launcher NRO is missing.";
            return false;
        }
        std::string argv = quoted(descriptor.launcherPath);
        if (descriptor.state != GameLaunchState::LauncherOnly &&
            !descriptor.contentPath.empty())
            argv += " " + quoted(descriptor.contentPath);
        const Result rc = envSetNextLoad(descriptor.launcherPath.c_str(), argv.c_str());
        if (R_FAILED(rc)) {
            char buf[96];
            std::snprintf(buf, sizeof(buf), "Homebrew game launch failed (0x%08X).", static_cast<unsigned>(rc));
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
