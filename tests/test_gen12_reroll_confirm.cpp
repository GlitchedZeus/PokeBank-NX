// Gen I / Gen II "Y Random DVs" asks before re-rolling (owner campaign directive): Y opens
// "Re-roll all DVs? Shiny/gender may change.", A or Y confirms, B cancels with nothing changed,
// and the prompt owns all input while open. Pure model transitions + both live routes.
#include "UI/Gen12RerollDVsConfirm.h"

#include <cassert>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

namespace Reroll = PokeBank::UIModel::Gen12RerollDVs;
using Outcome = Reroll::Outcome;

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
    if (b == std::string::npos) std::cerr << "Missing route end: " << end << '\n';
    assert(b != std::string::npos);
    return text.substr(a, b - a);
}
void has(const std::string& text, const char* needle) {
    if (text.find(needle) == std::string::npos) std::cerr << "Missing: " << needle << '\n';
    assert(text.find(needle) != std::string::npos);
}
void lacks(const std::string& text, const char* needle) {
    if (text.find(needle) != std::string::npos) std::cerr << "Forbidden: " << needle << '\n';
    assert(text.find(needle) == std::string::npos);
}
size_t count(const std::string& text, const char* needle) {
    size_t n = 0;
    for (auto at = text.find(needle); at != std::string::npos; at = text.find(needle, at + 1)) ++n;
    return n;
}

void model() {
    static_assert(std::char_traits<char>::length(Reroll::kTitle) > 0);
    assert(std::string(Reroll::kTitle) == "Re-roll all DVs?");
    assert(std::string(Reroll::kConsequence) == "Shiny/gender may change.");
    Reroll::State s;
    // Without a prompt only Y does anything; A/B belong to the workspace.
    assert(Reroll::step(s, false, true, false) == Outcome::NotHandled);
    assert(Reroll::step(s, false, false, true) == Outcome::NotHandled && !s.confirming);
    // Y asks; the same Y cannot also confirm.
    assert(Reroll::step(s, true, false, false) == Outcome::AskConfirm && s.confirming);
    // Navigation/X/other input is swallowed while open.
    assert(Reroll::step(s, false, false, false) == Outcome::Swallowed && s.confirming);
    // B cancels and wins over a simultaneous A.
    assert(Reroll::step(s, false, true, true) == Outcome::Cancelled && !s.confirming);
    // A confirms.
    assert(Reroll::step(s, true, false, false) == Outcome::AskConfirm);
    assert(Reroll::step(s, false, true, false) == Outcome::Reroll && !s.confirming);
    // Y confirms too.
    assert(Reroll::step(s, true, false, false) == Outcome::AskConfirm);
    assert(Reroll::step(s, true, false, false) == Outcome::Reroll && !s.confirming);
}

void gen2Route() {
    const auto unified = read("src/UI/Gen2UnifiedPokemonWorkspace.inc");
    // The one randomizeDVs call in the live editable route sits in the confirmed outcome, and
    // the prompt is resolved before any Details tap, X Add, B exit or A field activation.
    const auto editable = unified.substr(unified.find("if (handleUnifiedMoveEditor(screen, state, down, held, stickX, stickY)) return true;"));
    const auto confirmed = between(editable, "case Reroll::Outcome::Reroll:", "const int touchedDetail");
    has(confirmed, "randomizeDVs(state);");
    assert(count(editable.substr(0, editable.find("bool drawUnifiedWorkspace")), "randomizeDVs(state)") == 1);
    const auto step = editable.find("Reroll::step(unified.reroll,");
    assert(step != std::string::npos);
    assert(step < editable.find("const int touchedDetail = updateUnifiedDetailsTouch("));
    assert(step < editable.find("if ((down & HidNpadButton_X) && state.mode == Mode::Create)"));
    assert(step < editable.find("if (down & HidNpadButton_B) {"));
    has(editable, "if (tapped == kRerollCancelTouchId) down |= HidNpadButton_B;");
    // Unified touch focus/activation is inert while the prompt is open.
    const auto hw = read("src/UI/Gen2HardwareWorkspaceFix.inc");
    assert(count(hw, "unified.reroll.confirming") == 2);
    const auto final_ = read("src/UI/Gen2HardwareFinalFix.inc");
    has(final_, "if (unified.reroll.confirming) drawRerollConfirm(screen, fb);");
    has(final_, "} else if (unified.reroll.confirming) {\n        drawNavBar(fb, {{\"B\", \"Cancel\"}, {\"A\", \"Re-roll\"}});");
}

void gen1Route() {
    const auto f = read("src/UI/Gen1PokemonEditorOverlayFoundation.inc");
    const auto main = between(f, "if (foundation.editExitConfirm) {\n        const bool create", "std::string error;");
    const auto confirmed = between(main, "case Reroll::Outcome::Reroll:", "if (down & HidNpadButton_Up)");
    has(confirmed, "foundationRandomizeDVs(screen, add);");
    assert(count(main, "foundationRandomizeDVs(") == 1);
    const auto step = main.find("Reroll::step(foundation.reroll,");
    assert(step != std::string::npos);
    assert(step < main.find("if (down & HidNpadButton_Up)"));
    assert(step < main.find("if (add && (down & HidNpadButton_X)) {"));
    assert(step < main.find("if (down & HidNpadButton_B) {", step));
    // Touch: only the prompt's own buttons act while it is open.
    const auto touch = between(f, "uint64_t foundationDirectTouchInput(", "bool handleInputUX(");
    const auto modal = between(touch, "if (foundation.reroll.confirming) {", "if (foundation.editExitConfirm) {");
    has(modal, "return down;");
    lacks(modal, "focusId(");
    const auto fix = read("src/UI/Gen1PokemonEditorFoundationHardwareFix.inc");
    has(fix, "foundationDrawRerollConfirm(screen, fb);");
    has(fix, "} else if (foundationStateFor(screen).reroll.confirming) {\n        drawNavBar(fb, {{\"B\", \"Cancel\"}, {\"A\", \"Re-roll\"}});");
}

} // namespace

int main() {
    model();
    gen2Route();
    gen1Route();
    std::cout << "Gen I/II Y Random DVs confirmation (model + live routes): PASS\n";
}
