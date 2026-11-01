// Round 11 G5-RADAR: the Box summary (quick view) radar is the editor's radar in every Gen I-V
// generation. Owner (Extra 3): "gen 5 quick view graph ... doesn't work and isn't updated with
// the graph that is in the editor ... it also looks smaller in the quick view."
//  1 Route table: Gen I/II keep their own shared renderers, Gen III (RSE + FRLG), IV and V use
//    the shared six-stat radar; only formats without a shared editor keep the inherited hexagon.
//  2 A real (synthetic) boxed PK5 through Pokemon5ReadOnlyView: the quick-view values equal
//    displayStats5 (the editor's Gen5SharedReadOnlySurface::stats), so the radar model is equal.
//  3 Two low-level Gen V Pokemon draw visibly different shapes (the legacy 255 scale with its
//    0.06 floor drew both as the same blob).
//  4 Source contract: BoxPokemonPanel.cpp routes through BoxSummaryRadar and the shared renderer.
#include "Integration/Gen5/Gen5DisplayStats.h"
#include "Integration/Gen5/Gen5PidCoordination.h"
#include "Integration/Gen5/Gen5Progression.h"
#include "Pokemon/Pokemon5ReadOnlyView.h"
#include "UI/BattleStatRadarModel.h"
#include "UI/BoxSummaryRadar.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

