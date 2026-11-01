#include <cassert>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

namespace {
// The shared editor workspace lives in src/UI/SharedPokemonEditorSurface.inc,
// which a generation surface includes textually. Expand that one include so a
// surface's contract covers the shared code it compiles.
std::string expandSurfaceIncludes(const std::string& text) {
    std::string out;
    std::size_t pos = 0;
    const std::string marker = "#include \"";
    while (true) {
        const auto at = text.find(marker, pos);
        if (at == std::string::npos) { out += text.substr(pos); break; }
        const auto close = text.find('"', at + marker.size());
        const std::string name = text.substr(at + marker.size(), close - at - marker.size());
        out += text.substr(pos, close + 1 - pos);
        pos = close + 1;
        if (name == "SharedPokemonEditorSurface.inc") {
            std::ifstream nested("src/UI/" + name, std::ios::binary);
            if (nested) out += "\n" + expandSurfaceIncludes(
                {std::istreambuf_iterator<char>(nested), std::istreambuf_iterator<char>()});
        }
    }
    return out;
}
std::string read(const char* path) {
    std::ifstream in(path);
    assert(in);
    return expandSurfaceIncludes({std::istreambuf_iterator<char>(in), {}});
}

void contains(const std::string& text, const char* needle) {
    if (text.find(needle) == std::string::npos)
        std::cerr << "Missing species-picker surface contract: " << needle << '\n';
    assert(text.find(needle) != std::string::npos);
}
}

int main() {
    const auto shared = read("include/UI/SharedSpeciesPicker.h");
    const auto gen1 = read("src/UI/Gen1PokemonEditorOverlayUXCleanup3.inc");
    const auto gen2 = read("src/UI/Gen2PokemonPickerOverlay.inc");
    const auto gen3 = read("src/UI/Gen3SharedPokemonSurface.inc");
    const auto gen4 = read("src/UI/Gen4SharedPokemonSurface.inc");
    const auto gen5 = read("src/UI/Gen5SharedReadOnlySurface.inc");
    const auto gen2Actions = read("src/UI/Gen2SharedSurfaceParity.inc");

    contains(shared, "inline constexpr int ModalWidth = 1080;");
    contains(shared, "inline constexpr int ModalHeight = 520;");
    contains(shared, "SharedSpeciesPicker"); // namespace remains the one shared owner
    contains(shared, "Colors::Divider, 1");
    contains(shared, "Theme accent belongs on the focused row and hints");

    // All supported editor generations must invoke the same modal chrome rather than
    // owning their own full-dialog accent/border geometry.
    contains(gen1, "SharedSpeciesPicker::drawModalChrome(fb)");
    contains(gen2, "SharedSpeciesPicker::drawModalChrome(fb)");
    contains(gen3, "SharedSpeciesPicker::drawModalChrome(fb)");
    contains(gen4, "SharedSpeciesPicker::drawModalChrome(fb)");

    // Hardware regressions that prompted this contract.
    assert(gen2.find("fb.drawRoundedRect(x, y, w, h, 18, Colors::Accent, 2)") == std::string::npos);
    assert(gen3.find("constexpr int px = 110, py = 56, pw = 1060, ph = 600") == std::string::npos);
    assert(gen3.find("Choose Species — Generation III") == std::string::npos);

    // Gen II Empty Slot / Pokemon Actions must use the same quiet panel shell as Gen I/III.
    contains(gen2Actions, "drawPanelSurface(fb, x, y, w, h, false, 18)");
    assert(gen2Actions.find("fb.drawRoundedRect(x, y, w, h, 18, Colors::FocusBorder, 2)") ==
           std::string::npos);

    // Selection/preview controls remain generation-specific only in behavior/data, not shell.
    contains(gen1, "state.pickerValue, 151");
    contains(gen2, "model.speciesChoice(), 251");
    contains(gen3, "state.speciesPreview, 386");
    contains(gen4, "species, A::kMaxSpecies, previewShiny");
    contains(gen4, "static constexpr int kMaxSpecies = 493;");
    contains(gen4, "state.speciesPreviewShiny");
    contains(gen4, "footer.yLabel = \"Normal/Shiny\"");
    contains(gen4, "true, previewTypes");
    contains(gen4, "gen4DexLabel(value)");
    contains(gen4, "Pokemon::getPersonalInfo4HGSS(species, 0)");
    contains(gen4, "canonicalToClassicType(speciesPersonal.type1)");
    assert(shared.find("PersonalInfo4HGSS.h") == std::string::npos);
    contains(gen1, "pickerPreviewShiny");
    contains(gen2, "model.previewShiny");
    contains(gen3, "state.speciesPreviewShiny");

    // Round 11 SPRITE: one artwork in the current preview state (no fixed NORMAL + SHINY
    // pair), and a tap on it previews the other appearance in every generation: each owner
    // turns the shared chooser's previewShinyTapped into its own Y (same toggle, applied on A).
    contains(shared, "getSprite(preview, form, previewShiny)");
    assert(shared.find("getSprite(preview, form, false)") == std::string::npos);
    assert(shared.find("getSprite(preview, form, true)") == std::string::npos);
    contains(shared, "previewArtRect(modalX, modalY, c.searching())");
    contains(gen1, "if (handled.previewShinyTapped) down |= HidNpadButton_Y;");
    contains(gen2, "if (handled.previewShinyTapped) down |= HidNpadButton_Y;");
    contains(gen3, "if (touched.previewShinyTapped) down |= HidNpadButton_Y;");
    contains(gen4, "if (touched.previewShinyTapped) down |= HidNpadButton_Y;");   // Gen IV + V shared
    contains(gen5, "#include \"SharedPokemonEditorSurface.inc\"");
    contains(gen5, "if (touched.previewShinyTapped) down |= HidNpadButton_Y;");

    std::cout << "Shared Gen I/II/III species-picker modal chrome contract: PASS\n";
    return 0;
}
