#ifndef POKEBANK_UI_LINK_EMULATOR_MODEL_H
#define POKEBANK_UI_LINK_EMULATOR_MODEL_H

// One app-wide source/provider chooser. The provider is a discovery filter,
// NEVER validation or write authority. The existing generation-native Save
// Instances and immutable profile bindings remain the sole assignment path.
#include "Games/GameIdentity.h"

#include <array>
#include <cstddef>
#include <span>
#include <string_view>

namespace PokeBank::UIModel::LinkEmulator {

using Platform = PokeVault::Games::Platform;

struct Choice {
    std::string_view title;
    std::string_view description;
    std::string_view provider; // empty = all known roots; never a trust signal
    bool manualFile = false;
};

inline constexpr std::array<Choice, 5> Classic{{
    {"Find Automatically", "Scan supported battery-save folders.", "", false},
    {"RetroArch", "Use the configured RetroArch savefiles folder.", "RetroArch", false},
    {"mGBA", "Use mGBA cartridge saves when discovered.", "mGBA", false},
    {"Tico", "Use Tico cartridge saves when discovered.", "Tico", false},
    {"Other Sources", "Browse already discovered compatible saves.", "", false},
}};
inline constexpr std::array<Choice, 6> DS{{
    {"Find Automatically", "Scan known Nintendo DS emulator folders.", "", false},
    {"melonDS", "Search melonDS battery saves; not savestates.", "melonDS", false},
    {"DraStic", "Search DraStic backups; savestates cannot be linked.", "DraStic", false},
    {"RetroArch", "Search configured RetroArch battery saves.", "RetroArch", false},
    {"DeSmuME File", "Select a compatible .dsv file manually.", "Manual", true},
    {"Other / Manual", "Choose a compatible DS battery-save file.", "Manual", true},
}};
inline constexpr std::array<Choice, 5> ThreeDS{{
    {"Find Automatically", "Search known verified 3DS save export locations.", "", false},
    {"Dekopon", "Search Dekopon's known save folders.", "Dekopon", false},
    {"Checkpoint", "Search 3DS Checkpoint exported saves.", "Checkpoint", false},
    {"Azahar File", "Select an exported Azahar save manually.", "Manual", true},
    {"Other / Manual", "Select a compatible 3DS save archive.", "Manual", true},
}};

[[nodiscard]] constexpr bool nativeSwitch(Platform platform, bool gbaForwarder = false) noexcept {
    return platform == Platform::NintendoSwitch && !gbaForwarder;
}
[[nodiscard]] constexpr Platform effectivePlatform(Platform platform, bool gbaForwarder) noexcept {
    return gbaForwarder ? Platform::GameBoyAdvance : platform;
}
[[nodiscard]] constexpr std::span<const Choice> choices(Platform platform) noexcept {
    switch (platform) {
        case Platform::GameBoy:
        case Platform::GameBoyColor:
        case Platform::GameBoyAdvance: return Classic;
        case Platform::NintendoDS: return DS;
        case Platform::Nintendo3DS: return ThreeDS;
        case Platform::NintendoSwitch: return {};
    }
    return {};
}
[[nodiscard]] constexpr std::string_view platformLabel(Platform platform) noexcept {
    switch (platform) {
        case Platform::GameBoy: return "GAME BOY";
        case Platform::GameBoyColor: return "GAME BOY COLOR";
        case Platform::GameBoyAdvance: return "GAME BOY ADVANCE";
        case Platform::NintendoDS: return "NINTENDO DS";
        case Platform::Nintendo3DS: return "NINTENDO 3DS";
        case Platform::NintendoSwitch: return "NINTENDO SWITCH";
    }
    return "UNKNOWN";
}
[[nodiscard]] constexpr int wrapIndex(int i, int delta, size_t size) noexcept {
    const int count = static_cast<int>(size);
    if (count <= 0) return 0;
    return ((i + delta) % count + count) % count;
}
[[nodiscard]] constexpr bool providerMatches(std::string_view candidate, std::string_view selected) noexcept {
    if (selected.empty()) return true;
    if (selected == "DraStic")
        return candidate == "DraStic" || candidate == "DraStic Savestate";
    return candidate == selected;
}
} // namespace PokeBank::UIModel::LinkEmulator

#endif
