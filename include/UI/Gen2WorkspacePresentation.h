#include "UI/SharedPokemonShell.h"
#pragma once
#include "UI/Gen2NativePresentation.h"
#include "UI/SharedEditorPresentation.h"
#include "UI/Gen2PokemonSession.h"
#include "UI/SharedDataPaneLayout.h"
#include "UI/StatsRadar.h"
#include "Integration/Gen2/Gen2BattleStats.h"
#include <algorithm>
#include <array>
#include <string>
#include <vector>

namespace PokeBank::UIModel::Gen2Workspace {
namespace G = PokeVault::Integration::Gen2;
namespace Rules = Gen2PokemonEditor;
inline std::array<uint16_t, 6> battleStats(const G::PokemonRecord& p,bool draft=false) {
    if (p.partyRecord && !draft) return {p.maxHP, p.attack, p.defense, p.speed, p.specialAttack, p.specialDefense};
    return G::calculateBattleStats(p.species, p.level, Rules::storedDVs(p), p.statExperience).asArray();
}
// Gen II has one stored Special DV and Stat Exp but TWO derived Special stats (pret/pokecrystal
// CalcMonStats); neither derived value is a second editable DV or Stat Exp.
//
// Round 7 (owner Round 6 A22 / C01, R2-SPECIAL): the Gen I calculated-Special presentation is the
// reference: the two derived values are explained in the lower-middle of the STATS panel, below
// the grid and above Shiny, as Gen I shows its Special: "CALCULATED SPECIAL STATS", then
// "Sp. Atk <n>   Sp. Def <n>", then one note line.
// Round 11 (owner H22: "we still dont have a number in the calculated column for special
// calculated stat, add it"): like Gen I, the Special row's Calculated cell also holds numbers. The
// one Special DV / Stat Exp row feeds both stats, so the cell shows both, Sp. Atk then Sp. Def, in
// the block's order: plain numbers separated by spaces (no "a/b" slash, owner Round 3; no "Sp."
// tags in the numeric column, CG51 / CG84). Neither number is an editable DV or Stat Exp.
inline constexpr int kSpecialRow = 4;
inline constexpr const char* kSpecialCellGap = "  ";   // between Sp. Atk and Sp. Def
// The Calculated column's value for a grid row ("" out of range). Every Gen II STATS painter
// (unified workspace, final hardware repaint, legacy Foundation, View modal) uses it.
inline std::string calculatedCell(const std::array<uint16_t, 6>& battle, int row) {
    if (row >= 0 && row < kSpecialRow) return std::to_string(battle[static_cast<std::size_t>(row)]);
    if (row == kSpecialRow) return std::to_string(battle[4]) + kSpecialCellGap + std::to_string(battle[5]);
    return {};
}
// STATS geometry shared by every Gen II painter (relative to the STATS panel origin).
struct StatsGrid {
    static constexpr int kFirstRowY = 70, kRowStep = 34;
    static constexpr int kCalculatedX = 292;  // the Calculated column (header "Calculated")
    static constexpr int kPanelW = 398;       // the STATS panel width every Gen II painter passes
    static constexpr int rowY(int row) noexcept { return kFirstRowY + row * kRowStep; }
    static constexpr int kHpNoteY = 240;      // "* HP DV derived / read-only", directly below the grid
    static constexpr int kShinyY = 366;       // the Shiny row (redrawGen2StatsSemantics)
};
// The lower-middle Special block (Gen I reference: Gen1PokemonEditorFoundationHardwareFix.inc).
struct SpecialBlock {
    static constexpr const char* kSpecialAttack = "Sp. Atk";
    static constexpr const char* kSpecialDefense = "Sp. Def";
    static constexpr const char* kNote = "one stored Special DV / Stat Exp";
    static constexpr int kCaptionY = 270, kValuesY = 296, kNoteY = 322;  // below the HP note, above Shiny
    static constexpr int kLabelX = 18;        // "Sp. Atk" (and caption, note)
    static constexpr int kSpAValueX = 90;     // its number
    static constexpr int kSpDLabelX = 200;    // "Sp. Def"
    static constexpr int kSpDValueX = 272;    // its number
};
struct DataRow { std::string label, value; };
inline std::vector<DataRow> dataRows(const G::PokemonRecord& p, G::SourceGame game) {
    const auto* personal = G::personalRecord(p.species);
    const auto next = personal && p.level < 100 ? Pokemon::getExpForLevel(p.level + 1, personal->experienceGrowth) : p.experience;
    std::vector<DataRow> rows{
        {"Game", game == G::SourceGame::Gold ? "Gold" : game == G::SourceGame::Silver ? "Silver" : "Crystal"},
        {"Growth", personal ? Rules::growthGroupName(personal->experienceGrowth) : "Unknown"},
        {"EXP next", p.level >= 100 ? "MAX" : std::to_string(next > p.experience ? next - p.experience : 0)},
    };
    if (const auto party = Gen2Native::partyViewData(p)) {
        rows.push_back({"HP", std::to_string(party->currentHP) + " / " + std::to_string(party->maxHP)});
        rows.push_back({"Status", party->statusText});
    }
    const auto caught = Gen2Native::decodeCrystalCaughtData(p.caughtData);
    if (caught.present) {
        rows.push_back({"Caught history", game == G::SourceGame::Crystal
            ? "Crystal caught data" : "Retained Crystal data"});
        rows.push_back({"Met", Gen2Native::crystalCaughtLocationName(caught.location)});
        rows.push_back({"Met level", Gen2Native::crystalCaughtLevelText(caught)});
        rows.push_back({"Time", Gen2Native::crystalMetTimeName(caught.timeOfDay)});
        rows.push_back({"OT gender", Gen2Native::crystalOriginalTrainerGenderText(caught)});
    } else if (game == G::SourceGame::Gold || game == G::SourceGame::Silver) {
        rows.push_back({"Caught history", "Not recorded by Gold/Silver"});
    } else {
        rows.push_back({"Caught history", "No caught data recorded"});
    }
    // A level comparison is one native fact, not a complete legality verdict.
    if(game==G::SourceGame::Crystal)
        rows.push_back({"Level context",Gen2Native::caughtLevelLegalityText(
            Gen2Native::caughtLevelLegality(p.level,caught))});
    rows.push_back({"Legality",SharedEditorPresentation::legalityLabel(SharedEditorPresentation::LegalityState::NotChecked)});
    return rows;
}

// The Gen II Origin & Legality pane text is laid out by the ONE shared data-pane layout
// (UI/SharedDataPaneLayout.h, SharedDataPane::layoutRows): "Label: value" when it fits, otherwise
// the label then the value wrapped beneath it, "..." where the pane runs out, so caught-history
// text never spills into the BATTLE STATS graph or past the panel.
} // namespace PokeBank::UIModel::Gen2Workspace

