// Gen II Special: one stored Special DV and one Special Stat Exp, two real battle stats.
// pret/pokecrystal engine/pokemon/move_mon.asm CalcMonStats computes Special Attack and Special
// Defense from the same Special DV and Stat Exp with their own base stats. The editor's
// Calculated column must show both real numbers for G/S/C, never a placeholder.
#include "UI/Gen2WorkspacePresentation.h"

#include <cassert>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

namespace {
namespace G = PokeVault::Integration::Gen2;
namespace W = PokeBank::UIModel::Gen2Workspace;

// Independent CalcMonStats arithmetic: ((base + DV) * 2 + ceil(sqrt(statExp)) / 4) * L / 100 + 5.
uint16_t expectedStat(uint8_t base, uint8_t dv, uint16_t statExp, uint8_t level) {
    uint32_t root = 0;
    while (root < 255 && root * root < statExp) ++root;
    return static_cast<uint16_t>(((static_cast<uint32_t>(base + dv) * 2U + root / 4U) * level) / 100U + 5U);
}

std::string read(const char* path) {
    std::ifstream in(path, std::ios::binary);
    assert(in && "run from the repository root");
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}
} // namespace

int main() {
    struct Case { G::SourceGame game; uint16_t species; uint8_t level; uint8_t specialDV; uint16_t specialExp; };
    // Species with different Special Attack / Special Defense bases so a copy of one into the other
    // would be caught (Gengar 130/75, Snorlax 65/110, Mewtwo 154/90).
    const Case cases[] = {
        {G::SourceGame::Gold, 94, 50, 13, 40000},
        {G::SourceGame::Silver, 143, 37, 7, 1234},
        {G::SourceGame::Crystal, 150, 100, 15, 65535},
    };
    for (const auto& c : cases) {
        G::PokemonRecord p{};
        p.species = c.species;
        p.level = c.level;
        p.dvs = {0, 9, 4, 11, c.specialDV};
        p.statExperience = {100, 200, 300, 400, c.specialExp};
        const auto* personal = G::personalRecord(c.species);
        assert(personal && personal->specialAttack != personal->specialDefense);
        const auto battle = W::battleStats(p);
        const uint16_t spa = expectedStat(personal->specialAttack, c.specialDV, c.specialExp, c.level);
        const uint16_t spd = expectedStat(personal->specialDefense, c.specialDV, c.specialExp, c.level);
        assert(battle[4] == spa && battle[5] == spd && spa != spd);
        // Round 6 7A: Special's Calculated cell is SpA (tagged, NOT blank); SpD is the next grid
        // row in the SAME Calculated column; neither is a second DV / Stat Exp; no slash.
        const auto cell = W::calculatedCell(battle, 4);
        assert(cell.tag == "Sp. Atk" && cell.value == std::to_string(spa));
        static_assert(W::kSpecialDefenseRow == 5 && W::StatsGrid::rowY(5) == W::StatsGrid::rowY(4) + W::StatsGrid::kRowStep);
        const auto spdCell = W::calculatedCell(battle, W::kSpecialDefenseRow);
        assert(spdCell.tag == "Sp. Def" && spdCell.value == std::to_string(spd));
        for (int row = 0; row < 4; ++row) {
            const auto plain = W::calculatedCell(battle, row);
            assert(plain.tag.empty() && plain.value == std::to_string(battle[static_cast<std::size_t>(row)]));
        }
        for (int row = 0; row <= W::kSpecialDefenseRow; ++row)
            assert(W::calculatedCell(battle, row).value.find('/') == std::string::npos);
        (void)c.game; // the stat formula is the same in Gold, Silver and Crystal
    }
    // Both VALUES layers (the unified workspace and the final hardware repaint) use the helper.
    for (const char* path : {"src/UI/Gen2UnifiedPokemonWorkspace.inc", "src/UI/Gen2HardwareFinalFix.inc"}) {
        const auto src = read(path);
        // Round 6 7A: one shared Calculated-column painter on the shared grid (same column).
        assert(src.find("drawGen2CalculatedColumn(fb, battle, x, y, w);") != std::string::npos);
        assert(src.find("y + 246") == std::string::npos);       // no off-grid SpD line
        assert(src.find("\"Sp. Def\"") == std::string::npos);   // no left-column pseudo-stat label
        assert(src.find("\"Sp. Atk\"") == std::string::npos);   // no detached duplicate readout
        assert(src.find(": \"\xE2\x80\x94\";") == std::string::npos);
    }
    const auto unified = read("src/UI/Gen2UnifiedPokemonWorkspace.inc");
    const auto painter = unified.substr(unified.find("void drawGen2CalculatedColumn("),
                                        unified.find("void drawUnifiedValues(") - unified.find("void drawGen2CalculatedColumn("));
    assert(painter.find("W::calculatedCell(battle, row)") != std::string::npos);
    assert(painter.find("y + W::StatsGrid::rowY(row)") != std::string::npos);
    assert(painter.find("x + W::StatsGrid::tagX(w, vw, tw)") != std::string::npos);
    // The tag never runs into its number, and stays clear of the Stat Exp column ("65535" at x+184)
    // at the real panel width, with upper-bound caption widths (10 px/glyph: "Sp. Atk" 70, "999" 30).
    for (int valueW : {10, 20, 30})
        for (int tagW : {48, 56, 63, 70}) {
            const int tagX = W::StatsGrid::tagX(W::StatsGrid::kPanelW, valueW, tagW);
            const int valueX = W::StatsGrid::kPanelW - W::StatsGrid::kRightInset - valueW;
            assert(tagX + tagW + W::StatsGrid::kTagGap == valueX);
            assert(tagX >= 184 + 5 * 10 + 8);
        }
    for (const char* path : {"src/UI/Gen2UnifiedPokemonWorkspace.inc", "src/UI/Gen2SharedSurfaceParity.inc",
                             "src/UI/Gen2HardwareWorkspaceFix.inc"})
        assert(read(path).find("midW = 398") != std::string::npos); // == StatsGrid::kPanelW
    assert(painter.find("x + w - W::StatsGrid::kRightInset - vw") != std::string::npos);
    std::cout << "Gen II Special: real Special Attack / Special Defense in the Calculated column PASS\n";
    return 0;
}
