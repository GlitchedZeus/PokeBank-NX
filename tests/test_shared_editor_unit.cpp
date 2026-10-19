// Shared Gen I-V editor unit: Moves panel + Move chooser layout bounds, generation-correct
// move facts, picker footer, inline toggles, DETAILS viewport, and source contracts that the
// real Gen III / IV / V surfaces use these shared pieces (not a host-only mock).
#include "Names/LocationDisplayNames.h"
#include "UI/ClassicGenericUiGuard.h"
#include "UI/MovePickerPresentation.h"
#include "UI/SharedEditorPresentation.h"
#include "UI/SharedMoveFacts.h"
#include "UI/SharedMovesPanel.h"
#include "UI/SharedPokemonShell.h"

#include <cassert>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <set>
#include <initializer_list>
#include <string>
#include <utility>
#include <vector>

namespace {
struct Text { int x, y, w, h; std::string value; };
struct Rect { int x, y, w, h; };
std::vector<Text> texts;
std::vector<Rect> rects;

bool overlaps(const Text& a, const Text& b) {
    return a.x < b.x + b.w && a.x + a.w > b.x && a.y < b.y + b.h && a.y + a.h > b.y;
}
std::string read(const char* path) {
    std::ifstream in(path, std::ios::binary);
    assert(in && "run from the repository root");
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}
void require(bool ok, const std::string& what) {
    if (!ok) std::cerr << "FAILED: " << what << '\n';
    assert(ok);
}
} // namespace

namespace UI {
PKSEFramebuffer::PKSEFramebuffer() = default;
PKSEFramebuffer::~PKSEFramebuffer() = default;
// Generous stub metrics: ~9px per byte (wider than Nunito Caption), 20px line height.
void PKSEFramebuffer::measureText(const std::string& s, int& w, int& h, TextStyle style) {
    w = static_cast<int>(s.size()) * (style == TextStyle::Body ? 10 : 8);
    h = style == TextStyle::Body ? 22 : 18;
}
void PKSEFramebuffer::drawText(int x, int y, const std::string& s, Color, TextStyle style) {
    int w = 0, h = 0; measureText(s, w, h, style); texts.push_back({x, y, w, h, s});
}
void PKSEFramebuffer::drawText(int x, int y, const char* s, Color c, TextStyle style) {
    drawText(x, y, std::string(s), c, style);
}
void PKSEFramebuffer::drawFilledRect(int x, int y, int w, int h, Color) { rects.push_back({x, y, w, h}); }
void PKSEFramebuffer::drawFilledRoundedRect(int x, int y, int w, int h, int, Color) { rects.push_back({x, y, w, h}); }
void PKSEFramebuffer::drawRoundedRect(int x, int y, int w, int h, int, Color, int) { rects.push_back({x, y, w, h}); }
void PKSEFramebuffer::drawFilledCircle(int x, int y, int r, Color) { rects.push_back({x - r, y - r, 2 * r, 2 * r}); }
void PKSEFramebuffer::setClipRect(int, int, int, int) {}
void PKSEFramebuffer::clearClip() {}
// R2-BUG-007: the chooser's Type cell asks for the type badge sprite; with no artwork on the host
// the shared badge draws nothing and the row falls back to the type name.
void PKSEFramebuffer::drawImageScaled(int, int, int, int, int, int, const uint8_t*, int) {}
Sprite* SpriteManager::getTypeSprite(uint8_t) { return nullptr; }
} // namespace UI

static UI::PKSEFramebuffer& fb_global() {
    static UI::PKSEFramebuffer fb;
    return fb;
}

namespace Facts = PokeBank::UIModel::SharedMoveFacts;
namespace P = PokeBank::UIModel::SharedEditorPresentation;
using Enums::GameVersion;

