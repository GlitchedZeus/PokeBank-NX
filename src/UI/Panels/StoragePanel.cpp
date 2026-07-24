#include <algorithm>
#include <array>
#include <memory>
#include <string>

#include "UI/Panels/StoragePanel.h"
#include "UI/TrainerViewScreen.h"
#include "UI/Common.h"
#include "UI/PKSEFramebuffer.h"
#include "UI/SpriteManager.h"
#include "UI/ScreenChrome.h"          // drawGlyphButton -- on-button controller glyphs
#include "UI/Dialogs/DialogFrame.h"
#include "Names/FormNames.h"          // getDisplayName -- variant prefix ("Alolan Raichu", etc.)
#include "Trainer/Trainer.h"
#include "Trainer/Bank.h"
#include "Pokemon/Pokemon.h"

using namespace Trainer;

namespace UI {
namespace Panels {

    namespace {
        // Cursor-mode colors: red (Menu), blue (Move), green (Multi).
        constexpr Color MenuColor(232, 92, 92);
        constexpr Color MoveColor(86, 148, 244);
        constexpr Color MultiColor(96, 205, 128);

        Color cursorColorFor(TrainerViewScreen::CursorMode mode) {
            switch (mode) {
                case TrainerViewScreen::CursorMode::Move:  return MoveColor;
                case TrainerViewScreen::CursorMode::Multi: return MultiColor;
                case TrainerViewScreen::CursorMode::Menu:
                default:                                   return MenuColor;
            }
        }

        const char* modeName(TrainerViewScreen::CursorMode mode) {
            switch (mode) {
                case TrainerViewScreen::CursorMode::Move:  return "MOVE";
                case TrainerViewScreen::CursorMode::Multi: return "MULTI";
                case TrainerViewScreen::CursorMode::Menu:
                default:                                   return "MENU";
            }
        }

        // Draw one Pokemon (sprite + shiny/party markers) centered on a slot disc of radius discR.
        void drawSlotDisc(TrainerViewScreen& screen, PKSEFramebuffer& fb,
                          const Pokemon::Pokemon* pk, bool savePane, int boxIndex, int slotIndex,
                          int cx, int cy, int discR) {
            if (!pk || pk->speciesID() == 0) return;

            const std::string speciesName(pk->species());
            const bool isShiny = pk->isShiny(pk->id32(), speciesName);

            const int sz = static_cast<int>(discR * 1.75);
            if (pk->isEgg()) {
                fb.drawEgg(cx, cy, sz);  // eggs show as an egg in the grid
            } else {
                Sprite* sprite = SpriteManager::getIconSprite(pk->speciesID(), pk->form(), isShiny);
                if (sprite && sprite->data)
                    fb.drawImageScaled(cx - sz / 2, cy - sz / 2, sprite->width, sprite->height, sz, sz, sprite->data, sprite->channels);
            }

            if (isShiny) fb.drawShinyMark(cx + discR - 15, cy - discR + 1, 15, Colors::ShinyStar);  // top-right

            // Party-membership badge (save pane only): partner heart (top-left) or party number (bottom-left).
            if (savePane) {
                if (screen.trainer.isStarterPokemon(boxIndex, slotIndex)) {
                    fb.drawSymbol(cx - discR, cy - discR + 2, "\xE2\x99\xA5", Colors::PartnerHeart);
                } else {
                    int partyPos = screen.trainer.getPartyPosition(boxIndex, slotIndex);
                    if (partyPos > 0) {
                        // Gold badge + dark digit at the disc's bottom-left, legible on any sprite/theme.
                        const std::string n = std::to_string(partyPos);
                        const int bx = cx - discR + 9, by = cy + discR - 9;
                        fb.drawFilledCircle(bx, by, 9, Colors::PartyBadge);
                        int tw, th; fb.measureText(n, tw, th, TextStyle::Caption);
                        fb.drawText(bx - tw / 2, by - th / 2, n, Colors::PartyBadgeText, TextStyle::Caption);
                    }
                }
            }
        }

