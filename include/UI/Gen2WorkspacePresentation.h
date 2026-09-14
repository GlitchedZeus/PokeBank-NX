#pragma once
#include "UI/Gen2NativePresentation.h"
#include "UI/Gen2PokemonSession.h"
#include "UI/StatsRadar.h"
#include "Integration/Gen2/Gen2BattleStats.h"
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
    if (game == G::SourceGame::Crystal) {
        const auto caught = Gen2Native::decodeCrystalCaughtData(p.caughtData);
        rows.push_back({"Met", std::string(Gen2Native::crystalMetTimeName(caught.timeOfDay)) + " / " + Gen2Native::crystalCaughtLevelText(caught)});
        rows.push_back({"Location", Gen2Native::crystalCaughtLocationName(caught.location)});
        rows.push_back({"OT gender", Gen2Native::crystalOriginalTrainerGenderText(caught)});
    }
    return rows;
}
} // namespace PokeBank::UIModel::Gen2Workspace

namespace UI::Gen2WorkspacePresentation {
// Sibling native-data and graph panels use the accepted Gen I split composition.
// Values also exposes the editable Held Item / Friendship / Pokerus capabilities.
inline void drawDataAndGraph(PKSEFramebuffer& fb, int x, int y, int w, int h,
    const PokeVault::Integration::Gen2::PokemonRecord& p, PokeVault::Integration::Gen2::SourceGame game) {
    constexpr int inset = 12, gap = 10, dataW = 198;
    const int dataX = x + inset, graphX = dataX + dataW + gap;
    const int graphW = w - 2 * inset - gap - dataW;
    for (const auto& pane : {std::pair{dataX, dataW}, std::pair{graphX, graphW}}) {
        fb.drawFilledRoundedRect(pane.first, y, pane.second, h, 12, Colors::Surface);
        fb.drawRoundedRect(pane.first, y, pane.second, h, 12, Colors::Divider, 1);
    }
    fb.drawText(dataX + 10, y + 10, "GEN II DATA", Colors::Accent, TextStyle::Caption);
    int rowY = y + 34;
    for (const auto& row : PokeBank::UIModel::Gen2Workspace::dataRows(p, game)) {
        const auto combined = row.label + ": " + row.value;
        int tw = 0, th = 0;
        fb.measureText(combined, tw, th, TextStyle::Caption);
        if (tw <= dataW - 20) {
            fb.drawText(dataX + 10, rowY, combined, Colors::Text, TextStyle::Caption);
            rowY += 20;
        } else {
            fb.drawText(dataX + 10, rowY, row.label, Colors::TextDim, TextStyle::Caption);
            fb.drawText(dataX + 10, rowY + 20, row.value, Colors::Text, TextStyle::Caption);
            rowY += 40;
        }
    }
    fb.drawText(graphX + 10, y + 10, "BATTLE STATS", Colors::Accent, TextStyle::Caption);
    StatsRadar::drawGen2Labeled(fb, graphX + 8, y + 32, graphW - 16, h - 40,
        PokeBank::UIModel::Gen2Workspace::battleStats(p));
}
} // namespace UI::Gen2WorkspacePresentation
