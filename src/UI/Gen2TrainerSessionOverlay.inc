#include "UI/Gen2TrainerSession.h"

namespace UI::Gen2PokemonEditor {
namespace {
struct TrainerSessionState {
    const TrainerViewScreen* owner = nullptr;
    PokeBank::UIModel::Gen2Trainer::Session session;
    int row = 0;
    std::string error;
};
TrainerSessionState& trainerSessionFor(const TrainerViewScreen& screen) {
    static TrainerSessionState state;
    if (state.owner != &screen) {
        state = {};
        state.owner = &screen;
    }
    return state;
}

bool trainerMainPage(const TrainerViewScreen& screen) noexcept {
    return isGSC(screen) && screen.detailViewActive &&
        screen.selectedMode == TrainerViewScreen::ViewMode::Trainer &&
        !editorOverlayState(screen).active && !screen.helpOverlayActive &&
        !screen.details.active && !screen.actionSheet.isOpen() && !screen.saveConfirmActive &&
        !screen.pickerActive && !screen.itemEditDialogActive && !screen.carrying() &&
        !screen.currentlySelecting;
}

void beginTrainerSessionIfNeeded(TrainerViewScreen& screen, TrainerSessionState& state) {
    if (state.session.active) return;
    if (auto* editor = stagedEditor(screen)) state.session.begin(*editor);
}

bool handleTrainerSession(TrainerViewScreen& screen, uint64_t down, uint64_t held,
                          int stickX, int stickY) {
    if (!trainerMainPage(screen)) return false;
    auto* editor = stagedEditor(screen);
    if (!editor) return false;
    auto& state = trainerSessionFor(screen);

    if (state.session.confirmExit) {
        if (down & HidNpadButton_A) {
            state.error.clear();
            if (state.session.keep(*editor, state.error))
                screen.postStatus("Trainer Name / Money kept in staged copy", 260);
            else
                screen.postStatus(state.error.empty() ? "Trainer edit was rejected" : state.error, 320);
        } else if (down & HidNpadButton_X) {
            state.session.discard();
            screen.postStatus("This Trainer edit discarded; earlier staged work preserved", 280);
        } else if (down & HidNpadButton_B) {
            state.session.continueEditing();
        }
        return true;
    }

    const uint64_t nav = screen.controllerNavigation.apply(
        down, held, stickX, stickY,
        HidNpadButton_Up, HidNpadButton_Down, HidNpadButton_Left, HidNpadButton_Right);
    if (nav & HidNpadButton_Up) {
        state.row = (state.row + 3) % 4;
        beginTrainerSessionIfNeeded(screen, state);
        return true;
    }
    if (nav & HidNpadButton_Down) {
        state.row = (state.row + 1) % 4;
        beginTrainerSessionIfNeeded(screen, state);
        return true;
    }

    // Leaving the Trainer page while a local edit is dirty must resolve Keep/Discard/Continue first.
    if (down & (HidNpadButton_L | HidNpadButton_R | HidNpadButton_Y)) {
        if (state.session.active && state.session.dirty()) {
            state.session.back();
            return true;
        }
        if (state.session.active) state.session.active = false;
        return false;
    }

    if (down & HidNpadButton_B) {
        if (state.session.active && state.session.dirty()) {
            state.session.back();
            return true;
        }
        if (state.session.active) state.session.active = false;
        return false;
    }

    if (!(down & (HidNpadButton_A | HidNpadButton_X))) return false;
    beginTrainerSessionIfNeeded(screen, state);
    if (!state.session.active) return false;

    if (state.row == 0) {
        auto r = Utils::promptText("Trainer Name", "1-7 native Gen II characters", state.session.name, 7);
        if (r.accepted) state.session.name = r.text;
    } else if (state.row == 1) {
        auto r = Utils::promptNumber("Money", static_cast<int>(state.session.money), 0,
                                     static_cast<int>(Gen2::kMaxMoney));
        if (r.accepted) state.session.money = static_cast<uint32_t>(r.value);
    } else if (state.row == 2) {
        screen.postStatus("Trainer ID is read-only in this staged Trainer editor", 260);
    } else {
        screen.postStatus("Gender is save-derived/read-only for this exact Generation II game", 280);
    }
    return true; // consume A/X before the old detached Trainer editor route.
}

void drawTrainerRow(PKSEFramebuffer& fb, int x, int y, int w,
                    const std::string& label, const std::string& value,
                    bool selected, bool editable) {
    if (selected) fb.drawSelectionHighlight(x, y, w, 54);
    fb.drawText(x + 16, y + 9, label, Colors::TextDim, TextStyle::Caption);
    fb.drawText(x + 180, y + 9, value, selected ? Colors::Accent : Colors::Text, TextStyle::Body);
    fb.drawText(x + w - 108, y + 12, editable ? "Editable" : "Read only",
                editable ? Colors::Success : Colors::TextDim, TextStyle::Caption);
}

void drawTrainerSession(TrainerViewScreen& screen, PKSEFramebuffer& fb) {
    if (!trainerMainPage(screen)) return;
    auto* editor = stagedEditor(screen);
    if (!editor) return;
    auto& state = trainerSessionFor(screen);
    const auto& trainer = gscTrainer(screen);
    const std::string name = state.session.active ? state.session.name : editor->trainerName();
    const uint32_t money = state.session.active ? state.session.money : editor->money();

    // This is the Trainer page itself, not a detached modal. It mirrors the Gen I/PKSE
    // row-focused interaction: D-pad/Stick focus, A/X edit only when the row is editable.
    const int panelW = 760;
    const int panelH = 430;
    const int x = (fb.getWidth() - panelW) / 2;
    const int y = 78;
    fb.drawFilledRect(0, 0, fb.getWidth(), fb.getHeight() - 48, Colors::Background);
    drawPanelSurface(fb, x, y, panelW, panelH, false, 16);
    fb.drawText(x + 24, y + 18, "Trainer — Generation II", Colors::Text, TextStyle::Heading);
    fb.drawText(x + 24, y + 52, "Staged copy • original source immutable", Colors::TextDim, TextStyle::Caption);

    const std::string gender = trainer.hasTrainerGender()
        ? (trainer.trainerGender == 0 ? "Male" : trainer.trainerGender == 1 ? "Female" : "Unknown")
        : "Unavailable";
    const std::array<std::pair<std::string, std::string>, 4> rows{{
        {"Name", name},
        {"Money", "$" + std::to_string(money)},
        {"Trainer ID", std::to_string(editor->trainerId())},
        {"Gender", gender},
    }};
    for (int i = 0; i < 4; ++i)
        drawTrainerRow(fb, x + 24, y + 92 + i * 66, panelW - 48,
                       rows[static_cast<std::size_t>(i)].first,
                       rows[static_cast<std::size_t>(i)].second,
                       i == state.row, i < 2);

    fb.drawText(x + 24, y + panelH - 48,
        state.session.active && state.session.dirty()
            ? "Unkept Trainer edits are local until you leave this page and choose Keep."
            : "Name and Money are editable. Trainer ID and Gender remain truthful/read-only.",
        state.session.active && state.session.dirty() ? Colors::Warning : Colors::TextDim,
        TextStyle::Caption);

    if (state.session.confirmExit) {
        const int cw = 600, ch = 180;
        const int cx = (fb.getWidth() - cw) / 2, cy = 424;
        fb.drawSoftShadow(cx, cy, cw, ch, 14);
        fb.drawFilledRoundedRect(cx, cy, cw, ch, 14, Colors::Panel);
        fb.drawRoundedRect(cx, cy, cw, ch, 14, Colors::Accent, 2);
        fb.drawText(cx + 22, cy + 18, "Keep staged Trainer edits?", Colors::Text, TextStyle::Heading);
        fb.drawText(cx + 22, cy + 62, "A Keep    X Discard this Edit    B Continue",
                    Colors::Text, TextStyle::Body);
        fb.drawText(cx + 22, cy + 108, "Original source save is never modified.",
                    Colors::TextDim, TextStyle::Caption);
    }

    drawNavBar(fb, {{"D-pad/Stick", "Row"}, {"A/X", "Edit"}, {"Y", "Review"}, {"B", "Back"}});
}
} // namespace
} // namespace UI::Gen2PokemonEditor