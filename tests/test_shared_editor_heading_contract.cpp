#include "UI/PokemonEditorFoundationContract.h"

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

    const auto gen1Fix = readText("src/UI/Gen1PokemonEditorFoundationHardwareFix.inc");
    const auto gen1View = readText("src/UI/Gen1PokemonEditorPassiveView.inc");
    const auto gen2Fix = readText("src/UI/Gen2HardwareWorkspaceFix.inc");
    const auto gen2Final = readText("src/UI/Gen2HardwareFinalFix.inc");

    // Gen I Create/Edit: DETAILS / STATS / MOVES are always neutral. The old
    // VALUES label is visually replaced by STATS in the final hardware layer.
    assert(gen1Fix.find("redraw(leftX, \"DETAILS\", TextStyle::Caption)") != std::string::npos);
    assert(gen1Fix.find("redraw(midX, \"STATS\", TextStyle::Heading)") != std::string::npos);
    assert(gen1Fix.find("redraw(rightX, \"MOVES\", TextStyle::Heading)") != std::string::npos);
    assert(gen1Fix.find("fb.drawText(x + 14, contentY + 10, text, Colors::Text, style)") != std::string::npos);

    // Gen I read-only View receives the same final neutral-heading pass.
    assert(gen1View.find("drawNeutralGen1WorkspaceHeadings(fb);") != std::string::npos);

    // The final Gen I hardware layer replaces the stale internal workspace copy
    // with the exact source game label while keeping the immutable-source message.
    assert(gen1Fix.find("drawExactGen1WorkspaceSubtitle") != std::string::npos);
    assert(gen1Fix.find("case SourceGame::Red: return \"Red\"") != std::string::npos);
    assert(gen1Fix.find("case SourceGame::Blue: return \"Blue\"") != std::string::npos);
    assert(gen1Fix.find("case SourceGame::Yellow: return \"Yellow\"") != std::string::npos);
    assert(gen1Fix.find("foundationGen1GameShort(e->metadata().sourceGame)") != std::string::npos);
    assert(gen1Fix.find("Source save immutable") != std::string::npos);
    assert(gen1Fix.find("drawExactGen1WorkspaceSubtitle(screen, fb);") != std::string::npos);

    // Gen I HP DV is derived and must never be a focus/edit target. Normalization,
    // horizontal entry and vertical DV navigation all skip the derived cell.
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
    const auto levelDown = Foundation::moveFocus(
        {Foundation::Panel::Values, static_cast<uint8_t>(Foundation::ValueRow::Level),
         static_cast<uint8_t>(Foundation::ValueColumn::DV)}, Foundation::Direction::Down);
    assert(!Foundation::derivedHpDvFocus(levelDown));
    const auto hpStatExpLeft = Foundation::moveFocus(
        {Foundation::Panel::Values, 0, static_cast<uint8_t>(Foundation::ValueColumn::StatExperience)},
        Foundation::Direction::Left);
    assert(hpStatExpLeft.panel == Foundation::Panel::Identity);

    // Gen II already has a neutral-heading correction layer. The final parity
    // route must apply it after drawing an active Create/Edit workspace.
    assert(gen2Fix.find("clearHardwarePanelHeading(fb, leftX, contentY, 120, \"DETAILS\"") != std::string::npos);
    assert(gen2Fix.find("clearHardwarePanelHeading(fb, midX, contentY, 120, \"STATS\"") != std::string::npos);
    assert(gen2Fix.find("clearHardwarePanelHeading(fb, rightX, contentY, 120, \"MOVES\"") != std::string::npos);
    assert(gen2Fix.find("fb.drawText(x + 14, y + 10, text, Colors::Text, style)") != std::string::npos);
    assert(gen2Final.find("drawHardwareWorkspaceCorrections(screen, fb);") != std::string::npos);

    // Gen II final input/draw normalization keeps the derived HP DV visible but
    // unreachable by focus, including stale focus before A-button handling.
    assert(gen2Fix.find("normalizeHardwareDerivedHpDvFocus") != std::string::npos);
    assert(gen2Fix.find("focus.panel != Unified::Panel::Values || focus.row != 0 || focus.column != 0") != std::string::npos);
    assert(gen2Fix.find("normalizeHardwareDerivedHpDvFocus(screen);") != std::string::npos);
    assert(gen2Fix.find("normalizeHardwareDerivedHpDvFocus(screen, &previous);") != std::string::npos);

    std::cout << "Shared editor final polish: headings, exact game subtitle and derived HP DV focus PASS\n";
    return 0;
}
