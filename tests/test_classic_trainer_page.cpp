// Round 6 P2 (owner Oct 25: R2-TRAINER / NEW-18). The Gen I-IV Trainer Info page in the Gen V
// Trainer design, with no overlapping or duplicated rows: on Gold / Silver an old "Male" card was
// painted over the new rows by a second renderer.
//
// UI/ClassicTrainerPage.h is the one model drawTrainerView paints from (rows + rects + text
// rects). For every Gen I-IV game, with the staged Play Time / Gender writers that game really
// has, this checks: the native row set (no Gen I or Gold / Silver gender, Crystal / Gen III / IV
// gender once, two separate IDs only where stored), Gen V metrics, every card and text rect inside
// the panel, no two rects overlapping (rows, label vs value, the Y Review Changes button), no
// duplicate label, and that the session's focus order is the drawn order. Worst-case values use a
// conservative glyph width. The routing pins at the end bind the native-only renderer to it.
// Round 7 (owner B06 / C06): every row at the exact Gen V height (no compression), and a compact
// Review button in a measured slot below the last row, inside the panel. The colours and the
// painted geometry against Gen V's own Trainer frame are checked in tests/test_ui_layout_host.cpp.
#include "Integration/Common/NativeBadges.h"
#include "UI/ClassicTrainerPage.h"

#include <cassert>
#include <fstream>
#include <iostream>
#include <iterator>
#include <set>
#include <string>
#include <vector>

namespace Page = PokeBank::UIModel::ClassicTrainerPage;
using Page::Field;

