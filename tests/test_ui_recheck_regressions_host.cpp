// Round 13 recheck regressions (docs/qa/RECHECK_REGRESSIONS.md) through the REAL TrainerViewScreen
// (tests/host-ui harness): Hardware Accepted Gen II Create behaviour whose code changed after its
// accepted candidate, re-asserted on the changed route.
//
//   PBX-CRT-017  accepted cfb70357 (PR #68): the Add toast is concise, "<name> added to Box N", not
//                "Pokemon added to staged copy only; source .srm unchanged". Changed since: R11
//                620ee717 / ab072252 (required-fields gate before state.add) and R12 db4a9d29
//                (R12-010 scopes the status line to the context it was posted in). The toast is
//                posted as the editor closes; it must still be DRAWN over the box it names.
//   PBX-CRT-005  accepted cfb70357: after Add the box grid is shown with the new Pokemon selected; no
//                Pokemon Actions sheet auto-opens.
//   PBX-CRT-003 / PBX-CRT-001  accepted cfb70357 / R11 3634d1e6 H30: Create on a RetroArch Gen II
//                save stages into the working copy (never refused as "read-only"), and the source
//                save bytes never change.
//   PBX-CRT-012  accepted R6 26186464 A17: a created Pokemon starts with DVs 0; the existing
//                Pokemon in the box is untouched.
//   PBX-LNG-009  Gen II records store no language: no Language row in any Create panel.
//   PBX-SPC-001  accepted cfb70357: the shared species picker lists Gen II rows as "025 - Pikachu"
//                (picker reworked in R11 eb7fe8c9).
//   PBX-REL-002  accepted R3 62a2f8c3 C08: Release needs its own Y; A is not destructive; B keeps the
//                Pokemon (ClassicReleaseActionFix.inc changed in R11 dd69bc93 / 6f62b69e / 91cf155e).
#include "HostUi.h"
#include "fixtures/gsc_pokemon_fixture.h"

#include "Legacy/GSCReadOnlyTrainer.h"
#include "UI/PKSEFramebuffer.h"
#include "UI/TouchInput.h"
#include "UI/TrainerViewScreen.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace {
int failures = 0, checks = 0;
void check(bool ok, const std::string& what) {
    ++checks;
    if (!ok) { std::cerr << "FAILED: " << what << '\n'; ++failures; }
}
using View = UI::TrainerViewScreen::ViewMode;

struct Driver {
    UI::TrainerViewScreen& screen;
    UI::PKSEFramebuffer fb;
    UI::TouchInput touch;
    void frame(u64 down = 0) {
        PadState pad{down, down, 0};
        touch.update();
        screen.update(pad, touch);
        HostUi::texts.clear();
        HostUi::resetLayout();
        screen.draw(fb);
    }
    void press(u64 button) { frame(button); frame(); }
};
bool visible(const std::string& text) {
    for (const auto& t : HostUi::visibleText)
        if (t.text == text) return true;
    return false;
}
bool visibleContaining(const std::string& part) {
    for (const auto& t : HostUi::visibleText)
        if (t.text.find(part) != std::string::npos) return true;
    return false;
}
// The narrow Pokemon Actions sheet draws its row labels at x = 448 (NarrowPokemonActionMenu).
std::vector<std::string> sheetRows() {
    std::vector<std::string> rows;
    for (const auto& t : HostUi::visibleText)
        if (t.x == 448 && t.style == 1) rows.push_back(t.text);
    return rows;
}

// Boxes slot `slot` of Box 1 -> A (Empty Slot sheet) -> A (Add Pokemon): the Create workspace.
void openCreate(Driver& d, int slot) {
    d.screen.goBack = false;
    d.screen.detailViewActive = true;
    d.screen.selectedMode = View::Boxes;
    d.screen.selectedBoxIndex = 0;
    d.screen.selectedItemIndex = slot;
    d.frame();
    d.press(HidNpadButton_A);
    d.press(HidNpadButton_A);
}

