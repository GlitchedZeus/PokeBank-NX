#include "UI/Modals/Gen1PokemonDetailsModal.h"

#include "Names/MoveNames.h"
#include "Pokemon/Pokemon1ReadOnly.h"
#include "UI/Common.h"
#include "UI/PKSEFramebuffer.h"
#include "UI/ScreenChrome.h"
#include "UI/SpriteManager.h"
#include "UI/TrainerViewScreen.h"
#include "Utils/StringHelpers.h"

#include <cstdio>
#include <string>

namespace UI::Modals {
namespace {
    std::string statusText(uint8_t status) {
        if (status == 0) return "None (0x00)";
        char raw[16];
        std::snprintf(raw, sizeof(raw), "0x%02X", status);
        return raw;
    }

    void valueRow(PKSEFramebuffer& fb, int x, int y, int w,
                  const std::string& label, const std::string& value) {
        fb.drawText(x, y, label, Colors::TextDim, TextStyle::Body);
        int vw = 0, vh = 0;
        fb.measureText(value, vw, vh, TextStyle::Body);
        fb.drawText(x + w - vw, y, value, Colors::Text, TextStyle::Body);
    }
}

void drawGen1PokemonDetailsModal(TrainerViewScreen& screen, PKSEFramebuffer& fb,
                                 const Pokemon::Pokemon1ReadOnly& p) {
    const int W = fb.getWidth(), H = fb.getHeight();
    screen.touchButtons.clear();

    fb.drawFilledRect(0, 0, W, H, Color(0, 0, 0, 130));
    fb.drawVerticalGradient(0, 0, W, H,
        Color(Colors::Background.r, Colors::Background.g, Colors::Background.b, 250),
        Color(Colors::Background.r, Colors::Background.g, Colors::Background.b, 255));

    std::string name = Utils::utf16ToUtf8(p.nickname());
    if (name.empty()) name = p.species();
    fb.drawText(28, 16, name, Colors::Text, TextStyle::Heading);

    std::string dex = std::to_string(p.speciesID());
    while (dex.size() < 3) dex = "0" + dex;
    const std::string sub = "Lv. " + std::to_string(p.level()) + "     No. " + dex;
    int sw = 0, sh = 0;
    fb.measureText(sub, sw, sh);
    fb.drawText(W - 90 - sw, 24, sub, Colors::TextDim);

    const char* ro = "GEN I  /  READ ONLY";
    int rw = 0, rh = 0;
    fb.measureText(ro, rw, rh, TextStyle::Caption);
    fb.drawText((W - rw) / 2, 22, ro, Colors::Accent, TextStyle::Caption);
    fb.drawFilledRect(0, 60, W, 2, Colors::Accent);

    fb.drawFilledCircle(W - 40, 30, 20, Colors::PanelAlt);
    fb.drawCircle(W - 40, 30, 20, Colors::Border, 1);
    fb.drawText(W - 47, 18, "\xC3\x97", Colors::Text, TextStyle::Heading);
    screen.touchButtons.push_back({99, W - 64, 6, 52, 52});

    const int colY = 76;
    const int colH = H - colY - kNavBarH - 10;
    const int leftX = 24, leftW = 390;
    const int midX = 430, midW = 400;
    const int rightX = 846, rightW = W - rightX - 24;
    drawPanelSurface(fb, leftX, colY, leftW, colH, false, 16);
    drawPanelSurface(fb, midX, colY, midW, colH, false, 16);
    drawPanelSurface(fb, rightX, colY, rightW, colH, false, 16);

    // Left: species render + identity fields that physically exist in a Gen I PK1 record.
    constexpr int renderSz = 150;
    Sprite* sprite = SpriteManager::getSprite(p.speciesID(), 0, false);
    if (sprite && sprite->data) {
        fb.drawSpriteStaticContained(leftX + (leftW - renderSz) / 2, colY + 18,
                                     renderSz, renderSz, sprite->width, sprite->height,
                                     sprite->data, sprite->channels);
    }
    fb.drawText(leftX + 18, colY + 176, "Generation I record", Colors::Accent, TextStyle::Caption);
    int ly = colY + 208;
    const int lw = leftW - 36;
    valueRow(fb, leftX + 18, ly, lw, "Species", p.species()); ly += 36;
    valueRow(fb, leftX + 18, ly, lw, "Nickname", Utils::utf16ToUtf8(p.nickname())); ly += 36;
    valueRow(fb, leftX + 18, ly, lw, "OT", Utils::utf16ToUtf8(p.otName())); ly += 36;
    valueRow(fb, leftX + 18, ly, lw, "Trainer ID", std::to_string(p.tid16())); ly += 36;
    valueRow(fb, leftX + 18, ly, lw, "EXP", std::to_string(p.exp())); ly += 36;
    valueRow(fb, leftX + 18, ly, lw, "Current HP", std::to_string(p.statHPCurrent())); ly += 36;
    valueRow(fb, leftX + 18, ly, lw, "Status byte", statusText(p.statusByte())); ly += 36;
    valueRow(fb, leftX + 18, ly, lw, "Record", p.isPartyRecord() ? "Party (44 bytes)" : "Box (33 bytes)");

    // Center: native Gen I stat model. DVs are 4-bit values; Stat Exp is 16-bit and is NOT EVs.
    fb.drawText(midX + 18, colY + 16, "DVs / Stat Experience", Colors::Text, TextStyle::Heading);
    fb.drawText(midX + 18, colY + 50, "Gen I values - not modern IV / EV fields", Colors::TextDim, TextStyle::Caption);
    fb.drawText(midX + 164, colY + 84, "DV", Colors::TextDim, TextStyle::Caption);
    fb.drawText(midX + 246, colY + 84, "Stat Exp", Colors::TextDim, TextStyle::Caption);
    struct Gen1Stat { const char* label; unsigned dv; unsigned statExp; };
    const Gen1Stat stats[] = {
        {"HP", p.dvHP(), p.statExpHP()},
        {"Attack", p.dvATK(), p.statExpATK()},
        {"Defense", p.dvDEF(), p.statExpDEF()},
        {"Speed", p.dvSPE(), p.statExpSPE()},
        {"Special", p.dvSpecial(), p.statExpSpecial()},
    };
    int sy = colY + 116;
    for (const auto& stat : stats) {
        fb.drawText(midX + 18, sy, stat.label, Colors::TextDim);
        fb.drawText(midX + 164, sy, std::to_string(stat.dv), Colors::Text);
        fb.drawText(midX + 246, sy, std::to_string(stat.statExp), Colors::Text);
        sy += 42;
    }

    sy += 10;
    fb.drawText(midX + 18, sy, "Stored battle stats", Colors::Text, TextStyle::Heading);
    sy += 38;
    if (p.isPartyRecord()) {
        valueRow(fb, midX + 18, sy, midW - 36, "Max HP", std::to_string(p.statHPMax())); sy += 34;
        valueRow(fb, midX + 18, sy, midW - 36, "Attack", std::to_string(p.statATK())); sy += 34;
        valueRow(fb, midX + 18, sy, midW - 36, "Defense", std::to_string(p.statDEF())); sy += 34;
        valueRow(fb, midX + 18, sy, midW - 36, "Speed", std::to_string(p.statSPE())); sy += 34;
        valueRow(fb, midX + 18, sy, midW - 36, "Special", std::to_string(p.gen1Special()));
    } else {
        fb.drawText(midX + 18, sy, "Not stored in the 33-byte boxed record.", Colors::TextDim);
    }

    // Right: exact four move slots with the packed PP and PP-Up count from the save.
    fb.drawText(rightX + 18, colY + 16, "Moves", Colors::Text, TextStyle::Heading);
    int my = colY + 62;
    for (int slot = 0; slot < 4; ++slot) {
        const uint16_t move = p.move(slot);
        const std::string label = move == 0 ? std::string("-") : std::string(Names::getMoveName(move));
        fb.drawFilledRoundedRect(rightX + 14, my, rightW - 28, 88, 12, Colors::PanelAlt);
        fb.drawText(rightX + 28, my + 14, label, Colors::Text, TextStyle::Body);
        const std::string pp = "PP " + std::to_string(p.movePP(slot));
        const std::string ups = "PP Ups " + std::to_string(p.movePPUps(slot));
        fb.drawText(rightX + 28, my + 48, pp, Colors::TextDim, TextStyle::Caption);
        int uw = 0, uh = 0;
        fb.measureText(ups, uw, uh, TextStyle::Caption);
        fb.drawText(rightX + rightW - 28 - uw, my + 48, ups, Colors::TextDim, TextStyle::Caption);
        my += 102;
    }

    fb.drawText(rightX + 18, my + 8, "Unavailable in Generation I:", Colors::Accent, TextStyle::Caption);
    fb.drawText(rightX + 18, my + 32, "SID, nature, ability, held item,", Colors::TextDim, TextStyle::Caption);
    fb.drawText(rightX + 18, my + 52, "modern IV/EV, ribbons/marks,", Colors::TextDim, TextStyle::Caption);
    fb.drawText(rightX + 18, my + 72, "modern met / Ball metadata.", Colors::TextDim, TextStyle::Caption);

    screen.details.leftOrder.clear();
    screen.details.leftScroll = 0;
    drawNavBar(fb, {{"B", "Back"}});
}

} // namespace UI::Modals
