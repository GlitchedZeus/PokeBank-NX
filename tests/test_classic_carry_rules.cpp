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
// The end-to-end behaviour through the real TrainerViewScreen is tests/test_ui_clone_carry_host.cpp.
#include "UI/ClassicCarryRules.h"
#include "UI/ClassicHelpPresentation.h"

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
    check(Carry::carryKey(Carry::kButtonY) == Key::RefuseHint, "Y never places");
    check(Carry::carryKey(Carry::kButtonX) == Key::RefuseHint, "X is refused");
    check(Carry::carryKey(Carry::kButtonY | Carry::kButtonA) == Key::Place, "A with Y still places once");
    check(Carry::carryKey(Carry::kButtonPlus) == Key::Leave, "+ cancels and falls through");
    check(Carry::carryKey(1ull << 6) == Key::None && Carry::carryKey(1ull << 7) == Key::None, "L / R navigate");
    check(Carry::carryKey(1ull << 12) == Key::None, "D-pad navigates");
    for (const auto intent : {Carry::Intent::Move, Carry::Intent::Clone}) {
        const std::string hint = Carry::refuseHint(intent);
        check(has(hint, "A places") && has(hint, "B cancels"), "refusal hint names A and B: " + hint);
        const auto footer = Carry::carryFooter(intent, false);
        check(std::strcmp(footer.hints[0].glyph, "A") == 0 && std::strcmp(footer.hints[2].glyph, "B") == 0 &&
                  std::strcmp(footer.hints[2].label, "Cancel") == 0 && std::strcmp(footer.hints[1].glyph, "L/R") == 0,
              "carry footer: A place, L/R box, B cancel");
    }
    check(std::strcmp(Carry::carryFooter(Carry::Intent::Move, false).hints[0].label, "Place") == 0, "Move footer");
    check(std::strcmp(Carry::carryFooter(Carry::Intent::Move, true).hints[0].label, "Place Group") == 0, "group footer");
    check(std::strcmp(Carry::carryFooter(Carry::Intent::Clone, false).hints[0].label, "Place Clone") == 0, "Clone footer");
    std::cout << "carry keys: A places, B cancels, Y / X refused ok\n";
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
    const auto classic = between(overlay, "    if (state.active) {", "    if (state.selecting) {");
    for (const auto* site : {&gen5, &classic}) {
        check(!site->empty(), "carry dispatch site found");
        check(has(*site, "switch (Carry::carryKey(down))"), "dispatch routes through ClassicCarry::carryKey");
        check(has(*site, "case Carry::Key::Place: return place(screen, state);"), "A -> place()");
        check(has(*site, "case Carry::Key::Cancel: return cancel(screen, state);"), "B -> cancel()");
        check(has(*site, "Carry::refuseHint(state.intent)"), "Y / X -> the hint, consumed");
        check(!has(*site, "HidNpadButton_Y) return place("), "Y no longer places");
    }
    check(has(classic, "D-pad and L/R keep using the accepted box navigation path"), "navigation comment kept");
    // No "Y places" / "Place with Y" / "Y Drop" text remains in the carry.
    for (const char* stale : {"Y places", "Place with Y", "\"Drop\"", "\"Drop Group\"", "Y drops"})
        check(!has(overlay, stale), std::string("no stale Y-place text: ") + stale);
    check(has(overlay, "Carry::carryFooter(state.intent, state.visuals.size() > 1)"), "footer from the shared model");
    // The browse footer and the multi-select hold keep Y (pickup, unchanged).
    check(has(overlay, "{\"Y\", \"Move / Hold Multi\"}") && has(overlay, "{\"Y\", \"Hold / Release Pick Up\"}"),
          "Y stays the pickup key in the browse and multi-select footers");
    // The libnx bits the model restates.
    check(has(overlay, "static_assert(Carry::kButtonA == HidNpadButton_A"), "button bits pinned");
    std::cout << "carry overlay routes through the shared rules ok\n";
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
        // Gen I-IV Boxes pick up with Y; Gen V's Y is Review (Actions -> Move picks up there).
        check(contains("Y   Tap: pick up to move     Y   Hold: multi-select") == (gen != 5),
              "Gen " + std::to_string(gen) + ": the Y pickup row matches the grid");
        check(std::any_of(lines.begin(), lines.end(), [](const std::string& l) { return has(l, "Clone"); }),
              "Gen " + std::to_string(gen) + ": Help names Clone");
    }
    std::cout << "Help names A to place, B to cancel ok\n";
}

} // namespace

int main() {
    testKeys();
    testCloneDestinations();
    testOverlayUsesTheRules();
    testCloneIsTheSharedCarry();
    testHelp();
    if (failures) {
        std::cerr << failures << " check(s) failed\n";
        return 1;
    }
    std::cout << "test_classic_carry_rules: all passed\n";
    return 0;
}
