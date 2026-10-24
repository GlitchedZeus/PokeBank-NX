// Crystal Met Location chooser (owner campaign phase 11): sized to its choices, the same geometry
// for drawing and hit-testing, and names only in every row (no raw landmark index or source tag).
// Gen I stores no met location, so there is no Gen I chooser to resize; this pins that too.
#include "UI/Gen2PokemonPickerModel.h"

#include <cassert>
#include <cctype>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

namespace {
namespace P = PokeBank::UIModel::Gen2PokemonPicker;
namespace E = PokeVault::Integration::EncounterGuardrails;

std::string read(const char* path) {
    std::ifstream in(path, std::ios::binary);
    assert(in);
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}
} // namespace

int main() {
    // Proportional: one row per choice up to 9, then it scrolls; always above the footer.
    int previous = 0;
    for (int count = 0; count <= 40; ++count) {
        const auto g = P::locationPickerGeometry(count);
        assert(g.visible == std::clamp(count, 1, 9));
        assert(g.height == 80 + g.visible * g.rowStep + 16);           // NEW-10: compact header / bottom
        assert(g.listY == g.y + 80 && g.listY >= g.y + 52 + 20);       // rows clear the caption line
        assert(g.height >= previous);
        previous = g.height;
        assert(g.x >= 0 && g.x + g.width <= 1280 && g.y >= 0 && g.y + g.height <= 720 - 56);
        assert(g.listY + g.visible * g.rowStep <= g.y + g.height - 16); // every visible row fully inside
    }
    // Three choices is a small window, not the old fixed 568 px one.
    assert(P::locationPickerGeometry(3).height == 80 + 3 * 44 + 16 && P::locationPickerGeometry(3).height < 244);

    // Every Crystal encounter template: a real place name and a method/level detail.
    int rows = 0;
    for (uint16_t species = 1; species <= 251; ++species) {
        for (const auto& encounter : E::forGameSpecies("crystal_gbc", species)) {
            const auto text = P::locationRowText(encounter);
            assert(!text.place.empty() && text.place != "Unused / invalid");
            assert(text.place.find("0x") == std::string::npos && text.place.find('#') == std::string::npos);
            bool allDigits = true;
            for (const char c : text.place) allDigits = allDigits && std::isdigit(static_cast<unsigned char>(c));
            assert(!allDigits);
            assert(text.detail.find(E::methodName(encounter.method)) == 0);
            assert(text.detail.find(" · Lv. ") != std::string::npos);
            ++rows;
        }
    }
    assert(rows > 100);

    // Live: draw and input both use the one geometry; no developer wording; no Gen I chooser.
    const auto overlay = read("src/UI/Gen2PokemonPickerOverlay.inc");
    std::size_t uses = 0;
    for (auto at = overlay.find("locationPickerGeometry("); at != std::string::npos;
         at = overlay.find("locationPickerGeometry(", at + 1)) ++uses;
    assert(uses == 3); // draw, touch scroll, A reveal
    assert(overlay.find("Drag naturally to browse Crystal encounters") == std::string::npos);
    assert(overlay.find("\"Met Location\"") != std::string::npos);
    for (const char* path : {"src/UI/Gen1PokemonEditorOverlayFoundation.inc",
                             "src/UI/Gen1PokemonEditorFoundationHardwareFix.inc",
                             "src/UI/Gen1PokemonEditorOverlayUXCleanup2.inc",
                             "src/UI/Gen1PokemonEditorOverlayUXCleanup3.inc"})
        assert(read(path).find("Met Location") == std::string::npos);
    std::cout << "Crystal Met Location chooser: proportional, one geometry, names only PASS\n";
    return 0;
}
