#include <algorithm>
#include <cstdio>

#include "UI/BackupSelectionScreen.h"
#include "UI/Common.h"
#include "UI/ScreenChrome.h"
#include "UI/TouchInput.h"
#include "UI/Dialogs/DialogFrame.h"
#include "Utils/HelperUtilities.h"
#include "Utils/FileUtilities.h"
#include "Utils/PokeBankPaths.h"
#include "Globals.h"
#include "Games/GameIdentity.h"

using namespace Utils;

namespace UI {
    // UI Layout constants
    constexpr int CARD_X = 40;
    constexpr int CARD_Y = 84;
    constexpr int CARD_W = 1200;
    constexpr int LIST_ROW_H = 62;
    constexpr int LIST_MAX_VISIBLE = 8;   // rows that fit in the card before scrolling

    BackupSelectionScreen::BackupSelectionScreen(
        AccountUid userUid, u64 titleId, const std::string& titleName)
        : userUid(userUid), titleId(titleId), titleName(titleName), selectedIndex(0),
          backupSelected(false), createNewBackup(false), goBack(false),
          showDeleteConfirmation(false), deleteConfirmationIndex(-1) {

        const auto* identity = PokeVault::Games::findSwitchGame(titleId);
        if (identity) {
            exactGameId = std::string(identity->id);
            gameDirectory = PokeBank::Paths::exactGameBackupsRoot(userUid, identity->id);
            namespaceReady = !gameDirectory.empty();
        }

        // Historical title-only backups have no persisted AccountUid ownership. Keep them visible
        // as quarantined recovery evidence, never silently claim them for the current profile.
        legacyGameDirectory = PokeBank::Paths::legacyUnscopedGameBackupsRoot(titleName);

        loadBackups();
    }

    void BackupSelectionScreen::loadBackups() {
        backups.clear();

        BackupInfo newBackupOption;
        newBackupOption.timestamp = "";
        newBackupOption.displayName = namespaceReady
            ? (g_autoBackupEnabled
                ? "Browse installed source (read-only / new backup)"
                : "Browse installed source (read-only / working copy)")
            : "Backup namespace unavailable (profile/game identity required)";
        backups.push_back(std::move(newBackupOption));

        if (namespaceReady) {
            for (const auto& name : listBackupDirectories(gameDirectory.c_str())) {
                // Named backups may contain spaces. The name came from readdir below an already
                // profile/exact-game-scoped root, so preserve it verbatim after rejecting traversal.
                if (name.empty() || name == "." || name == ".." ||
                    name.find('/') != std::string::npos) continue;
                const std::string path = gameDirectory + "/" + name;
                if (!PokeBank::Paths::isOwnedPath(path)) continue;
                BackupInfo info;
                info.timestamp = name;
                info.displayName = "Edit backup workspace: " + formatTimestamp(name);
                info.path = path;
                backups.push_back(std::move(info));
            }
        }

        // Legacy unscoped folders are deliberately visible but not editable. The historical layout
        // never stored AccountUid, so current-user ownership cannot be proven. Include old Working/
        // as well: it may contain the only surviving edited staging copy and must not disappear.
        for (const auto& name : listBackupDirectories(legacyGameDirectory.c_str(), true)) {
            // name is a directory entry discovered under legacyGameDirectory. Preserve historical
            // custom names verbatim (including spaces); reject only traversal/separator shapes.
            if (name.empty() || name == "." || name == ".." ||
                name.find('/') != std::string::npos) continue;
            const std::string path = legacyGameDirectory + "/" + name;
            if (!PokeBank::Paths::isOwnedPath(path)) continue;
            BackupInfo info;
            info.timestamp = name;
            info.path = path;
            info.legacyUnscoped = true;
            info.displayName = "LEGACY UNSCOPED / OWNERSHIP UNKNOWN — " +
                (name == "Working" ? std::string("Working workspace") : formatTimestamp(name));
            backups.push_back(std::move(info));
        }
    }

    std::string BackupSelectionScreen::formatTimestamp(const std::string& timestamp) {
        // Convert YYYYMMDD_HHMMSS to readable format: YYYY-MM-DD HH:MM:SS
        if (timestamp.length() != 15) return timestamp;

        std::string formatted;
        formatted += timestamp.substr(0, 4);  // YYYY
        formatted += "-";
        formatted += timestamp.substr(4, 2);  // MM
        formatted += "-";
        formatted += timestamp.substr(6, 2);  // DD
        formatted += " ";
        formatted += timestamp.substr(9, 2);  // HH
        formatted += ":";
        formatted += timestamp.substr(11, 2); // MM
        formatted += ":";
        formatted += timestamp.substr(13, 2); // SS

        return formatted;
    }

