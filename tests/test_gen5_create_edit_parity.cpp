// S03 (owner round 1 FAIL: "gen 5 editor doesnt match gen 1-4, the left side menu text for the
// species names is cut off"; "gen 5 saves dont tell me in the create menu what game"):
//   * Gen III, IV and V DETAILS use one label column; no label is wider than it, and no label
//     carries "(read-only)" (a locked value is drawn in the gold ReadOnly field state instead);
//   * every species value ("NNN - Name") of each generation fits the shared value column, so
//     nothing on the left panel is clipped;
//   * Gen V lists the same 16 DETAILS rows as Gen III / IV, in the same order;
//   * the Create / Edit / View header names the exact game for B, W, B2 and W2 (and Gen III / IV);
//   * each Gen III-V surface passes its per-row field state to the shared DETAILS drawer.
// Labels are read from the real sources, so a renamed label is checked as it ships.

#include "Names/SpeciesNames.h"
#include "UI/ClassicGameContext.h"
#include "UI/SharedEditorPresentation.h"
#include "UI/SharedMenuGeometry.h"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

namespace {

namespace G = PokeBank::UIModel::SharedMenuGeometry;
namespace P = PokeBank::UIModel::SharedEditorPresentation;
namespace C = PokeBank::UIModel::ClassicGameContext;

int failures = 0;
void require(bool ok, const std::string& what) {
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

// The string literals of the array that follows `marker` up to its closing "}};".
std::vector<std::string> labelsAfter(const std::string& text, const std::string& marker) {
    std::vector<std::string> out;
    const auto start = text.find(marker);
    if (start == std::string::npos) return out;
    const auto end = text.find("}};", start);
    for (auto at = text.find('"', start); at != std::string::npos && at < end;) {
        const auto close = text.find('"', at + 1);
        out.push_back(text.substr(at + 1, close - at - 1));
        at = text.find('"', close + 1);
    }
    return out;
}

} // namespace

int main() {
    const std::string gen3 = read("src/UI/Gen3SharedPokemonSurface.inc");
    const std::string gen4 = read("src/UI/Gen4SharedPokemonSurface.inc");
    const std::string gen5Fields = read("include/Integration/Gen5/Gen5EditorFields.h");
    const std::string gen5 = read("src/UI/Gen5SharedReadOnlySurface.inc");
    const std::string shared = read("src/UI/SharedPokemonEditorSurface.inc");
    const std::string shell = read("include/UI/SharedPokemonShell.h");

    const auto labels3 = labelsAfter(gen3, "constexpr std::array<const char*,16> detailLabels{{");
    const auto labels4 = labelsAfter(gen4, "constexpr std::array<const char*, 16> detailLabels{{");
    const auto labels5 = labelsAfter(gen5Fields, "kDetailLabels{{");
    require(labels3.size() == 16 && labels4.size() == 16 && labels5.size() == 18, "DETAILS label arrays found");

    // One label column: every label fits it, no "(read-only)" text.
    for (const auto* labels : {&labels3, &labels4, &labels5})
        for (const auto& label : *labels) {
            require(G::fitsWidth(label, G::Text::Caption, P::kDetailLabelWidth), "S03 label fits the column: " + label);
            require(label.find("read-only") == std::string::npos, "S03 no \"(read-only)\" label: " + label);
        }

    // Gen V shows the same 16 rows as Gen III / IV, in the same order (+ OT Gender, Fateful).
    for (std::size_t i = 0; i < 16 && i < labels5.size() && i < labels4.size() && i < labels3.size(); ++i) {
        require(labels5[i] == labels4[i], "S03 Gen V row " + std::to_string(i) + " matches Gen IV: " + labels5[i]);
        require(labels4[i] == labels3[i], "Gen IV row " + std::to_string(i) + " matches Gen III: " + labels4[i]);
    }

    // Every species value fits the shared value column (the clipped text the owner saw).
    const int valueWidth = P::detailValueWidth(P::kDetailsPanelWidth);
    for (const auto& [maxSpecies, gen] : {std::pair{386, "III"}, std::pair{493, "IV"}, std::pair{649, "V"}})
        for (int s = 1; s <= maxSpecies; ++s) {
            const std::string value = std::to_string(s) + " - " + Names::getSpeciesName(static_cast<uint16_t>(s));
            require(G::fitsWidth(value, G::Text::Caption, valueWidth), std::string("S03 Gen ") + gen +
                    " species value fits: " + value);
        }
    // Before the fix Gen V's value column started after "Language (read-only)": prove that was the clip.
    {
        const int oldInset = G::textWidth("Language (read-only)", G::Text::Caption) + 28;
        const int oldWidth = P::kDetailsPanelWidth - oldInset - P::kDetailValueRightPad;
        require(!G::fitsWidth("460 - Abomasnow", G::Text::Caption, oldWidth) &&
                G::fitsWidth("460 - Abomasnow", G::Text::Caption, valueWidth),
                "S03 reproduces the clipped species and shows it fixed");
    }

    // The exact game in the Create / Edit / View header (B, W, B2, W2 audited separately).
    for (const auto& [id, name] : {std::pair{"black_nds", "Black"}, std::pair{"white_nds", "White"},
                                   std::pair{"black2_nds", "Black 2"}, std::pair{"white2_nds", "White 2"},
                                   std::pair{"platinum_nds", "Platinum"}, std::pair{"emerald_gba", "Emerald"},
                                   std::pair{"crystal_gbc", "Crystal"}, std::pair{"red_gb", "Red"}}) {
        require(C::exactGameName(id) == name, std::string("S03 exact game: ") + id);
        require(C::contextLine("Box 1 / Slot 1", id, C::SurfaceMode::CreateDraft) ==
                std::string("Box 1 / Slot 1 \xE2\x80\xA2 ") + name + " \xE2\x80\xA2 Source save immutable \xE2\x80\xA2 DRAFT",
                std::string("S03 Create header names the game: ") + id);
    }

    // Every Gen III-V surface passes per-row field state; the drawer uses the one column model.
    require(shared.find("A::detailLocked(screen, state, static_cast<int>(row), *raw)") != std::string::npos,
            "shared surface (Gen IV / V) passes field state");
    require(gen3.find("return detailLocked(p, static_cast<int>(row))") != std::string::npos,
            "Gen III passes field state");
    require(gen4.find("static bool detailLocked(") != std::string::npos &&
            gen5.find("static bool detailLocked(") != std::string::npos &&
            gen5Fields.find("inline bool detailLocked(") != std::string::npos, "Gen IV / V locked-row rules");
    require(shell.find("const int valueInset = kDetailValueInset;") != std::string::npos &&
            shell.find("FieldState::style(state(row), selected)") != std::string::npos,
            "DETAILS drawer: fixed column + field state");

    if (failures) {
        std::cerr << failures << " failure(s)\n";
        return EXIT_FAILURE;
    }
    std::cout << "S03 Gen V Create/Edit parity (DETAILS column, labels, species values, exact game): PASS\n";
    return EXIT_SUCCESS;
}