static void testMoveFacts() {
    // Moves never exist outside their generation's table.
    require(Facts::moveExists(354, GameVersion::FRLG) && !Facts::moveExists(355, GameVersion::FRLG), "Gen III ends at Psycho Boost");
    require(Facts::moveExists(467, GameVersion::HGSS) && !Facts::moveExists(468, GameVersion::PT), "Gen IV ends at Shadow Force");
    require(Facts::moveExists(559, GameVersion::B2W2) && !Facts::moveExists(560, GameVersion::BW), "Gen V ends at Fusion Bolt");
    require(!Facts::facts(400, GameVersion::FRLG).exists, "a Gen IV move has no Gen III facts");

    // Gen III: type only, never a per-move category (it follows the type).
    const auto bite3 = Facts::facts(44, GameVersion::FRLG);
    require(bite3.exists && bite3.type == 16 && bite3.category == -1, "Gen III Bite is Dark with no per-move category");
    require(Facts::detailLine(bite3).find("Physical") == std::string::npos &&
            Facts::detailLine(bite3).find("Special") == std::string::npos, "Gen III detail never shows a category");
    require(Facts::categoryRule(Facts::eraFor(GameVersion::FRLG)) == Facts::CategoryRule::None, "Gen III has no category rule");
    // Gen IV / V: per-move category.
    const auto bite4 = Facts::facts(44, GameVersion::DP);
    require(bite4.type == 16 && bite4.category == 0 && std::string(Facts::categoryLabel(bite4)) == "Physical", "Gen IV Bite is Physical");
    require(Facts::facts(85, GameVersion::HGSS).category == 1, "Gen IV Thunderbolt is Special");
    require(Facts::facts(85, GameVersion::BW).power == 95, "Gen V Thunderbolt power 95");
    // Gen I / II types from pinned pret moves.asm: Gen I has Karate Chop, Gust, Sand Attack and
    // Bite as Normal; Gen II matches Gen III for 1..251. No per-move category before Gen IV.
    for (const auto& [move, gen1, gen2] : std::array<std::array<int, 3>, 8>{{
             {2, 0, 1}, {16, 0, 2}, {28, 0, 4}, {44, 0, 16}, {85, 12, 12}, {53, 9, 9}, {94, 13, 13}, {57, 10, 10}}}) {
        require(Facts::facts(static_cast<uint16_t>(move), GameVersion::RBY).type == gen1, "Gen I type " + std::to_string(move));
        require(Facts::facts(static_cast<uint16_t>(move), GameVersion::GSC).type == gen2, "Gen II type " + std::to_string(move));
    }
    require(Facts::typeLabel(Facts::facts(174, GameVersion::GSC)) == "???" &&
            Facts::typeLabel(Facts::facts(237, GameVersion::GSC)) == "Varies" &&
            Facts::typeLabel(Facts::facts(185, GameVersion::GSC)) == "Dark", "Gen II Curse / Hidden Power / Feint Attack");
    require(Facts::facts(85, GameVersion::RBY).category == -1 && Facts::categoryLabel(Facts::facts(85, GameVersion::GSC))[0] == '\0',
            "Gen I/II show no per-move category");
    require(!Facts::moveExists(166, GameVersion::RBY) && !Facts::moveExists(252, GameVersion::GSC), "Gen I/II move bounds");
    for (uint16_t m = 1; m <= 165; ++m)
        require(Facts::facts(m, GameVersion::RBY).type != Facts::kNoType, "every Gen I move has a type");
    // Curse is ??? before Gen V and Ghost in Gen V.
    require(Facts::typeLabel(Facts::facts(174, GameVersion::PT)) == "???", "Gen IV Curse is ???");
    require(Facts::typeLabel(Facts::facts(174, GameVersion::B2W2)) == "Ghost", "Gen V Curse is Ghost");
    // Hidden Power's type and power are IV-derived: shown as Varies / em dash, never a number.
    for (auto game : {GameVersion::FRLG, GameVersion::DP, GameVersion::BW}) {
        const auto hp = Facts::facts(237, game);
        require(Facts::typeLabel(hp) == "Varies" && Facts::powerLabel(hp) == "\xE2\x80\x94", "Hidden Power varies");
    }
    // Variable-damage moves are not shown as power 1.
    require(Facts::powerLabel(Facts::facts(69, GameVersion::FRLG)) == "\xE2\x80\x94", "Seismic Toss has no base power");
    // Max PP with PP Ups.
    require(Facts::maxPP(35, 3) == 56 && Facts::maxPP(5, 3) == 8 && Facts::maxPP(40, 0) == 40, "max PP formula");
    // Gen I/II: per-PP-Up bonus capped at 7 (pret ComputeMaxPP; Gen2StagedEditor maxPP).
    using Facts::Era;
    require(Facts::maxPP(40, 3, Era::Gen2) == 61 && Facts::maxPP(40, 3, Era::Gen1) == 61, "Gen I/II 40 PP + 3 Ups = 61");
    require(Facts::maxPP(40, 3, Era::Gen3) == 64, "Gen III 40 PP + 3 Ups = 64");
    require(Facts::maxPP(35, 3, Era::Gen2) == 56 && Facts::maxPP(5, 3, Era::Gen1) == 8, "Gen I/II below the cap match");
    for (int base = 0; base <= 40; base += 5)
        for (uint8_t ups = 0; ups <= 3; ++ups)
            require(Facts::maxPP(base, ups, Era::Gen2) == std::min(63, base + std::min(7, base / 5) * ups),
                    "Gen II formula");
}

