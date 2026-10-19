#include <algorithm>
#include <cstdio>

#include "UI/BackupSelectionScreen.h"
#include "UI/Common.h"
#include "UI/ScreenChrome.h"
#include "UI/SharedEditorChrome.h"
#include "UI/SharedMenuGeometry.h"
#include "UI/HomeQuitConfirm.h"
#include "UI/TouchInput.h"
#include "UI/Dialogs/DialogFrame.h"
#include "Utils/HelperUtilities.h"
#include "Utils/FileUtilities.h"
#include "Utils/PokeBankPaths.h"
#include "Globals.h"
#include "Games/GameIdentity.h"

using namespace Utils;

namespace UI {
    // UI Layout constants: the accepted hardware pixels, written out. SharedMenuGeometry's
    // BackupMenu rule records exactly these values (the same contract as the emulator Backups
    // list), and the static_assert below fails the build if the screen and the rule ever diverge.
    namespace BackupGeometry = PokeBank::UIModel::SharedMenuGeometry;
    constexpr BackupGeometry::Rule kBackupRule = BackupGeometry::rule(BackupGeometry::MenuType::BackupMenu);
    constexpr int CARD_X = 40;
    constexpr int CARD_Y = 84;
    constexpr int CARD_W = 1200;
    constexpr int LIST_ROW_H = 62;
    constexpr int LIST_MAX_VISIBLE = 8;   // rows that fit in the card before scrolling
    constexpr int LIST_TOP = CARD_Y + 62;
    constexpr int LIST_X = CARD_X + 14;
    constexpr int LIST_W = CARD_W - 28;
    static_assert(CARD_W == kBackupRule.width && CARD_X == (BackupGeometry::kScreenW - kBackupRule.width) / 2 &&
                      LIST_ROW_H == kBackupRule.rowHeight && LIST_MAX_VISIBLE == kBackupRule.maxVisibleRows &&
                      LIST_TOP == CARD_Y + kBackupRule.headerH && LIST_X == CARD_X + kBackupRule.padX &&
                      LIST_W == CARD_W - 2 * kBackupRule.padX && kBackupRule.rowGap == 10,
                  "Save Backups and SharedMenuGeometry's BackupMenu rule must stay one contract");

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
        // A modal owns its own A/B touch rectangles. Never let the underlying
        // footer synthesize Delete while the confirmation dialog is on top.
        // Physical A/B controller buttons still use the normal pad path.
        const u64 footerTouch = showDeleteConfirmation ? 0 : navTouchButton(touch);
        u64 kDown = controllerNavigation.apply(
            padGetButtonsDown(&pad), padGetButtons(&pad), stick.x, stick.y,
            HidNpadButton_Up, HidNpadButton_Down, HidNpadButton_Left, HidNpadButton_Right)
            | footerTouch;
        if (statusFrames > 0) --statusFrames;                          // expire the failure notice

