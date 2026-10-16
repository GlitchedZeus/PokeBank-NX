#include "UI/SharedPokemonShell.h"
#pragma once
#include "UI/Gen2NativePresentation.h"
#include "UI/Gen2PokemonSession.h"
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
    return rows;
}

// The GEN II DATA pane is 198 px wide (SharedPokemonShell::drawDataAndGraph, dataW) with a
// 10 px text inset either side. Rows are laid out inside it: "Label: value" on one line when it
// fits, otherwise the label then the value word-wrapped beneath it (a word wider than the pane is
// broken by characters). Lines that would run below the pane end with an ellipsis instead, so
// caught-history text can never spill into the BATTLE STATS graph or past the panel.
constexpr int kDataPaneTextWidth = 198 - 20;
constexpr int kDataLineHeight = 20;
constexpr int kDataTextTop = 34;  // below the pane title
constexpr int kDataTextBottom = 8;

struct DataLine {
    std::string text;
    bool label = false; // dim label line (the value follows on its own line)
    int y = 0;          // offset from the first text line
};

template <class Measure>
std::vector<DataLine> layoutDataRows(const std::vector<DataRow>& rows, int width, int height,
                                     Measure measure) {
    std::vector<std::pair<std::string, bool>> wrapped;
    const auto wrapValue = [&](const std::string& value) {
        std::string line;
        std::size_t i = 0;
        while (i < value.size()) {
            std::size_t end = value.find(' ', i);
            if (end == std::string::npos) end = value.size();
            std::string word = value.substr(i, end - i);
            i = end < value.size() ? end + 1 : end;
            const std::string candidate = line.empty() ? word : line + " " + word;
            if (measure(candidate) <= width) { line = candidate; continue; }
            if (!line.empty()) { wrapped.push_back({line, false}); line.clear(); }
            while (!word.empty() && measure(word) > width) {
                std::size_t cut = word.size();
                while (cut > 1 && measure(word.substr(0, cut)) > width) --cut;
                wrapped.push_back({word.substr(0, cut), false});
                word = word.substr(cut);
            }
            line = word;
        }
        if (!line.empty()) wrapped.push_back({line, false});
    };
    for (const auto& row : rows) {
        const std::string combined = row.label + ": " + row.value;
        if (measure(combined) <= width) { wrapped.push_back({combined, false}); continue; }
        wrapped.push_back({row.label, true});
        wrapValue(row.value);
    }
    const int capacity = std::max(0, height / kDataLineHeight);
    std::vector<DataLine> lines;
    for (std::size_t i = 0; i < wrapped.size() && static_cast<int>(lines.size()) < capacity; ++i)
        lines.push_back({wrapped[i].first, wrapped[i].second, static_cast<int>(lines.size()) * kDataLineHeight});
    if (lines.size() < wrapped.size() && !lines.empty()) {
        auto& last = lines.back();
        last.label = false;
        std::string text = last.text;
        while (!text.empty() && measure(text + "...") > width) text.pop_back();
        last.text = text + "...";
    }
    return lines;
}
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
    const auto lines = W::layoutDataRows(rows, W::kDataPaneTextWidth,
        h - W::kDataTextTop - W::kDataTextBottom, [&fb](const std::string& text) {
            int tw = 0, th = 0;
            fb.measureText(text, tw, th, TextStyle::Caption);
            return tw;
        });
    for (const auto& line : lines)
        fb.drawText(x + 12 + 10, y + W::kDataTextTop + line.y, line.text,
                    line.label ? Colors::TextDim : Colors::Text, TextStyle::Caption);
}
} // namespace UI::Gen2WorkspacePresentation
