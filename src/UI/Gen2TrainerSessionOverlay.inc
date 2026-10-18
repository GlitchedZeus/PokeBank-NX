#include "UI/ClassicTrainerSessionInput.h"
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

// Reuse the existing Trainer page and its two cursor rows through the one classic Trainer session
// flow (UI/ClassicTrainerSessionInput.h, shared with Gen I). Accepted fields are committed only to
// the staged backend; review/export/discard happen in the staged editor flow.
bool handleTrainerSession(TrainerViewScreen& screen, uint64_t down, uint64_t held,
                          int stickX, int stickY, const TouchInput& touch) {
    if (!trainerMainPage(screen)) return false;
    auto* editor = stagedEditor(screen);
    if (!editor) return false;
    UI::ClassicTrainerSession::Fields fields;
    fields.name = editor->trainerName();
    fields.money = editor->money();
    fields.nameMax = 7;
    fields.moneyMax = Gen2::kMaxMoney;
    fields.nameGuide = "1-7 native Gen II characters";
    return UI::ClassicTrainerSession::handle(screen, down, held, stickX, stickY, touch, fields,
        [&](const std::string& name, uint32_t money, std::string& error) {
            return editor->stageTrainerEdit(name, money, error);
        });
}
} // namespace
} // namespace UI::Gen2PokemonEditor
