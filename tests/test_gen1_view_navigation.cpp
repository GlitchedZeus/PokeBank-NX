// Divergence item 3: Gen I View is navigable and focusable, read-only, like Gen II-V View.
//  - the pure foundation focus model reaches every panel from View exactly as from Edit;
//  - the live View handler applies D-pad/stick/L/R and tap focus but never activates (no A path,
//    no staging call), and B / the Back target still leaves;
//  - the fullscreen workspace draws focus outlines and field targets in View too.
#include "UI/PokemonEditorFoundationContract.h"

#include <cassert>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

namespace Foundation = PokeBank::UIModel::PokemonEditorFoundation;

namespace {
std::string read(const char* path) {
    std::ifstream in(path, std::ios::binary);
    assert(in);
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}
std::string between(const std::string& text, const char* begin, const char* end) {
    const auto a = text.find(begin);
    if (a == std::string::npos) std::cerr << "Missing route start: " << begin << '\n';
    assert(a != std::string::npos);
    const auto b = text.find(end, a + 1);
    assert(b != std::string::npos);
    return text.substr(a, b - a);
}
void has(const std::string& t, const char* n) {
    if (t.find(n) == std::string::npos) std::cerr << "Missing: " << n << '\n';
    assert(t.find(n) != std::string::npos);
}
void lacks(const std::string& t, const char* n) {
    if (t.find(n) != std::string::npos) std::cerr << "Forbidden: " << n << '\n';
    assert(t.find(n) == std::string::npos);
}
} // namespace

int main() {
    // The same pure navigation the View handler uses reaches all three panels.
    Foundation::Focus focus{Foundation::Panel::Identity, 0, 0};
    focus = Foundation::nextPanel(focus);
    assert(focus.panel == Foundation::Panel::Values);
    focus = Foundation::nextPanel(focus);
    assert(focus.panel == Foundation::Panel::Moves);
    focus = Foundation::nextPanel(focus);
    assert(focus.panel == Foundation::Panel::Identity);
    const auto down = Foundation::moveFocus({Foundation::Panel::Identity, 0, 0}, Foundation::Direction::Down);
    assert(down.panel == Foundation::Panel::Identity && down.row == 1);

    const auto view = read("src/UI/Gen1PokemonEditorPassiveView.inc");
    const auto handler = between(view, "bool handleFoundationPassiveViewInput(TrainerViewScreen& screen, uint64_t down, int touchTarget,",
                                 "bool handleFoundationPassiveViewInput(TrainerViewScreen& screen, uint64_t down,\n");
    has(handler, "foundation.navigation.apply(");
    has(handler, "Foundation::moveFocus(foundation.focus, Foundation::Direction::Up)");
    has(handler, "Foundation::moveFocus(foundation.focus, Foundation::Direction::Right)");
    has(handler, "if (down & HidNpadButton_R) foundation.focus = Foundation::nextPanel(foundation.focus);");
    has(handler, "(void)foundationFocusTouchTarget(foundation, touchTarget);");
    has(handler, "PassiveViewAction::Back");
    // Read-only: no activation or staging from View.
    lacks(handler, "HidNpadButton_A");
    lacks(handler, "ux2CommitEdit");
    lacks(handler, "stage");

    // Edit/Create and View share one touch->focus mapping; only Edit/Create add A.
    const auto f = read("src/UI/Gen1PokemonEditorOverlayFoundation.inc");
    const auto map = between(f, "bool foundationFocusTouchTarget(FoundationState& foundation, int id) {",
                             "uint64_t foundationDirectTouchInput(");
    lacks(map, "HidNpadButton_A");
    has(f, "if (!foundationFocusTouchTarget(foundation, id)) return false;\n        if (activate) down |= HidNpadButton_A;");

    // The workspace draws focus and registers targets in every mode (no View gating).
    const auto fix = read("src/UI/Gen1PokemonEditorFoundationHardwareFix.inc");
    const auto draw = between(fix, "void drawFullscreenGen1Workspace(", "void drawFoundationPicker(");
    lacks(draw, "const bool editable = !view;");
    lacks(draw, "if (editable) screen.touchButtons");
    has(draw, "screen.touchButtons.push_back({5000 + i,");
    has(draw, "{{\"D-pad/Stick\", \"Navigate\"}, {\"L/R\", \"Panel\"}, {\"B\", \"Back\"}, {\"Touch\", \"Back\"}}");

    std::cout << "Gen I View: navigable, focusable, read-only: PASS\n";
}
