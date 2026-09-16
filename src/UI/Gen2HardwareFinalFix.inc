// Final hardware-retest overlay for the normal Gen II Trainer page.
// Name/Money remain the only cursor rows. Trainer ID and Gender are informational.

namespace UI::Gen2PokemonEditor {
namespace {

void drawGoldSilverTrainerGender(TrainerViewScreen& screen, PKSEFramebuffer& fb) {
    if (screen.sourceGameId != "gold_gbc" && screen.sourceGameId != "silver_gbc") return;
    const auto& bridge = gscTrainer(screen);
    if (!bridge.hasTrainerGender()) return;

    constexpr int contentX = 12;
    constexpr int contentY = 80;
    constexpr int headerH = 46;
    constexpr int cardW = 680;
    constexpr int editableRowH = 60;
    constexpr int editableRowGap = 14;
    constexpr int dividerBefore = 6;
    constexpr int dividerAfter = 20;
    constexpr int infoRowStep = 54;

    const int contentW = fb.getWidth() - contentX;
    const int cardX = contentX + (contentW - cardW) / 2;
    const int trainerIdY = contentY + headerH + 26 + 2 * (editableRowH + editableRowGap) +
                           dividerBefore + dividerAfter;
    const int genderY = trainerIdY + infoRowStep;
    const std::string value = screen.trainer.trainerGender == 0 ? "Male" :
                              screen.trainer.trainerGender == 1 ? "Female" : "Unknown";

    fb.drawFilledRoundedRect(cardX, genderY, cardW, 46, 12, Colors::PanelAlt);
    fb.drawText(cardX + 24, genderY + (46 - fb.lineHeight(TextStyle::Body)) / 2,
                "Gender", Colors::TextDim, TextStyle::Body);
    int vw = 0, vh = 0;
    fb.measureText(value, vw, vh, TextStyle::Body);
    fb.drawText(cardX + cardW - 24 - vw, genderY + (46 - vh) / 2,
                value, Colors::Text, TextStyle::Body);
}

} // namespace

bool drawFinalGen2Surface(TrainerViewScreen& screen, PKSEFramebuffer& fb) {
    if (drawFinalGen2SurfaceBase(screen, fb)) {
        // The accepted parity route draws the raw unified workspace directly during
        // Create/Edit. Re-apply the hardware correction layer so DETAILS/STATS/MOVES
        // always remain neutral; the compact field/cell cursor is the sole focus cue.
        if (isGSC(screen) && stateFor(screen).mode != Mode::None)
            drawHardwareWorkspaceCorrections(screen, fb);
        return true;
    }
    if (trainerMainPage(screen) &&
        (screen.sourceGameId == "gold_gbc" || screen.sourceGameId == "silver_gbc")) {
        drawGoldSilverTrainerGender(screen, fb);
        return true;
    }
    return false;
}

} // namespace UI::Gen2PokemonEditor
