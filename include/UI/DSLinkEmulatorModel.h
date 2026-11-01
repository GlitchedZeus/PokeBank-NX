#ifndef POKEBANK_UI_DS_LINK_EMULATOR_MODEL_H
#define POKEBANK_UI_DS_LINK_EMULATOR_MODEL_H

// Presentation-only Nintendo DS provider chooser. The selected provider is a
// discovery FILTER, never a trust signal: Gen IV/V still require native parsing,
// source fingerprint verification and exclusive profile/game assignment.
#include <array>
#include <string_view>

namespace PokeBank::UIModel::DSLinkEmulator {

struct Choice {
    std::string_view title;
    std::string_view description;
    std::string_view providerFilter; // empty = scan all known providers
    bool manual = false;
};

inline constexpr std::array<Choice, 5> Choices{{
    {"melonDS", "Scan known melonDS battery-save folders. No savestates.", "melonDS", false},
    {"DraStic", "Scan DraStic cartridge backups; savestates are not assignable.", "DraStic", false},
    {"RetroArch", "Scan the configured RetroArch savefiles folder.", "RetroArch", false},
    {"All Known Emulators", "Scan all supported DS providers, newest saves first.", "", false},
    {"Choose Save File", "Browse a specific cartridge save if discovery finds none.", "", true},
}};

[[nodiscard]] constexpr int wrapIndex(int index, int delta) noexcept {
    constexpr int count = static_cast<int>(Choices.size());
    return ((index + delta) % count + count) % count;
}

[[nodiscard]] constexpr bool acceptsProvider(std::string_view actual,
                                              std::string_view filter) noexcept {
    if (filter.empty()) return true;
    if (filter == "DraStic") return actual == "DraStic" || actual == "DraStic Savestate";
    return actual == filter;
}

} // namespace PokeBank::UIModel::DSLinkEmulator
#endif