// Round 11 (CreateRequiredFields): a Create draft needs one move before Keep. The same real-UI route
// test_ui_gen2_create_review_host uses: R to the Moves panel, open Move 1, pick the first compatible
// move, Apply.
bool chooseFirstMove(Driver& d) {
    bool opened = false;
    for (int panel = 0; panel < 3 && !opened; ++panel) {
        d.press(HidNpadButton_R);
        d.press(HidNpadButton_A);
        opened = visible("Move \xE2\x80\x94 Generation II");
        if (!opened) {
            d.press(HidNpadButton_A);
            opened = visible("Move \xE2\x80\x94 Generation II");
        }
        if (!opened) { d.press(HidNpadButton_B); d.press(HidNpadButton_B); }
    }
    if (!opened) return false;
    d.press(HidNpadButton_Down);
    d.press(HidNpadButton_A);
    for (int i = 0; i < 6 && !visible("Keep these move details"); ++i) d.press(HidNpadButton_B);
    for (int i = 0; i < 3; ++i) d.press(HidNpadButton_Down);
    d.press(HidNpadButton_A);   // Apply
    const bool applied = !visible("Keep these move details") && !visible("Empty move slot");
    for (int i = 0; i < 3; ++i) d.press(HidNpadButton_L);
    return applied && visible("DRAFT");
}

