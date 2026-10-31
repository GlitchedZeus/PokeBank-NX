#include "Games/GameIdentity.h"

#include <array>

namespace PokeVault::Games {
    namespace {
        using enum Platform;
        using enum SourceSupport;

        constexpr std::array<GameDescriptor, 42> kGames{{
            {"red_gb",                       "Red",               GameBoy,        1, 0, ReadOnly},
            {"blue_gb",                      "Blue",              GameBoy,        1, 0, ReadOnly},
            {"yellow_gb",                    "Yellow",            GameBoy,        1, 0, ReadOnly},
            {"gold_gbc",                     "Gold",              GameBoyColor,   2, 0, ReadOnly},
            {"silver_gbc",                   "Silver",            GameBoyColor,   2, 0, ReadOnly},
            {"crystal_gbc",                  "Crystal",           GameBoyColor,   2, 0, ReadOnly},
            {"ruby_gba",                     "Ruby",              GameBoyAdvance, 3, 0, ReadOnly},
            {"sapphire_gba",                 "Sapphire",          GameBoyAdvance, 3, 0, ReadOnly},
            {"emerald_gba",                  "Emerald",           GameBoyAdvance, 3, 0, ReadOnly},
            {"firered_gba",                  "FireRed",           GameBoyAdvance, 3, 0, ReadOnly},
            {"leafgreen_gba",                "LeafGreen",         GameBoyAdvance, 3, 0, ReadOnly},
            // G4-02 exposes these through the persistent game-card assignment flow.
            // Support remains strictly read-only: no Gen IV editor, source writes or True Move.
            {"diamond_nds",                   "Diamond",           NintendoDS,     4, 0, ReadOnly},
            {"pearl_nds",                     "Pearl",             NintendoDS,     4, 0, ReadOnly},
            {"platinum_nds",                  "Platinum",          NintendoDS,     4, 0, ReadOnly},
            {"heartgold_nds",                 "HeartGold",         NintendoDS,     4, 0, ReadOnly},
            {"soulsilver_nds",                "SoulSilver",        NintendoDS,     4, 0, ReadOnly},
            // G5-01: exact game identities only. Planned keeps all user-facing
            // open/edit/assignment routes disabled until provider and UI validation.
            {"black_nds",                     "Black",             NintendoDS,     5, 0, Planned},
            {"white_nds",                     "White",             NintendoDS,     5, 0, Planned},
            {"black2_nds",                    "Black 2",           NintendoDS,     5, 0, Planned},
            {"white2_nds",                    "White 2",           NintendoDS,     5, 0, Planned},
            {"firered_switch",               "FireRed",           NintendoSwitch, 3, 0x0100554023408000ULL, NativeSwitch},
            {"leafgreen_switch",             "LeafGreen",         NintendoSwitch, 3, 0x010034D02340E000ULL, NativeSwitch},
            // Q04 (independent QA 2026-10-27): every localized FireRed/LeafGreen Switch release is its
            // own title, so each keeps its own identity and backup namespace (exactGameBackupsRoot keys on
            // the id). Title ids: upstream PKSE include/Enums/GameVersion.h:163-174 at a79fe61a. The
            // English pair above keeps its original id so existing backups stay where they are.
            {"firered_switch_jpn",           "FireRed (JPN)",     NintendoSwitch, 3, 0x01006FA0233F8000ULL, NativeSwitch},
            {"firered_switch_fra",           "FireRed (FRA)",     NintendoSwitch, 3, 0x01004B3023412000ULL, NativeSwitch},
            {"firered_switch_ita",           "FireRed (ITA)",     NintendoSwitch, 3, 0x010092302342A000ULL, NativeSwitch},
            {"firered_switch_deu",           "FireRed (DEU)",     NintendoSwitch, 3, 0x01007F8023416000ULL, NativeSwitch},
            {"firered_switch_esp",           "FireRed (ESP)",     NintendoSwitch, 3, 0x0100EB702342C000ULL, NativeSwitch},
            {"leafgreen_switch_jpn",         "LeafGreen (JPN)",   NintendoSwitch, 3, 0x0100F1E0233FA000ULL, NativeSwitch},
            {"leafgreen_switch_fra",         "LeafGreen (FRA)",   NintendoSwitch, 3, 0x010087C02342E000ULL, NativeSwitch},
            {"leafgreen_switch_ita",         "LeafGreen (ITA)",   NintendoSwitch, 3, 0x01005C7023432000ULL, NativeSwitch},
            {"leafgreen_switch_deu",         "LeafGreen (DEU)",   NintendoSwitch, 3, 0x0100FD6023430000ULL, NativeSwitch},
            {"leafgreen_switch_esp",         "LeafGreen (ESP)",   NintendoSwitch, 3, 0x01002B5023434000ULL, NativeSwitch},
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

    std::span<const GameDescriptor> allGameDescriptors() noexcept { return kGames; }

    const GameDescriptor* findGame(std::string_view id) noexcept {
        for (const auto& game : kGames) if (game.id == id) return &game;
        return nullptr;
    }

    const GameDescriptor* findSwitchGame(uint64_t titleId) noexcept {
        if (titleId == 0) return nullptr;
        for (const auto& game : kGames)
            if (game.switchTitleId == titleId) return &game;
        return nullptr;
    }

    std::string_view switchReleaseFamily(std::string_view id) noexcept {
        if (id.rfind("firered_switch", 0) == 0 && findGame(id)) return "firered_switch";
        if (id.rfind("leafgreen_switch", 0) == 0 && findGame(id)) return "leafgreen_switch";
        return id;
    }

    uint8_t switchReleaseLanguage(std::string_view id) noexcept {
        // PKHeX LanguageID values, as upstream PKSE's title table records them.
        if (switchReleaseFamily(id) == id && id != "firered_switch" && id != "leafgreen_switch") return 0;
        const std::string_view suffix = id.substr(id.find_last_of('_') + 1);
        if (suffix == "jpn") return 1;
        if (suffix == "fra") return 3;
        if (suffix == "ita") return 4;
        if (suffix == "deu") return 5;
        if (suffix == "esp") return 7;
        return 2;  // the original English releases
    }

    std::string_view platformName(Platform platform) noexcept {
        switch (platform) {
            case Platform::GameBoy: return "Game Boy";
            case Platform::GameBoyColor: return "Game Boy Color";
            case Platform::GameBoyAdvance: return "Game Boy Advance";
            case Platform::NintendoDS: return "Nintendo DS";
            case Platform::NintendoSwitch: return "Nintendo Switch";
        }
        return "Unknown platform";
    }

    std::string_view legacyPlatformAbbreviation(std::string_view id) noexcept {
        const GameDescriptor* game = findGame(id);
        if (!game || game->support != SourceSupport::ReadOnly) return {};
        switch (game->platform) {
            case Platform::GameBoy: return "GB";
            case Platform::GameBoyColor: return "GBC";
            case Platform::GameBoyAdvance: return "GBA";
            case Platform::NintendoDS: return "NDS";
            case Platform::NintendoSwitch: return {};
        }
        return {};
    }

    std::string_view gameCardArtworkPath(std::string_view id) noexcept {
        if (id == "red_gb") return "romfs:/game_cards/red_gb.png";
        if (id == "blue_gb") return "romfs:/game_cards/blue_gb.png";
        if (id == "yellow_gb") return "romfs:/game_cards/yellow_gb.png";
        if (id == "gold_gbc") return "romfs:/game_cards/gold_gbc.png";
        if (id == "silver_gbc") return "romfs:/game_cards/silver_gbc.png";
        if (id == "crystal_gbc") return "romfs:/game_cards/crystal_gbc.png";
        if (id == "ruby_gba") return "romfs:/game_cards/ruby_gba.png";
        if (id == "sapphire_gba") return "romfs:/game_cards/sapphire_gba.png";
        if (id == "emerald_gba") return "romfs:/game_cards/emerald_gba.png";
        if (id == "firered_gba") return "romfs:/game_cards/firered_gba.png";
        if (id == "leafgreen_gba") return "romfs:/game_cards/leafgreen_gba.png";
        if (id == "diamond_nds") return "romfs:/game_cards/diamond_nds.png";
        if (id == "pearl_nds") return "romfs:/game_cards/pearl_nds.png";
        if (id == "platinum_nds") return "romfs:/game_cards/platinum_nds.png";
        if (id == "heartgold_nds") return "romfs:/game_cards/heartgold_nds.png";
        if (id == "soulsilver_nds") return "romfs:/game_cards/soulsilver_nds.png";
        if (id == "black_nds") return "romfs:/game_cards/black_nds.png";
        if (id == "white_nds") return "romfs:/game_cards/white_nds.png";
        if (id == "black2_nds") return "romfs:/game_cards/black2_nds.png";
        if (id == "white2_nds") return "romfs:/game_cards/white2_nds.png";
        return {};
    }

    std::string_view gameRegionBackdropKey(std::string_view exactId) noexcept {
        const std::string_view id = switchReleaseFamily(exactId);
        if (id == "red_gb" || id == "blue_gb" || id == "yellow_gb" ||
            id == "firered_gba" || id == "leafgreen_gba" ||
            id == "firered_switch" || id == "leafgreen_switch" ||
            id == "letsgo_pikachu_switch" || id == "letsgo_eevee_switch")
            return "kanto";

        if (id == "gold_gbc" || id == "silver_gbc" || id == "crystal_gbc" ||
            id == "heartgold_nds" || id == "soulsilver_nds")
            return "johto";

        if (id == "ruby_gba" || id == "sapphire_gba" || id == "emerald_gba")
            return "hoenn";

        if (id == "diamond_nds" || id == "pearl_nds" || id == "platinum_nds" ||
            id == "brilliant_diamond_switch" || id == "shining_pearl_switch")
            return "sinnoh";

        if (id == "black_nds" || id == "white_nds" ||
            id == "black2_nds" || id == "white2_nds") return "unova";

        if (id == "legends_arceus_switch") return "hisui";
        if (id == "sword_switch" || id == "shield_switch") return "galar";
        if (id == "scarlet_switch" || id == "violet_switch") return "paldea";
        if (id == "legends_za_switch") return "kalos";
        return {};
    }
}