static void testMovesPanelBounds() {
    constexpr UI::SharedPokemonShell::Geometry g(1280, 720, 56);
    const auto l = UI::SharedMovesPanel::layout(g.rightX, g.y, g.rightW);
    require(l.bottom() <= g.y + UI::SharedMovesPanel::kDataPaneOffset - 4, "moves rows end above the data pane");
    require(l.nameMaxW() >= 200, "move names have room");
    {
        // S01 (owner round 1): "PP" and "PP Ups" captions were too close to read.
        namespace Geo = PokeBank::UIModel::SharedMenuGeometry;
        const int ppCaptionRight = l.ppRight();
        const int upsCaptionLeft = l.upsRight() - Geo::textWidth("PP Ups", Geo::Text::Caption);
        require(upsCaptionLeft - ppCaptionRight >= 20, "S01: PP and PP Ups captions are at least 20 px apart");
        require(l.ppRight() - Geo::textWidth("63/63", Geo::Text::Caption) >= l.nameX() + l.nameMaxW(),
                "S01: PP values never reach the move name column");
    }
    for (auto game : {GameVersion::RBY, GameVersion::GSC, GameVersion::FRLG, GameVersion::DP,
                      GameVersion::PT, GameVersion::HGSS, GameVersion::BW, GameVersion::B2W2}) {
        const uint16_t last = Facts::maxMoveId(Facts::eraFor(game));
        for (int focus = -1; focus < 4; ++focus) {
            texts.clear(); rects.clear();
            std::array<UI::SharedMovesPanel::Slot, 4> slots{{
                {last, 64, 3, -1, "Preserved", UI::Colors::Warning},
                {237, 15, 0, -1, "Transfer", UI::Colors::Warning},
                {0, 0, 0, -1, "", UI::Colors::TextDim},
                {44, 40, 3, 56, "OK", UI::Colors::Success},
            }};
            UI::SharedMovesPanel::draw(fb_global(), l, slots, game, focus);
            for (std::size_t i = 0; i < texts.size(); ++i) {
                const auto& t = texts[i];
                require(t.x >= g.rightX + 8 && t.x + t.w <= g.rightX + g.rightW - 8, "moves text inside panel: " + t.value);
                require(t.y >= g.y + 30 && t.y + t.h <= g.y + UI::SharedMovesPanel::kDataPaneOffset, "moves text above data pane: " + t.value);
                for (std::size_t j = i + 1; j < texts.size(); ++j)
                    require(!overlaps(t, texts[j]), "moves text overlap: " + t.value + " / " + texts[j].value);
            }
            for (const auto& r : rects) {
                require(r.x >= g.rightX && r.x + r.w <= g.rightX + g.rightW, "focus inside panel");
                require(r.y + r.h <= g.y + UI::SharedMovesPanel::kDataPaneOffset, "focus above data pane");
            }
            require((focus >= 0) == !rects.empty(), "focus outline only when focused");
            bool emptyShown = false, ppUps = false, category = false;
            for (const auto& t : texts) {
                emptyShown |= t.value == "Empty";
                ppUps |= t.value == "+3";
                category |= t.value.find("Physical") != std::string::npos ||
                            t.value.find("Special") != std::string::npos;
            }
            require(emptyShown && ppUps, "Empty slot and PP Ups visible");
            require(category == (Facts::categoryRule(Facts::eraFor(game)) == Facts::CategoryRule::PerMove),
                    "category only where the generation has it");
        }
    }
}

static void testChooserBounds() {
    // Same geometry the shared and Gen III pickers use: compact picker x+40, width w-80.
    constexpr auto pl = PokeBank::UIModel::MovePickerPresentation::compactPickerLayout();
    const int x = (1280 - pl.width) / 2;
    const auto c = UI::SharedMovesPanel::chooserColumns(x + 40, pl.width - 80);
    for (auto game : {GameVersion::RBY, GameVersion::GSC, GameVersion::FRLG, GameVersion::DP,
                      GameVersion::HGSS, GameVersion::BW}) {
        texts.clear();
        UI::SharedMovesPanel::drawChooserCaptions(fb_global(), c, 58, game);
        const auto captions = texts;
        for (std::size_t i = 0; i < captions.size(); ++i)
            for (std::size_t j = i + 1; j < captions.size(); ++j)
                require(!overlaps(captions[i], captions[j]), "chooser caption overlap");
        const uint16_t last = Facts::maxMoveId(Facts::eraFor(game));
        for (uint16_t move = 0; move <= last; ++move) {
            texts.clear();
            UI::SharedMovesPanel::drawChooserRow(fb_global(), c, 100, move, game, move == 1);
            for (std::size_t i = 0; i < texts.size(); ++i) {
                require(texts[i].x >= x + 24 && texts[i].x + texts[i].w <= x + pl.width - 24, "chooser text inside modal");
                require(texts[i].h <= pl.highlightHeight - 6, "chooser text inside the row highlight");
                for (std::size_t j = i + 1; j < texts.size(); ++j)
                    require(!overlaps(texts[i], texts[j]), "chooser overlap in move " + std::to_string(move));
            }
            if (Facts::categoryRule(Facts::eraFor(game)) == Facts::CategoryRule::None)
                for (const auto& t : texts)
                    require(t.value != "Physical" && t.value != "Special" && t.value != "Status",
                            "Gen III chooser never shows a per-move category");
        }
    }
}

