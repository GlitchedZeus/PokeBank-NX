// Gen II Create/Edit workspace: every A/B/X/Y press is owned by exactly one layer and does what
// that layer's footer says (Gen2WorkspaceLayers.h). Hardware report on a0633b6d: in Create, B
// opened "New Pokémon" (B Back, Y Discard, A Keep) while the workspace footer underneath still
// read "A Edit, Y Random DVs, X Add, B Exit" -- and those footer glyphs stayed tappable, so
// "Y Random DVs" discarded the draft and "A Edit" staged it.
//
// The model is driven with synthetic presses through the real selector and re-roll models, and
// the live handler / footers / exit dialog are checked to route through it.
#include "UI/Gen2WorkspaceLayers.h"
#include "UI/Gen2FieldSelector.h"
#include "UI/Gen12RerollDVsConfirm.h"

#include <cassert>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

namespace {

namespace L = PokeBank::UIModel::Gen2WorkspaceLayers;
namespace Selector = PokeBank::UIModel::Gen2FieldSelector;
namespace Reroll = PokeBank::UIModel::Gen12RerollDVs;

constexpr L::Button kButtons[] = {L::Button::A, L::Button::B, L::Button::X, L::Button::Y};
constexpr L::Session kSessions[] = {L::Session::Create, L::Session::Edit};

const char* glyph(L::Button b) {
    switch (b) {
        case L::Button::A: return "A";
        case L::Button::B: return "B";
        case L::Button::X: return "X";
        case L::Button::Y: return "Y";
    }
    return "?";
}

const char* hinted(const L::Footer& f, L::Button b) {
    for (std::size_t i = 0; i < f.count; ++i)
        if (std::strcmp(f.hints[i].glyph, glyph(b)) == 0) return f.hints[i].label;
    return nullptr;
}

bool footerSays(const L::Footer& f, const char* label) {
    for (std::size_t i = 0; i < f.count; ++i)
        if (std::strcmp(f.hints[i].label, label) == 0) return true;
    return false;
}

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
    if (text.find(needle) != std::string::npos) std::cerr << "Unexpected: " << needle << '\n';
    assert(text.find(needle) == std::string::npos);
}

void everyOverlapHasOneOwner() {
    // All 128 combinations of open layers: the owner is the highest open one, never two.
    for (int bits = 0; bits < 128; ++bits) {
        L::Open open;
        open.picker = bits & 1;
        open.heldItemPicker = bits & 2;
        open.exitConfirm = bits & 4;
        open.progressionEditor = bits & 8;
        open.moveEditor = bits & 16;
        open.fieldSelector = bits & 32;
        open.rerollConfirm = bits & 64;
        const auto owner = L::topLayer(open);
        int expected = 0;
        while (expected < 7 && !(bits & (1 << expected))) ++expected;
        assert(static_cast<int>(owner) == expected); // Layer::Workspace == 7 when nothing is open
    }
}

void footersMatchActions() {
    for (const auto session : kSessions) {
        const bool create = session == L::Session::Create;
        for (const auto layer : {L::Layer::Workspace, L::Layer::ExitConfirm, L::Layer::FieldSelector,
                                 L::Layer::RerollConfirm}) {
            const auto f = L::footer(layer, session);
            for (const auto button : kButtons) {
                const char* label = hinted(f, button);
                const auto act = L::action(layer, session, button);
                // A hinted button always acts; an unhinted one is inert.
                if (label == nullptr) {
                    if (act != L::Action::None)
                        std::cerr << "Unhinted " << glyph(button) << " acts in layer "
                                  << static_cast<int>(layer) << '\n';
                    assert(act == L::Action::None);
                } else {
                    assert(act != L::Action::None);
                }
            }
        }

        // The exit dialog's footer is the dialog's: B Back, Y Discard, A Keep (Create) / Save (Edit).
        const auto exit = L::footer(L::Layer::ExitConfirm, session);
        assert(std::strcmp(hinted(exit, L::Button::B), "Back") == 0);
        assert(std::strcmp(hinted(exit, L::Button::Y), "Discard") == 0);
        assert(std::strcmp(hinted(exit, L::Button::A), create ? "Keep" : "Save") == 0);
        assert(hinted(exit, L::Button::X) == nullptr);
        assert(!footerSays(exit, "Random DVs") && !footerSays(exit, "Edit") &&
               !footerSays(exit, "Add") && !footerSays(exit, "Exit"));
        assert(L::action(L::Layer::ExitConfirm, session, L::Button::Y) == L::Action::Discard);
        assert(L::action(L::Layer::ExitConfirm, session, L::Button::B) == L::Action::ContinueEditing);
        assert(L::action(L::Layer::ExitConfirm, session, L::Button::A) ==
               (create ? L::Action::StageAdd : L::Action::StageKeep));

        // The workspace: B never leaves directly, X stages, Y asks before re-rolling.
        const auto ws = L::footer(L::Layer::Workspace, session);
        assert(std::strcmp(hinted(ws, L::Button::X), create ? "Add" : "Keep") == 0);
        assert(std::strcmp(hinted(ws, L::Button::B), "Exit") == 0);
        assert(L::action(L::Layer::Workspace, session, L::Button::B) == L::Action::AskExit);
        assert(L::action(L::Layer::Workspace, session, L::Button::Y) == L::Action::AskReroll);

        // The re-roll prompt names both confirming buttons (Gen12RerollDVs: A or Y re-rolls).
        const auto rr = L::footer(L::Layer::RerollConfirm, session);
        assert(hinted(rr, L::Button::A) && hinted(rr, L::Button::Y) && hinted(rr, L::Button::B));
    }
}

