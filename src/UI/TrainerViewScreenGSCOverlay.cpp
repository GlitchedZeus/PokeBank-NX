#include "UI/TrainerViewScreen.h"

#include "Enums/GameVersion.h"
#include "UI/Common.h"
#include "UI/LegacyPresentationRules.h"
#include "UI/PKSEFramebuffer.h"
#include "Trainer/Trainer.h"

#include <algorithm>
#include <string>

namespace UI {
namespace {

bool isGSCSource(const TrainerViewScreen& screen) noexcept {
    return screen.sourceGameId == "gold_gbc" ||
           screen.sourceGameId == "silver_gbc" ||
           screen.sourceGameId == "crystal_gbc";
}

bool itemCategoryNavigationAvailable(const TrainerViewScreen& screen) noexcept {
    return screen.selectedMode == TrainerViewScreen::ViewMode::Items &&
           !screen.pickerActive && !screen.itemEditDialogActive &&
           !screen.itemRemoveConfirmActive && !screen.statEdit.dialogActive &&
           !screen.saveConfirmActive && !screen.releaseConfirmActive &&
           !screen.storageExitConfirmActive && !screen.groupMenuActive &&
           !screen.creator.keepConfirmActive && !screen.details.active &&
           !screen.details.discardConfirmActive && !screen.gen3ConvertConfirmActive &&
           !screen.lgpeTransferConfirmActive && !screen.actionSheet.isOpen();
}

void drawGSCTrainerCard(TrainerViewScreen& screen, PKSEFramebuffer& fb) {
    constexpr int x = 12;
    constexpr int y = 80;
    constexpr int h = 560;
    constexpr int headerH = 46;
    const int w = fb.getWidth() - x;

    // The accepted base screen drew the later-generation Trainer card first. Replace only the content
    // panel for GSC so no SID, full modern IDs, account/profile data, or unproven GS player gender can
    // remain visible. The title bar and existing navigation footer stay untouched.
    fb.drawFilledRoundedRect(x, y, w, h, 16, Colors::Panel);
    fb.drawRoundedRect(x, y, w, h, 16, Colors::Border, 1);
    fb.drawFilledRoundedRect(x, y, w, headerH, 16, Colors::AccentDim);
    fb.drawFilledRect(x, y + headerH - 16, w, 16, Colors::AccentDim);
    fb.drawText(x + 22, y + (headerH - fb.lineHeight(TextStyle::Heading)) / 2,
                "Trainer — Generation II / Read Only", Colors::Text, TextStyle::Heading);

    screen.touchButtons.clear();

    const int cardW = 680;
    const int cardX = x + (w - cardW) / 2;
    int rowY = y + headerH + 36;
    constexpr int rowH = 56;
    constexpr int rowGap = 14;

    auto row = [&](const char* label, const std::string& value) {
        fb.drawSoftShadow(cardX, rowY, cardW, rowH, 14);
        fb.drawFilledRoundedRect(cardX, rowY, cardW, rowH, 14, Colors::PanelAlt);
        fb.drawText(cardX + 24, rowY + (rowH - fb.lineHeight(TextStyle::Body)) / 2,
                    label, Colors::TextDim, TextStyle::Body);
        int vw = 0, vh = 0;
        fb.measureText(value, vw, vh, TextStyle::Body);
        fb.drawText(cardX + cardW - 24 - vw, rowY + (rowH - vh) / 2,
                    value, Colors::Text, TextStyle::Body);
        rowY += rowH + rowGap;
    };

    const Trainer::Trainer& trainer = screen.trainer;
    row("Name", trainer.trainerName.empty() ? std::string("(none)") : trainer.trainerName);
    row("Money", "$" + std::to_string(trainer.money));
    row("Trainer ID", std::to_string(trainer.TID16));

    // Gold/Silver do not carry a proven player-gender field. Crystal's strict parser validates the
    // family and accepts only the stored 0/1 gender byte before this source can reach the runtime.
    if (screen.sourceGameId == "crystal_gbc" && trainer.trainerGender <= 1)
        row("Player Gender", trainer.trainerGender == 0 ? "Male" : "Female");

    rowY += 8;
    const char* note = screen.sourceGameId == "crystal_gbc"
        ? "Crystal gender is shown only from the validated Crystal save field."
        : "Gold/Silver do not expose a proven player-gender field.";
    fb.drawText(cardX + 12, rowY, note, Colors::TextDim, TextStyle::Caption);
    fb.drawText(cardX + 12, rowY + 28,
                "No SID or later-generation trainer metadata is stored in this Gen II view.",
                Colors::TextDim, TextStyle::Caption);

    // The accepted base draws transient status before this overlay; redraw it so a read-only-action
    // refusal is not hidden by the corrected card.
    if (screen.storageStatusFrames > 0 && !screen.storageStatus.empty()) {
        int tw = 0, th = 0;
        fb.measureText(screen.storageStatus, tw, th);
        const int padX = 18, bw = tw + padX * 2, bh = th + 14;
        const int footerY = fb.getHeight() - kNavBarH;
        const int bx = (fb.getWidth() - bw) / 2, by = footerY - bh - 12;
        fb.drawFilledRoundedRect(bx, by, bw, bh, 8, Colors::Panel);
        fb.drawRoundedRect(bx, by, bw, bh, 8, Colors::Accent, 2);
        fb.drawText(bx + padX, by + 7, screen.storageStatus, Colors::Text);
    }
}

} // namespace

void TrainerViewScreen::update(const PadState& pad, const TouchInput& touch) {
    // The accepted large implementation has no GSC switch arm, so its L/R item-tab handling is a
    // deliberate no-op for group 70. Handle the five real Gen II pockets here, then delegate every
    // other input path unchanged. This preserves RBY=2 and every existing Gen III+ pouch count.
    if (trainer.getGameGroup() == Enums::GameVersion::GSC &&
        isGSCSource(*this) && itemCategoryNavigationAvailable(*this)) {
        const u64 down = padGetButtonsDown(&pad);
        const int count = legacyInventoryCategoryCount(Enums::GameVersion::GSC);
        if (count > 0 && (down & HidNpadButton_L)) {
            selectedCategory = (selectedCategory - 1 + count) % count;
            currentPage = 0;
            selectedItemIndex = 0;
        }
        if (count > 0 && (down & HidNpadButton_R)) {
            selectedCategory = (selectedCategory + 1) % count;
            currentPage = 0;
            selectedItemIndex = 0;
        }
    }

    updateLegacyBase(pad, touch);
}

void TrainerViewScreen::draw(PKSEFramebuffer& fb) {
    drawLegacyBase(fb);

    if (trainer.getGameGroup() == Enums::GameVersion::GSC && isGSCSource(*this) &&
        detailViewActive && selectedMode == ViewMode::Trainer && !helpOverlayActive) {
        drawGSCTrainerCard(*this, fb);
    }
}

} // namespace UI
