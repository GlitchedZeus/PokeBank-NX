#pragma once
// The ONE shared Pokemon editor workspace (DETAILS | STATS | MOVES), its value
// pickers and its contextual move editor. Extracted unchanged from the accepted
// Generation IV surface; every generation that uses it supplies only an adapter
// (rows, native values, capabilities and validation). Gen IV behaviour is the
// reference: Gen IV's adapter (Gen4SharedPokemonSurface.inc) supplies the exact
// strings and setters it always used.
//
// Adapter contract (all static, `A::`):
//   generation, kDetailRows, kMaxSpecies, Record           layout / preview type
//   editable passive creating confirmExit hasWorking        session state
//   preview previewFailed sessionBack continueEditing discard keep
//   detailLabel detailEditable detailValue editDetail passiveActivate
//   ivs evs stats setIv setEv pidRowEditable pidRowValue gender editPidRow
//   moves pp ppUps workingMove workingPP workingPPUps setMove setPP setPPUps
//   maxPP moveChoices moveStatus moveGame moveEditorPrompt
//   pickerLabel applyPicker speciesPreviewTypes speciesLabel
//   heldItemArtKey chrome portraitTypes dataRows + text constants (k*)
#include "UI/SharedPokemonEditorWorkspace.h"
#include "UI/Dialogs/DialogFrame.h"
#include "UI/ItemPickerArtwork.h"
#include "UI/ItemPickerSort.h"
#include "UI/MovePickerPresentation.h"
#include "UI/MovePickerRowUI.h"
#include "UI/NaturePickerLayout.h"
#include "UI/SharedHeldItemPicker.h"
#include "UI/SharedHeldItemPickerPresentation.h"
#include "UI/SharedPokemonEditorContract.h"
#include "UI/SharedEditorChrome.h"
#include "UI/SharedBoxedRows.h"
#include "UI/SharedEditorPresentation.h"
#include "UI/SharedMovesPanel.h"
#include "UI/SharedStatReroll.h"
#include "Utils/HelperUtilities.h"
#include "UI/SharedPokemonShell.h"
#include "UI/SharedSpeciesPicker.h"
#include "UI/TouchScroll.h"
#include "Utils/Keyboard.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace UI::SharedPokemonEditorSurface {

namespace Shared = PokeBank::UIModel::SharedPokemonEditor;
namespace Workspace = PokeBank::UIModel::SharedPokemonWorkspace;
namespace HeldItemGrid = PokeBank::UIModel::SharedHeldItemPicker;
namespace MoveUI = PokeBank::UIModel::MovePickerPresentation;
namespace Context = PokeBank::UIModel::ClassicGameContext;
namespace Presentation = PokeBank::UIModel::SharedEditorPresentation;

enum class PickerTarget : uint8_t {
    None, Species, HeldItem, Language, Ball, MetLocation, Pokerus, Form, Move, Nature, Ability
};

// Editor focus, picker and move-editor state shared by every generation.
struct EditorState {
    Shared::Focus focus{};
    int detailsFocus = 0;
    TouchScrollState detailsTouchScroll;
    TouchPickerViewport detailsViewport;
    SharedScrollbar::State detailsScrollbar;   // draggable thumb beside the Details rows

    bool speciesPreviewShiny = false;
    bool valuePicker = false;
    PickerTarget pickerTarget = PickerTarget::None;
    int pickerRow = 0;
    TouchScrollState pickerScroll;
    TouchPickerViewport pickerViewport;
    SharedScrollbar::State pickerScrollbar;    // draggable thumb on the value / move picker list
    SharedSpeciesPicker::Chooser speciesChooser; // R2-SEARCH: live species filter + key grid
    std::vector<uint16_t> pickerValues;
    std::vector<uint16_t> pickerNativeValues;
    ItemPickerSort::Mode pickerSort = ItemPickerSort::Mode::Native;

    bool moveEditor = false;
    int moveEditorSlot = 0;
    int moveEditorRow = 0;
    uint16_t moveBaseline = 0;
    uint8_t movePpBaseline = 0;
    uint8_t movePpUpsBaseline = 0;
};

// Touch ids are written literally below; they are the shared model's ids.
static_assert(Workspace::kPickerTouchBase == 6000 && Workspace::kDetailTouchBase == 41000 &&
              Workspace::kIvTouchBase == 41100 && Workspace::kEvTouchBase == 41120 &&
              Workspace::kPidTouchBase == 41200 && Workspace::kMoveTouchBase == 41300 &&
              Workspace::kMoveEditorTouchBase == 41400 && Workspace::kMoveEditorRows == 5);

struct DataRow { std::string label, value; };
struct ChromeInfo {
    uint16_t species = 0;
    std::string nickname;
    uint8_t level = 1;
    uint8_t gender = 2;
    bool shiny = false;
    Context::SurfaceMode mode = Context::SurfaceMode::ViewReadOnly;
    std::string context;
};

inline void resetWorkspaceFocus(EditorState& state) noexcept {
    state.focus = {Shared::Panel::Details, 0, 0};
    state.detailsFocus = 0;
    state.detailsTouchScroll.reset();
    state.detailsViewport.reset();
    state.valuePicker = false;
    state.pickerTarget = PickerTarget::None;
    state.pickerValues.clear();
    state.speciesPreviewShiny = false;
    state.moveEditor = false;
    state.moveEditorRow = 0;
}

inline bool editorModalOpen(const EditorState& state) noexcept {
    return state.valuePicker || state.moveEditor;
}

inline const char* genderText(uint8_t gender) noexcept {
    return gender == 0 ? "Male" : gender == 1 ? "Female" : "Genderless";
}

inline Color genderValueColor(uint8_t gender) {
    if (gender == 0)
        return g_themeMode == ThemeMode::Light ? Color(24,112,184) : Color(92,205,255);
    if (gender == 1) return Colors::Magenta;
    return Colors::Text;
}

inline void drawPanel(PKSEFramebuffer& fb, int x, int y, int w, int h, const char* title) {
    drawPanelSurface(fb, x, y, w, h, false, 14);
    fb.drawText(x + 14, y + 10, title, Colors::Accent, TextStyle::Caption);
}

inline void drawFocus(PKSEFramebuffer& fb, int x, int y, int w, int h, bool selected) {
    if (selected)
        fb.drawRoundedRect(x, y, w, h, 6, Colors::FocusBorder, 2);
}

inline std::string dexLabel(uint16_t value) {
    std::string number = std::to_string(value);
    while (number.size() < 3) number = "0" + number;
    return "#" + number + "  " + Names::getSpeciesName(value);
}

const char* pickerTitle(PickerTarget target) noexcept {
    switch (target) {
        case PickerTarget::Species: return "Choose Species";
        case PickerTarget::HeldItem: return "Choose Held Item";
        case PickerTarget::Language: return "Choose Language";
        case PickerTarget::Ball: return "Choose Ball";
        case PickerTarget::MetLocation: return "Choose Met Location";
        case PickerTarget::Pokerus: return "Choose Pokérus";
        case PickerTarget::Form: return "Choose Form";
        case PickerTarget::Move: return "Choose Move";
        case PickerTarget::Nature: return "Choose Nature";
        case PickerTarget::Ability: return "Choose Ability";
        default: return "Choose Value";
    }
}

// Open a value picker on the given native choices with the current value focused.
inline void openPicker(TrainerViewScreen& screen, EditorState& state, PickerTarget target,
                       std::vector<uint16_t> values, uint16_t current) {
    state.valuePicker = true;
    state.pickerScroll.reset();
    state.pickerViewport.reset();
    state.pickerTarget = target;
    state.pickerValues = std::move(values);
    state.pickerNativeValues = state.pickerValues;
    state.pickerSort = ItemPickerSort::Mode::Native;
    if (target == PickerTarget::Species) state.speciesChooser.reset();   // R2-SEARCH: full native list
    const auto it = std::find(state.pickerValues.begin(), state.pickerValues.end(), current);
    state.pickerRow = it == state.pickerValues.end() ? 0 :
        static_cast<int>(std::distance(state.pickerValues.begin(), it));
    screen.controllerNavigation.reset();
}

