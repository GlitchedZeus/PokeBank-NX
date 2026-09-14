// Named Generation II picker layer. It intercepts only Species, Move and Pokerus activation before
// the preserved shared editor launches its legacy numeric prompts. Browsing lives entirely in this
// overlay model; the Gen2PokemonSession changes only when A explicitly applies a selected value.

#include "UI/Gen2PokemonPickerModel.h"

namespace UI::Gen2PokemonEditor {
namespace {

namespace Gen2Picker = PokeBank::UIModel::Gen2PokemonPicker;
namespace Gen2Native = PokeBank::UIModel::Gen2Native;

struct PickerOverlayState {
    const TrainerViewScreen* owner = nullptr;
    Gen2Picker::Model model;
};

PickerOverlayState& pickerStateFor(const TrainerViewScreen& screen) {
    static PickerOverlayState picker;
    if (picker.owner != &screen) {
        picker = {};
        picker.owner = &screen;
    }
    return picker;
}

void closePicker(TrainerViewScreen& screen, PickerOverlayState& picker) {
    picker.model.close();
    screen.controllerNavigation.reset();
}

bool openPickerForFocusedField(TrainerViewScreen& screen, State& state,
                               PickerOverlayState& picker, uint64_t down) {
    if (!(down & HidNpadButton_A) || state.itemPicker || state.confirmExit) return false;
    if (state.panel == Foundation::Panel::Identity && state.row == 0) {
        picker.model.openSpecies(state.working.species);
    } else if (state.panel == Foundation::Panel::Moves && state.column == 0) {
        picker.model.openMove(state.working.moves[static_cast<std::size_t>(state.row)],
                              static_cast<std::size_t>(state.row));
    } else if (state.panel == Foundation::Panel::Values && state.row == 7) {
        picker.model.openPokerus(state.working.pokerus);
    } else {
        return false;
    }
    screen.controllerNavigation.reset();
    return true;
}

bool applyPickerChoice(TrainerViewScreen& screen, State& state, PickerOverlayState& picker) {
    if (picker.model.kind == Gen2Picker::Kind::Species) {
        const std::string oldDefault = Names::getSpeciesName(state.working.species);
        const bool renameDefault = state.mode == Mode::Create && state.working.nickname == oldDefault;
        if (!Gen2Picker::applySpeciesChoice(state, picker.model.speciesChoice())) return false;
        if (renameDefault) state.working.nickname = Names::getSpeciesName(state.working.species);
        refreshDerived(state.working);
    } else if (picker.model.kind == Gen2Picker::Kind::Move) {
        if (!Gen2Picker::applyMoveChoice(state, picker.model.moveSlot, picker.model.moveChoice())) return false;
    } else if (picker.model.kind == Gen2Picker::Kind::Pokerus) {
        if (!Gen2Picker::applyPokerusChoice(state, picker.model.pokerusRaw())) return false;
    } else {
        return false;
    }
    closePicker(screen, picker);
    return true;
}

const char* movePickerStatus(const TrainerViewScreen& screen, const State& state, uint16_t move) {
    const auto game = Rules::exactGameFromSourceId(screen.sourceGameId);
    if (game && Gen2::MoveCompatibility::canLearnMove(*game, state.working.species, move)) return "Compatible";
    const auto slot = pickerStateFor(screen).model.moveSlot;
    if (state.mode == Mode::Edit && state.working.species == state.baseline.species &&
        slot < state.baseline.moves.size() && state.baseline.moves[slot] == move)
        return "Unusual preserved";
    return "Needs correction";
}

Color movePickerStatusColor(const char* status) {
    const std::string value(status);
    if (value == "Compatible") return Colors::Success;
    if (value == "Unusual preserved") return Colors::Warning;
    return Colors::Error;
}

void drawPickerRow(PKSEFramebuffer& fb, int x, int y, int w, const std::string& text,
                   bool selected, Color textColor = Colors::Text) {
    if (selected) {
        fb.drawFilledRoundedRect(x, y - 5, w, 30, 8, Colors::SurfaceSelected);
        fb.drawRoundedRect(x, y - 5, w, 30, 8, Colors::FocusBorder, 1);
    }
    fb.drawText(x + 12, y, text, selected ? Colors::Accent : textColor, TextStyle::Caption);
}

void drawListPicker(TrainerViewScreen& screen, PKSEFramebuffer& fb, const State& state,
                    const Gen2Picker::Model& model) {
    const int W = fb.getWidth(), H = fb.getHeight();
    constexpr int panelW = 760, panelH = 568;
    const int x = (W - panelW) / 2, y = (H - panelH) / 2;
    fb.drawFilledRect(0, 0, W, H, Color(0, 0, 0, 165));
    drawPanelSurface(fb, x, y, panelW, panelH, false, 18);

    const bool species = model.kind == Gen2Picker::Kind::Species;
    fb.drawText(x + 24, y + 18, species ? "Species — Generation II" : "Move — Generation II",
                Colors::Text, TextStyle::Heading);
    fb.drawText(x + 24, y + 52,
                species ? "#001–#251 • browsing does not change the staged Pokemon"
                        : "Empty + moves 1–251 • compatibility shown for this exact game",
                Colors::TextDim, TextStyle::Caption);

    const int count = species ? 251 : 252;
    constexpr int visible = 13;
    const int first = std::clamp(model.index - visible / 2, 0, std::max(0, count - visible));
    const int last = std::min(count, first + visible);
    for (int i = first; i < last; ++i) {
        const int yy = y + 88 + (i - first) * 33;
        const bool selected = i == model.index;
        if (species) {
            const uint16_t dex = static_cast<uint16_t>(i + 1);
            std::string number = std::to_string(dex);
            while (number.size() < 3) number = "0" + number;
            drawPickerRow(fb, x + 20, yy, panelW - 40,
                          "#" + number + "  " + Names::getSpeciesName(dex), selected);
        } else {
            const uint16_t move = static_cast<uint16_t>(i);
            const std::string name = move == 0 ? "Empty" : std::string(Names::getMoveName(move));
            drawPickerRow(fb, x + 20, yy, panelW - 40, name, selected);
            const char* status = movePickerStatus(screen, state, move);
            int sw = 0, sh = 0;
            fb.measureText(status, sw, sh, TextStyle::Caption);
            fb.drawText(x + panelW - 34 - sw, yy, status, movePickerStatusColor(status), TextStyle::Caption);
        }
    }
    drawNavBar(fb, {{"D-pad/Stick", "Browse"}, {"Left/Right", "Page"}, {"A", "Select"}, {"B", "Cancel"}});
}

void drawPokerusPicker(PKSEFramebuffer& fb, const Gen2Picker::Model& model) {
    const int W = fb.getWidth(), H = fb.getHeight();
    constexpr int panelW = 650, panelH = 380;
    const int x = (W - panelW) / 2, y = (H - panelH) / 2;
    fb.drawFilledRect(0, 0, W, H, Color(0, 0, 0, 165));
    drawPanelSurface(fb, x, y, panelW, panelH, false, 18);
    fb.drawText(x + 24, y + 18, "Pokerus — Generation II", Colors::Text, TextStyle::Heading);
    fb.drawText(x + 24, y + 52, "Edit the native strain/day fields; the stored byte remains authentic.",
                Colors::TextDim, TextStyle::Caption);

    const std::array<std::pair<std::string, std::string>, 3> rows{{
        {"State", Gen2Picker::pokerusModeText(model.pokerusMode)},
        {"Strain", model.pokerusMode == Gen2Picker::PokerusMode::None ? "—" : std::to_string(model.strain)},
        {"Days remaining", model.pokerusMode == Gen2Picker::PokerusMode::Active ? std::to_string(model.days) : "0"},
    }};
    for (int i = 0; i < 3; ++i) {
        const int yy = y + 110 + i * 58;
        drawPickerRow(fb, x + 24, yy, panelW - 48, rows[static_cast<std::size_t>(i)].first,
                      model.pokerusRow == i);
        fb.drawText(x + 270, yy, rows[static_cast<std::size_t>(i)].second,
                    model.pokerusRow == i ? Colors::Accent : Colors::Text, TextStyle::Caption);
    }
    fb.drawText(x + 24, y + 300,
                "Stored byte preview (read-only): " + std::to_string(model.pokerusRaw()),
                Colors::TextDim, TextStyle::Caption);
    drawNavBar(fb, {{"Up/Down", "Field"}, {"Left/Right", "Adjust"}, {"A", "Apply"}, {"B", "Cancel"}});
}

} // namespace

bool handlePickerInput(TrainerViewScreen& screen, uint64_t down, uint64_t held,
                       int stickX, int stickY, const TouchInput& touch) {
    if (!isGSC(screen)) return false;
    auto& state = stateFor(screen);
    auto& picker = pickerStateFor(screen);

    if (state.mode == Mode::View) {
        if (PokeBank::UIModel::passiveViewAction(false, screen.touchedButtonId(touch)) ==
            PokeBank::UIModel::PassiveViewAction::Back) {
            closeToActions(screen);
            return true;
        }
        if (picker.model.active()) picker.model.close();
        return false;
    }
    if (state.mode == Mode::None) {
        if (picker.model.active()) picker.model.close();
        return false;
    }

    if (!picker.model.active())
        return openPickerForFocusedField(screen, state, picker, down);

    const uint64_t nav = screen.controllerNavigation.apply(
        down, held, stickX, stickY,
        HidNpadButton_Up, HidNpadButton_Down, HidNpadButton_Left, HidNpadButton_Right);

    if (down & HidNpadButton_B) {
        closePicker(screen, picker);
        return true;
    }

    if (picker.model.kind == Gen2Picker::Kind::Species ||
        picker.model.kind == Gen2Picker::Kind::Move) {
        if (nav & HidNpadButton_Up) picker.model.stepList(-1);
        if (nav & HidNpadButton_Down) picker.model.stepList(1);
        if (nav & HidNpadButton_Left) picker.model.stepList(-10);
        if (nav & HidNpadButton_Right) picker.model.stepList(10);
    } else if (picker.model.kind == Gen2Picker::Kind::Pokerus) {
        if (nav & HidNpadButton_Up) picker.model.stepPokerusRow(-1);
        if (nav & HidNpadButton_Down) picker.model.stepPokerusRow(1);
        if (nav & HidNpadButton_Left) picker.model.adjustPokerus(-1);
        if (nav & HidNpadButton_Right) picker.model.adjustPokerus(1);
    }

    if (down & HidNpadButton_A) applyPickerChoice(screen, state, picker);
    return true;
}

void drawPickerOverlay(TrainerViewScreen& screen, PKSEFramebuffer& fb) {
    const auto& state = stateFor(screen);
    if (state.mode == Mode::View) {
        screen.touchButtons.push_back({PokeBank::UIModel::passiveBackTarget,
                                       16, fb.getHeight() - 68, 190, 58});
        return;
    }
    const auto& picker = pickerStateFor(screen).model;
    if (!picker.active()) return;
    if (picker.kind == Gen2Picker::Kind::Pokerus) drawPokerusPicker(fb, picker);
    else drawListPicker(screen, fb, state, picker);
}

} // namespace UI::Gen2PokemonEditor
