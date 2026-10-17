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
        const std::string cell = W::calculatedCell(battle, 4);
        assert(cell == std::to_string(spa) + "/" + std::to_string(spd));
        assert(cell.find("\xE2\x80\x94") == std::string::npos); // never the em-dash placeholder
        for (int row = 0; row < 4; ++row)
            assert(W::calculatedCell(battle, row) == std::to_string(battle[static_cast<std::size_t>(row)]));
        (void)c.game; // the stat formula is the same in Gold, Silver and Crystal
    }
    // Both VALUES layers (the unified workspace and the final hardware repaint) use the helper.
    for (const char* path : {"src/UI/Gen2UnifiedPokemonWorkspace.inc", "src/UI/Gen2HardwareFinalFix.inc"}) {
        const auto src = read(path);
        assert(src.find("Gen2Workspace::calculatedCell(battle, r)") != std::string::npos);
        assert(src.find(": \"\xE2\x80\x94\";") == std::string::npos);
    }
    std::cout << "Gen II Special: real Special Attack / Special Defense in the Calculated column PASS\n";
    return 0;
}
