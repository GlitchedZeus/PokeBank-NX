#ifndef UI_GAME_LAUNCH_MODEL_H
#define UI_GAME_LAUNCH_MODEL_H

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <string>
#include <string_view>

namespace UI {

enum class GameLaunchBackend : uint8_t {
    None,
    SwitchTitle,
    RetroArch,
    HomebrewNro,
};

enum class GameLaunchState : uint8_t {
    Ready,
    LauncherOnly,
    ChooseSource,
    NeedsContentLink,
    LauncherMissing,
    Unavailable,
};

enum class GameLaunchProviderKind : uint8_t {
    Unknown,
    RetroArch,
    MGBA,
    Tico,
    DraStic,
    MelonDS,
};

inline std::string compactLaunchProvider(std::string_view providerId) {
    std::string compact;
    compact.reserve(providerId.size());
    for (unsigned char c : providerId)
        if (std::isalnum(c)) compact.push_back(static_cast<char>(std::tolower(c)));
    return compact;
}

enum class DraSticDirectLaunchSupport : uint8_t {
    Supported,
    Unsupported,
    Unknown,
};

// DrasticDS_nx gained the positional ROM/archive forwarder in 1.1.0. Older launchers accept the
// NRO handoff itself but ignore argv[1], which drops the user into DraStic's own file chooser and
// looks like PokeBank failed to launch the selected game. Keep the version rule independently
// testable; the Switch-only probe below reads the installed NRO's NACP display version.
inline bool drasticDirectLaunchVersionSupportsPositionalRom(std::string_view version) noexcept {
    size_t pos = 0;
    while (pos < version.size() && !std::isdigit(static_cast<unsigned char>(version[pos]))) ++pos;
    if (pos == version.size()) return false;

    unsigned major = 0;
    while (pos < version.size() && std::isdigit(static_cast<unsigned char>(version[pos]))) {
        major = major * 10u + static_cast<unsigned>(version[pos] - '0');
        ++pos;
    }
    if (pos == version.size() || version[pos] != '.') return false;
    ++pos;
    if (pos == version.size() || !std::isdigit(static_cast<unsigned char>(version[pos]))) return false;

    unsigned minor = 0;
    while (pos < version.size() && std::isdigit(static_cast<unsigned char>(version[pos]))) {
        minor = minor * 10u + static_cast<unsigned>(version[pos] - '0');
        ++pos;
    }
    return major > 1u || (major == 1u && minor >= 1u);
}

#ifdef __SWITCH__
inline uint32_t gameLaunchReadLe32(const unsigned char* p) noexcept {
    return static_cast<uint32_t>(p[0]) |
           (static_cast<uint32_t>(p[1]) << 8) |
           (static_cast<uint32_t>(p[2]) << 16) |
           (static_cast<uint32_t>(p[3]) << 24);
}

inline uint64_t gameLaunchReadLe64(const unsigned char* p) noexcept {
    return static_cast<uint64_t>(gameLaunchReadLe32(p)) |
           (static_cast<uint64_t>(gameLaunchReadLe32(p + 4)) << 32);
}

inline DraSticDirectLaunchSupport inspectDraSticDirectLaunchSupport(
    std::string_view launcherPath, std::string& displayVersion) {
    // Product Home queries ready() every frame. Cache the one installed DraStic path so the NRO is
    // never reopened during rendering/navigation; replacing the emulator while PokeBank is running
    // simply takes effect on the next app launch.
    static std::string cachedPath;
    static std::string cachedVersion;
    static DraSticDirectLaunchSupport cachedSupport = DraSticDirectLaunchSupport::Unknown;
    static bool cached = false;
    if (cached && cachedPath == launcherPath) {
        displayVersion = cachedVersion;
        return cachedSupport;
    }

    cached = true;
    cachedPath = std::string(launcherPath);
    cachedVersion.clear();
    cachedSupport = DraSticDirectLaunchSupport::Unknown;
    displayVersion.clear();

    if (launcherPath.empty()) return cachedSupport;
    FILE* file = std::fopen(cachedPath.c_str(), "rb");
    if (!file) return cachedSupport;

    auto readAt = [&](uint64_t offset, void* out, size_t size) -> bool {
        if (offset > 0x7fffffffULL) return false;
        return std::fseek(file, static_cast<long>(offset), SEEK_SET) == 0 &&
               std::fread(out, 1, size, file) == size;
    };

    std::array<unsigned char, 0x20> nro{};
    if (!readAt(0, nro.data(), nro.size()) ||
        nro[0x10] != 'N' || nro[0x11] != 'R' || nro[0x12] != 'O' || nro[0x13] != '0') {
        std::fclose(file);
        return cachedSupport;
    }
    const uint32_t nroSize = gameLaunchReadLe32(nro.data() + 0x18);
    if (nroSize < nro.size()) {
        std::fclose(file);
        return cachedSupport;
    }

    // ASET immediately follows the NRO image. AssetSection #2 is NACP; its offset is relative to
    // the ASET start (the NRO size). Only the header through the NACP pair is needed here.
    std::array<unsigned char, 0x28> aset{};
    if (!readAt(nroSize, aset.data(), aset.size()) ||
        aset[0] != 'A' || aset[1] != 'S' || aset[2] != 'E' || aset[3] != 'T') {
        std::fclose(file);
        return cachedSupport;
    }
    const uint64_t nacpOffset = gameLaunchReadLe64(aset.data() + 0x18);
    const uint64_t nacpSize = gameLaunchReadLe64(aset.data() + 0x20);
    constexpr uint64_t kNacpDisplayVersionOffset = 0x3060;
    constexpr size_t kNacpDisplayVersionSize = 0x10;
    if (nacpSize < kNacpDisplayVersionOffset + kNacpDisplayVersionSize) {
        std::fclose(file);
        return cachedSupport;
    }

    std::array<char, kNacpDisplayVersionSize> rawVersion{};
    const uint64_t displayOffset = static_cast<uint64_t>(nroSize) + nacpOffset +
                                   kNacpDisplayVersionOffset;
    if (!readAt(displayOffset, rawVersion.data(), rawVersion.size())) {
        std::fclose(file);
        return cachedSupport;
    }
    std::fclose(file);

    size_t length = 0;
    while (length < rawVersion.size() && rawVersion[length] != '\0') ++length;
    cachedVersion.assign(rawVersion.data(), length);
    while (!cachedVersion.empty() && std::isspace(static_cast<unsigned char>(cachedVersion.back())))
        cachedVersion.pop_back();
    displayVersion = cachedVersion;
    if (cachedVersion.empty()) return cachedSupport;

    cachedSupport = drasticDirectLaunchVersionSupportsPositionalRom(cachedVersion)
        ? DraSticDirectLaunchSupport::Supported
        : DraSticDirectLaunchSupport::Unsupported;
    return cachedSupport;
}
#endif

struct GameLaunchDescriptor {
    GameLaunchBackend backend = GameLaunchBackend::None;
    GameLaunchState state = GameLaunchState::Unavailable;
    uint64_t titleId = 0;
    std::string providerId;
    std::string launcherPath;
    std::string contentPath;
    std::string corePath;
    mutable std::string detail;

