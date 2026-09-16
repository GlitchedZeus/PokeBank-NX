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

    // Gen II already has a neutral-heading correction layer. The final parity
    // route must apply it after drawing an active Create/Edit workspace.
    assert(gen2Fix.find("clearHardwarePanelHeading(fb, leftX, contentY, 120, \"DETAILS\"") != std::string::npos);
    assert(gen2Fix.find("clearHardwarePanelHeading(fb, midX, contentY, 120, \"STATS\"") != std::string::npos);
    assert(gen2Fix.find("clearHardwarePanelHeading(fb, rightX, contentY, 120, \"MOVES\"") != std::string::npos);
    assert(gen2Fix.find("fb.drawText(x + 14, y + 10, text, Colors::Text, style)") != std::string::npos);
    assert(gen2Final.find("drawHardwareWorkspaceCorrections(screen, fb);") != std::string::npos);

    std::cout << "Shared editor headings: neutral DETAILS/STATS/MOVES contract PASS\n";
    return 0;
}
