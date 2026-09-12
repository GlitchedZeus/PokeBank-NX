#include "UI/Modals/Gen2PokemonDetailsModal.h"

#include "Integration/Gen2/Gen2ReadOnlyInventory.h"
#include "Names/MoveNames.h"
#include "Pokemon/Pokemon2ReadOnly.h"
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

std::string typeText(const Pokemon::Pokemon2ReadOnly& p) {
    const char* first = gen2TypeName(p.rawType1());
    const char* second = gen2TypeName(p.rawType2());
    if (p.rawType1() == p.rawType2()) return first;
    return std::string(first) + " / " + second;
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
    return "Unknown";
}

std::string sourceLeafName(const std::string& path) {
    const std::size_t slash = path.find_last_of("/\\");
    const std::string leaf = slash == std::string::npos ? path : path.substr(slash + 1);
    constexpr std::size_t maxChars = 30;
    return leaf.size() <= maxChars ? leaf : leaf.substr(0, maxChars - 3) + "...";
}

std::string unownFormText(const Pokemon::Pokemon2ReadOnly& p) {
    if (p.speciesID() != 201) return "--";
    if (p.form() >= 26) return "Unknown";
    return std::string(1, static_cast<char>('A' + p.form()));
}

void valueRow(PKSEFramebuffer& fb, int x, int y, int w,
              const std::string& label, const std::string& value) {
    fb.drawText(x, y, label, Colors::TextDim, TextStyle::Body);
    int vw = 0, vh = 0;
    fb.measureText(value, vw, vh, TextStyle::Body);
    fb.drawText(x + w - vw, y, value, Colors::Text, TextStyle::Body);
}

} // namespace