static void testFooterAndToggles() {
    for (const auto& spec : {P::PickerFooterSpec{}, P::PickerFooterSpec{true, true, true, true, "Sort", "Normal/Shiny"},
                             P::PickerFooterSpec{false, false, false, false, nullptr, nullptr}}) {
        const auto f = P::pickerFooter(spec);
        require(f.count >= 2, "footer has hints");
        require(std::string(f.hints[f.count - 2].glyph) == "A" && std::string(f.hints[f.count - 2].label) == "Select", "A Select");
        require(std::string(f.hints[f.count - 1].glyph) == "B" && std::string(f.hints[f.count - 1].label) == "Cancel", "B Cancel");
    }
    const auto confirm = P::confirmFooter("A", "Save", "Y", "Discard");
    require(confirm.has("B", "Back") && confirm.has("Y", "Discard") && confirm.has("A", "Save") && confirm.count == 3, "confirm footer");

    // Pokerus cycles None -> Infected -> Cured -> None and every byte is native-valid.
    static_assert(P::pokerusState(0x00) == P::PokerusState::None);
    static_assert(P::pokerusState(0x13) == P::PokerusState::Infected);
    static_assert(P::pokerusState(0x30) == P::PokerusState::Cured);
    for (unsigned baseline = 0; baseline < 256; ++baseline) {
        if (!P::pokerusByteValid(static_cast<uint8_t>(baseline))) continue;
        uint8_t value = static_cast<uint8_t>(baseline);
        for (int step = 0; step < 6; ++step) {
            const auto before = P::pokerusState(value);
            value = P::nextPokerusByte(value, static_cast<uint8_t>(baseline));
            require(P::pokerusByteValid(value), "Pokerus byte valid");
            const auto after = P::pokerusState(value);
            require((before == P::PokerusState::None && after == P::PokerusState::Infected) ||
                    (before == P::PokerusState::Infected && after == P::PokerusState::Cured) ||
                    (before == P::PokerusState::Cured && after == P::PokerusState::None), "Pokerus cycle order");
        }
        require(value == baseline, "three full cycles return the untouched native byte");
    }
    // Gender: only species-valid values.
    static_assert(P::genderChoices(255).count == 1 && P::genderChoices(255).values[0] == 2);
    static_assert(P::genderChoices(254).count == 1 && P::genderChoices(254).values[0] == 1);
    static_assert(P::genderChoices(0).count == 1 && P::genderChoices(0).values[0] == 0);
    static_assert(P::nextGender(31, 0) == 1 && P::nextGender(31, 1) == 0 && P::nextGender(255, 0) == 2);
    // Gen III language cycle never offers id 6.
    uint8_t language = 1;
    for (int i = 0; i < 12; ++i) {
        language = P::nextGen3Language(language);
        require(language >= 1 && language <= 7 && language != 6, "Gen III language ids");
    }
    require(P::nextGen3Language(5) == 7 && P::nextGen3Language(7) == 1, "Gen III language order");
    require(std::string(P::legalityLabel(P::LegalityState::Unknown)) == "Unknown" &&
            std::string(P::legalityLabel(P::LegalityState::Incomplete)) == "Incomplete", "honest legality labels");
}

static void testDetailsViewport() {
    constexpr UI::SharedPokemonShell::Geometry g(1280, 720, 56);
    constexpr auto v = UI::SharedPokemonShell::detailsViewport(g);
    static_assert(v.visibleRows == 8);
    static_assert(v.bottom() <= g.y + g.h - 8 && v.bottom() < 720 - 56);
    const char* labels[15] = {"Species", "Nickname", "Level", "Experience", "OT", "Trainer ID", "SID",
        "Held Item", "Friendship", "Language", "Origin", "Ball", "Met Level", "Met Location", "Pokerus"};
    for (std::size_t focus = 0; focus < 15; ++focus) {
        texts.clear(); rects.clear();
        UI::SharedPokemonShell::drawScrollableDetails(fb_global(), g.leftX, g.y, g.leftW, g.h, 15, focus, true,
            [&](std::size_t row) { return std::string(labels[row]); },
            [](std::size_t row) { return "Value " + std::to_string(row); });
        bool focusedVisible = false;
        for (const auto& t : texts) {
            require(t.value.rfind("Rows ", 0) != 0, "no Rows N-M/T counter");
            require(t.y >= v.top && t.y + t.h <= v.bottom(), "row text inside the viewport");
            focusedVisible |= t.value == labels[focus];
        }
        require(focusedVisible, "first..last row reachable and visible when focused");
    }
}

