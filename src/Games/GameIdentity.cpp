#include "Games/GameIdentity.h"

#include <array>

namespace PokeVault::Games {
    namespace {
        using enum Platform;
        using enum SourceSupport;

        // Stable identity is release + platform, never display name alone. The Switch FireRed and
        // LeafGreen releases deliberately remain distinct from their GBA originals even though the
        // inner Pokemon data is Gen III compatible.
        constexpr std::array<GameDescriptor, 23> kGames{{
            {"red_gb",                       "Red",               GameBoy,        1, 0, Planned},
            {"blue_gb",                      "Blue",              GameBoy,        1, 0, Planned},
            {"yellow_gb",                    "Yellow",            GameBoy,        1, 0, Planned},
            {"gold_gbc",                     "Gold",              GameBoyColor,   2, 0, Planned},
            {"silver_gbc",                   "Silver",            GameBoyColor,   2, 0, Planned},
            {"crystal_gbc",                  "Crystal",           GameBoyColor,   2, 0, Planned},
            {"ruby_gba",                     "Ruby",              GameBoyAdvance, 3, 0, Planned},
            {"sapphire_gba",                 "Sapphire",          GameBoyAdvance, 3, 0, Planned},
            {"emerald_gba",                  "Emerald",           GameBoyAdvance, 3, 0, Planned},
            {"firered_gba",                  "FireRed",           GameBoyAdvance, 3, 0, Planned},
            {"leafgreen_gba",                "LeafGreen",         GameBoyAdvance, 3, 0, Planned},
            {"firered_switch",               "FireRed",           NintendoSwitch, 3, 0x0100554023408000ULL, NativeSwitch},
            {"leafgreen_switch",             "LeafGreen",         NintendoSwitch, 3, 0x010034D02340E000ULL, NativeSwitch},
            {"letsgo_pikachu_switch",        "Let's Go Pikachu",  NintendoSwitch, 7, 0x010003F003A34000ULL, NativeSwitch},
            {"letsgo_eevee_switch",          "Let's Go Eevee",    NintendoSwitch, 7, 0x0100187003A36000ULL, NativeSwitch},
            {"sword_switch",                 "Sword",             NintendoSwitch, 8, 0x0100ABF008968000ULL, NativeSwitch},
            {"shield_switch",                "Shield",            NintendoSwitch, 8, 0x01008DB008C2C000ULL, NativeSwitch},
            {"brilliant_diamond_switch",     "Brilliant Diamond", NintendoSwitch, 8, 0x0100000011D90000ULL, NativeSwitch},
            {"shining_pearl_switch",         "Shining Pearl",     NintendoSwitch, 8, 0x010018E011D92000ULL, NativeSwitch},
            {"legends_arceus_switch",        "Legends Arceus",    NintendoSwitch, 8, 0x01001F5010DFA000ULL, NativeSwitch},
            {"scarlet_switch",               "Scarlet",           NintendoSwitch, 9, 0x0100A3D008C5C000ULL, NativeSwitch},
            {"violet_switch",                "Violet",            NintendoSwitch, 9, 0x01008F6008C5E000ULL, NativeSwitch},
            {"legends_za_switch",            "Legends Z-A",       NintendoSwitch, 9, 0x0100F43008C44000ULL, NativeSwitch},
        }};
    }

    std::span<const GameDescriptor> allGameDescriptors() noexcept {
        return kGames;
    }

    const GameDescriptor* findGame(std::string_view id) noexcept {
        for (const auto& game : kGames) {
            if (game.id == id) return &game;
        }
        return nullptr;
    }

    const GameDescriptor* findSwitchGame(uint64_t titleId) noexcept {
        if (titleId == 0) return nullptr;
        for (const auto& game : kGames) {
            if (game.switchTitleId == titleId) return &game;
        }
        return nullptr;
    }

    std::string_view platformName(Platform platform) noexcept {
        switch (platform) {
            case Platform::GameBoy:        return "Game Boy";
            case Platform::GameBoyColor:   return "Game Boy Color";
            case Platform::GameBoyAdvance: return "Game Boy Advance";
            case Platform::NintendoSwitch: return "Nintendo Switch";
        }
        return "Unknown platform";
    }
}