    void BackupSelectionScreen::update(const PadState& pad, const TouchInput& touch) {
        const HidAnalogStickState stick = padGetStickPos(&pad, 0);
        u64 kDown = controllerNavigation.apply(
            padGetButtonsDown(&pad), padGetButtons(&pad), stick.x, stick.y,
            HidNpadButton_Up, HidNpadButton_Down, HidNpadButton_Left, HidNpadButton_Right)
            | navTouchButton(touch);
        if (statusFrames > 0) --statusFrames;                          // expire the failure notice

        // Handle delete confirmation dialog
        if (showDeleteConfirmation) {
            // Tappable buttons (captured last frame): A = Delete, B = Cancel.
            if (touch.justPressed()) {
                auto in = [&](const DlgBtn& b) {
                    return touch.startX() >= b.x && touch.startX() < b.x + b.w &&
                           touch.startY() >= b.y && touch.startY() < b.y + b.h &&
                           touch.x() >= b.x && touch.x() < b.x + b.w &&
                           touch.y() >= b.y && touch.y() < b.y + b.h;
                };
                if (in(deleteDeleteBtn))      kDown |= HidNpadButton_A;
                else if (in(deleteCancelBtn)) kDown |= HidNpadButton_B;
            }
            if (kDown & HidNpadButton_A) {
                // Confirm deletion
                deleteBackup(deleteConfirmationIndex);
                showDeleteConfirmation = false;
                deleteConfirmationIndex = -1;
                return;
            }
            if (kDown & HidNpadButton_B) {
                // Cancel deletion
                showDeleteConfirmation = false;
                deleteConfirmationIndex = -1;
                return;
            }
            return;  // Ignore other inputs while confirmation is shown
        }

        // Touch moves the PHYSICAL viewport, not the selected backup. Keeping these independent
        // prevents an entire row of visual scroll from snapping back when focus stays in view.
        const int count = static_cast<int>(backups.size());
        const int maxFirstRow = std::max(0, count - LIST_MAX_VISIBLE);
        backupFirstRow = std::clamp(backupFirstRow, 0, maxFirstRow);
        selectedIndex = std::clamp(selectedIndex, 0, std::max(0, count - 1));
        const int listY = CARD_Y + 62;
        const int listX = CARD_X + 14;
        const int listW = CARD_W - 28;
        if (maxFirstRow > 0) {
            backupScroll.updateVertical(
                touch, listX, listY, listW, LIST_MAX_VISIBLE * LIST_ROW_H,
                LIST_ROW_H, backupFirstRow, maxFirstRow + 1);
        } else {
            backupScroll.stop();
        }

        // The highlighted backup belongs to controller/tap focus, not the finger-scrolled
        // viewport. Keep it unchanged even when its row scrolls out of view. An off-screen
        // A/X action is revealed below instead of silently targeting a different backup.

        // Hit-test the same pixels that were rendered: account for residual scroll, the 10px
        // row gap, and viewport clipping. Both contact and release must remain on one visible tile.
        if (touch.justTapped()) {
            const int listBottom = listY + LIST_MAX_VISIBLE * LIST_ROW_H;
            const int liveOffset = backupScroll.offset();
            const int drawFirst = std::max(0, backupFirstRow - 1);
            const int drawLast = std::min(count, backupFirstRow + LIST_MAX_VISIBLE + 1);
            for (int i = drawFirst; i < drawLast; ++i) {
                const int itemY = listY + (i - backupFirstRow) * LIST_ROW_H + liveOffset;
                const int top = std::max(listY, itemY);
                const int bottom = std::min(listBottom, itemY + LIST_ROW_H - 10);
                const auto insideTile = [&](int px, int py) {
                    return px >= listX && px < listX + listW &&
                           py >= top && py < bottom;
                };
                if (top < bottom &&
                    insideTile(touch.startX(), touch.startY()) &&
                    insideTile(touch.x(), touch.y())) {
                    selectedIndex = i;
                    kDown |= HidNpadButton_A;
                    break;
                }
            }
        }

        if (kDown & HidNpadButton_B) {
            backupScroll.stop();
            goBack = true;
            return;
        }

        // Never select or delete a highlighted backup that is no longer on screen.
        // A real tile tap already sets selectedIndex to a visible row above; controller
        // A/X must first reveal the prior focus without changing backup identity.
        if ((kDown & (HidNpadButton_A | HidNpadButton_X)) && count > 0 &&
            (selectedIndex < backupFirstRow ||
             selectedIndex >= backupFirstRow + LIST_MAX_VISIBLE)) {
            backupScroll.stop();
            backupFirstRow = std::clamp(
                selectedIndex - LIST_MAX_VISIBLE / 2, 0, maxFirstRow);
            statusMessage = "Focused backup brought into view; press A or X again";
            statusFrames = 220;
            return;
        }

        if (kDown & HidNpadButton_X) {
            // Modal entry freezes the underlying list. Never suspend a coasting scroll and then
            // resume that stale momentum after the confirmation closes.
            if (selectedIndex > 0 && selectedIndex < (int)backups.size() &&
                !backups[selectedIndex].legacyUnscoped) {
                backupScroll.stop();
                showDeleteConfirmation = true;
                deleteConfirmationIndex = selectedIndex;
            }
        }

        const bool controllerMoved =
            (kDown & (HidNpadButton_Up | HidNpadButton_Down)) != 0;
        if (controllerMoved) backupScroll.stop();

        if (kDown & HidNpadButton_Up) {
            if (selectedIndex > 0) --selectedIndex;
        }
        if (kDown & HidNpadButton_Down) {
            if (selectedIndex < count - 1) ++selectedIndex;
        }
        if (controllerMoved) {
            // D-pad / stick navigation keeps its focused row visible without fighting a drag.
            if (selectedIndex < backupFirstRow)
                backupFirstRow = selectedIndex;
            else if (selectedIndex >= backupFirstRow + LIST_MAX_VISIBLE)
                backupFirstRow = selectedIndex - LIST_MAX_VISIBLE + 1;
        }

        if (kDown & HidNpadButton_A) {
            if (selectedIndex == 0) {
                if (!namespaceReady) {
                    reportFailure("No safe profile/exact-game backup namespace is available.");
                    return;
                }
                createNewBackup = true;
                backupSelected = true;
            } else {
                const auto& chosen = backups[selectedIndex];
                if (chosen.legacyUnscoped) {
                    reportFailure(
                        "Legacy backup ownership is unknown; explicit import/assignment is required.");
                    return;
                }
                if (chosen.path.empty() || !PokeBank::Paths::isOwnedPath(chosen.path)) {
                    reportFailure("Backup path failed the PokeBank-owned path policy.");
                    return;
                }
                createNewBackup = false;
                selectedBackupPath = chosen.path;
                backupSelected = true;
            }
        }
    }

