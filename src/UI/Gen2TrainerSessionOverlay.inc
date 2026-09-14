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
    if (state.owner != &screen) { state = {}; state.owner = &screen; }
    return state;
}
bool handleTrainerSession(TrainerViewScreen& screen, uint64_t down, uint64_t held, int stickX, int stickY) {
    auto& state = trainerSessionFor(screen);
    auto& session = state.session;
    if (!session.active) return false;
    if (session.confirmExit) {
        if (down & HidNpadButton_B) session.continueEditing();
        else if (down & HidNpadButton_X) session.discard();
        else if (down & HidNpadButton_A) {
            if (auto* editor = stagedEditor(screen); editor && session.keep(*editor, state.error))
                screen.postStatus("Trainer changes kept in staged copy; source unchanged");
        }
        return true;
    }
    if (down & HidNpadButton_B) { session.back(); return true; }
    const auto nav = screen.controllerNavigation.apply(down, held, stickX, stickY,
        HidNpadButton_Up, HidNpadButton_Down, HidNpadButton_Left, HidNpadButton_Right);
    if (nav & (HidNpadButton_Up | HidNpadButton_Down)) state.row = 1 - state.row;
    if (down & HidNpadButton_A) {
        state.error.clear();
        if (state.row == 0) {
            const auto result = Utils::promptText("Trainer Name", "1-7 native Gen II characters", session.name, 7);
            if (result.accepted) session.name = result.text;
        } else {
            const auto result = Utils::promptNumber("Money", static_cast<int>(session.money), 0,
                static_cast<int>(Gen2::kMaxMoney));
            if (result.accepted) session.money = static_cast<uint32_t>(result.value);
        }
    }
    return true;
}
void drawTrainerSession(TrainerViewScreen& screen, PKSEFramebuffer& fb) {
    auto& state = trainerSessionFor(screen);
    const auto& session = state.session;
    constexpr int x = 220, y = 96, w = 840, h = 480;
    fb.drawFilledRect(0, 0, fb.getWidth(), fb.getHeight() - 48, Color(0, 0, 0, 105));
    drawPanelSurface(fb, x, y, w, h, false, 18);
    fb.drawText(x + 24, y + 18, "Edit Trainer — STAGED", Colors::Text, TextStyle::Heading);
    fb.drawText(x + 24, y + 52, "Local edit session  •  original source immutable", Colors::TextDim, TextStyle::Caption);
    drawEditorRow(fb, x + 24, y + 96, w - 48, "Name", session.name, state.row == 0);
    drawEditorRow(fb, x + 24, y + 156, w - 48, "Money", "$" + std::to_string(session.money), state.row == 1);
    fb.drawText(x + 36, y + 238, "Trainer ID: " + std::to_string(screen.trainer.TID16) + "  •  Read only", Colors::TextDim);
    fb.drawText(x + 36, y + 278, screen.sourceGameId == "crystal_gbc"
        ? std::string("Gender: ") + (screen.trainer.trainerGender == 1 ? "Female" : "Male") + "  •  Native Crystal / Read only"
        : "Gender: Male  •  Gold/Silver fixed / Read only", Colors::TextDim);
    fb.drawText(x + 36, y + 330, "B: Keep / Discard this Edit / Continue", Colors::Accent, TextStyle::Caption);
    fb.drawText(x + 36, y + 370, "No SID is stored. Review / Export is available from Trainer with Y.", Colors::TextDim, TextStyle::Caption);
    if (!state.error.empty()) fb.drawText(x + 36, y + 414, state.error, Colors::Error, TextStyle::Caption);
    drawNavBar(fb, {{"D-pad/Stick", "Navigate"}, {"A", "Edit"}, {"B", "Back"}});
    if (session.confirmExit) {
        drawPanelSurface(fb, 340, 210, 600, 260, false, 18);
        fb.drawText(366, 234, "Keep Trainer changes?", Colors::Text, TextStyle::Heading);
        fb.drawText(366, 286, "A  Keep in staged copy", Colors::Success);
        fb.drawText(366, 330, "X  Discard this Edit", Colors::Warning);
        fb.drawText(366, 374, "B  Continue editing", Colors::Text);
        if (!state.error.empty()) fb.drawText(366, 424, state.error, Colors::Error, TextStyle::Caption);
        drawNavBar(fb, {{"A", "Keep"}, {"X", "Discard this Edit"}, {"B", "Continue"}});
    }
}
} // namespace
} // namespace UI::Gen2PokemonEditor
