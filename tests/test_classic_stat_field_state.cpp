// R2-GOLD: Gen I / Gen II STATS grids use the ONE shared field-state contract
// (UI/SharedFieldState.h through UI/ClassicStatFieldState.h). The derived HP DV and the calculated
// Stat column are ReadOnly (gold) in View, Create and Edit; stored DV / Stat Exp are Editable. The
// HP DV shown is always the one derived from the four stored DVs (pret CalcStat; PKHeX PK1/PK2).
#include "Integration/Gen1/Gen1StagedPokemonEditor.h"
#include "Integration/Gen2/Gen2StagedEditor.h"
#include "UI/ClassicStatFieldState.h"

#include <cassert>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

namespace {
namespace CSF = PokeBank::UIModel::ClassicStatFields;
namespace FS = PokeBank::UIModel::SharedFieldState;

std::string read(const char* path) {
    std::ifstream in(path, std::ios::binary);
    assert(in && "run from the repository root");
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}
} // namespace

int main() {
    // Cell states: every row's Stat column and the HP DV are ReadOnly; the rest Editable.
    for (int row = 0; row < 5; ++row) {
        for (int column = 0; column < 3; ++column) {
            const bool readOnly = column == CSF::kStatColumn || (row == CSF::kHpRow && column == CSF::kDvColumn);
            const auto state = CSF::cellState(row, column);
            assert((state == FS::State::ReadOnly) == readOnly);
            const auto unfocused = FS::style(state, false);
            assert((unfocused.value == FS::Token::Gold) == readOnly); // gold only for read-only values
            const auto focused = FS::style(state, true);
            assert(focused.focusOutline);                             // focus is never lost
            assert(focused.activates == !readOnly);                   // gold never implies editability
            if (readOnly) assert(focused.value == FS::Token::Gold);
        }
    }
    // The derived HP DV matches both staged editors for every stored DV combination.
    for (int a = 0; a < 16; ++a) for (int d = 0; d < 16; ++d) for (int s = 0; s < 16; ++s) for (int c = 0; c < 16; ++c) {
        const std::array<uint8_t, 4> dvs{static_cast<uint8_t>(a), static_cast<uint8_t>(d),
                                         static_cast<uint8_t>(s), static_cast<uint8_t>(c)};
        const uint8_t hp = CSF::derivedHpDv(dvs);
        assert(hp == PokeVault::Integration::Gen1::StagedPokemonEditor::derivedHPDV(dvs));
        assert(hp == PokeVault::Integration::Gen2::StagedEditor::derivedHPDV(dvs));
    }
    // Every Gen I / Gen II STATS surface draws through the contract and shows the derived HP DV.
    const auto gen1Edit = read("src/UI/Gen1PokemonEditorFoundationHardwareFix.inc");
    assert(gen1Edit.find("PokeBank::UIModel::ClassicStatFields::cellState(r, c), selected).value") != std::string::npos);
    assert(gen1Edit.find("std::to_string(Editor::derivedHPDV(dvs))") != std::string::npos);
    assert(gen1Edit.find("splitSpecial, Colors::TextSecondary") == std::string::npos);
    const auto gen1View = read("src/UI/Gen1PokemonDetailsPresentation.cpp");
    assert(gen1View.find("CSF::derivedHpDv({p.dvs[1], p.dvs[2], p.dvs[3], p.dvs[4]})") != std::string::npos);
    assert(gen1View.find("cell(i, 2), TextStyle::Caption") != std::string::npos);
    assert(gen1View.find("splitSpecial, Colors::Accent") == std::string::npos);
    for (const char* path : {"src/UI/Gen2UnifiedPokemonWorkspace.inc", "src/UI/Gen2HardwareFinalFix.inc"}) {
        const auto src = read(path);
        assert(src.find("dvs{gen2ShownHpDv(p), p.dvs[1], p.dvs[2], p.dvs[3], p.dvs[4]}") != std::string::npos);
        assert(src.find("gen2StatCellColor(r, 0)") != std::string::npos);
        assert(src.find("stat, gen2StatCellColor(r, 2)") != std::string::npos);
        assert(src.find("std::to_string(battle[4]), gen2StatCellColor(4, 2)") != std::string::npos);
    }
    const auto gen2View = read("src/UI/Modals/Gen2PokemonDetailsModal.cpp");
    assert(gen2View.find("CSF::derivedHpDv(") != std::string::npos);
    assert(gen2View.find("stat, cellColor(i, 2)") != std::string::npos);
    std::cout << "Gen I/II STATS field states: derived HP DV and calculated stats gold everywhere PASS\n";
    return 0;
}