void exitDialogActsOncePerFrame() {
    // A failed Keep never falls through: A+Y and A+B are Keep only.
    assert(L::exitConfirmStep(true, false, false) == L::ExitChoice::Keep);
    assert(L::exitConfirmStep(true, false, true) == L::ExitChoice::Keep);
    assert(L::exitConfirmStep(true, true, true) == L::ExitChoice::Keep);
    assert(L::exitConfirmStep(false, true, true) == L::ExitChoice::Discard);
    assert(L::exitConfirmStep(false, true, false) == L::ExitChoice::Continue);
    assert(L::exitConfirmStep(false, false, false) == L::ExitChoice::None);
}

// A small Create-session driver: each frame the top layer consumes the press using the real
// selector and re-roll models, and the step records which layer acted and what it did.
struct Driver {
    L::Open open;
    Selector::Selector selector;
    Reroll::State reroll;
    int staged = 0, discarded = 0, rerolled = 0, applied = 0;
    bool sessionOpen = true;

    struct Result { L::Layer layer; L::Action action; };

    Result press(L::Button b) {
        open.fieldSelector = selector.open();
        open.rerollConfirm = reroll.confirming;
        const auto layer = L::topLayer(open);
        const auto expected = L::action(layer, L::Session::Create, b);
        const bool a = b == L::Button::A, bb = b == L::Button::B, y = b == L::Button::Y;
        switch (layer) {
            case L::Layer::ExitConfirm:
                switch (L::exitConfirmStep(a, bb, y)) {
                    case L::ExitChoice::Keep: ++staged; open.exitConfirm = false; sessionOpen = false; break;
                    case L::ExitChoice::Discard: ++discarded; open.exitConfirm = false; sessionOpen = false; break;
                    case L::ExitChoice::Continue: open.exitConfirm = false; break;
                    case L::ExitChoice::None: break;
                }
                break;
            case L::Layer::FieldSelector: {
                const auto r = Selector::step(selector, false, false, a, bb);
                if (r.outcome == Selector::Outcome::Apply) ++applied;
                break;
            }
            case L::Layer::RerollConfirm:
            case L::Layer::Workspace: {
                const auto r = Reroll::step(reroll, y, a, bb);
                if (r == Reroll::Outcome::Reroll) ++rerolled;
                if (r != Reroll::Outcome::NotHandled) break;
                if (bb) open.exitConfirm = true;
                if (b == L::Button::X) { ++staged; sessionOpen = false; }
                if (a) selector = Selector::pokerus(0); // A on the focused Pokérus row
                break;
            }
            default: break;
        }
        return {layer, expected};
    }
};

