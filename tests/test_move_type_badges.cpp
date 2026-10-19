// R2-BUG-007: the Move chooser shows each move's type as the Pokémon summary's type badge
// (UI/ClassicTypeBadges.h, the romfs/sprites/types/<id>.png images), not as plain text.
// Host-only: a recording framebuffer and a SpriteManager stub with the real 200x40 badge size.
#include "UI/ClassicTypeBadges.h"
#include "UI/SharedMovesPanel.h"

#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <set>
#include <string>
#include <vector>

namespace {
struct Text { int x, y, w, h; std::string value; UI::Color color; };
struct Image { int x, y, w, h; int typeId; };
std::vector<Text> texts;
std::vector<Image> images;
std::array<std::array<uint8_t, 4>, 18> pixels{};   // one distinct buffer per type sprite
std::array<UI::Sprite, 18> typeSprites{};
bool spritesAvailable = true;

void require(bool ok, const std::string& what) {
    if (!ok) std::cerr << "FAIL: " << what << '\n';
    assert(ok);
}
bool overlapX(int ax, int aw, int bx, int bw) { return ax < bx + bw && bx < ax + aw; }
} // namespace

namespace UI {
PKSEFramebuffer::PKSEFramebuffer() = default;
PKSEFramebuffer::~PKSEFramebuffer() = default;
void PKSEFramebuffer::measureText(const std::string& s, int& w, int& h, TextStyle) {
    w = static_cast<int>(s.size()) * 9; h = 18;
}
void PKSEFramebuffer::drawText(int x, int y, const std::string& s, Color c, TextStyle style) {
    int w = 0, h = 0; measureText(s, w, h, style);
    texts.push_back({x, y, w, h, s, c});
}
void PKSEFramebuffer::drawText(int x, int y, const char* s, Color c, TextStyle style) {
    drawText(x, y, std::string(s), c, style);
}
void PKSEFramebuffer::drawImageScaled(int x, int y, int, int, int w, int h, const uint8_t* data, int) {
    int id = -1;
    for (int i = 0; i < 18; ++i)
        if (data == pixels[static_cast<std::size_t>(i)].data()) id = i;
    images.push_back({x, y, w, h, id});
}
Sprite::~Sprite() = default;   // the test's sprites point at static buffers
Sprite* SpriteManager::getTypeSprite(uint8_t typeId) {
    if (!spritesAvailable || typeId >= 18) return nullptr;
    auto& s = typeSprites[typeId];
    s.width = 200; s.height = 40; s.channels = 4; s.data = pixels[typeId].data();
    return &s;
}
} // namespace UI