        // Draws one storage pane in the HOME box style (rounded card + indigo header band + disc
        // grid). Reports the cursor disc's pixel rect via outCursorRect (to lift a carried Pokemon).
        void drawPane(TrainerViewScreen& screen, PKSEFramebuffer& fb,
                     int px, int py, int pw, int ph, bool savePane,
                     int boxIndex, int cursorSlot, bool focused, bool entered,
                     int cols, int rows, int slotsPerBox,
                     const std::string& label, int boxCount, int outCursorRect[4]) {
            fb.drawFilledRoundedRect(px, py, pw, ph, 16, Colors::Panel);

            // Header band (rounded top). Focused pane = accent indigo; unfocused = dim.
            constexpr int headerH = 44;
            const Color band = focused ? Colors::AccentDim : Colors::Border;
            fb.drawFilledRoundedRect(px, py, pw, headerH, 16, band);
            fb.drawFilledRect(px, py + headerH - 16, pw, 16, band);

            int lw, lh; fb.measureText(label, lw, lh, TextStyle::Body);
            const int pillW = std::min(pw - 120, lw + 40), pillH = 28;
            const int pillX = px + (pw - pillW) / 2, pillY = py + (headerH - pillH) / 2;
            // Amber pill when the header itself is focused (navigate up to it, or tap it, to rename
            // this box). cursorSlot == -1 is the "header focused" sentinel.
            const bool headerFocused = focused && entered && cursorSlot == -1;
            fb.drawPill(pillX, pillY, pillW, pillH, headerFocused ? Colors::Primary : Colors::Panel);
            fb.drawText(px + (pw - lw) / 2, pillY + (pillH - lh) / 2, label, headerFocused ? Colors::PrimaryText : Colors::Text);

            const int arrowY = py + (headerH - lh) / 2;
            fb.drawSymbol(px + 18, arrowY, "\xE2\x97\x80", focused ? Colors::Text : Colors::TextDim);        // ◀
            fb.drawSymbol(px + pw - 32, arrowY, "\xE2\x96\xB6", focused ? Colors::Text : Colors::TextDim);   // ▶
            // Tappable box arrows (special slot ids -2 = prev box, -3 = next box) and the name pill
            // (-4 = rename this box). The name target sits between the two arrow zones.
            screen.storageTouchTargets.push_back({ savePane ? 0 : 1, boxIndex, -2, px, py, 64, headerH });
            screen.storageTouchTargets.push_back({ savePane ? 0 : 1, boxIndex, -3, px + pw - 64, py, 64, headerH });
            screen.storageTouchTargets.push_back({ savePane ? 0 : 1, boxIndex, -4, px + 64, py, pw - 128, headerH });
            std::string counter = std::to_string(boxIndex + 1) + " / " + std::to_string(boxCount);
            int cw, ch; fb.measureText(counter, cw, ch, TextStyle::Caption);
            fb.drawText(px + pw - 44 - cw, py + (headerH - ch) / 2, counter, focused ? Colors::Text : Colors::TextDim, TextStyle::Caption);

            const int gridTop = py + headerH + 8;
            const int gridX = px + 14, gridW = pw - 28, gridH = ph - headerH - 18;
            const int colPitch = gridW / cols, rowPitch = gridH / rows;
            int discR = std::min(colPitch, rowPitch) / 2 - 4;
            if (discR < 12) discR = 12;

            const int thisPane = savePane ? 0 : 1;
            auto isMultiSel = [&](int i) {
                for (const auto& s : screen.multiSel)
                    if (s.pane == thisPane && s.box == boxIndex && s.slot == i) return true;
                return false;
            };
            const Color cur = cursorColorFor(screen.cursorMode);

            for (int i = 0; i < slotsPerBox; ++i) {
                const int row = i / cols, col = i % cols;
                const int cellX = gridX + col * colPitch, cellY = gridTop + row * rowPitch;
                const int cx = cellX + colPitch / 2, cy = cellY + rowPitch / 2;

                const bool isCursor = focused && entered && i == cursorSlot;
                const bool selected = isMultiSel(i);

                // Whole-cell touch target (consumed next frame in update()).
                screen.storageTouchTargets.push_back({thisPane, boxIndex, i, cellX, cellY, colPitch, rowPitch});

                const Pokemon::Pokemon* pk = savePane ? screen.trainer.boxes[boxIndex][i].get()
                                                      : (screen.bank ? screen.bank->boxes[boxIndex][i].get() : nullptr);
                const bool empty = !pk || pk->speciesID() == 0;

                if (isCursor) fb.drawFilledCircle(cx, cy, discR + 5, Color(cur.r, cur.g, cur.b, 70));  // mode-color halo
                fb.drawFilledCircle(cx, cy, discR, empty ? Colors::Panel : Colors::PanelAlt);
                if (empty) fb.drawCircle(cx, cy, discR, Colors::Border, 1);

                drawSlotDisc(screen, fb, pk, savePane, boxIndex, i, cx, cy, discR);

                if (selected) fb.drawCircle(cx, cy, discR + 2, MultiColor, 3);      // green multi-select ring
                if (isCursor) {
                    fb.drawCircle(cx, cy, discR + 2, cur, 3);                        // mode-color cursor ring
                    outCursorRect[0] = cx - discR; outCursorRect[1] = cy - discR;
                    outCursorRect[2] = 2 * discR;  outCursorRect[3] = 2 * discR;
                }
            }
        }
    }