// The Met Location picker is wider so place names and met level fit on one row (Gen III too).
inline constexpr int kMetLocationPickerWidth = 900;
static_assert(PokeBank::UIModel::SharedMenuGeometry::rule(
    PokeBank::UIModel::SharedMenuGeometry::MenuType::LocationPicker).width == kMetLocationPickerWidth);

// Phase 2: ONE geometry for every editor value list (UI/SharedMenuGeometry.h). Ball, Met
// Location and the short choice lists (Language, Ability, Form, Gender, Pokerus) share their
// rows; draw and touch handling both read this layout, so they cannot drift apart.
inline PokeBank::UIModel::SharedMenuGeometry::Layout valueListLayout(PickerTarget target, int count) noexcept {
    namespace G = PokeBank::UIModel::SharedMenuGeometry;
    return G::layout(target == PickerTarget::Ball ? G::MenuType::BallPicker
                     : target == PickerTarget::MetLocation ? G::MenuType::LocationPicker
                                                           : G::MenuType::ChoicePicker, count);
}

inline bool compactPicker(PickerTarget target) noexcept {
    return target == PickerTarget::Language || target == PickerTarget::Ball ||
           target == PickerTarget::MetLocation || target == PickerTarget::Nature ||
           target == PickerTarget::Ability || target == PickerTarget::Pokerus ||
           target == PickerTarget::Form;
}

// R2-SEARCH: the species chooser filters the game's NATIVE species list (pickerValues) by the
// bare species name; the owner's choice stays pickerRow (an index into pickerValues).
inline bool speciesAvailableIn(const std::vector<uint16_t>& values, uint16_t species) {
    return std::find(values.begin(), values.end(), species) != values.end();
}
inline int speciesRowIn(const std::vector<uint16_t>& values, uint16_t species, int fallback) {
    const auto it = std::find(values.begin(), values.end(), species);
    return it == values.end() ? fallback : static_cast<int>(std::distance(values.begin(), it));
}
constexpr auto sharedSpeciesName = [](uint16_t species) { return std::string(Names::getSpeciesName(species)); };

