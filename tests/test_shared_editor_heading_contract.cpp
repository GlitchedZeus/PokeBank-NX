#include "UI/PokemonEditorFoundationContract.h"
#include "UI/ClassicGameContext.h"

#include <array>
#include <cassert>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

namespace {

std::string readText(const char* path) {
    std::ifstream input(path);
    assert(input && "expected source file missing");
    std::ostringstream text;
    text << input.rdbuf();
    return text.str();
}

} // namespace

int main() {
    namespace Foundation = PokeBank::UIModel::PokemonEditorFoundation;
    namespace Classic = PokeBank::UIModel::ClassicGameContext;

    const auto gen1Fix = readText("src/UI/Gen1PokemonEditorFoundationHardwareFix.inc");
    const auto gen1View = readText("src/UI/Gen1PokemonEditorPassiveView.inc");
    const auto gen2Fix = readText("src/UI/Gen2HardwareWorkspaceFix.inc");
    const auto gen2Final = readText("src/UI/Gen2HardwareFinalFix.inc");

    // All six exact identities are one shared contract across View/Edit/Create.
    const std::array<std::pair<const char*, const char*>, 6> games{{
        {"red_gb", "Red"}, {"blue_gb", "Blue"}, {"yellow_gb", "Yellow"},
        {"gold_gbc", "Gold"}, {"silver_gbc", "Silver"}, {"crystal_gbc", "Crystal"},
    }};
    const std::array<Classic::SurfaceMode, 3> modes{{
        Classic::SurfaceMode::ViewReadOnly,
        Classic::SurfaceMode::EditStaged,
        Classic::SurfaceMode::CreateDraft,
    }};
    for (const auto& [id, expected] : games) {
        assert(Classic::exactGameName(id) == expected);
        for (const auto mode : modes) {
            const auto context = Classic::contextLine("Box 1 / Slot 1", id, mode);
            assert(context.find(expected) != std::string::npos);
            assert(context.find("Source save immutable") != std::string::npos);
            assert(context.find(Classic::modeLabel(mode)) != std::string::npos);
        }
    }
    const auto yellow = Classic::contextLine("Box 1 / Slot 1", "yellow_gb", Classic::SurfaceMode::ViewReadOnly);
    assert(yellow.find("Box 1 / Slot 1 • Yellow • Source save immutable • READ ONLY") != std::string::npos);

    // Nature remains absent from Gen I/II; the shared convention only begins with Gen III.
    assert(!Classic::generationHasNature(1));
    assert(!Classic::generationHasNature(2));
    assert(Classic::generationHasNature(3));

    // Classic native type bytes are normalized into the canonical 18 ROMFS badge set.
    assert(Classic::normalizedTypeSpriteId(0) == 0);   // Normal
    assert(Classic::normalizedTypeSpriteId(7) == 6);   // Bug
    assert(Classic::normalizedTypeSpriteId(8) == 7);   // Ghost
    assert(Classic::normalizedTypeSpriteId(9) == 8);   // Steel
    assert(Classic::normalizedTypeSpriteId(20) == 9);  // Fire
    assert(Classic::normalizedTypeSpriteId(21) == 10); // Water
    assert(Classic::normalizedTypeSpriteId(22) == 11); // Grass
    assert(Classic::normalizedTypeSpriteId(23) == 12); // Electric
    assert(Classic::normalizedTypeSpriteId(27) == 16); // Dark

    // Gen I View/Edit/Create are distinct modes but share a true page-owned fullscreen renderer.
    assert(gen1Fix.find("drawFullscreenGen1Workspace") != std::string::npos);
    assert(gen1Fix.find("fb.drawVerticalGradient(0, 0, fb.getWidth(), fb.getHeight()") != std::string::npos);
    assert(gen1Fix.find("drawPanelSurface(fb, leftX, contentY") != std::string::npos);
    assert(gen1Fix.find("drawPanelSurface(fb, midX, contentY") != std::string::npos);
    assert(gen1Fix.find("drawPanelSurface(fb, rightX, contentY") != std::string::npos);
    assert(gen1Fix.find("fb.drawText(28, 16, name") != std::string::npos);
    assert(gen1Fix.find("No. \" + foundationDexLabel(species)") != std::string::npos);
    assert(gen1View.find("drawFullscreenGen1Workspace(screen, fb);") != std::string::npos);
    assert(gen1View.find("drawGen1PokemonDetailsPresentation") == std::string::npos);

    // Headings are neutral; the focus cursor is a separate red/theme focus border.
    assert(gen1Fix.find("\"DETAILS\", Colors::Text") != std::string::npos);
    assert(gen1Fix.find("\"STATS\", Colors::Text") != std::string::npos);
    assert(gen1Fix.find("\"MOVES\", Colors::Text") != std::string::npos);
    assert(gen1Fix.find("Colors::FocusBorder, 2") != std::string::npos);

    // Type badges use the canonical SpriteManager asset path, never a guessed color palette.
    assert(gen1Fix.find("SpriteManager::getTypeSprite") != std::string::npos);
    assert(gen2Final.find("SpriteManager::getTypeSprite") != std::string::npos);

    // Calculated Stat is semantic accent; DV and Stat Exp are redrawn neutral.
    assert(gen1Fix.find("const Color valueColor = c == 2 ? Colors::Accent : Colors::Text") != std::string::npos);
    assert(gen2Final.find("Colors::Accent, TextStyle::Caption") != std::string::npos);
    assert(gen2Final.find("std::to_string(p.statExperience") != std::string::npos);

    // Generation II restores semantic gender colors and has a fullscreen final repaint for both
    // active shared modes and the external passive View route.
    assert(gen2Final.find("Colors::Blue") != std::string::npos);
    assert(gen2Final.find("Colors::Magenta") != std::string::npos);
    assert(gen2Final.find("drawFullscreenGen2Active") != std::string::npos);
    assert(gen2Final.find("drawFullscreenGen2Passive") != std::string::npos);
    assert(gen2Final.find("fb.drawText(28, 16, name") != std::string::npos);
    assert(gen2Final.find("No. \" + gen2DexLabel(p.species)") != std::string::npos);

    // Gen I HP DV is derived and must never be a focus/edit target.
    assert(Foundation::hpDVIsDerived());
    assert(!Foundation::valueCellEditable(Foundation::ValueRow::HP, Foundation::ValueColumn::DV));
    const auto normalizedHp = Foundation::normalize({Foundation::Panel::Values, 0, 0});
    assert(normalizedHp.panel == Foundation::Panel::Values);
    assert(normalizedHp.row == 0);
    assert(normalizedHp.column == static_cast<uint8_t>(Foundation::ValueColumn::StatExperience));
    const auto attackUp = Foundation::moveFocus(
        {Foundation::Panel::Values, static_cast<uint8_t>(Foundation::ValueRow::Attack),
         static_cast<uint8_t>(Foundation::ValueColumn::DV)}, Foundation::Direction::Up);
    assert(!Foundation::derivedHpDvFocus(attackUp));
    const auto hpStatExpLeft = Foundation::moveFocus(
        {Foundation::Panel::Values, 0, static_cast<uint8_t>(Foundation::ValueColumn::StatExperience)},
        Foundation::Direction::Left);
    assert(hpStatExpLeft.panel == Foundation::Panel::Identity);

    // Gen II final input normalization keeps derived HP DV visible but unreachable by focus.
    assert(gen2Fix.find("normalizeHardwareDerivedHpDvFocus") != std::string::npos);
    assert(gen2Fix.find("focus.panel != Unified::Panel::Values || focus.row != 0 || focus.column != 0") != std::string::npos);
    assert(gen2Fix.find("normalizeHardwareDerivedHpDvFocus(screen, &previous);") != std::string::npos);

    // The final hardware layers must never present the stale workspace label.
    assert(gen1Fix.find("PKSE three-panel workspace") == std::string::npos);
    assert(gen1View.find("PKSE three-panel workspace") == std::string::npos);
    assert(gen2Final.find("PKSE three-panel workspace") == std::string::npos);

    std::cout << "Shared Gen I/II fullscreen presentation and exact identity contract PASS\n";
    return 0;
}