void syntheticCreateSession() {
    Driver d;
    // B in the workspace asks; B in the dialog keeps editing (nothing staged or discarded).
    auto r = d.press(L::Button::B);
    assert(r.layer == L::Layer::Workspace && r.action == L::Action::AskExit && d.open.exitConfirm);
    r = d.press(L::Button::B);
    assert(r.layer == L::Layer::ExitConfirm && r.action == L::Action::ContinueEditing);
    assert(!d.open.exitConfirm && d.sessionOpen && d.staged == 0 && d.discarded == 0);

    // X while the dialog is open is inert (the footer no longer offers "X Add" there).
    d.press(L::Button::B);
    r = d.press(L::Button::X);
    assert(r.layer == L::Layer::ExitConfirm && r.action == L::Action::None);
    assert(d.open.exitConfirm && d.staged == 0);
    // Y there discards -- and the footer says Discard, not Random DVs.
    r = d.press(L::Button::Y);
    assert(r.action == L::Action::Discard && d.discarded == 1 && !d.sessionOpen && d.rerolled == 0);

    // Fresh session: A opens the Pokérus selector; B there cancels and is not also Back/Exit.
    Driver e;
    r = e.press(L::Button::A);
    assert(r.layer == L::Layer::Workspace && r.action == L::Action::EditField && e.selector.open());
    r = e.press(L::Button::B);
    assert(r.layer == L::Layer::FieldSelector && r.action == L::Action::Cancel);
    assert(!e.selector.open() && !e.open.exitConfirm && e.applied == 0);
    // A then A: open, then apply -- the applying A does not reopen anything.
    e.press(L::Button::A);
    r = e.press(L::Button::A);
    assert(r.layer == L::Layer::FieldSelector && r.action == L::Action::Select);
    assert(e.applied == 1 && !e.selector.open());

    // Y asks; B cancels the prompt without opening the exit dialog; A in the prompt re-rolls only.
    r = e.press(L::Button::Y);
    assert(r.action == L::Action::AskReroll && e.reroll.confirming);
    r = e.press(L::Button::B);
    assert(r.layer == L::Layer::RerollConfirm && r.action == L::Action::Cancel);
    assert(!e.reroll.confirming && !e.open.exitConfirm);
    e.press(L::Button::Y);
    r = e.press(L::Button::A);
    assert(r.layer == L::Layer::RerollConfirm && r.action == L::Action::Reroll);
    assert(e.rerolled == 1 && !e.selector.open());

    // X in the workspace stages exactly once.
    r = e.press(L::Button::X);
    assert(r.action == L::Action::StageAdd && e.staged == 1 && !e.sessionOpen);
}

void liveRoutesThroughTheModel() {
    const auto workspace = read("src/UI/Gen2UnifiedPokemonWorkspace.inc");
    const auto input = between(workspace, "bool handleUnifiedWorkspaceInput(", "Color unifiedStatusColor(");
    const auto edit = input.substr(input.find("auto& unified = unifiedStateFor(screen);"));
    // After the View branch, the first decision is the single owner.
    const auto dispatch = edit.find("const auto layer = workspaceTopLayer(screen);");
    assert(dispatch != std::string::npos);
    const auto afterView = edit.find("if (state.mode == Mode::View)");
    const auto viewEnd = edit.find("return true; // A/Y/X never mutate a View session.");
    assert(afterView < viewEnd && viewEnd < dispatch);
    for (const char* owner : {"Layers::Layer::Picker", "Layers::Layer::HeldItemPicker",
                              "Layers::Layer::ExitConfirm", "Layers::Layer::ProgressionEditor",
                              "Layers::Layer::MoveEditor", "Layers::Layer::FieldSelector"}) {
        const auto at = edit.find(owner);
        assert(at != std::string::npos && at > dispatch);
        // Owners are dispatched before any workspace A/B/X handling.
        assert(at < edit.find("if ((down & HidNpadButton_X) && state.mode == Mode::Create)"));
    }
    lacks(edit, "if (state.confirmExit) return handleConfirm");

    // The exit dialog acts through exitConfirmStep only.
    const auto foundation = read("src/UI/Gen2PokemonEditorFoundation.inc");
    const auto confirm = between(foundation, "bool handleConfirm(", "} // namespace");
    has(confirm, "Layers::exitConfirmStep(");
    assert(confirm.find("if (down & HidNpadButton_") == std::string::npos);

    // Both live footers come from the model; no hard-coded workspace footer remains.
    has(workspace, "void drawWorkspaceFooter(");
    has(between(workspace, "void drawUnifiedWorkspace(", "bool unifiedGen2SurfaceOwnsFrame("),
        "drawWorkspaceFooter(fb, screen);");
    const auto hardware = read("src/UI/Gen2HardwareFinalFix.inc");
    const auto fullscreen = between(hardware, "void drawFullscreenGen2Workspace(", "void drawFullscreenGen2Active(");
    has(fullscreen, "drawWorkspaceFooter(fb, screen);");
    lacks(fullscreen, "\"Random DVs\"");
    lacks(between(workspace, "void drawUnifiedWorkspace(", "bool unifiedGen2SurfaceOwnsFrame("), "\"Random DVs\"");

    // A mode change closes the Move editor too, so no stale child layer outlives its session.
    const auto reset = between(workspace, "} else if (state.lastMode != mode) {", "state.crystal =");
    has(reset, "state.moveEditor = false;");
}

} // namespace

int main() {
    everyOverlapHasOneOwner();
    footersMatchActions();
    exitDialogActsOncePerFrame();
    syntheticCreateSession();
    liveRoutesThroughTheModel();
    std::cout << "Gen II workspace layers PASS\n";
    return 0;
}