    void drawStorageView(TrainerViewScreen& screen, PKSEFramebuffer& fb, int x, int y, int width, int height) {
        if (!screen.bank) {
            fb.drawCard(x, y, width, height);
            fb.drawText(x + 16, y + 70, "Bank unavailable", Colors::TextDim);
            return;
        }

        const bool entered = screen.detailViewActive;  // cursor only shows once the view is entered
        screen.storageTouchTargets.clear();             // rebuilt each frame by drawPane below

        constexpr int gap = 16;
        constexpr int infoH = 60;
        const int paneW = (width - gap) / 2;
        const int paneH = height - infoH - 8;

        const int saveSlots = static_cast<int>(screen.trainer.getSlotsPerBox());
        const int saveCols = (saveSlots == 25) ? 5 : 6;
        const int bankSlots = static_cast<int>(Bank::BANK_SLOTS_PER_BOX);
        const int bankCols = 6;
        const int rows = 5;

        const bool saveFocused = screen.storageFocusPane == 0;
        const bool bankFocused = screen.storageFocusPane == 1;

        std::string saveLabel = screen.stSaveBox < static_cast<int>(screen.trainer.boxNames.size())
            ? screen.trainer.boxNames[screen.stSaveBox]
            : ("Box " + std::to_string(screen.stSaveBox + 1));
        std::string bankLabel = screen.bank->boxDisplayName(screen.stBankBox);

        int saveCursorRect[4] = {-1, -1, -1, -1};
        int bankCursorRect[4] = {-1, -1, -1, -1};

        drawPane(screen, fb, x, y, paneW, paneH, /*savePane*/true,
                 screen.stSaveBox, screen.stSaveSlot, saveFocused, entered,
                 saveCols, rows, saveSlots, saveLabel, static_cast<int>(screen.trainer.getBoxCount()), saveCursorRect);

        drawPane(screen, fb, x + paneW + gap, y, paneW, paneH, /*savePane*/false,
                 screen.stBankBox, screen.stBankSlot, bankFocused, entered,
                 bankCols, rows, bankSlots, bankLabel, static_cast<int>(Bank::BANK_BOX_COUNT), bankCursorRect);

        // Carried Pokemon: lift it over the cursor with a soft shadow (no harsh outline).
        if (entered && screen.heldPokemon) {
            const int* r = saveFocused ? saveCursorRect : bankCursorRect;
            if (r[0] >= 0) {
                const int sp = (r[2] < r[3] ? r[2] : r[3]) - 6;
                const int hx = r[0] + (r[2] - sp) / 2;
                const int hy = r[1] - 12;  // lifted so it reads as "in hand"
                fb.drawFilledEllipse(r[0] + r[2] / 2, r[1] + r[3] - 8, sp / 2 - 2, 5, Color(0, 0, 0, 90));
                if (screen.heldPokemon->isEgg()) {
                    fb.drawEgg(hx + sp / 2, hy + sp / 2, sp);  // held egg shows as an egg
                } else {
                    bool shiny = screen.heldPokemon->isShiny(screen.heldPokemon->id32(), std::string(screen.heldPokemon->species()));
                    Sprite* sprite = SpriteManager::getIconSprite(screen.heldPokemon->speciesID(), screen.heldPokemon->form(), shiny);
                    if (sprite && sprite->data)
                        fb.drawImageScaled(hx, hy, sprite->width, sprite->height, sp, sp, sprite->data, sprite->channels);
                }
            }
        }

        // --- Info strip ---
        const int iy = y + paneH + 8;
        fb.drawCard(x, iy, width, infoH);

        if (!entered) {
            fb.drawText(x + 16, iy + infoH / 2 - 11, "Press A to open storage", Colors::TextDim, TextStyle::Body);
            return;
        }

        // Mode swatch (dot) + name.
        const Color modeCol = cursorColorFor(screen.cursorMode);
        fb.drawFilledCircle(x + 22, iy + infoH / 2, 8, modeCol);
        fb.drawText(x + 38, iy + infoH / 2 - 11, modeName(screen.cursorMode), Colors::Text, TextStyle::Body);

        // Right side: multi-selection count (Multi mode).
        if (!screen.multiSel.empty()) {
            std::string sel = std::to_string(screen.multiSel.size()) + " selected";
            int sw, sh; fb.measureText(sel, sw, sh);
            fb.drawText(x + width - 16 - sw, iy + infoH / 2 - 11, sel, MultiColor, TextStyle::Body);
        }

        // Middle: the held Pokemon, else the one under the focused cursor. When the box-name header
        // is focused (slot == -1) there is no cursor mon, so prompt the rename instead of indexing.
        const int focusSlot = saveFocused ? screen.stSaveSlot : screen.stBankSlot;
        const Pokemon::Pokemon* focus = screen.heldPokemon.get();
        if (!focus && focusSlot >= 0) {
            focus = saveFocused ? screen.trainer.boxes[screen.stSaveBox][focusSlot].get()
                                : screen.bank->boxes[screen.stBankBox][focusSlot].get();
        }

        const int tx = x + 120;
        if (focusSlot == -1 && !screen.heldPokemon) {
            fb.drawText(tx, iy + infoH / 2 - 11, "Box name selected - press A to rename", Colors::TextDim, TextStyle::Body);
        } else if (focus && focus->speciesID() != 0) {
            std::string name(focus->species());
            const bool shiny = focus->isShiny(focus->id32(), name);
            std::string display = Names::getDisplayName(focus->speciesID(), focus->form(), name);
            std::string line = (screen.heldPokemon ? "Holding: " : "") + display + "   Lv. " + std::to_string(focus->level());
            fb.drawText(tx, iy + infoH / 2 - 11, line, Colors::Text, TextStyle::Body);
            int lw, lh; fb.measureText(line, lw, lh, TextStyle::Body);
            int mx = tx + lw + 10;
            const char* g = focus->genderSymbol();
            if (g[0] != '\0') {
                fb.drawSymbol(mx, iy + infoH / 2 - 11, g, (std::string(g) == "\xE2\x99\x82") ? Colors::Blue : Colors::Magenta);
                mx += 20;
            }
            if (shiny) fb.drawShinyMark(mx, iy + infoH / 2 - 11, 16, Colors::ShinyStar);
        } else {
            fb.drawText(tx, iy + infoH / 2 - 11, "Empty slot", Colors::TextDim, TextStyle::Body);
        }
    }

