// Gen II Special: one stored Special DV and one Special Stat Exp, two real battle stats.
// pret/pokecrystal engine/pokemon/move_mon.asm CalcMonStats computes Special Attack and Special
// Defense from the same Special DV and Stat Exp with their own base stats. The editor must show
// both real numbers for G/S/C, never a placeholder.
// Round 7 (owner Round 6 A22 / C01): the Gen I calculated-Special presentation is the reference.
// The numeric STATS columns hold plain numbers, and Sp. Atk / Sp. Def are explained in the
// lower-middle of the STATS panel, below the grid and above Shiny.
// Round 11 (owner H22: "we still dont have a number in the calculated column for special
// calculated stat, add it"): the Special row's Calculated cell is no longer empty; like Gen I it
// shows numbers: Sp. Atk then Sp. Def, plain and space-separated (no slash, no "Sp." tags).
// This deliberately replaces the Round 7 "Special cell empty" assertions.
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
        // The Calculated column: plain numbers for HP / Attack / Defense / Speed, and for the
        // Special row both split stats in the Special block's order; never a slash or a tag.
        for (int row = 0; row < W::kSpecialRow; ++row)
            assert(W::calculatedCell(battle, row) == std::to_string(battle[static_cast<std::size_t>(row)]));
        const auto special = W::calculatedCell(battle, W::kSpecialRow);
        assert(special == std::to_string(spa) + "  " + std::to_string(spd));
        assert(special.find('/') == std::string::npos && special.find("Sp.") == std::string::npos);
        assert(W::calculatedCell(battle, 5).empty() && W::calculatedCell(battle, -1).empty());
        (void)c.game; // the stat formula is the same in Gold, Silver and Crystal
    }
    // Both VALUES layers (the unified workspace and the final hardware repaint) paint the plain
    // Calculated column and the one Special block, after the HP note.
    for (const char* path : {"src/UI/Gen2UnifiedPokemonWorkspace.inc", "src/UI/Gen2HardwareFinalFix.inc"}) {
        const auto src = read(path);
        const auto column = src.find("drawGen2CalculatedColumn(fb, battle, x, y, w);");
        const auto note = src.find("y + PokeBank::UIModel::Gen2Workspace::StatsGrid::kHpNoteY, \"* HP DV derived / read-only\"");
        const auto block = src.find("drawGen2SpecialBlock(fb, battle, x, y);");
        assert(column != std::string::npos && note != std::string::npos && block != std::string::npos);
        assert(column < note && note < block);
        assert(src.find("Special DV / Stat Exp shared by SpA and SpD") == std::string::npos); // replaced by the block
        assert(src.find(": \"\xE2\x80\x94\";") == std::string::npos);
    }
    const auto unified = read("src/UI/Gen2UnifiedPokemonWorkspace.inc");
    const auto at = [&](const char* fn) { return unified.find(fn); };
    const auto column = unified.substr(at("void drawGen2CalculatedColumn("),
                                       at("void drawGen2SpecialBlock(") - at("void drawGen2CalculatedColumn("));
    const auto block = unified.substr(at("void drawGen2SpecialBlock("),
                                      at("void drawUnifiedValues(") - at("void drawGen2SpecialBlock("));
    // The column: plain numbers on the grid, at the Calculated header, Special row included; no tag.
    assert(column.find("for (int row = 0; row <= W::kSpecialRow; ++row)") != std::string::npos);
    assert(column.find("x + W::StatsGrid::kCalculatedX, y + W::StatsGrid::rowY(row), W::calculatedCell(battle, row)") !=
           std::string::npos);
    assert(column.find("Sp.") == std::string::npos && column.find("tag") == std::string::npos);
    // The block: caption, "Sp. Atk n   Sp. Def n", note, from SpecialBlock; display only.
    for (const char* call : {"\"CALCULATED SPECIAL STATS\"", "B::kSpecialAttack", "std::to_string(battle[4])", "B::kSpecialDefense",
                             "std::to_string(battle[5])", "B::kNote"})
        assert(block.find(call) != std::string::npos);
    for (const char* forbidden : {"touchButtons", "dvs", "statExperience", "\" / \""})
        assert(block.find(forbidden) == std::string::npos && column.find(forbidden) == std::string::npos);
    // Geometry (STATS panel origin, Caption 10 px/glyph upper bound, 24 px line): the block sits
    // below the last grid row and the HP note, above the Shiny focus box, inside the panel; its
    // lines never overlap and "Sp. Def" starts right of the widest Sp. Atk number.
    using B = W::SpecialBlock;
    using S = W::StatsGrid;
    constexpr int line = 24, glyph = 10;
    static_assert(S::rowY(W::kSpecialRow) + line <= S::kHpNoteY);
    static_assert(S::kHpNoteY + line <= B::kCaptionY && B::kCaptionY + line <= B::kValuesY &&
                  B::kValuesY + line <= B::kNoteY && B::kNoteY + line <= S::kShinyY - 5);
    static_assert(B::kLabelX + 7 * glyph < B::kSpAValueX && B::kSpAValueX + 3 * glyph + 8 <= B::kSpDLabelX &&
                  B::kSpDLabelX + 7 * glyph < B::kSpDValueX && B::kSpDValueX + 3 * glyph <= S::kPanelW - 8);
    static_assert(B::kLabelX + 24 * glyph <= S::kPanelW - 8);          // "CALCULATED SPECIAL STATS"
    static_assert(S::kCalculatedX + 8 * glyph <= S::kPanelW - 8);      // the widest Special cell "999  999"
    assert(B::kLabelX + static_cast<int>(std::string(B::kNote).size()) * glyph <= S::kPanelW - 8);
    for (const char* path : {"src/UI/Gen2UnifiedPokemonWorkspace.inc", "src/UI/Gen2SharedSurfaceParity.inc",
                             "src/UI/Gen2HardwareWorkspaceFix.inc"})
        assert(read(path).find("midW = 398") != std::string::npos); // == StatsGrid::kPanelW
    // The passive View modal follows the same rule: the Stat cells come from the same
    // calculatedCell (Special row included, no "A n / D n" cell), and Sp. Atk / Sp. Def on their
    // own line from the same SpecialBlock tokens. Its STATS panel is the same 398 px wide.
    const auto modal = read("src/UI/Modals/Gen2PokemonDetailsModal.cpp");
    assert(modal.find("\" / D\"") == std::string::npos && modal.find("feeds SpA and SpD") == std::string::npos);
    assert(modal.find("if (i < 4)") == std::string::npos);
    assert(modal.find("fb.drawText(midX + 290, y, PokeBank::UIModel::Gen2Workspace::calculatedCell(battleStats, i)") !=
           std::string::npos);
    assert(modal.find("midW = 398") != std::string::npos);
    static_assert(290 + 8 * glyph <= S::kPanelW - 8);
    for (const char* call : {"B::kSpecialAttack", "std::to_string(battleStats[4])", "B::kSpecialDefense",
                             "std::to_string(battleStats[5])"})
        assert(modal.find(call) != std::string::npos);
    // The Shiny row really is where the block ends above (the final hardware repaint).
    assert(read("src/UI/Gen2HardwareFinalFix.inc").find("const int shinyY = y + 366;") != std::string::npos);
    std::cout << "Gen II Special: real Special Attack / Special Defense in the Calculated column and the Gen I-style "
                 "block PASS\n";
    return 0;
}
