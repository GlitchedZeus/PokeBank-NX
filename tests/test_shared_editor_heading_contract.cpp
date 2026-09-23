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

std::size_t countOccurrences(const std::string& text, const std::string& needle) {
    std::size_t count = 0;
    std::size_t pos = 0;
    while ((pos = text.find(needle, pos)) != std::string::npos) {
        ++count;
        pos += needle.size();
    }
    return count;
}

} // namespace

int main() {
    namespace Foundation = PokeBank::UIModel::PokemonEditorFoundation;
    namespace Classic = PokeBank::UIModel::ClassicGameContext;

    const auto gen1Fix = readText("src/UI/Gen1PokemonEditorFoundationHardwareFix.inc");
    const auto gen1View = readText("src/UI/Gen1PokemonEditorPassiveView.inc");
    const auto gen1Passive = readText("src/UI/Gen1PokemonDetailsPresentation.cpp");
    const auto gen2Fix = readText("src/UI/Gen2HardwareWorkspaceFix.inc");
    const auto gen2Final = readText("src/UI/Gen2HardwareFinalFix.inc");
    const auto gen2Data = readText("include/UI/Gen2WorkspacePresentation.h");
    const auto sharedPicker = readText("include/UI/SharedSpeciesPicker.h");
    const auto sharedTypes = readText("include/UI/ClassicTypeBadges.h");

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

    // Generation labels stay exact on the two classic presentation families.
    assert(gen1Fix.find("\"GEN I DATA\"") != std::string::npos);
    assert(gen1Passive.find("\"GEN I DATA\"") != std::string::npos);
    assert(gen2Data.find("\"GEN II DATA\"") != std::string::npos);
    assert(gen2Data.find("\"GEN I DATA\"") == std::string::npos);

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

    // Mature Party/Box passive View now owns the same true fullscreen language rather than
    // the old outer workspace card, and includes exact source identity when available.
    assert(gen1Passive.find("fb.drawText(28, 16, name") != std::string::npos);
    assert(gen1Passive.find("passiveContextLine(p)") != std::string::npos);
    assert(gen1Passive.find("drawPanelSurface(fb, workspaceX") == std::string::npos);
    assert(gen1Passive.find("\"STATS\", Colors::Text") != std::string::npos);
    assert(gen1Passive.find("\"VALUES\"") == std::string::npos);

    // Headings are neutral; the focus cursor is a separate red/theme focus border.
    assert(gen1Fix.find("\"DETAILS\", Colors::Text") != std::string::npos);
    assert(gen1Fix.find("\"STATS\", Colors::Text") != std::string::npos);
    assert(gen1Fix.find("\"MOVES\", Colors::Text") != std::string::npos);
    assert(gen1Fix.find("Colors::FocusBorder, 2") != std::string::npos);
    assert(gen1Passive.find("\"DETAILS\", Colors::Text") != std::string::npos);
    assert(gen1Passive.find("\"MOVES\", Colors::Text") != std::string::npos);

    // Canonical type badge helper is shared by mature passive View and the shared RBY/GSC picker.
    assert(sharedTypes.find("SpriteManager::getTypeSprite") != std::string::npos);
    assert(sharedTypes.find("normalizedTypeSpriteId") != std::string::npos);
    assert(gen1Passive.find("ClassicTypeBadges::drawPairCentered") != std::string::npos);
    assert(sharedPicker.find("ClassicTypeBadges::drawPairCentered") != std::string::npos);
    assert(sharedPicker.find("speciesCount == 151") != std::string::npos);
    assert(sharedPicker.find("speciesCount == 251") != std::string::npos);
    assert(sharedPicker.find("StagedPokemonEditor::personalTypes") != std::string::npos);
    assert(sharedPicker.find("Gen2::personalRecord") != std::string::npos);
    assert(gen1Fix.find("SpriteManager::getTypeSprite") != std::string::npos);
    assert(gen2Final.find("SpriteManager::getTypeSprite") != std::string::npos);

    // Calculated Gen II stats are neutral; stored DV/Stat Exp stay bright and derived HP DV stays dim.
    assert(gen1Fix.find("const Color valueColor = (r == 0 && c == 0) ? Colors::TextDim") != std::string::npos);
    assert(gen1Passive.find("p.hasBattleStats ? std::to_string(p.battleStats") != std::string::npos);
    assert(gen1Passive.find("Colors::Accent, TextStyle::Caption") != std::string::npos);
    assert(gen2Final.find("std::to_string(p.statExperience") != std::string::npos);
    assert(gen2Final.find("stat, Colors::TextSecondary, TextStyle::Caption") != std::string::npos);
    assert(gen2Final.find("stat, Colors::Accent, TextStyle::Caption") == std::string::npos);

    // Move status owns a far-right region independent of PP/Ups; empty rows skip PP/Ups.
    assert(gen1Passive.find("moveStatusRightPad") != std::string::npos);
    assert(gen1Passive.find("ppRightOffset") != std::string::npos);
    assert(gen1Passive.find("if (move != 0)") != std::string::npos);
    assert(gen2Final.find("const int ppX = x + w - 226") != std::string::npos);
    assert(gen2Final.find("x + w - 18 - statusW") != std::string::npos);
    assert(gen2Final.find("fb.drawText(x + 292, yy") == std::string::npos);

    // Classic derivation/storage notes have dedicated baselines instead of one long overlapping line.
    assert(gen1Passive.find("* HP DV derived / read-only") != std::string::npos);
    assert(gen1Passive.find("* one stored Gen I Special stat; split display only") != std::string::npos);
    assert(gen2Final.find("* HP DV derived / read-only") != std::string::npos);
    assert(gen2Final.find("One Gen II Special DV / Stat Exp feeds both") != std::string::npos);
    assert(gen2Final.find("* HP DV derived / read-only • one stored Special DV / Stat Exp") == std::string::npos);
    assert(gen2Final.find("const int shinyY = y + 366") != std::string::npos);
    assert(gen2Final.find("const int genderY = y + 408") != std::string::npos);

    // Generation II gender semantics: one final row survives the legacy underpaint, male uses
    // a readable cyan-leaning blue, female stays magenta, and genderless stays neutral.
    assert(gen2Final.find("Color gen2MaleColor() noexcept") != std::string::npos);
    assert(gen2Final.find("Color(92, 205, 255)") != std::string::npos);
    assert(gen2Final.find("Color(24, 112, 184)") != std::string::npos);
    assert(gen2Final.find("Colors::Blue") == std::string::npos);
    assert(gen2Final.find("Colors::Magenta") != std::string::npos);
    assert(gen2Final.find("return Colors::Text;") != std::string::npos);
    assert(gen2Final.find("const int semanticClearBottom = genderY + 30") != std::string::npos);
    assert(gen2Final.find("semanticClearBottom - clearTop") != std::string::npos);
    assert(countOccurrences(gen2Final, "fb.drawText(x + 18, genderY, \"Gender\"") == 1);
    assert(countOccurrences(gen2Final, "genderText(p.gender), genderSelected ? Colors::SelectedText : gen2GenderColor(p.gender)") == 1);

    // Generation II has a fullscreen final repaint for both active shared modes and external passive View.
    assert(gen2Final.find("drawFullscreenGen2Active") != std::string::npos);
    assert(gen2Final.find("drawFullscreenGen2Passive") != std::string::npos);
    const auto shell = readText("include/UI/SharedPokemonShell.h");
    assert(gen2Final.find("SharedPokemonShell::drawChrome(fb, p.species, p.nickname, p.level, p.gender, p.shiny") != std::string::npos);
    assert(shell.find("fb.drawText(28, 16, name") != std::string::npos);
    assert(shell.find("No. \" + dexLabel(species)") != std::string::npos);

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
    assert(hpStatExpLeft.panel == Foundation::Panel::Values);
    assert(hpStatExpLeft.row == static_cast<uint8_t>(Foundation::ValueRow::Attack));
    assert(hpStatExpLeft.column == static_cast<uint8_t>(Foundation::ValueColumn::DV));

    // Gen II final input normalization keeps derived HP DV visible but unreachable by focus.
    assert(gen2Fix.find("normalizeHardwareDerivedHpDvFocus") != std::string::npos);
    assert(gen2Fix.find("focus.panel != Unified::Panel::Values || focus.row != 0 || focus.column != 0") != std::string::npos);
    assert(gen2Fix.find("normalizeHardwareDerivedHpDvFocus(screen, &previous);") != std::string::npos);
    assert(gen2Fix.find("focus.row = 1;") != std::string::npos);
    assert(gen2Fix.find("focus.column = 0;") != std::string::npos);
    assert(gen2Fix.find("focus.panel = Unified::Panel::Details") == std::string::npos);
    assert(gen2Final.find("r == 0 ? Colors::TextDim : Colors::Text") != std::string::npos);

    // Final hardware layers must never present the stale workspace label.
    assert(gen1Fix.find("PKSE three-panel workspace") == std::string::npos);
    assert(gen1View.find("PKSE three-panel workspace") == std::string::npos);
    assert(gen1Passive.find("PKSE three-panel workspace") == std::string::npos);
    assert(gen2Final.find("PKSE three-panel workspace") == std::string::npos);

    std::cout << "Shared Gen I/II fullscreen presentation and hardware regression contract PASS\n";
    return 0;
}
