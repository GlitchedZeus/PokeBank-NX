#include "UI/SharedPokemonShell.h"
#pragma once
#include "UI/Gen2NativePresentation.h"
#include "UI/Gen2PokemonSession.h"
#include "UI/SharedDataPaneLayout.h"
#include "UI/StatsRadar.h"
#include "Integration/Gen2/Gen2BattleStats.h"
#include <algorithm>
#include <string>
#include <vector>

namespace PokeBank::UIModel::Gen2Workspace {
namespace G = PokeVault::Integration::Gen2;
namespace Rules = Gen2PokemonEditor;
inline std::array<uint16_t, 6> battleStats(const G::PokemonRecord& p) {
    if (p.partyRecord) return {p.maxHP, p.attack, p.defense, p.speed, p.specialAttack, p.specialDefense};
    return G::calculateBattleStats(p.species, p.level, Rules::storedDVs(p), p.statExperience).asArray();
}
// The Calculated cell of a VALUES row (HP, Attack, Defense, Speed, Special). Gen II stores ONE
// Special DV and ONE Special Stat Exp, but its battle stats are separate Special Attack and Special
// Defense, each calculated from that same pair with its own base stat (pret/pokecrystal
// engine/pokemon/move_mon.asm CalcMonStats). The Special row therefore shows both real numbers.
inline std::string calculatedCell(const std::array<uint16_t, 6>& battle, int row) {
    if (row >= 0 && row < 4) return std::to_string(battle[static_cast<std::size_t>(row)]);
    return std::to_string(battle[4]) + "/" + std::to_string(battle[5]);
}
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
    // Origin & Legality (formerly under the move rows): Crystal checks the current level against
    // the recorded met level; Gold/Silver record nothing to check.
    rows.push_back({"Legality", game == G::SourceGame::Crystal
        ? Gen2Native::caughtLevelLegalityText(Gen2Native::caughtLevelLegality(p.level, caught))
        : "Not checked"});
    return rows;
}

// The GEN II DATA pane text is laid out by the ONE shared data-pane layout
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
    SharedPokemonShell::drawDataAndGraph(fb, x, y, w, h, "GEN II DATA", std::vector<W::DataRow>{},
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