    void BackupSelectionScreen::draw(PKSEFramebuffer& fb) {
        drawAppBackdrop(fb);
        drawTitleBar(fb, titleName);

        // Leave a real gap under the card so the nav bar's shadow falls on the background, not on
        // the card edge. (Was a bare "- 46", which sat flush against the taller bar.)
        const int cardH = fb.getHeight() - CARD_Y - kNavBarH - 12;
        fb.drawCard(CARD_X, CARD_Y, CARD_W, cardH);
        fb.drawText(CARD_X + 18, CARD_Y + 14, "Save Backups", Colors::Text, TextStyle::Heading);
        fb.drawHDivider(CARD_X + 18, CARD_Y + 50, CARD_W - 36);

        drawBackupList(fb);

        if (selectedIndex > 0 && selectedIndex < (int)backups.size() &&
            backups[selectedIndex].legacyUnscoped) {
            drawNavBar(fb, {{"Up/Down", "Choose"}, {"A", "Ownership Info"}, {"B", "Back"}});
        } else if (selectedIndex > 0) {
            drawNavBar(fb, {{"Up/Down", "Choose"}, {"A", "Select"}, {"X", "Delete"}, {"B", "Back"}});
        } else {
            drawNavBar(fb, {{"Up/Down", "Choose"}, {"A", "Select"}, {"B", "Back"}});
        }

        // Transient failure notice, centred just above the nav bar (same treatment as the editor's).
        if (statusFrames > 0 && !statusMessage.empty()) {
            int tw, th; fb.measureText(statusMessage, tw, th);
            const int padX = 18, bw = tw + padX * 2, bh = th + 14;
            const int bx = (fb.getWidth() - bw) / 2, by = fb.getHeight() - kNavBarH - bh - 12;
            fb.drawSoftShadow(bx, by, bw, bh, 8);
            fb.drawFilledRoundedRect(bx, by, bw, bh, 8, Colors::Panel);
            fb.drawRoundedRect(bx, by, bw, bh, 8, Colors::Warning, 2);
            fb.drawText(bx + padX, by + 7, statusMessage, Colors::Text);
        }

        if (showDeleteConfirmation) {
            drawDeleteConfirmation(fb);
        }
    }