static void testOriginPaneBounds() {
    // The Gen IV pane is the longest (13 rows incl. Legality); nothing may run past the panel.
    constexpr UI::SharedPokemonShell::Geometry g(1280, 720, 56);
    struct Row { std::string label, value; };
    const std::vector<Row> rows{{"Game", "HeartGold"}, {"Origin", "SoulSilver"}, {"Language", "English"},
        {"Ball", "Poke Ball"}, {"Met", "Route 29 / Lv. 5"}, {"Friendship", "255"},
        {"Pok\xC3\xA9rus", "Infected"}, {"Fateful", "No"}, {"Egg status", "Not egg"},
        {"OT gender", "Female"}, {"PC markings", "\xE2\x97\x8F\xE2\x96\xB2"},
        {"Legality", P::legalityLabel(P::LegalityState::Incomplete)}};
    texts.clear(); rects.clear();
    UI::SharedPokemonShell::drawDataAndGraph(fb_global(), g.rightX, g.y + 244, g.rightW, g.h - 256,
        P::kOriginLegalityHeading, rows, {301, 200, 180, 245, 279, 185});
    bool heading = false;
    for (const auto& t : texts) {
        require(t.y + t.h <= g.y + g.h - 8, "origin pane text above the panel edge: " + t.value);
        heading |= t.value == P::kOriginLegalityHeading;
    }
    require(heading, "Origin & Legality heading drawn");
}

static void testDataPaneWrap() {
    // Phase 9: over-long values wrap inside the 198px data pane; a no-space word is broken on a
    // code point; overflow ends in "..." -- never into the graph pane or below the panel.
    constexpr UI::SharedPokemonShell::Geometry g(1280, 720, 56);
    struct Row { std::string label, value; };
    const int paneX = g.rightX + 12, paneRight = paneX + 198;
    for (int extra = 0; extra < 12; ++extra) {
        std::vector<Row> rows{{"Met Location", "Altering Cave (Sevii Islands)"},
            {"Encounter origin", "Bulbasaur \xE2\x86\x92 Venusaur"},
            {"Long", std::string(40, 'W') + "\xC3\xA9\xC3\xA9\xC3\xA9"},
            {"Legality", "Not checked"}};
        for (int i = 0; i < extra; ++i) rows.push_back({"Row " + std::to_string(i), "Pok\xC3\xA9mon League Front Gate"});
        texts.clear(); rects.clear();
        UI::SharedPokemonShell::drawDataAndGraph(fb_global(), g.rightX, g.y + 244, g.rightW, g.h - 256,
            P::kOriginLegalityHeading, rows, {1, 2, 3, 4, 5, 6});
        bool ellipsis = false;
        for (const auto& t : texts) {
            if (t.x >= paneRight) continue; // graph pane labels
            require(t.x >= paneX && t.x + t.w <= paneRight - 10, "data text inside the pane: " + t.value);
            require(t.y + t.h <= g.y + g.h - 8, "data text above the panel edge: " + t.value);
            for (std::size_t i = 0; i < t.value.size(); ++i)
                if ((static_cast<unsigned char>(t.value[i]) & 0xE0) == 0xC0)
                    require(i + 1 < t.value.size(), "no split UTF-8 sequence");
            ellipsis |= t.value.size() >= 3 && t.value.compare(t.value.size() - 3, 3, "...") == 0;
        }
        if (extra >= 8) require(ellipsis, "overflowing pane ends in an ellipsis");
    }
}

static void testFieldAccessParity() {
    // Phase 12: one editable set wherever the format allows it.
    namespace SE = PokeBank::UIModel::SharedPokemonEditor;
    using FI = SE::FieldIdentity;
    for (const auto gen : {SE::Generation::Gen3, SE::Generation::Gen4, SE::Generation::Gen5})
        for (const auto field : {FI::Species, FI::Nickname, FI::Nature, FI::Gender, FI::Shiny, FI::Ability,
                                 FI::Form, FI::IV, FI::EV, FI::HeldItem, FI::Pokerus, FI::Ball,
                                 FI::MetLevel, FI::MetLocation, FI::Move, FI::MovePP, FI::MovePPUps,
                                 FI::TrainerId, FI::OriginalTrainer, FI::Level, FI::Experience})
            require(SE::fieldAccessForGeneration(gen, field) == SE::FieldAccess::Editable,
                    "Gen III-V editable parity for field " + std::to_string(static_cast<int>(field)));
    for (const auto gen : {SE::Generation::Gen1, SE::Generation::Gen2})
        for (const auto field : {FI::Move, FI::MovePP, FI::MovePPUps})
            require(SE::fieldAccessForGeneration(gen, field) == SE::FieldAccess::Editable, "Gen I/II moves editable");
    // The PID itself is never typed in; Gen V language has no verified native mutation.
    for (const auto gen : {SE::Generation::Gen3, SE::Generation::Gen4, SE::Generation::Gen5})
        require(SE::fieldAccessForGeneration(gen, FI::PersonalityId) == SE::FieldAccess::ReadOnly, "PID read-only");
    require(SE::fieldAccessForGeneration(SE::Generation::Gen5, FI::Language) == SE::FieldAccess::ReadOnly,
            "Gen V language read-only with a reason");
}

