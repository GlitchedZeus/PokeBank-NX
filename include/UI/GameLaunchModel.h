#ifndef UI_GAME_LAUNCH_MODEL_H
#define UI_GAME_LAUNCH_MODEL_H

#include <algorithm>
#include <array>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string>
#include <string_view>

// Shared with the bundled return host so PokeBank and the host agree byte-for-byte on argv.
#include "../../runtime/return_host/source/return_host_policy.h"

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
    const size_t archiveMember = path.find_last_of('#');
    size_t first = slash == std::string_view::npos ? 0 : slash + 1;
    if (archiveMember != std::string_view::npos && archiveMember + 1 > first)
        first = archiveMember + 1;
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
    // A Black/White ROM name never names Black 2/White 2 and vice versa, even by containment
    // (a .zip has no readable cartridge header to catch the mix-up later).
    // No-Intro names the sequels "Pokemon - Black Version 2"; others write "Pokemon Black 2".
    const bool unovaSequelName = candidate.find("black2") != std::string::npos ||
                                 candidate.find("white2") != std::string::npos ||
                                 candidate.find("blackversion2") != std::string::npos ||
                                 candidate.find("whiteversion2") != std::string::npos;
    if ((gameId == "black_nds" || gameId == "white_nds") && unovaSequelName) return false;
    if ((gameId == "black2_nds" || gameId == "white2_nds") && !unovaSequelName) return false;
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
    // The original/sequel split was already enforced by unovaSequelName above.
    if (gameId == "black_nds" || gameId == "black2_nds") return officialRelease("black");
    if (gameId == "white_nds" || gameId == "white2_nds") return officialRelease("white");

    const size_t underscore = gameId.find('_');
    const std::string_view identity = gameId.substr(0, underscore);
    std::string compact;
    compact.reserve(identity.size());
    for (unsigned char c : identity)
        if (std::isalnum(c)) compact.push_back(static_cast<char>(std::tolower(c)));
    return compact.size() >= 5 && candidate.find(compact) != std::string::npos;
}

// ROM-content identity. A filename match is not proof of the game: before any content file is
// linked or launched, its cartridge header is compared with the selected game. Only a header that
// positively identifies a DIFFERENT Pokémon release is rejected; archives, hacks with unknown
// headers and unreadable files stay "Unknown" so legitimate localized dumps are never refused.
enum class GameLaunchRomIdentity : uint8_t { Unknown, Matches, Contradicts };

// Bytes a caller must supply (from file offset 0) for gameLaunchRomHeaderGameId().
inline constexpr size_t kGameLaunchRomHeaderBytes = 0x150;

inline std::string_view gameLaunchRomHeaderGameId(std::string_view extension,
                                                   const unsigned char* data,
                                                   size_t size) noexcept {
    if (!data) return {};
    if (extension == ".gb" || extension == ".gbc") {
        if (size < 0x144) return {};
        // Title is 0x134..0x143; on CGB-aware carts 0x143 is the CGB flag, not text.
        const size_t titleEnd = (data[0x143] & 0x80u) ? 0x143 : 0x144;
        size_t end = 0x134;
        while (end < titleEnd && data[end] != 0) ++end;
        while (end > 0x134 && data[end - 1] == ' ') --end;
        const std::string_view title(reinterpret_cast<const char*>(data + 0x134), end - 0x134);
        constexpr std::string_view kRed[] = {"POKEMON RED", "POKEMON ROT", "POKEMON ROUGE",
                                             "POKEMON ROSSO", "POKEMON ROJO"};
        constexpr std::string_view kBlue[] = {"POKEMON BLUE", "POKEMON BLAU", "POKEMON BLEU",
                                              "POKEMON BLU", "POKEMON AZUL"};
        for (auto t : kRed) if (title == t) return "red_gb";
        for (auto t : kBlue) if (title == t) return "blue_gb";
        if (title == "POKEMON GREEN") return "green_gb";
        if (title.rfind("POKEMON YEL", 0) == 0) return "yellow_gb";
        if (title.rfind("POKEMON_GLD", 0) == 0) return "gold_gbc";
        if (title.rfind("POKEMON_SLV", 0) == 0) return "silver_gbc";
        if (title.rfind("PM_CRYSTAL", 0) == 0) return "crystal_gbc";
        return {};
    }
    if (extension == ".gba") {
        if (size < 0xBD || data[0xB2] != 0x96) return {};  // fixed GBA header value
        const std::string_view code(reinterpret_cast<const char*>(data + 0xAC), 3);
        if (code == "AXV") return "ruby_gba";
        if (code == "AXP") return "sapphire_gba";
        if (code == "BPE") return "emerald_gba";
        if (code == "BPR") return "firered_gba";
        if (code == "BPG") return "leafgreen_gba";
        return {};
    }
    if (extension == ".nds") {
        if (size < 0x10) return {};
        const std::string_view code(reinterpret_cast<const char*>(data + 0x0C), 3);
        if (code == "ADA") return "diamond_nds";
        if (code == "APA") return "pearl_nds";
        if (code == "CPU") return "platinum_nds";
        if (code == "IPK") return "heartgold_nds";
        if (code == "IPG") return "soulsilver_nds";
        // Unova cartridge codes (all regions share the first three characters).
        if (code == "IRB") return "black_nds";
        if (code == "IRA") return "white_nds";
        if (code == "IRE") return "black2_nds";
        if (code == "IRD") return "white2_nds";
        return {};
    }
    return {};
}

