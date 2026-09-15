#include "UI/Gen1PokemonDetailsPresentation.h"

#include "Integration/Gen1/Gen1StagedPokemonEditor.h"
#include "Names/MoveNames.h"
#include "Pokemon/Experience.h"
#include "UI/Common.h"
#include "UI/PKSEFramebuffer.h"
#include "UI/ScreenChrome.h"
#include "UI/SpriteManager.h"
#include "UI/StatsRadar.h"
#include "UI/TrainerViewScreen.h"

#include <algorithm>
#include <string>

namespace UI::Modals {
namespace {

namespace Gen1 = PokeVault::Integration::Gen1;

std::string dexLabel(uint16_t species) {
    std::string dex = std::to_string(species);
    while (dex.size() < 3) dex = "0" + dex;
    return dex;
}

const char* typeName(uint8_t type) noexcept {
    switch (type) {
        case 0: return "Normal";
        case 1: return "Fighting";
        case 2: return "Flying";
        case 3: return "Poison";
        case 4: return "Ground";
        case 5: return "Rock";
        case 7: return "Bug";
        case 8: return "Ghost";
        case 20: return "Fire";
        case 21: return "Water";
        case 22: return "Grass";
        case 23: return "Electric";
        case 24: return "Psychic";
        case 25: return "Ice";
        case 26: return "Dragon";
        default: return "Unknown";
    }
}

const char* growthName(uint8_t growth) noexcept {
    switch (growth) {
        case 0: return "Medium Fast";
        case 1: return "Erratic";
        case 2: return "Fluctuating";
        case 3: return "Medium Slow";
        case 4: return "Fast";
        case 5: return "Slow";
        default: return "Unknown";
    }
}

void drawBadge(PKSEFramebuffer& fb, int x, int y, const std::string& text) {
    int tw = 0, th = 0;
    fb.measureText(text, tw, th, TextStyle::Caption);
    const int w = std::max(64, tw + 24);
    fb.drawFilledRoundedRect(x, y, w, 28, 10, Colors::Panel);
    fb.drawRoundedRect(x, y, w, 28, 10, Colors::Accent, 1);
    fb.drawText(x + 12, y + 6, text, Colors::Text, TextStyle::Caption);
}

void compactRow(PKSEFramebuffer& fb, int x, int y, const std::string& label,
                const std::string& value, int valueX = 100,
                Color valueColor = Colors::Text) {
    fb.drawText(x, y, label, Colors::TextDim, TextStyle::Caption);
    fb.drawText(x + valueX, y, value, valueColor, TextStyle::Caption);
}

std::string shortRecordLabel(const std::string& record) {
    if (record.find("Party") != std::string::npos) return "Party";
    if (record.find("Box") != std::string::npos) return "Box";
    return record.empty() ? std::string("Pokemon") : record;
}

} // namespace

void drawGen1PokemonDetailsPresentation(TrainerViewScreen& screen, PKSEFramebuffer& fb,
                                        const Gen1PokemonDetailsPresentation& p) {
    const int W = fb.getWidth(), H = fb.getHeight();
    screen.touchButtons.clear();

    fb.drawFilledRect(0, 0, W, H, Color(0, 0, 0, 130));
    fb.drawVerticalGradient(0, 0, W, H,
        Color(Colors::Background.r, Colors::Background.g, Colors::Background.b, 250),
        Color(Colors::Background.r, Colors::Background.g, Colors::Background.b, 255));

    constexpr int workspaceX = 40;
    constexpr int workspaceY = 31;
    constexpr int workspaceW = 1200;
    constexpr int workspaceH = 609;
    drawPanelSurface(fb, workspaceX, workspaceY, workspaceW, workspaceH, false, 18);

    const std::string name = p.nickname.empty() ? p.speciesName : p.nickname;
    fb.drawText(workspaceX + 24, workspaceY + 14, "View Pokemon — READ ONLY", Colors::Text, TextStyle::Heading);
    if (p.shiny) {
        int titleW = 0, titleH = 0;
        fb.measureText("View Pokemon — READ ONLY", titleW, titleH, TextStyle::Heading);
        fb.drawShinyMark(workspaceX + 36 + titleW, workspaceY + 18, 18, Colors::ShinyStar);
    }
    const std::string subtitle = p.recordLabel + "  •  Source save immutable";
    fb.drawText(workspaceX + 24, workspaceY + 50, subtitle, Colors::TextDim, TextStyle::Caption);

    constexpr int contentY = workspaceY + 74;
    constexpr int contentH = 516;
    constexpr int leftX = workspaceX + 18;
    constexpr int leftW = 300;
    constexpr int midX = leftX + leftW + 14;
    constexpr int midW = 398;
    constexpr int rightX = midX + midW + 14;
    constexpr int rightW = workspaceX + workspaceW - 18 - rightX;

    drawPanelSurface(fb, leftX, contentY, leftW, contentH, false, 14);
    drawPanelSurface(fb, midX, contentY, midW, contentH, false, 14);
    drawPanelSurface(fb, rightX, contentY, rightW, contentH, false, 14);

    fb.drawText(leftX + 14, contentY + 10, "DETAILS", Colors::Accent, TextStyle::Caption);
    constexpr int renderSize = 174;
    if (auto* sprite = SpriteManager::getSprite(p.species, p.shiny); sprite && sprite->data) {
        fb.drawSpriteStaticContained(leftX + 63, contentY + 34, renderSize, 146,
                                     sprite->width, sprite->height, sprite->data, sprite->channels);
    } else {
        fb.drawText(leftX + 82, contentY + 104, "Sprite unavailable", Colors::TextDim, TextStyle::Caption);
    }

    int badgeX = leftX + 18;
    drawBadge(fb, badgeX, contentY + 184, typeName(p.nativeTypes[0]));
    if (p.nativeTypes[1] != p.nativeTypes[0]) {
        int tw = 0, th = 0;
        fb.measureText(typeName(p.nativeTypes[0]), tw, th, TextStyle::Caption);
        badgeX += std::max(64, tw + 24) + 8;
        drawBadge(fb, badgeX, contentY + 184, typeName(p.nativeTypes[1]));
    }

    compactRow(fb, leftX + 18, contentY + 232, "Species", dexLabel(p.species) + " - " + p.speciesName, 90);
    compactRow(fb, leftX + 18, contentY + 275, "Nickname", name, 90);
    compactRow(fb, leftX + 18, contentY + 318, "Level / EXP",
               "Lv " + std::to_string(p.level) + " / " + std::to_string(p.experience), 90);
    compactRow(fb, leftX + 18, contentY + 361, "OT", p.originalTrainer, 90);
    compactRow(fb, leftX + 18, contentY + 404, "Trainer ID", std::to_string(p.trainerId), 90);

    fb.drawText(midX + 14, contentY + 10, "VALUES", Colors::Text, TextStyle::Heading);
    fb.drawText(midX + 104, contentY + 48, "DV", Colors::TextDim, TextStyle::Caption);
    fb.drawText(midX + 198, contentY + 48, "Stat Exp", Colors::TextDim, TextStyle::Caption);
    fb.drawText(midX + 310, contentY + 48, "Stat", Colors::TextDim, TextStyle::Caption);

    static constexpr const char* statNames[] = {"HP", "Attack", "Defense", "Speed", "Special"};
    for (int i = 0; i < 5; ++i) {
        const int y = contentY + 88 + i * 48;
        fb.drawText(midX + 16, y, statNames[i], Colors::TextDim, TextStyle::Caption);
        const std::string dv = std::to_string(p.dvs[static_cast<size_t>(i)]) + (i == 0 ? " *" : "");
        fb.drawText(midX + 96, y, dv, Colors::Text, TextStyle::Caption);
        fb.drawText(midX + 192, y, std::to_string(p.statExperience[static_cast<size_t>(i)]), Colors::Text,
                    TextStyle::Caption);
        fb.drawText(midX + 300, y,
                    p.hasBattleStats ? std::to_string(p.battleStats[static_cast<size_t>(i)]) : "-",
                    Colors::Text, TextStyle::Caption);
    }
    fb.drawText(midX + 16, contentY + 322, "* HP DV derived", Colors::TextDim, TextStyle::Caption);
    compactRow(fb, midX + 16, contentY + 366, "Shiny", p.shiny ? "Yes" : "No", 124);
    compactRow(fb, midX + 16, contentY + 412, "Level", std::to_string(p.level), 124);
    fb.drawText(midX + 16, contentY + 458,
                p.battleStatsCalculated ? "Calculated Stat cells are read-only" : "Party battle Stat cells are read-only",
                Colors::TextDim, TextStyle::Caption);

    // RIGHT — match the shared Gen II move-row language: move, PP, Up count and a truthful status marker.
    fb.drawText(rightX + 14, contentY + 10, "MOVES", Colors::Text, TextStyle::Heading);
    for (int slot = 0; slot < 4; ++slot) {
        const auto i = static_cast<size_t>(slot);
        const int y = contentY + 50 + slot * 44;
        const uint16_t move = p.moves[i];
        const std::string moveName = move == 0 ? std::string("Empty") : std::string(Names::getMoveName(move));
        fb.drawText(rightX + 20, y, moveName, Colors::Text, TextStyle::Caption);
        const std::string ppText = "PP " + std::to_string(p.pp[i]) + "  Up " + std::to_string(p.ppUps[i]);
        int ppW = 0, ppH = 0;
        fb.measureText(ppText, ppW, ppH, TextStyle::Caption);
        if (p.moveCompatibilityChecked) {
            const std::string status = p.moveCompatible[i] ? "OK" : "Unusual";
            const Color statusColor = p.moveCompatible[i] ? Colors::Success : Colors::Warning;
            int statusW = 0, statusH = 0;
            fb.measureText(status, statusW, statusH, TextStyle::Caption);
            const int statusX = rightX + rightW - 20 - statusW;
            fb.drawText(statusX, y, status, statusColor, TextStyle::Caption);
            fb.drawText(statusX - 14 - ppW, y, ppText, Colors::TextDim, TextStyle::Caption);
        } else {
            fb.drawText(rightX + rightW - 20 - ppW, y, ppText, Colors::TextDim, TextStyle::Caption);
        }
    }
    fb.drawFilledRect(rightX + 14, contentY + 220, rightW - 28, 1, Colors::Divider);
    const std::string compatibilityText = !p.moveCompatibilityChecked
        ? "Not checked"
        : (p.moveCompatibilityCompatible ? "OK" : "Unusual preserved");
    const Color compatibilityColor = !p.moveCompatibilityChecked
        ? Colors::TextDim
        : (p.moveCompatibilityCompatible ? Colors::Success : Colors::Warning);
    compactRow(fb, rightX + 18, contentY + 235, "Move compatibility", compatibilityText, 300,
               compatibilityColor);
    compactRow(fb, rightX + 18, contentY + 261, "Encounter legality", "Not checked", 300,
               Colors::TextDim);

    constexpr int splitY = contentY + 288;
    constexpr int splitH = 216;
    constexpr int inset = 12;
    constexpr int gap = 10;
    constexpr int innerW = rightW - inset * 2;
    constexpr int leftPaneW = 136;
    constexpr int rightPaneW = innerW - gap - leftPaneW;
    constexpr int leftPaneX = rightX + inset;
    constexpr int rightPaneX = leftPaneX + leftPaneW + gap;

    drawPanelSurface(fb, leftPaneX, splitY, leftPaneW, splitH, false, 12);
    drawPanelSurface(fb, rightPaneX, splitY, rightPaneW, splitH, false, 12);

    const uint8_t growth = Gen1::StagedPokemonEditor::growthRate(p.species);
    const uint32_t nextLevelExp = p.level < 100
        ? Pokemon::getExpForLevel(static_cast<uint8_t>(p.level + 1), growth)
        : p.experience;
    const uint32_t toNextLevel = p.level < 100 && nextLevelExp > p.experience
        ? nextLevelExp - p.experience : 0;

    fb.drawText(leftPaneX + 10, splitY + 10, "GEN I DATA", Colors::Accent, TextStyle::Caption);
    fb.drawText(leftPaneX + 10, splitY + 40, "Game / record", Colors::TextDim, TextStyle::Caption);
    fb.drawText(leftPaneX + 10, splitY + 58,
                p.sourceGameLabel + " / " + shortRecordLabel(p.recordLabel), Colors::Text, TextStyle::Caption);
    fb.drawText(leftPaneX + 10, splitY + 82, "Growth", Colors::TextDim, TextStyle::Caption);
    fb.drawText(leftPaneX + 10, splitY + 100, growthName(growth), Colors::Text, TextStyle::Caption);
    fb.drawText(leftPaneX + 10, splitY + 124, "To next Lv", Colors::TextDim, TextStyle::Caption);
    fb.drawText(leftPaneX + 10, splitY + 142,
                p.level >= 100 ? std::string("MAX") : std::to_string(toNextLevel) + " EXP",
                Colors::Text, TextStyle::Caption);
    fb.drawText(leftPaneX + 10, splitY + 166, "Catch byte", Colors::TextDim, TextStyle::Caption);
    fb.drawText(leftPaneX + 10, splitY + 184, std::to_string(p.catchRate), Colors::Text, TextStyle::Caption);

    fb.drawText(rightPaneX + 10, splitY + 10, "BATTLE STATS", Colors::Accent, TextStyle::Caption);
    if (p.hasBattleStats)
        StatsRadar::drawGen1Labeled(fb, rightPaneX + 8, splitY + 32, rightPaneW - 16, splitH - 40, p.battleStats);

    screen.details.leftOrder.clear();
    screen.details.leftScroll = 0;
    drawNavBar(fb, {{"B", "Back"}});
}

} // namespace UI::Modals