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
    constexpr auto layout = PokeBank::UIModel::MovePickerPresentation::compactPickerLayout();
    constexpr int panelW = layout.width, panelH = layout.height;
    const int x = (W - panelW) / 2, y = (H - panelH) / 2;
    fb.drawFilledRect(0, 0, W, H, Color(0, 0, 0, 165));
    drawPanelSurface(fb, x, y, panelW, panelH, false, 18);
    fb.drawText(x + 24, y + 18, "Move — Generation II", Colors::Text, TextStyle::Heading);
    fb.drawText(x + 24, y + 52,
                "Empty + compatible moves only • exact Gen II Acc / Pwr / PP",
                Colors::TextDim, TextStyle::Caption);

    const auto choices = hardwareCompatibleMoveChoices(screen, state);
    const int count = static_cast<int>(choices.size());
    constexpr int visible = layout.visibleRows;
    const int selectedPos = hardwareMoveChoicePosition(choices, model.moveChoice());
    const int viewportTop = y + layout.rowStart;
    const int viewportH = visible * layout.rowStep;
    const auto visual = liveVerticalListVisual(
        selectedPos, count, layout.rowStep,
        x + 24, viewportTop, panelW - 48, viewportH);
    const int first = std::clamp(
        visual.index - visible / 2, 0, std::max(0, count - visible));
    const int drawFirst = std::max(0, first - 1);
    const int drawLast = std::min(count, first + visible + 1);
    screen.touchButtons.clear();
    fb.setClipRect(x + 24, viewportTop, panelW - 48, viewportH);
    for (int i = drawFirst; i < drawLast; ++i) {
        const int yy = viewportTop + (i - first) * layout.rowStep + visual.offset;
        const uint16_t move = choices[static_cast<std::size_t>(i)];
        const bool selected = i == visual.index;
        if (yy + layout.highlightHeight > viewportTop && yy < viewportTop + viewportH)
            screen.touchButtons.push_back(
                {2100 + i, x + 24, yy, panelW - 48, layout.highlightHeight});
        const std::string label =
            PokeBank::UIModel::MovePickerPresentation::rowLabel(move, Enums::GameVersion::GSC);
        if (selected)
            fb.drawSelectionHighlight(x + 24, yy, panelW - 48, layout.highlightHeight);
        fb.drawText(x + 40, yy + 8, label,
                    selected ? Colors::Text : Colors::TextDim, TextStyle::Body);
    }
    fb.clearClip();
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

    const auto touchChoices = hardwareCompatibleMoveChoices(screen, state);
    const int touchDown = screen.touchedButtonDownId(touch);
    if (touchDown >= 2100 && touchDown < 2100 + static_cast<int>(touchChoices.size()))
        picker.model.index = static_cast<int>(touchChoices[static_cast<std::size_t>(touchDown - 2100)]);
    const int touchTap = screen.touchedButtonId(touch);
    if (touchTap >= 2100 && touchTap < 2100 + static_cast<int>(touchChoices.size())) {
        picker.model.index = static_cast<int>(touchChoices[static_cast<std::size_t>(touchTap - 2100)]);
        down |= HidNpadButton_A;
    }
    if (touch.justReleased() && touch.dragged()) {
        const int steps = std::max(1, std::abs(touch.deltaY()) / std::max(1, layout.rowStep));
        stepHardwareMovePicker(screen, state, picker, touch.deltaY() < 0 ? steps : -steps);
    }

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