namespace {

// The base screen's content rect (Gen1PokemonEditorOverlay.h LEFT_PANEL_X / CONTENT_PANEL_*),
// 1280 wide.
constexpr int kX = 12, kY = 80, kW = 1280 - 12, kH = 560;
constexpr int kGlyphW = 15; // wider than any Body glyph on the Switch font, so widths are upper bounds
constexpr int kTextH = 26;

int textW(const std::string& text) { return static_cast<int>(text.size()) * kGlyphW; }

std::string worstValue(Field field) {
    PokeVault::Integration::Common::NativeBadges sixteen{0xFFFF, 16};
    switch (field) {
        case Field::Name: return "WWWWWWW";              // 7 widest characters (Gen III / IV max)
        case Field::Money: return "$999999";
        case Field::PlayTime: return "999:59:59";
        case Field::Gender: return "Female";
        case Field::TrainerId: return "65535";
        case Field::SecretId: return "65535";
        case Field::Badges: return sixteen.text();       // "Johto 8/8 / Kanto 8/8"
        case Field::Pokedex: return "Seen 493 / Caught 493 / 493";
    }
    return {};
}

struct Game {
    const char* id;
    int generation;
    bool crystal;
    bool playTime; // the generation's native Play Time writer exists
    bool gender;   // the generation offers a Gender writer
};

// Lane S f775db01: Gen I offers neither; Gen II offers Play Time and Gender (Crystal byte only);
// Gen III / IV offer both.
constexpr Game kGames[] = {
    {"red_gb", 1, false, false, false},      {"blue_gb", 1, false, false, false},
    {"yellow_gb", 1, false, false, false},   {"gold_gbc", 2, false, true, false},
    {"silver_gbc", 2, false, true, false},   {"crystal_gbc", 2, true, true, true},
    {"ruby_gba", 3, false, true, true},      {"sapphire_gba", 3, false, true, true},
    {"emerald_gba", 3, false, true, true},   {"firered_gba", 3, false, true, true},
    {"leafgreen_gba", 3, false, true, true}, {"diamond_nds", 4, false, true, true},
    {"pearl_nds", 4, false, true, true},     {"platinum_nds", 4, false, true, true},
    {"heartgold_nds", 4, false, true, true}, {"soulsilver_nds", 4, false, true, true},
};

int countField(const std::vector<Page::Row>& rows, Field field) {
    int n = 0;
    for (const auto& row : rows) n += row.field == field;
    return n;
}

void checkPage(const Game& game, bool playTime, bool gender) {
    const auto rows = Page::rows({game.generation, game.crystal, playTime, gender});
    const std::string who = std::string(game.id) + (playTime ? " +time" : "") + (gender ? " +gender" : "");

    // Native row set: nothing invented, nothing twice.
    std::set<std::string> labels;
    for (const auto& row : rows) {
        const bool fresh = labels.insert(std::string(Page::label(row.field))).second;
        if (!fresh) std::cerr << who << ": duplicate row " << Page::label(row.field) << '\n';
        assert(fresh);
    }
    const bool genderStored = game.generation >= 3 || game.crystal;
    assert(countField(rows, Field::Gender) == (genderStored ? 1 : 0)); // no Gen I / Gold / Silver gender
    assert(countField(rows, Field::SecretId) == (game.generation >= 3 ? 1 : 0));
    for (Field always : {Field::Name, Field::Money, Field::TrainerId, Field::PlayTime, Field::Badges, Field::Pokedex})
        assert(countField(rows, always) == 1);
    // Editable rows come first and their touch ids are the session's focus order 0..n-1.
    int expected = 0;
    bool seenReadOnly = false;
    for (const auto& row : rows) {
        if (row.editable) {
            assert(!seenReadOnly && row.touchId == expected++);
        } else {
            seenReadOnly = true;
            assert(row.touchId == -1);
        }
    }
    assert(expected == PokeBank::UIModel::ClassicTrainerSession::rowCount(playTime, gender && genderStored));
    for (const auto& row : rows) {
        if (row.field == Field::PlayTime) assert(row.editable == playTime);
        if (row.field == Field::Gender) assert(row.editable == (gender && genderStored));
        if (row.field == Field::Badges || row.field == Field::Pokedex || row.field == Field::TrainerId ||
            row.field == Field::SecretId)
            assert(!row.editable);
    }

    // Layout: Gen V metrics, inside the panel, nothing overlapping.
    const auto page = Page::layout(rows, kX, kY, kW, kH);
    assert(page.fits && page.rows.size() == rows.size());
    // The compact Review button: smaller than a row card and than Gen V's 240 x 44, right-aligned
    // with the cards, ending kReviewMargin or more above the panel's bottom edge.
    assert(page.review.w == Page::kReviewW && page.review.h == Page::kReviewH);
    assert(page.review.w < 240 && page.review.h < 44 && page.review.h < Page::kInfoH);
    assert(page.review.right() == page.rows.front().card.right());
    assert(page.review.bottom() <= kY + kH - Page::kReviewMargin);
    const Page::Rect body{kX, kY + Page::kHeaderH, kW, kH - Page::kHeaderH};
    assert(page.review.inside(body));
    std::vector<Page::Rect> rects{page.review};
    for (const auto& placed : page.rows) {
        const auto& c = placed.card;
        assert(c.inside(body) && c.w == Page::kCardW);
        assert(c.h == (placed.row.editable ? Page::kEditH : Page::kInfoH));   // the Gen V heights, never squeezed
        assert(page.review.y >= c.bottom() + Page::kReviewGap);               // the Review slot is below every row
        if (!placed.row.editable) assert(c.y > page.dividerY);
        else assert(c.bottom() < page.dividerY);
        const std::string label(Page::label(placed.row.field));
        const auto text = Page::textRects(placed, textW(label), textW(worstValue(placed.row.field)), kTextH);
        assert(text.label.inside(c) && text.value.inside(c));
        assert(!text.label.overlaps(text.value)); // a long value never runs into its label
        rects.push_back(c);
    }
    for (size_t i = 0; i < rects.size(); ++i)
        for (size_t j = i + 1; j < rects.size(); ++j) {
            if (rects[i].overlaps(rects[j]))
                std::cerr << who << ": rects " << i << " and " << j << " overlap\n";
            assert(!rects[i].overlaps(rects[j]));
        }
}

std::string read(const char* path) {
    std::ifstream in(path, std::ios::binary);
    assert(in);
    return {std::istreambuf_iterator<char>(in), {}};
}

void routes() {
    const auto base = read("src/UI/TrainerViewScreenBase.inc");
    const auto view = base.substr(base.find("static void drawTrainerView("));
    assert(view.find("drawClassicTrainerView(screen, fb, x, y, w, h, generation);") < view.find("\n    }\n"));
    const auto classic = base.substr(base.find("static void drawClassicTrainerView("),
                                     base.find("static void drawTrainerView(") - base.find("static void drawClassicTrainerView("));
    assert(classic.find("const auto rows = Page::rows({generation, screen.sourceGameId == \"crystal_gbc\",") != std::string::npos);
    assert(classic.find("const auto page = Page::layout(rows, x, y, w, h);") != std::string::npos);
    assert(classic.find("screen.touchButtons.push_back({placed.row.touchId, c.x, c.y, c.w, c.h});") != std::string::npos);
    assert(classic.find("ClassicTrainerSession::kReviewTouchId") != std::string::npos);
    // Round 7: painted by the one shared Trainer painter, the one Gen V's Trainer page uses.
    for (const char* call : {"Panel::drawPanel(fb, x, y, w, h,", "Panel::drawEditableRow(fb, c.x, c.y, c.w, c.h,",
                             "Panel::drawInfoRow(fb, c.x, c.y, c.w, c.h,", "Panel::drawDivider(fb, c.x, page.dividerY, c.w);",
                             "drawGlyphButton(fb, page.review.x, page.review.y, page.review.w, page.review.h,"})
        assert(classic.find(call) != std::string::npos);
    for (const char* direct : {"fb.drawText(", "fb.drawFilledRoundedRect(", "fb.drawPill(", "SharedFieldState"})
        assert(classic.find(direct) == std::string::npos);   // nothing painted outside the shared painter
    const auto gen5 = read("src/UI/Gen5SharedReadOnlySurface.inc");
    const auto g5 = gen5.substr(gen5.find("void drawTrainerEditor(TrainerViewScreen& screen,PKSEFramebuffer& fb) {"));
    for (const char* call : {"TPanel::drawPanel(fb,x,y,w,h,", "TPanel::drawEditableRow(fb,cardX,cy,cardW,rowH,",
                             "TPanel::drawInfoRow(fb,cardX,cy,cardW,infoH,", "TPanel::drawDivider(fb,cardX,cy,cardW);"})
        assert(g5.find(call) != std::string::npos);
    const auto painter = read("include/UI/SharedTrainerPanel.h");
    assert(painter.find("inline Color valueColor() noexcept { return Colors::Text; }") != std::string::npos);
    assert(base.find("Male (fixed by game)") == std::string::npos);
    // The second Gold / Silver renderer that overlapped the page is gone.
    const auto gen2 = read("src/UI/Gen2HardwareFinalFix.inc");
    assert(gen2.find("drawGoldSilverTrainerGender") == std::string::npos);
    // Discard All from a Review (Trainer included) reverts without leaving the session: say so.
    for (const char* file : {"src/UI/Gen1PokemonEditorOverlayUXCleanup2.inc", "src/UI/Gen2SharedPokemonSurface.inc",
                             "src/UI/Gen3SharedPokemonSurface.inc", "src/UI/Gen4SharedPokemonSurface.inc"})
        assert(read(file).find("\"All staged changes discarded - still editing; the original save is unchanged\"") !=
               std::string::npos);
}

} // namespace

int main() {
    for (const auto& game : kGames) {
        checkPage(game, game.playTime, game.gender);     // what the game really offers
        checkPage(game, false, false);                   // writers unavailable (new-game / read-only)
        checkPage(game, true, true);                     // worst case: every writer
    }
    const auto gen4 = Page::layout(Page::rows({4, false, true, true}), kX, kY, kW, kH);
    assert(gen4.rows.front().card.h == Page::kEditH && gen4.rows.back().card.h <= Page::kInfoH);
    routes();
    std::cout << "Gen I-IV Trainer Info (Gen V design): native rows, no overlap, no duplicate gender, "
                 "16 games x 3 writer sets: PASS\n";
    return 0;
}
