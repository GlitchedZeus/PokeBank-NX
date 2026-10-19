// R2-BUG-013: every controller button named in a help window or a modal instruction is a coloured
// badge (ScreenChrome::buttonGlyph), never plain "A/B/X/Y" text. The pure splitter
// (UI/GlyphInstruction.h) is exercised on the app's real strings:
// - help rows: Game Browser Controls (the owner's screenshot reference), Inventory Help and the
//   app-shell help — each key part becomes badges, the description stays text;
// - instruction prose: delete / forget-link confirms, "press A again" notices, hold-Y hints;
// - prose that merely contains an article "A" or a word like "Up" or "Left" gets no badge.
#include "UI/GlyphInstruction.h"

#include <cassert>
#include <iostream>
#include <string>

namespace GI = PokeBank::UIModel::GlyphInstruction;
using GI::Kind;

namespace {

std::string glyphs(const GI::Segments& s) {
    std::string r;
    for (const auto& seg : s)
        if (seg.kind == Kind::Glyph) r += (r.empty() ? "" : ",") + seg.text;
    return r;
}
std::string text(const GI::Segments& s) {
    std::string r;
    for (const auto& seg : s) r += seg.kind == Kind::Text ? seg.text : "[" + seg.text + "]";
    return r;
}
void expect(const GI::Segments& s, const std::string& wantGlyphs, const char* what) {
    if (glyphs(s) != wantGlyphs) {
        std::cerr << what << ": got {" << glyphs(s) << "} want {" << wantGlyphs << "} in \"" << text(s) << "\"\n";
        assert(false);
    }
}

} // namespace

int main() {
    // Help rows: the Game Browser Controls window (TrainerViewScreenBase.inc), verbatim.
    expect(GI::helpLine("D-pad / Left Stick   Navigate (hold to scroll)"), "D-pad/Stick", "dpad row");
    expect(GI::helpLine("A   Open or show Pokémon actions"), "A", "A row");
    expect(GI::helpLine("B   Back or cancel"), "B", "B row");
    expect(GI::helpLine("L / R   Previous or next box / tab"), "L/R", "LR row");
    expect(GI::helpLine("ZL / ZR   Larger jumps where available"), "ZL/ZR", "ZLZR row");
    expect(GI::helpLine("X / Y   Labeled contextual tools"), "X,Y", "XY row");
    expect(GI::helpLine("+   Options / compatibility     -   Close help"), "+,-", "plus/minus row");
    // Banner lines without a key part stay text.
    expect(GI::helpLine("Legacy Storage is app-owned bank.dat, NOT Master Vault"), "", "banner");
    expect(GI::helpLine("INSTALLED SOURCE: read-only browsing"), "", "source banner");
    expect(GI::helpLine("BACKUP WORKSPACE: edits affect backup files only"), "", "workspace banner");
    // Description text is kept verbatim after the badge.
    assert(text(GI::helpLine("A   Open or show Pokémon actions")) == "[A]  Open or show Pokémon actions");
    assert(text(GI::helpLine("X / Y   Labeled contextual tools")) == "[X] / [Y]  Labeled contextual tools");

    // Inventory Help (ClassicInventoryOverlay.cpp) and the app-shell help (AppShellScreen.cpp).
    expect(GI::helpLine("A   Edit amount (only when this item has an editable quantity)"), "A", "inv A");
    expect(GI::helpLine("X   Add item from the exact-game, exact-category catalog"), "X", "inv X");
    expect(GI::helpLine("Y   Remove the highlighted item"), "Y", "inv Y");
    expect(GI::helpLine("L / R   Previous / next supported inventory category"), "L/R", "inv LR");
    expect(GI::helpLine("D-pad / Left Stick   Navigate (hold to repeat)"), "D-pad/Stick", "inv dpad");
    expect(GI::helpLine("+   Inventory Options     -   Help     B   Back / Cancel"), "+,-,B", "inv triple");
    expect(GI::helpLine("Empty valid categories show (None); X Add remains available."), "", "inv note");
    expect(GI::helpLine("A   Open the focused item"), "A", "shell A");
    expect(GI::helpLine("B   Return to Games"), "B", "shell B");

    // Instruction prose (SaveSelectScreen / BackupSelectionScreen modals and notices).
    expect(GI::sentence("Removes its app backup and exported copy. Press Y to delete; A or B keeps it."),
           "Y,A,B", "delete confirm");
    expect(GI::sentence("Y: Confirm forgetting link     B: Keep assignment"), "Y,B", "forget link");
    assert(text(GI::sentence("Y: Confirm forgetting link     B: Keep assignment")) ==
           "[Y] Confirm forgetting link     [B] Keep assignment");
    expect(GI::sentence("Focused backup brought into view; press A or X again"), "A,X", "backup reveal");
    expect(GI::sentence("Game file linked. Press A to launch this source."), "A", "linked");
    expect(GI::sentence("This is the full artwork game browser. Press Y on Product Home for the quick drawer."),
           "Y", "browser note");
    expect(GI::sentence("Hold Y and use D-pad/Stick to resize; release Y to pick up"), "Y,D-pad/Stick,Y", "hold Y");
    expect(GI::sentence("Y: Remove  |  A/B: Cancel"), "Y,A,B", "A/B pair");
    expect(GI::sentence("L/R: Box  |  ZL/ZR: Jump"), "L/R,ZL/ZR", "shoulders");

    // No badge for ordinary words: an article "A", "Up", "Left", "Down", a hyphen, "or" alone.
    expect(GI::sentence("A backup was created before editing."), "", "article A");
    expect(GI::sentence("Up to 30 Pokémon per box; Left column shows the party."), "", "Up/Left words");
    expect(GI::sentence("Source - read only"), "", "hyphen");
    expect(GI::sentence("Original save files remain untouched."), "", "plain");

    // Canonical tokens are exactly the ones buttonGlyph draws.
    for (const char* t : {"A", "B", "X", "Y", "L", "R", "ZL", "ZR", "L/R", "ZL/ZR", "+", "-", "D-pad/Stick",
                          "Up/Down", "Left/Right", "Left", "Right", "Up", "Down"})
        assert(GI::canonicalButton(t) == t);
    assert(GI::canonicalButton("L / R") == "L/R" && GI::canonicalButton("D-pad / Left Stick") == "D-pad/Stick");
    assert(GI::canonicalButton("Pokémon").empty() && GI::canonicalButton("AB").empty());

    std::cout << "R2-BUG-013 glyph instructions: help rows and modal prose name every button as a badge, "
                 "ordinary words stay text PASS\n";
}