template <class A>
bool handleValuePicker(TrainerViewScreen& screen, EditorState& state, u64 down, const TouchInput& touch) {
    if (!state.valuePicker) return false;
    const int count = static_cast<int>(state.pickerValues.size());
    const bool gridPicker = state.pickerTarget == PickerTarget::HeldItem;
    const int columns = gridPicker ? HeldItemGrid::columns : 1;
    const int compactCount = std::max(0, count);
    const bool nature = state.pickerTarget == PickerTarget::Nature;
    const auto natureLayout = PokeBank::UIModel::NaturePickerLayout::forChoices(compactCount);
    const bool compact = compactPicker(state.pickerTarget) && (A::kCompactForm || state.pickerTarget != PickerTarget::Form);
    const int visibleRows = gridPicker ? HeldItemGrid::rows
        : state.pickerTarget == PickerTarget::Move ? MoveUI::compactPickerLayout().visibleRows
        : nature ? natureLayout.visibleRows
        : compact ? std::min(8, compactCount) : 10;
    if (state.pickerTarget != PickerTarget::Species)
        state.pickerViewport.ensure(state.pickerRow, count, visibleRows, columns);
    const int maxFirstRow = TouchPickerViewport::maxFirst(count, visibleRows, columns);
    // Shared scrollbar first on the row lists that draw one: a held thumb moves the viewport,
    // never the selection, and owns the frame's touch. The Species modal and the held-item grid
    // publish no track, so a stale one from an earlier picker must not capture here.
    const bool pickerHasScrollbar = state.pickerTarget != PickerTarget::Species &&
                                    state.pickerTarget != PickerTarget::HeldItem;
    if (!pickerHasScrollbar) state.pickerScrollbar = {};
    const auto pickerBar = pickerHasScrollbar
        ? SharedScrollbar::handleTouch(state.pickerScrollbar, SharedScrollbar::sample(touch),
              SharedScrollbar::rows(maxFirstRow + visibleRows, visibleRows, state.pickerViewport.firstRow))
        : SharedScrollbar::TouchResult{};
    if (pickerBar.changed) {
        state.pickerViewport.firstRow = pickerBar.scroll;
        state.pickerScroll.stop();
    }
    // The Species modal owns its own gesture. Other pickers scroll only when overflowing.
    bool speciesTouchOwned = false;   // R2-SEARCH: key grid / scrollbar took this frame's touch
    if (pickerBar.consumed) {
        state.pickerScroll.stop();
    } else if (count > 0 && (state.pickerTarget == PickerTarget::Species || maxFirstRow > 0)) {
        if (state.pickerTarget == PickerTarget::Species) {
            const auto available = [&](uint16_t s) { return speciesAvailableIn(state.pickerValues, s); };
            uint16_t chosen = state.pickerValues[static_cast<size_t>(state.pickerRow)];
            const auto touched = SharedSpeciesPicker::handleTouch(
                state.speciesChooser, SharedSearchInput::sample(touch), chosen, A::kMaxSpecies,
                SharedSpeciesPicker::ModalX, SharedSpeciesPicker::ModalY, available, sharedSpeciesName);
            const PokeBank::UIModel::SharedPickerPaging::Keys gridKeys{
                (down & HidNpadButton_Up) != 0, (down & HidNpadButton_Down) != 0,
                (down & HidNpadButton_Left) != 0, (down & HidNpadButton_Right) != 0,
                (down & HidNpadButton_L) != 0, (down & HidNpadButton_R) != 0,
                (down & HidNpadButton_ZL) != 0, (down & HidNpadButton_ZR) != 0};
            const auto keyed = SharedSpeciesPicker::handleKeys(
                state.speciesChooser, gridKeys, (down & HidNpadButton_A) != 0, (down & HidNpadButton_B) != 0,
                (down & (HidNpadButton_X | HidNpadButton_Minus)) != 0, chosen, A::kMaxSpecies,
                available, sharedSpeciesName);
            state.pickerRow = speciesRowIn(state.pickerValues, chosen, state.pickerRow);
            speciesTouchOwned = touched.touch;
            if (touched.keys || keyed.keys)
                down &= ~(HidNpadButton_Up | HidNpadButton_Down | HidNpadButton_Left | HidNpadButton_Right |
                          HidNpadButton_L | HidNpadButton_R | HidNpadButton_ZL | HidNpadButton_ZR |
                          HidNpadButton_A | HidNpadButton_B);
            const int dy = touch.deltaY();
            if (!speciesTouchOwned && touch.justReleased() && touch.dragged() &&
                SharedSpeciesPicker::gestureStartedInList(touch.startX(), touch.startY()) &&
                std::abs(dy) >= 24 && std::abs(dy) > std::abs(touch.deltaX())) {
                const int steps = std::max(1, std::abs(dy) / 43);
                state.pickerRow = speciesRowIn(state.pickerValues,
                    SharedSpeciesPicker::neighbour(state.speciesChooser, chosen, dy < 0 ? steps : -steps),
                    state.pickerRow);
            }
        } else if (state.pickerTarget == PickerTarget::HeldItem) {
            constexpr int pickerW = HeldItemGrid::modalWidth;
            constexpr int pickerH = HeldItemGrid::modalHeight;
            constexpr int rowStep = HeldItemGrid::rowPitch;
            const int px = (1280 - pickerW) / 2;
            const int py = (720 - kNavBarH - pickerH) / 2;
            state.pickerScroll.updateVertical(
                touch, px + HeldItemGrid::gridInsetX, py + HeldItemGrid::gridInsetY - 3,
                pickerW - 2 * HeldItemGrid::gridInsetX,
                HeldItemGrid::rows * rowStep, rowStep,
                state.pickerViewport.firstRow, maxFirstRow + 1);
        } else if (state.pickerTarget == PickerTarget::Move) {
            constexpr auto pickerLayout = MoveUI::compactPickerLayout();
            const int px = (1280 - pickerLayout.width) / 2;
            const int py = (720 - kNavBarH - pickerLayout.height) / 2;
            state.pickerScroll.updateVertical(
                touch, px + 24, py + pickerLayout.rowStart,
                pickerLayout.width - 48,
                pickerLayout.visibleRows * pickerLayout.rowStep,
                pickerLayout.rowStep, state.pickerViewport.firstRow, maxFirstRow + 1);
        } else {
            const auto valueList = valueListLayout(state.pickerTarget, count);
            const int pickerW = nature ? natureLayout.width : compact ? valueList.frame.w : 680;
            const int pickerH = nature ? natureLayout.height : compact ? valueList.frame.h : 560;
            const int rowStep = nature ? natureLayout.rowStep : 42;
            const int px = (1280 - pickerW) / 2;
            const int py = (720 - kNavBarH - pickerH) / 2;
            state.pickerScroll.updateVertical(
                touch, px + 22, py + 86, pickerW - 44,
                visibleRows * rowStep, rowStep, state.pickerViewport.firstRow, maxFirstRow + 1);
        }
    }
    if (maxFirstRow == 0)
        state.pickerScroll.stop();
    // Keep semantic focus fixed during picker drags; commit only the
    // release-confirmed visible row or explicit controller activation.
    const int touchTap = screen.touchedButtonId(touch);
    if (!pickerBar.consumed && !speciesTouchOwned && touchTap >= 6000 && touchTap < 6000 + count) {
        state.pickerRow = touchTap - 6000;
        down |= HidNpadButton_A;
    }
    if (count <= 0) {
        state.valuePicker = false;
        return true;
    }
    if ((gridPicker || state.pickerTarget == PickerTarget::Move) && (down & HidNpadButton_X)) {
        state.pickerSort = ItemPickerSort::next(state.pickerSort);
        state.pickerRow = ItemPickerSort::reorder(
            state.pickerValues, state.pickerNativeValues,
            state.pickerSort, state.pickerRow,
            [&](uint16_t id) {
                if (state.pickerTarget == PickerTarget::Move)
                    return id == 0 ? std::string() : std::string(Names::getMoveName(id));
                return A::pickerLabel(screen, state, PickerTarget::HeldItem, id);
            });
        state.pickerScroll.stop();
        state.pickerViewport.reset();
        state.pickerViewport.reveal(state.pickerRow, count, visibleRows, columns);
        return true;
    }
    if (down & (HidNpadButton_Up | HidNpadButton_Down | HidNpadButton_Left | HidNpadButton_Right | HidNpadButton_L | HidNpadButton_R | HidNpadButton_ZL | HidNpadButton_ZR))
        state.pickerScroll.stop();
    if (state.pickerTarget == PickerTarget::Species) {
        // Shared paging over the filtered list (R2-SEARCH); Up/Down wrap inside the filter.
        const PokeBank::UIModel::SharedPickerPaging::Keys keys{
            (down & HidNpadButton_Up) != 0, (down & HidNpadButton_Down) != 0,
            (down & HidNpadButton_Left) != 0, (down & HidNpadButton_Right) != 0,
            (down & HidNpadButton_L) != 0, (down & HidNpadButton_R) != 0,
            (down & HidNpadButton_ZL) != 0, (down & HidNpadButton_ZR) != 0};
        if (keys.any())
            state.pickerRow = speciesRowIn(state.pickerValues,
                SharedSpeciesPicker::paged(state.speciesChooser,
                    state.pickerValues[static_cast<size_t>(state.pickerRow)], keys),
                state.pickerRow);
    } else if (state.pickerTarget == PickerTarget::HeldItem) {
        if (down & HidNpadButton_Up)
            state.pickerRow = HeldItemGrid::move(state.pickerRow, count, 0, -1);
        if (down & HidNpadButton_Down)
            state.pickerRow = HeldItemGrid::move(state.pickerRow, count, 0, 1);
        if (down & HidNpadButton_Left)
            state.pickerRow = HeldItemGrid::move(state.pickerRow, count, -1, 0);
        if (down & HidNpadButton_Right)
            state.pickerRow = HeldItemGrid::move(state.pickerRow, count, 1, 0);
        if (down & HidNpadButton_L)
            state.pickerRow = HeldItemGrid::move(state.pickerRow, count, 0, 0, -1);
        if (down & HidNpadButton_R)
            state.pickerRow = HeldItemGrid::move(state.pickerRow, count, 0, 0, 1);
    } else {
        if (down & HidNpadButton_Up)
            state.pickerRow = (state.pickerRow - 1 + count) % count;
        if (down & HidNpadButton_Down)
            state.pickerRow = (state.pickerRow + 1) % count;
        if (down & (HidNpadButton_Left | HidNpadButton_L))
            state.pickerRow = std::max(0, state.pickerRow - 10);
        if (down & (HidNpadButton_Right | HidNpadButton_R))
            state.pickerRow = std::min(count - 1, state.pickerRow + 10);
        if (down & HidNpadButton_ZL)
            state.pickerRow = std::max(0, state.pickerRow - 50);
        if (down & HidNpadButton_ZR)
            state.pickerRow = std::min(count - 1, state.pickerRow + 50);
    }
    if (state.pickerTarget != PickerTarget::Species &&
        (down & (HidNpadButton_Up | HidNpadButton_Down | HidNpadButton_Left |
                 HidNpadButton_Right | HidNpadButton_L | HidNpadButton_R |
                 HidNpadButton_ZL | HidNpadButton_ZR)))
        state.pickerViewport.reveal(state.pickerRow, count, visibleRows, columns);
    if (state.pickerTarget == PickerTarget::Species && (down & HidNpadButton_Y)) {
        state.speciesPreviewShiny = !state.speciesPreviewShiny;
        return true;
    }
    if (down & HidNpadButton_B) {
        state.pickerScroll.stop();
        state.valuePicker = false;
        state.pickerTarget = PickerTarget::None;
        screen.controllerNavigation.reset();
        return true;
    }
    if (!(down & HidNpadButton_A) || !A::hasWorking(screen, state)) return true;

    // Species uses its own modal gesture. In other pickers the highlighted controller
    // choice can be off-screen after a finger scroll; A reveals rather than accepting it unseen.
    if (state.pickerTarget != PickerTarget::Species &&
        !state.pickerViewport.containsSelection(
            state.pickerRow, count, visibleRows, columns)) {
        state.pickerScroll.stop();
        state.pickerViewport.reveal(state.pickerRow, count, visibleRows, columns);
        screen.postStatus("Focused choice brought into view; press A again or tap a visible row", 220);
        return true;
    }

    state.pickerScroll.stop();
    const uint16_t value = state.pickerValues[static_cast<size_t>(state.pickerRow)];
    // The adapter applies the native choice to the draft, or reports why not
    // and leaves the picker open on the same row.
    if (!A::applyPicker(screen, state, state.pickerTarget, value)) return true;
    state.valuePicker = false;
    state.pickerTarget = PickerTarget::None;
    screen.controllerNavigation.reset();
    return true;
}