    namespace {
        // A small centered popup menu (scrim + card + title + item list). Each row is registered as
        // a touch button (id = item index) so a tap selects + confirms it.
        void drawPopupMenu(TrainerViewScreen& screen, PKSEFramebuffer& fb, const std::string& title,
                           const char* const* items, int count, int sel, uint32_t disabledMask = 0) {
            // Touch-friendly sizing: 56px rows (TouchTargetMin) and a wide card so rows are easy to tap.
            constexpr int w = 440, rowH = TouchTargetMin;
            constexpr int headerH = 60;
            const int h = headerH + count * rowH + 14;
            const int x = (fb.getWidth() - w) / 2;
            const int y = (fb.getHeight() - h) / 2;
            constexpr int r = Dialogs::kDialogRadius;   // same card shape as every other modal
            fb.drawFilledRect(0, 0, fb.getWidth(), fb.getHeight(), Color(0, 0, 0, 130));
            fb.drawSoftShadow(x, y, w, h, r);
            fb.drawFilledRoundedRect(x, y, w, h, r, Colors::Panel);
            fb.drawRoundedRect(x, y, w, h, r, Colors::Border, 1);
            fb.drawText(x + 22, y + 16, title, Colors::Text, TextStyle::Heading);
            fb.drawFilledRect(x + r, y + headerH - 4, w - r * 2, 2, Colors::Accent);
            screen.touchButtons.clear();
            int ry = y + headerH;
            for (int i = 0; i < count; ++i) {
                const bool disabled = (disabledMask >> i) & 1u;
                const bool selRow = (i == sel) && !disabled;
                if (selRow) fb.drawSelectionHighlight(x + 10, ry + 3, w - 20, rowH - 6);
                int tw, th; fb.measureText(items[i], tw, th, TextStyle::Body);
                const Color rowColor = disabled ? Colors::Border : (selRow ? Colors::Text : Colors::TextDim);
                fb.drawText(x + 28, ry + (rowH - th) / 2, items[i], rowColor, TextStyle::Body);
                if (!disabled) screen.touchButtons.push_back({i, x + 10, ry, w - 20, rowH});  // greyed rows aren't tappable
                ry += rowH;
            }
        }
    }

