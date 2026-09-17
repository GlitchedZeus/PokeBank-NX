#include "UI/Gen1PokemonDetailsPresentation.h"

#include "Integration/Gen1/Gen1StagedPokemonEditor.h"
#include "Names/MoveNames.h"
#include "Pokemon/Experience.h"
#include "UI/ClassicGameContext.h"
#include "UI/ClassicTypeBadges.h"
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
namespace ClassicContext = PokeBank::UIModel::ClassicGameContext;

std::string dexLabel(uint16_t species) {
    std::string dex = std::to_string(species);
    while (dex.size() < 3) dex = "0" + dex;
    return dex;
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

std::string passiveContextLine(const Gen1PokemonDetailsPresentation& p) {
    std::string line = p.recordLabel.empty() ? std::string("Pokemon") : p.recordLabel;
    const auto exact = ClassicContext::exactGameName(p.sourceGameLabel);
    if (!exact.empty()) {
        line += " • ";
        line += exact;
    }
    line += " • Source save immutable • ";
    line += p.sourceStateLabel.empty() ? "READ ONLY" : p.sourceStateLabel;
    return line;
}

} // namespace

void drawGen1PokemonDetailsPresentation(TrainerViewScreen& screen, PKSEFramebuffer& fb,
                                        const Gen1PokemonDetailsPresentation& p) {
    const int W = fb.getWidth(), H = fb.getHeight();
    screen.touchButtons.clear();

    // The mature Party/Box presenter now owns the same true fullscreen page as the
    // shared editor instead of drawing a second rounded workspace over the browser.
    fb.drawVerticalGradient(0, 0, W, H,
        Color(Colors::Background.r, Colors::Background.g, Colors::Background.b, 250),
        Color(Colors::Background.r, Colors::Background.g, Colors::Background.b, 255));

    const std::string name = p.nickname.empty() ? p.speciesName : p.nickname;
    fb.drawText(28, 16, name, Colors::Text, TextStyle::Heading);
    if (p.shiny) {
        int nw = 0, nh = 0;
        fb.measureText(name, nw, nh, TextStyle::Heading);
        fb.drawShinyMark(40 + nw, 20, 18, Colors::ShinyStar);
    }

    const std::string rightHeader = "Lv. " + std::to_string(p.level) + "     No. " + dexLabel(p.species);
    int rw = 0, rh = 0;
    fb.measureText(rightHeader, rw, rh);
    fb.drawText(W - 28 - rw, 24, rightHeader, Colors::TextDim);

    const std::string mode = p.sourceStateLabel.empty() ? std::string("READ ONLY") : p.sourceStateLabel;
    int mw = 0, mh = 0;
    fb.measureText(mode, mw, mh, TextStyle::Caption);
    fb.drawText((W - mw) / 2, 23, mode, Colors::Accent, TextStyle::Caption);
    fb.drawFilledRect(0, 60, W, 2, Colors::Accent);
    fb.drawText(28, 66, passiveContextLine(p), Colors::TextDim, TextStyle::Caption);

    constexpr int contentY = 96;
    const int contentH = H - contentY - kNavBarH - 8;
    constexpr int leftX = 24;
    constexpr int leftW = 300;
    constexpr int gap = 14;
    constexpr int midX = leftX + leftW + gap;
    constexpr int midW = 398;
    constexpr int rightX = midX + midW + gap;
    const int rightW = W - 24 - rightX;

    drawPanelSurface(fb, leftX, contentY, leftW, contentH, false, 14);
    drawPanelSurface(fb, midX, contentY, midW, contentH, false, 14);
    drawPanelSurface(fb, rightX, contentY, rightW, contentH, false, 14);

    // DETAILS — neutral heading and the same canonical ROMFS type sprites used by
    // fullscreen Edit/Create. Gen I intentionally does not invent native gender.
    fb.drawText(leftX + 14, contentY + 10, "DETAILS", Colors::Text, TextStyle::Caption);
    constexpr int renderSize = 174;
    if (auto* sprite = SpriteManager::getSprite(p.species, p.shiny); sprite && sprite->data) {
        fb.drawSpriteStaticContained(leftX + 63, contentY + 34, renderSize, 136,
                                     sprite->width, sprite->height, sprite->data, sprite->channels);
    } else {
        fb.drawText(leftX + 82, contentY + 100, "Sprite unavailable", Colors::TextDim, TextStyle::Caption);
    }
    ClassicTypeBadges::drawPairCentered(fb, leftX, leftW, contentY + 176,
                                        p.nativeTypes[0], p.nativeTypes[1]);

    compactRow(fb, leftX + 18, contentY + 214, "Species", dexLabel(p.species) + " - " + p.speciesName, 90);
    compactRow(fb, leftX + 18, contentY + 257, "Nickname", name, 90);
    compactRow(fb, leftX + 18, contentY + 300, "Level / EXP",
               "Lv " + std::to_string(p.level) + " / " + std::to_string(p.experience), 90);
    compactRow(fb, leftX + 18, contentY + 343, "OT", p.originalTrainer, 90);
    compactRow(fb, leftX + 18, contentY + 386, "Trainer ID", std::to_string(p.trainerId), 90);

    // STATS — DV / Stat Exp are neutral; calculated battle Stat values are accent.
    fb.drawText(midX + 14, contentY + 10, "STATS", Colors::Text, TextStyle::Heading);
    fb.drawText(midX + 104, contentY + 48, "DV", Colors::TextDim, TextStyle::Caption);
    fb.drawText(midX + 198, contentY + 48, "Stat Exp", Colors::TextDim, TextStyle::Caption);
    fb.drawText(midX + 310, contentY + 48, "Stat", Colors::TextDim, TextStyle::Caption);

    static constexpr const char* statNames[] = {"HP", "Attack", "Defense", "Speed", "Special"};
    for (int i = 0; i < 5; ++i) {
        const int y = contentY + 80 + i * 38;
        fb.drawText(midX + 16, y, statNames[i], Colors::TextDim, TextStyle::Caption);
        const std::string dv = std::to_string(p.dvs[static_cast<size_t>(i)]) + (i == 0 ? " *" : "");
        fb.drawText(midX + 96, y, dv, Colors::Text, TextStyle::Caption);
        fb.drawText(midX + 192, y, std::to_string(p.statExperience[static_cast<size_t>(i)]),
                    Colors::Text, TextStyle::Caption);
        fb.drawText(midX + 300, y,
                    p.hasBattleStats ? std::to_string(p.battleStats[static_cast<size_t>(i)]) : "-",
                    Colors::Accent, TextStyle::Caption);
    }

    // Reserve two independent explanatory regions. They never share a baseline with
    // calculated Special, Shiny, Level, or later read-only status rows.
    fb.drawText(midX + 16, contentY + 272, "* HP DV derived / read-only", Colors::TextDim, TextStyle::Caption);
    const std::string splitSpecial = p.hasBattleStats ? std::to_string(p.battleStats[4]) : "-";
    fb.drawText(midX + 16, contentY + 306, "CALCULATED SPECIAL STATS", Colors::TextDim, TextStyle::Caption);
    fb.drawText(midX + 16, contentY + 334, "SpA", Colors::TextDim, TextStyle::Caption);
    fb.drawText(midX + 82, contentY + 334, splitSpecial, Colors::Accent, TextStyle::Caption);
    fb.drawText(midX + 198, contentY + 334, "SpD", Colors::TextDim, TextStyle::Caption);
    fb.drawText(midX + 264, contentY + 334, splitSpecial, Colors::Accent, TextStyle::Caption);
    fb.drawText(midX + 16, contentY + 364,
                "* one stored Gen I Special stat; split display only",
                Colors::TextDim, TextStyle::Caption);
    compactRow(fb, midX + 16, contentY + 404, "Shiny", p.shiny ? "Yes" : "No", 124,
               p.shiny ? Colors::ShinyStar : Colors::Text);
    compactRow(fb, midX + 16, contentY + 442, "Level", std::to_string(p.level), 124);
    fb.drawText(midX + 16, contentY + 482,
                p.battleStatsCalculated ? "Calculated Stat cells are read-only" : "Party battle Stat cells are read-only",
                Colors::TextDim, TextStyle::Caption);

    // MOVES — PP/Ups and compatibility status own independent fixed regions. The
    // status is always right-aligned and empty moves never display PP/Ups noise.
    fb.drawText(rightX + 14, contentY + 10, "MOVES", Colors::Text, TextStyle::Heading);
    constexpr int moveStatusRightPad = 20;
    constexpr int ppRightOffset = 112;
    for (int slot = 0; slot < 4; ++slot) {
        const auto i = static_cast<size_t>(slot);
        const int y = contentY + 50 + slot * 44;
        const uint16_t move = p.moves[i];
        const std::string moveName = move == 0 ? std::string("Empty") : std::string(Names::getMoveName(move));
        fb.drawText(rightX + 20, y, moveName, Colors::Text, TextStyle::Caption);

        if (move != 0) {
            const std::string ppText = "PP " + std::to_string(p.pp[i]) + "  Up " + std::to_string(p.ppUps[i]);
            int ppW = 0, ppH = 0;
            fb.measureText(ppText, ppW, ppH, TextStyle::Caption);
            const int ppRight = rightX + rightW - ppRightOffset;
            fb.drawText(ppRight - ppW, y, ppText, Colors::TextDim, TextStyle::Caption);
        }

        if (p.moveCompatibilityChecked) {
            const std::string status = p.moveCompatible[i] ? "OK" : "Unusual";
            const Color statusColor = p.moveCompatible[i] ? Colors::Success : Colors::Warning;
            int statusW = 0, statusH = 0;
            fb.measureText(status, statusW, statusH, TextStyle::Caption);
            const int statusX = rightX + rightW - moveStatusRightPad - statusW;
            fb.drawText(statusX, y, status, statusColor, TextStyle::Caption);
        }
    }

    fb.drawHDivider(rightX + 14, contentY + 220, rightW - 28);
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

    const int splitY = contentY + 288;
    const int splitH = contentH - 300;
    constexpr int inset = 12;
    constexpr int paneGap = 10;
    constexpr int leftPaneW = 136;
    const int innerW = rightW - inset * 2;
    const int rightPaneW = innerW - paneGap - leftPaneW;
    const int leftPaneX = rightX + inset;
    const int rightPaneX = leftPaneX + leftPaneW + paneGap;

    drawPanelSurface(fb, leftPaneX, splitY, leftPaneW, splitH, false, 12);
    drawPanelSurface(fb, rightPaneX, splitY, rightPaneW, splitH, false, 12);

    const uint8_t growth = Gen1::StagedPokemonEditor::growthRate(p.species);
    const uint32_t nextLevelExp = p.level < 100
        ? Pokemon::getExpForLevel(static_cast<uint8_t>(p.level + 1), growth)
        : p.experience;
    const uint32_t toNextLevel = p.level < 100 && nextLevelExp > p.experience
        ? nextLevelExp - p.experience : 0;

    fb.drawText(leftPaneX + 10, splitY + 10, "GEN I DATA", Colors::Accent, TextStyle::Caption);
    fb.drawText(leftPaneX + 10, splitY + 38, "Game / record", Colors::TextDim, TextStyle::Caption);
    fb.drawText(leftPaneX + 10, splitY + 56,
                p.sourceGameLabel + " / " + shortRecordLabel(p.recordLabel), Colors::Text, TextStyle::Caption);
    fb.drawText(leftPaneX + 10, splitY + 80, "Growth", Colors::TextDim, TextStyle::Caption);
    fb.drawText(leftPaneX + 10, splitY + 98, growthName(growth), Colors::Text, TextStyle::Caption);
    fb.drawText(leftPaneX + 10, splitY + 122, "To next Lv", Colors::TextDim, TextStyle::Caption);
    fb.drawText(leftPaneX + 10, splitY + 140,
                p.level >= 100 ? std::string("MAX") : std::to_string(toNextLevel) + " EXP",
                Colors::Text, TextStyle::Caption);
    fb.drawText(leftPaneX + 10, splitY + 164, "Catch byte", Colors::TextDim, TextStyle::Caption);
    fb.drawText(leftPaneX + 10, splitY + 182, std::to_string(p.catchRate), Colors::Text, TextStyle::Caption);

    fb.drawText(rightPaneX + 10, splitY + 10, "BATTLE STATS", Colors::Accent, TextStyle::Caption);
    if (p.hasBattleStats)
        StatsRadar::drawGen1Labeled(fb, rightPaneX + 8, splitY + 32,
                                    rightPaneW - 16, splitH - 40, p.battleStats);

    screen.details.leftOrder.clear();
    screen.details.leftScroll = 0;
    drawNavBar(fb, {{"B", "Back"}});
}

} // namespace UI::Modals