namespace {
namespace R = PokeBank::UIModel::BoxSummaryRadar;
namespace G = PokeVault::Integration::Gen5;
namespace C = PokeVault::Integration::Gen5::Crypto;
using Gen = PokeBank::UIModel::PokemonEditorFoundation::Generation;

void require(bool ok, const char* what) {
    if (!ok) { std::cerr << "FAIL: " << what << '\n'; std::abort(); }
}
std::string read(const char* path) {
    std::ifstream in(path, std::ios::binary);
    require(static_cast<bool>(in), path);
    return {std::istreambuf_iterator<char>(in), {}};
}

void routeTable() {
    // Gen I and II records keep their own shared renderers whatever the save says.
    require(R::route(true, false, false, Gen::Gen1) == R::Route::Gen1Special, "Gen I -> Special radar");
    require(R::route(true, false, false, std::nullopt) == R::Route::Gen1Special, "PK1 tag wins");
    require(R::route(false, true, false, Gen::Gen2) == R::Route::Gen2Calculated, "Gen II -> calculated radar");
    require(R::route(false, true, false, std::nullopt) == R::Route::Gen2Calculated, "GSC tag wins");
    // Gen III: Ruby/Sapphire/Emerald and FireRed/LeafGreen records all report the FRLG group.
    require(R::route(false, false, true, std::nullopt) == R::Route::SharedSixStat, "Gen III record -> shared");
    require(R::route(false, false, true, Gen::Gen3) == R::Route::SharedSixStat, "Gen III exact -> shared");
    require(R::route(false, false, false, Gen::Gen4) == R::Route::SharedSixStat, "Gen IV -> shared");
    // The reported defect: Gen V fell through to the inherited hexagon.
    require(R::route(false, false, false, Gen::Gen5) == R::Route::SharedSixStat, "Gen V -> shared (G5-RADAR)");
    // Unchanged on purpose: inherited loaders without a shared editor.
    for (const auto later : {Gen::Gen6, Gen::Gen7, Gen::Gen8, Gen::Gen9})
        require(R::route(false, false, false, later) == R::Route::InheritedHexagon, "Gen VI+ unchanged");
    require(R::route(false, false, false, std::nullopt) == R::Route::InheritedHexagon, "no exact entry unchanged");
}

// --- Gen V native fixture (synthetic, never a real save) ---
void write32(std::vector<uint8_t>& dst, size_t off, uint32_t value) {
    C::write16(dst, off, static_cast<uint16_t>(value));
    C::write16(dst, off + 2, static_cast<uint16_t>(value >> 16));
}
G::Pokemon5ReadOnly boxedPk5(uint16_t species, uint8_t level, uint32_t pid, uint32_t ivBits,
                             uint8_t nature) {
    const auto personal = G::personal5(species, 0, true);
    require(static_cast<bool>(personal), "fixture species has B2W2 personal data");
    std::vector<uint8_t> pk(C::StoredSize, 0);
    write32(pk, 0, pid);
    C::write16(pk, 0x08, species);
    C::write16(pk, 0x0C, 12345);
    C::write16(pk, 0x0E, 54321);
    write32(pk, 0x10, G::experienceForLevel5(personal.info->growthRate, level));
    pk[0x15] = G::abilityForSlot5(*personal.info, G::abilityBit5(pid));
    write32(pk, 0x38, ivBits & 0x3FFFFFFFu);
    pk[0x40] = static_cast<uint8_t>(G::genderFromPid5(pid, personal.info->genderRatio) << 1);
    pk[0x41] = nature;
    pk[0x5F] = 23; // Black 2
    C::write16(pk, 0x80, 1);
    pk[0x83] = 4;
    pk[0x84] = level;
    C::write16(pk, 6, C::checksum(pk));
    const auto encrypted = C::encryptCandidate(pk);
    return G::Pokemon5ReadOnly(encrypted);
}

void gen5QuickViewMatchesEditor() {
    // Pidove Lv 8 (Attack-leaning) and Solosis Lv 7 (Sp. Atk-leaning): typical early-route box
    // Pokemon, every stat under 30.
    const Pokemon::Pokemon5ReadOnlyView pidove(boxedPk5(519, 8, 0x00012345u, 0x1084210Fu, 3),
                                               Enums::GameVersion::B2W2);
    const Pokemon::Pokemon5ReadOnlyView solosis(boxedPk5(577, 7, 0x00054321u, 0x0F7BDEF0u, 10),
                                               Enums::GameVersion::B2W2);
    for (const auto* view : {&pidove, &solosis}) {
        const auto quick = R::sixStats(*view);
        // The editor's value source (Gen5SharedReadOnlySurface::stats) is displayStats5.
        const auto editor = G::displayStats5(view->source(), true);
        require(editor[0] > 0, "fixture has real display stats");
        require(quick == editor, "quick view reads the editor's displayStats5, HP/Atk/Def/Spe/SpA/SpD");
        require(*std::max_element(quick.begin(), quick.end()) < 30, "fixture is low level");
        const auto a = PokeBank::UIModel::radarModel(quick);
        const auto b = PokeBank::UIModel::radarModel(editor);
        require(a.scale == b.scale && a.normalized == b.normalized, "same radar model as the editor");
        require(a.scale <= 35.0f, "adaptive scale, not the fixed 255");
    }

    // The reported symptom: two different low-level Pokemon must draw different shapes.
    const auto mA = PokeBank::UIModel::radarModel(R::sixStats(pidove));
    const auto mB = PokeBank::UIModel::radarModel(R::sixStats(solosis));
    float maxDiff = 0.0f;
    for (size_t i = 0; i < 6; ++i) maxDiff = std::max(maxDiff, std::fabs(mA.normalized[i] - mB.normalized[i]));
    require(maxDiff >= 0.1f, "adjacent low-level Gen V Pokemon draw visibly different radars");
    // ...which the inherited fixed 255 scale (clamped to [0.06, 1]) could not show.
    float legacyDiff = 0.0f;
    const auto sA = R::sixStats(pidove), sB = R::sixStats(solosis);
    for (size_t i = 0; i < 6; ++i)
        legacyDiff = std::max(legacyDiff, std::fabs(std::clamp(sA[i] / 255.0f, 0.06f, 1.0f) -
                                                    std::clamp(sB[i] / 255.0f, 0.06f, 1.0f)));
    require(legacyDiff < maxDiff / 2, "the legacy renderer collapsed them (documents the defect)");
}

void sourceContract() {
    const auto panel = read("src/UI/Panels/BoxPokemonPanel.cpp");
    require(panel.find("#include \"UI/BoxSummaryRadar.h\"") != std::string::npos, "panel uses the shared route");
    require(panel.find("namespace Radar = PokeBank::UIModel::BoxSummaryRadar;") != std::string::npos &&
            panel.find("switch (Radar::route(") != std::string::npos, "route selects the renderer");
    require(panel.find("case Radar::Route::SharedSixStat") != std::string::npos, "six-stat case");
    require(panel.find("StatsRadar::drawGen3Labeled(fb, x + 18, cy2 + 4, width - 36, radarH,\n"
                       "                Radar::sixStats(*p))") != std::string::npos,
            "six-stat case draws the editor's radar with the record's display stats");
    // The old FRLG/Gen4 whitelist that excluded Gen V is gone.
    require(panel.find("exact->identity.generation == Foundation::Generation::Gen4)) {") == std::string::npos,
            "no Gen III/IV-only whitelist");
    // Gen I/II branches and the Gen IV carried-source hiding stay as accepted (H-PASS).
    require(panel.find("StatsRadar::drawGen1Labeled(") != std::string::npos, "Gen I radar kept");
    require(panel.find("StatsRadar::drawGen2Labeled(") != std::string::npos &&
            panel.find("Gen2Workspace::battleStats(record)") != std::string::npos, "Gen II radar kept");
    require(panel.find("ClassicPackedMove::hidesGen4CarriedSource(") != std::string::npos, "carried source hidden");
    require(panel.find("{ 2000, x, y + headerH, width, height - headerH }") != std::string::npos, "tap target kept");
    // The editor itself draws the same renderer (SharedPokemonShell::drawDataAndGraph).
    const auto shell = read("include/UI/SharedPokemonShell.h");
    require(shell.find("StatsRadar::drawGen3Labeled(") != std::string::npos, "editor uses drawGen3Labeled");
    const auto gen5 = read("src/UI/Gen5SharedReadOnlySurface.inc");
    require(gen5.find("return PokeVault::Integration::Gen5::displayStats5(p, b2w2(screen));") != std::string::npos,
            "Gen V editor stats are displayStats5");
}
} // namespace

int main() {
    routeTable();
    gen5QuickViewMatchesEditor();
    sourceContract();
    std::cout << "Box summary radar: Gen I-V route to the editor's shared radar; Gen V values, scale and shape match the editor PASS\n";
    return 0;
}