    void drawStorageActionMenu(TrainerViewScreen& screen, PKSEFramebuffer& fb) {
        const Pokemon::Pokemon* pk = screen.storageSlot(screen.menuPane, screen.menuBox, screen.menuSlot).get();
        const std::string title = (pk && pk->speciesID() != 0)
            ? Names::getDisplayName(pk->speciesID(), pk->form(), pk->species()) : "Pokémon";
        static const char* const items[] = {"Move", "Edit", "Clone", "Release", "Cancel"};
        // A party-linked (LGPE) slot can be edited or cloned, but not moved or released -- grey those
        // two out. (Same lock rule as storageSlotLocked: save pane + a party member points here.)
        const bool locked = screen.menuPane == 0 &&
                            screen.trainer.getPartyPosition(screen.menuBox, screen.menuSlot) > 0;
        const uint32_t disabled = locked ? ((1u << 0) | (1u << 3)) : 0u;   // Move (0), Release (3)
        drawPopupMenu(screen, fb, title, items, 5, screen.storageMenuIndex, disabled);
    }

    void drawStorageGroupMenu(TrainerViewScreen& screen, PKSEFramebuffer& fb) {
        const std::string title = std::to_string(screen.multiSel.size()) + " selected";
        static const char* const items[] = {"Move to other side", "Release", "Clear selection", "Cancel"};
        drawPopupMenu(screen, fb, title, items, 4, screen.groupMenuIndex);
    }

    void drawStorageReleaseConfirm(TrainerViewScreen& screen, PKSEFramebuffer& fb) {
        std::string msg;
        if (screen.releaseGroup) {
            msg = "Release " + std::to_string(screen.multiSel.size()) + " Pokémon?";
        } else {
            const Pokemon::Pokemon* pk = screen.storageSlot(screen.releasePane, screen.releaseBox, screen.releaseSlot).get();
            const std::string who = (pk && pk->speciesID() != 0)
                ? Names::getDisplayName(pk->speciesID(), pk->form(), pk->species()) : "this Pokémon";
            msg = "Release " + who + "?";
        }
        constexpr int w = 540, h = 226;
        const int x = (fb.getWidth() - w) / 2;
        const int y = (fb.getHeight() - h) / 2;
        int cy = Dialogs::drawDialogFrame(fb, x, y, w, h, "Release", Colors::Red);
        fb.drawText(x + 28, cy, msg, Colors::Text);
        fb.drawText(x + 28, cy + 34, "This can't be undone.", Colors::TextDim, TextStyle::Caption);

        // Buttons carry their glyph (B: Cancel, A: Release); Release stays red.
        screen.touchButtons.clear();
        const int bw = 190, bh = TouchTargetMin, by = y + h - bh - 18;
        const int relX = x + w - bw - 20;    // right = Release (id 1 -> A)
        const int cancelX = relX - bw - 16;  // left  = Cancel  (id 0 -> B)
        drawGlyphButton(fb, cancelX, by, bw, bh, "B", "Cancel",  Colors::PanelAlt);
        screen.touchButtons.push_back({0, cancelX, by, bw, bh});
        drawGlyphButton(fb, relX,    by, bw, bh, "A", "Release", Colors::Red, Colors::White);
        screen.touchButtons.push_back({1, relX, by, bw, bh});
    }

