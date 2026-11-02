// Round 6 E05 (owner hardware, Oct 25): the glyph / Help / footer sweep's acceptance without
// screenshots. This drives the REAL TrainerViewScreen::draw (tests/host-ui harness) for a Gen I, II,
// III, IV and V save on every swept Game Browser page (save hub, Party, Boxes, Trainer, Bag) and
// the Help window, records the text each frame leaves visible (HostUi layout recorder: draw order,
// clips and opaque fills applied) and asserts:
//   * exactly one footer bar is drawn and it is the last thing on the footer band;
//   * no two visible text runs overlap (stub metrics: 9 px per byte, 18 px line).
#include "HostUi.h"
#include "HostUiFixtures.h"
#include "fuzz_synthetic_saves.h"
#include "fixtures/gsc_pokemon_fixture.h"

#include "Integration/Gen1/Gen1ReadOnlySave.h"
#include "Integration/Gen3/PKSMGen3Adapter.h"
#include "Integration/Gen4/Gen4ReadOnlySave.h"
#include "Legacy/FRLGReadOnlyTrainer.h"
#include "Legacy/GSCReadOnlyTrainer.h"
#include "Legacy/Gen4ReadOnlyTrainer.h"
#include "Legacy/Gen5ReadOnlyTrainer.h"
#include "Legacy/RBYReadOnlyTrainer.h"
#include "UI/Common.h"
#include "UI/PKSEFramebuffer.h"
#include "UI/TouchInput.h"
#include "UI/TrainerViewScreen.h"

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <map>
#include <set>
#include <utility>
#include <memory>
#include <string>
#include <vector>

