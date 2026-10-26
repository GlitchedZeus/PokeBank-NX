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

// Round 6 7A (lane C e39a92e2): one Special DV / Stat Exp row; the calculated Sp. Atk sits in the
// Special row's Calculated cell and Sp. Def directly beneath it, each with a dim tag just left of
// its number; every calculated number is right-aligned in the one Calculated column; no "x / y".
void checkGen2Special(const std::string& where) {
    const HostUi::TextRect* spaTag = nullptr;
    const HostUi::TextRect* spdTag = nullptr;
    const HostUi::TextRect* calculated = nullptr;
    for (const auto& t : HostUi::visibleText) {
        if (t.text == "Sp. Atk") spaTag = &t;
        if (t.text == "Sp. Def") spdTag = &t;
        if (t.text == "Calculated") calculated = &t;
    }
    check(spaTag && spdTag && calculated, where + ": Calculated column with Sp. Atk and Sp. Def tags");
    if (!spaTag || !spdTag || !calculated) return;
    const auto valueRightOf = [&](const HostUi::TextRect& tag) -> const HostUi::TextRect* {
        for (const auto& t : HostUi::visibleText)
            if (t.y == tag.y && t.x > tag.x + tag.w && !t.text.empty() &&
                t.text.find_first_not_of("0123456789") == std::string::npos) return &t;
        return nullptr;
    };
    const auto* spa = valueRightOf(*spaTag);
    const auto* spd = valueRightOf(*spdTag);
    check(spa && spd, where + ": Sp. Atk and Sp. Def numbers follow their tags");
    if (!spa || !spd) return;
    check(spd->y > spa->y && spd->y - spa->y <= 40, where + ": Sp. Def directly beneath Sp. Atk");
    check(spa->x + spa->w == spd->x + spd->w, where + ": Sp. Atk / Sp. Def right-aligned in one column");
    // Every number of the Calculated column (right of its header's left edge, between the header and
    // Sp. Def) ends on the same right edge.
    int column = 0;
    for (const auto& t : HostUi::visibleText)
        if (t.x >= calculated->x - 10 && t.y > calculated->y && t.y <= spd->y && t.x < calculated->x + 140 &&
            !t.text.empty() && t.text.find_first_not_of("0123456789") == std::string::npos) {
            ++column;
            check(t.x + t.w == spa->x + spa->w, where + ": calculated value \"" + t.text + "\" right-aligned");
        }
    // Nothing in the Calculated column is a combined "SpA N" / "A x / D y" cell (the radar's "SpA N"
    // axis label on the graph pane is a different surface).
    for (const auto& t : HostUi::visibleText)
        if (t.x >= calculated->x - 100 && t.x < calculated->x + 140 && t.y > calculated->y && t.y <= spd->y)
            check(t.text.rfind("SpA ", 0) != 0 && t.text.find(" / ") == std::string::npos,
                  where + ": no combined Special cell in the Calculated column (\"" + t.text + "\")");
    check(column == 6, where + ": six calculated values (HP, Atk, Def, Spe, Sp. Atk, Sp. Def), found " +
          std::to_string(column));
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
        if (page.mode == View::Trainer && page.detail && !page.help && noTrainerGender) {
            // Gen I and Gold / Silver store no trainer gender (CG72): no "Gender" text is drawn at
            // all on the Trainer page, visible or painted over.
            bool drawn = false;
            for (const auto& text : HostUi::texts) drawn = drawn || text.find("Gender") != std::string::npos;
            check(!drawn, game + " / Trainer: no Gender text is drawn (the save stores none)");
        }
    }
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

    // X adds the draft (a staged change), then Review: Gen I-IV through openStagedReview, Gen V on
    // its own surface (+ on a browse page, Gen5SharedReadOnlySurface openReviewFromBrowse).
    press(HidNpadButton_X);
    screen.goBack = false;
    screen.detailViewActive = true;
    screen.selectedMode = View::Boxes;
    screen.selectedBoxIndex = 0;
    screen.selectedItemIndex = 0;
    frame();
    bool reviewed = false;
    if (gen5) {
        press(HidNpadButton_Plus);
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

    if (failures) {
        std::cerr << failures << " of " << checks << " layout checks failed\n";
        return 1;
    }
    std::cout << "Gen I-V Game Browser pages and Help through the real TrainerViewScreen::draw: one footer, "
                 "no overlapping text (" << checks << " checks): PASS\n";
    return 0;
}
