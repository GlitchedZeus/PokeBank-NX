#include "UI/Modals/Gen2PokemonDetailsModal.h"

#include "Integration/Gen2/Gen2BattleStats.h"
#include "Integration/Gen2/Gen2ReadOnlyInventory.h"
#include "Names/MoveNames.h"
#include "Pokemon/Pokemon2ReadOnly.h"
#include "UI/Common.h"
#include "UI/PKSEFramebuffer.h"
#include "UI/ScreenChrome.h"
#include "UI/SpriteManager.h"
#include "UI/StatsRadar.h"
#include "UI/TrainerViewScreen.h"
#include "Utils/StringHelpers.h"

#include <algorithm>
#include <array>
#include <cstdio>
#include <string>

namespace UI::Modals {
namespace {

namespace Gen2 = PokeVault::Integration::Gen2;

const char* gen2TypeName(uint8_t type) noexcept {
    switch (type) {
        case 0x00: return "Normal";
        case 0x01: return "Fighting";
        case 0x02: return "Flying";
        case 0x03: return "Poison";
        case 0x04: return "Ground";
        case 0x05: return "Rock";
        case 0x07: return "Bug";
        case 0x08: return "Ghost";
        case 0x09: return "Steel";
        case 0x14: return "Fire";
        case 0x15: return "Water";
        case 0x16: return "Grass";
        case 0x17: return "Electric";
        case 0x18: return "Psychic";
        case 0x19: return "Ice";
        case 0x1A: return "Dragon";
        case 0x1B: return "Dark";
        default: return "Unknown";
    }
}

const char* genderText(uint8_t gender) noexcept {
    switch (gender) {
        case 0: return "Male";
        case 1: return "Female";
        case 2: return "Genderless";
        default: return "Unknown";
    }
}

const char* sourceGameTitle(const std::string& id) noexcept {
    if (id == "gold_gbc") return "Gold";
    if (id == "silver_gbc") return "Silver";
    if (id == "crystal_gbc") return "Crystal";
    return "Gen II";
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

std::string heldItemText(uint16_t held) {
    if (held == 0) return "None";
    const auto name = Gen2::gen2ItemName(static_cast<uint8_t>(held));
    return name.empty() ? std::string("Item ") + std::to_string(held) : std::string(name);
}

std::string pokerusText(uint8_t raw) {
    if (raw == 0) return "None";
    const unsigned strain = raw >> 4U;
    const unsigned days = raw & 0x0FU;
    return "0x" + [&] {
        char buf[3];
        std::snprintf(buf, sizeof(buf), "%02X", raw);
        return std::string(buf);
    }() + " (S" + std::to_string(strain) + "/D" + std::to_string(days) + ")";
}

} // namespace

void drawGen2PokemonDetailsModal(TrainerViewScreen& screen, PKSEFramebuffer& fb,
                                 const Pokemon::Pokemon2ReadOnly& p) {
    const int W = fb.getWidth(), H = fb.getHeight();
    screen.touchButtons.clear();

    // Same passive three-panel visual language accepted for Generation I. No field cursor,
    // no A/Y/L/R actions and no mutation route live on this surface.
    fb.drawFilledRect(0, 0, W, H, Color(0, 0, 0, 130));
    fb.drawVerticalGradient(0, 0, W, H,
        Color(Colors::Background.r, Colors::Background.g, Colors::Background.b, 250),
        Color(Colors::Background.r, Colors::Background.g, Colors::Background.b, 255));

    constexpr int workspaceX = 40;
    constexpr int workspaceY = 31;
    constexpr int workspaceW = 1200;
    constexpr int workspaceH = 609;
    drawPanelSurface(fb, workspaceX, workspaceY, workspaceW, workspaceH, false, 18);

    fb.drawText(workspaceX + 24, workspaceY + 14, "View Pokemon — READ ONLY", Colors::Text, TextStyle::Heading);
    if (p.isShiny(0, {})) {
        int titleW = 0, titleH = 0;
        fb.measureText("View Pokemon — READ ONLY", titleW, titleH, TextStyle::Heading);
        fb.drawShinyMark(workspaceX + 36 + titleW, workspaceY + 18, 18, Colors::ShinyStar);
    }
    const std::string record = p.isPartyRecord() ? "Party PK2" : "Box PK2";
    fb.drawText(workspaceX + 24, workspaceY + 50,
                record + "  •  " + sourceGameTitle(screen.sourceGameId) + "  •  Source save immutable",
                Colors::TextDim, TextStyle::Caption);

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

    // DETAILS
    fb.drawText(leftX + 14, contentY + 10, "DETAILS", Colors::Accent, TextStyle::Caption);
    constexpr int renderSize = 174;
    if (auto* sprite = SpriteManager::getSprite(p.speciesID(), p.form(), p.isShiny(0, {})); sprite && sprite->data) {
        fb.drawSpriteStaticContained(leftX + 63, contentY + 34, renderSize, 146,
                                     sprite->width, sprite->height, sprite->data, sprite->channels);
    } else {
        fb.drawText(leftX + 82, contentY + 104, "Sprite unavailable", Colors::TextDim, TextStyle::Caption);
    }
    int badgeX = leftX + 18;
    drawBadge(fb, badgeX, contentY + 184, gen2TypeName(p.rawType1()));
    if (p.rawType2() != p.rawType1()) {
        int tw = 0, th = 0;
        fb.measureText(gen2TypeName(p.rawType1()), tw, th, TextStyle::Caption);
        badgeX += std::max(64, tw + 24) + 8;
        drawBadge(fb, badgeX, contentY + 184, gen2TypeName(p.rawType2()));
    }

    std::string dex = std::to_string(p.speciesID());
    while (dex.size() < 3) dex = "0" + dex;
    compactRow(fb, leftX + 18, contentY + 232, "Species", dex + " - " + p.species(), 90);
    compactRow(fb, leftX + 18, contentY + 275, "Nickname", Utils::utf16ToUtf8(p.nickname()), 90);
    compactRow(fb, leftX + 18, contentY + 318, "Level / EXP",
               "Lv " + std::to_string(p.level()) + " / " + std::to_string(p.exp()), 90);
    compactRow(fb, leftX + 18, contentY + 361, "OT", Utils::utf16ToUtf8(p.otName()), 90);
    compactRow(fb, leftX + 18, contentY + 404, "Trainer ID", std::to_string(p.tid16()), 90);
    compactRow(fb, leftX + 18, contentY + 447, "Gender", genderText(p.gender()), 90);

    // VALUES — four stored DVs plus derived HP DV; one Special DV/Stat Exp feeds both split stats.
    const auto editableDVs = std::array<uint8_t,4>{p.dvATK(), p.dvDEF(), p.dvSPE(), p.dvSpecial()};
    const auto calculated = Gen2::calculateBattleStats(
        p.speciesID(), p.level(), editableDVs,
        {p.statExpHP(), p.statExpATK(), p.statExpDEF(), p.statExpSPE(), p.statExpSpecial()});
    std::array<uint16_t,6> battleStats = calculated.asArray();
    bool calculatedBoxStats = true;
    if (p.isPartyRecord()) {
        battleStats = {p.statHPMax(), p.statATK(), p.statDEF(), p.statSPE(), p.statSPA(), p.statSPD()};
        calculatedBoxStats = false;
    }

    fb.drawText(midX + 14, contentY + 10, "VALUES", Colors::Text, TextStyle::Heading);
    fb.drawText(midX + 104, contentY + 48, "DV", Colors::TextDim, TextStyle::Caption);
    fb.drawText(midX + 198, contentY + 48, "Stat Exp", Colors::TextDim, TextStyle::Caption);
    fb.drawText(midX + 304, contentY + 48, "Stat", Colors::TextDim, TextStyle::Caption);

    const std::array<const char*,5> labels{"HP", "Attack", "Defense", "Speed", "Special"};
    const std::array<uint8_t,5> dvs{p.dvHP(), p.dvATK(), p.dvDEF(), p.dvSPE(), p.dvSpecial()};
    const std::array<uint16_t,5> statExp{p.statExpHP(), p.statExpATK(), p.statExpDEF(), p.statExpSPE(), p.statExpSpecial()};
    for (int i = 0; i < 5; ++i) {
        const int y = contentY + 88 + i * 48;
        fb.drawText(midX + 16, y, labels[static_cast<size_t>(i)], Colors::TextDim, TextStyle::Caption);
        fb.drawText(midX + 96, y, std::to_string(dvs[static_cast<size_t>(i)]) + (i == 0 ? " *" : ""),
                    Colors::Text, TextStyle::Caption);
        fb.drawText(midX + 192, y, std::to_string(statExp[static_cast<size_t>(i)]), Colors::Text,
                    TextStyle::Caption);
        std::string statText;
        if (i < 4) statText = std::to_string(battleStats[static_cast<size_t>(i)]);
        else statText = "A" + std::to_string(battleStats[4]) + " / D" + std::to_string(battleStats[5]);
        fb.drawText(midX + 290, y, statText, Colors::Text, TextStyle::Caption);
    }
    fb.drawText(midX + 16, contentY + 322, "* HP DV derived", Colors::TextDim, TextStyle::Caption);
    compactRow(fb, midX + 16, contentY + 356, "Shiny", p.isShiny(0, {}) ? "Yes" : "No", 124);
    compactRow(fb, midX + 16, contentY + 390, "Level", std::to_string(p.level()), 124);
    fb.drawText(midX + 16, contentY + 426,
                "One Special DV + Stat Exp feeds SpA and SpD", Colors::TextDim, TextStyle::Caption);
    fb.drawText(midX + 16, contentY + 454,
                calculatedBoxStats ? "Box battle stats calculated / read-only" : "Party battle stats stored / read-only",
                Colors::TextDim, TextStyle::Caption);

    // MOVES
    fb.drawText(rightX + 14, contentY + 10, "MOVES", Colors::Text, TextStyle::Heading);
    for (int slot = 0; slot < 4; ++slot) {
        const int y = contentY + 50 + slot * 44;
        const uint16_t move = p.move(slot);
        const std::string moveName = move == 0 ? std::string("Empty") : std::string(Names::getMoveName(move));
        fb.drawText(rightX + 20, y, moveName, Colors::Text, TextStyle::Caption);
        const std::string ppText = "PP " + std::to_string(p.movePP(slot)) + "  Up " + std::to_string(p.movePPUps(slot));
        int ppW = 0, ppH = 0;
        fb.measureText(ppText, ppW, ppH, TextStyle::Caption);
        fb.drawText(rightX + rightW - 20 - ppW, y, ppText, Colors::TextDim, TextStyle::Caption);
    }
    fb.drawFilledRect(rightX + 14, contentY + 220, rightW - 28, 1, Colors::Divider);
    compactRow(fb, rightX + 18, contentY + 235, "Move compatibility", "Not checked", 300, Colors::TextDim);
    compactRow(fb, rightX + 18, contentY + 261, "Encounter legality", "Not checked", 300, Colors::TextDim);

    // Supplemental native Gen II data + six-axis battle-stat radar.
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

    fb.drawText(leftPaneX + 10, splitY + 10, "GEN II DATA", Colors::Accent, TextStyle::Caption);
    fb.drawText(leftPaneX + 10, splitY + 37, "Held Item", Colors::TextDim, TextStyle::Caption);
    fb.drawText(leftPaneX + 10, splitY + 55, heldItemText(p.heldItem()), Colors::Text, TextStyle::Caption);
    fb.drawText(leftPaneX + 10, splitY + 79, "Friendship", Colors::TextDim, TextStyle::Caption);
    fb.drawText(leftPaneX + 10, splitY + 97, std::to_string(p.friendship()), Colors::Text, TextStyle::Caption);
    fb.drawText(leftPaneX + 10, splitY + 121, "Pokerus", Colors::TextDim, TextStyle::Caption);
    fb.drawText(leftPaneX + 10, splitY + 139, pokerusText(p.pokerusByte()), Colors::Text, TextStyle::Caption);
    fb.drawText(leftPaneX + 10, splitY + 163, "Caught / Met", Colors::TextDim, TextStyle::Caption);
    fb.drawText(leftPaneX + 10, splitY + 181, std::to_string(p.caughtData()), Colors::Text, TextStyle::Caption);

    fb.drawText(rightPaneX + 10, splitY + 10, "BATTLE STATS", Colors::Accent, TextStyle::Caption);
    StatsRadar::drawGen2Labeled(fb, rightPaneX + 8, splitY + 32, rightPaneW - 16, splitH - 40, battleStats);

    screen.details.leftOrder.clear();
    screen.details.leftScroll = 0;
    drawNavBar(fb, {{"B", "Back"}});
}

} // namespace UI::Modals