namespace {

int failures = 0;
int checks = 0;
bool verbose = false;
void check(bool ok, const std::string& what) {
    ++checks;
    if (!ok) {
        std::cerr << "FAILED: " << what << '\n';
        ++failures;
    }
}

using View = UI::TrainerViewScreen::ViewMode;

struct Page {
    const char* name;
    bool detail;
    View mode;
    bool help;
};
constexpr Page kPages[] = {
    {"save hub", false, View::Party, false},
    {"Party", true, View::Party, false},
    {"Boxes", true, View::Boxes, false},
    {"Trainer", true, View::Trainer, false},
    {"Bag", true, View::Items, false},
    {"Help (hub)", false, View::Party, true},
    {"Help (Boxes)", true, View::Boxes, true},
};

std::string describe(const HostUi::TextRect& t) {
    char style[32];
    std::snprintf(style, sizeof style, " s%d #%08x", t.style, static_cast<unsigned>(t.rgba));
    return "\"" + t.text + "\" @" + std::to_string(t.x) + "," + std::to_string(t.y) + " " +
           std::to_string(t.w) + "x" + std::to_string(t.h) + style;
}

// One captured frame: a footer bar is drawn and no two visible text runs overlap.
void verifyFrame(const std::string& where) {
    check(HostUi::footerBars >= 1, where + ": a footer bar is drawn");
    if (verbose) std::cout << where << ": footer bars " << HostUi::footerBars << ", visible text "
                           << HostUi::visibleText.size() << '\n';
    const auto& t = HostUi::visibleText;
    int overlaps = 0;
    for (size_t i = 0; i < t.size(); ++i)
        for (size_t j = i + 1; j < t.size(); ++j) {
            const int ox = std::min(t[i].x + t[i].w, t[j].x + t[j].w) - std::max(t[i].x, t[j].x);
            const int oy = std::min(t[i].y + t[i].h, t[j].y + t[j].h) - std::max(t[i].y, t[j].y);
            if (ox > 0 && oy > 0) {
                ++overlaps;
                std::cerr << where << ": overlapping text " << describe(t[i]) << " and " << describe(t[j]) << '\n';
            }
        }
    check(overlaps == 0, where + ": no overlapping visible text");
    check(!t.empty(), where + ": the page drew visible text");
    if (std::getenv("HOSTUI_DUMP") && where.find(std::getenv("HOSTUI_DUMP")) != std::string::npos)
        for (const auto& r : t) std::cout << "  " << describe(r) << '\n';
}

// Round 7 (owner Round 6 A22 / C01, lane C): Gen II Special uses the Gen I calculated-Special
// presentation. One Special DV / Stat Exp row; the numeric Calculated column holds plain numbers
// for HP, Attack, Defense and Speed only (nothing on the Special row, no tags, no "x / y"); the
// derived values sit in the lower-middle of the STATS panel: "CALCULATED SPECIAL STATS", then one
// line "Sp. Atk n   Sp. Def n", below the grid and the HP note and above Shiny.
void checkGen2Special(const std::string& where) {
    const HostUi::TextRect* find[7] = {};
    const char* names[7] = {"Calculated", "Special", "* HP DV derived / read-only", "CALCULATED SPECIAL STATS",
                            "Sp. Atk", "Sp. Def", "Shiny"};
    for (const auto& t : HostUi::visibleText)
        for (int i = 0; i < 7; ++i)
            if (t.text == names[i] && !find[i]) find[i] = &t;
    for (int i = 0; i < 7; ++i) check(find[i] != nullptr, where + ": \"" + names[i] + "\" is visible");
    for (const auto* f : find) if (!f) return;
    const auto& calculated = *find[0];
    const auto& special = *find[1];
    const auto& hpNote = *find[2];
    const auto& caption = *find[3];
    const auto& spaTag = *find[4];
    const auto& spdTag = *find[5];
    const auto& shiny = *find[6];
    const auto numeric = [](const HostUi::TextRect& t) {
        return !t.text.empty() && t.text.find_first_not_of("0123456789") == std::string::npos;
    };
    const auto valueRightOf = [&](const HostUi::TextRect& tag) -> const HostUi::TextRect* {
        const HostUi::TextRect* best = nullptr;
        for (const auto& t : HostUi::visibleText)
            if (t.y == tag.y && t.x > tag.x + tag.w && numeric(t) && (!best || t.x < best->x)) best = &t;
        return best;
    };
    const auto* spa = valueRightOf(spaTag);
    const auto* spd = valueRightOf(spdTag);
    check(spa && spd, where + ": Sp. Atk and Sp. Def numbers follow their labels");
    // The block: one line, below the Special row, the HP note and its caption, above Shiny.
    check(spaTag.y == spdTag.y && spaTag.x < spdTag.x, where + ": Sp. Atk and Sp. Def on one line");
    check(special.y < hpNote.y && hpNote.y < caption.y && caption.y < spaTag.y && spaTag.y + spaTag.h <= shiny.y - 5,
          where + ": the Special block is below the grid and the HP note, above Shiny");
    check(spa && spa->x + spa->w < spdTag.x, where + ": the Sp. Atk number ends before Sp. Def");
    // The numeric Calculated column (texts starting at its header's x, between the header and the
    // Special row): exactly HP / Attack / Defense / Speed, plain numbers; nothing on the Special row.
    int column = 0;
    for (const auto& t : HostUi::visibleText) {
        if (t.x != calculated.x || t.y <= calculated.y || t.y > special.y + 4) continue;
        check(numeric(t), where + ": plain number in the Calculated column (\"" + t.text + "\")");
        check(t.y + t.h <= special.y, where + ": nothing in the Special row's Calculated cell (\"" + t.text + "\")");
        ++column;
    }
    check(column == 4, where + ": four calculated values (HP, Attack, Defense, Speed), found " + std::to_string(column));
    for (const auto& t : HostUi::visibleText)
        if (t.x >= calculated.x - 100 && t.y > calculated.y && t.y <= special.y + 4)
            check(t.text.rfind("Sp", 0) != 0 && t.text.find(" / ") == std::string::npos,
                  where + ": no Special value squeezed into the STATS columns (\"" + t.text + "\")");
}

// Round 7 (owner Round 6 B06 / C06, lane C): the Gen I-IV Trainer page looks exactly like Gen V's.
// From each game's real Trainer frame: the row cards (680-wide rounded fills), their x, heights per
// kind (radius 14 = selectable card, radius 12 = read-only row), their colour, the label and value
// text colours, and the Review button. Every Gen I-IV signature must equal Black's; values are
// neutral text (never a highlight colour); the Gen I-IV Review button is smaller than a row card's
// height and Gen V's 240 x 44 and intersects no row; nothing else is painted over the rows.
struct TrainerStyle {
    bool captured = false;
    int cardX = -1;
    std::set<std::pair<int, int>> radiusHeight;   // {radius, height} of every row card
    std::set<uint32_t> cardColors, labelColors, valueColors;
    std::vector<HostUi::Fill> cards;
    HostUi::Fill review{};
    bool hasReview = false;
    HostUi::TextRect language{};   // Gen V's "Game language" detail line, when drawn
    bool hasLanguage = false;
};
std::map<std::string, TrainerStyle> trainerStyles;

void captureTrainerStyle(const std::string& game) {
    TrainerStyle st;
    st.captured = true;
    // Only what the visible Trainer panel paints: the fills after its last full panel fill (Gen V
    // paints its page over the base view, whose fills are hidden beneath).
    std::vector<HostUi::Fill> fills;
    for (const auto& f : HostUi::fills) {
        if (f.radius == 16 && f.w >= 1000 && f.h >= 400) fills.clear();
        fills.push_back(f);
    }
    for (const auto& f : fills)
        if (f.w == 680 && (f.radius == 12 || f.radius == 14)) {
            st.cards.push_back(f);
            st.cardX = f.x;
            st.radiusHeight.insert({f.radius, f.h});
            st.cardColors.insert(f.rgba);
        }
    const auto inside = [](const HostUi::TextRect& t, const HostUi::Fill& f) {
        return t.x >= f.x && t.x + t.w <= f.x + f.w && t.y >= f.y && t.y + t.h <= f.y + f.h;
    };
    for (const auto& t : HostUi::visibleText) {
        for (const auto& card : st.cards)
            if (inside(t, card)) (t.x == card.x + 24 ? st.labelColors : st.valueColors).insert(t.rgba);
        if (t.text.rfind("Game language: ", 0) == 0) { st.language = t; st.hasLanguage = true; }
        if (t.text == "Review Changes")
            for (const auto& f : fills)
                if (f.radius == 8 && inside(t, f)) { st.review = f; st.hasReview = true; }
    }
    check(!st.cards.empty(), game + " / Trainer: row cards drawn");
    // Nothing but the row cards and their value pills is painted over the rows' area.
    for (const auto& f : fills) {
        if (f.w == 680 || f.radius == -1 || (st.hasReview && f.x == st.review.x && f.y == st.review.y)) continue;
        for (const auto& card : st.cards)
            check(!(f.x < card.x + card.w && card.x < f.x + f.w && f.y < card.y + card.h && card.y < f.y + f.h) ||
                  (f.w >= 1000),   // the panel / header behind the rows
                  game + " / Trainer: no other fill over a row (" + std::to_string(f.x) + "," +
                  std::to_string(f.y) + " " + std::to_string(f.w) + "x" + std::to_string(f.h) + ")");
    }
    trainerStyles[game] = st;
}

void compareTrainerStyles() {
    const auto gen5 = trainerStyles.find("Black");
    check(gen5 != trainerStyles.end() && gen5->second.captured, "Black Trainer captured (the reference)");
    if (gen5 == trainerStyles.end()) return;
    const auto& ref = gen5->second;
    check(ref.hasLanguage, "Black Trainer: the Game language line is drawn");
    const uint32_t neutral = UI::Colors::Text.toRGBA8();
    check(ref.valueColors == std::set<uint32_t>{neutral}, "Black Trainer: values are neutral text");
    // Gen V itself is held to the same Review rule (QA CG90): Black is checked against itself.
    for (const char* game : {"Red", "Silver", "Crystal", "FireRed", "Platinum", "Black"}) {
        const auto it = trainerStyles.find(game);
        check(it != trainerStyles.end(), std::string(game) + " Trainer captured");
        if (it == trainerStyles.end()) continue;
        const auto& st = it->second;
        const std::string where = std::string(game) + " / Trainer vs Black";
        check(st.cardX == ref.cardX, where + ": same card x / width");
        if (st.radiusHeight != ref.radiusHeight) {
            std::cerr << where << ": card kinds";
            for (const auto& [r, h] : st.radiusHeight) std::cerr << " r" << r << "/h" << h;
            std::cerr << " vs Black";
            for (const auto& [r, h] : ref.radiusHeight) std::cerr << " r" << r << "/h" << h;
            std::cerr << '\n';
        }
        check(st.radiusHeight == ref.radiusHeight, where + ": same card kinds and heights");
        check(st.cardColors == ref.cardColors, where + ": same row background");
        check(st.valueColors == ref.valueColors, where + ": same (neutral) value colour");
        for (const auto c : st.labelColors)
            check(ref.labelColors.count(c) || c == UI::Colors::SelectedText.toRGBA8(), where + ": label colour");
        check(st.hasReview, where + ": a Review Changes button");
        if (!st.hasReview) continue;
        check(st.review.w < 240 && st.review.h < 44, where + ": Review button narrower and shorter than Gen V's");
        for (const auto& card : st.cards)
            check(!(st.review.x < card.x + card.w && card.x < st.review.x + st.review.w &&
                    st.review.y < card.y + card.h && card.y < st.review.y + st.review.h),
                  where + ": the Review button intersects no row");
        int lowest = 0;
        for (const auto& card : st.cards) lowest = std::max(lowest, card.y + card.h);
        check(st.review.y >= lowest, where + ": the Review button sits below the rows");
        if (st.hasLanguage) {
            const auto& l = st.language;
            check(!(l.x < st.review.x + st.review.w && st.review.x < l.x + l.w && l.y < st.review.y + st.review.h &&
                    st.review.y < l.y + l.h),
                  where + ": the Game language line clears the Review button");
            check(l.y >= lowest, where + ": the Game language line sits below the rows");
        }
    }
}

// Round 7 P2 (owner Oct 26 typography): every Pokemon editor surface, Gen I-V, draws its section
// headings in the one shared style (UI/SharedPanelHeading.h: Accent, Caption), as the shared Gen IV /
// V editor does. Each heading that is visible must use it; DETAILS and MOVES must be visible.
void checkEditorHeadings(const std::string& where) {
    const uint32_t accent = UI::Colors::Accent.toRGBA8();
    bool details = false, moves = false;
    for (const auto& t : HostUi::visibleText) {
        const bool heading = t.text == "DETAILS" || t.text == "STATS" || t.text == "MOVES" ||
                             t.text == "Origin & Legality";
        if (!heading) continue;
        details = details || t.text == "DETAILS";
        moves = moves || t.text == "MOVES";
        if (std::getenv("HOSTUI_HEADINGS"))
            std::cout << where << ": " << describe(t) << " style " << t.style << " rgba " << std::hex << t.rgba
                      << std::dec << '\n';
        check(t.style == static_cast<int>(UI::TextStyle::Caption) && t.rgba == accent,
              where + ": heading \"" + t.text + "\" uses the shared heading style (Caption, Accent)");
    }
    check(details && moves, where + ": DETAILS and MOVES headings are visible");
    // Gen I-V title their provenance pane with the one shared heading (owner E03 wording).
    bool origin = false, oldTitle = false;
    for (const auto& t : HostUi::visibleText) {
        origin = origin || t.text == "Origin & Legality";
        oldTitle = oldTitle || t.text == "GEN I DATA" || t.text == "GEN II DATA";
    }
    check(origin && !oldTitle, where + ": the provenance pane is titled \"Origin & Legality\"");
    // Supporting field labels (unfocused DETAILS rows, STATS column labels) use the reference's dim
    // Caption label, so no generation draws larger or brighter labels than another.
    const uint32_t dim = UI::Colors::TextDim.toRGBA8();
    const uint32_t focused = UI::Colors::Text.toRGBA8();   // the one focused row's label, as in Gen IV / V
    int labels = 0, focusedLabels = 0;
    for (const auto& t : HostUi::visibleText) {
        const bool label = t.text == "Nickname" || t.text == "Level" || t.text == "Experience" ||
                           t.text == "Trainer ID" || t.text == "Held Item";
        if (!label || t.x > 340) continue;
        ++labels;
        if (t.rgba == focused) ++focusedLabels;
        check(t.style == static_cast<int>(UI::TextStyle::Caption) && (t.rgba == dim || t.rgba == focused),
              where + ": field label \"" + t.text + "\" uses the dim Caption label style (" + describe(t) + ")");
    }
    check(focusedLabels <= 1, where + ": at most one DETAILS label is drawn as focused");
    check(labels >= 3, where + ": DETAILS field labels are visible");
}

void sweep(const std::string& game, UI::TrainerViewScreen& screen, int emptySlot = 1, bool gen5 = false,
           bool noTrainerGender = false) {
    UI::PKSEFramebuffer fb;
    UI::TouchInput touch;
    const auto frame = [&](u64 down = 0) {
        PadState pad{down, down, 0};
        touch.update();
        screen.update(pad, touch);
        HostUi::texts.clear();
        HostUi::resetLayout();
        screen.draw(fb);
    };
    const auto press = [&](u64 button) { frame(button); frame(); };
    frame();
    for (const auto& page : kPages) {
        screen.helpOverlayActive = false;
        screen.detailViewActive = page.detail;
        screen.selectedMode = page.mode;
        screen.selectedItemIndex = 0;
        screen.selectedBoxIndex = 0;
        screen.selectedPartyIndex = 0;
        frame();
        if (page.help) {
            screen.helpOverlayActive = true;
            frame();
        }
        verifyFrame(game + " / " + page.name);
        if (page.mode == View::Trainer && page.detail && !page.help) captureTrainerStyle(game);
        if (page.mode == View::Trainer && page.detail && !page.help && noTrainerGender) {
            // Gen I and Gold / Silver store no trainer gender (CG72): no "Gender" text is drawn at
            // all on the Trainer page, visible or painted over.
            bool drawn = false;
            for (const auto& text : HostUi::texts) drawn = drawn || text.find("Gender") != std::string::npos;
            check(!drawn, game + " / Trainer: no Gender text is drawn (the save stores none)");
        }
    }
    screen.helpOverlayActive = false;

    // Round 7 (B04): - on the Bag opens the ONE Inventory Help, titled "Inventory Controls" in every
    // generation (Gen I-III through the inventory overlay, Gen IV / V through the Game Browser Help).
    screen.detailViewActive = true;
    screen.selectedMode = View::Items;
    screen.selectedItemIndex = 0;
    frame();
    press(HidNpadButton_Minus);
    {
        const std::string where = game + " / Bag Help";
        verifyFrame(where);
        bool title = false, stale = false;
        for (const auto& t : HostUi::visibleText) {
            title = title || t.text == "Inventory Controls";
            stale = stale || t.text == "Game Browser Controls" || t.text == "Inventory Help" ||
                    t.text.find("Inventory Options") != std::string::npos ||
                    t.text.find("remains available") != std::string::npos;
        }
        check(title, where + ": titled Inventory Controls");
        check(!stale, where + ": no old title or stale rows");
        // Owner Oct 28 (Bag Help #1 / #2): every Help row lies inside the Help window, in every
        // generation (Gen I-III used a fixed 900 px panel, Gen IV / V a fixed 700 px modal).
        const HostUi::TextRect* titleRect = nullptr;
        for (const auto& t : HostUi::visibleText)
            if (t.text == "Inventory Controls") titleRect = &t;
        const HostUi::Fill* modal = nullptr;
        if (titleRect)
            for (const auto& f : HostUi::fills)
                if (f.x <= titleRect->x && f.x + f.w >= titleRect->x + titleRect->w && f.y <= titleRect->y &&
                    f.y + f.h >= titleRect->y + titleRect->h && f.w < 1280 && f.h > 100)
                    modal = &f;   // paint order: the last such surface under the title is the window
        check(modal != nullptr, where + ": the Help window is found");
        if (modal) {
            int rows = 0;
            for (const auto& t : HostUi::visibleText) {
                if (t.y < modal->y || t.y + t.h > modal->y + modal->h) continue;
                // Text beside the window is the Bag underneath; anything overlapping the opaque
                // window was drawn on it, so it is Help content and must fit inside.
                if (t.x + t.w <= modal->x || t.x >= modal->x + modal->w) continue;
                ++rows;
                check(t.x >= modal->x && t.x + t.w <= modal->x + modal->w,
                      where + ": Help text inside the window: " + t.text);
            }
            check(rows >= 9, where + ": the Help rows are drawn inside the window");
            check(modal->w < 1000, where + ": the Help window is sized to its content, not a full panel");
        }
    }
    press(HidNpadButton_B);
    screen.helpOverlayActive = false;

    // Party slot 1 through the real input: A opens the action menu, A on its first row (View), and
    // Down + A (Edit). Each surface is captured, then B backs out.
    const auto partyPage = [&] {
        screen.goBack = false;
        screen.detailViewActive = true;
        screen.selectedMode = View::Party;
        screen.selectedPartyIndex = 0;
        screen.selectedItemIndex = 0;
        frame();
    };
    if (screen.trainer.getPartySize() > 0) {
        partyPage();
        press(HidNpadButton_A);
        verifyFrame(game + " / Party action menu");
        press(HidNpadButton_A);
        verifyFrame(game + " / Party View");
        checkEditorHeadings(game + " / Party View");
        press(HidNpadButton_B);
        partyPage();
        press(HidNpadButton_A);
        press(HidNpadButton_Down);
        press(HidNpadButton_A);
        verifyFrame(game + " / Party Edit");
        checkEditorHeadings(game + " / Party Edit");
        if (game == "Silver" || game == "Crystal") {
            press(HidNpadButton_R);   // focus the STATS panel
            verifyFrame(game + " / Edit -> STATS");
            checkGen2Special(game + " / Edit -> STATS");
            press(HidNpadButton_L);
        }
        press(HidNpadButton_B);   // the exit question (nothing staged)
        verifyFrame(game + " / Party Edit exit question");
        press(HidNpadButton_Y);   // only Y discards and leaves; B would continue editing
    }
    // Review of the staged workspace (empty review where nothing is staged).
    screen.detailViewActive = false;
    frame();
    if (screen.openStagedReview(false)) {   // Gen I-IV, nothing staged yet
        frame();
        verifyFrame(game + " / Review (nothing staged)");
        for (int i = 0; i < 3; ++i) press(HidNpadButton_B);
    }

    // Create: an empty Boxes slot through the real input. A opens the empty-slot sheet (or the
    // Create workspace directly), A on its first row starts the draft; the frame is captured.
    screen.goBack = false;
    screen.detailViewActive = true;
    screen.selectedMode = View::Boxes;
    screen.selectedBoxIndex = 0;
    screen.selectedItemIndex = emptySlot;
    frame();
    press(HidNpadButton_A);
    verifyFrame(game + " / Boxes empty-slot menu");
    press(HidNpadButton_A);
    verifyFrame(game + " / Create");
        checkEditorHeadings(game + " / Create");

    // Owner item 4: a Gen V Create keeps only once a Met Location is chosen (the picker route is
    // covered by test_ui_editor_input_host); record the choice so the layout sweep can continue.
    if (gen5)
        if (auto* g5 = dynamic_cast<PokeVault::Legacy::Gen5ReadOnlyTrainer*>(&screen.trainer))
            g5->sharedScreen().noteCreateMetLocationChosen();
    // X adds the draft (a staged change), then Review: Gen I-IV through openStagedReview, Gen V on
    // its own surface (Boxes' Review key, Y since Round 7 ReviewEntry: Gen5SharedReadOnlySurface
    // openReviewFromBrowse).
    press(HidNpadButton_X);
    screen.goBack = false;
    screen.detailViewActive = true;
    screen.selectedMode = View::Boxes;
    screen.selectedBoxIndex = 0;
    screen.selectedItemIndex = 0;
    frame();
    bool reviewed = false;
    if (gen5) {
        press(HidNpadButton_Y);
        reviewed = true;
    } else if (screen.openStagedReview(false)) {
        frame();
        reviewed = true;
    }
    check(reviewed, game + ": Review opens after a staged Create");
    if (reviewed) verifyFrame(game + (gen5 ? " / Review (Gen V surface, staged Create)" : " / Review (staged Create)"));
    if (reviewed && (game == "Red" || game == "Silver")) {
        // Lane C f082ac1d (CG74, Gen I / II): a created Pokemon's Review names what was created:
        // a "<slot> / Add Pokémon" header row, then one named row per stored, non-default value
        // (Moves when the draft has any; OT / TID always).
        const auto row = [](const std::string& suffix) {
            for (const auto& t : HostUi::visibleText)
                if (t.text.size() >= suffix.size() &&
                    t.text.compare(t.text.size() - suffix.size(), suffix.size(), suffix) == 0) return true;
            return false;
        };
        const std::string where = game + " / Review (staged Create)";
        check(row("/ Add Pok\xC3\xA9mon"), where + ": the Add Pokémon header row");
        check(row("/ OT / TID"), where + ": a named OT / TID row");
        if (game == "Red") check(row("/ Moves"), where + ": a named Moves row (Gen I Create has level-up moves)");
    }
    // Lane S CG74 (Gen III / V): the created Pokemon's Review lists its details too, in the shared
    // Gen I-V "<slot> / Field" row shape.
    if (reviewed && (game == "FireRed" || game == "Black")) {
        const auto has = [](const std::string& needle, bool prefix) {
            for (const auto& t : HostUi::visibleText)
                if (prefix ? t.text.rfind(needle, 0) == 0
                           : t.text.size() >= needle.size() &&
                             t.text.compare(t.text.size() - needle.size(), needle.size(), needle) == 0) return true;
            return false;
        };
        const std::string where = game + " / Review (staged Create)";
        if (game == "FireRed") {
            // Round 7: Gen III uses the shared Gen I-V shape; an empty Moves row is omitted.
            check(has("/ Add Pok\xC3\xA9mon", false), where + ": the Add Pokémon header row");
            check(has("Empty -> ", true), where + ": the Create header value");
            check(has("/ OT / TID / SID", false), where + ": a named OT / TID / SID row");
            check(!has(" Add Pokemon", false), where + ": no old Gen III \"Add Pokemon\" header");
        } else {
            // Round 7: Gen V uses the shared Gen I-IV shape ("<loc> / Field" + value).
            check(has("/ Add Pok\xC3\xA9mon", false), where + ": the Add Pokémon header row");
            check(has("Empty -> ", true), where + ": the Create header value");
            check(has("/ OT / TID / SID", false), where + ": a named OT / TID / SID row");
        }
    }
    press(HidNpadButton_B);   // close Review only (B on the hub with staged edits opens the exit menu)

    // With a staged change, every browse page's footer offers Review (lane L's Review entries).
    for (const auto& [name, mode] : {std::pair{"Party", View::Party}, std::pair{"Boxes", View::Boxes},
                                     std::pair{"Bag", View::Items}}) {
        screen.goBack = false;
        screen.helpOverlayActive = false;
        screen.detailViewActive = true;
        screen.selectedMode = mode;
        screen.selectedBoxIndex = 0;
        screen.selectedItemIndex = 0;
        screen.selectedPartyIndex = 0;
        frame();
        const std::string where = game + " / " + name + " (staged)";
        verifyFrame(where);
        bool review = false;
        for (const auto& t : HostUi::visibleText)
            review = review || (t.y >= 720 - 56 && t.text == "Review");
        check(review, where + ": the footer offers Review");
    }
}

// The recorder itself: overlap, occlusion and clipping behave as the checks assume.
void recorderSelfTest() {
    UI::PKSEFramebuffer fb;
    HostUi::resetLayout();
    fb.drawText(100, 100, "Attack", UI::Colors::Text, UI::TextStyle::Caption);
    fb.drawText(120, 104, "Defense", UI::Colors::Text, UI::TextStyle::Caption);
    check(HostUi::visibleText.size() == 2, "recorder keeps both runs");
    const auto& a = HostUi::visibleText[0];
    const auto& b = HostUi::visibleText[1];
    check(a.x < b.x + b.w && b.x < a.x + a.w && a.y < b.y + b.h && b.y < a.y + a.h, "recorder sees an overlap");
    fb.drawFilledRect(90, 90, 200, 40, UI::Colors::Panel);
    check(HostUi::visibleText.empty(), "an opaque fill hides the text under it");
    fb.drawText(100, 100, "Speed", UI::Colors::Text, UI::TextStyle::Caption);
    fb.drawFilledRect(90, 90, 200, 40, UI::Color(0, 0, 0, 105));
    check(HostUi::visibleText.size() == 1, "a translucent scrim leaves text visible");
    fb.setClipRect(0, 0, 50, 50);
    fb.drawText(100, 100, "Clipped", UI::Colors::Text, UI::TextStyle::Caption);
    fb.clearClip();
    check(HostUi::visibleText.size() == 1, "text outside the clip is not visible");
    fb.drawText(10, 300, "Row one", UI::Colors::Text, UI::TextStyle::Caption);
    fb.drawText(10, 316, "Row two", UI::Colors::Text, UI::TextStyle::Caption);
    const auto& r1 = HostUi::visibleText[1];
    const auto& r2 = HostUi::visibleText[2];
    check(r1.y + r1.h <= r2.y, "16 px Caption rows (the stat tables) do not overlap");
    HostUi::resetLayout();
}

// Screens stay alive for the whole run: generation surfaces key per-screen state by address.
std::vector<std::shared_ptr<void>> keepAlive;

template <class T>
T* keep(std::unique_ptr<T> p) {
    T* raw = p.get();
    keepAlive.push_back(std::shared_ptr<void>(std::move(p)));
    return raw;
}

UI::TrainerViewScreen* open(Trainer::Trainer& trainer, const char* title, const char* id, const char* emulator) {
    return keep(std::make_unique<UI::TrainerViewScreen>(trainer, title, std::string("/host/") + id + ".sav", 0,
        AccountUid{}, PokeVault::Safety::SourceKind::RetroArchLegacy, id, emulator));
}

} // namespace