inline GameLaunchRomIdentity gameLaunchRomIdentity(std::string_view gameId,
                                                   std::string_view contentPath,
                                                   const unsigned char* header,
                                                   size_t size) noexcept {
    const std::string extension = lowerLaunchExtension(contentPath);
    const std::string_view identified = gameLaunchRomHeaderGameId(extension, header, size);
    if (identified.empty()) return GameLaunchRomIdentity::Unknown;
    return identified == gameId ? GameLaunchRomIdentity::Matches
                                : GameLaunchRomIdentity::Contradicts;
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

// ---------------------------------------------------------------------------------------------
// Chainload plan: exactly what PokeBank hands to envSetNextLoad() for a homebrew launch.
//
// hbloader keeps a 512-byte next path and a 2048-byte next argv, and libnx envSetNextLoad()
// strcpy()s into them unchecked, so an over-long or unrepresentable path must be refused here,
// before PokeBank exits (see return_host_policy.h for the pinned references).
// ---------------------------------------------------------------------------------------------

// Where the emulator's own quit action leads, so the UI can state it before launching.
enum class GameLaunchQuitRoute : uint8_t {
    None,              // no homebrew chain (Switch title or not launchable)
    LoaderDefault,     // nothing scheduled after quit: hbloader loads sdmc:/hbmenu.nro, forwarders
                       // usually end the process (HOME)
    EmulatorLauncher,  // DraStic "Quit to launcher" reloads DrasticDS.nro (its launcher)
    ReturnsToPokeBank, // bundled return host reloads this PokeBank NRO
};

struct GameLaunchReturnOptions {
    std::string_view pokeBankNroPath;   // argv[0] of the running PokeBank (validated by caller)
    std::string_view returnHostNroPath; // installed PokeBankReturnHost.nro
    bool returnHostEnabled = false;
};

struct GameLaunchChainPlan {
    bool ok = false;
    std::string nroPath;
    std::string argv;
    bool viaReturnHost = false;
    GameLaunchQuitRoute quitRoute = GameLaunchQuitRoute::None;
    std::string error;
    std::string returnHostSkipped; // why an enabled return host was not used (direct fallback)
};

inline bool gameLaunchPathFitsLoader(std::string_view path) noexcept {
    if (path.empty() || path.size() >= RH_LOADER_PATH_CAP) return false;
    for (unsigned char c : path)
        if (c == '"' || c < 0x20 || c == 0x7F) return false;
    return true;
}

// libnx argvSetup() has no escape character, so a value is either quoted verbatim or refused.
inline bool gameLaunchAppendArg(std::string& argv, std::string_view value) {
    if (value.empty()) return false;
    for (unsigned char c : value)
        if (c == '"' || c < 0x20 || c == 0x7F) return false;
    if (!argv.empty()) argv.push_back(' ');
    argv.push_back('"');
    argv.append(value);
    argv.push_back('"');
    return true;
}

inline bool gameLaunchArgvFitsLoader(std::string_view argv) noexcept {
    return !argv.empty() && argv.size() < RH_LOADER_ARGV_CAP;
}

inline GameLaunchProviderKind gameLaunchDescriptorKind(const GameLaunchDescriptor& d) {
    if (d.backend == GameLaunchBackend::RetroArch) return GameLaunchProviderKind::RetroArch;
    return gameLaunchProviderKind(d.providerId);
}

inline GameLaunchChainPlan buildGameLaunchChainPlan(const GameLaunchDescriptor& descriptor,
                                                    const GameLaunchReturnOptions& options = {}) {
    GameLaunchChainPlan plan;
    if (descriptor.backend != GameLaunchBackend::RetroArch &&
        descriptor.backend != GameLaunchBackend::HomebrewNro) {
        plan.error = "This launch does not chain to a homebrew NRO.";
        return plan;
    }
    const std::string& target = descriptor.backend == GameLaunchBackend::RetroArch &&
                                        !descriptor.corePath.empty()
                                    ? descriptor.corePath
                                    : descriptor.launcherPath;
    const bool withContent = descriptor.state != GameLaunchState::LauncherOnly &&
                             !descriptor.contentPath.empty();
    if (!gameLaunchPathFitsLoader(target)) {
        plan.error = "The emulator path cannot be passed to the homebrew loader.";
        return plan;
    }
    if (withContent && !gameLaunchPathFitsLoader(descriptor.contentPath)) {
        plan.error = "The game file path cannot be passed to the emulator (too long or contains '\"').";
        return plan;
    }

    const GameLaunchProviderKind kind = gameLaunchDescriptorKind(descriptor);
    std::string direct;
    gameLaunchAppendArg(direct, target);
    if (withContent) gameLaunchAppendArg(direct, descriptor.contentPath);
    if (!gameLaunchArgvFitsLoader(direct)) {
        plan.error = "The emulator launch arguments are too long.";
        return plan;
    }

    if (options.returnHostEnabled) {
        const char* mode = kind == GameLaunchProviderKind::DraStic
            ? RH_MODE_RETURN_ON_LAUNCHER : RH_MODE_RETURN_ON_EXIT;
        std::string hosted;
        if (!gameLaunchPathFitsLoader(options.pokeBankNroPath)) {
            plan.returnHostSkipped = "PokeBank's own NRO path is unknown or cannot be passed back.";
        } else if (!gameLaunchPathFitsLoader(options.returnHostNroPath)) {
            plan.returnHostSkipped = "The return host is not installed.";
        } else if (!gameLaunchAppendArg(hosted, options.returnHostNroPath) ||
                   !gameLaunchAppendArg(hosted, options.pokeBankNroPath) ||
                   !gameLaunchAppendArg(hosted, mode) ||
                   !gameLaunchAppendArg(hosted, target) ||
                   (withContent && !gameLaunchAppendArg(hosted, descriptor.contentPath)) ||
                   !gameLaunchArgvFitsLoader(hosted)) {
            plan.returnHostSkipped = "The return handoff arguments are too long.";
        } else {
            plan.ok = true;
            plan.viaReturnHost = true;
            plan.nroPath = std::string(options.returnHostNroPath);
            plan.argv = std::move(hosted);
            plan.quitRoute = GameLaunchQuitRoute::ReturnsToPokeBank;
            return plan;
        }
    }

    plan.ok = true;
    plan.nroPath = target;
    plan.argv = std::move(direct);
    plan.quitRoute = kind == GameLaunchProviderKind::DraStic
        ? GameLaunchQuitRoute::EmulatorLauncher : GameLaunchQuitRoute::LoaderDefault;
    return plan;
}

inline const char* gameLaunchQuitRouteNotice(GameLaunchQuitRoute route) noexcept {
    switch (route) {
        case GameLaunchQuitRoute::ReturnsToPokeBank:
            return "Quitting the emulator returns to PokeBank NX.";
        case GameLaunchQuitRoute::EmulatorLauncher:
            return "DraStic's Quit to launcher opens DraStic's own launcher; reopen PokeBank NX from the Homebrew Menu.";
        case GameLaunchQuitRoute::LoaderDefault:
            return "Quitting the emulator returns to the Homebrew Menu (or HOME when PokeBank NX was started from a forwarder).";
        case GameLaunchQuitRoute::None:
            break;
    }
    return "";
}

// ---------------------------------------------------------------------------------------------
// Return selection: Product Home's exact focus at the moment a launch handed off, so the next
// PokeBank start (after the emulator quits) reopens on the same game instead of the first card.
// Consumed once; text format, one key per line; values may not contain control characters.
// ---------------------------------------------------------------------------------------------

struct GameLaunchReturnSelection {
    std::string profileIdentity;
    std::string gameId;
    std::string sourceIdentity;
    int sourceKind = 0;
    uint64_t titleId = 0;
    int userIndex = 0;
    int titleIndex = 0;
    bool classicGamesActive = false;
    int64_t savedAt = 0;
};

inline constexpr std::string_view kGameLaunchReturnSelectionHeader = "POKEBANK_LAUNCH_RETURN_V1";
// A record older than this is from an abandoned session, not a return.
inline constexpr int64_t kGameLaunchReturnSelectionMaxAgeSeconds = 7 * 24 * 60 * 60;

inline bool gameLaunchReturnValueSafe(std::string_view value) noexcept {
    if (value.size() > 512) return false;
    for (unsigned char c : value)
        if (c < 0x20 || c == 0x7F) return false;
    return true;
}

inline bool serializeGameLaunchReturnSelection(const GameLaunchReturnSelection& selection,
                                               std::string& out) {
    out.clear();
    if (selection.gameId.empty() || !gameLaunchReturnValueSafe(selection.gameId) ||
        !gameLaunchReturnValueSafe(selection.profileIdentity) ||
        !gameLaunchReturnValueSafe(selection.sourceIdentity))
        return false;
    char numbers[192];
    std::snprintf(numbers, sizeof(numbers),
                  "kind=%d\ntitle=%016llx\nuser=%d\nindex=%d\nclassic=%d\nsaved=%lld\n",
                  selection.sourceKind, static_cast<unsigned long long>(selection.titleId),
                  selection.userIndex, selection.titleIndex,
                  selection.classicGamesActive ? 1 : 0,
                  static_cast<long long>(selection.savedAt));
    out.append(kGameLaunchReturnSelectionHeader);
    out.append("\nprofile=").append(selection.profileIdentity);
    out.append("\ngame=").append(selection.gameId);
    out.append("\nsource=").append(selection.sourceIdentity);
    out.push_back('\n');
    out.append(numbers);
    return true;
}

inline bool gameLaunchParseInt64(std::string_view text, int64_t& out, int base = 10) {
    if (text.empty() || text.size() > 20) return false;
    bool negative = false;
    size_t i = 0;
    if (base == 10 && text[0] == '-') { negative = true; i = 1; }
    if (i >= text.size()) return false;
    uint64_t value = 0;
    for (; i < text.size(); ++i) {
        const char c = text[i];
        int digit = -1;
        if (c >= '0' && c <= '9') digit = c - '0';
        else if (base == 16 && c >= 'a' && c <= 'f') digit = c - 'a' + 10;
        if (digit < 0 || digit >= base) return false;
        if (value > (UINT64_MAX - static_cast<uint64_t>(digit)) / static_cast<uint64_t>(base))
            return false;
        value = value * static_cast<uint64_t>(base) + static_cast<uint64_t>(digit);
    }
    if (base == 10 && value > static_cast<uint64_t>(INT64_MAX)) return false;
    out = negative ? -static_cast<int64_t>(value) : static_cast<int64_t>(value);
    return true;
}

inline bool parseGameLaunchReturnSelection(std::string_view text, int64_t now,
                                           GameLaunchReturnSelection& out) {
    out = {};
    GameLaunchReturnSelection parsed;
    bool header = false, haveGame = false, haveSaved = false;
    size_t pos = 0;
    while (pos < text.size()) {
        size_t end = text.find('\n', pos);
        if (end == std::string_view::npos) end = text.size();
        std::string_view line = text.substr(pos, end - pos);
        pos = end + 1;
        if (!line.empty() && line.back() == '\r') line.remove_suffix(1);
        if (!header) {
            if (line != kGameLaunchReturnSelectionHeader) return false;
            header = true;
            continue;
        }
        if (line.empty()) continue;
        const size_t eq = line.find('=');
        if (eq == std::string_view::npos) return false;
        const std::string_view key = line.substr(0, eq);
        const std::string_view value = line.substr(eq + 1);
        if (!gameLaunchReturnValueSafe(value)) return false;
        int64_t number = 0;
        if (key == "profile") parsed.profileIdentity = std::string(value);
        else if (key == "game") { parsed.gameId = std::string(value); haveGame = !value.empty(); }
        else if (key == "source") parsed.sourceIdentity = std::string(value);
        else if (key == "title") {
            if (value.size() != 16 || !gameLaunchParseInt64(value.substr(0, 8), number, 16)) return false;
            int64_t low = 0;
            if (!gameLaunchParseInt64(value.substr(8), low, 16)) return false;
            parsed.titleId = (static_cast<uint64_t>(number) << 32) | static_cast<uint64_t>(low);
        } else if (key == "kind" || key == "user" || key == "index" || key == "classic") {
            if (!gameLaunchParseInt64(value, number) || number < 0 || number > 0xFFFF) return false;
            if (key == "kind") parsed.sourceKind = static_cast<int>(number);
            else if (key == "user") parsed.userIndex = static_cast<int>(number);
            else if (key == "index") parsed.titleIndex = static_cast<int>(number);
            else {
                if (number > 1) return false;
                parsed.classicGamesActive = number == 1;
            }
        } else if (key == "saved") {
            if (!gameLaunchParseInt64(value, number)) return false;
            parsed.savedAt = number;
            haveSaved = true;
        }
        // Unknown keys are ignored so a newer writer never strands an older reader.
    }
    if (!header || !haveGame || !haveSaved) return false;
    if (parsed.savedAt <= 0 || parsed.savedAt > now + 60 ||
        now - parsed.savedAt > kGameLaunchReturnSelectionMaxAgeSeconds)
        return false;
    out = std::move(parsed);
    return true;
}

} // namespace UI

#endif