void scenario(const std::string& game, bool crystal) {
    const std::string what = game + " Create: ";
    std::vector<uint8_t> bytes = fixture(crystal ? C : GS, true);
    const std::vector<uint8_t> original = bytes;
    auto parsed = parse(bytes, crystal ? SourceGame::Crystal : SourceGame::Silver);
    check(static_cast<bool>(parsed), what + "fixture parses");
    if (!parsed) return;
    std::string error;
    auto trainer = PokeVault::Legacy::GSCReadOnlyTrainer::create(*parsed.save, error);
    check(trainer != nullptr, what + "trainer: " + error);
    if (!trainer || !trainer->stagedEditor()) return;
    auto* editor = trainer->stagedEditor();
    const auto existing = editor->boxedPokemon(0, 0, error);
    check(existing.has_value(), what + "Box 1 / Slot 1 holds the fixture Pokemon");
    const std::string id = crystal ? "crystal_gbc" : "silver_gbc";
    UI::TrainerViewScreen screen(*trainer, "Pokemon " + game, "/host/" + id + ".sav", 0, AccountUid{},
                                 PokeVault::Safety::SourceKind::RetroArchLegacy, id, "mGBA");
    Driver d{screen, {}, {}};
    d.frame();

    openCreate(d, 1);
    check(visible("PIKACHU") && visible("DRAFT"), what + "Create opens the shared workspace on a RetroArch save");
    // (The stats grid's "* HP DV derived / read-only" note is a field label, not a refusal.)
    check(!visibleContaining("source is read-only") && screen.storageStatus.find("read-only") == std::string::npos,
          what + "Create is not refused as \"RetroArch source is read-only\" (PBX-CRT-003)");

    // PBX-LNG-009: walk every panel (R) and back (L); Gen II has no Language row anywhere.
    bool language = false;
    for (int i = 0; i < 4; ++i) {
        for (const auto& t : HostUi::visibleText) language = language || t.text == "Language";
        d.press(HidNpadButton_R);
    }
    for (int i = 0; i < 4; ++i) d.press(HidNpadButton_L);
    check(!language, what + "no Language row in any Gen II Create panel (PBX-LNG-009)");

    // PBX-SPC-001: the shared species picker (R11 eb7fe8c9 one-sprite rework) lists Gen II rows as
    // "025 - Pikachu": a three-digit dex number, " - ", the name. B leaves the draft unchanged.
    d.press(HidNpadButton_A);
    int numbered = 0, rows = 0;
    for (const auto& t : HostUi::visibleText) {
        const auto& s = t.text;
        const bool looksNumbered = s.size() > 6 && std::isdigit(static_cast<unsigned char>(s[0])) &&
                                   std::isdigit(static_cast<unsigned char>(s[1])) &&
                                   std::isdigit(static_cast<unsigned char>(s[2])) && s.compare(3, 3, " - ") == 0;
        if (looksNumbered) ++numbered;
        if (s.size() > 4 && s.compare(3, 3, " - ") == 0) ++rows;
    }
    check(visible("025 - Pikachu"), what + "the species picker lists \"025 - Pikachu\" (PBX-SPC-001)");
    check(numbered >= 5 && numbered == rows, what + "every species row reads \"NNN - Name\" (PBX-SPC-001)");
    d.press(HidNpadButton_B);
    check(visible("PIKACHU") && visible("DRAFT"), what + "B closes the picker; the draft is still Pikachu");

    check(chooseFirstMove(d), what + "Move 1 chosen through the real Move picker");
    d.press(HidNpadButton_B);   // exit question
    d.press(HidNpadButton_A);   // Keep / Add

    // PBX-CRT-005: back on the box grid, the new Pokemon selected, no Actions sheet.
    check(!visible("DRAFT"), what + "Keep / Add closed the Create workspace");
    check(sheetRows().empty(), what + "no Pokemon Actions sheet opens after Add (PBX-CRT-005)");
    check(screen.selectedMode == View::Boxes && screen.selectedBoxIndex == 0 && screen.selectedItemIndex == 1,
          what + "the box grid shows Box 1 with the added slot selected (PBX-CRT-005)");

    // PBX-CRT-017: the concise toast, drawn over the box it names (R12-010 scope keeps it there).
    check(screen.storageStatus == "PIKACHU added to Box 1",
          what + "Add toast reads \"PIKACHU added to Box 1\" (got \"" + screen.storageStatus + "\")");
    check(visibleContaining("added to Box 1"), what + "the Add toast is drawn on the box grid (PBX-CRT-017)");
    check(!visibleContaining("staged copy only") && !visibleContaining(".srm"),
          what + "no implementation-facing toast wording (PBX-CRT-017)");
    d.frame();
    check(visibleContaining("added to Box 1"), what + "the Add toast stays while the box is shown");

    // PBX-CRT-001 / PBX-CRT-003 / PBX-CRT-012: staged, DVs 0, neighbour untouched, source untouched.
    const auto created = editor->boxedPokemon(0, 1, error);
    check(created.has_value() && created->species == 25, what + "Box 1 / Slot 2 holds the staged Pikachu");
    if (created)
        check(std::all_of(created->dvs.begin(), created->dvs.end(), [](uint8_t v) { return v == 0; }),
              what + "the created Pokemon starts with DVs 0 (PBX-CRT-012)");
    const auto after = editor->boxedPokemon(0, 0, error);
    check(existing && after && after->rawBody == existing->rawBody && after->nickname == existing->nickname,
          what + "the existing Pokemon is untouched");
    check(editor->hasPendingChanges(), what + "the Create is a staged change (PBX-CRT-003)");
    check(bytes == original, what + "the source save bytes are unchanged (PBX-CRT-001)");

    // PBX-REL-002 (owner PASS R1 / R3 62a2f8c3 C08): Release needs its own Y; A is not destructive; B
    // keeps the Pokemon. The Gen II Release wiring (ClassicReleaseActionFix.inc) changed in R11
    // dd69bc93 / 6f62b69e / 91cf155e.
    const auto count = [&] {
        int n = 0;
        while (n < 20 && editor->boxedPokemon(0, static_cast<std::size_t>(n), error)) ++n;
        return n;
    };
    const int before = count();
    const auto first = editor->boxedPokemon(0, 0, error);
    const auto second = editor->boxedPokemon(0, 1, error);
    const auto openRelease = [&] {
        d.screen.selectedMode = View::Boxes;
        d.screen.detailViewActive = true;
        d.screen.selectedBoxIndex = 0;
        d.screen.selectedItemIndex = 0;
        d.frame();
        d.press(HidNpadButton_A);
        const auto rows = sheetRows();
        const auto it = std::find(rows.begin(), rows.end(), std::string("Release"));
        if (it == rows.end()) return false;
        for (auto i = it - rows.begin(); i > 0; --i) d.press(HidNpadButton_Down);
        d.press(HidNpadButton_A);
        return true;
    };
    check(openRelease(), what + "Box 1 / Slot 1 Actions list Release");
    d.press(HidNpadButton_A);
    check(count() == before, what + "A on the Release question releases nothing (PBX-REL-002)");
    d.press(HidNpadButton_B);
    const auto kept = editor->boxedPokemon(0, 0, error);
    check(count() == before && first && kept && kept->rawBody == first->rawBody,
          what + "B cancels the Release question; the Pokemon stays (PBX-REL-002)");
    for (int i = 0; i < 3 && !sheetRows().empty(); ++i) d.press(HidNpadButton_B);
    check(openRelease(), what + "Release offered again");
    d.press(HidNpadButton_Y);
    check(count() == before - 1, what + "only Y releases, and it is staged (PBX-REL-002)");
    const auto now = editor->boxedPokemon(0, 0, error);
    check(second && now && now->rawBody == second->rawBody, what + "the packed box closes the gap with the next Pokemon");
    check(bytes == original, what + "Release leaves the source save bytes unchanged");
}
} // namespace

int main() {
    scenario("Silver", false);
    scenario("Crystal", true);
    if (failures) {
        std::cerr << failures << " of " << checks << " check(s) failed\n";
        return EXIT_FAILURE;
    }
    std::cout << "Recheck regressions through the real screen (" << checks << " checks): PASS\n";
    return EXIT_SUCCESS;
}
