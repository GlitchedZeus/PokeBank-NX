#ifndef UI_CLASSIC_GAME_CONTEXT_H
#define UI_CLASSIC_GAME_CONTEXT_H

#include <string>
#include <string_view>

namespace PokeBank::UIModel::ClassicGameContext {

enum class SurfaceMode {
    ViewReadOnly,
    EditStaged,
    CreateDraft,
};

inline std::string_view exactGameName(std::string_view sourceGameId) noexcept {
    if (sourceGameId == "red_gb" || sourceGameId == "red" || sourceGameId == "Red") return "Red";
    if (sourceGameId == "blue_gb" || sourceGameId == "blue" || sourceGameId == "Blue") return "Blue";
    if (sourceGameId == "yellow_gb" || sourceGameId == "yellow" || sourceGameId == "Yellow") return "Yellow";
    if (sourceGameId == "gold_gbc" || sourceGameId == "gold" || sourceGameId == "Gold") return "Gold";
    if (sourceGameId == "silver_gbc" || sourceGameId == "silver" || sourceGameId == "Silver") return "Silver";
    if (sourceGameId == "crystal_gbc" || sourceGameId == "crystal" || sourceGameId == "Crystal") return "Crystal";
    if (sourceGameId == "ruby_gba") return "Ruby";
    if (sourceGameId == "sapphire_gba") return "Sapphire";
    if (sourceGameId == "emerald_gba") return "Emerald";
    if (sourceGameId == "firered_gba") return "FireRed";
    if (sourceGameId == "leafgreen_gba") return "LeafGreen";
    return {};
}

inline std::string_view modeLabel(SurfaceMode mode) noexcept {
    switch (mode) {
        case SurfaceMode::ViewReadOnly: return "READ ONLY";
        case SurfaceMode::EditStaged: return "STAGED";
        case SurfaceMode::CreateDraft: return "DRAFT";
    }
    return {};
}

inline std::string contextLine(const std::string& recordContext, std::string_view sourceGameId,
                               SurfaceMode mode) {
    std::string line = recordContext;
    const auto game = exactGameName(sourceGameId);
    if (!game.empty()) {
        line += " • ";
        line += game;
    }
    line += " • Source save immutable • ";
    line += modeLabel(mode);
    return line;
}

// RBY/GSC store their native type bytes using the classic numbering, while the
// canonical ROMFS type sprites use the normalized 0-17 numbering.
inline int normalizedTypeSpriteId(unsigned char classicType) noexcept {
    switch (classicType) {
        case 0: return 0;   // Normal
        case 1: return 1;   // Fighting
        case 2: return 2;   // Flying
        case 3: return 3;   // Poison
        case 4: return 4;   // Ground
        case 5: return 5;   // Rock
        case 7: return 6;   // Bug
        case 8: return 7;   // Ghost
        case 9: return 8;   // Steel (Gen II)
        case 20: return 9;  // Fire
        case 21: return 10; // Water
        case 22: return 11; // Grass
        case 23: return 12; // Electric
        case 24: return 13; // Psychic
        case 25: return 14; // Ice
        case 26: return 15; // Dragon
        case 27: return 16; // Dark (Gen II)
        default: return -1;
    }
}

inline bool generationHasNature(int generation) noexcept {
    return generation >= 3;
}

} // namespace PokeBank::UIModel::ClassicGameContext

#endif
