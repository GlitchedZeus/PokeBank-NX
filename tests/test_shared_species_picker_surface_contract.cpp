#include <cassert>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

namespace {
std::string read(const char* path) {
    std::ifstream in(path);
    assert(in);
    return {std::istreambuf_iterator<char>(in), {}};
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

    // Hardware regressions that prompted this contract.
    assert(gen2.find("fb.drawRoundedRect(x, y, w, h, 18, Colors::Accent, 2)") == std::string::npos);
    assert(gen3.find("constexpr int px = 110, py = 56, pw = 1060, ph = 600") == std::string::npos);
    assert(gen3.find("Choose Species — Generation III") == std::string::npos);

    // Selection/preview controls remain generation-specific only in behavior/data, not shell.
    contains(gen1, "state.pickerValue, 151");
    contains(gen2, "model.speciesChoice(), 251");
    contains(gen3, "state.speciesPreview, 386");
    contains(gen1, "pickerPreviewShiny");
    contains(gen2, "model.previewShiny");
    contains(gen3, "state.speciesPreviewShiny");

    std::cout << "Shared Gen I/II/III species-picker modal chrome contract: PASS\n";
    return 0;
}
