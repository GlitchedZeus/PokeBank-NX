// Round 11 H23 / H31 / H32: the one Gen I-V box carry rule set (include/UI/ClassicCarryRules.h) and
// the source contracts that tie every generation to it.
//
//  * H23 (owner): while carrying, A places, B cancels, Y / X are refused with a hint and consumed;
//    Gen I-IV (handleInput) and Gen V (handleGen5CarryInput) route through the same carryKey().
//    The carry footer, the pickup status lines and Help say A, never "Y places" / "Y Drop".
//  * H31 (owner): every generation's Actions -> Clone (Gen I both sheets, Gen II both entry points,
//    Gen III, Gen IV, Gen V) closes its sheet and starts the shared Clone carry
//    (ClassicPackedMove::beginCloneFromAction). No number keypad, no Gen II Clone dialog, and no
//    Actions menu reopened after the clone is placed.
//  * H32: an occupied or protected destination is refused for every generation (never overwritten);
//    Gen I / II packed boxes land at the box's next free slot; a full box is refused.
//  * Round 12 R12-001 (owner H03 / H16): Gen V Boxes use the Gen I-IV keys - Y tap / hold Move, +
//    Review - through the same overlay (handleGen5BoxInput), footer and Help.
//  * Round 12 R12-002 (owner H03 / H04): routine pickup / place / multi-move / clone / cancel / swallowed
//    keys post no toast (EditFeedback::Kind::Carry); refusals and failures still do.
//  * Round 12 R12-003 (owner H04): a packed (Gen I / II) box has one focus - the cursor is the landing slot.
// The end-to-end behaviour through the real TrainerViewScreen is tests/test_ui_clone_carry_host.cpp.
#include "UI/ClassicCarryRules.h"
#include "UI/ClassicHelpPresentation.h"
#include "UI/EditFeedback.h"

#include <algorithm>
#include <cassert>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