template <class A>
void openMoveEditor(TrainerViewScreen& screen, EditorState& state, int slot) {
    if (!A::hasWorking(screen, state) || slot < 0 || slot >= 4) return;
    state.moveEditor = true;
    state.moveEditorSlot = slot;
    state.moveEditorRow = 0; // Shared move editor opens on the Move row; PP/PP Ups stay contextual.
    state.moveBaseline = A::workingMove(screen, state, slot);
    state.movePpBaseline = A::workingPP(screen, state, slot);
    state.movePpUpsBaseline = A::workingPPUps(screen, state, slot);
    screen.controllerNavigation.reset();
}

template <class A>
void cancelMoveEditor(TrainerViewScreen& screen, EditorState& state) {
    if (A::hasWorking(screen, state) && state.moveEditorSlot >= 0 && state.moveEditorSlot < 4) {
        const int slot = state.moveEditorSlot;
        A::setMove(screen, state, slot, state.moveBaseline);
        A::setPPUps(screen, state, slot, state.movePpUpsBaseline);
        A::setPP(screen, state, slot, state.movePpBaseline);
    }
    state.moveEditor = false;
    state.moveEditorRow = 0;
}

template <class A>
void openMovePicker(TrainerViewScreen& screen, EditorState& state) {
    if (!A::hasWorking(screen, state)) return;
    // Never list a move that does not exist in this generation (Empty stays the first choice).
    auto choices = A::moveChoices(screen, state);
    const auto game = A::moveGame(screen);
    choices.erase(std::remove_if(choices.begin(), choices.end(), [&](uint16_t move) {
        return move != 0 && !PokeBank::UIModel::SharedMoveFacts::moveExists(move, game);
    }), choices.end());
    openPicker(screen, state, PickerTarget::Move, std::move(choices),
               A::workingMove(screen, state, state.moveEditorSlot));
}

template <class A>
bool handleMoveEditor(TrainerViewScreen& screen, EditorState& state, u64 down, const TouchInput& touch) {
    if (!state.moveEditor || !A::hasWorking(screen, state)) return false;
    // Move edit rows are selectable only after a completed tap.
    const int moveTap = screen.touchedButtonId(touch);
    if (moveTap >= 41400 && moveTap < 41405) { state.moveEditorRow = moveTap - 41400; down |= HidNpadButton_A; }
    if (down & HidNpadButton_Up) state.moveEditorRow = Workspace::moveEditorStep(state.moveEditorRow, -1);
    if (down & HidNpadButton_Down) state.moveEditorRow = Workspace::moveEditorStep(state.moveEditorRow, 1);
    if (down & HidNpadButton_B) {
        cancelMoveEditor<A>(screen, state);
        screen.controllerNavigation.reset();
        return true;
    }
    if (!(down & HidNpadButton_A)) return true;

    const int slot = state.moveEditorSlot;
    const auto move = A::workingMove(screen, state, slot);
    if (state.moveEditorRow == 0) {
        openMovePicker<A>(screen, state);
    } else if (state.moveEditorRow == 1) {
        if (move == 0) {
            screen.postStatus("Choose a move before editing PP", 240);
        } else {
            const int maximum = A::maxPP(screen, move, A::workingPPUps(screen, state, slot));
            const auto r = Utils::promptNumber("Current PP",
                std::min<int>(A::workingPP(screen, state, slot), maximum), 0, maximum);
            if (r.accepted) A::setPP(screen, state, slot, static_cast<uint8_t>(r.value));
        }
    } else if (state.moveEditorRow == 2) {
        if (move == 0) {
            screen.postStatus("Empty move slots keep PP Ups at zero", 240);
        } else {
            const auto r = Utils::promptNumber(
                "PP Ups", A::workingPPUps(screen, state, slot), 0, 3);
            if (r.accepted) {
                A::setPPUps(screen, state, slot, static_cast<uint8_t>(r.value));
                const int maximum = A::maxPP(screen, move, static_cast<uint8_t>(r.value));
                if (A::workingPP(screen, state, slot) > maximum)
                    A::setPP(screen, state, slot, static_cast<uint8_t>(maximum));
            }
        }
    } else if (state.moveEditorRow == 3) {
        state.moveEditor = false;
        state.moveEditorRow = 0;
        screen.controllerNavigation.reset();
    } else {
        cancelMoveEditor<A>(screen, state);
        screen.controllerNavigation.reset();
    }
    return true;
}

template <class A>
bool editStatCell(TrainerViewScreen& screen, EditorState& state, const typename A::Record& p) {
    if (!A::hasWorking(screen, state)) return false;
    const auto activation = Workspace::activationFor(state.focus);
    if (activation.kind == Workspace::ActivationKind::IvCell) {
        const auto stat = static_cast<size_t>(activation.row);
        const auto r = Utils::promptNumber("IV", A::ivs(p)[stat], 0, 31);
        if (r.accepted) A::setIv(screen, state, stat, static_cast<uint8_t>(r.value));
        return true;
    }
    if (activation.kind == Workspace::ActivationKind::EvCell) {
        const auto stat = static_cast<size_t>(activation.row);
        const auto r = Utils::promptNumber("EV", A::evs(p)[stat], 0, 255);
        if (r.accepted) A::setEv(screen, state, stat, static_cast<uint8_t>(r.value));
        return true;
    }
    return A::editPidRow(screen, state, activation.row, p);
}

