// Gen II Pokemon Actions -> Clone ownership (coordinator-authorised fix in
// ClassicReleaseActionFix.inc). The live Gen II Actions handler runs before every Gen II surface
// handler in TrainerViewScreen::update, so it must stop owning input once Clone starts:
//  - choosing Clone hides the action sheet (like Legality & Provenance) so the Clone dialog
//    receives the very next input and frame;
//  - the Actions handler and draw also refuse ownership while Clone/Provenance is active;
//  - in the Clone dialog, B returns to Pokemon Actions without staging, and only A stages;
//  - TrainerViewScreen::update routes Actions before the final Gen II surface (why this matters).
#include <cassert>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

namespace {

std::string read(const char* path) {
    std::ifstream in(path, std::ios::binary);
    assert(in);
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}
std::string between(const std::string& text, const char* begin, const char* end, size_t from = 0) {
    const auto a = text.find(begin, from);
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

} // namespace

int main() {
    const auto release = read("src/UI/ClassicReleaseActionFix.inc");
    const auto gen2 = release.substr(release.find("namespace UI::Gen2PokemonEditor {"));

    // Actions -> Clone hands ownership to the Clone dialog.
    const auto clone = between(gen2, "case Action::Clone: {", "case Action::Remove:");
    has(clone, "shared.cloneActive = true;");
    has(clone, "legacy.pokemonActions = false;");

    // Input and draw guards: the Actions layer never owns a frame under Clone/Provenance, and the
    // guard runs before the action sheet consumes A/B.
    const auto input = between(gen2, "bool handleReleaseActionInput(", "bool drawReleaseActionSurface(");
    const auto inputGuard = input.find("shared.cloneActive || shared.provenanceActive");
    assert(inputGuard != std::string::npos);
    assert(inputGuard < input.find("if (down & HidNpadButton_B)"));
    assert(inputGuard < input.find("if (!(down & HidNpadButton_A)) return true;"));
    const auto draw = between(gen2, "bool drawReleaseActionSurface(", "} // namespace UI::Gen2PokemonEditor");
    const auto drawGuard = draw.find("shared.cloneActive || shared.provenanceActive");
    assert(drawGuard != std::string::npos && drawGuard < draw.find("drawNarrowPokemonActions("));

    // Clone dialog: B returns to Actions before anything can stage; staging happens only after A.
    const auto surface = read("src/UI/Gen2SharedPokemonSurface.inc");
    const auto dialog = between(surface, "bool handleSharedClone(", "bool handleSharedReview(");
    has(dialog, "actions.pokemonActions = true;");
    const auto bPos = dialog.find("if (down & HidNpadButton_B) {");
    const auto aGate = dialog.find("if (!(down & HidNpadButton_A)) return true;");
    const auto stage = dialog.find("stageCloneBoxPokemon(");
    assert(bPos != std::string::npos && aGate != std::string::npos && stage != std::string::npos);
    assert(bPos < aGate && aGate < stage);
    has(between(dialog, "if (down & HidNpadButton_B) {", "if (!(down & HidNpadButton_A))"), "backToActions();");
    // After staging the dialog also returns to Actions (same as before the fix).
    assert(dialog.find("backToActions();", stage) != std::string::npos);

    // Both Clone entry points (live Actions and the shared surface) hide the action sheet.
    const auto sharedClone = between(surface, "case SharedEditor::Action::Clone: {", "case SharedEditor::Action::LegalityProvenance: {");
    has(sharedClone, "legacy.pokemonActions = false;");

    // Why: the Actions handler is routed ahead of the Gen II final surface.
    const auto composite = read("src/UI/TrainerViewScreenCompositeOverlay.cpp");
    const auto actionsRoute = composite.find("Gen2PokemonEditor::handleReleaseActionInput(*this");
    const auto finalRoute = composite.find("Gen2PokemonEditor::handleFinalGen2SurfaceInput(*this");
    assert(actionsRoute != std::string::npos && finalRoute != std::string::npos && actionsRoute < finalRoute);

    std::cout << "Gen II Actions -> Clone ownership, B cancel and staging gate: PASS\n";
}
