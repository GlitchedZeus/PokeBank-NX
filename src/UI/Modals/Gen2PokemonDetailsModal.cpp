#include "UI/Modals/Gen2PokemonDetailsModal.h"

#include "Integration/Gen2/Gen2BattleStats.h"
#include "Integration/Gen2/Gen2MoveCompatibility.h"
#include "Integration/Gen2/Gen2PersonalData.h"
#include "Integration/Gen2/Gen2ReadOnlyInventory.h"
#include "Integration/Gen2/Gen2HeldItems.h"
#include "Names/MoveNames.h"
#include "Pokemon/Experience.h"
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
#include <optional>
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

std::optional<Gen2::SourceGame> sourceGameFromId(const std::string& id) noexcept {
    if (id == "gold_gbc") return Gen2::SourceGame::Gold;
    if (id == "silver_gbc") return Gen2::SourceGame::Silver;
    if (id == "crystal_gbc") return Gen2::SourceGame::Crystal;
    return std::nullopt;
}

const char* growthName(uint8_t group) noexcept {
    switch (group) {
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

std::string heldItemText(uint16_t held) {
    if (held == 0) return "None";
    const auto name = Gen2::gen2ItemName(static_cast<uint8_t>(held));
    return Gen2::selectableHeldItem(static_cast<uint8_t>(held)) ? std::string(name) : "Raw item " + std::to_string(held);
}

std::string pokerusText(uint8_t raw) {
    if (raw == 0) return "None";
    char buf[3];
    std::snprintf(buf, sizeof(buf), "%02X", raw);
    return std::string("0x") + buf;
}

} // namespace

void drawGen2PokemonDetailsModal(TrainerViewScreen& screen, PKSEFramebuffer& fb,
                                 const Pokemon::Pokemon2ReadOnly& p) {
    const int W = fb.getWidth(), H = fb.getHeight();
    screen.touchButtons.clear();

    // Passive user-facing summary only: no cursor, no A/Y/L/R actions, no mutation route.
    fb.drawFilledRect(0, 0, W, H, Color(0, 0, 0, 130));
    fb.drawVerticalGradient(0, 0, W, H,
        Color(Colors::Background.r, Colors::Background.g, Colors::Background.b, 250),
        Color(Colors::Background.r, Colors::Background.g, Colors::Background.b, 255));

    constexpr int workspaceX = 40, workspaceY = 31, workspaceW = 1200, workspaceH = 609;
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

    constexpr int contentY = workspaceY + 74, contentH = 516;
    constexpr int leftX = workspaceX + 18, leftW = 300;
    constexpr int midX = leftX + leftW + 14, midW = 398;
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

    // VALUES: four stored DVs plus derived HP DV; one Special DV/Stat Exp drives both split stats.
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
        fb.drawText(midX + 16, y, labels[static_cast<std::size_t>(i)], Colors::TextDim, TextStyle::Caption);
        fb.drawText(midX + 96, y, std::to_string(dvs[static_cast<std::size_t>(i)]) + (i == 0 ? " *" : ""),
                    Colors::Text, TextStyle::Caption);
        fb.drawText(midX + 192, y, std::to_string(statExp[static_cast<std::size_t>(i)]), Colors::Text, TextStyle::Caption);
        const std::string stat = i < 4 ? std::to_string(battleStats[static_cast<std::size_t>(i)])
                                      : "A" + std::to_string(battleStats[4]) + " / D" + std::to_string(battleStats[5]);
        fb.drawText(midX + 290, y, stat, Colors::Text, TextStyle::Caption);
    }
    fb.drawText(midX + 16, contentY + 322, "* HP DV derived", Colors::TextDim, TextStyle::Caption);
    compactRow(fb, midX + 16, contentY + 356, "Shiny", p.isShiny(0, {}) ? "Yes" : "No", 124);
    compactRow(fb, midX + 16, contentY + 390, "Level", std::to_string(p.level()), 124);
    fb.drawText(midX + 16, contentY + 426, "One Special DV + Stat Exp feeds SpA and SpD",
                Colors::TextDim, TextStyle::Caption);
    fb.drawText(midX + 16, contentY + 454,
                calculatedBoxStats ? "Box battle stats calculated / read-only" : "Party battle stats stored / read-only",
                Colors::TextDim, TextStyle::Caption);

    // MOVES: calculate exact-game compatibility fresh from current source game/species/four moves.
    fb.drawText(rightX + 14, contentY + 10, "MOVES", Colors::Text, TextStyle::Heading);
    std::array<uint8_t, 4> currentMoves{};
    for (int slot = 0; slot < 4; ++slot) {
        const int y = contentY + 50 + slot * 44;
        const uint16_t move = p.move(slot);
        currentMoves[static_cast<std::size_t>(slot)] = static_cast<uint8_t>(move);
        const std::string moveName = move == 0 ? "Empty" : std::string(Names::getMoveName(move));
        fb.drawText(rightX + 20, y, moveName, Colors::Text, TextStyle::Caption);
        const std::string pp = "PP " + std::to_string(p.movePP(slot)) + "  Up " + std::to_string(p.movePPUps(slot));
        int ppW = 0, ppH = 0;
        fb.measureText(pp, ppW, ppH, TextStyle::Caption);
        fb.drawText(rightX + rightW - 20 - ppW, y, pp, Colors::TextDim, TextStyle::Caption);
    }
    const auto exactGame = sourceGameFromId(screen.sourceGameId);
    const bool compatible = exactGame &&
        !Gen2::MoveCompatibility::firstIncompatible(*exactGame, p.speciesID(), currentMoves).has_value();
    compactRow(fb, rightX + 18, contentY + 235, "Move compatibility",
               compatible ? "OK" : "Unusual preserved", 300,
               compatible ? Colors::Success : Colors::Warning);
    compactRow(fb, rightX + 18, contentY + 261, "Encounter legality", "Not checked", 300, Colors::TextDim);

    // Native exact-format data + six-axis battle radar.
    constexpr int splitY = contentY + 288, splitH = 216, inset = 12, gap = 10;
    constexpr int innerW = rightW - inset * 2;
    constexpr int leftPaneW = 220;
    constexpr int rightPaneW = innerW - gap - leftPaneW;
    constexpr int leftPaneX = rightX + inset;
    constexpr int rightPaneX = leftPaneX + leftPaneW + gap;
    drawPanelSurface(fb, leftPaneX, splitY, leftPaneW, splitH, false, 12);
    drawPanelSurface(fb, rightPaneX, splitY, rightPaneW, splitH, false, 12);

    const auto* personal = Gen2::personalRecord(p.speciesID());
    const uint32_t nextExp = personal && p.level() < 100
        ? Pokemon::getExpForLevel(static_cast<uint8_t>(p.level() + 1), personal->experienceGrowth)
        : p.exp();
    const uint32_t expRemaining = nextExp > p.exp() ? nextExp - p.exp() : 0;

    fb.drawText(leftPaneX + 10, splitY + 8, "GEN II DATA", Colors::Accent, TextStyle::Caption);
    compactRow(fb, leftPaneX + 10, splitY + 34, "Game", sourceGameTitle(screen.sourceGameId), 82);
    compactRow(fb, leftPaneX + 10, splitY + 57, "Record", p.isPartyRecord() ? "Party" : "Box", 82);
    compactRow(fb, leftPaneX + 10, splitY + 80, "Growth", personal ? growthName(personal->experienceGrowth) : "Unknown", 82);
    compactRow(fb, leftPaneX + 10, splitY + 103, "EXP next", std::to_string(expRemaining), 82);
    compactRow(fb, leftPaneX + 10, splitY + 126, "Held", heldItemText(p.heldItem()), 82);
    compactRow(fb, leftPaneX + 10, splitY + 149, "Friendship", std::to_string(p.friendship()), 82);
    compactRow(fb, leftPaneX + 10, splitY + 172, "Pokerus", pokerusText(p.pokerusByte()), 82);
    if (screen.sourceGameId == "crystal_gbc")
        compactRow(fb, leftPaneX + 10, splitY + 195, "Caught/Met", std::to_string(p.caughtData()), 82);

    fb.drawText(rightPaneX + 10, splitY + 8, "BATTLE STATS", Colors::Accent, TextStyle::Caption);
    StatsRadar::drawGen2Labeled(fb, rightPaneX + 6, splitY + 30, rightPaneW - 12, splitH - 38, battleStats);

    screen.details.leftOrder.clear();
    screen.details.leftScroll = 0;
    drawNavBar(fb, {{"B", "Back"}});
}

} // namespace UI::Modals