template <class A>
bool handleWorkspace(TrainerViewScreen& screen, EditorState& state, u64 down, const TouchInput& touch) {
    if (handleValuePicker<A>(screen, state, down, touch)) return true;
    if (handleMoveEditor<A>(screen, state, down, touch)) return true;
    const auto record = A::preview(screen, state);
    if (!record) {
        A::previewFailed(screen, state);
        return true;
    }

    if (A::confirmExit(screen, state)) {
        if (down & HidNpadButton_A) {
            A::keep(screen, state);
            return true;
        }
        if (down & HidNpadButton_Y) {
            A::discard(screen, state);
            return true;
        }
        if (down & HidNpadButton_B) {
            A::continueEditing(screen, state);
            return true;
        }
        return true;
    }

    constexpr int rows = A::kDetailRows;
    const bool passive = A::passive(screen, state);
    const bool editable = A::editable(screen, state);
    constexpr int visibleDetailRows = Workspace::kVisibleDetailRows;
    const SharedPokemonShell::Geometry detailGeometry(1280, 720, kNavBarH);
    state.detailsViewport.ensure(state.detailsFocus, rows, visibleDetailRows);
    // Shared scrollbar first: a held thumb moves the Details viewport (not the focused field)
    // and owns the frame's touch, so no row under the finger is dragged or tapped.
    const auto detailsBar = SharedScrollbar::handleTouch(state.detailsScrollbar, SharedScrollbar::sample(touch),
        SharedScrollbar::rows(rows, visibleDetailRows, state.detailsViewport.firstRow));
    if (detailsBar.changed) {
        state.detailsViewport.firstRow = detailsBar.scroll;
        state.detailsTouchScroll.stop();
    }
    if (detailsBar.consumed)
        state.detailsTouchScroll.stop();
    else
        state.detailsTouchScroll.updateVertical(
            touch, detailGeometry.leftX + 8, detailGeometry.y + 216,
            detailGeometry.leftW - 16, visibleDetailRows * 38, 38,
            state.detailsViewport.firstRow,
            TouchPickerViewport::maxFirst(rows, visibleDetailRows) + 1);
    if (down & (HidNpadButton_Up | HidNpadButton_Down |
                HidNpadButton_L | HidNpadButton_R))
        state.detailsTouchScroll.stop();

    const Workspace::Keys keys{
        (down & HidNpadButton_L) != 0, (down & HidNpadButton_R) != 0,
        (down & HidNpadButton_Up) != 0, (down & HidNpadButton_Down) != 0,
        (down & HidNpadButton_Left) != 0, (down & HidNpadButton_Right) != 0};
    const auto detailEditable = [&](int row) { return A::detailEditable(screen, state, row, *record); };
    const auto pidEditable = [&](int row) { return A::pidRowEditable(screen, state, row, *record); };
    state.focus = Workspace::navigate(A::generation, state.focus, keys, passive, editable,
                                      rows, detailEditable, pidEditable).focus;
    state.detailsFocus = Workspace::detailsScrollRow(state.focus, state.detailsFocus, rows);
    if (down & (HidNpadButton_Up | HidNpadButton_Down |
                HidNpadButton_L | HidNpadButton_R))
        state.detailsViewport.reveal(state.detailsFocus, rows, visibleDetailRows);

    // Touch-down is provisional: do not jump focus while a finger is still
    // dragging through DETAILS/STATS/MOVES. Only a completed tap can select.
    const int pressedRow = screen.touchedButtonId(touch);
    Shared::Focus tapped{};
    if (!detailsBar.consumed && Workspace::focusForTouch(pressedRow, rows, tapped)) {
        state.focus = tapped;
        if (tapped.panel == Shared::Panel::Details) state.detailsFocus = tapped.row;
        if (editable) down |= HidNpadButton_A;
        else if (passive && tapped.panel == Shared::Panel::Details)
            A::passiveActivate(screen, state, tapped.row);
    }
    if (touch.justReleased() && touch.dragged()) {
        const int dx = touch.deltaX(), dy = touch.deltaY();
        if (std::abs(dx) >= 96 && std::abs(dx) > std::abs(dy)) {
            state.focus = passive
                ? Shared::passiveViewSwitchPanel(A::generation, state.focus, dx < 0 ? 1 : -1)
                : Shared::switchPanel(A::generation, state.focus, dx < 0 ? 1 : -1);
            state.detailsFocus = Workspace::detailsScrollRow(state.focus, state.detailsFocus, rows);
        }
    }

    if (down & HidNpadButton_B) {
        A::sessionBack(screen, state);
        return true;
    }
    if ((down & HidNpadButton_X) && editable) {
        A::keep(screen, state);
        return true;
    }
    if ((down & HidNpadButton_Y) && editable) {
        // Y re-rolls IVs/EVs in the working draft at once: no prompt, a new result every press.
        // PK4/PK5 IVs and EVs are not PID-derived, so Shiny/Gender/Nature/Ability are kept.
        // EVs are cleared first so the adapter's 510-total check never sees a transient overflow.
        namespace Reroll = PokeBank::UIModel::SharedStatReroll;
        Reroll::Rng rng(Utils::rand32());
        const auto next = Reroll::reroll(rng, {A::ivs(*record), A::evs(*record)});
        for (size_t stat = 0; stat < 6; ++stat) A::setEv(screen, state, stat, 0);
        for (size_t stat = 0; stat < 6; ++stat) A::setIv(screen, state, stat, next.ivs[stat]);
        for (size_t stat = 0; stat < 6; ++stat) A::setEv(screen, state, stat, next.evs[stat]);
        screen.postStatus(Reroll::kRerollStatus, 200);
        return true;
    }
    if ((down & HidNpadButton_A) && passive &&
        state.focus.panel == Shared::Panel::Details)
        A::passiveActivate(screen, state, state.focus.row);
    if (!(down & HidNpadButton_A) || !editable) return true;
    if (state.focus.panel == Shared::Panel::Details &&
        !state.detailsViewport.containsSelection(state.focus.row, rows, visibleDetailRows)) {
        state.detailsTouchScroll.stop();
        state.detailsViewport.reveal(state.focus.row, rows, visibleDetailRows);
        screen.postStatus("Focused field brought into view; press A again", 200);
        return true;
    }

    const auto activation = Workspace::activationFor(state.focus);
    if (activation.kind == Workspace::ActivationKind::DetailField) {
        if (!A::editDetail(screen, state, activation.row, *record))
            screen.postStatus(A::kDetailReadOnly, 300);
    } else if (activation.kind == Workspace::ActivationKind::MoveRow) {
        openMoveEditor<A>(screen, state, activation.row);
    } else if (!editStatCell<A>(screen, state, *record)) {
        screen.postStatus(A::kValueReadOnly, 240);
    }
    return true;
}

