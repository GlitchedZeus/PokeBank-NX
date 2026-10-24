#pragma once

#include "UI/SharedEditorPresentation.h"

#include <array>
#include <cstddef>
#include <cstdint>

// Who owns a press in the Gen II Create/Edit workspace, what A/B/X/Y do there, and the footer
// that says so. The live input handler (handleUnifiedWorkspaceInput) dispatches on topLayer() and
// the live footers draw footer(), so a hint can never advertise a button that does something
// else -- the hardware failure where the Create exit dialog ("B Back, Y Discard, A Keep") sat
// above the workspace footer ("A Edit, Y Random DVs, X Add, B Exit"), whose glyphs stayed tappable.
namespace PokeBank::UIModel::Gen2WorkspaceLayers {

// Highest first: the order handleUnifiedWorkspaceInput checks them. Exactly one layer owns a frame.
enum class Layer : uint8_t {
    Picker,            // Species / Move / Crystal Location list (handled by the picker overlay)
    HeldItemPicker,
    ExitConfirm,       // "New Pokémon" / "Unsaved changes" dialog opened by B
    ProgressionEditor, // Level / Experience
    MoveEditor,        // Move N contextual editor
    FieldSelector,     // Pokérus / Crystal Met Time / Crystal OT Gender
    Workspace,
};

struct Open {
    bool picker = false;
    bool heldItemPicker = false;
    bool exitConfirm = false;
    bool progressionEditor = false;
    bool moveEditor = false;
    bool fieldSelector = false;
};

constexpr Layer topLayer(const Open& open) noexcept {
    if (open.picker) return Layer::Picker;
    if (open.heldItemPicker) return Layer::HeldItemPicker;
    if (open.exitConfirm) return Layer::ExitConfirm;
    if (open.progressionEditor) return Layer::ProgressionEditor;
    if (open.moveEditor) return Layer::MoveEditor;
    if (open.fieldSelector) return Layer::FieldSelector;
    return Layer::Workspace;
}

enum class Session : uint8_t { Create, Edit };
enum class Button : uint8_t { A, B, X, Y };

enum class Action : uint8_t {
    None,            // inert in this layer
    EditField,       // Workspace A: open the focused field's editor/picker/selector
    AskExit,         // Workspace B: open the exit dialog (never leaves directly)
    StageAdd,        // Create: X in the workspace, A in the exit dialog
    StageKeep,       // Edit: X in the workspace, A in the exit dialog
    Reroll,          // Workspace Y: re-roll the draft's DVs now (no prompt; Gen12DVReroll.h)
    Discard,         // Y in the exit dialog
    ContinueEditing, // B in the exit dialog
    Select,          // A in a list/selector/child editor
    Cancel,          // B in a list/selector/child editor/re-roll prompt
};

constexpr Action action(Layer layer, Session session, Button button) noexcept {
    const bool create = session == Session::Create;
    switch (layer) {
        case Layer::Workspace:
            switch (button) {
                case Button::A: return Action::EditField;
                case Button::B: return Action::AskExit;
                case Button::X: return create ? Action::StageAdd : Action::StageKeep;
                case Button::Y: return Action::Reroll;
            }
            break;
        case Layer::ExitConfirm:
            switch (button) {
                case Button::A: return create ? Action::StageAdd : Action::StageKeep;
                case Button::B: return Action::ContinueEditing;
                case Button::Y: return Action::Discard;
                case Button::X: return Action::None;
            }
            break;
        case Layer::Picker:
        case Layer::HeldItemPicker:
        case Layer::ProgressionEditor:
        case Layer::MoveEditor:
        case Layer::FieldSelector:
            if (button == Button::A) return Action::Select;
            if (button == Button::B) return Action::Cancel;
            return Action::None; // Y in the Species picker toggles Normal/Shiny; that footer is the picker's own
    }
    return Action::None;
}

// The exit dialog acts on exactly one button per frame: A (Keep/Save), else Y (Discard), else B
// (Back). A failed Keep leaves the dialog open; it never falls through to Discard or Back.
enum class ExitChoice : uint8_t { None, Keep, Discard, Continue };

constexpr ExitChoice exitConfirmStep(bool a, bool b, bool y) noexcept {
    if (a) return ExitChoice::Keep;
    if (y) return ExitChoice::Discard;
    if (b) return ExitChoice::Continue;
    return ExitChoice::None;
}

struct Hint {
    const char* glyph;
    const char* label;
};

struct Footer {
    std::array<Hint, 6> hints{};
    std::size_t count = 0;
};

namespace detail {
constexpr void push(Footer& footer, const char* glyph, const char* label) noexcept {
    if (footer.count < footer.hints.size()) footer.hints[footer.count++] = {glyph, label};
}
} // namespace detail

// The footer for the layers whose footer the workspace itself draws. The pickers, the
// Held Item grid, the Move editor and the Level/Experience editor draw their own footer over it.
constexpr Footer footer(Layer layer, Session session) noexcept {
    const bool create = session == Session::Create;
    Footer f;
    switch (layer) {
        case Layer::ExitConfirm:
            // The shared confirm footer: B Back, Y Discard, A Keep (Create) / Save (Edit).
            for (const auto& hint : SharedEditorPresentation::confirmFooter(
                     "A", create ? "Keep" : "Save", "Y", "Discard"))
                detail::push(f, hint.glyph, hint.label);
            break;
        case Layer::FieldSelector: {
            // The shared picker footer (SharedEditorPresentation::pickerFooter): Browse, A Select,
            // B Cancel; a short chooser has no paging.
            for (const auto& hint : SharedEditorPresentation::pickerFooter({true, false}))
                detail::push(f, hint.glyph, hint.label);
            break;
        }
        default:
            detail::push(f, "D-pad/Stick", "Navigate");
            detail::push(f, "L/R", "Panel");
            detail::push(f, "A", "Edit");
            detail::push(f, "Y", "Random DVs");
            detail::push(f, "X", create ? "Add" : "Keep");
            detail::push(f, "B", "Exit");
            break;
    }
    return f;
}

} // namespace PokeBank::UIModel::Gen2WorkspaceLayers