void drawGen2PokemonDetailsModal(TrainerViewScreen& screen, PKSEFramebuffer& fb,
                                 const Pokemon::Pokemon2ReadOnly& p) {
    const int width = fb.getWidth();
    const int height = fb.getHeight();
    screen.touchButtons.clear();

    fb.drawFilledRect(0, 0, width, height, Color(0, 0, 0, 130));
    fb.drawVerticalGradient(0, 0, width, height,
        Color(Colors::Background.r, Colors::Background.g, Colors::Background.b, 250),
        Color(Colors::Background.r, Colors::Background.g, Colors::Background.b, 255));

    std::string name = Utils::utf16ToUtf8(p.nickname());
    if (name.empty()) name = p.species();
    fb.drawText(28, 16, name, Colors::Text, TextStyle::Heading);
    int nameW = 0, nameH = 0;
    fb.measureText(name, nameW, nameH, TextStyle::Heading);
    int markerX = 28 + nameW + 12;
    const char* gender = p.genderSymbol();
    if (gender[0] != '\0') {
        fb.drawSymbol(markerX, 22, gender,
                      p.gender() == 0 ? Colors::Blue : Colors::Magenta);
        markerX += 24;
    }
    if (p.isShiny(0, {})) fb.drawShinyMark(markerX, 20, 18, Colors::ShinyStar);

    std::string dex = std::to_string(p.speciesID());
    while (dex.size() < 3) dex = "0" + dex;
    const std::string subtitle = "Lv. " + std::to_string(p.level()) + "     No. " + dex;
    int subW = 0, subH = 0;
    fb.measureText(subtitle, subW, subH);
    fb.drawText(width - 90 - subW, 24, subtitle, Colors::TextDim);

    const char* readOnly = "GEN II  /  READ ONLY";
    int roW = 0, roH = 0;
    fb.measureText(readOnly, roW, roH, TextStyle::Caption);
    fb.drawText((width - roW) / 2, 22, readOnly, Colors::Accent, TextStyle::Caption);
    fb.drawFilledRect(0, 60, width, 2, Colors::Accent);

    fb.drawFilledCircle(width - 40, 30, 20, Colors::PanelAlt);
    fb.drawCircle(width - 40, 30, 20, Colors::Border, 1);
    fb.drawText(width - 47, 18, "\xC3\x97", Colors::Text, TextStyle::Heading);
    screen.touchButtons.push_back({99, width - 64, 6, 52, 52});

    const int colY = 76;
    const int colH = height - colY - kNavBarH - 10;
    const int leftX = 24, leftW = 398;
    const int midX = 438, midW = 402;
    const int rightX = 856, rightW = width - rightX - 24;
    drawPanelSurface(fb, leftX, colY, leftW, colH, false, 16);
    drawPanelSurface(fb, midX, colY, midW, colH, false, 16);
    drawPanelSurface(fb, rightX, colY, rightW, colH, false, 16);

    constexpr int renderSize = 142;
    const bool shiny = p.isShiny(0, {});
    Sprite* sprite = SpriteManager::getSprite(p.speciesID(), p.form(), shiny);
    if (sprite && sprite->data) {
        fb.drawSpriteStaticContained(leftX + (leftW - renderSize) / 2, colY + 12,
                                     renderSize, renderSize, sprite->width, sprite->height,
                                     sprite->data, sprite->channels);
    }
    fb.drawText(leftX + 18, colY + 162, typeText(p), Colors::Accent, TextStyle::Caption);

    int ly = colY + 194;
    const int leftValueW = leftW - 36;
    valueRow(fb, leftX + 18, ly, leftValueW, "Species", p.species()); ly += 31;
    valueRow(fb, leftX + 18, ly, leftValueW, "Nickname", Utils::utf16ToUtf8(p.nickname())); ly += 31;
    valueRow(fb, leftX + 18, ly, leftValueW, "Gender", genderText(p.gender())); ly += 31;
    valueRow(fb, leftX + 18, ly, leftValueW, "Shiny", shiny ? "Yes" : "No"); ly += 31;
    if (p.speciesID() == 201) {
        valueRow(fb, leftX + 18, ly, leftValueW, "Unown form", unownFormText(p)); ly += 31;
    }
    const std::string held = p.heldItem() == 0
        ? std::string("None")
        : std::string(PokeVault::Integration::Gen2::gen2ItemName(
              static_cast<uint8_t>(p.heldItem())));
    valueRow(fb, leftX + 18, ly, leftValueW, "Held Item", held); ly += 31;
    valueRow(fb, leftX + 18, ly, leftValueW, "Friendship", std::to_string(p.friendship())); ly += 31;
    valueRow(fb, leftX + 18, ly, leftValueW, "OT", Utils::utf16ToUtf8(p.otName())); ly += 31;
    valueRow(fb, leftX + 18, ly, leftValueW, "Trainer ID", std::to_string(p.tid16())); ly += 31;
    valueRow(fb, leftX + 18, ly, leftValueW, "Status byte", statusText(p.statusByte())); ly += 31;
    valueRow(fb, leftX + 18, ly, leftValueW, "Current source", sourceGameTitle(screen.sourceGameId)); ly += 31;
    valueRow(fb, leftX + 18, ly, leftValueW, "Platform", "Game Boy Color"); ly += 31;
    valueRow(fb, leftX + 18, ly, leftValueW, "Source file", sourceLeafName(screen.backupDir)); ly += 31;
    valueRow(fb, leftX + 18, ly, leftValueW, "Origin game", "Not stored in PK2");

    fb.drawText(midX + 18, colY + 16, "DVs / Stat Experience", Colors::Text, TextStyle::Heading);
    fb.drawText(midX + 18, colY + 49,
                "Native Gen II values - not modern IV / EV fields",
                Colors::TextDim, TextStyle::Caption);
    fb.drawText(midX + 166, colY + 82, "DV", Colors::TextDim, TextStyle::Caption);
    fb.drawText(midX + 250, colY + 82, "Stat Exp", Colors::TextDim, TextStyle::Caption);
    struct Gen2Stat { const char* label; unsigned dv; unsigned statExp; };
    const Gen2Stat stats[] = {
        {"HP", p.dvHP(), p.statExpHP()},
        {"Attack", p.dvATK(), p.statExpATK()},
        {"Defense", p.dvDEF(), p.statExpDEF()},
        {"Speed", p.dvSPE(), p.statExpSPE()},
        {"Special", p.dvSpecial(), p.statExpSpecial()},
    };
    int sy = colY + 112;
    for (const auto& stat : stats) {
        fb.drawText(midX + 18, sy, stat.label, Colors::TextDim);
        fb.drawText(midX + 166, sy, std::to_string(stat.dv), Colors::Text);
        fb.drawText(midX + 250, sy, std::to_string(stat.statExp), Colors::Text);
        sy += 37;
    }

    sy += 8;
    fb.drawText(midX + 18, sy, "Stored battle stats", Colors::Text, TextStyle::Heading);
    sy += 36;
    if (p.isPartyRecord()) {
        valueRow(fb, midX + 18, sy, midW - 36, "Current HP", std::to_string(p.statHPCurrent())); sy += 30;
        valueRow(fb, midX + 18, sy, midW - 36, "Max HP", std::to_string(p.statHPMax())); sy += 30;
        valueRow(fb, midX + 18, sy, midW - 36, "Attack", std::to_string(p.statATK())); sy += 30;
        valueRow(fb, midX + 18, sy, midW - 36, "Defense", std::to_string(p.statDEF())); sy += 30;
        valueRow(fb, midX + 18, sy, midW - 36, "Speed", std::to_string(p.statSPE())); sy += 30;
        valueRow(fb, midX + 18, sy, midW - 36, "Sp. Attack", std::to_string(p.statSPA())); sy += 30;
        valueRow(fb, midX + 18, sy, midW - 36, "Sp. Defense", std::to_string(p.statSPD()));
    } else {
        fb.drawText(midX + 18, sy, "Not stored in the 32-byte boxed PK2 record.", Colors::TextDim,
                    TextStyle::Caption);
    }

    fb.drawText(rightX + 18, colY + 16, "Moves", Colors::Text, TextStyle::Heading);
    int moveY = colY + 58;
    for (int slot = 0; slot < 4; ++slot) {
        const uint16_t move = p.move(slot);
        const std::string moveName = move == 0 ? std::string("-")
                                                : std::string(Names::getMoveName(move));
        fb.drawFilledRoundedRect(rightX + 14, moveY, rightW - 28, 82, 12, Colors::PanelAlt);
        fb.drawText(rightX + 28, moveY + 12, moveName, Colors::Text, TextStyle::Body);
        const std::string pp = "PP " + std::to_string(p.movePP(slot));
        const std::string ups = "PP Ups " + std::to_string(p.movePPUps(slot));
        fb.drawText(rightX + 28, moveY + 46, pp, Colors::TextDim, TextStyle::Caption);
        int upsW = 0, upsH = 0;
        fb.measureText(ups, upsW, upsH, TextStyle::Caption);
        fb.drawText(rightX + rightW - 28 - upsW, moveY + 46, ups,
                    Colors::TextDim, TextStyle::Caption);
        moveY += 94;
    }

    fb.drawText(rightX + 18, moveY + 5, "Record", Colors::TextDim, TextStyle::Caption);
    fb.drawText(rightX + 18, moveY + 27,
                p.isPartyRecord() ? "Party PK2 (48 bytes)" : "Box PK2 (32 bytes)",
                Colors::Text, TextStyle::Caption);
    fb.drawText(rightX + 18, moveY + 55, "Not present in Generation II:",
                Colors::Accent, TextStyle::Caption);
    fb.drawText(rightX + 18, moveY + 76, "SID, nature, ability, modern IV/EV,",
                Colors::TextDim, TextStyle::Caption);
    fb.drawText(rightX + 18, moveY + 96, "modern met/Ball data, ribbons/marks,",
                Colors::TextDim, TextStyle::Caption);
    fb.drawText(rightX + 18, moveY + 116, "HOME tracker or per-Pokemon language.",
                Colors::TextDim, TextStyle::Caption);

    screen.details.leftOrder.clear();
    screen.details.leftScroll = 0;
    drawNavBar(fb, {{"B", "Back"}});
}

} // namespace UI::Modals