namespace {
namespace MP = UI::SharedMovesPanel;
namespace Facts = PokeBank::UIModel::SharedMoveFacts;
namespace Badges = UI::ClassicTypeBadges;
using G = Enums::GameVersion;

constexpr auto kColumns = MP::chooserColumns((1280 - 760) / 2 + 40, 760 - 80);

std::set<int> typesUpTo(int last, std::set<int> skip = {}) {
    std::set<int> out;
    for (int t = 0; t <= last; ++t) if (!skip.count(t)) out.insert(t);
    return out;
}

const Text* findText(const std::string& value) {
    for (const auto& t : texts) if (t.value == value) return &t;
    return nullptr;
}

void testEveryGen15MoveTypeIsABadge() {
    struct Era { G game; Facts::Era era; std::set<int> expected; };
    // Gen I has 15 types (no Steel = 8, no Dark = 16); Gen II-V have 17. Fairy (17) never appears.
    const std::vector<Era> eras{
        {G::RBY, Facts::Era::Gen1, typesUpTo(15, {8})},
        {G::GSC, Facts::Era::Gen2, typesUpTo(16)},
        {G::FRLG, Facts::Era::Gen3, typesUpTo(16)},
        {G::HGSS, Facts::Era::Gen4, typesUpTo(16)},
        {G::B2W2, Facts::Era::Gen5, typesUpTo(16)},
    };
    UI::PKSEFramebuffer fb;
    for (const auto& e : eras) {
        std::set<int> seen;
        int textTypes = 0;
        for (uint16_t move = 1; move <= Facts::maxMoveId(e.era); ++move) {
            const auto f = Facts::facts(move, e.game);
            if (!f.exists) continue;
            texts.clear(); images.clear();
            MP::drawChooserRow(fb, kColumns, 100, move, e.game, move % 2 == 0);
            if (f.typeVaries || f.type > Badges::kLastGen15Type) {
                // Hidden Power ("Varies") and Curse ("???", Gen II-IV) keep a text label.
                require(images.empty(), "no badge for a varying / unknown type");
                ++textTypes;
                continue;
            }
            require(images.size() == 1, "exactly one badge per move row");
            const auto& b = images.front();
            require(b.typeId == f.type, "badge is the move's own type sprite");
            require(b.typeId != 17, "Gen I-V never draws a Fairy badge");
            require(b.h == MP::kChooserBadgeHeight && b.w == 80, "badge keeps the 5:1 sprite ratio");
            // Alignment: badge sits in the Type column, clear of Category / Pwr and the name.
            require(b.x == kColumns.typeX(), "badge starts at the Type column");
            require(b.x + b.w <= kColumns.categoryX() - 8, "badge stays clear of the Category column");
            for (const auto& t : texts) {
                require(!overlapX(t.x, t.w, b.x, b.w) || t.y + t.h <= b.y || t.y >= b.y + b.h,
                        "no text under the badge: " + t.value);
                require(t.x != kColumns.typeX(), "type is not also drawn as text: " + t.value);
            }
            // Number, name and PP keep their columns.
            const auto* number = findText(MP::moveNumber(move));
            require(number && number->x == kColumns.numberX(), "number column unchanged");
            const std::string pp = f.basePP > 0 ? std::to_string(f.basePP) : "\xE2\x80\x94";
            bool ppAligned = false;   // Pwr can equal PP, so look for the PP cell by its edge
            for (const auto& t : texts) ppAligned |= t.value == pp && t.x + t.w == kColumns.ppRight();
            require(ppAligned, "PP right-aligned in its column");
            seen.insert(f.type);
        }
        require(seen == e.expected, "every native type of the era is drawn as a badge");
        require(textTypes <= 2, "only Hidden Power / Curse fall back to text");
    }
    std::cout << "every Gen I-V move type is a badge ok\n";
}

void testFallbackWhenSpriteMissing() {
    UI::PKSEFramebuffer fb;
    spritesAvailable = false;
    texts.clear(); images.clear();
    MP::drawChooserRow(fb, kColumns, 100, 44 /* Bite */, G::GSC, true);
    require(images.empty() && findText("Dark"), "missing artwork falls back to the type name");
    spritesAvailable = true;
    // The Empty row draws neither badge nor type.
    texts.clear(); images.clear();
    MP::drawChooserRow(fb, kColumns, 100, 0, G::GSC, true);
    require(images.empty() && findText("Empty move slot"), "Empty row unchanged");
    // Bite is Normal in Gen I and Dark in Gen II (native type, not a cross-gen assumption).
    texts.clear(); images.clear();
    MP::drawChooserRow(fb, kColumns, 100, 44, G::RBY, true);
    require(images.size() == 1 && images[0].typeId == 0, "Gen I Bite badge is Normal");
    texts.clear(); images.clear();
    MP::drawChooserRow(fb, kColumns, 100, 44, G::GSC, true);
    require(images.size() == 1 && images[0].typeId == 16, "Gen II Bite badge is Dark");
    // Fairy and out-of-range ids are refused by the shared badge itself.
    images.clear();
    require(Badges::drawCanonical(fb, 0, 0, 17, 16) == 0 && images.empty(), "Fairy badge refused");
    require(Badges::drawCanonical(fb, 0, 0, 255, 16) == 0, "unknown type refused");
    // Classic (Gen I/II save) ids reach the same canonical sprite.
    images.clear();
    Badges::drawSprite(fb, 0, 0, 27 /* Gen II Dark */, 16);
    require(images.size() == 1 && images[0].typeId == 16, "classic Dark -> canonical Dark sprite");
    std::cout << "fallback / native types ok\n";
}

void testBadgesAcrossThemes() {
    UI::PKSEFramebuffer fb;
    using UI::ThemeMode;
    for (const ThemeMode mode : {ThemeMode::OLEDBlack, ThemeMode::Dark, ThemeMode::Light, ThemeMode::Red,
                                 ThemeMode::Blue, ThemeMode::Green, ThemeMode::Gray, ThemeMode::PokeClassic,
                                 ThemeMode::Purple, ThemeMode::Orange, ThemeMode::PS5, ThemeMode::Xbox,
                                 ThemeMode::PokemonHome, ThemeMode::Switch, ThemeMode::Midnight,
                                 ThemeMode::Graphite, ThemeMode::Arctic, ThemeMode::Ivory, ThemeMode::Aurora,
                                 ThemeMode::Sunset}) {
        UI::applyTheme(mode);
        texts.clear(); images.clear();
        MP::drawChooserRow(fb, kColumns, 100, 85 /* Thunderbolt */, G::B2W2, true);
        // Same artwork and geometry in every theme: the badge carries no theme palette of its own.
        require(images.size() == 1 && images[0].typeId == 12 && images[0].x == kColumns.typeX() &&
                    images[0].w == 80 && images[0].h == MP::kChooserBadgeHeight,
                "theme does not change the badge");
        const auto* name = findText("Thunderbolt");
        require(name && name->color == UI::Colors::Text, "selected name uses the theme's text colour");
        texts.clear(); images.clear();
        MP::drawChooserRow(fb, kColumns, 100, 85, G::B2W2, false);
        name = findText("Thunderbolt");
        require(name && name->color == UI::Colors::TextSecondary, "unselected name uses the secondary colour");
    }
    UI::applyTheme(UI::ThemeMode::Dark);
    std::cout << "badges across 20 themes ok\n";
}

} // namespace

int main() {
    testEveryGen15MoveTypeIsABadge();
    testFallbackWhenSpriteMissing();
    testBadgesAcrossThemes();
    std::cout << "test_move_type_badges: all passed\n";
    return 0;
}
