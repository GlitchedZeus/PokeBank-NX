// L06 / E02 / NEW-20 (Round 3 hardware): Gen I-III Party A / touch opens the Pokemon Actions popup
// (View / Edit / Origin & Legality / Cancel) instead of jumping straight to View.
//   * the pure rules: Gen IV Party order, Edit listed but disabled (no Gen I-III party backend),
//     never focusable, Gen I-III only;
//   * the device wiring: the popup sees Party A / taps before every Gen I-III handler and before the
//     base View route, consumes every input while open, keeps the existing View route, and opens
//     the one shared Origin & Legality panel from the party record.
#include "UI/ClassicPartyActions.h"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

namespace M = PokeBank::UIModel::ClassicPartyActions;

namespace {

int failures = 0;
int checks = 0;
void check(bool ok, const std::string& what) {
    ++checks;
    if (!ok) {
        std::cerr << "FAILED: " << what << '\n';
        ++failures;
    }
}

std::string read(const char* path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        std::cerr << "missing " << path << '\n';
        std::exit(EXIT_FAILURE);
    }
    return {std::istreambuf_iterator<char>(in), {}};
}

std::size_t at(const std::string& text, const std::string& needle) {
    const auto p = text.find(needle);
    check(p != std::string::npos, "found: " + needle);
    return p;
}

} // namespace

int main() {
    // Pure rules.
    check(M::kEntries.size() == 4, "four Party actions");
    check(std::string(M::kEntries[0].label) == "View" && std::string(M::kEntries[1].label) == "Edit" &&
              std::string(M::kEntries[2].label) == "Origin & Legality" && std::string(M::kEntries[3].label) == "Cancel",
          "Gen IV Party order: View / Edit / Origin & Legality / Cancel");
    check(M::kEntries[0].enabled && !M::kEntries[1].enabled && M::kEntries[2].enabled && M::kEntries[3].enabled,
          "Edit is listed but disabled (Gen I-III staged editors are box-only)");
    for (int row = 0; row < 4; ++row)
        for (const int delta : {-1, 1})
            check(M::kEntries[static_cast<std::size_t>(M::moveRow(row, delta))].enabled,
                  "Up/Down never focuses the disabled Edit row");
    check(M::appliesTo(1) && M::appliesTo(2) && M::appliesTo(3) && !M::appliesTo(4) && !M::appliesTo(5) &&
              !M::appliesTo(0),
          "Gen I-III only (Gen IV / V draw their own Party Actions)");

    // Device wiring.
    const auto composite = read("src/UI/TrainerViewScreenCompositeOverlay.cpp");
    const auto update = composite.substr(at(composite, "void TrainerViewScreen::update(const PadState& pad"));
    const auto party = at(update, "ClassicPartyActions::handleInput(*this, down, held, stick.x, stick.y, touch)");
    check(party > at(update, "Gen5SharedReadOnlySurface::handleInput(") &&
              party < at(update, "ClassicPackedMove::handleInput(") &&
              party < at(update, "Gen3SharedEditorSurface::handleInput(") &&
              party < at(update, "Gen1PokemonEditor::handleInputUX(") &&
              party < at(update, "handleBasePartyTouch(*this, touch)") &&
              party < at(update, "updateGSCOverlay(pad, touch);"),
          "Party Actions sees Party input before every Gen I-III handler and the base View route");
    const auto drawFn = composite.substr(at(composite, "void TrainerViewScreen::draw(PKSEFramebuffer& fb)"));
    check(at(drawFn, "if (ClassicPartyActions::draw(*this, fb)) return;") < at(drawFn, "Gen4SharedEditorSurface::draw("),
          "the popup owns its frame");

    const auto overlay = read("src/UI/ClassicPartyActionsOverlay.inc");
    check(overlay.find("tap >= kPartyTouchBase && tap < kPartyTouchBase + 6") != std::string::npos &&
              overlay.find("(down & HidNpadButton_A) && partyPokemon(screen, screen.selectedPartyIndex)") !=
                  std::string::npos,
          "both a Party tap and Party A open the popup (occupied slots only)");
    check(overlay.find("screen.openPokemonActionSheet({PokeVault::UIModel::PokemonLocation::Party, 0, slot});") !=
              std::string::npos,
          "View keeps the existing read-only View route");
    check(overlay.find("SharedOriginLegalityPanel::draw(fb, screen.touchButtons,") != std::string::npos &&
              overlay.find("partyReport(screen, *pokemon)") != std::string::npos &&
              overlay.find("Gen1PokemonEditor::gen1LegalityReport(") != std::string::npos &&
              overlay.find("Gen2PokemonEditor::gen2LegalityReport(") != std::string::npos,
          "Origin & Legality is the shared panel from the party record (Gen I/II reuse their builders)");
    check(overlay.find("model.append(Model::kEntries[i].label, Model::kEntries[i].enabled ? static_cast<int>(i) : -1);") !=
              std::string::npos,
          "the disabled Edit row is drawn greyed (no touch target)");
    const auto handle = overlay.substr(at(overlay, "bool handleInput(TrainerViewScreen& screen"));
    const auto openPart = handle.substr(at(handle, "// The popup and its panel own every input until they close."));
    check(openPart.substr(0, openPart.find("\n}\n")).find("return false") == std::string::npos,
          "while open, the popup consumes every input (nothing reaches the Party view)");
    check(overlay.find("drawNarrowPokemonActions(fb, screen.touchButtons, model, state.row, title,") != std::string::npos,
          "the popup is the shared narrow Pokemon Actions menu");

    if (failures) {
        std::cerr << failures << " of " << checks << " check(s) failed\n";
        return EXIT_FAILURE;
    }
    std::cout << "Gen I-III Party Actions popup (L06 / E02 / NEW-20, " << checks << " checks): PASS\n";
    return EXIT_SUCCESS;
}