namespace UI::Gen2WorkspacePresentation {
// Sibling native-data and graph panels use the accepted Gen I split composition.
// Values also exposes the editable Held Item / Friendship / Pokerus capabilities.
inline void drawDataAndGraph(PKSEFramebuffer& fb, int x, int y, int w, int h,
    const PokeVault::Integration::Gen2::PokemonRecord& p, PokeVault::Integration::Gen2::SourceGame game) {
    auto rows = PokeBank::UIModel::Gen2Workspace::dataRows(p, game);
    // The accepted shared legacy layer gives this sibling panel only 260 px.
    // When genuine caught history is present, prioritize those record bytes over
    // auxiliary party HP/status here. The final 1280x720 hardware surface is
    // taller (312 px) and therefore keeps every real row.
    if (h < 300 && p.caughtData != 0) {
        rows.erase(std::remove_if(rows.begin(), rows.end(), [](const auto& row) {
            return row.label == "HP" || row.label == "Status";
        }), rows.end());
    }
    // The shared composition draws both panes, the title and the graph; the rows are laid out
    // here so long caught-history values wrap inside the data pane instead of overflowing it.
    namespace W = PokeBank::UIModel::Gen2Workspace;
    // The pane title, as Gen III-V's adapters name it (A::kDataTitle): the shared heading.
    constexpr const char* kDataTitle = PokeBank::UIModel::SharedEditorPresentation::kOriginLegalityHeading;
    SharedPokemonShell::drawDataAndGraph(fb, x, y, w, h, kDataTitle, std::vector<W::DataRow>{},
        W::battleStats(p));
    namespace Pane = PokeBank::UIModel::SharedDataPane;
    const auto lines = Pane::layoutRows(rows, Pane::kTextWidth,
        h - Pane::kTextTop - Pane::kTextBottom, [&fb](const std::string& text) {
            int tw = 0, th = 0;
            fb.measureText(text, tw, th, TextStyle::Caption);
            return tw;
        });
    for (const auto& line : lines)
        fb.drawText(x + 12 + Pane::kTextInset, y + Pane::kTextTop + line.y, line.text,
                    line.label ? Colors::TextDim : Colors::Text, TextStyle::Caption);
}
} // namespace UI::Gen2WorkspacePresentation
