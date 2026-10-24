// Gen I / Gen II shared-editor contract fixes (Thread C audit):
//  - Review -> Discard Staged is a two-step, modal confirmation (pure model + both live routes).
//  - Gen I exit dialog footer offers only the dialog's own B/Y/A (no "Random DVs" under it).
//  - Gen I workspace keeps its stick repeat state current while a picker owns input.
//  - Gen II Species row tap opens the picker exactly like A; the Move editor's picker entry uses
//    the same normalization as the controller path.
#include "UI/Gen12ReviewDiscardConfirm.h"
#include "UI/SharedReviewPresentation.h"
#include "UI/ClassicStagedExit.h"
#include "UI/ClassicReviewOrigin.h"

#include <cassert>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

namespace Discard = PokeBank::UIModel::Gen12ReviewDiscard;
using Outcome = Discard::Outcome;

namespace {

std::string read(const char* path) {
    std::ifstream in(path, std::ios::binary);
    assert(in);
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}
std::string between(const std::string& text, const char* begin, const char* end) {
    const auto a = text.find(begin);
    if (a == std::string::npos) std::cerr << "Missing route start: " << begin << '\n';
    assert(a != std::string::npos);
    const auto b = text.find(end, a + 1);
    if (b == std::string::npos) std::cerr << "Missing route end: " << end << '\n';
    assert(b != std::string::npos);
    return text.substr(a, b - a);
}
void has(const std::string& text, const char* needle) {
    if (text.find(needle) == std::string::npos) std::cerr << "Missing: " << needle << '\n';
    assert(text.find(needle) != std::string::npos);
}
void lacks(const std::string& text, const char* needle) {
    if (text.find(needle) != std::string::npos) std::cerr << "Forbidden: " << needle << '\n';
    assert(text.find(needle) == std::string::npos);
}

void modelContract() {
    Discard::State s;
    // Nothing open: ordinary review input is not handled; Y alone (Gen II) is not Discard.
    assert(Discard::step(s, false, false, false) == Outcome::NotHandled);
    assert(Discard::step(s, false, true, false) == Outcome::NotHandled && !s.confirming);
    assert(Discard::step(s, false, false, true) == Outcome::NotHandled);
    // First Discard press only asks.
    assert(Discard::step(s, true, false, false) == Outcome::AskConfirm && s.confirming);
    // While open: A / navigation / repeated non-Y input never discards or leaves.
    assert(Discard::step(s, false, false, false) == Outcome::Swallowed && s.confirming);
    assert(Discard::step(s, true, false, false) == Outcome::Swallowed && s.confirming); // Gen II X again
    // B cancels and wins over a simultaneous Y.
    assert(Discard::step(s, false, true, true) == Outcome::Cancelled && !s.confirming);
    // Gen I uses Y for both steps: the same press cannot both ask and confirm.
    assert(Discard::step(s, true, true, false) == Outcome::AskConfirm && s.confirming);
    assert(Discard::step(s, true, true, false) == Outcome::Discard && !s.confirming);
    // After a discard the next press starts over.
    assert(Discard::step(s, true, false, false) == Outcome::AskConfirm);
    static_assert([] {
        Discard::State c;
        return Discard::step(c, true, false, false) == Outcome::AskConfirm &&
               Discard::step(c, false, true, false) == Outcome::Discard;
    }());
}

void gen2Routes() {
    const auto surface = read("src/UI/Gen2SharedPokemonSurface.inc");
    const auto review = between(surface, "bool handleSharedReview(", "void drawSharedActionSurface(");
    has(review, "Discard::step(sharedSurfaceStateFor(screen).reviewDiscard,");
    // editor->discard() appears exactly once, inside the confirmed outcome.
    const auto confirmed = between(review, "case Discard::Outcome::Discard:", "const auto& changes");
    has(confirmed, "editor->discard();");
    assert(review.find("editor->discard();") == review.find("editor->discard();", 0) &&
           review.find("editor->discard();", review.find("editor->discard();") + 1) == std::string::npos);
    has(review, "if (down & HidNpadButton_X)");
    assert(review.find("Discard::step(") < review.find("if (down & HidNpadButton_X)"));
    const auto parity = read("src/UI/Gen2SharedSurfaceParity.inc");
    const auto draw = between(parity, "void drawParityGen2Review(", "void drawParityGen2Clone(");
    has(draw, "drawSharedReviewSurface(screen,fb)");
    const auto sharedDraw=between(surface,"void drawSharedReviewSurface(","void drawSharedDetails(");
    has(sharedDraw,"reviewDiscard.confirming");
    has(sharedDraw,"SharedEditorChrome::drawReviewPanel");
    has(sharedDraw,"SharedEditorChrome::drawReviewDiscardConfirm");
    // Gen II Review touch: reviewRow is the shared presenter's PAGE. A swipe turns one page; a tap
    // never writes a row index into it (that jumped pages on hardware).
    const auto touch = read("src/UI/Gen2HardwareWorkspaceFix.inc");
    const auto reviewTouch = between(touch, "    // The shared paged Review (round 5)", "\n}\n");
    has(reviewTouch, "PokeBank::UIModel::SharedReview::move(");
    has(reviewTouch, "!sharedSurfaceStateFor(screen).reviewDiscard.confirming");
    lacks(touch, "legacy.reviewRow = i;");

    const auto unified = read("src/UI/Gen2UnifiedPokemonWorkspace.inc");
    // The editable (Edit/Create) route, after the item/confirm/move-editor modals.
    const auto editRoute = unified.substr(unified.find("return handleUnifiedMoveEditor(screen, state, down, held, stickX, stickY);"));
    const auto tap = between(editRoute, "const int touchedDetail = updateUnifiedDetailsTouch(",
                             "const uint64_t nav = screen.controllerNavigation.apply(");
    has(tap, "if (!activateUnifiedField(screen, state, unified.focus))");
    has(tap, "openPickerForFocusedField(screen, state, pickerStateFor(screen), HidNpadButton_A)");
    const auto moveEntry = between(unified, "if (unified.moveEditorRow == 0) {", "if (unified.moveEditorRow == 1) {");
    has(moveEntry, "normalizeHardwareMovePicker(screen, state, picker);");
}

void gen1Routes() {
    const auto ux2 = read("src/UI/Gen1PokemonEditorOverlayUXCleanup2.inc");
    const auto review = between(ux2, "if (state.mode == UX2Mode::Review) {", "void ux2DrawPanel(");
    has(review, "Discard::step(state.reviewDiscard, y, y,");
    const auto confirmed = between(review, "case Discard::Outcome::Discard:", "const auto lines = pendingLines(screen);");
    has(confirmed, "discardAll(screen);");
    assert(review.find("discardAll(screen);") == review.find("discardAll(screen);", 0) &&
           review.find("discardAll(screen);", review.find("discardAll(screen);") + 1) == std::string::npos);
    has(review, "if (down & HidNpadButton_X)");
    assert(review.find("Discard::step(") < review.find("if (down & HidNpadButton_X)"));

    const auto ux3 = read("src/UI/Gen1PokemonEditorOverlayUXCleanup3.inc");
    const auto sharedDraw=between(ux3,"void ux3DrawReview(","void ux3DrawStagedView(");
    has(sharedDraw,"SharedEditorChrome::drawReviewPanel");
    has(sharedDraw,"SharedEditorChrome::drawReviewDiscardConfirm");
    // The confirmation stays modal on touch (only its own B / Y act) ...
    has(ux3, "} else if (state.mode == UX2Mode::Review && state.reviewDiscard.confirming) {");
    // ... and only the shared presenter paints Review: no older confirm panel or footer is drawn
    // over drawReviewPanel / drawReviewDiscardConfirm (round 5, owner Oct 24 "Gen I/II use
    // outdated, different Review screens").
    lacks(ux3, "Gen12ReviewDiscard::kTitle");
    lacks(ux3, "{\"A\", \"Export Copy\"}");
    const auto navBar = between(ux3, "        case UX2Mode::Review:\n            // The shared Review panel", "case UX2Mode::Closed: return;");
    lacks(navBar, "drawNavBar(");

    // Exit dialog footer: only the dialog's actions while it is open.
    const auto fix = read("src/UI/Gen1PokemonEditorFoundationHardwareFix.inc");
    const auto footer = between(fix, "} else if (foundationStateFor(screen).editExitConfirm) {", "} else if (add) {");
    has(footer, "{\"B\", \"Back\"}, {\"Y\", \"Discard\"}, {\"A\", add ? \"Keep\" : \"Save\"}");
    lacks(footer, "Random DVs");
    lacks(footer, "\"Edit\"");

    // Held-stick state is tracked on both routes that hand input to another surface.
    const auto foundation = read("src/UI/Gen1PokemonEditorOverlayFoundation.inc");
    size_t at = 0;
    int tracked = 0;
    while ((at = foundation.find("if (state.picker != UX2Picker::None || !foundationMainMode(state.mode)) {", at)) != std::string::npos) {
        const auto block = foundation.substr(at, foundation.find("return handleInputUXCleanup3(", at) - at);
        has(block, "foundationStateFor(screen).navigation.apply(");
        ++tracked;
        ++at;
    }
    assert(tracked == 2);
}


void lowSeverityRoutes() {
    const auto foundation = read("src/UI/Gen1PokemonEditorOverlayFoundation.inc");
    // X Stage Add resets the session exactly like the dialog's A Keep.
    const auto stageX = between(foundation, "if (add && (down & HidNpadButton_X)) {", "if (down & HidNpadButton_B) {");
    has(stageX, "if (ux3StageAdd(screen)) {");
    has(stageX, "foundation.lastMainMode = UX2Mode::Closed;");
    // The exit dialog only claims unsaved changes when the edit session is dirty.
    const auto exitText = between(foundation, "void foundationDrawExitConfirm(", "drawGlyphButton(");
    has(exitText, "foundationEditSessionDirty(screen)");
    has(exitText, "No changes in this edit session.");

    // Gen I Move picker: one width for every draw path and the drag region.
    const auto ux3 = read("src/UI/Gen1PokemonEditorOverlayUXCleanup3.inc");
    has(ux3, "constexpr int kGen1MovePickerW = 760;");
    has(ux3, "touch, (1280 - kGen1MovePickerW) / 2 + 24, (672 - 520) / 2 + 86,");
    lacks(ux3, "touch, 164, 162, 952,");
    const auto fix = read("src/UI/Gen1PokemonEditorFoundationHardwareFix.inc");
    has(fix, "const int w = state.picker == UX2Picker::Species ? 1080 : kGen1MovePickerW;");

    // Gen II Shiny toggle always reports the DVs it rewrote.
    const auto gen2 = read("src/UI/Gen2PokemonEditorFoundation.inc");
    const auto toggle = between(gen2, "void toggleShiny(", "bool randomizeDVs(");
    has(toggle, "\"Shiny off: DVs \" + describeDVChanges(before, dvs)");
    has(toggle, "\"Shiny on: \" + changes");
    has(toggle, "changed gender for this species ratio; \" + changes");
}

} // namespace