    // Creator "Keep this new Pokemon?" confirm. Three glyph buttons (like the Edit Item dialog),
    // so each is one controller button rather than a Left/Right selector: B = Back (return to
    // editing), Y = Discard (remove it), A = Keep. ids 0 / 2 / 1 respectively.
    void drawCreatorKeepConfirm(TrainerViewScreen& screen, PKSEFramebuffer& fb) {
        constexpr int w = 540, h = 226;
        const int x = (fb.getWidth() - w) / 2;
        const int y = (fb.getHeight() - h) / 2;
        int cy = Dialogs::drawDialogFrame(fb, x, y, w, h, "New Pokémon", Colors::Primary);
        fb.drawText(x + 28, cy, "Keep this new Pokémon?", Colors::Text);
        fb.drawText(x + 28, cy + 34, "Discard removes it; Back keeps editing.", Colors::TextDim, TextStyle::Caption);

        screen.touchButtons.clear();
        const int cbw = 160, cbh = TouchTargetMin, cby = y + h - cbh - 18;
        drawGlyphButton(fb, x + 24,            cby, cbw, cbh, "B", "Back",    Colors::PanelAlt);
        screen.touchButtons.push_back({0, x + 24, cby, cbw, cbh});
        drawGlyphButton(fb, x + (w - cbw) / 2, cby, cbw, cbh, "Y", "Discard", Colors::PanelAlt);
        screen.touchButtons.push_back({2, x + (w - cbw) / 2, cby, cbw, cbh});
        drawGlyphButton(fb, x + w - 24 - cbw,  cby, cbw, cbh, "A", "Keep",    Colors::PanelAlt);
        screen.touchButtons.push_back({1, x + w - 24 - cbw, cby, cbw, cbh});
    }

    void drawLgpeMoveConfirm(TrainerViewScreen& screen, PKSEFramebuffer& fb) {
        constexpr int w = 580, h = 300;
        const int x = (fb.getWidth() - w) / 2;
        const int y = (fb.getHeight() - h) / 2;
        if (screen.moveConfirmGen3) {
            // Down-converting to Gen 3 rebuilds the PID to keep the nature (Gen 3 has no nature field),
            // which is destructive -- the more severe notice wins even when the move is ALSO an LGPE one.
            int cy = Dialogs::drawDialogFrame(fb, x, y, w, h, "Convert to Gen 3?", Colors::Warning);
            fb.drawText(x + 28, cy,       "Gen 3 has no nature field, so the PID is",    Colors::Text);
            fb.drawText(x + 28, cy + 28,  "regenerated to preserve it -- the result",    Colors::Text);
            fb.drawText(x + 28, cy + 56,  "may read as illegal, and nickname, ribbons",  Colors::Text);
            fb.drawText(x + 28, cy + 84,  "and held item are dropped. Can't be undone.", Colors::Text);
            fb.drawText(x + 28, cy + 124, "Not recommended. Continue anyway?",           Colors::TextDim, TextStyle::Caption);
        } else {
            int cy = Dialogs::drawDialogFrame(fb, x, y, w, h, "Let's Go transfer", Colors::Primary);
            fb.drawText(x + 28, cy,       "Moving to/from Let's Go resets AVs/EVs",  Colors::Text);
            fb.drawText(x + 28, cy + 28,  "to 0, removes held items, and drops any", Colors::Text);
            fb.drawText(x + 28, cy + 56,  "move it can't legally learn.",           Colors::Text);
            fb.drawText(x + 28, cy + 96,  "Continue with this transfer?",           Colors::TextDim, TextStyle::Caption);
        }

        // On-button glyphs (B: Cancel, A: Continue) -- a single button each, no Left/Right selector.
        screen.touchButtons.clear();
        const int bw = 190, bh = TouchTargetMin, by = y + h - bh - 18;
        const int contX = x + w - bw - 20;      // right = Continue (id 1 -> A)
        const int cancelX = contX - bw - 16;    // left  = Cancel   (id 0 -> B)
        drawGlyphButton(fb, cancelX, by, bw, bh, "B", "Cancel",   Colors::PanelAlt);
        screen.touchButtons.push_back({0, cancelX, by, bw, bh});
        drawGlyphButton(fb, contX,   by, bw, bh, "A", "Continue", Colors::PanelAlt);
        screen.touchButtons.push_back({1, contX, by, bw, bh});
    }

    void drawStorageExitConfirm(TrainerViewScreen& screen, PKSEFramebuffer& fb) {
        // The bank has unsaved changes; ask before leaving (PKSM-style). Bank saving is separate
        // from the game (X) save, so this is the bank's own persistence decision.
        static const char* const items[] = {"Save & Exit", "Discard changes", "Cancel"};
        drawPopupMenu(screen, fb, "Save bank changes?", items, 3, screen.storageExitConfirmIndex);
    }
}
}