namespace {
namespace Carry = PokeBank::UIModel::ClassicCarry;
namespace Help = PokeBank::UIModel::ClassicHelp;

int failures = 0;
void check(bool ok, const std::string& what) {
    if (!ok) {
        std::cerr << "FAILED: " << what << '\n';
        ++failures;
    }
}

std::string read(const char* path) {
    std::ifstream in(path, std::ios::binary);
    assert(in && "run from the repository root");
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}
// The text from `begin` up to the next `end` (empty when either is missing).
std::string between(const std::string& text, const std::string& begin, const std::string& end) {
    const auto a = text.find(begin);
    if (a == std::string::npos) return {};
    const auto b = text.find(end, a + begin.size());
    if (b == std::string::npos) return {};
    return text.substr(a, b - a);
}
bool has(const std::string& text, const std::string& needle) { return text.find(needle) != std::string::npos; }

void testKeys() {
    using Key = Carry::Key;
    check(Carry::carryKey(Carry::kButtonA) == Key::Place, "A places");
    check(Carry::carryKey(Carry::kButtonB) == Key::Cancel, "B cancels");
    check(Carry::carryKey(Carry::kButtonA | Carry::kButtonB) == Key::Cancel, "B wins over A");
    check(Carry::carryKey(Carry::kButtonY) == Key::Consume, "Y never places");
    check(Carry::carryKey(Carry::kButtonX) == Key::Consume, "X is consumed");
    check(Carry::carryKey(Carry::kButtonY | Carry::kButtonA) == Key::Place, "A with Y still places once");
    check(Carry::carryKey(Carry::kButtonPlus) == Key::Leave, "+ cancels and falls through");
    check(Carry::carryKey(1ull << 6) == Key::None && Carry::carryKey(1ull << 7) == Key::None, "L / R navigate");
    check(Carry::carryKey(1ull << 12) == Key::None, "D-pad navigates");
    for (const auto intent : {Carry::Intent::Move, Carry::Intent::Clone}) {
        const auto footer = Carry::carryFooter(intent, false);
        check(std::strcmp(footer.hints[0].glyph, "A") == 0 && std::strcmp(footer.hints[2].glyph, "B") == 0 &&
                  std::strcmp(footer.hints[2].label, "Cancel") == 0 && std::strcmp(footer.hints[1].glyph, "L/R") == 0,
              "carry footer: A place, L/R box, B cancel");
    }
    check(std::strcmp(Carry::carryFooter(Carry::Intent::Move, false).hints[0].label, "Place") == 0, "Move footer");
    check(std::strcmp(Carry::carryFooter(Carry::Intent::Move, true).hints[0].label, "Place Group") == 0, "group footer");
    check(std::strcmp(Carry::carryFooter(Carry::Intent::Clone, false).hints[0].label, "Place Clone") == 0, "Clone footer");
    std::cout << "carry keys: A places, B cancels, Y / X consumed ok\n";
}

// R12-002: routine carry steps are silent; refusals are not.
void testRoutineFeedback() {
    namespace F = PokeBank::UIModel::EditFeedback;
    check(F::routine(F::Kind::Carry) && !F::postsStatus(F::Kind::Carry, F::Outcome::Applied),
          "a routine carry step posts no toast");
    check(F::postsStatus(F::Kind::Carry, F::Outcome::Refused), "a carry refusal still posts");
    struct Fake {
        std::string text = "That slot is occupied";
        int frames = 200;
        void postStatus(const std::string& t, int f) { text = t; frames = f; }
    } screen;
    F::applied(screen, F::Kind::Carry);
    check(screen.frames == 200, "a pickup leaves the line alone");
    F::settled(screen, F::Kind::Carry);
    check(screen.text.empty() && screen.frames == 0, "a placed / cancelled carry retires the refusal it answered");
    std::cout << "routine carry feedback: silent on success, refusals kept ok\n";
}

// R12-003: one focus over a packed box; Gen III-V groups keep their shape.
void testPackedFocusAndGroups() {
    using Carry::Intent;
    for (const int capacity : {20, 30}) {
        for (int count = 0; count <= capacity; ++count)
            for (int slot = -1; slot <= capacity; ++slot) {
                const int clone = Carry::packedFocusSlot(Intent::Clone, count, capacity, slot);
                const int move = Carry::packedFocusSlot(Intent::Move, count, capacity, slot);
                const int end = count < capacity ? count : capacity - 1;
                check(clone == end, "packed Clone: the cursor is the box end");
                check(move >= 0 && move <= end && (slot < 0 || slot > end || move == slot),
                      "packed Move: the cursor stays on an insertion point");
                // The Clone focus is exactly where decideCloneDrop lands it (or refuses a full box).
                Carry::CloneTarget t{true, capacity, count, clone, clone < count, false};
                const auto d = Carry::decideCloneDrop(t);
                check(count < capacity ? (d.outcome == Carry::Drop::Accept && d.slot == clone)
                                       : d.outcome == Carry::Drop::BoxFull,
                      "the packed Clone focus is the landing slot, or a full box is refused");
                check(Carry::ghostSlot(t) == clone, "the ghost is drawn on the cursor: one indicator");
            }
    }
    check(Carry::packedFocusSlot(Intent::Clone, 3, 0, 0) == -1, "no box, no focus");
    std::vector<std::pair<std::size_t, std::size_t>> out;
    check(Carry::sparseGroupLanding({0, 1, 6, 7}, 10, out) && out.size() == 4 && out[0].second == 10 &&
              out[1].second == 11 && out[2].second == 16 && out[3].second == 17,
          "a 2x2 group keeps its shape at the cursor");
    check(!Carry::sparseGroupLanding({0, 1}, 5, out), "a group running off the right edge does not fit");
    check(!Carry::sparseGroupLanding({0, 6}, 24, out), "a group running off the bottom does not fit");
    check(Carry::sparseGroupLanding({7, 9}, 0, out) && out[0].second == 0 && out[1].second == 2,
          "sparse holes inside the group are kept");
    std::cout << "packed focus is the landing slot; sparse groups keep their shape ok\n";
}

// Every generation's destination rule. Gen I 20 / Gen II 20 (30 Japanese) packed; Gen III-V 30 sparse.
void testCloneDestinations() {
    using Drop = Carry::Drop;
    struct Gen { int generation; bool packed; int capacity; };
    for (const Gen g : {Gen{1, true, 20}, Gen{2, true, 20}, Gen{2, true, 30}, Gen{3, false, 30}, Gen{4, false, 30},
                        Gen{5, false, 30}}) {
        const std::string tag = "Gen " + std::to_string(g.generation) + ": ";
        Carry::CloneTarget t;
        t.packed = g.packed;
        t.capacity = g.capacity;
        t.count = 4;
        // Occupied slot: refused, never overwritten (the source slot itself is occupied too).
        t.slot = 2; t.occupied = true;
        check(Carry::decideCloneDrop(t).outcome == Drop::Occupied, tag + "occupied slot refused");
        check(Carry::ghostSlot(t) == 2, tag + "a refused ghost stays on the hovered cell");
        // Protected slot (unreadable / already staged / not a native blank): refused.
        t.slot = 9; t.occupied = false; t.protectedSlot = true;
        check(Carry::decideCloneDrop(t).outcome == Drop::Protected, tag + "protected slot refused");
        // An empty slot is accepted: sparse lands there; packed lands at the next free slot.
        t.protectedSlot = false;
        const auto ok = Carry::decideCloneDrop(t);
        check(ok.outcome == Drop::Accept && ok.slot == (g.packed ? 4 : 9), tag + "empty slot accepted");
        check(Carry::ghostSlot(t) == ok.slot, tag + "the ghost is drawn where the clone lands");
        // Out of range / no slot.
        t.slot = g.capacity;
        check(Carry::decideCloneDrop(t).outcome == Drop::NoSlot, tag + "slot past the box refused");
        t.slot = -1;
        check(Carry::decideCloneDrop(t).outcome == Drop::NoSlot, tag + "no slot refused");
        if (g.packed) {
            // Packed: a full box is refused wherever the cursor is.
            t.count = g.capacity; t.slot = 0; t.occupied = true;
            check(Carry::decideCloneDrop(t).outcome == Drop::BoxFull, tag + "full packed box refused");
            // A slot inside the packed list is occupied even if the caller's view lags.
            t.count = 4; t.slot = 1; t.occupied = false;
            check(Carry::decideCloneDrop(t).outcome == Drop::Occupied, tag + "a slot inside the list is occupied");
        }
    }
    for (const auto drop : {Drop::Occupied, Drop::Protected, Drop::BoxFull, Drop::NoSlot})
        check(has(Carry::dropStatus(drop), "B cancels"), std::string("refusal status says how to leave: ") +
                                                          Carry::dropStatus(drop));
    check(has(Carry::dropStatus(Drop::Occupied), "nothing was overwritten"), "occupied status is explicit");
    std::cout << "clone destinations: occupied / protected / full refused, packed next free slot ok\n";
}

void testOverlayUsesTheRules() {
    const auto overlay = read("src/UI/ClassicPackedMoveOverlay.inc");
    // Both dispatch sites share carryKey(); neither places on Y any more.
    const auto gen5 = between(overlay, "bool handleGen5CarryInput(", "bool handleInput(");
    const auto classic = between(overlay, "    if (state.active) {", "    if (state.selecting || state.yPending)");
    for (const auto* site : {&gen5, &classic}) {
        check(!site->empty(), "carry dispatch site found");
        check(has(*site, "switch (Carry::carryKey(down))"), "dispatch routes through ClassicCarry::carryKey");
        check(has(*site, "case Carry::Key::Place: return place(screen, state);"), "A -> place()");
        check(has(*site, "case Carry::Key::Cancel: return cancel(screen, state);"), "B -> cancel()");
        check(has(*site, "case Carry::Key::Consume: return true;"), "Y / X are consumed");
        check(!has(*site, "postStatus"), "R12-002: no toast from the carry keys");
        check(!has(*site, "HidNpadButton_Y) return place("), "Y no longer places");
    }
    check(has(classic, "D-pad and L/R keep using the accepted box navigation path"), "navigation comment kept");
    // R12-003: a packed carry settles on its landing slot and owns its navigation.
    check(has(classic, "settlePackedFocus(screen, state);") &&
              has(classic, "if (packedCarry(state)) return packedCarryNavigation("),
          "a packed carry's cursor is its landing slot");
    check(has(overlay, "Carry::packedFocusSlot(state.intent, packedCount(screen, box), classicCapacity(screen), slot)"),
          "the packed focus comes from the shared rule");
    // R12-002: no routine pickup / place / clone / cancel / hint toast anywhere in the carry.
    for (const char* routine : {"picked up - A", " Pokemon moved", "moved in the staged", "Clone staged in",
                                "Move cancelled", "Clone cancelled", "returned to", "Multi-select: hold Y",
                                "Touch multi-select: drag", "Hold Y and use", "multi-select cancelled",
                                "Multi-select cancelled", "Carry::refuseHint(", "clonePickupStatus("})
        check(!has(overlay, routine), std::string("R12-002: no routine toast: ") + routine);
    for (const char* kept : {"kNoPokemonToMove", "Carry::dropStatus(", "Carry::kCloneSourceChangedStatus",
                             "Group does not fit at that exact destination", "Multi requires empty destination slots",
                             "The selected rectangle contains no Pokemon"})
        check(has(overlay, kept), std::string("R12-002: refusals still post: ") + kept);
    check(has(overlay, "\"There is no Pokemon in this slot to move\""), "the one pickup message the owner keeps");
    // No "Y places" / "Place with Y" / "Y Drop" text remains in the carry.
    for (const char* stale : {"Y places", "Place with Y", "\"Drop\"", "\"Drop Group\"", "Y drops"})
        check(!has(overlay, stale), std::string("no stale Y-place text: ") + stale);
    check(has(overlay, "Carry::carryFooter(state.intent, state.visuals.size() > 1)"), "footer from the shared model");
    // The browse footer and the multi-select hold keep Y (pickup, unchanged).
    // (Round 11 X-IMPEXP builds the browse footer from one capability model: Y is still the pickup.)
    check(has(overlay, "{\"Y\", occupied ? \"Move / Hold Multi\" : \"Hold Multi\"}") &&
              has(overlay, "{\"Y\", \"Hold / Release Pick Up\"}"),
          "Y stays the pickup key in the browse and multi-select footers");
    // The libnx bits the model restates.
    check(has(overlay, "static_assert(Carry::kButtonA == HidNpadButton_A"), "button bits pinned");
    std::cout << "carry overlay routes through the shared rules ok\n";
}

// R12-001: the Gen V Boxes grid has the Gen I-IV keys through the same overlay.
void testGen5BoxesKeys() {
    const auto overlay = read("src/UI/ClassicPackedMoveOverlay.inc");
    const auto gen5 = read("src/UI/Gen5SharedReadOnlySurface.inc");
    check(has(gen5, "ClassicPackedMove::handleGen5BoxInput(screen,down,held,stickX,stickY,tap))return true;"),
          "the Gen V Boxes grid routes Y / carry keys to the shared overlay");
    const auto boxInput = between(overlay, "bool handleGen5BoxInput(", "\n}\n");
    check(has(boxInput, "return selectionInput(screen, state, down, held, stickX, stickY);") &&
              has(boxInput, "state.yPending = true;"),
          "Gen V Y tap / hold use the shared selection state machine");
    const auto selection = between(overlay, "bool selectionInput(", "\n}\n");
    check(has(selection, "if (isGen5(screen)) return beginGen5(screen, state, box, slot);") &&
              has(selection, "if (!(held & HidNpadButton_Y)) return beginSelectedGroup(screen, state);"),
          "a Gen V Y tap picks up, a Y hold release picks up the group");
    check(has(overlay, "trainer.sharedScreen().dropCarriedGroup(") &&
              has(read("include/Integration/Gen5/Gen5SharedScreenState.h"), "bool dropCarriedGroup("),
          "a Gen V group lands as one staged transaction");
    check(has(gen5, "spec.actions.push_back({\"Y\",occupied?\"Move / Hold Multi\":\"Hold Multi\"});") &&
              has(gen5, "PokeBank::UIModel::ReviewEntry::Screen::Boxes,5).glyph;"),
          "the Gen V Boxes footer names Y Move / Hold Multi and the + Review key");
    check(!has(gen5, "hints.push_back({\"Y\",\"Review\"});"), "the Gen V Boxes footer no longer says Y Review");
    std::cout << "Gen V Boxes: Y Move / Hold Multi, + Review, shared carry ok\n";
}

void testCloneIsTheSharedCarry() {
    const auto overlay = read("src/UI/ClassicPackedMoveOverlay.inc");
    check(has(overlay, "bool beginCloneFromAction(TrainerViewScreen& screen, int box, int slot, bool party)"),
          "one Clone entry for Gen I-V");
    // A Clone carry is deferred: it never takes the source out of the staged image.
    const auto begin = between(overlay, "bool beginClone(", "\n}\n");
    check(!begin.empty() && !has(begin, "beginPackedMove") && !has(begin, "beginSparseMove") &&
              !has(begin, "beginPackedGroupMove"),
          "Clone pickup never moves the source out of the staged image");
    const auto placeClone = between(overlay, "bool placeClone(", "\nbool place(");
    check(!placeClone.empty(), "placeClone found");
    check(has(placeClone, "Carry::decideCloneDrop("), "the drop asks the shared destination rule");
    check(has(placeClone, "Carry::dropStatus(") && has(placeClone, "return true;"), "a refusal keeps the carry");
    check(has(placeClone, "stagedBytesMatch(") && has(placeClone, "Carry::kCloneSourceChangedStatus"),
          "the drop re-verifies the staged save it picked up");
    for (const char* reopen : {"pokemonActions = true", "state.actions = true", "Surface::Actions", "UX2Mode::Actions"})
        check(!has(placeClone, reopen), std::string("no Actions menu after the clone: ") + reopen);
    for (const char* backend : {"stageClone(", "stageCloneBoxPokemon(", "dropCarriedClone("})
        check(has(placeClone, backend), std::string("staged through the generation's backend: ") + backend);
    check(has(overlay, "state.intent == Carry::Intent::Clone"), "Move and Clone share one carry state");

    // Gen I: both Actions sheets start the carry (ux2OpenClone), close the sheet first.
    const auto gen1 = read("src/UI/Gen1PokemonEditorOverlayUXCleanup2.inc");
    const auto open = between(gen1, "void ux2OpenClone(TrainerViewScreen& screen) {", "\n}\n");
    check(has(open, "ux2Close(state);") && has(open, "ClassicPackedMove::beginCloneFromAction(screen, box, slot"),
          "Gen I Clone closes Actions and starts the shared carry");
    check(open.find("ux2Close(state);") < open.find("beginCloneFromAction"), "Gen I closes the sheet first");
    check(!has(gen1, "state.mode = UX2Mode::CloneDestination"), "the Gen I Clone destination modal is gone");
    check(has(read("src/UI/ClassicReleaseActionFix.inc"), "case Action::Clone: ux2OpenClone(screen); break;") &&
              has(gen1, "case UX::Action::Clone: ux2OpenClone(screen); break;"),
          "both Gen I sheets use ux2OpenClone");

    // Gen II: both entry points (live Actions and the shared surface).
    const auto release = read("src/UI/ClassicReleaseActionFix.inc");
    const auto gen2Release = release.substr(release.find("namespace UI::Gen2PokemonEditor {"));
    const auto surface = read("src/UI/Gen2SharedPokemonSurface.inc");
    for (const auto& clone : {between(gen2Release, "case Action::Clone: {", "case Action::Remove:"),
                              between(surface, "case SharedEditor::Action::Clone: {",
                                      "case SharedEditor::Action::LegalityProvenance: {")}) {
        check(has(clone, "legacy.active = false;") && has(clone, "legacy.pokemonActions = false;"),
              "Gen II Clone closes the Actions sheet");
        check(has(clone, "ClassicPackedMove::beginCloneFromAction("), "Gen II Clone starts the shared carry");
        check(!has(clone, "cloneActive"), "no Gen II Clone dialog");
    }
    check(!has(surface, "handleSharedClone") && !has(surface, "drawSharedCloneSurface") &&
              !has(surface, "bool cloneActive"),
          "the Gen II Clone dialog and its Actions reopen are gone");
    check(!has(read("src/UI/Gen2SharedSurfaceParity.inc"), "drawParityGen2Clone"), "no Gen II parity Clone dialog");
    check(!has(read("src/UI/TrainerViewScreenGSCOverlay.inc"), "Clone to Gen II Box"), "no legacy Gen II keypad clone");

    // Gen III / IV: no keypads; the sheet closes; the shared carry starts.
    const auto gen3 = between(read("src/UI/Gen3SharedPokemonSurface.inc"), "case Shared::Action::Clone: {",
                              "case Shared::Action::Close:");
    const auto gen4 = between(read("src/UI/Gen4SharedPokemonSurface.inc"), "case MenuAction::Clone: {",
                              "case MenuAction::Release:");
    for (const auto& clone : {gen3, gen4}) {
        check(!clone.empty(), "Gen III / IV Clone case found");
        check(!has(clone, "promptNumber"), "Gen III / IV Clone opens no keypad");
        check(has(clone, "state.actions = false;") && has(clone, "ClassicPackedMove::beginCloneFromAction("),
              "Gen III / IV Clone closes Actions and starts the shared carry");
    }
    // Gen V: Clone uses the carry too; the keypad function is gone.
    const auto gen5 = read("src/UI/Gen5SharedReadOnlySurface.inc");
    check(!has(gen5, "stageCloneOrMoveByKeypad") && !has(gen5, "Clone to Gen V Box"), "no Gen V keypad");
    check(has(gen5, "state.beginCloneCarry(staged,error)") && has(gen5, "ClassicPackedMove::beginCloneFromAction("),
          "Gen V Clone starts the shared carry");
    for (const char* path : {"src/UI/Gen3SharedPokemonSurface.inc", "src/UI/Gen4SharedPokemonSurface.inc",
                             "src/UI/Gen5SharedReadOnlySurface.inc", "src/UI/TrainerViewScreenGSCOverlay.inc"}) {
        const auto text = read(path);
        check(!has(text, "promptNumber(\"Clone") && !has(text, "\"Clone to Gen") && !has(text, "\"Clone to Slot\""),
              std::string("no Clone keypad prompt left in ") + path);
    }
    std::cout << "every generation's Clone is the shared visual carry ok\n";
}

void testHelp() {
    for (int gen = 1; gen <= 5; ++gen) {
        const auto lines = Help::lines(Help::Page::Pokemon, gen);
        const auto contains = [&](const std::string& prefix) {
            return std::any_of(lines.begin(), lines.end(), [&](const std::string& l) { return l.rfind(prefix, 0) == 0; });
        };
        check(contains("A   While moving or cloning: place it here     B   While carrying: cancel"),
              "Gen " + std::to_string(gen) + ": Help names A as the place key");
        check(!contains("Y   While moving"), "Gen " + std::to_string(gen) + ": Help never says Y places");
        // R12-001: every generation's Boxes pick up with Y and open Review with + (Gen V included).
        check(contains("Y   Tap: pick up to move     Y   Hold: multi-select"),
              "Gen " + std::to_string(gen) + ": the Y pickup row matches the grid");
        check(contains(Help::kPlusReview) && !contains(Help::kYReview),
              "Gen " + std::to_string(gen) + ": Boxes Help names + Review");
        check(lines.size() <= 10, "Gen " + std::to_string(gen) + ": the Pokemon Help fits 10 rows");
        check(std::any_of(lines.begin(), lines.end(), [](const std::string& l) { return has(l, "Clone"); }),
              "Gen " + std::to_string(gen) + ": Help names Clone");
    }
    std::cout << "Help names A to place, B to cancel ok\n";
}

} // namespace

