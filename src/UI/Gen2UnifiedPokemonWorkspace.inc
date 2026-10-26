#include "UI/ReviewEntry.h"
#include "UI/SharedPokemonEditorContract.h"
#include "UI/Gen2WorkspacePresentation.h"
#include "UI/Gen2FieldSelector.h"
#include "UI/Gen2WorkspaceLayers.h"
#include "UI/Gen2MovesPanel.h"
#include "UI/ClassicFieldColor.h"
#include "UI/SharedEditorPresentation.h"
#include "Integration/Encounter/EncounterGuardrails.h"

namespace UI::Gen2PokemonEditor {
namespace {

namespace Unified = PokeBank::UIModel::SharedPokemonEditor;
namespace Encounter = PokeVault::Integration::EncounterGuardrails;

struct UnifiedWorkspaceState {
    const TrainerViewScreen* owner = nullptr;
    uint64_t ownerSerial = 0;   // TrainerViewScreen::instanceSerial (never reused, unlike the address)
    std::string sourceGameId;
    Mode lastMode = Mode::None;
    Unified::Focus focus{};
    std::size_t detailsFocus = 0;
    // Pokerus / Crystal Met Time / Crystal OT Gender selector (Gen2FieldSelector.h).
    PokeBank::UIModel::Gen2FieldSelector::Selector selector;
    // Stable row viewport: drag moves the content, not the semantic focus.
    // Otherwise the older scrollWindow(focusedRow) snaps to its old page,
    // making Held Item and Crystal metadata painful to reach by touch.
    TouchScrollState detailsTouchScroll;
    TouchPickerViewport detailsViewport;
    SharedScrollbar::State detailsScrollbar;   // draggable thumb beside the Details rows
    int detailsTouchX = 0, detailsTouchY = 0;
    int detailsTouchW = 0, detailsTouchH = 0;
    bool crystal = false;
    bool progressionEditor = false;
    int progressionRow = 0;
    Gen2::PokemonRecord progressionBaseline{};
    bool moveEditor = false;
    int moveEditorRow = 0;
    int moveEditorSlot = 0;
    uint16_t moveBaseline = 0;
    uint8_t movePpBaseline = 0;
    uint8_t movePpUpsBaseline = 0;
    // Round 7 (B07 / C09): the Unown Letter grid and its two questions (Gen2UnownLetter.h).
    PokeBank::UIModel::Gen2UnownLetter::Picker unownPicker;
    PokeBank::UIModel::Gen2UnownLetter::Question unownQuestion;
};

UnifiedWorkspaceState& unifiedStateFor(const TrainerViewScreen& screen) {
    static UnifiedWorkspaceState state;
    const auto mode = stateFor(screen).mode;
    if ((state.owner != &screen || state.ownerSerial != screen.instanceSerial) || state.sourceGameId != screen.sourceGameId) {
        state = {};
        state.owner = &screen; state.ownerSerial = screen.instanceSerial;
        state.sourceGameId = screen.sourceGameId;
        state.lastMode = mode;
    } else if (state.lastMode != mode) {
        state.focus = {};
        state.detailsFocus = 0;
        state.detailsTouchScroll.reset();
        state.detailsViewport.reset();
        state.detailsTouchW = state.detailsTouchH = 0;
        state.progressionEditor = false;
        state.progressionRow = 0;
        state.moveEditor = false;
        state.moveEditorRow = 0;
        state.selector = {};
        state.unownPicker = {};
        state.unownQuestion = {};
        state.lastMode = mode;
    }
    state.crystal = screen.sourceGameId == "crystal_gbc";
    return state;
}

// The single owner of this frame's input (and footer); see Gen2WorkspaceLayers.h.
PokeBank::UIModel::Gen2WorkspaceLayers::Layer workspaceTopLayer(const TrainerViewScreen& screen) {
    const auto& state = stateFor(screen);
    const auto& unified = unifiedStateFor(screen);
    PokeBank::UIModel::Gen2WorkspaceLayers::Open open;
    open.picker = pickerStateFor(screen).model.active();
    open.heldItemPicker = state.itemPicker;
    open.exitConfirm = state.confirmExit;
    open.progressionEditor = unified.progressionEditor;
    open.moveEditor = unified.moveEditor;
    open.fieldSelector = unified.selector.open();
    open.unownQuestion = unified.unownQuestion.ask != PokeBank::UIModel::Gen2UnownLetter::Ask::None;
    open.unownPicker = unified.unownPicker.open;
    return PokeBank::UIModel::Gen2WorkspaceLayers::topLayer(open);
}

void drawWorkspaceFooter(PKSEFramebuffer& fb, const TrainerViewScreen& screen) {
    namespace Layers = PokeBank::UIModel::Gen2WorkspaceLayers;
    const auto footer = Layers::footer(workspaceTopLayer(screen),
        stateFor(screen).mode == Mode::Create ? Layers::Session::Create : Layers::Session::Edit);
    std::vector<ControllerHint> hints;
    for (std::size_t i = 0; i < footer.count; ++i)
        hints.push_back({footer.hints[i].glyph, footer.hints[i].label});
    drawNavBar(fb, hints);
}

void resetUnifiedFocus(TrainerViewScreen& screen) {
    auto& unified = unifiedStateFor(screen);
    unified.focus = {};
    unified.detailsFocus = 0;
    unified.detailsTouchScroll.reset();
    unified.detailsViewport.reset();
    unified.detailsTouchW = unified.detailsTouchH = 0;
    unified.progressionEditor = false;
    unified.progressionRow = 0;
    unified.moveEditor = false;
    unified.moveEditorRow = 0;
    unified.selector = {};
    unified.unownPicker = {};
    unified.unownQuestion = {};
    unified.lastMode = stateFor(screen).mode;
    screen.controllerNavigation.reset();
}

void mirrorPickerFocus(State& state, const Unified::Focus& focus) {
    if (focus.panel == Unified::Panel::Details) {
        // Pokérus lives on shared DETAILS row 8; the proven named picker adapter still uses
        // its internal legacy Values row 7. Keep that backend detail hidden behind the shared shell.
        if (focus.row == 8) {
            state.panel = Foundation::Panel::Values;
            state.row = 7;
            state.column = 0;
        } else {
            state.panel = Foundation::Panel::Identity;
            state.row = static_cast<int>(focus.row);
            state.column = 0;
        }
        return;
    }
    if (focus.panel == Unified::Panel::Moves) {
        state.panel = Foundation::Panel::Moves;
        state.row = focus.row;
        state.column = focus.column;
        return;
    }
    state.panel = Foundation::Panel::Values;
    state.row = static_cast<int>(focus.row);
    state.column = focus.column;
}

bool sharedPickerField(const Unified::Focus& focus) noexcept {
    return (focus.panel == Unified::Panel::Details && (focus.row == 0 || focus.row == 8)) ||
           (focus.panel == Unified::Panel::Moves && focus.column == 0);
}

void beginProgressionEditor(TrainerViewScreen& screen) {
    auto& unified = unifiedStateFor(screen);
    unified.progressionEditor = true;
    unified.progressionRow = 0;
    unified.progressionBaseline = stateFor(screen).working;
    screen.controllerNavigation.reset();
}

bool handleProgressionEditor(TrainerViewScreen& screen, uint64_t down, uint64_t held,
                             int stickX, int stickY) {
    auto& unified = unifiedStateFor(screen);
    if (!unified.progressionEditor) return false;
    auto& state = stateFor(screen);
    const uint64_t nav = screen.controllerNavigation.apply(
        down, held, stickX, stickY,
        HidNpadButton_Up, HidNpadButton_Down, HidNpadButton_Left, HidNpadButton_Right);
    if (nav & HidNpadButton_Up) unified.progressionRow = (unified.progressionRow + 3) % 4;
    if (nav & HidNpadButton_Down) unified.progressionRow = (unified.progressionRow + 1) % 4;
    if (down & HidNpadButton_B) {
        state.working = unified.progressionBaseline;
        unified.progressionEditor = false;
        screen.controllerNavigation.reset();
        return true;
    }
    if (!(down & HidNpadButton_A)) return true;

    if (unified.progressionRow == 0) {
        const auto result = Utils::promptNumber("Level", state.working.level, 1, 100);
        if (result.accepted) {
            const auto caught = Gen2Native::decodeCrystalCaughtData(state.working.caughtData);
            if (screen.sourceGameId == "crystal_gbc" && caught.present && caught.levelCode >= 2 &&
                result.value < caught.levelCode) {
                screen.postStatus("Current level cannot be below this Crystal met level", 300);
            } else {
                state.setLevel(static_cast<uint8_t>(result.value));
            }
        }
    } else if (unified.progressionRow == 1) {
        const auto result = Utils::promptNumber("Experience", static_cast<int>(state.working.experience),
                                                0, static_cast<int>(state.maximumExperience()));
        if (result.accepted) {
            const auto caught = Gen2Native::decodeCrystalCaughtData(state.working.caughtData);
            const auto* personal = Gen2::personalRecord(state.working.species);
            const uint8_t nextLevel = personal
                ? Pokemon::getLevelFromExp(static_cast<uint32_t>(result.value), personal->experienceGrowth)
                : state.working.level;
            if (screen.sourceGameId == "crystal_gbc" && caught.present && caught.levelCode >= 2 &&
                nextLevel < caught.levelCode) {
                screen.postStatus("Experience would put current level below this Crystal met level", 300);
            } else {
                state.setExperience(static_cast<uint32_t>(result.value));
            }
        }
    } else if (unified.progressionRow == 2) {
        unified.progressionEditor = false;
        screen.controllerNavigation.reset();
    } else {
        state.working = unified.progressionBaseline;
        unified.progressionEditor = false;
        screen.controllerNavigation.reset();
    }
    return true;
}


void beginUnifiedMoveEditor(TrainerViewScreen& screen, State& state, int slot) {
    auto& unified = unifiedStateFor(screen);
    unified.moveEditor = true;
    unified.moveEditorRow = 0;
    unified.moveEditorSlot = std::clamp(slot, 0, 3);
    const auto index = static_cast<std::size_t>(unified.moveEditorSlot);
    unified.moveBaseline = state.working.moves[index];
    unified.movePpBaseline = state.working.pp[index];
    unified.movePpUpsBaseline = state.working.ppUps[index];
    screen.controllerNavigation.reset();
}

void cancelUnifiedMoveEditor(TrainerViewScreen& screen, State& state) {
    auto& unified = unifiedStateFor(screen);
    if (!unified.moveEditor) return;
    const auto index = static_cast<std::size_t>(unified.moveEditorSlot);
    state.working.moves[index] = unified.moveBaseline;
    state.working.pp[index] = unified.movePpBaseline;
    state.working.ppUps[index] = unified.movePpUpsBaseline;
    unified.moveEditor = false;
    unified.moveEditorRow = 0;
    screen.controllerNavigation.reset();
}

bool handleUnifiedMoveEditor(TrainerViewScreen& screen, State& state, uint64_t down,
                             uint64_t held, int stickX, int stickY) {
    auto& unified = unifiedStateFor(screen);
    if (!unified.moveEditor) return false;
    if (pickerStateFor(screen).model.active()) return false;

    const uint64_t nav = screen.controllerNavigation.apply(
        down, held, stickX, stickY,
        HidNpadButton_Up, HidNpadButton_Down, HidNpadButton_Left, HidNpadButton_Right);
    if (nav & HidNpadButton_Up) unified.moveEditorRow = (unified.moveEditorRow + 4) % 5;
    if (nav & HidNpadButton_Down) unified.moveEditorRow = (unified.moveEditorRow + 1) % 5;
    if (down & HidNpadButton_B) {
        cancelUnifiedMoveEditor(screen, state);
        return true;
    }
    if (!(down & HidNpadButton_A)) return true;

    const int slot = unified.moveEditorSlot;
    const auto index = static_cast<std::size_t>(slot);
    if (unified.moveEditorRow == 0) {
        Unified::Focus moveFocus{Unified::Panel::Moves, static_cast<uint8_t>(slot), 0};
        mirrorPickerFocus(state, moveFocus);
        auto& picker = pickerStateFor(screen);
        // Same entry normalization as the controller path: the highlighted row is exactly
        // the choice A applies, even when the slot holds a move outside the compatible list.
        if (openPickerForFocusedField(screen, state, picker, HidNpadButton_A))
            normalizeHardwareMovePicker(screen, state, picker);
        return true;
    }
    if (unified.moveEditorRow == 1) {
        if (state.working.moves[index] == 0) {
            screen.postStatus("Choose a move before editing PP", 220);
            return true;
        }
        const auto result = Utils::promptNumber(
            "Move PP", std::min(state.working.pp[index], state.maximumPP(slot)),
            0, state.maximumPP(slot));
        if (result.accepted) state.setPP(slot, static_cast<uint8_t>(result.value));
        return true;
    }
    if (unified.moveEditorRow == 2) {
        if (state.working.moves[index] == 0) {
            screen.postStatus("Empty move slots keep PP Ups at zero", 220);
            return true;
        }
        const auto result = Utils::promptNumber("PP Ups", state.working.ppUps[index], 0, 3);
        if (result.accepted) state.setPPUps(slot, static_cast<uint8_t>(result.value));
        return true;
    }
    if (unified.moveEditorRow == 3) {
        unified.moveEditor = false;
        unified.moveEditorRow = 0;
        screen.controllerNavigation.reset();
        return true;
    }
    cancelUnifiedMoveEditor(screen, state);
    return true;
}

uint8_t crystalAllowedTimeMask(const TrainerViewScreen& screen,
                               const Gen2::PokemonRecord& p,
                               const Gen2Native::CrystalCaughtData& caught) {
    uint8_t mask = 0;
    for (const auto& encounter : Encounter::forGameSpecies(screen.sourceGameId, p.species)) {
        if (encounter.location != caught.location) continue;
        const bool levelMatches = caught.levelCode == 1
            ? encounter.method == Encounter::Method::Egg
            : caught.levelCode >= 2 && encounter.containsLevel(caught.levelCode);
        if (!levelMatches) continue;
        mask |= encounter.timeMask == 0 ? static_cast<uint8_t>(0x0E) : encounter.timeMask;
    }
    return mask == 0 ? static_cast<uint8_t>(0x0E) : mask;
}

constexpr int kSelectorTouchBase = 1760;

// Writes a confirmed selector choice with exactly the side effects the previous in-place cycling
// had. Intentionally do not touch p.caughtData for pending values: they must never fabricate
// Crystal met location/level/time history; recorded history is re-encoded.
void applyFieldSelector(TrainerViewScreen& screen, State& state,
                        PokeBank::UIModel::Gen2FieldSelector::Field field, uint8_t value) {
    namespace Selector = PokeBank::UIModel::Gen2FieldSelector;
    auto& p = state.working;
    if (field == Selector::Field::Pokerus) {
        p.pokerus = value;
        return;
    }
    if (screen.sourceGameId != "crystal_gbc") return;
    auto caught = Gen2Native::decodeCrystalCaughtData(p.caughtData);
    if (field == Selector::Field::MetTime) {
        state.crystalMetTime = value;
        if (!caught.present) return;
        caught.timeOfDay = value;
    } else if (field == Selector::Field::OtGender) {
        state.crystalOtGender = static_cast<Gen2Native::CrystalOtGenderChoice>(value);
        if (!caught.present) return;
        caught.originalTrainerFemale = Gen2Native::crystalOtGenderFemale(state.crystalOtGender);
    }
    p.caughtData = Gen2Native::encodeCrystalCaughtData(caught);
}

void drawFieldSelector(TrainerViewScreen& screen, PKSEFramebuffer& fb,
                       const PokeBank::UIModel::Gen2FieldSelector::Selector& selector) {
    namespace Selector = PokeBank::UIModel::Gen2FieldSelector;
    const char* title = selector.field == Selector::Field::Pokerus ? "Pokérus"
                      : selector.field == Selector::Field::MetTime ? "Met Time" : "OT Gender";
    constexpr int w = 440, rowH = 52;
    const int h = 96 + selector.count * rowH;
    const int x = (fb.getWidth() - w) / 2;
    const int y = (fb.getHeight() - kNavBarH - h) / 2;
    fb.drawFilledRect(0, 0, fb.getWidth(), fb.getHeight() - kNavBarH, Color(0, 0, 0, 120));
    const int cy = Dialogs::drawDialogFrame(fb, x, y, w, h, title, Colors::Primary);
    for (int i = 0; i < selector.count; ++i) {
        const auto& choice = selector.choices[static_cast<std::size_t>(i)];
        const int rowY = cy + i * rowH;
        SharedBoxedRows::drawBox(fb, SharedBoxedRows::rect(x + 20, rowY, w - 40, rowH - 6), {i == selector.row});  // R2-BUG-012 boxed row
        fb.drawText(x + 36, rowY + 12, choice.label,
                    !choice.enabled ? Colors::TextDim
                                    : i == selector.row ? Colors::SelectedText : Colors::Text,
                    TextStyle::Body);
        if (choice.enabled)
            screen.touchButtons.push_back({kSelectorTouchBase + i, x + 20, rowY, w - 40, rowH - 6});
    }
}

// ---- Round 7 (owner B07 / C09): Unown Letter grid and its questions --------------------------
constexpr int kUnownPickerTouchBase = 1780;   // + letter (0..25)
constexpr int kUnownConfirmTouch = 1810;
constexpr int kUnownCancelTouch = 1811;

void applyUnownDVs(TrainerViewScreen& screen, State& state, const std::array<uint8_t, 4>& dvs) {
    namespace Letter = PokeBank::UIModel::Gen2UnownLetter;
    const auto before = editableDVs(state.working);
    applyEditableDVs(state.working, dvs);
    screen.postStatus("Unown " + Letter::letterText(PokeBank::UIModel::Gen12DVReroll::unownLetter(dvs)) +
                      (state.working.shiny ? ", shiny" : ", not shiny") + ": " +
                      Letter::dvChanges(before, dvs) + " (stats follow)", 360);
}

bool handleUnownQuestion(TrainerViewScreen& screen, State& state, uint64_t down, const TouchInput& touch) {
    auto& unified = unifiedStateFor(screen);
    const int tapped = screen.touchedButtonId(touch);
    if ((down & HidNpadButton_A) || tapped == kUnownConfirmTouch) {
        const auto question = unified.unownQuestion;
        unified.unownQuestion = {};
        unified.unownPicker = {};
        applyUnownDVs(screen, state, question.dvs);
        screen.controllerNavigation.reset();
    } else if ((down & HidNpadButton_B) || tapped == kUnownCancelTouch) {
        unified.unownQuestion = {};
        screen.postStatus("Unown unchanged", 200);
        screen.controllerNavigation.reset();
    }
    return true;
}

bool handleUnownPicker(TrainerViewScreen& screen, State& state, uint64_t down, uint64_t held,
                       int stickX, int stickY, const TouchInput& touch) {
    namespace Letter = PokeBank::UIModel::Gen2UnownLetter;
    auto& unified = unifiedStateFor(screen);
    auto& picker = unified.unownPicker;
    const uint64_t nav = screen.controllerNavigation.apply(
        down, held, stickX, stickY,
        HidNpadButton_Up, HidNpadButton_Down, HidNpadButton_Left, HidNpadButton_Right);
    if (nav & HidNpadButton_Left) picker.cursor = Letter::moveCursor(picker.cursor, -1, 0);
    if (nav & HidNpadButton_Right) picker.cursor = Letter::moveCursor(picker.cursor, 1, 0);
    if (nav & HidNpadButton_Up) picker.cursor = Letter::moveCursor(picker.cursor, 0, -1);
    if (nav & HidNpadButton_Down) picker.cursor = Letter::moveCursor(picker.cursor, 0, 1);
    if (down & HidNpadButton_B) {
        picker = {};
        screen.controllerNavigation.reset();
        return true;
    }
    const int tapped = screen.touchedButtonId(touch);
    const bool tappedLetter = tapped >= kUnownPickerTouchBase && tapped < kUnownPickerTouchBase + Letter::kLetterCount;
    if (tappedLetter) picker.cursor = tapped - kUnownPickerTouchBase;
    if (!(down & HidNpadButton_A) && !tappedLetter) return true;
    const auto result = Letter::pick(editableDVs(state.working), picker.cursor);
    if (result.action == Letter::PickAction::Ask) {
        unified.unownQuestion = result.question;   // the grid stays open under the question
    } else {
        if (result.action == Letter::PickAction::Apply) applyUnownDVs(screen, state, result.dvs);
        picker = {};
    }
    screen.controllerNavigation.reset();
    return true;
}

void drawUnownLetterPicker(TrainerViewScreen& screen, PKSEFramebuffer& fb, const State& state,
                           const UnifiedWorkspaceState& unified) {
    namespace Letter = PokeBank::UIModel::Gen2UnownLetter;
    const auto dvs = editableDVs(state.working);
    const int current = PokeBank::UIModel::Gen12DVReroll::unownLetter(dvs);
    constexpr int cell = 64, gap = 8, w = 32 + Letter::kGridColumns * (cell + gap) + 8;
    const int rows = (Letter::kLetterCount + Letter::kGridColumns - 1) / Letter::kGridColumns;
    const int h = 150 + rows * (cell + gap);
    const int x = (fb.getWidth() - w) / 2;
    const int y = (fb.getHeight() - kNavBarH - h) / 2;
    fb.drawFilledRect(0, 0, fb.getWidth(), fb.getHeight() - kNavBarH, Color(0, 0, 0, 120));
    const int cy = Dialogs::drawDialogFrame(fb, x, y, w, h, Letter::pickerTitle(dvs), Colors::Primary);
    for (int letter = 0; letter < Letter::kLetterCount; ++letter) {
        const int cx = x + 20 + (letter % Letter::kGridColumns) * (cell + gap);
        const int ry = cy + (letter / Letter::kGridColumns) * (cell + gap);
        const bool focused = letter == unified.unownPicker.cursor;
        SharedBoxedRows::drawBox(fb, SharedBoxedRows::rect(cx, ry, cell, cell), {focused});
        if (letter == current) fb.drawRoundedRect(cx + 3, ry + 3, cell - 6, cell - 6, 6, Colors::Accent, 2);
        fb.drawText(cx + 22, ry + 18, Letter::letterText(letter),
                    focused ? Colors::SelectedText : Colors::Text, TextStyle::Body);
        if (Letter::canBeShiny(letter)) fb.drawShinyMark(cx + cell - 20, ry + 6, 14, Colors::ShinyStar);
        screen.touchButtons.push_back({kUnownPickerTouchBase + letter, cx, ry, cell, cell});
    }
    fb.drawText(x + 20, y + h - 44, Letter::pickerNote(), Colors::TextDim, TextStyle::Caption);
}

void drawUnownQuestion(TrainerViewScreen& screen, PKSEFramebuffer& fb, const UnifiedWorkspaceState& unified) {
    namespace Letter = PokeBank::UIModel::Gen2UnownLetter;
    const auto& question = unified.unownQuestion;
    constexpr int w = 760, h = 230;
    const int x = (fb.getWidth() - w) / 2;
    const int y = (fb.getHeight() - kNavBarH - h) / 2;
    fb.drawFilledRect(0, 0, fb.getWidth(), fb.getHeight() - kNavBarH, Color(0, 0, 0, 120));
    const int cy = Dialogs::drawDialogFrame(fb, x, y, w, h, "Unown letter and Shiny", Colors::Primary);
    fb.drawText(x + 24, cy + 4, Letter::questionText(question), Colors::Text, TextStyle::Body);
    fb.drawText(x + 24, cy + 40, Letter::pickerNote(), Colors::TextDim, TextStyle::Caption);
    const int by = y + h - 70;
    drawGlyphButton(fb, x + 24, by, 330, 48, "A", Letter::confirmLabel(question));
    screen.touchButtons.push_back({kUnownConfirmTouch, x + 24, by, 330, 48});
    drawGlyphButton(fb, x + w - 24 - 220, by, 220, 48, "B", "Cancel");
    screen.touchButtons.push_back({kUnownCancelTouch, x + w - 24 - 220, by, 220, 48});
}

bool activateUnifiedField(TrainerViewScreen& screen, State& state, const Unified::Focus& focus) {
    auto& p = state.working;
    if (focus.panel == Unified::Panel::Details) {
        if (focus.row == 0) {
            mirrorPickerFocus(state, focus);
            return false;
        }
        if (focus.row == 1) {
            const auto result = Utils::promptText("Nickname", "1-10 Generation II characters", p.nickname, 10);
            if (result.accepted) p.nickname = result.text;
            return true;
        }
        if (focus.row == 2) {
            const auto result = Utils::promptNumber("Generation II Level", p.level, 1, 100);
            if (!result.accepted) return true;
            const uint8_t requested = static_cast<uint8_t>(result.value);
            if (screen.sourceGameId == "crystal_gbc") {
                const auto caught = Gen2Native::decodeCrystalCaughtData(p.caughtData);
                if (caught.present && caught.levelCode >= 2 && requested < caught.levelCode) {
                    screen.postStatus("Current level cannot be below Crystal met level", 300);
                    return true;
                }
            }
            state.setLevel(requested);
            return true;
        }
        if (focus.row == 3) {
            const auto result = Utils::promptNumber("Generation II Experience",
                static_cast<int>(p.experience), 0,
                static_cast<int>(state.maximumExperience()));
            if (!result.accepted) return true;
            const uint32_t experience = static_cast<uint32_t>(result.value);
            const uint8_t requestedLevel = Pokemon::getLevelFromExp(
                experience, Pokemon::getGrowthRate(p.species));
            if (screen.sourceGameId == "crystal_gbc") {
                const auto caught = Gen2Native::decodeCrystalCaughtData(p.caughtData);
                if (caught.present && caught.levelCode >= 2 && requestedLevel < caught.levelCode) {
                    screen.postStatus("Experience would put current level below Crystal met level", 300);
                    return true;
                }
            }
            state.setExperience(experience);
            return true;
        }
        if (focus.row == 4) {
            const auto result = Utils::promptText("OT Name", "1-10 Generation II characters",
                                                  p.originalTrainer, 10);
            if (result.accepted) p.originalTrainer = result.text;
            return true;
        }
        if (focus.row == 5) {
            const auto result = Utils::promptNumber("Trainer ID", p.trainerId, 0, 65535);
            if (result.accepted) p.trainerId = static_cast<uint16_t>(result.value);
            return true;
        }
        if (focus.row == 6) {
            openHeldItemPicker(state);
            return true;
        }
        if (focus.row == 7) {
            const auto result = Utils::promptNumber("Friendship", p.friendship, 0, 255);
            if (result.accepted) p.friendship = static_cast<uint8_t>(result.value);
            return true;
        }
        if (focus.row == 8) {
            // A cycles None -> Infected -> Cured in place, no popup, with the shared Gen II-V rule:
            // a state the record started in comes back with its exact strain/days.
            p.pokerus = PokeBank::UIModel::SharedEditorPresentation::nextPokerusByte(
                p.pokerus, state.baseline.pokerus);
            return true;
        }
        if (focus.row >= 9 && screen.sourceGameId == "crystal_gbc") {
            auto caught = Gen2Native::decodeCrystalCaughtData(p.caughtData);
            // Time, met-level and OT-gender belong to an existing Crystal caught record.
            // Raw zero means "not recorded"; never manufacture partial history from it.
            if (focus.row == 10 && !caught.present) {
                screen.postStatus("Choose a legitimate Crystal encounter first", 300);
                return true;
            }
            if (focus.row == 9) {
                // A cycles Met Time in place (like Pokerus), no popup. Pending Met Time is
                // editor-only until a legitimate location creates caught history; recorded history
                // offers only the encounter's allowed times and keeps every other caught bit.
                namespace Selector = PokeBank::UIModel::Gen2FieldSelector;
                const uint8_t next = caught.present
                    ? Selector::nextMetTime(caught.timeOfDay, true, crystalAllowedTimeMask(screen, p, caught))
                    : Selector::nextMetTime(state.crystalMetTime, false, 0);
                applyFieldSelector(screen, state, Selector::Field::MetTime, next);
                return true;
            }
            if (focus.row == 10) {
                const auto choices = Encounter::forGameSpecies(screen.sourceGameId, p.species);
                const Encounter::EncounterTemplate* selected = nullptr;
                for (const auto& encounter : choices) {
                    if (encounter.location != caught.location) continue;
                    const bool timeMatches = Encounter::timeAllowed(encounter, caught.timeOfDay);
                    if (caught.levelCode == 1 && encounter.method == Encounter::Method::Egg && timeMatches) {
                        selected = &encounter; break;
                    }
                    if (caught.levelCode >= 2 && encounter.containsLevel(caught.levelCode) && timeMatches) {
                        selected = &encounter; break;
                    }
                    if (!selected && timeMatches) selected = &encounter;
                    if (!selected) selected = &encounter;
                }
                if (!selected) {
                    screen.postStatus("Choose a legitimate Crystal encounter first", 300);
                    return true;
                }
                if (selected->method == Encounter::Method::Egg) {
                    caught.levelCode = 1;
                    screen.postStatus("Egg encounter uses Crystal's native egg marker", 240);
                } else if (selected->fixedLevel()) {
                    caught.levelCode = static_cast<uint8_t>(std::clamp<int>(selected->minLevel, 2, 63));
                    screen.postStatus("This encounter has fixed met level " +
                                      std::to_string(caught.levelCode), 240);
                } else {
                    const int minLevel = std::clamp<int>(selected->minLevel, 2, 63);
                    const int maxLevel = std::clamp<int>(selected->maxLevel, minLevel, 63);
                    const int current = caught.levelCode >= minLevel && caught.levelCode <= maxLevel
                        ? caught.levelCode : minLevel;
                    const auto result = Utils::promptNumber("Met Level for selected Crystal encounter",
                                                             current, minLevel, maxLevel);
                    if (!result.accepted) return true;
                    caught.levelCode = static_cast<uint8_t>(result.value);
                }
            }
            if (focus.row == 11) {
                pickerStateFor(screen).model.openLocation(
                    screen.sourceGameId, p.species, caught.location, caught.levelCode);
                if (pickerStateFor(screen).model.locationCount() == 0) {
                    pickerStateFor(screen).model.close();
                    screen.postStatus("No supported Crystal encounter templates for this species", 300);
                    return true;
                }
                screen.controllerNavigation.reset();
                return true;
            }
            if (focus.row == 12) {
                // NEW-07: A cycles OT Gender in place (like Met Time), no popup. Recorded caught
                // history toggles Male / Female and keeps every other caught bit; a pending value
                // stays editor-only (applyFieldSelector never fabricates caught history).
                namespace Selector = PokeBank::UIModel::Gen2FieldSelector;
                applyFieldSelector(screen, state, Selector::Field::OtGender,
                    static_cast<uint8_t>(Selector::nextOtGender(state.crystalOtGender, caught.present)));
                return true;
            }
            p.caughtData = Gen2Native::encodeCrystalCaughtData(caught);
            return true;
        }
        return true;
    }

    if (focus.panel == Unified::Panel::Values) {
        if (focus.row <= 4) {
            if (focus.column == 0) {
                if (focus.row == 0) {
                    screen.postStatus("HP DV is derived from Attack, Defense, Speed and Special DVs", 260);
                    return true;
                }
                auto dvs = editableDVs(p);
                const int index = static_cast<int>(focus.row) - 1;
                const auto result = Utils::promptNumber("DV (0-15)", dvs[static_cast<std::size_t>(index)], 0, 15);
                if (result.accepted) {
                    dvs[static_cast<std::size_t>(index)] = static_cast<uint8_t>(result.value);
                    applyEditableDVs(p, dvs);
                }
                return true;
            }
            if (focus.column == 1) {
                auto statExp = p.statExperience;
                const auto result = Utils::promptNumber("Stat Experience (0-65535)",
                    statExp[static_cast<std::size_t>(focus.row)], 0, 65535);
                if (result.accepted) {
                    statExp[static_cast<std::size_t>(focus.row)] = static_cast<uint16_t>(result.value);
                    p.statExperience = statExp;
                }
                return true;
            }
            screen.postStatus("Calculated battle stats are read-only; edit DVs or Stat Exp instead", 280);
            return true;
        }
        namespace Letter = PokeBank::UIModel::Gen2UnownLetter;
        const bool unown = p.species == Letter::kUnownSpecies;
        if (focus.row == 5) {
            // Round 7: Shiny on a letter that cannot be shiny asks before it changes the letter.
            if (unown) {
                const auto request = Letter::requestShinyToggle(editableDVs(p));
                if (request.ask) {
                    unifiedStateFor(screen).unownQuestion = request.question;
                    screen.controllerNavigation.reset();
                    return true;
                }
            }
            toggleShiny(screen, p);
            return true;
        }
        // Unown is genderless; its row 6 is the Letter picker (the letter follows the DVs).
        if (unown) {
            unifiedStateFor(screen).unownPicker = Letter::openPicker(editableDVs(p));
            screen.controllerNavigation.reset();
            return true;
        }
        chooseGender(screen, p);
        return true;
    }

    // Move rows share the accepted Gen I interaction: A opens Move N first.
    // The Choose Move picker is a child of that editor, never the box Empty Slot action sheet.
    beginUnifiedMoveEditor(screen, state, static_cast<int>(focus.row));
    return true;
}

// Shared Gen II DETAILS uses true clipped finger scrolling. Gestures do not
// move a focus cursor; a clean tap on a visible row explicitly selects it.
int updateUnifiedDetailsTouch(TrainerViewScreen& screen, UnifiedWorkspaceState& unified,
                              const TouchInput& touch, uint64_t down) {
    constexpr int kRowsVisible = 8, kRowHeight = 38;
    // Read hit geometry from the actual last rendered left panel. Gen II
    // has both a modal shell and an edge-to-edge hardware shell; hardcoded
    // modal coordinates miss Held Item on the latter.
    if (unified.detailsTouchW <= 0 || unified.detailsTouchH <= 0) return -1;
    const int count = unified.crystal ? 13 : 9;
    unified.detailsViewport.ensure(unified.focus.row, count, kRowsVisible);
    const int maximum = TouchPickerViewport::maxFirst(count, kRowsVisible);
    // Shared scrollbar first: a held thumb moves the Details viewport (not the focused field)
    // and owns the frame's touch, so no row under the finger is dragged or tapped.
    const auto detailsBar = SharedScrollbar::handleTouch(unified.detailsScrollbar, SharedScrollbar::sample(touch),
        SharedScrollbar::rows(count, kRowsVisible, unified.detailsViewport.firstRow));
    if (detailsBar.changed) {
        unified.detailsViewport.firstRow = detailsBar.scroll;
        unified.detailsTouchScroll.stop();
    }
    if (detailsBar.consumed)
        unified.detailsTouchScroll.stop();
    else
        unified.detailsTouchScroll.updateVertical(
            touch, unified.detailsTouchX, unified.detailsTouchY,
            unified.detailsTouchW, unified.detailsTouchH, kRowHeight,
            unified.detailsViewport.firstRow, maximum + 1);
    if (down & (HidNpadButton_Up | HidNpadButton_Down |
                HidNpadButton_L | HidNpadButton_R))
        unified.detailsTouchScroll.stop();
    const int hit = screen.touchedButtonId(touch);
    return !detailsBar.consumed && hit >= 1700 && hit < 1700 + count ? hit - 1700 : -1;
}

bool handleUnifiedWorkspaceInput(TrainerViewScreen& screen, uint64_t down, uint64_t held,
                                 int stickX, int stickY, const TouchInput& touch) {
    auto& state = stateFor(screen);
    if (state.mode == Mode::None) return false;
    auto& unified = unifiedStateFor(screen);

    if (state.mode == Mode::View) {
        const int touchedDetail = updateUnifiedDetailsTouch(screen, unified, touch, down);
        if (touchedDetail >= 0)
            unified.focus = {Unified::Panel::Details, static_cast<uint8_t>(touchedDetail), 0};
        if (PokeBank::UIModel::passiveViewAction(down & HidNpadButton_B, screen.touchedButtonId(touch)) ==
            PokeBank::UIModel::PassiveViewAction::Back) {
            closeToActions(screen);
            resetUnifiedFocus(screen);
            return true;
        }
        const uint64_t nav = screen.controllerNavigation.apply(
            down, held, stickX, stickY,
            HidNpadButton_Up, HidNpadButton_Down, HidNpadButton_Left, HidNpadButton_Right);
        if (down & HidNpadButton_L)
            unified.focus = Unified::passiveViewSwitchPanel(Unified::Generation::Gen2, unified.focus, -1, unified.crystal);
        if (down & HidNpadButton_R)
            unified.focus = Unified::passiveViewSwitchPanel(Unified::Generation::Gen2, unified.focus, 1, unified.crystal);
        if (nav & HidNpadButton_Up)
            unified.focus = Unified::passiveViewMoveVertical(Unified::Generation::Gen2, unified.focus, -1, unified.crystal);
        if (nav & HidNpadButton_Down)
            unified.focus = Unified::passiveViewMoveVertical(Unified::Generation::Gen2, unified.focus, 1, unified.crystal);
        if (nav & HidNpadButton_Left)
            unified.focus = Unified::passiveViewMoveColumn(Unified::Generation::Gen2, unified.focus, -1, unified.crystal);
        if (nav & HidNpadButton_Right)
            unified.focus = Unified::passiveViewMoveColumn(Unified::Generation::Gen2, unified.focus, 1, unified.crystal);
        unified.detailsFocus = Unified::detailsScrollFocus(unified.focus, unified.detailsFocus);
        if (nav & (HidNpadButton_Up | HidNpadButton_Down) ||
            down & (HidNpadButton_L | HidNpadButton_R))
            unified.detailsViewport.reveal(unified.focus.row,
                                           unified.crystal ? 13 : 9, 8);
        return true; // A/Y/X never mutate a View session.
    }

    // Exactly one layer owns this frame (Gen2WorkspaceLayers::topLayer), highest first.
    namespace Layers = PokeBank::UIModel::Gen2WorkspaceLayers;
    const auto layer = workspaceTopLayer(screen);
    if (layer == Layers::Layer::Picker) return false; // the picker overlay handles it next
    if (layer == Layers::Layer::HeldItemPicker)
        return handleItemPicker(screen, state, down, held, stickX, stickY, touch);
    if (layer == Layers::Layer::ExitConfirm) return handleConfirm(screen, state, down);
    if (layer == Layers::Layer::ProgressionEditor)
        return handleProgressionEditor(screen, down, held, stickX, stickY);
    if (layer == Layers::Layer::MoveEditor)
        return handleUnifiedMoveEditor(screen, state, down, held, stickX, stickY);

    if (layer == Layers::Layer::UnownQuestion) return handleUnownQuestion(screen, state, down, touch);
    if (layer == Layers::Layer::UnownPicker)
        return handleUnownPicker(screen, state, down, held, stickX, stickY, touch);

    if (layer == Layers::Layer::FieldSelector) {
        // The field selector owns all input while open: Up/Down move, A or a tap applies, B
        // cancels; the field keeps focus either way.
        namespace Selector = PokeBank::UIModel::Gen2FieldSelector;
        const uint64_t selectorNav = screen.controllerNavigation.apply(
            down, held, stickX, stickY,
            HidNpadButton_Up, HidNpadButton_Down, HidNpadButton_Left, HidNpadButton_Right);
        const int tapped = screen.touchedButtonId(touch);
        const int tappedRow = tapped >= kSelectorTouchBase && tapped < kSelectorTouchBase + unified.selector.count
            ? tapped - kSelectorTouchBase : -1;
        const auto field = unified.selector.field;
        const auto result = Selector::step(unified.selector, (selectorNav & HidNpadButton_Up) != 0,
            (selectorNav & HidNpadButton_Down) != 0, (down & HidNpadButton_A) != 0,
            (down & HidNpadButton_B) != 0, tappedRow);
        if (result.outcome == Selector::Outcome::Apply) applyFieldSelector(screen, state, field, result.value);
        if (result.outcome == Selector::Outcome::Apply || result.outcome == Selector::Outcome::Cancelled)
            screen.controllerNavigation.reset();
        return true;
    }

    // Round 6 (P1-RETURN): + opens the shared Review from Create / Edit (Y stays Random DVs). The
    // workspace stays open underneath; B in Review returns here with the draft and focus intact.
    {
        namespace Entry = PokeBank::UIModel::ReviewEntry;
        if ((down & HidNpadButton_Plus) &&
            Entry::plusOpensReview(state.mode == Mode::Create ? Entry::Screen::Create : Entry::Screen::Edit, 2)) {
            if (!screen.openStagedReview(false))
                screen.postStatus("No staged Generation II changes to review", 220);
            return true;
        }
    }

    // Y Random DVs re-rolls this draft at once (Gen12DVReroll.h): every press is a new roll that
    // keeps the shiny state, the gender and the Unown letter. Nothing is staged until Keep/Add.
    // Round 7 (B07): Unown keeps its letter and shiny state, so its sprite does not change; the
    // status names what was kept and the DVs that changed (or exactly why none could).
    const auto rerollBefore = editableDVs(state.working);
    const bool rerollUnown = state.working.species == PokeBank::UIModel::Gen2UnownLetter::kUnownSpecies;
    if (down & HidNpadButton_Y) {
        if (randomizeDVs(state))
            screen.postStatus(rerollUnown
                ? PokeBank::UIModel::Gen2UnownLetter::rerollStatus(rerollBefore, editableDVs(state.working), true)
                : std::string("Generation II DVs re-rolled in this draft; Keep/Add stages them"),
                rerollUnown ? 360 : 240);
        else if (rerollUnown)
            screen.postStatus(PokeBank::UIModel::Gen2UnownLetter::rerollStatus(rerollBefore, rerollBefore, false), 300);
        else
            screen.postStatus(state.working.shiny
                ? "No other shiny DVs keep this Pokemon's gender; DVs unchanged"
                : "No other DVs keep this Pokemon's gender and form; DVs unchanged", 300);
        return true;
    }

    const int touchedDetail = updateUnifiedDetailsTouch(screen, unified, touch, down);
    if (touchedDetail >= 0) {
        unified.focus = {Unified::Panel::Details, static_cast<uint8_t>(touchedDetail), 0};
        unified.detailsFocus = static_cast<std::size_t>(touchedDetail);
        mirrorPickerFocus(state, unified.focus);
        // Species (row 0) is a picker field: a completed tap opens it exactly like A does.
        if (!activateUnifiedField(screen, state, unified.focus))
            (void)openPickerForFocusedField(screen, state, pickerStateFor(screen), HidNpadButton_A);
        return true; // the modal always consumes a row tap, even for inert fields
    }

    const uint64_t nav = screen.controllerNavigation.apply(
        down, held, stickX, stickY,
        HidNpadButton_Up, HidNpadButton_Down, HidNpadButton_Left, HidNpadButton_Right);
    if (down & HidNpadButton_L) unified.focus = Unified::switchPanel(Unified::Generation::Gen2, unified.focus, -1, unified.crystal);
    if (down & HidNpadButton_R) unified.focus = Unified::switchPanel(Unified::Generation::Gen2, unified.focus, 1, unified.crystal);
    if (nav & HidNpadButton_Up) unified.focus = Unified::moveVertical(Unified::Generation::Gen2, unified.focus, -1, unified.crystal);
    if (nav & HidNpadButton_Down) unified.focus = Unified::moveVertical(Unified::Generation::Gen2, unified.focus, 1, unified.crystal);
    if (nav & HidNpadButton_Left) unified.focus = Unified::moveRowColumn(Unified::Generation::Gen2, unified.focus, -1, unified.crystal);
    if (nav & HidNpadButton_Right) unified.focus = Unified::moveRowColumn(Unified::Generation::Gen2, unified.focus, 1, unified.crystal);

    unified.focus = Unified::normalizeMoveRowFocus(Unified::Generation::Gen2, unified.focus, unified.crystal);
    unified.detailsFocus = Unified::detailsScrollFocus(unified.focus, unified.detailsFocus);
    if (nav & (HidNpadButton_Up | HidNpadButton_Down) ||
        down & (HidNpadButton_L | HidNpadButton_R))
        unified.detailsViewport.reveal(unified.focus.row,
                                       unified.crystal ? 13 : 9, 8);

    if ((down & HidNpadButton_X) && state.mode == Mode::Create) {
        if (stageCreate(screen)) resetUnifiedFocus(screen);
        return true;
    }
    // X Keep, as in the Gen III-V shared editor: stage this edit draft and return to Actions
    // (the same commit as the exit dialog's A Save). Nothing is staged before this.
    if ((down & HidNpadButton_X) && state.mode == Mode::Edit) {
        if (stageEdit(screen)) resetUnifiedFocus(screen);
        return true;
    }
    if (down & HidNpadButton_B) {
        if (state.back()) {
            closeToActions(screen);
            resetUnifiedFocus(screen);
        } else {
            // Create/Edit now always stop here for an explicit Add/Keep, Discard,
            // or Continue decision. No local draft is lost on a stray Back press.
            screen.controllerNavigation.reset();
        }
        return true;
    }
    if (!(down & HidNpadButton_A)) return true;
    // A following a swipe must never activate an off-screen Held Item or
    // another previously selected field. Reveal it without editing first.
    if (unified.focus.panel == Unified::Panel::Details &&
        !unified.detailsViewport.containsSelection(
            unified.focus.row, unified.crystal ? 13 : 9, 8)) {
        unified.detailsTouchScroll.stop();
        unified.detailsViewport.reveal(
            unified.focus.row, unified.crystal ? 13 : 9, 8);
        screen.postStatus("Focused field brought into view; press A again", 200);
        return true;
    }

    mirrorPickerFocus(state, unified.focus);
    if (!activateUnifiedField(screen, state, unified.focus))
        return false;
    return true;
}

Color unifiedStatusColor(Rules::CompatibilityState status) {
    if (status == Rules::CompatibilityState::Ok) return Colors::Success;
    if (status == Rules::CompatibilityState::UnusualPreserved) return Colors::Warning;
    return Colors::Error;
}

// R2-GOLD: the classic STATS grid on the shared field-state contract (UI/ClassicStatFieldState.h).
inline Color gen2StatCellColor(int row, int column) {
    return UI::SharedEditorChrome::fieldColor(UI::SharedEditorChrome::FieldState::style(
        PokeBank::UIModel::ClassicStatFields::cellState(row, column), false).value);
}
// Unown's letter as shown on the Values Letter row: computed from the DVs every frame, so it can
// never disagree with the sprite (the draft form) or the stored record.
inline std::string gen2UnownLetterText(const PokeVault::Integration::Gen2::PokemonRecord& p) {
    return std::string(1, PokeBank::UIModel::Gen2UnownLetter::letterChar(
        PokeBank::UIModel::Gen12DVReroll::unownLetter({p.dvs[1], p.dvs[2], p.dvs[3], p.dvs[4]})));
}
inline std::string gen2UnownLetterNote() {
    return "Letter follows DVs; only " + PokeBank::UIModel::Gen2UnownLetter::shinyLettersText() +
           " can be shiny";
}
// The HP DV shown is always the one derived from the four stored DVs.
inline uint8_t gen2ShownHpDv(const PokeVault::Integration::Gen2::PokemonRecord& p) {
    return PokeBank::UIModel::ClassicStatFields::derivedHpDv({p.dvs[1], p.dvs[2], p.dvs[3], p.dvs[4]});
}

// Round 6 7A: the STATS Calculated column for every Gen II painter (Gen2Workspace::StatsGrid).
// Rows 0-3 and the Special row are on the grid; SpD is the continuation row directly beneath
// SpA. Numbers are right-aligned in one column; the derived Special values carry matching tags.
void drawGen2CalculatedColumn(PKSEFramebuffer& fb, const std::array<uint16_t, 6>& battle,
                              int x, int y, int w) {
    namespace W = PokeBank::UIModel::Gen2Workspace;
    for (int row = 0; row <= W::kSpecialDefenseRow; ++row) {
        const auto cell = W::calculatedCell(battle, row);
        const int yy = y + W::StatsGrid::rowY(row);
        int vw = 0, vh = 0;
        fb.measureText(cell.value, vw, vh, TextStyle::Caption);
        if (!cell.tag.empty()) {
            int tw = 0, th = 0;
            fb.measureText(cell.tag, tw, th, TextStyle::Caption);
            fb.drawText(x + W::StatsGrid::tagX(w, vw, tw), yy, cell.tag, Colors::TextDim, TextStyle::Caption);
        }
        fb.drawText(x + w - W::StatsGrid::kRightInset - vw, yy, cell.value,
                    gen2StatCellColor(std::min(row, 4), 2), TextStyle::Caption);
    }
}

void drawUnifiedValues(PKSEFramebuffer& fb, const State& state, const UnifiedWorkspaceState& unified,
                       int x, int y, int w, int h, bool passive) {
    drawPanelSurface(fb, x, y, w, h, false, 14);
    fb.drawText(x + 14, y + 10, Unified::statsHeading(),
        !passive && unified.focus.panel == Unified::Panel::Values ? Colors::Accent : Colors::Text,
        TextStyle::Heading);
    fb.drawText(x + 114, y + 42, "DV", Colors::TextDim, TextStyle::Caption);
    fb.drawText(x + 184, y + 42, "Stat Exp", Colors::TextDim, TextStyle::Caption);
    fb.drawText(x + 292, y + 42, "Calculated", Colors::TextDim, TextStyle::Caption);

    const auto& p = state.working;
    const auto battle = PokeBank::UIModel::Gen2Workspace::battleStats(p,state.mode!=Mode::View);
    const std::array<const char*, 5> labels{"HP", "Attack", "Defense", "Speed", "Special"};
    const std::array<uint8_t, 5> dvs{gen2ShownHpDv(p), p.dvs[1], p.dvs[2], p.dvs[3], p.dvs[4]};
    for (int r = 0; r < 5; ++r) {
        const int yy = y + 70 + r * 34;
        const bool rowSelected = !passive && unified.focus.panel == Unified::Panel::Values && unified.focus.row == r;
        if (rowSelected) {
            const auto cell = Unified::cellFocus(unified.focus);
            fb.drawRoundedRect(x + cell.x, yy - 5, cell.width, 28, 6, Colors::Accent, 2);
        }
        fb.drawText(x + 18, yy, labels[static_cast<std::size_t>(r)], rowSelected ? Colors::SelectedText : Colors::TextDim, TextStyle::Caption);
        fb.drawText(x + 114, yy, std::to_string(dvs[static_cast<std::size_t>(r)]) + (r == 0 ? " *" : ""),
                    gen2StatCellColor(r, 0), TextStyle::Caption);
        fb.drawText(x + 184, yy, std::to_string(p.statExperience[static_cast<std::size_t>(r)]),
                    gen2StatCellColor(r, 1), TextStyle::Caption);
    }
    // One real editable Special DV/Stat Exp row; both computed Special stats stay in the one
    // Calculated column, SpD on the next grid row under SpA (no fake second DV row).
    drawGen2CalculatedColumn(fb, battle, x, y, w);
    fb.drawText(x + 18, y + 291, "Special DV / Stat Exp shared by SpA and SpD",
                Colors::TextDim, TextStyle::Caption);
    fb.drawText(x + 18, y + 328, "* HP DV derived / read-only", Colors::TextDim, TextStyle::Caption);

    const bool unown = p.species == PokeBank::UIModel::Gen2UnownLetter::kUnownSpecies;
    const std::array<std::pair<std::string, std::string>, 2> capabilityRows{{
        {"Shiny", p.shiny ? "Yes" : "No"},
        unown ? std::pair<std::string, std::string>{"Letter", gen2UnownLetterText(p)}
              : std::pair<std::string, std::string>{"Gender", genderText(p.gender)},
    }};
    for (int i = 0; i < 2; ++i) {
        const int row = 5 + i;
        const int yy = y + 350 + i * 42;
        const bool selected = !passive && unified.focus.panel == Unified::Panel::Values && unified.focus.row == row;
        if (selected) {
            const auto cell = Unified::cellFocus(unified.focus);
            fb.drawRoundedRect(x + cell.x, yy - 5, cell.width, 28, 6, Colors::Accent, 2);
        }
        fb.drawText(x + 18, yy, capabilityRows[static_cast<std::size_t>(i)].first, selected ? Colors::SelectedText : Colors::TextDim, TextStyle::Caption);
        fb.drawText(x + 134, yy, capabilityRows[static_cast<std::size_t>(i)].second,
                    ::UI::ClassicFieldColor::shinyValueColor(selected),
                    TextStyle::Caption);
    }
}


void drawUnifiedMoveEditor(TrainerViewScreen& screen, PKSEFramebuffer& fb,
                           State& state, const UnifiedWorkspaceState& unified) {
    if (!unified.moveEditor) return;
    const auto slot = static_cast<std::size_t>(unified.moveEditorSlot);
    constexpr int w = 650, h = 390, navH = 48;
    const int x = (fb.getWidth() - w) / 2;
    const int y = (fb.getHeight() - navH - h) / 2;
    fb.drawFilledRect(0, 0, fb.getWidth(), fb.getHeight() - navH, Color(0, 0, 0, 95));
    drawPanelSurface(fb, x, y, w, h, false, 18);
    fb.drawRoundedRect(x, y, w, h, 18, Colors::Divider, 1);
    fb.drawText(x + 24, y + 18, "Move " + std::to_string(unified.moveEditorSlot + 1),
                Colors::Text, TextStyle::Heading);
    fb.drawText(x + 24, y + 52, "Contextual editor — B always cancels this dialog",
                Colors::TextDim, TextStyle::Caption);

    const uint16_t move = state.working.moves[slot];
    const std::array<std::pair<std::string, std::string>, 5> rows{{
        {"Move", move == 0 ? std::string("Empty") : std::string(Names::getMoveName(move))},
        {"PP", std::to_string(state.working.pp[slot]) + "/" +
               std::to_string(state.maximumPP(unified.moveEditorSlot))},
        {"PP Ups", std::to_string(state.working.ppUps[slot])},
        {"Apply", "Keep these move details"},
        {"Cancel", "Discard these move details"},
    }};
    int rowY = y + 88;
    for (int i = 0; i < static_cast<int>(rows.size()); ++i) {
        const bool selected = i == unified.moveEditorRow;
        SharedBoxedRows::drawBox(fb, SharedBoxedRows::rect(x + 22, rowY, w - 44, 42), {selected});  // R2-BUG-012 boxed row
        fb.drawText(x + 38, rowY + 10, rows[static_cast<std::size_t>(i)].first,
                    selected ? Colors::Text : Colors::TextDim);
        int valueW = 0, valueH = 0;
        fb.measureText(rows[static_cast<std::size_t>(i)].second, valueW, valueH, TextStyle::Body);
        fb.drawText(x + w - 38 - valueW, rowY + 10, rows[static_cast<std::size_t>(i)].second,
                    selected ? Colors::Text : Colors::TextDim);
        rowY += 48;
    }
    drawNavBar(fb, {{"D-pad/Stick", "Navigate"}, {"A", "Edit / Apply"}, {"B", "Cancel"}});
}

void drawUnifiedDetails(TrainerViewScreen& screen, PKSEFramebuffer& fb, const State& state,
                        UnifiedWorkspaceState& unified,
                        int x, int y, int w, int h, bool passive) {
    drawPanelSurface(fb, x, y, w, h, false, 14);
    fb.drawText(x + 14, y + 10, "DETAILS",
        !passive && unified.focus.panel == Unified::Panel::Details ? Colors::Accent : Colors::Text,
        TextStyle::Caption);
    const auto& p = state.working;
    // The portrait letter is computed from the current DVs on every frame (never a cached form).
    SharedPokemonShell::drawPortrait(fb, x, y, p.species, p.shiny,
        p.species == 201 ? static_cast<uint8_t>(PokeBank::UIModel::Gen12DVReroll::unownLetter(
                               {p.dvs[1], p.dvs[2], p.dvs[3], p.dvs[4]})) : uint8_t{0});
    if (const auto* personal = Gen2::personalRecord(p.species)) {
        int badgeX = x + 18;
        sharedBadge(fb, badgeX, y + 184, sharedTypeName(personal->rawType1));
        if (personal->rawType2 != personal->rawType1) {
            int tw = 0, th = 0;
            fb.measureText(sharedTypeName(personal->rawType1), tw, th, TextStyle::Caption);
            badgeX += std::max(64, tw + 24) + 8;
            sharedBadge(fb, badgeX, y + 184, sharedTypeName(personal->rawType2));
        }
    }
    std::vector<std::pair<std::string, std::string>> rows{
        {"Species", std::string(Names::getSpeciesName(p.species)) + " #" + std::to_string(p.species)},
        {"Nickname", p.nickname},
        {"Level", std::to_string(p.level)},
        {"Experience", std::to_string(p.experience)},
        {"OT Name", p.originalTrainer},
        {"Trainer ID", std::to_string(p.trainerId)},
        {"Held Item", itemText(p.heldItem)},
        {"Friendship", std::to_string(p.friendship)},
        {"Pokerus", Gen2Native::pokerusText(p.pokerus)},
    };
    if (unified.crystal) {
        const auto caught = Gen2Native::decodeCrystalCaughtData(p.caughtData);
        const uint8_t displayedMetTime = state.mode == Mode::View
            ? caught.timeOfDay
            : state.crystalMetTime;
        rows.push_back({"Met time", Gen2Native::crystalMetTimeName(displayedMetTime)});
        rows.push_back({"Met level", Gen2Native::crystalCaughtLevelText(caught)});
        rows.push_back({"Location", Gen2Native::crystalCaughtLocationName(caught.location)});
        const auto displayedOtGender = state.mode == Mode::View
            ? Gen2Native::crystalOtGenderChoiceFromCaught(caught)
            : state.crystalOtGender;
        rows.push_back({"OT gender", Gen2Native::crystalOtGenderChoiceText(displayedOtGender)});
    }

    constexpr int visible = 8, rowHeight = 38;
    const int count = static_cast<int>(rows.size());
    unified.detailsViewport.ensure(unified.focus.row, count, visible);
    const int first = unified.detailsViewport.firstRow;
    const int top = y + 224;
    unified.detailsTouchX = x + 8;
    unified.detailsTouchY = top;
    unified.detailsTouchW = w - 16;
    unified.detailsTouchH = visible * rowHeight;
    const int offset = unified.detailsTouchScroll.offset();
    fb.setClipRect(x + 8, top, w - 16, visible * rowHeight);
    for (int row = std::max(0, first - 1); row < std::min(count, first + visible + 1); ++row) {
        const int yy = top + (row - first) * rowHeight + offset;
        const bool selected = unified.focus.panel == Unified::Panel::Details && unified.focus.row == row;
        if (selected) {
            const auto cell = Unified::cellFocus(unified.focus);
            fb.drawRoundedRect(x + cell.x, yy + 2, cell.width, 30, 6, Colors::Accent, 2);
        }
        fb.drawText(x + 18, yy + 7, rows[static_cast<std::size_t>(row)].first,
                    selected ? Colors::SelectedText : Colors::TextDim, TextStyle::Caption);
        fb.drawText(x + 108, yy + 7, rows[static_cast<std::size_t>(row)].second,
                    Colors::Text, TextStyle::Caption);
        appendClippedTouchButton(screen.touchButtons, 1700 + row,
            x + 8, yy, w - 16, rowHeight - 2,
            x + 8, top, w - 16, visible * rowHeight);
    }
    fb.clearClip();
    // Same track geometry as the hardware repaint (Gen2HardwareFinalFix.inc) so the two draws
    // of this list share one thumb; this call publishes it for next frame's hit-test.
    SharedScrollbar::drawAndPublish(fb, unified.detailsScrollbar, x + w - 10, y + 222,
        SharedScrollbar::kTrackWidth, std::max(0, h - 262),
        SharedScrollbar::rows(rows.size(), static_cast<std::size_t>(visible),
                              static_cast<std::size_t>(first)));
}

std::array<UI::SharedMovesPanel::Slot, 4> gen2MoveSlots(const TrainerViewScreen& screen, const State& state) {
    std::array<UI::Gen2MovesPanel::Status, 4> status{};
    for (int slot = 0; slot < 4; ++slot) {
        const auto compatibility = moveStatus(screen, state, slot);
        status[static_cast<std::size_t>(slot)] =
            compatibility == Rules::CompatibilityState::Ok ? UI::Gen2MovesPanel::Status::Ok :
            compatibility == Rules::CompatibilityState::UnusualPreserved ? UI::Gen2MovesPanel::Status::Preserved :
            UI::Gen2MovesPanel::Status::Invalid;
    }
    return UI::Gen2MovesPanel::slots(state.working.moves, state.working.pp, state.working.ppUps, status);
}

// The ONE shared Moves panel (UI/SharedMovesPanel.h via UI/Gen2MovesPanel.h), same rows as
// Gen III-V. Origin & Legality is a row of the GEN II DATA pane below it.
void drawGen2SharedMoveRows(PKSEFramebuffer& fb, const TrainerViewScreen& screen, const State& state,
                            const UnifiedWorkspaceState& unified, int x, int y, int w, bool passive) {
    const auto visibleMoveFocus = Unified::normalizeMoveRowFocus(
        Unified::Generation::Gen2, unified.focus, unified.crystal);
    UI::Gen2MovesPanel::draw(fb, x, y, w, gen2MoveSlots(screen, state),
        !passive && visibleMoveFocus.panel == Unified::Panel::Moves ? static_cast<int>(visibleMoveFocus.row) : -1);
}

void drawUnifiedMoves(PKSEFramebuffer& fb, const TrainerViewScreen& screen, const State& state,
                      const UnifiedWorkspaceState& unified, int x, int y, int w, int h, bool passive) {
    drawPanelSurface(fb, x, y, w, h, false, 14);
    fb.drawText(x + 14, y + 10, "MOVES",
        !passive && unified.focus.panel == Unified::Panel::Moves ? Colors::Accent : Colors::Text,
        TextStyle::Heading);
    drawGen2SharedMoveRows(fb, screen, state, unified, x, y, w, passive);
    if (const auto game = Rules::exactGameFromSourceId(screen.sourceGameId))
        Gen2WorkspacePresentation::drawDataAndGraph(fb, x, y + UI::SharedMovesPanel::kDataPaneOffset,
                                                   w, h - 256, state.working, *game);
}

void drawProgressionEditor(PKSEFramebuffer& fb, const State& state, const UnifiedWorkspaceState& unified) {
    constexpr int x = 320, y = 168, w = 640, h = 330;
    fb.drawFilledRect(0, 0, fb.getWidth(), fb.getHeight() - 48, Color(0, 0, 0, 150));
    drawPanelSurface(fb, x, y, w, h, false, 18);
    fb.drawText(x + 24, y + 18, "Level / Experience", Colors::Text, TextStyle::Heading);
    fb.drawText(x + 24, y + 52, "Contextual editor — changes remain local until Add / Keep",
                Colors::TextDim, TextStyle::Caption);
    const std::array<std::pair<std::string, std::string>, 4> rows{{
        {"Level", std::to_string(state.working.level)},
        {"Experience", std::to_string(state.working.experience)},
        {"Apply", "Return to shared Pokemon editor"},
        {"Cancel", "Restore values from before this dialog"},
    }};
    for (int i = 0; i < 4; ++i) {
        const int yy = y + 96 + i * 48;
        SharedBoxedRows::drawBox(fb, SharedBoxedRows::rect(x + 22, yy - 5, w - 44, 40), {unified.progressionRow == i});  // R2-BUG-012 boxed row
        fb.drawText(x + 38, yy + 5, rows[static_cast<std::size_t>(i)].first,
                    unified.progressionRow == i ? Colors::Text : Colors::TextDim, TextStyle::Caption);
        fb.drawText(x + 250, yy + 5, rows[static_cast<std::size_t>(i)].second, Colors::Text, TextStyle::Caption);
    }
    drawNavBar(fb, {{"D-pad/Stick", "Field"}, {"A", "Edit/Apply"}, {"B", "Cancel"}});
}

void drawUnifiedWorkspace(TrainerViewScreen& screen, PKSEFramebuffer& fb) {
    auto& state = stateFor(screen);
    auto& unified = unifiedStateFor(screen);
    const bool view = state.mode == Mode::View;
    const bool create = state.mode == Mode::Create;
    const auto g = Unified::geometry720p();
    screen.touchButtons.clear();
    fb.drawFilledRect(0, 0, fb.getWidth(), fb.getHeight() - 48, Color(0, 0, 0, 105));
    drawPanelSurface(fb, g.x, g.y, g.width, g.height, false, 18);
    const auto surface = view ? Unified::Surface::View : create ? Unified::Surface::CreateDraft : Unified::Surface::Edit;
    fb.drawText(g.x + 24, g.y + 14, Unified::titleFor(surface), Colors::Text, TextStyle::Heading);
    std::string subtitle = "Box " + std::to_string(state.box + 1);
    if (state.slot >= 0) subtitle += " / Slot " + std::to_string(state.slot + 1);
    subtitle += "   •   "; subtitle += sharedGameName(screen); subtitle += "   •   Source save immutable";
    fb.drawText(g.x + 24, g.y + 46, subtitle, Colors::TextDim, TextStyle::Caption);

    const int contentY = g.y + 74;
    const int contentH = g.height - 94;
    const int leftX = g.x + 18, leftW = 300;
    const int midX = leftX + leftW + 14, midW = 398;
    const int rightX = midX + midW + 14, rightW = g.x + g.width - 18 - rightX;
    drawUnifiedDetails(screen, fb, state, unified, leftX, contentY, leftW, contentH, view);
    drawUnifiedValues(fb, state, unified, midX, contentY, midW, contentH, view);
    drawUnifiedMoves(fb, screen, state, unified, rightX, contentY, rightW, contentH, view);

    if (view) {
        screen.touchButtons.push_back({PokeBank::UIModel::passiveBackTarget, 16, fb.getHeight() - 68, 190, 58});
        drawNavBar(fb, {{"D-pad/Stick", "Navigate"}, {"L/R", "Panel"}, {"B", "Back"}});
    } else {
        drawWorkspaceFooter(fb, screen);
    }
    if (state.itemPicker) drawHeldItemPicker(fb, state);
    if (state.confirmExit) drawConfirm(screen, fb, state);
    if (unified.progressionEditor) drawProgressionEditor(fb, state, unified);
}

} // namespace

bool unifiedGen2SurfaceOwnsFrame(const TrainerViewScreen& screen) noexcept {
    return sharedSurfaceOwnsFrame(screen);
}

bool handleUnifiedGen2SurfaceInput(TrainerViewScreen& screen, uint64_t down, uint64_t held,
                                   int stickX, int stickY, const TouchInput& touch) {
    if (!isGSC(screen)) return false;
    if (stateFor(screen).mode != Mode::None)
        return handleUnifiedWorkspaceInput(screen, down, held, stickX, stickY, touch);
    return handleSharedSurfaceInput(screen, down, held, stickX, stickY, touch);
}

bool drawUnifiedGen2Surface(TrainerViewScreen& screen, PKSEFramebuffer& fb) {
    if (!isGSC(screen)) return false;
    if (stateFor(screen).mode != Mode::None) {
        drawUnifiedWorkspace(screen, fb);
        return true;
    }
    return drawSharedSurface(screen, fb);
}

} // namespace UI::Gen2PokemonEditor