static void testLocationNames() {
    // Gen III: no PKHeX source tags or raw ids; duplicates only for the SAME place.
    const std::set<std::string> samePlace{"Birth Island", "Fiery Path", "Jagged Pass",
                                          "Meteor Falls", "Navel Rock"};
    for (uint8_t origin = 1; origin <= 5; ++origin) {
        std::map<std::string, int> seen;
        for (uint16_t id = 0; id < 256; ++id) {
            const std::string name = Names::getMetLocationDisplayName(origin, id);
            for (const char* tag : {"(RSE)", "(FRLG)", "(RS)", "(E)", "(unused)", "ID ", " / "})
                require(name.find(tag) == std::string::npos, "Gen III location tag: " + name);
            const char* raw = Names::getMetLocationName(origin, id);
            if (id == 0 || !raw || !*raw) continue;
            if (++seen[name] > 1) require(samePlace.count(name) == 1, "Gen III duplicate location: " + name);
        }
    }
    require(Names::getMetLocationDisplayName(3, 57) == "Safari Zone (Hoenn)" &&
            Names::getMetLocationDisplayName(4, 136) == "Safari Zone (Kanto)", "Safari Zone disambiguated");
    require(Names::getMetLocationDisplayName(3, 58) == "Battle Frontier" &&
            Names::getMetLocationDisplayName(1, 58) == "Battle Tower", "Battle Tower/Frontier by origin");
    require(Names::getMetLocationDisplayName(1, 0) == "None", "no met location");
    // Gen IV / V: never a raw id, even for unknown or placeholder entries.
    for (const uint8_t origin : {uint8_t{10}, uint8_t{12}, uint8_t{7}, uint8_t{20}, uint8_t{22}})
        for (uint16_t id : {uint16_t{0}, uint16_t{1}, uint16_t{50}, uint16_t{2000}, uint16_t{3002},
                            uint16_t{30001}, uint16_t{60002}, uint16_t{65000}}) {
            const std::string name = Names::getMetLocationDisplayName(origin, id);
            require(!name.empty() && name.find("ID ") == std::string::npos &&
                    name.find('\xEF') == std::string::npos, "Gen IV/V location shows no raw id");
        }
}