// Round 13 owner decision 4: touch drag multi-select with controller parity, ONE implementation for
// Gen I-V (behaviour: tests/test_ui_touch_multiselect_host.cpp; this pins the rule and the wiring).
void testTouchParity() {
    using Tap = Carry::Tap;
    // Owner Nov 2: one tap on a cell places there (the controller's cursor + A); outside is B.
    check(Carry::carryTap(7, 30, false) == Tap::Place, "a tap on a cell is cursor there + A");
    check(Carry::carryTap(0, 30, true) == Tap::Place, "a cell tap is never a cancel");
    check(Carry::carryTap(-1, 30, true) == Tap::Cancel, "a tap outside every target is B");
    check(Carry::carryTap(-1, 30, false) == Tap::None, "a tap on another target keeps its meaning");
    check(Carry::carryTap(1000, 30, false) == Tap::None && Carry::carryTap(1001, 30, false) == Tap::None &&
              Carry::carryTap(2000, 30, false) == Tap::None,
          "box arrows and the summary panel are not cells");
    check(Carry::carryTap(25, 20, false) == Tap::None, "a cell past the box capacity is not a cell");
    const auto overlay = read("src/UI/ClassicPackedMoveOverlay.inc");
    const auto gen5 = read("src/UI/Gen5SharedReadOnlySurface.inc");
    const auto gen5Touch = between(overlay, "bool handleGen5BoxTouch(", "\n}\n");
    const auto classic = between(overlay, "bool handleInput(TrainerViewScreen& screen, uint64_t down", "\n}\n");
    check(has(gen5Touch, "touchSelectionInput(screen, state, down, touch, gen5TouchGridOwnsInput(screen))") &&
              has(classic, "touchSelectionInput(screen, state, down, touch, gridOwnsInput(screen))"),
          "Gen V and Gen I-IV share one touch rectangle selection");
    check(has(gen5Touch, "carryTouch(screen, state, touch, gen5TouchGridOwnsInput(screen), slot)") &&
              has(classic, "carryTouch(screen, state, touch, boxesGridOwnsInput(screen), tappedCell)"),
          "Gen V and Gen I-IV share one carry tap rule");
    for (const auto* site : {&gen5Touch, &classic}) {
        check(has(*site, "case Carry::Tap::Place: return tapPlace(screen, state, ") &&
                  has(*site, "case Carry::Tap::Cancel: return cancel(screen, state);"),
              "a carry tap runs the same place() / cancel() as A / B");
        check(!has(*site, "Tap::Focus"), "no focus-only carry tap any more");
        check(!has(*site, "postStatus"), "no new toast from touch");
    }
    const auto selection = between(overlay, "                         bool owns) {", "\n}\n");
    check(has(selection, "return beginSelectedGroup(screen, state);") && !has(selection, "postStatus"),
          "lifting the finger picks up through the Y-release path, silently");
    check(has(overlay, "bool gen5TouchGridOwnsInput(") &&
              has(between(overlay, "bool gen5TouchGridOwnsInput(", "\n}\n"), "!screen.actionSheet.isOpen()"),
          "Gen V touch is refused while a modal is up");
    check(has(gen5, "ClassicPackedMove::handleGen5BoxTouch(screen,down,touch))return true;") &&
              gen5.find("ClassicPackedMove::handleGen5BoxTouch(") < gen5.find("ClassicPackedMove::handleGen5BoxInput("),
          "the Gen V Boxes grid gives the finger to the shared overlay first");
    const auto tapRule = between(overlay, "Carry::Tap carryTouch(", "\n}\n");
    check(has(tapRule, "state.carryTapArmed = owns;") && has(tapRule, "if (!armed) return Carry::Tap::None;") &&
              has(tapRule, "SharedPopupDismiss::kFooterTop"),
          "a carry tap needs a touch-down the grid owned (no modal-close tap), above the footer");
    check(has(between(overlay, "bool tapPlace(", "\n}\n"), "settlePackedFocus(screen, state);\n    return place(screen, state);"),
          "a tapped cell places exactly as cursor + A");
    check(has(between(overlay, "bool gen5BoxLayerBusy(", "\n}\n"), "state.touchSelecting"),
          "Gen V draws the touch rectangle");
    std::cout << "touch drag multi-select: one rule and one selection for Gen I-V ok\n";
}

int main() {
    testKeys();
    testRoutineFeedback();
    testPackedFocusAndGroups();
    testCloneDestinations();
    testOverlayUsesTheRules();
    testGen5BoxesKeys();
    testTouchParity();
    testCloneIsTheSharedCarry();
    testHelp();
    if (failures) {
        std::cerr << failures << " check(s) failed\n";
        return 1;
    }
    std::cout << "test_classic_carry_rules: all passed\n";
    return 0;
}
