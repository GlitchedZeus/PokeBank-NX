// Hardware-retest adapter for the shared Generation II move picker.
// The underlying picker/session code remains unchanged; this layer narrows the
// normal Move list to Empty + exact-game-compatible choices, matching Gen I.

namespace UI::Gen2PokemonEditor {
namespace {

bool hardwareMoveAllowed(const TrainerViewScreen& screen, const State& state, uint16_t move) {
    if (move == 0) return true;
    const auto game = PokeBank::UIModel::Gen2PokemonEditor::exactGameFromSourceId(screen.sourceGameId);
    return game && PokeVault::Integration::Gen2::MoveCompatibility::canLearnMove(
        *game, state.working.species, move);
}

std::vector<uint16_t> hardwareCompatibleMoveChoices(const TrainerViewScreen& screen,
                                                     const State& state) {
    std::vector<uint16_t> choices;
    choices.reserve(252);
    choices.push_back(0);
    for (uint16_t move = 1; move <= 251; ++move) {
        if (hardwareMoveAllowed(screen, state, move)) choices.push_back(move);
    }
    return choices;
}

int hardwareMoveChoicePosition(const std::vector<uint16_t>& choices, uint16_t move) {
    for (std::size_t i = 0; i < choices.size(); ++i)
        if (choices[i] == move) return static_cast<int>(i);
    return 0;
}

void normalizeHardwareMovePicker(const TrainerViewScreen& screen, const State& state,
                                 PickerOverlayState& picker) {
    if (picker.model.kind != PokeBank::UIModel::Gen2PokemonPicker::Kind::Move) return;
    if (!hardwareMoveAllowed(screen, state, picker.model.moveChoice())) picker.model.index = 0;
}

void stepHardwareMovePicker(const TrainerViewScreen& screen, const State& state,
                            PickerOverlayState& picker, int delta) {
    const auto choices = hardwareCompatibleMoveChoices(screen, state);
    if (choices.empty()) { picker.model.index = 0; return; }
    const int count = static_cast<int>(choices.size());
    const int current = hardwareMoveChoicePosition(choices, picker.model.moveChoice());
    int next = (current + delta) % count;
    if (next < 0) next += count;
    picker.model.index = static_cast<int>(choices[static_cast<std::size_t>(next)]);
}

void drawHardwareMovePicker(TrainerViewScreen& screen, PKSEFramebuffer& fb, const State& state,
                            const PokeBank::UIModel::Gen2PokemonPicker::Model& model) {
    const int W = fb.getWidth(), H = fb.getHeight();
    constexpr int panelW = 760, panelH = 568;
    const int x = (W - panelW) / 2, y = (H - panelH) / 2;
    fb.drawFilledRect(0, 0, W, H, Color(0, 0, 0, 165));
    drawPanelSurface(fb, x, y, panelW, panelH, false, 18);
    fb.drawText(x + 24, y + 18, "Move — Generation II", Colors::Text, TextStyle::Heading);
    fb.drawText(x + 24, y + 52,
                "Empty + compatible moves only • exact current game",
                Colors::TextDim, TextStyle::Caption);

    const auto choices = hardwareCompatibleMoveChoices(screen, state);
    const int count = static_cast<int>(choices.size());
    constexpr int visible = 13;
    const int selectedPos = hardwareMoveChoicePosition(choices, model.moveChoice());
    const int first = std::clamp(selectedPos - visible / 2, 0, std::max(0, count - visible));
    const int last = std::min(count, first + visible);
    for (int i = first; i < last; ++i) {
        const int yy = y + 88 + (i - first) * 33;
        const uint16_t move = choices[static_cast<std::size_t>(i)];
        const bool selected = i == selectedPos;
        const std::string name = move == 0 ? "Empty" : std::string(Names::getMoveName(move));
        drawPickerRow(fb, x + 20, yy, panelW - 40, name, selected);
    }
    drawNavBar(fb, {{"D-pad/Stick", "Browse"}, {"Left/Right", "Page"},
                    {"A", "Select"}, {"B", "Cancel"}});
}

} // namespace

bool handlePickerInput(TrainerViewScreen& screen, uint64_t down, uint64_t held,
                       int stickX, int stickY, const TouchInput& touch) {
    if (!isGSC(screen)) return false;
    auto& state = stateFor(screen);
    auto& picker = pickerStateFor(screen);

    if (!picker.model.active()) {
        const bool handled = handlePickerInputBase(screen, down, held, stickX, stickY, touch);
        if (handled) normalizeHardwareMovePicker(screen, state, picker);
        return handled;
    }

    if (picker.model.kind != PokeBank::UIModel::Gen2PokemonPicker::Kind::Move)
        return handlePickerInputBase(screen, down, held, stickX, stickY, touch);

    if (state.mode == Mode::View || state.mode == Mode::None)
        return handlePickerInputBase(screen, down, held, stickX, stickY, touch);

    const uint64_t nav = screen.controllerNavigation.apply(
        down, held, stickX, stickY,
        HidNpadButton_Up, HidNpadButton_Down, HidNpadButton_Left, HidNpadButton_Right);

    if (down & HidNpadButton_B) {
        closePicker(screen, picker);
        return true;
    }
    if (nav & HidNpadButton_Up) stepHardwareMovePicker(screen, state, picker, -1);
    if (nav & HidNpadButton_Down) stepHardwareMovePicker(screen, state, picker, 1);
    if (nav & HidNpadButton_Left) stepHardwareMovePicker(screen, state, picker, -10);
    if (nav & HidNpadButton_Right) stepHardwareMovePicker(screen, state, picker, 10);
    if (down & HidNpadButton_A) applyPickerChoice(screen, state, picker);
    return true;
}

void drawPickerOverlay(TrainerViewScreen& screen, PKSEFramebuffer& fb) {
    const auto& state = stateFor(screen);
    const auto& picker = pickerStateFor(screen).model;
    if (state.mode != Mode::View && picker.active() &&
        picker.kind == PokeBank::UIModel::Gen2PokemonPicker::Kind::Move) {
        drawHardwareMovePicker(screen, fb, state, picker);
        return;
    }
    drawPickerOverlayBase(screen, fb);
}

} // namespace UI::Gen2PokemonEditor