        // Handle delete confirmation dialog
        if (showDeleteConfirmation) {
            // Deleting a backup is irreversible, so it is never on A (P0 #320): the same
            // Y-confirms rule as Release / Discard. Tappable buttons (captured last frame):
            // Y = Delete, B = Cancel.
            // Explicit tap terminology: justTapped() is the same clean-release
            // gesture historically exposed as justPressed(). Never use
            // justTouchedDown() to confirm deleting app-owned backup content.
            if (touch.justTapped()) {
                auto in = [&](const DlgBtn& b) {
                    return touch.startX() >= b.x && touch.startX() < b.x + b.w &&
                           touch.startY() >= b.y && touch.startY() < b.y + b.h &&
                           touch.x() >= b.x && touch.x() < b.x + b.w &&
                           touch.y() >= b.y && touch.y() < b.y + b.h;
                };
                if (in(deleteDeleteBtn))      kDown |= HidNpadButton_Y;
                else if (in(deleteCancelBtn)) kDown |= HidNpadButton_B;
            }
            // One host-tested decision table (HomeQuitConfirm.h): only an explicit Y deletes,
            // A and B cancel, and an ambiguous A+Y / B+Y frame resolves to Cancel.
            using PokeBank::UIModel::HomeQuit::Decision;
            const Decision decision = PokeBank::UIModel::HomeQuit::decide(kDown);
            if (decision == Decision::Quit) {
                deleteBackup(deleteConfirmationIndex);
                showDeleteConfirmation = false;
                deleteConfirmationIndex = -1;
                return;
            }
            if (decision == Decision::Stay) {
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
        const int listY = LIST_TOP;
        const int listX = LIST_X;
        const int listW = LIST_W;
        // The shared scrollbar runs first: a held thumb owns the frame's touch, so the list
        // neither finger-scrolls nor taps the tile underneath it.
        const auto backupBar = SharedScrollbar::handleTouch(backupScrollbar, SharedScrollbar::sample(touch),
            SharedScrollbar::pixels(count * LIST_ROW_H, LIST_MAX_VISIBLE * LIST_ROW_H,
                                    backupFirstRow * LIST_ROW_H));
        if (backupBar.changed) {
            backupFirstRow = std::clamp((backupBar.scroll + LIST_ROW_H / 2) / LIST_ROW_H, 0, maxFirstRow);
            backupScroll.stop();
        }
        if (maxFirstRow > 0 && !backupBar.consumed) {
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
            // A release that ends a scrollbar drag or track tap selects no tile.
            const int drawLast = backupBar.consumed ? drawFirst
                : std::min(count, backupFirstRow + LIST_MAX_VISIBLE + 1);
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
            // The shared green-A Select / red-B Cancel picker footer (SharedEditorChrome).
            SharedEditorChrome::drawPickerFooter(fb, {.browse = true, .page = false, .xLabel = "Delete"});
        } else {
            SharedEditorChrome::drawPickerFooter(fb, {.browse = true, .page = false});
        }

        // Transient failure notice, centred just above the nav bar (same treatment as the editor's).
        if (statusFrames > 0 && !statusMessage.empty()) {
            int tw, th; fb.measureText(statusMessage, tw, th);
            tw = drawGlyphSentence(fb, 0, 0, statusMessage, Colors::Text, TextStyle::Body, true);
            const int padX = 18, bw = tw + padX * 2, bh = th + 14;
            const int bx = (fb.getWidth() - bw) / 2, by = fb.getHeight() - kNavBarH - bh - 12;
            fb.drawSoftShadow(bx, by, bw, bh, 8);
            fb.drawFilledRoundedRect(bx, by, bw, bh, 8, Colors::Panel);
            fb.drawRoundedRect(bx, by, bw, bh, 8, Colors::Warning, 2);
            drawGlyphSentence(fb, bx + padX, by + 7, statusMessage, Colors::Text);   // "press A or X" as badges
        }

        if (showDeleteConfirmation) {
            drawDeleteConfirmation(fb);
        }
    }

    void BackupSelectionScreen::drawBackupList(PKSEFramebuffer& fb) {
        const int startY = LIST_TOP;
        const int total = (int)backups.size();
        const int first = std::clamp(backupFirstRow, 0, std::max(0, total - LIST_MAX_VISIBLE));
        const int liveOffset = backupScroll.offset();
        const int drawFirst = std::max(0, first - 1);
        const int drawLast = std::min(total, first + LIST_MAX_VISIBLE + 1);
        fb.setClipRect(LIST_X, startY, LIST_W, LIST_MAX_VISIBLE * LIST_ROW_H);
        for (int i = drawFirst; i < drawLast; i++) {
            const int itemY = startY + (i - first) * LIST_ROW_H + liveOffset;
            const bool selected = (i == selectedIndex);
            // The first row is the "create new backup" action — accent it to stand out.
            drawHomeTile(fb, LIST_X, itemY, LIST_W, LIST_ROW_H - 10,
                         backups[i].displayName, selected, (i == 0));
        }
        fb.clearClip();
        // Shared draggable scrollbar; follows the exact residual pixel offset, not whole rows.
        drawScrollbar(fb, backupScrollbar, CARD_X + CARD_W - 13, startY, LIST_MAX_VISIBLE * LIST_ROW_H,
                      total * LIST_ROW_H, first * LIST_ROW_H - liveOffset);
    }

    void BackupSelectionScreen::drawDeleteConfirmation(PKSEFramebuffer& fb) {
        // The modal owns touch: the covered backup-list footer is dropped (P0 #320 layering), and
        // only the Cancel / Delete buttons drawn below publish targets.
        beginTopTouchLayer();
        constexpr int w = 560, h = 248;
        const int x = (fb.getWidth() - w) / 2;
        const int y = (fb.getHeight() - h) / 2;

        int cy = Dialogs::drawDialogFrame(fb, x, y, w, h, "Delete this backup?", Colors::Warning);
        fb.drawText(x + 24, cy, "This action cannot be undone.", Colors::TextDim);
        if (deleteConfirmationIndex >= 0 && deleteConfirmationIndex < (int)backups.size()) {
            fb.drawText(x + 24, cy + 28, backups[deleteConfirmationIndex].displayName, Colors::Text);
        }

        // Buttons carry their glyph (B: Cancel, Y: Delete); Delete stays red. Rects are stashed so
        // update() can hit-test taps next frame. The footer is the shared confirm footer.
        const int bh = TouchTargetMin, by = y + h - bh - 16, bw = (w - 48 - 16) / 2;
        const int cancelX = x + 24, delX = x + w - 24 - bw;
        SharedEditorChrome::drawFooter(fb, SharedEditorChrome::Presentation::confirmFooter(
                                               "Y", "Delete", "A", "Cancel"));
        drawGlyphButton(fb, cancelX, by, bw, bh, "B", "Cancel", Colors::PanelAlt);
        drawGlyphButton(fb, delX,    by, bw, bh, "Y", "Delete", Colors::Red, Colors::White);
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
