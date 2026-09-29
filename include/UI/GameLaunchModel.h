#ifndef UI_GAME_LAUNCH_MODEL_H
#define UI_GAME_LAUNCH_MODEL_H

#include <algorithm>
#include <cctype>
#include <cstdint>
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

struct GameLaunchDescriptor {
    GameLaunchBackend backend = GameLaunchBackend::None;
    GameLaunchState state = GameLaunchState::Unavailable;
    uint64_t titleId = 0;
    std::string providerId;
    std::string launcherPath;
    std::string contentPath;
    std::string corePath;
    std::string detail;

    [[nodiscard]] bool ready() const noexcept {
        return state == GameLaunchState::Ready ||
               state == GameLaunchState::LauncherOnly;
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
        if (std::isalnum(c)) out.push_back(static_cast<char>(std::tolower(c)));
    }
    return out;
}

inline std::string compactLaunchProvider(std::string_view providerId) {
    std::string compact;
    compact.reserve(providerId.size());
    for (unsigned char c : providerId)
        if (std::isalnum(c)) compact.push_back(static_cast<char>(std::tolower(c)));
    return compact;
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
    // Current DraSticDS_nx boots ROMs from its own Drastic/RomPath configuration and exposes
    // main(void), so passing a linked ROM as argv would falsely imply selected-game launch.
    // melonDS, mGBA, RetroArch and the existing Tico adapters accept content arguments.
    return kind != GameLaunchProviderKind::Unknown &&
           kind != GameLaunchProviderKind::DraStic;
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
    if (suffix("_nds")) return ext == ".nds";
    return false;
}

inline std::string gameLaunchBindingKey(std::string_view profileIdentity,
                                        std::string_view gameId,
                                        std::string_view sourceIdentity) {
    if (profileIdentity.empty() || gameId.empty() || sourceIdentity.empty()) return {};
    std::string out;
    out.reserve(profileIdentity.size() + gameId.size() + sourceIdentity.size() + 2);
    out.append(profileIdentity);
    out.push_back('|');
    out.append(gameId);
    out.push_back('|');
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