template <class A>
void drawValuePicker(TrainerViewScreen& screen, PKSEFramebuffer& fb, EditorState& state) {
    if (!state.valuePicker || state.pickerValues.empty()) return;
    SharedEditorChrome::beginModal(screen.touchButtons);
    const bool heldItems = state.pickerTarget == PickerTarget::HeldItem;
    const bool movePicker = state.pickerTarget == PickerTarget::Move;
    const bool speciesPicker = state.pickerTarget == PickerTarget::Species;
    constexpr auto moveLayout = MoveUI::compactPickerLayout();
    const int totalChoices=static_cast<int>(state.pickerValues.size());
    const bool compactChoices=compactPicker(state.pickerTarget) &&
        (A::kCompactForm || state.pickerTarget != PickerTarget::Form);
    const bool naturePicker=state.pickerTarget==PickerTarget::Nature;
    const auto natureLayout=PokeBank::UIModel::NaturePickerLayout::forChoices(totalChoices);
    const int visibleChoices=naturePicker ? natureLayout.visibleRows :
        compactChoices ? std::min(8,totalChoices) : 10;
    const auto valueList = valueListLayout(state.pickerTarget, totalChoices);
    const int w = heldItems ? HeldItemGrid::modalWidth : movePicker ? moveLayout.width :
        speciesPicker ? 760 : naturePicker ? natureLayout.width :
        compactChoices ? valueList.frame.w : 680;
    const int h = heldItems ? HeldItemGrid::modalHeight : movePicker ? moveLayout.height :
        speciesPicker ? 560 : naturePicker ? natureLayout.height :
        compactChoices ? valueList.frame.h : 560;
    const int x = (fb.getWidth() - w) / 2;
    const int y = (fb.getHeight() - kNavBarH - h) / 2;

    // Match the accepted Gen I-III picker family: neutral dialog chrome,
    // theme focus only on the active row/cell, and one authoritative footer.
    fb.drawFilledRect(0, 0, fb.getWidth(), fb.getHeight(), Color(0, 0, 0, 165));
    fb.drawFilledRoundedRect(x, y, w, h, 18, Colors::Panel);
    fb.drawRoundedRect(x, y, w, h, 18, Colors::Divider, 1);

    if (speciesPicker) {
        const auto modal = SharedSpeciesPicker::drawModalChrome(fb);
        const bool previewShiny = state.speciesPreviewShiny;
        const uint16_t species =
            state.pickerValues[static_cast<size_t>(state.pickerRow)];
        const auto previewTypes = A::speciesPreviewTypes(screen, species);
        SharedSpeciesPicker::refilter(state.speciesChooser, A::kMaxSpecies,
            [&](uint16_t s) { return speciesAvailableIn(state.pickerValues, s); }, sharedSpeciesName);
        SharedSpeciesPicker::drawContent(
            fb, modal.x, modal.y, species, A::kMaxSpecies, previewShiny,
            [](uint16_t value) { return A::speciesLabel(value); },
            [](uint16_t value) { return A::speciesLabel(value); },
            &latestTouchGesture(), true, previewTypes, &state.speciesChooser);
        // Touch ids stay 6000 + pickerValues index; the chooser publishes the rows it drew.
        for (int i = 0; i < state.speciesChooser.rowCount; ++i) {
            const auto& row = state.speciesChooser.rows[static_cast<std::size_t>(i)];
            const int index = speciesRowIn(state.pickerValues, row.species, -1);
            if (index >= 0) screen.touchButtons.push_back({6000 + index, row.x, row.y, row.w, row.h});
        }
        Presentation::PickerFooterSpec footer;
        footer.jump = true;
        footer.xLabel = "Search";   // R2-SEARCH: in-modal live search in every generation
        footer.yLabel = "Normal/Shiny";
        SharedEditorChrome::drawPickerFooter(fb, footer);
        return;
    }

    if (movePicker) {
        fb.drawText(x + 24, y + 18, A::kMovePickerTitle, Colors::Text, TextStyle::Heading);
        {
            const std::string sort = std::string("Sort: ") + ItemPickerSort::label(state.pickerSort);
            int sw = 0, sh = 0;
            fb.measureText(sort, sw, sh, TextStyle::Caption);
            fb.drawText(x + w - 24 - sw, y + 26, sort, Colors::TextDim, TextStyle::Caption);
        }
        const auto columns = SharedMovesPanel::chooserColumns(x + 40, w - 80);
        SharedMovesPanel::drawChooserCaptions(fb, columns, y + 58, A::moveGame(screen));
        const int count = static_cast<int>(state.pickerValues.size());
        constexpr int visible = moveLayout.visibleRows;
        const int first = state.pickerViewport.firstRow;
        const int last = std::min(count, first + visible);
        const int viewportTop = y + moveLayout.rowStart;
        fb.setClipRect(x + 24, viewportTop, w - 48, visible * moveLayout.rowStep);
        for (int i = first; i < last; ++i) {
            const int yy = viewportTop + (i - first) * moveLayout.rowStep + state.pickerScroll.offset();
            appendClippedTouchButton(screen.touchButtons, 6000 + i,
                x + 24, yy, w - 48, moveLayout.highlightHeight,
                x + 24, viewportTop, w - 48, visible * moveLayout.rowStep);
            const bool selected = i == state.pickerRow;
            SharedBoxedRows::drawBox(fb, SharedBoxedRows::rect(x + 24, yy, w - 48, moveLayout.highlightHeight), {selected});  // R2-BUG-012 boxed row
            const auto value = state.pickerValues[static_cast<size_t>(i)];
            SharedMovesPanel::drawChooserRow(fb, columns, yy + 6, value, A::moveGame(screen), selected);
        }
        fb.clearClip();
        SharedScrollbar::drawAndPublish(fb, state.pickerScrollbar, x + w - 18, y + moveLayout.rowStart,
            SharedScrollbar::kTrackWidth, visible * moveLayout.rowStep,
            SharedScrollbar::rows(count, visible, first));
        Presentation::PickerFooterSpec footer;
        footer.jump = true;
        footer.xLabel = "Sort";
        SharedEditorChrome::drawPickerFooter(fb, footer);
        return;
    }

    const int count = static_cast<int>(state.pickerValues.size());
    if (heldItems) {
        SharedHeldItemPickerPresentation::drawHeading(
            fb, x, y, A::kHeldItemTitle, state.pickerRow, count);
        fb.drawText(x + w - 250, y + 55,
                    std::string("Sort: ") + ItemPickerSort::label(state.pickerSort),
                    Colors::TextDim, TextStyle::Caption);
        SharedHeldItemPickerPresentation::drawGrid(
            fb, x, y, state.pickerValues, state.pickerRow,
            [&](uint16_t value) { return A::pickerLabel(screen, state, PickerTarget::HeldItem, value); },
            [&](uint16_t value) { return A::heldItemArtKey(screen, value); },
            state.pickerViewport.firstRow, state.pickerScroll.offset());
        const int gridX = x + HeldItemGrid::gridInsetX;
        const int gridY = y + HeldItemGrid::gridInsetY;
        const int cellHeight = HeldItemGrid::rowPitch;
        const int cellW = (w - 2 * HeldItemGrid::gridInsetX) / HeldItemGrid::columns;
        const int firstRow = state.pickerViewport.firstRow;
        const int firstVisibleItem = state.pickerViewport.firstRow * HeldItemGrid::columns + 1;
        const int lastVisibleItem = std::min(count,
            firstVisibleItem + HeldItemGrid::pageSize - 1);
        (void)lastVisibleItem;
        for (int i = std::max(0, (firstRow - 1) * HeldItemGrid::columns);
             i < std::min(count, (firstRow + HeldItemGrid::rows + 1) * HeldItemGrid::columns); ++i) {
            const int offset = i - firstRow * HeldItemGrid::columns;
            const int cellX = gridX + offset % HeldItemGrid::columns * cellW;
            const int cellY = gridY + offset / HeldItemGrid::columns * cellHeight + state.pickerScroll.offset();
            appendClippedTouchButton(screen.touchButtons, 6000 + i,
                cellX, cellY - 3, cellW - 10, cellHeight - 4,
                gridX, gridY - 3, w - 44, HeldItemGrid::rows * cellHeight);
        }
        Presentation::PickerFooterSpec footer;
        footer.gridPaging = true;
        footer.xLabel = "Sort";
        SharedEditorChrome::drawPickerFooter(fb, footer);
        return;
    }
    fb.drawText(x + 28, y + 20, pickerTitle(state.pickerTarget),
                Colors::Text, TextStyle::Heading);

    fb.drawText(x + 28, y + 55,
                std::to_string(state.pickerRow + 1) + " / " +
                    std::to_string(state.pickerValues.size()),
                Colors::TextDim, TextStyle::Caption);
    const int visible = compactChoices ? visibleChoices : 10;
    const int first = state.pickerViewport.firstRow;
    const int last = std::min(count, first + visible);
    const int rowStep = naturePicker ? natureLayout.rowStep : 42;
    const int viewportTop = y + 90;
    fb.setClipRect(x + 22, viewportTop - 4, w - 44, visible * rowStep);
    for (int i = first; i < last; ++i) {
        const int yy = viewportTop + (i - first) * rowStep + state.pickerScroll.offset();
        appendClippedTouchButton(screen.touchButtons, 6000 + i,
            x + 22, yy - 4, w - 44, naturePicker ? natureLayout.highlightHeight : 38,
            x + 22, viewportTop - 4, w - 44, visible * rowStep);
        const bool selected = i == state.pickerRow;
        SharedBoxedRows::drawBox(fb, SharedBoxedRows::rect(x + 22, yy - 4, w - 44, naturePicker ? natureLayout.highlightHeight : 38), {selected});  // R2-BUG-012 boxed row
        const auto value = state.pickerValues[static_cast<size_t>(i)];
        const std::string label = A::pickerLabel(screen, state, state.pickerTarget, value);
        const auto split = label.find("  |  ");
        if (state.pickerTarget == PickerTarget::MetLocation && split != std::string::npos) {
            // Same row style as Gen III: bright place name left, met level right.
            const std::string detail = label.substr(split + 5);
            int dw = 0, dh = 0;
            fb.measureText(detail, dw, dh, TextStyle::Caption);
            fb.drawText(x + 38, yy + 7, SharedMovesPanel::fit(fb, label.substr(0, split),
                        w - 76 - dw - 20, TextStyle::Body), Colors::Text, TextStyle::Body);
            fb.drawText(x + w - 38 - dw, yy + 10, detail,
                        selected ? Colors::Text : Colors::TextSecondary, TextStyle::Caption);
        } else if (state.pickerTarget == PickerTarget::Ball) {
            // The ball sprite leads its name at the BallPicker rule's gap (no far-right sprite).
            const auto cell = PokeBank::UIModel::SharedMenuGeometry::Rect{x + 22, yy - 4, w - 44, 38};
            const auto sprite = PokeBank::UIModel::SharedMenuGeometry::spriteRect(valueList, cell);
            ItemPickerArtwork::draw(fb, sprite.right(), sprite.y, sprite.w,
                                    Enums::getBallName(static_cast<uint8_t>(value)));
            fb.drawText(PokeBank::UIModel::SharedMenuGeometry::labelRect(valueList, cell).x, yy + 7,
                        label, Colors::Text, TextStyle::Body);
        } else {
            fb.drawText(x + 38, yy + 7, label, Colors::Text, TextStyle::Body);
        }
    }
    fb.clearClip();
    SharedScrollbar::drawAndPublish(fb, state.pickerScrollbar, x + w - 14, viewportTop - 4,
        SharedScrollbar::kTrackWidth, visible * rowStep,
        SharedScrollbar::rows(count, visible, first));
    Presentation::PickerFooterSpec footer;
    footer.page = count > visible;
    footer.jump = count > 50;
    SharedEditorChrome::drawPickerFooter(fb, footer);
}