    [[nodiscard]] bool ready() const {
        const bool nominallyReady = state == GameLaunchState::Ready ||
                                    state == GameLaunchState::LauncherOnly;
        if (!nominallyReady) return false;
#ifdef __SWITCH__
        // Launcher-only mode does not promise a selected game. Gate only the direct-content path.
        if (state == GameLaunchState::Ready && !contentPath.empty() &&
            compactLaunchProvider(providerId).find("drastic") != std::string::npos) {
            std::string version;
            const auto support = inspectDraSticDirectLaunchSupport(launcherPath, version);
            if (support != DraSticDirectLaunchSupport::Supported) {
                if (support == DraSticDirectLaunchSupport::Unsupported && !version.empty()) {
                    detail = "DraStic " + version +
                        " cannot direct-launch the selected game. Update to DraStic 1.1.0 or newer to bypass its file chooser.";
                } else {
                    detail = "DraStic direct-launch support could not be verified. Install DraStic 1.1.0 or newer to bypass its file chooser.";
                }
                return false;
            }
        }
#endif
        return true;
    }
};

inline std::string normalizedLaunchStem(std::string_view path) {
    const size_t slash = path.find_last_of("/\\");
    const size_t first = slash == std::string_view::npos ? 0 : slash + 1;
    const size_t dot = path.find_last_of('.');
    const size_t last = dot == std::string_view::npos || dot < first ? path.size() : dot;
    std::string out;
    out.reserve(last - first);
    for (size_t i = first; i < last; ++i) {
        const unsigned char c = static_cast<unsigned char>(path[i]);
        // ROM libraries and RetroArch playlists commonly use the official “Pokémon” spelling.
        // Normalize UTF-8 é/É instead of dropping both bytes and turning it into “pokmon”.
        if (c == 0xC3 && i + 1 < last) {
            const unsigned char next = static_cast<unsigned char>(path[i + 1]);
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

inline GameLaunchProviderKind gameLaunchProviderKind(std::string_view providerId) {
    const std::string compact = compactLaunchProvider(providerId);
    if (compact.find("retroarch") != std::string::npos) return GameLaunchProviderKind::RetroArch;
    if (compact == "mgba" || compact.find("mgba") != std::string::npos)
        return GameLaunchProviderKind::MGBA;
    if (compact.find("tico") != std::string::npos) return GameLaunchProviderKind::Tico;
    if (compact.find("drastic") != std::string::npos) return GameLaunchProviderKind::DraStic;
    if (compact.find("melonds") != std::string::npos) return GameLaunchProviderKind::MelonDS;
    return GameLaunchProviderKind::Unknown;
}

inline bool launchProviderIsRetroArch(std::string_view providerId) {
    return gameLaunchProviderKind(providerId) == GameLaunchProviderKind::RetroArch;
}

inline bool gameLaunchProviderAcceptsContentArgument(GameLaunchProviderKind kind) noexcept {
    // DrasticDS_nx 1.1.0+ accepts a direct ROM/archive path as argv[1], matching the content
    // handoff already used by melonDS, mGBA, RetroArch and Tico. The installed-version gate lives
    // on GameLaunchDescriptor::ready(), where the actual launcher path is available.
    return kind != GameLaunchProviderKind::Unknown;
}

inline std::string lowerLaunchExtension(std::string_view path) {
    const size_t slash = path.find_last_of("/\\");
    const size_t dot = path.find_last_of('.');
    if (dot == std::string_view::npos || (slash != std::string_view::npos && dot < slash))
        return {};
    std::string out(path.substr(dot));
    std::transform(out.begin(), out.end(), out.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return out;
}

inline bool gameLaunchContentSupported(std::string_view gameId, std::string_view path) {
    const std::string ext = lowerLaunchExtension(path);
    const auto suffix = [&](std::string_view value) {
        return gameId.size() >= value.size() &&
            gameId.substr(gameId.size() - value.size()) == value;
    };
    if (suffix("_gb"))  return ext == ".gb"  || ext == ".zip" || ext == ".7z";
    if (suffix("_gbc")) return ext == ".gbc" || ext == ".zip" || ext == ".7z";
    if (suffix("_gba")) return ext == ".gba" || ext == ".zip" || ext == ".7z";
    if (suffix("_nds")) return ext == ".nds" || ext == ".zip" || ext == ".rar";
    return false;
}

inline bool gameLaunchCandidateStemMatches(std::string_view gameId,
                                            std::string_view savePath,
                                            std::string_view contentPath) {
    const std::string wanted = normalizedLaunchStem(savePath);
    const std::string candidate = normalizedLaunchStem(contentPath);
    if (candidate.empty()) return false;
    if (!wanted.empty() && candidate == wanted) return true;

    // Emulator saves often add/remove words such as "Pokemon", "Version", region tags, or archive
    // suffixes. Accept a containment match only for a reasonably distinctive stem; the caller still
    // requires EXACTLY ONE filesystem match before launching, so ambiguity remains fail-closed.
    if (wanted.size() >= 5 &&
        (candidate.find(wanted) != std::string::npos ||
         wanted.find(candidate) != std::string::npos))
        return true;

    // Release-specific official-name prefixes make short identities safe too.
    // "pokemonred..." cannot collide with "pokemonfirered..."; likewise Gold/HeartGold and
    // Diamond/Brilliant Diamond remain distinct. Region/revision tags may follow the prefix.
    auto officialRelease = [&](std::string_view token) {
        const std::string plain(token);
        const std::string pokemon = "pokemon" + plain;
        return candidate == plain ||
               candidate.rfind(plain + "version", 0) == 0 ||
               candidate.rfind(pokemon, 0) == 0;
    };
    if (gameId == "red_gb") return officialRelease("red");
    if (gameId == "blue_gb") return officialRelease("blue");
    if (gameId == "yellow_gb") return officialRelease("yellow");
    if (gameId == "gold_gbc") return officialRelease("gold");
    if (gameId == "silver_gbc") return officialRelease("silver");
    if (gameId == "crystal_gbc") return officialRelease("crystal");
    if (gameId == "ruby_gba") return officialRelease("ruby");
    if (gameId == "sapphire_gba") return officialRelease("sapphire");
    if (gameId == "firered_gba") return officialRelease("firered");
    if (gameId == "leafgreen_gba") return officialRelease("leafgreen");
    if (gameId == "emerald_gba") return officialRelease("emerald");
    if (gameId == "diamond_nds") return officialRelease("diamond");
    if (gameId == "pearl_nds") return officialRelease("pearl");
    if (gameId == "platinum_nds") return officialRelease("platinum");
    if (gameId == "heartgold_nds") return officialRelease("heartgold");
    if (gameId == "soulsilver_nds") return officialRelease("soulsilver");

    const size_t underscore = gameId.find('_');
    const std::string_view identity = gameId.substr(0, underscore);
    std::string compact;
    compact.reserve(identity.size());
    for (unsigned char c : identity)
        if (std::isalnum(c)) compact.push_back(static_cast<char>(std::tolower(c)));
    return compact.size() >= 5 && candidate.find(compact) != std::string::npos;
}

inline int gameLaunchCandidateScore(std::string_view gameId,
                                    std::string_view savePath,
                                    std::string_view contentPath) {
    if (!gameLaunchContentSupported(gameId, contentPath) ||
        !gameLaunchCandidateStemMatches(gameId, savePath, contentPath))
        return -1;

    const std::string wanted = normalizedLaunchStem(savePath);
    const std::string candidate = normalizedLaunchStem(contentPath);
    int score = 100;
    if (!wanted.empty() && candidate == wanted) {
        score += 10000;
    } else if (!wanted.empty()) {
        if (candidate.find(wanted) != std::string::npos ||
            wanted.find(candidate) != std::string::npos)
            score += 4000;
        size_t prefix = 0;
        while (prefix < wanted.size() && prefix < candidate.size() &&
               wanted[prefix] == candidate[prefix]) ++prefix;
        score += static_cast<int>(std::min<size_t>(prefix, 200) * 4);
    }

    // For DS providers prefer a raw ROM in the emulator-owned library over duplicate archives
    // or generic mirrors. A genuine equal-strength tie still fails closed and is shown in-app.
    if (gameId.size() >= 4 && gameId.substr(gameId.size() - 4) == "_nds") {
        const std::string ext = lowerLaunchExtension(contentPath);
        if (ext == ".nds") score += 600;
        else if (ext == ".zip") score += 250;
        else if (ext == ".rar") score += 150;

        std::string lower(contentPath);
        std::transform(lower.begin(), lower.end(), lower.begin(),
            [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        if (lower.find("/switch/drastic/games/") != std::string::npos) score += 900;
        else if (lower.find("/switch/drastic/roms/") != std::string::npos) score += 800;
        else if (lower.find("/roms/nds/") != std::string::npos) score += 500;
    }
    return score;
}

inline std::string gameLaunchBindingFamilyPrefix(std::string_view profileIdentity,
                                                  std::string_view gameId) {
    if (profileIdentity.empty() || gameId.empty()) return {};
    std::string out;
    out.reserve(profileIdentity.size() + gameId.size() + 2);
    out.append(profileIdentity);
    out.push_back('|');
    out.append(gameId);
    out.push_back('|');
    return out;
}

inline std::string gameLaunchBindingKey(std::string_view profileIdentity,
                                        std::string_view gameId,
                                        std::string_view sourceIdentity) {
    if (sourceIdentity.empty()) return {};
    std::string out = gameLaunchBindingFamilyPrefix(profileIdentity, gameId);
    if (out.empty()) return {};
    out.append(sourceIdentity);
    return out;
}

inline const char* gameLaunchActionLabel(GameLaunchState state) noexcept {
    switch (state) {
        case GameLaunchState::Ready:            return "Launch";
        case GameLaunchState::LauncherOnly:     return "Launch Emulator";
        case GameLaunchState::ChooseSource:     return "Choose & Launch";
        case GameLaunchState::NeedsContentLink: return "Link Game File";
        case GameLaunchState::LauncherMissing:  return "Launcher Missing";
        case GameLaunchState::Unavailable:      return "Unavailable";
    }
    return "Unavailable";
}

} // namespace UI

#endif