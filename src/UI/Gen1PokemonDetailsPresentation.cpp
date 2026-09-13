#include "UI/Gen1PokemonDetailsPresentation.h"

#include "Names/MoveNames.h"
#include "UI/Common.h"
#include "UI/PKSEFramebuffer.h"
#include "UI/ScreenChrome.h"
#include "UI/SpriteManager.h"
#include "UI/TrainerViewScreen.h"

#include <algorithm>
#include <string>

namespace UI::Modals {
namespace {
void valueRow(PKSEFramebuffer& fb, int x, int y, int w,
              const std::string& label, const std::string& value) {
    fb.drawText(x, y, label, Colors::TextDim, TextStyle::Body);
    int vw = 0, vh = 0;
    fb.measureText(value, vw, vh, TextStyle::Body);
    fb.drawText(x + w - vw, y, value, Colors::Text, TextStyle::Body);
}

std::string dexLabel(uint16_t species) {
    std::string dex = std::to_string(species);
    while (dex.size() < 3) dex = "0" + dex;
    return dex;
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

    const std::string name = p.nickname.empty() ? p.speciesName : p.nickname;
    fb.drawText(28, 16, name, Colors::Text, TextStyle::Heading);
    int nameW = 0, nameH = 0;
    fb.measureText(name, nameW, nameH, TextStyle::Heading);
    if (p.shiny) fb.drawShinyMark(40 + nameW, 20, 18, Colors::ShinyStar);

    const std::string sub = "Lv. " + std::to_string(p.level) + "     No. " + dexLabel(p.species);
    int subW = 0, subH = 0;
    fb.measureText(sub, subW, subH);
    fb.drawText(W - 90 - subW, 24, sub, Colors::TextDim);

    const std::string state = "GEN I  /  " + p.sourceStateLabel;
    int stateW = 0, stateH = 0;
    fb.measureText(state, stateW, stateH, TextStyle::Caption);
    fb.drawText((W - stateW) / 2, 22, state, Colors::Accent, TextStyle::Caption);
    fb.drawFilledRect(0, 60, W, 2, Colors::Accent);

    fb.drawFilledCircle(W - 40, 30, 20, Colors::PanelAlt);
    fb.drawCircle(W - 40, 30, 20, Colors::Border, 1);
    fb.drawText(W - 47, 18, "\xC3\x97", Colors::Text, TextStyle::Heading);
    screen.touchButtons.push_back({99, W - 64, 6, 52, 52});

    // Match the inherited PKSE three-column summary/editor proportions so View and
    // Create/Edit feel like one application rather than unrelated homebrew screens.
    const int colY = 72;
    const int colH = H - colY - kNavBarH - 6;
    const int leftX = 24, leftW = 430;
    const int centerX = 474, centerW = 340;
    const int rightX = 838, rightW = W - rightX - 24;
    drawPanelSurface(fb, leftX, colY, leftW, colH, false, 16);
    drawPanelSurface(fb, centerX, colY, centerW, colH, false, 16);
    drawPanelSurface(fb, rightX, colY, rightW, colH, false, 16);

    // LEFT — large cached artwork + identity/source truth.
    constexpr int renderSize = 170;
    if (auto* sprite = SpriteManager::getSprite(p.species, p.shiny); sprite && sprite->data) {
        fb.drawSpriteStaticContained(leftX + (leftW - renderSize) / 2, colY + 14,
                                     renderSize, renderSize, sprite->width, sprite->height,
                                     sprite->data, sprite->channels);
    } else {
        fb.drawText(leftX + 132, colY + 88, "Sprite unavailable", Colors::TextDim, TextStyle::Caption);
    }

    fb.drawText(leftX + 18, colY + 194, p.shiny ? "SHINY" : "NORMAL",
                p.shiny ? Colors::ShinyStar : Colors::Accent, TextStyle::Caption);
    int ly = colY + 226;
    const int identityW = leftW - 36;
    valueRow(fb, leftX + 18, ly, identityW, "Species", p.speciesName); ly += 36;
    valueRow(fb, leftX + 18, ly, identityW, "Nickname", name); ly += 36;
    valueRow(fb, leftX + 18, ly, identityW, "Level", std::to_string(p.level)); ly += 36;
    valueRow(fb, leftX + 18, ly, identityW, "EXP", std::to_string(p.experience)); ly += 36;
    valueRow(fb, leftX + 18, ly, identityW, "OT", p.originalTrainer); ly += 36;
    valueRow(fb, leftX + 18, ly, identityW, "Trainer ID", std::to_string(p.trainerId)); ly += 36;
    if (!p.recordLabel.empty())
        valueRow(fb, leftX + 18, ly, identityW, "Record", p.recordLabel);

    // CENTER — native Gen I five-stat/DV model. No modern IV/EV split and no
    // Sp. Atk / Sp. Def invention.
    fb.drawText(centerX + 18, colY + 16, "Values", Colors::Text, TextStyle::Heading);
    fb.drawText(centerX + 18, colY + 50, "Gen I DVs / Stat Exp", Colors::TextDim, TextStyle::Caption);
    fb.drawText(centerX + 132, colY + 82, "DV", Colors::TextDim, TextStyle::Caption);
    fb.drawText(centerX + 204, colY + 82, "Stat Exp", Colors::TextDim, TextStyle::Caption);
    static constexpr const char* statNames[] = {"HP", "Attack", "Defense", "Speed", "Special"};
    int sy = colY + 112;
    for (int i = 0; i < 5; ++i) {
        fb.drawText(centerX + 18, sy, statNames[i], Colors::TextDim);
        fb.drawText(centerX + 132, sy, std::to_string(p.dvs[static_cast<size_t>(i)]), Colors::Text);
        fb.drawText(centerX + 204, sy, std::to_string(p.statExperience[static_cast<size_t>(i)]), Colors::Text);
        sy += 40;
    }

    sy += 12;
    fb.drawText(centerX + 18, sy,
                p.battleStatsCalculated ? "Calculated battle stats" : "Battle stats",
                Colors::Text, TextStyle::Heading);
    sy += 38;
    if (p.hasBattleStats) {
        for (int i = 0; i < 5; ++i) {
            valueRow(fb, centerX + 18, sy, centerW - 36, statNames[i],
                     std::to_string(p.battleStats[static_cast<size_t>(i)]));
            sy += 32;
        }
    } else {
        fb.drawText(centerX + 18, sy, "Not stored in this boxed record.", Colors::TextDim, TextStyle::Caption);
    }

    // RIGHT — the same four-card move presentation used by the mature Gen I View,
    // including PP and PP Ups rather than exposing raw packed bytes.
    fb.drawText(rightX + 18, colY + 16, "Moves", Colors::Text, TextStyle::Heading);
    int my = colY + 62;
    for (int slot = 0; slot < 4; ++slot) {
        const auto i = static_cast<size_t>(slot);
        const uint16_t move = p.moves[i];
        const std::string moveName = move == 0 ? std::string("-") : std::string(Names::getMoveName(move));
        fb.drawFilledRoundedRect(rightX + 14, my, rightW - 28, 88, 12, Colors::PanelAlt);
        fb.drawText(rightX + 28, my + 14, moveName, Colors::Text, TextStyle::Body);
        fb.drawText(rightX + 28, my + 48, "PP " + std::to_string(p.pp[i]), Colors::TextDim, TextStyle::Caption);
        const std::string ups = "PP Ups " + std::to_string(p.ppUps[i]);
        int upsW = 0, upsH = 0;
        fb.measureText(ups, upsW, upsH, TextStyle::Caption);
        fb.drawText(rightX + rightW - 28 - upsW, my + 48, ups, Colors::TextDim, TextStyle::Caption);
        my += 102;
    }

    fb.drawText(rightX + 18, my + 8, "Generation I truth", Colors::Accent, TextStyle::Caption);
    fb.drawText(rightX + 18, my + 32, "No SID, nature, ability, held item,", Colors::TextDim, TextStyle::Caption);
    fb.drawText(rightX + 18, my + 52, "modern IV/EV split or modern met fields.", Colors::TextDim, TextStyle::Caption);

    screen.details.leftOrder.clear();
    screen.details.leftScroll = 0;
    drawNavBar(fb, {{"B", "Back"}});
}

} // namespace UI::Modals
