namespace UI::Gen2PokemonEditor {
namespace {
bool trainerMainPage(const TrainerViewScreen& screen) noexcept {
    return isGSC(screen) && screen.detailViewActive &&
        screen.selectedMode == TrainerViewScreen::ViewMode::Trainer &&
        !editorOverlayState(screen).active && !screen.helpOverlayActive &&
        !screen.details.active && !screen.actionSheet.isOpen() && !screen.saveConfirmActive &&
        !screen.pickerActive && !screen.itemEditDialogActive && !screen.carrying() &&
        !screen.currentlySelecting;
}

// Reuse the existing Trainer page and its two cursor rows. Accepted fields are
// committed only to the staged backend; Y opens the shared review/export flow.
bool handleTrainerSession(TrainerViewScreen& screen, uint64_t down, uint64_t held,
                          int stickX, int stickY, const TouchInput& touch) {
    if (!trainerMainPage(screen)) return false;
    auto* editor = stagedEditor(screen);
    if (!editor) return false;
    screen.trainer.trainerName = editor->trainerName();
    screen.trainer.money = editor->money();
    const int touched = screen.touchedButtonId(touch);
    if (touched >= 0 && touched < 2) {
        screen.trainerSelectedRow = touched;
        down |= HidNpadButton_A;
    }
    const uint64_t nav = screen.controllerNavigation.apply(
        down, held, stickX, stickY,
        HidNpadButton_Up, HidNpadButton_Down, HidNpadButton_Left, HidNpadButton_Right);
    if (nav & (HidNpadButton_Up | HidNpadButton_Down)) {
        screen.trainerSelectedRow = (screen.trainerSelectedRow + 1) % 2;
        return true;
    }
    if (!(down & (HidNpadButton_A | HidNpadButton_X))) return false;
    std::string name = editor->trainerName();
    uint32_t money = editor->money();
    bool accepted = false;
    if (screen.trainerSelectedRow == 0) {
        const auto result = Utils::promptText("Trainer Name", "1-7 native Gen II characters", name, 7);
        accepted = result.accepted;
        if (accepted) name = result.text;
    } else {
        const auto result = Utils::promptNumber("Money", static_cast<int>(money), 0,
                                                static_cast<int>(Gen2::kMaxMoney));
        accepted = result.accepted;
        if (accepted) money = static_cast<uint32_t>(result.value);
    }
    if (accepted) {
        std::string error;
        if (editor->stageTrainerEdit(name, money, error)) {
            screen.trainer.trainerName = editor->trainerName();
            screen.trainer.money = editor->money();
            screen.postStatus("Trainer edit staged", 240);
        } else screen.postStatus(error, 300);
    }
    return true;
}
} // namespace
} // namespace UI::Gen2PokemonEditor