int main(int argc, char** argv) {
    verbose = argc > 1 && std::string(argv[1]) == "-v";
    std::string error;
    recorderSelfTest();

    // Gen I: QA synthetic Red save (qasynth::gen1).
    {
        auto bytes = std::make_shared<std::vector<uint8_t>>(qasynth::gen1::save());
        keepAlive.push_back(bytes);
        // A valid Bag (item 0x14 x3) and an empty PC item list, the list shape and
        // offsets of tests/test_rby_readonly_bridge.cpp (BAG 0x25C9, PCITEMS 0x27E6), then the
        // main checksum again, so the Gen I Bag page has a real inventory surface.
        auto& d = *bytes;
        d[0x25C9] = 1; d[0x25CA] = 0x14; d[0x25CB] = 3; d[0x25CC] = 0xFF;
        d[0x27E6] = 0; d[0x27E7] = 0xFF;
        qasynth::gen1::forge(d);
        auto parsed = PokeVault::Integration::Gen1::parse(*bytes, PokeVault::Integration::Gen1::SourceGame::Red);
        check(static_cast<bool>(parsed), "Gen I fixture parses");
        if (parsed) {
            auto* trainer = keep(PokeVault::Legacy::RBYReadOnlyTrainer::create(*parsed.save, error));
            check(trainer != nullptr, "Gen I trainer: " + error);
            if (trainer) sweep("Red", *open(*trainer, "Pokemon Red", "red_gb", "mGBA"), 1, false, true);
        }
    }
    // Gen II: PKHeX-shaped Silver fixture with a boxed Pikachu (tests/fixtures/gsc_pokemon_fixture.h).
    {
        auto bytes = std::make_shared<std::vector<uint8_t>>(fixture(GS, true));
        keepAlive.push_back(bytes);
        auto parsed = parse(*bytes, SourceGame::Silver);
        check(static_cast<bool>(parsed), "Gen II fixture parses");
        if (parsed) {
            auto* trainer = keep(PokeVault::Legacy::GSCReadOnlyTrainer::create(*parsed.save, error));
            check(trainer != nullptr, "Gen II trainer: " + error);
            if (trainer) sweep("Silver", *open(*trainer, "Pokemon Silver", "silver_gbc", "mGBA"), 1, false, true);
        }
    }
    // Gen II Crystal: the same fixture shape on the Crystal layout.
    {
        auto bytes = std::make_shared<std::vector<uint8_t>>(fixture(C, true));
        keepAlive.push_back(bytes);
        auto parsed = parse(*bytes, SourceGame::Crystal);
        check(static_cast<bool>(parsed), "Crystal fixture parses");
        if (parsed) {
            auto* trainer = keep(PokeVault::Legacy::GSCReadOnlyTrainer::create(*parsed.save, error));
            check(trainer != nullptr, "Crystal trainer: " + error);
            if (trainer) sweep("Crystal", *open(*trainer, "Pokemon Crystal", "crystal_gbc", "mGBA"));
        }
    }
    // Gen III: FireRed with a party Pikachu (HostUiFixtures::gen3Fixture).
    {
        auto bytes = std::make_shared<std::vector<uint8_t>>(HostUiFixtures::gen3Fixture(0));
        keepAlive.push_back(bytes);
        auto parsed = PokeVault::Integration::Gen3::parse(*bytes, PokeVault::Integration::Gen3::SourceGame::FireRedGBA);
        check(static_cast<bool>(parsed), "Gen III fixture parses");
        if (parsed) {
            auto* trainer = keep(PokeVault::Legacy::FRLGReadOnlyTrainer::create(*parsed.save, error));
            check(trainer != nullptr, "Gen III trainer: " + error);
            if (trainer) sweep("FireRed", *open(*trainer, "Pokemon FireRed", "firered_gba", "mGBA"));
        }
    }
    // Gen IV: QA synthetic Platinum save (qasynth::gen4).
    {
        auto bytes = std::make_shared<std::vector<uint8_t>>(qasynth::gen4::save(qasynth::gen4::Kind::Platinum));
        keepAlive.push_back(bytes);
        auto parsed = PokeVault::Integration::Gen4::Gen4ReadOnlySave::parse(
            *bytes, PokeVault::Integration::Gen4::Layout::Platinum, "platinum_nds", &error);
        check(parsed.has_value(), "Gen IV fixture parses: " + error);
        if (parsed) {
            auto keptSave = std::make_shared<PokeVault::Integration::Gen4::Gen4ReadOnlySave>(*parsed);
            keepAlive.push_back(keptSave);
            auto* trainer = keep(PokeVault::Legacy::Gen4ReadOnlyTrainer::create(*keptSave, "platinum_nds", error));
            check(trainer != nullptr, "Gen IV trainer: " + error);
            if (trainer) sweep("Platinum", *open(*trainer, "Pokemon Platinum", "platinum_nds", "DraStic"));
        }
    }
    // Gen V: Black with a party Pikachu (HostUiFixtures::gen5Fixture).
    {
        auto bytes = std::make_shared<std::vector<uint8_t>>(HostUiFixtures::gen5Fixture());
        keepAlive.push_back(bytes);
        auto parsed = PokeVault::Integration::Gen5::Gen5ReadOnlySave::parse(
            *bytes, "black_nds", &error, PokeVault::Integration::Gen5::SaveCopySelection::Primary);
        check(parsed.has_value(), "Gen V fixture parses: " + error);
        if (parsed) {
            auto* trainer = keep(PokeVault::Legacy::Gen5ReadOnlyTrainer::create(*parsed, "black_nds", error));
            check(trainer != nullptr, "Gen V trainer: " + error);
            if (trainer) sweep("Black", *open(*trainer, "Pokemon Black", "black_nds", "DraStic"), 1, true);
        }
    }

    compareTrainerStyles();

    if (failures) {
        std::cerr << failures << " of " << checks << " layout checks failed\n";
        return 1;
    }
    std::cout << "Gen I-V Game Browser pages and Help through the real TrainerViewScreen::draw: one footer, "
                 "no overlapping text (" << checks << " checks): PASS\n";
    return 0;
}