    void BackupSelectionScreen::drawBackupList(PKSEFramebuffer& fb) {
        const int startY = CARD_Y + 62;
        const int total = (int)backups.size();
        const int first = std::clamp(backupFirstRow, 0, std::max(0, total - LIST_MAX_VISIBLE));
        const int liveOffset = backupScroll.offset();
        const int drawFirst = std::max(0, first - 1);
        const int drawLast = std::min(total, first + LIST_MAX_VISIBLE + 1);
        fb.setClipRect(CARD_X + 14, startY, CARD_W - 28, LIST_MAX_VISIBLE * LIST_ROW_H);
        for (int i = drawFirst; i < drawLast; i++) {
            const int itemY = startY + (i - first) * LIST_ROW_H + liveOffset;
            const bool selected = (i == selectedIndex);
            // The first row is the "create new backup" action — accent it to stand out.
            drawHomeTile(fb, CARD_X + 14, itemY, CARD_W - 28, LIST_ROW_H - 10,
                         backups[i].displayName, selected, (i == 0));
        }
        fb.clearClip();
        // Scrollbar follows the exact residual pixel offset instead of snapping by whole rows.
        drawScrollbar(fb, CARD_X + CARD_W - 10, startY, LIST_MAX_VISIBLE * LIST_ROW_H,
                      total * LIST_ROW_H, first * LIST_ROW_H - liveOffset);
    }

    void BackupSelectionScreen::drawDeleteConfirmation(PKSEFramebuffer& fb) {
        constexpr int w = 560, h = 248;
        const int x = (fb.getWidth() - w) / 2;
        const int y = (fb.getHeight() - h) / 2;

        int cy = Dialogs::drawDialogFrame(fb, x, y, w, h, "Delete Backup?", Colors::Warning);
        fb.drawText(x + 24, cy, "This action cannot be undone.", Colors::TextDim);
        if (deleteConfirmationIndex >= 0 && deleteConfirmationIndex < (int)backups.size()) {
            fb.drawText(x + 24, cy + 28, backups[deleteConfirmationIndex].displayName, Colors::Text);
        }

        // Buttons carry their glyph (B: Cancel, A: Delete); Delete stays red. Rects are stashed so
        // update() can hit-test taps next frame.
        const int bh = TouchTargetMin, by = y + h - bh - 16, bw = (w - 48 - 16) / 2;
        const int cancelX = x + 24, delX = x + w - 24 - bw;
        drawGlyphButton(fb, cancelX, by, bw, bh, "B", "Cancel", Colors::PanelAlt);
        drawGlyphButton(fb, delX,    by, bw, bh, "A", "Delete", Colors::Red, Colors::White);
        deleteCancelBtn = { cancelX, by, bw, bh };
        deleteDeleteBtn = { delX,    by, bw, bh };
    }

    void BackupSelectionScreen::deleteBackup(int index) {
        if (index <= 0 || index >= (int)backups.size() || backups[index].legacyUnscoped) {
            return;  // Action row, invalid entry, or ownership-unknown legacy evidence.
        }

        const std::string& backupPath = backups[index].path;
        if (backupPath.empty() || !PokeBank::Paths::isOwnedPath(backupPath)) return;

        // Delete only a profile/exact-game scoped workspace selected through normal navigation.
        if (deleteDirectoryRecursive(backupPath.c_str())) {
            // Remove from backups list
            backups.erase(backups.begin() + index);

            // A shorter list must not leave the highlight or viewport beyond its final row.
            selectedIndex = std::clamp(selectedIndex, 0, static_cast<int>(backups.size()) - 1);
            backupFirstRow = std::clamp(
                backupFirstRow, 0,
                std::max(0, static_cast<int>(backups.size()) - LIST_MAX_VISIBLE));
            backupScroll.stop();
        }
    }
}
