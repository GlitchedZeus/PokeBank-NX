#ifndef POKEVAULT_GAMES_GAME_IDENTITY_H
#define POKEVAULT_GAMES_GAME_IDENTITY_H

#include <cstdint>
#include <span>
#include <string_view>

namespace PokeVault::Games {
    enum class Platform : uint8_t {
        GameBoy,
        GameBoyColor,
        GameBoyAdvance,
        NintendoSwitch,
    };

    enum class SourceSupport : uint8_t {
        Planned,
        ReadOnly,
        NativeSwitch,
    };

    struct GameDescriptor {
        std::string_view id;
        std::string_view title;
        Platform platform;
        uint8_t dataGeneration;
        uint64_t switchTitleId;
        SourceSupport support;

        constexpr bool isSwitchRelease() const noexcept {
            return platform == Platform::NintendoSwitch;
        }
    };

    std::span<const GameDescriptor> allGameDescriptors() noexcept;
    const GameDescriptor* findGame(std::string_view id) noexcept;
    const GameDescriptor* findSwitchGame(uint64_t titleId) noexcept;
    std::string_view platformName(Platform platform) noexcept;
}

#endif