// Round 5: a Review opened outside Box Actions (staged-exit menu, Bag, Trainer card) closes back
// to the screen underneath; only Box Actions -> Review Changes returns to an Actions sheet.
struct FakeScreen { std::string sourceGameId; };
void reviewOriginContract() {
    namespace Origin = PokeBank::UIModel::ClassicReviewOrigin;
    FakeScreen red{"red_gb"}, crystal{"crystal_gbc"};
    assert(!Origin::takeStandalone(red));          // Box Actions -> Review: back to Actions
    Origin::markStandalone(red);
    assert(Origin::takeStandalone(red));           // standalone: B closes to the screen beneath
    assert(!Origin::takeStandalone(red));          // taken once; the next Review is unmarked
    Origin::markStandalone(red);
    assert(!Origin::takeStandalone(crystal));      // another screen/game never inherits the mark
    assert(!Origin::takeStandalone(red));

    const auto composite = read("src/UI/TrainerViewScreenCompositeOverlay.cpp");
    const auto open = between(composite, "bool TrainerViewScreen::openStagedReview(", "namespace {");
    has(open, "PokeBank::UIModel::ClassicReviewOrigin::markStandalone(*this);");
    const std::pair<const char*, int> exits[] = {
        {"src/UI/Gen1PokemonEditorOverlayUXCleanup2.inc", 1}, {"src/UI/Gen2SharedPokemonSurface.inc", 2},
        {"src/UI/Gen3SharedPokemonSurface.inc", 2}, {"src/UI/Gen4SharedPokemonSurface.inc", 2}};
    for (const auto& [file, expected] : exits) {
        const auto text = read(file);
        int uses = 0;
        for (auto at = text.find("ClassicReviewOrigin::takeStandalone(screen)"); at != std::string::npos;
             at = text.find("ClassicReviewOrigin::takeStandalone(screen)", at + 1)) ++uses;
        if (uses != expected) std::cerr << file << ": takeStandalone uses " << uses << '\n';
        assert(uses == expected); // every Review B / closing Discard All consults the origin
    }
    // Gen I / II Bag -> Options -> Review is the shared Review, not a separate list.
    const auto bag = read("src/UI/ClassicInventoryOverlay.cpp");
    has(bag, "if (!screen.openStagedReview(false))");
    lacks(bag, "Pending Classic Save Changes");
    lacks(bag, "reviewActive");
}