template <class A>
void drawMoveEditor(TrainerViewScreen& screen, PKSEFramebuffer& fb, EditorState& state) {
    if (!state.moveEditor || !A::hasWorking(screen, state)) return;
    const int slot = state.moveEditorSlot;
    const uint16_t move = A::workingMove(screen, state, slot);
    constexpr int w = 680, h = 390;
    const int x = (fb.getWidth() - w) / 2;
    const int y = (fb.getHeight() - kNavBarH - h) / 2;
    SharedEditorChrome::beginModal(screen.touchButtons);
    fb.drawFilledRect(0, 0, fb.getWidth(), fb.getHeight() - kNavBarH, Color(0,0,0,100));
    drawPanelSurface(fb, x, y, w, h, false, 18);
    fb.drawText(x + 24, y + 18, "Move " + std::to_string(state.moveEditorSlot + 1),
                Colors::Text, TextStyle::Heading);
    {
        const std::string detail = PokeBank::UIModel::SharedMoveFacts::detailLine(
            PokeBank::UIModel::SharedMoveFacts::facts(move, A::moveGame(screen)));
        fb.drawText(x + 24, y + 52, detail.empty() ? std::string(A::kMoveEditorSubtitle) : detail,
                    Colors::TextDim, TextStyle::Caption);
    }
    const std::array<std::pair<std::string,std::string>,5> rows{{
        {"Move", move == 0 ? "Empty" : std::string(Names::getMoveName(move))},
        {"PP", std::to_string(A::workingPP(screen, state, slot)) + "/" +
               std::to_string(A::maxPP(screen, move, A::workingPPUps(screen, state, slot)))},
        {"PP Ups", std::to_string(A::workingPPUps(screen, state, slot))},
        {"Apply", "Keep these move details"},
        {"Cancel", "Discard these move details"},
    }};
    for (int i = 0; i < 5; ++i) {
        const int yy = y + 88 + i * 48;
        const bool selected = i == state.moveEditorRow;
        screen.touchButtons.push_back({41400 + i, x + 22, yy, w - 44, 42});
        SharedBoxedRows::drawBox(fb, SharedBoxedRows::rect(x + 22, yy, w - 44, 42), {selected});  // R2-BUG-012 boxed row
        fb.drawText(x + 38, yy + 10, rows[static_cast<size_t>(i)].first,
                    selected ? Colors::Text : Colors::TextDim);
        int vw=0,vh=0;
        fb.measureText(rows[static_cast<size_t>(i)].second, vw, vh);
        fb.drawText(x + w - 38 - vw, yy + 10, rows[static_cast<size_t>(i)].second,
                    selected ? Colors::Text : Colors::TextDim);
    }
    Presentation::PickerFooterSpec footer;
    footer.page = false;
    SharedEditorChrome::drawPickerFooter(fb, footer);
}

