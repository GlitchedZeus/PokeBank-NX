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
    ChooseSource,
    NeedsContentLink,
    LauncherMissing,
    Unavailable,
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

    [[nodiscard]] bool ready() const noexcept { return state == GameLaunchState::Ready; }
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

inline bool launchProviderIsRetroArch(std::string_view providerId) {
    std::string compact;
    compact.reserve(providerId.size());
    for (unsigned char c : providerId)
        if (std::isalnum(c)) compact.push_back(static_cast<char>(std::tolower(c)));
    return compact.find("retroarch") != std::string::npos;
}

inline const char* gameLaunchActionLabel(GameLaunchState state) noexcept {
    switch (state) {
        case GameLaunchState::Ready:            return "Launch";
        case GameLaunchState::ChooseSource:     return "Choose & Launch";
        case GameLaunchState::NeedsContentLink: return "Link Game File";
        case GameLaunchState::LauncherMissing:  return "Launcher Missing";
        case GameLaunchState::Unavailable:      return "Unavailable";
    }
    return "Unavailable";
}

} // namespace UI

#endif