static void testRealSourcesUseSharedUnit() {
    const std::string gen3 = read("src/UI/Gen3SharedPokemonSurface.inc");
    const std::string shared = read("src/UI/SharedPokemonEditorSurface.inc");
    const std::string species = read("include/UI/SharedSpeciesPicker.h");
    for (const auto* text : {&gen3, &shared}) {
        require(text->find("SharedMovesPanel::draw(") != std::string::npos, "real surface draws the shared Moves panel");
        require(text->find("SharedMovesPanel::drawChooserRow(") != std::string::npos, "real surface uses the shared chooser row");
        require(text->find("SharedEditorChrome::drawPickerFooter(") != std::string::npos, "real surface uses the shared picker footer");
        require(text->find("SharedEditorChrome::beginModal(") != std::string::npos, "real modals own the touch layer");
        require(text->find("{\"A\", \"Choose\"}") == std::string::npos, "no ad-hoc A Choose footers");
        require(text->find("MovePickerRowUI::draw(") == std::string::npos, "old move row retired");
    }
    // Phase 6 / 10: Y re-roll in every Gen III-V workspace; inline Pokerus / Language toggles.
    const std::string gen4 = read("src/UI/Gen4SharedPokemonSurface.inc");
    const std::string gen5 = read("src/UI/Gen5SharedReadOnlySurface.inc");
    require(gen3.find("Reroll::reroll(rng, {w.ivs, w.evs})") != std::string::npos, "Gen III Y re-roll");
    require(shared.find("Reroll::reroll(rng, {A::ivs(*record), A::evs(*record)})") != std::string::npos, "Gen IV/V Y re-roll");
    require(gen3.find("Presentation::nextPokerusByte(p.pokerus, state.session.baseline.pokerus)") != std::string::npos, "Gen III Pokerus inline");
    require(gen3.find("Presentation::nextGen3Language(p.language)") != std::string::npos, "Gen III Language inline");
    require(gen4.find("SharedEditorPresentation::nextPokerusByte(") != std::string::npos, "Gen IV Pokerus inline");
    require(gen4.find("openPokerusPicker") == std::string::npos && gen4.find("openLanguagePicker") == std::string::npos, "Gen IV popups retired");
    require(gen5.find("const uint32_t next = mode == 0 ? 2u : mode == 2 ? 1u : 0u;") != std::string::npos, "Gen V Pokerus inline");
    // Phase 9/12 routing: Gen III/IV summary-panel taps and Gen IV party taps open the native
    // sheet (never the inherited generic action sheet); Gen IV adopts passive View in draw.
    for (const auto* text : {&gen3, &gen4})
        require(text->find("tapped == PokeBank::UIModel::Gen4BoxTouchActions::SummaryPanelTouchId") != std::string::npos,
                "summary tap routed to the native sheet");
    require(gen4.find("screen.selectedPartyIndex = previous;") != std::string::npos, "Gen IV party tap guard");
    require(gen4.find("if (isGen4Nds(screen)) beginPassiveView(screen);\n    if (!ownsFrameImpl(screen)) return false;") != std::string::npos,
            "Gen IV passive View adopted in draw");
    require(gen5.find("hints.push_back({\"\",") == std::string::npos, "no dropped glyph-less Gen V hint");
    for (const auto* text : {&gen3, &gen4, &gen5}) {
        require(text->find("(ID \" + std::to_string") == std::string::npos &&
                text->find("\"Location ID \"") == std::string::npos, "no raw location ids");
        require(text->find("getMetLocationDisplayName(") != std::string::npos, "player-facing location names");
    }
    namespace Guard = PokeBank::UIModel::ClassicGenericUi;
    for (const char* id : {"red_gb", "crystal_gbc", "emerald_gba", "leafgreen_gba", "platinum_nds", "white2_nds"})
        require(Guard::classicSession(id) && !Guard::genericUiAllowed(true, Guard::GenericUi::ActionSheet), "Gen I-V session");
    for (const char* id : {"", "x_3ds", "sword_switch", "scarlet"})
        require(!Guard::classicSession(id) && Guard::genericUiAllowed(Guard::classicSession(id), Guard::GenericUi::Picker),
                "Gen VI+ keeps the generic UI");
    require(Guard::classicGeneration("emerald_gba") == 3 && Guard::classicGeneration("heartgold_nds") == 4 &&
            Guard::classicGeneration("black_nds") == 5 && Guard::classicGeneration("gold_gbc") == 2, "session generation");
    // CG11: ONE named Gen I-V router in front of the generic-UI open points of the base screen.
    const std::string base = read("src/UI/TrainerViewScreenBase.inc");
    for (const char* site : {
             "void TrainerViewScreen::openStorageEditor(int pane, int box, int slot) {\n        if (!genericUiAllowedForSession(*this, PokeBank::UIModel::ClassicGenericUi::GenericUi::DetailsEditor)) {",
             "if (genericUiAllowedForSession(*this, Guard::GenericUi::ActionSheet)) return;",
             "!genericUiAllowedForSession(*this, PokeBank::UIModel::ClassicGenericUi::GenericUi::DetailsEditor);",
             "if (!genericUiAllowedForSession(*this, PokeBank::UIModel::ClassicGenericUi::GenericUi::Picker)) {\n            pickerActive = false;\n            statEdit.dialogActive = false;"})
        require(base.find(site) != std::string::npos, std::string("CG11 router site: ") + site);
    std::size_t creatorGuards = 0, pickerGuards = 0;
    for (std::size_t at = base.find("GenericUi::Creator)) return;"); at != std::string::npos;
         at = base.find("GenericUi::Creator)) return;", at + 1)) ++creatorGuards;
    for (std::size_t at = base.find("GenericUi::Picker)) return;"); at != std::string::npos;
         at = base.find("GenericUi::Picker)) return;", at + 1)) ++pickerGuards;
    require(creatorGuards == 2 && pickerGuards == 2, "generic creator / item picker opens are guarded");
    // CG08: base-reached modals own the touch layer and draw their own footer in Gen I-V,
    // guarded so Gen VI+ keeps its presentation.
    for (const char* path : {"src/UI/Dialogs/SaveConfirmDialog.cpp", "src/UI/Dialogs/ItemEditDialog.cpp",
                             "src/UI/Dialogs/StatEditDialog.cpp", "src/UI/Panels/StoragePanel.cpp"}) {
        const std::string text = read(path);
        require(text.find("ClassicGenericUi::classicSession(screen.sourceGameId)") != std::string::npos &&
                text.find("SharedEditorChrome::beginModal(screen.touchButtons);") != std::string::npos,
                std::string("Gen I-V modal owns the touch layer: ") + path);
    }
    // touchButtons.clear() alone leaves covered glyph/footer hits live (P0 #320): every
    // Gen I-V-reachable confirm starts its own touch layer.
    for (const auto& [path, fn] : std::initializer_list<std::pair<const char*, const char*>>{
             {"src/UI/Panels/StoragePanel.cpp", "void drawStorageReleaseConfirm("},
             {"src/UI/Panels/StoragePanel.cpp", "void drawCreatorKeepConfirm("},
             {"src/UI/Panels/StoragePanel.cpp", "void drawDetailsDiscardConfirm("},
             {"src/UI/Panels/StoragePanel.cpp", "void drawStorageExitConfirm("},
             {"src/UI/Dialogs/ItemEditDialog.cpp", "void drawItemRemoveConfirm("},
             {"src/UI/Dialogs/ItemEditDialog.cpp", "void drawItemEditDialog("},
             {"src/UI/Gen5SharedReadOnlySurface.inc", "void drawPidConfirmation("}}) {
        const std::string text = read(path);
        const std::size_t at = text.find(fn);
        require(at != std::string::npos, std::string("modal exists: ") + fn);
        const std::size_t next = text.find("\nvoid ", at + 1) < text.find("    void ", at + 1)
            ? text.find("\nvoid ", at + 1) : text.find("    void ", at + 1);
        const std::string body = text.substr(at, next == std::string::npos ? std::string::npos : next - at);
        require(body.find("SharedEditorChrome::beginModal(screen.touchButtons);") != std::string::npos &&
                (body.find("drawNavBar(fb,") != std::string::npos ||
                 body.find("SharedEditorChrome::drawFooter(fb,") != std::string::npos),
                std::string("modal owns the touch layer and its footer: ") + fn);
    }
    // Gen I-V destructive confirms: only Y commits; A, B and Y+B cancel; the button taps as Y.
    {
        namespace P = PokeBank::UIModel::SharedEditorPresentation;
        require(P::destructiveConfirmStep(true, false, false) == P::DestructiveConfirm::Commit, "Y commits");
        require(P::destructiveConfirmStep(false, true, false) == P::DestructiveConfirm::Cancel, "A cancels");
        require(P::destructiveConfirmStep(false, false, true) == P::DestructiveConfirm::Cancel, "B cancels");
        require(P::destructiveConfirmStep(true, false, true) == P::DestructiveConfirm::Cancel, "Y+B cancels");
        require(P::destructiveConfirmStep(true, true, false) == P::DestructiveConfirm::Cancel, "Y+A cancels");
        require(P::destructiveConfirmStep(false, false, false) == P::DestructiveConfirm::None, "idle");
        for (const char* handler : {"        if (releaseConfirmActive) {\n", "        if (itemRemoveConfirmActive) {\n"}) {
            const std::size_t at = base.find(handler);
            require(at != std::string::npos, std::string("confirm handler: ") + handler);
            const std::string body = base.substr(at, 1600);
            require(body.find("if (tb == 1) kDown |= yCommits ? HidNpadButton_Y : HidNpadButton_A;") != std::string::npos &&
                    body.find("Presentation::destructiveConfirmStep(") != std::string::npos &&
                    body.find("ClassicGenericUi::classicSession(sourceGameId)") != std::string::npos,
                    std::string("Gen I-V confirm commits on Y only: ") + handler);
        }
        require(read("src/UI/Panels/StoragePanel.cpp").find("confirmFooter(\"Y\", \"Release\")") != std::string::npos &&
                read("src/UI/Dialogs/ItemEditDialog.cpp").find("confirmFooter(\"Y\", \"Remove\")") != std::string::npos,
                "Release / Remove footers name Y");
    }
    require(read("src/UI/Dialogs/SaveConfirmDialog.cpp").find(
                "SharedEditorChrome::drawFooter(fb, classicSaveConfirmFooter(screen));") != std::string::npos,
            "exit save dialog draws its own Gen I-V footer");
    for (const auto* text : {&gen3, &gen4}) {
        require(text->find("void drawDiscardAllConfirm(TrainerViewScreen& screen, PKSEFramebuffer& fb, const char* generation) {\n"
                           "    SharedEditorChrome::beginModal(screen.touchButtons);") != std::string::npos,
                "Discard All confirm owns the touch layer");
        require(text->find("{\"A\", \"Select\"}") == std::string::npos, "action menus use the shared footer");
    }
    require(gen5.find("dimShared(screen,fb);\n    // The decision owns the frame's touch; the covered hub's footer is never tappable.\n"
                      "    SharedEditorChrome::beginModal(screen.touchButtons);") != std::string::npos,
            "Gen V decision owns the touch layer");
    require(gen4.find("PokeVault::Inventory::QuantityPolicy::gameFromId(screen.sourceGameId, game)") != std::string::npos,
            "Gen IV bag quantity resolves the exact game through QuantityPolicy");
    require(species.find("commits to draft") == std::string::npos &&
            species.find("restores previous committed") == std::string::npos, "species picker has no developer copy");
    require(read("include/UI/SharedPokemonShell.h").find("\"Rows \"") == std::string::npos, "shell row counter removed");
}

int main() {
    testMoveFacts();
    testMovesPanelBounds();
    testChooserBounds();
    testFooterAndToggles();
    testDetailsViewport();
    testOriginPaneBounds();
    testFieldAccessParity();
    testDataPaneWrap();
    testLocationNames();
    testRealSourcesUseSharedUnit();
    std::cout << "Shared Gen I-V editor unit (moves panel, chooser, footer, toggles, viewport) PASS\n";
}