int main() {
    reviewOriginContract();
    namespace Review=PokeBank::UIModel::SharedReview;
    const auto rows=Review::fromLines({"Party Slot 1: Move 1: Thunder Shock -> Thunderbolt",
        "Bag / Potion: 1 -> 2", "Trainer Name: RED -> BLUE", "Unstructured native note"});
    assert(rows.size()==4 && rows[0].first=="Party Slot 1");
    assert(rows[0].second=="Move 1: Thunder Shock -> Thunderbolt");
    assert(rows[1].second=="1 -> 2" && rows[3].second=="Unstructured native note");
    for(size_t count=0;count<50;++count) {
        assert(Review::pages(count)>=1);
        size_t page=0;
        for(size_t i=0;i<100;++i)page=Review::move(page,1,count);
        assert(page+1==Review::pages(count));
        for(size_t i=0;i<100;++i)page=Review::move(page,-1,count);
        assert(page==0);
    }
    namespace Exit=PokeBank::UIModel::ClassicStagedExit;
    for(int touch=0;touch<4;++touch) {
        const auto expected=touch==0?Exit::Action::ContinueEditing:touch==1?Exit::Action::DiscardExit:
            touch==2?Exit::Action::Review:Exit::Action::ExportCopy;
        assert(Exit::choose(false,false,false,false,touch)==expected);
        assert(Exit::choose(true,true,true,true,touch)==Exit::Action::ContinueEditing);
    }
    for(const char* path:{"src/UI/Gen1PokemonEditorOverlayUXCleanup3.inc",
        "src/UI/Gen2SharedPokemonSurface.inc","src/UI/Gen3SharedPokemonSurface.inc",
        "src/UI/Gen4SharedPokemonSurface.inc","src/UI/Gen5SharedReadOnlySurface.inc"}) {
        const auto surface=read(path);
        has(surface,"SharedEditorChrome::drawReviewPanel");
        has(surface,"SharedEditorChrome::drawReviewDiscardConfirm");
    }
    modelContract();
    gen2Routes();
    gen1Routes();
    lowSeverityRoutes();
    std::cout << "Gen I/II review discard confirmation, exit-dialog footer, stick handoff and Gen II picker entry: PASS\n";
}