template <class A>
void drawWorkspace(TrainerViewScreen& screen, PKSEFramebuffer& fb, EditorState& state) {
    if (!A::hasWorking(screen, state)) return;
    const auto raw = A::preview(screen, state);
    if (!raw) return;
    constexpr int detailRows = A::kDetailRows;

    const ChromeInfo chrome = A::chrome(screen, state, *raw);
    SharedPokemonShell::drawChrome(
        fb, chrome.species, chrome.nickname, chrome.level,
        chrome.gender, chrome.shiny, chrome.mode, chrome.context, screen.sourceGameId);

    const SharedPokemonShell::Geometry g(fb.getWidth(), fb.getHeight(), kNavBarH);
    drawPanel(fb, g.leftX, g.y, g.leftW, g.h, "DETAILS");
    SharedPokemonShell::drawPortrait(fb, g.leftX, g.y, chrome.species, chrome.shiny);
    const std::array<uint8_t,2> types = A::portraitTypes(screen, *raw);
    int typeX = g.leftX + 66;
    for (size_t i = 0; i < types.size(); ++i) {
        if (types[i] == 255 || (i && types[i] == types[0])) continue;
        if (auto* sprite = SpriteManager::getTypeSprite(types[i]);
            sprite && sprite->data && sprite->height > 0) {
            const int width = sprite->width * 22 / sprite->height;
            fb.drawImageScaled(typeX, g.y + 180, sprite->width, sprite->height,
                               width, 22, sprite->data, sprite->channels);
            typeX += width + 8;
        }
    }
    constexpr std::size_t detailVisibleRows = Workspace::kVisibleDetailRows;
    constexpr int detailRowStep = 38;
    const auto detailViewport = SharedPokemonShell::detailsViewport(g);
    const int detailViewportY = detailViewport.top;
    const int detailViewportH = detailViewport.height;
    state.detailsViewport.ensure(state.detailsFocus, detailRows,
                                 static_cast<int>(detailVisibleRows));
    const int firstDetailRow = state.detailsViewport.firstRow;
    const int detailPixelOffset = state.detailsTouchScroll.offset();
    SharedPokemonShell::drawScrollableDetails(
        fb, g.leftX, g.y, g.leftW, g.h, static_cast<std::size_t>(detailRows),
        static_cast<std::size_t>(state.detailsFocus),
        state.focus.panel == Shared::Panel::Details,
        [](size_t row) { return A::detailLabel(static_cast<int>(row)); },
        [&](size_t row) { return A::detailValue(screen, state, *raw, static_cast<int>(row)); },
        [&](size_t row) {
            return A::detailLocked(screen, state, static_cast<int>(row), *raw)
                ? PokeBank::UIModel::SharedFieldState::State::ReadOnly
                : PokeBank::UIModel::SharedFieldState::State::Editable;
        },
        detailPixelOffset, firstDetailRow, &state.detailsScrollbar);
    {
        for (int row = std::max(0, firstDetailRow - 1);
             row < std::min(detailRows,
                            firstDetailRow + static_cast<int>(detailVisibleRows) + 1); ++row) {
            const int yy = detailViewport.firstRowY + (row - firstDetailRow) * detailRowStep + detailPixelOffset;
            appendClippedTouchButton(screen.touchButtons, 41000 + row,
                g.leftX + 8, yy + 2, g.leftW - 36, 30,
                g.leftX + 8, detailViewportY, g.leftW - 16, detailViewportH);
        }
    }

    drawPanel(fb, g.midX, g.y, g.midW, g.h, "STATS");
    fb.drawText(g.midX + 126, g.y + 50, "IV", Colors::TextDim, TextStyle::Caption);
    fb.drawText(g.midX + 190, g.y + 50, "EV", Colors::TextDim, TextStyle::Caption);
    fb.drawText(g.midX + 274, g.y + 50, "Stat", Colors::TextDim, TextStyle::Caption);
    const auto iv = A::ivs(*raw);
    const auto ev = A::evs(*raw);
    const std::array<uint16_t,6> stats = A::stats(screen, state, *raw);
    constexpr std::array<const char*,6> statLabels{{"HP","Attack","Defense","Speed","Sp. Atk","Sp. Def"}};
    for (int row = 0; row < 6; ++row) {
        const int yy = g.y + 78 + row * 40;
        const bool selected = state.focus.panel == Shared::Panel::Values && state.focus.row == row;
        screen.touchButtons.push_back({41100 + row, g.midX + 112, yy - 5, 62, 30});
        screen.touchButtons.push_back({41120 + row, g.midX + 176, yy - 5, 70, 30});
        if (selected) {
            const auto cell = Shared::cellFocusFor(A::generation, state.focus);
            drawFocus(fb, g.midX + cell.x, yy - 5, cell.width, 30, true);
        }
        fb.drawText(g.midX + 18, yy, statLabels[static_cast<size_t>(row)],
                    selected ? Colors::SelectedText : Colors::TextDim, TextStyle::Caption);
        fb.drawText(g.midX + 126, yy, std::to_string(iv[static_cast<size_t>(row)]), Colors::Text, TextStyle::Caption);
        fb.drawText(g.midX + 190, yy, std::to_string(ev[static_cast<size_t>(row)]), Colors::Text, TextStyle::Caption);
        // Phase 4: a calculated stat is derived, never edited: the gold ReadOnly field state.
        fb.drawText(g.midX + 274, yy, std::to_string(stats[static_cast<size_t>(row)]),
                    SharedFieldStateColors::color(PokeBank::UIModel::SharedFieldState::Token::Gold), TextStyle::Caption);
    }
    fb.drawHDivider(g.midX + 14, g.y + 310, g.midW - 28);
    fb.drawText(g.midX + 18, g.y + 318, "PID-LINKED / SAFE EDIT", Colors::TextDim, TextStyle::Caption);
    constexpr std::array<const char*,5> derivedLabels{{"Nature", "Gender", "Shiny", "Ability", "PID"}};
    const bool viewMode = A::passive(screen, state);
    for (int i = 0; i < 5; ++i) {
        const int row = i + Workspace::kNatureRow;
        const int yy = g.y + 346 + i * 35;
        const bool editable = A::pidRowEditable(screen, state, row, *raw);
        const bool selected = state.focus.panel == Shared::Panel::Values &&
                              state.focus.row == row && (viewMode || editable);
        screen.touchButtons.push_back({41200 + row, g.midX + 8, yy - 5, g.midW - 16, 29});
        drawFocus(fb, g.midX + 8, yy - 5, g.midW - 16, 29, selected);
        fb.drawText(g.midX + 18, yy, derivedLabels[static_cast<size_t>(i)],
                    selected ? Colors::SelectedText : editable ? Colors::TextDim : Colors::TextSecondary,
                    TextStyle::Caption);
        Color valueColor = selected ? Colors::SelectedText : Colors::Text;
        if (i == 1 && editable && !selected) valueColor = genderValueColor(A::gender(*raw));
        if (i == 4) valueColor = SharedFieldStateColors::color(PokeBank::UIModel::SharedFieldState::Token::Gold);   // PID: derived, gold ReadOnly
        fb.drawText(g.midX + 130, yy, A::pidRowValue(screen, *raw, row),
                    valueColor, TextStyle::Caption);
        // Keep the PID status on its own subordinate line, so the 10-digit
        // value never overlaps the label in the compact shared middle panel.
        if (i == 4)
            fb.drawText(g.midX + 18, yy + 15, "read-only",
                        Colors::TextDim, TextStyle::Caption);
    }

    drawPanel(fb, g.rightX, g.y, g.rightW, g.h, "MOVES");
    {
        // The ONE shared Moves panel (UI/SharedMovesPanel.h); the adapter supplies only native
        // move ids, PP, PP Ups, its compatibility status and its exact game version.
        const auto moves = A::moves(*raw);
        const auto pp = A::pp(*raw);
        const auto ups = A::ppUps(*raw);
        const auto movesLayout = SharedMovesPanel::layout(g.rightX, g.y, g.rightW);
        std::array<SharedMovesPanel::Slot, 4> slots{};
        for (int row = 0; row < 4; ++row) {
            const auto i = static_cast<size_t>(row);
            auto& slot = slots[i];
            slot.move = moves[i];
            slot.pp = pp[i];
            slot.ppUps = ups[i];
            slot.maxPP = moves[i] == 0 ? 0 : A::maxPP(screen, moves[i], ups[i]);
            const char* status = "";
            Color statusColor = Colors::TextDim;
            A::moveStatus(screen, state, *raw, moves[i], status, statusColor);
            slot.status = moves[i] == 0 ? "" : status;
            slot.statusColor = statusColor;
            screen.touchButtons.push_back({41300 + row, movesLayout.focusX(), movesLayout.rowTop(row),
                                           movesLayout.focusW(), SharedMovesPanel::Layout::kRowHeight});
        }
        const bool movesFocused = state.focus.panel == Shared::Panel::Moves;
        SharedMovesPanel::draw(fb, movesLayout, slots, A::moveGame(screen),
                               movesFocused ? static_cast<int>(state.focus.row) : -1);
    }

    const std::vector<DataRow> rows = A::dataRows(screen, state, *raw);
    // Every generation titles its provenance pane with the one shared heading.
    static_assert(std::string_view(A::kDataTitle) == Presentation::kOriginLegalityHeading);
    SharedPokemonShell::drawDataAndGraph(
        fb, g.rightX, g.y + 244, g.rightW, g.h - 256, A::kDataTitle, rows, stats);

    if (viewMode)
        drawNavBar(fb, {{"D-pad/Stick", "Navigate"}, {"L/R", "Panel"}, {"B", "Back"}});
    else
        drawNavBar(fb, {{"D-pad/Stick", "Navigate"}, {"L/R", "Panel"}, {"A", "Edit"},
                        {"Y", PokeBank::UIModel::SharedStatReroll::kRerollHint},
                        {"X", A::creating(screen, state) ? "Add" : "Keep"}, {"B", "Exit"}});

    if (A::confirmExit(screen, state)) {
        constexpr int w = 560, h = 226;
        const int x = (fb.getWidth() - w) / 2;
        const int y = (fb.getHeight() - h) / 2;
        // The dialog owns this frame: the workspace footer and rows underneath stop being tappable.
        SharedEditorChrome::beginModal(screen.touchButtons);
        const int cy = Dialogs::drawDialogFrame(fb, x, y, w, h, "Unsaved changes", Colors::Orange);
        const bool creating = A::creating(screen, state);
        fb.drawText(x + 28, cy,
                    creating ? "This new Pokémon draft has not been added."
                             : "This Pokémon has unsaved changes.",
                    Colors::Text);
        fb.drawText(x + 28, cy + 34, "Discard loses them; Back keeps editing.",
                    Colors::TextDim, TextStyle::Caption);
        const int cbw = 160, cbh = TouchTargetMin, cby = y + h - cbh - 18;
        drawGlyphButton(fb, x + 24, cby, cbw, cbh, "B", "Back", Colors::PanelAlt);
        drawGlyphButton(fb, x + (w - cbw) / 2, cby, cbw, cbh, "Y", "Discard", Colors::PanelAlt);
        drawGlyphButton(fb, x + w - 24 - cbw, cby, cbw, cbh, "A",
                        creating ? "Add" : "Save", Colors::PanelAlt);
        SharedEditorChrome::drawFooter(fb, Presentation::confirmFooter(
            "A", creating ? "Add" : "Save", "Y", "Discard"));
    }

    if (state.moveEditor) drawMoveEditor<A>(screen, fb, state);
    if (state.valuePicker) drawValuePicker<A>(screen, fb, state);
}

} // namespace UI::SharedPokemonEditorSurface
