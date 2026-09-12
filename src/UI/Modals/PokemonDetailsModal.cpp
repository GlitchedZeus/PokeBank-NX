#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

#include "UI/Modals/PokemonDetailsModal.h"
#include "UI/TrainerViewScreen.h"
#include "UI/Common.h"
#include "UI/PKSEFramebuffer.h"
#include "UI/ScreenChrome.h"
#include "UI/SpriteManager.h"
#include "Trainer/Trainer.h"
#include "Utils/HelperUtilities.h"
#include "Pokemon/Pokemon.h"
#include "Pokemon/Pokemon1ReadOnly.h"
#include "Pokemon/Pokemon2ReadOnly.h"
#include "UI/LegacyPresentationRules.h"
#include "UI/Modals/Gen1PokemonDetailsModal.h"
#include "UI/Modals/Gen2PokemonDetailsModal.h"
#include "Pokemon/PokemonTypes.h"
#include "Pokemon/Experience.h"
#include "Pokemon/BaseStatsGen89.h"
#include "Names/FormNames.h"
#include "Pokemon/PersonalInfoTable.h"   // getPersonalInfo -> formCount (Form row/picker)
#include "Names/MoveNames.h"
#include "Names/LocationNames.h"
#include "Names/RibbonNames.h"
#include "Names/ItemNames.h"
#include "Enums/Ball.h"
#include "Enums/LanguageID.h"
#include "Enums/GameVersion.h"
#include "Legality/Legality.h"

using namespace Trainer;
using namespace Utils;

namespace UI {
namespace Modals {

    // HOME "Check Summary"-style editor page. Full-screen, three columns: the render + details
    // (left), the editable stat table + shiny/nature/gender (center — the navigable "Values" column),
    // and the moveset editor (right — moves + held item). Editing is handled in TrainerViewScreen; this
    // only draws + captures touch.
    void drawPokemonDetailsModal(TrainerViewScreen& screen, PKSEFramebuffer& fb) {
        const Pokemon::Pokemon* p = screen.detailsTargetPokemon();
        if (!p || p->speciesID() == 0) return;

        // Legacy formats have their own truthful layouts. Dispatch BEFORE the modern editor computes
        // Nature/Ability/PID/met/ribbon fields so neutral compatibility values can never be presented
        // as native Gen I/II data. Group checks are safe without RTTI because each read-only wrapper
        // owns a unique legacy group and the bridge constructs only that wrapper for the group.
        switch (pokemonDetailsLayoutFor(p->getGameGroup())) {
            case PokemonDetailsLayout::Generation1:
                drawGen1PokemonDetailsModal(
                    screen, fb, static_cast<const Pokemon::Pokemon1ReadOnly&>(*p));
                return;
            case PokemonDetailsLayout::Generation2:
                drawGen2PokemonDetailsModal(
                    screen, fb, static_cast<const Pokemon::Pokemon2ReadOnly&>(*p));
                return;
            case PokemonDetailsLayout::Modern:
                break;
        }

        const int W = fb.getWidth(), H = fb.getHeight();
        const bool isShiny = p->isShiny(p->id32(), p->species());
        const bool av = p->hasAwakeningValues();
        const int sel = screen.details.selectedField;   // 0-5 stats, 6 shiny, 7 nature, 8 gender, 9 level, 10-13 moves, 14 item
        const Legality::Report legalityRep = Legality::analyze(*p, p->getGameGroup());

        screen.touchButtons.clear();

        // Full-screen page over a dimmed box screen.
        fb.drawFilledRect(0, 0, W, H, Color(0, 0, 0, 130));
        fb.drawVerticalGradient(0, 0, W, H, Color(Colors::Background.r, Colors::Background.g, Colors::Background.b, 250),
                                Color(Colors::Background.r, Colors::Background.g, Colors::Background.b, 255));

        // ---- Top bar: name + gender + shiny + level + dex + close ----
        std::string name = utf16ToUtf8(p->nickname());
        if (name.empty()) name = std::string(p->species());
        // Regional/variant label ABOVE the name ("Combat Breed", "Alolan", "Hisuian", ...), so a
        // variant reads as what it is even when the mon is nicknamed. Only drawn when the form has a
        // name; the name drops a row to make space for it.
        const char* variant = Names::getFormName(p->speciesID(), p->form());
        int nameY = 16;
        if (variant[0] != '\0') { fb.drawText(28, 3, variant, Colors::Accent, TextStyle::Caption); nameY = 25; }
        fb.drawText(28, nameY, name, Colors::Text, TextStyle::Heading);
        int nW, nH; fb.measureText(name, nW, nH, TextStyle::Heading);
        int mx = 28 + nW + 12;
        const char* g = p->genderSymbol();
        if (g[0] != '\0') { fb.drawSymbol(mx, nameY + 6, g, (std::string(g) == "\xE2\x99\x82") ? Colors::Blue : Colors::Magenta); mx += 24; }
        if (isShiny) fb.drawShinyMark(mx, nameY + 4, 18, Colors::ShinyStar);

        std::string dex = std::to_string(p->speciesID());
        while (dex.size() < 3) dex = "0" + dex;
        // Box mons carry no party-stat block, so level() reads 0 for them in the packed formats
        // (SwSh/BDSP/SV/Z-A); fall back to the EXP-derived level (authoritative) for display.
        uint8_t subLvl = p->level();
        if (subLvl == 0) subLvl = Pokemon::getLevelFromExp(p->exp(), Pokemon::getGrowthRate(p->speciesID()));
        std::string sub = "Lv. " + std::to_string(subLvl) + "     No. " + dex;
        int sW, sH; fb.measureText(sub, sW, sH);
        fb.drawText(W - 90 - sW, 24, sub, Colors::TextDim);
        if (screen.details.readOnly) {
            const char* ro = "READ ONLY";
            int rw, rh; fb.measureText(ro, rw, rh, TextStyle::Caption);
            fb.drawText((W - rw) / 2, 22, ro, Colors::Accent, TextStyle::Caption);
        }
        fb.drawFilledRect(0, 60, W, 2, Colors::Accent);

        // Close button (top-right).
        fb.drawFilledCircle(W - 40, 30, 20, Colors::PanelAlt);
        fb.drawCircle(W - 40, 30, 20, Colors::Border, 1);
        fb.drawText(W - 47, 18, "\xC3\x97", Colors::Text, TextStyle::Heading);  // ×
        screen.touchButtons.push_back({ 99, W - 64, 6, 52, 52 });

        // "Unsaved changes" marker (edits are applied live; X commits them and clears this). Centered
        // at the top so it crowds neither the name nor the Lv/No. A being-CREATED mon is unsaved by
        // nature -- every field is -- so the marker is just noise there and is hidden.
        if (screen.pokemonEditDirty() && !screen.creator.editing) {
            const char* u = "Unsaved changes";
            int uw, uh; fb.measureText(u, uw, uh, TextStyle::Caption);
            fb.drawText((W - uw) / 2, 8, u, Colors::Warning, TextStyle::Caption);
        }

        const int colY = 72, colH = H - colY - kNavBarH - 6;   // leave room for the bottom nav bar

        // =========================== LEFT: render + details ===========================
        const int Lx = 24, Lw = 430;
        fb.drawFilledRoundedRect(Lx, colY, Lw, colH, 16, Colors::Panel);
        fb.drawRoundedRect(Lx, colY, Lw, colH, 16, Colors::Border, 1);

        // Render sprite (top of the details panel).
        const int renderSz = 150;
        Sprite* sprite = SpriteManager::getSprite(p->speciesID(), p->form(), isShiny);
        if (sprite && sprite->data) {
            fb.drawSpriteStaticContained(Lx + (Lw - renderSz) / 2, colY + 14,
                                         renderSz, renderSz, sprite->width, sprite->height,
                                         sprite->data, sprite->channels);
        }
        // Type badges under the render (icons, centered).
        {
            Pokemon::TypePair types = Pokemon::getPokemonTypes(p->speciesID(), p->form(), p->getGameGroup());
            Sprite* t1 = SpriteManager::getTypeSprite(types.type1);
            Sprite* t2 = Pokemon::hasSecondType(types) ? SpriteManager::getTypeSprite(types.type2) : nullptr;
            const int th = 22;
            int w1 = (t1 && t1->data) ? (t1->width * th) / t1->height : 0;
            int w2 = (t2 && t2->data) ? (t2->width * th) / t2->height : 0;
            int gap = (w2 > 0) ? 8 : 0;
            int tx = Lx + (Lw - (w1 + gap + w2)) / 2, tyy = colY + 14 + renderSz + 6;
            if (t1 && t1->data) { fb.drawImageScaled(tx, tyy, t1->width, t1->height, w1, th, t1->data, t1->channels); tx += w1 + gap; }
            if (t2 && t2->data) { fb.drawImageScaled(tx, tyy, t2->width, t2->height, w2, th, t2->data, t2->channels); }
        }

        // ----- Scrollable info column (below the sprite). Rows are TALL for easy touch; the column
        // clips + scrolls and auto-follows the selected field so it never runs off screen. -----
        char buf[160];
        const int RH = 40;                                     // row pitch -- big, touch-friendly targets
        const int contentTop = colY + 14 + renderSz + 40;
        const int legalityH = 34;                              // legality line is pinned below the scroll
        const int contentBottom = colY + colH - legalityH - 6;
        const int scroll = screen.details.leftScroll;
        int iy = contentTop;
        int selRowY = -1;                                      // absolute content-Y of the selected row
        std::vector<int> leftOrder;                            // editable field ids in draw order (nav list)
        auto rowVisible = [&](int ry) { return (ry + RH) > contentTop && ry < contentBottom; };

        fb.setClipRect(Lx + 1, contentTop, Lw - 2, contentBottom - contentTop);
        auto row = [&](const char* label, const std::string& value) {
            const int ry = iy - scroll;
            if (rowVisible(ry)) {
                fb.drawText(Lx + 18, ry + 11, label, Colors::TextDim, TextStyle::Body);
                int vw, vh; fb.measureText(value, vw, vh, TextStyle::Body);
                fb.drawText(Lx + Lw - 18 - vw, ry + 11, value, Colors::Text, TextStyle::Body);
            }
            iy += RH;
        };
        // Editable row: selection highlight + touch target, registered only while visible.
        auto editRow = [&](const char* label, const std::string& value, int fieldIdx) {
            leftOrder.push_back(fieldIdx);   // nav list: every editable row, in draw order (even off-screen)
            const int ry = iy - scroll;
            const bool es = (sel == fieldIdx);
            if (es) selRowY = iy;
            if (rowVisible(ry)) {
                if (es) { fb.drawFilledRoundedRect(Lx + 8, ry + 2, Lw - 16, RH - 6, 8, Colors::Selected);
                          fb.drawRoundedRect(Lx + 8, ry + 2, Lw - 16, RH - 6, 8, Colors::Accent, 2); }
                fb.drawText(Lx + 18, ry + 11, label, es ? Colors::Text : Colors::TextDim, TextStyle::Body);
                int vw, vh; fb.measureText(value, vw, vh, TextStyle::Body);
                fb.drawText(Lx + Lw - 18 - vw, ry + 11, value, es ? Colors::Accent : Colors::Text, TextStyle::Body);
                screen.touchButtons.push_back({ fieldIdx, Lx + 8, ry, Lw - 16, RH });
            }
            iy += RH;
        };
        // FireRed/LeafGreen (Gen 3) wires none of exp / form / met date / fateful, so those rows are
        // editable everywhere EXCEPT there (notGen3). Stat Nature (mints) and the egg-met conditions
        // are Gen 8+ / breeding-only -> the tighter modernFmt gate.
        const bool notGen3   = (p->getGameGroup() != Enums::GameVersion::FRLG);
        const bool modernFmt = (notGen3 && p->getGameGroup() != Enums::GameVersion::GG);
        // Nickname leads the column -- it's the name shown in-game. Gen 3 is NOT excluded: its field is
        // shorter (10) and its character set narrower, but it is as editable as any other format's.
        editRow("Nickname", utf16ToUtf8(p->nickname()), 23);
        editRow("Ability",    getAbilityName(p->ability()), 15);
        editRow("Friendship", std::to_string(p->friendship()), 16);
        // Form -- when the species has alternate forms AND the format can set them (Gen 3 forms are
        // PID-derived, so setForm is a no-op there -> exclude only Gen 3).
        if (notGen3 && Pokemon::getPersonalInfo(p->speciesID(), 0).formCount > 1) {
            const char* fn = Names::getFormName(p->speciesID(), p->form());
            editRow("Form", (fn[0] != '\0') ? std::string(fn) : std::string("Base"), 26);
        }
        // Stat Nature (the mint / effective-stat nature) -- modern formats only.
        if (modernFmt) editRow("Stat Nature", getNatureName(p->statNature()), 27);
        snprintf(buf, sizeof(buf), "%08X", p->pid());   row("PID", buf);
        snprintf(buf, sizeof(buf), "%u", p->exp());
        if (notGen3) editRow("EXP", buf, 24); else row("EXP", buf);
        editRow("Egg", p->isEgg() ? "Yes" : "No", 17);

        // ---- Original Trainer / met info ----
        std::string ot = utf16ToUtf8(p->otName());
        if (!ot.empty()) { ot += (p->otGender() == 0) ? " (M)" : " (F)"; row("OT", ot); }
        // Trainer ID in the format the game shows (six digits for Gen 7+ origins, 16-bit TID otherwise;
        // a mon with no origin falls back to its own format -- only Gen 3 is 16-bit among these games).
        {
            const uint8_t ver = p->originGame();
            const bool sixDigit = (ver != 0) ? Enums::usesSixDigitTrainerID(ver)
                                             : (p->getGameGroup() != Enums::GameVersion::FRLG);
            if (sixDigit) snprintf(buf, sizeof(buf), "%06u", p->id32() % 1000000u);
            else          snprintf(buf, sizeof(buf), "%05u", p->id32() & 0xFFFFu);
            row("OT ID", buf);
        }
        // Handling Trainer (Gen 7+): shown only once a mon has actually been handled by someone (htName
        // non-empty), like the OT row above. This makes the OT/HT re-stamp verifiable on-device --
        // change trainer gender/name, reopen a traded-in mon, and HT should track you. FireRed/LeafGreen
        // has no handler concept, so htName() is empty there and this row stays hidden.
        {
            std::string ht = utf16ToUtf8(p->htName());
            if (!ht.empty()) { ht += (p->htGender() == 0) ? " (M)" : " (F)"; row("HT", ht); }
        }
        { snprintf(buf, sizeof(buf), "Lv. %u", p->metLevel()); editRow("Met Lv", buf, 18); }
        // Origin-generation location routing: a Gen 3/4 mon's MET id is remapped into the current
        // format's numbering when it is transferred up (Gen 5+ keep their own table), so a Gen 3/4 met
        // must be named with the format's table -- else a Platinum starter link-traded to SV reads "(none)".
        // `locationTableVersion` encodes that rule. Only an origin we actually know should produce a
        // table; unknown stays "(none)" rather than borrowing the open game's table.
        const uint8_t originVer = p->originGame();
        const uint8_t formatVer = Enums::getGroupRepVersion(p->getGameGroup());
        const uint8_t metVer = originVer ? Enums::locationTableVersion(originVer, formatVer, false) : 0;
        const uint8_t eggVer = originVer ? Enums::locationTableVersion(originVer, formatVer, true) : 0;
        const char* metName = metVer ? Names::getLocationName(metVer, p->metLocation()) : "(none)";
        const char* eggName = eggVer ? Names::getLocationName(eggVer, p->eggLocation()) : "(none)";
        editRow("Met Location", metName, 19);
        if (p->isEgg()) editRow("Egg Location", eggName, 20);
        if (notGen3) {
            char date[32]; snprintf(date, sizeof(date), "%04u-%02u-%02u", p->metYear(), p->metMonth(), p->metDay());
            editRow("Met Date", date, 21);
        }
        {
            const uint8_t ver = p->originGame();
            const std::string origin = ver ? Enums::getOriginGameName(ver) : std::string("Unknown");
            editRow("Origin", origin, 22);
        }
        if (notGen3) editRow("Fateful", p->fatefulEncounter() ? "Yes" : "No", 25);
        editRow("Ball", getBallName(p->ball()), 28);
        editRow("Language", getLanguageName(p->language()), 29);
        fb.resetClipRect();
        screen.details.leftOrder = leftOrder;                 // update() uses the exact rendered list for navigation

        // Keep the selected left row inside the viewport. Draw uses the OLD scroll this frame, then
        // adjusts it for the next; that avoids modifying layout halfway through the same render pass.
        if (selRowY >= 0) {
            const int visibleH = contentBottom - contentTop;
            const int totalH = iy - contentTop;
            const int maxScroll = std::max(0, totalH - visibleH);
            if (selRowY - screen.details.leftScroll < contentTop)
                screen.details.leftScroll = std::clamp(selRowY - contentTop, 0, maxScroll);
            else if (selRowY + RH - screen.details.leftScroll > contentBottom)
                screen.details.leftScroll = std::clamp(selRowY + RH - contentBottom, 0, maxScroll);
            else
                screen.details.leftScroll = std::clamp(screen.details.leftScroll, 0, maxScroll);
        }

        // Legality summary pinned at the bottom of the left column.
        const int legY = colY + colH - legalityH;
        if (legalityRep.valid) {
            fb.drawText(Lx + 18, legY + 8, "Legality: Valid", Colors::Green, TextStyle::Caption);
        } else {
            const std::string text = "Legality: " + std::to_string(legalityRep.issues.size()) + " issue(s)";
            fb.drawText(Lx + 18, legY + 8, text, Colors::Warning, TextStyle::Caption);
        }

        // =========================== CENTER: stats / values ===========================
        const int Cx = Lx + Lw + 12, Cw = 360;
        fb.drawFilledRoundedRect(Cx, colY, Cw, colH, 16, Colors::Panel);
        fb.drawRoundedRect(Cx, colY, Cw, colH, 16, Colors::Border, 1);

        fb.drawText(Cx + 20, colY + 18, av ? "Stats / IV / AV" : "Stats / IV / EV", Colors::Text, TextStyle::Heading);
        const char* statNames[6] = { "HP", "Attack", "Defense", "Sp. Atk", "Sp. Def", "Speed" };
        uint16_t statVals[6] = { p->statHPMax(), p->statATK(), p->statDEF(), p->statSPA(), p->statSPD(), p->statSPE() };
        uint8_t ivVals[6] = { p->ivHP(), p->ivATK(), p->ivDEF(), p->ivSPA(), p->ivSPD(), p->ivSPE() };
        uint16_t evVals[6] = { p->evHP(), p->evATK(), p->evDEF(), p->evSPA(), p->evSPD(), p->evSPE() };
        uint8_t avVals[6] = { p->avHP(), p->avATK(), p->avDEF(), p->avSPA(), p->avSPD(), p->avSPE() };
        int sy = colY + 62;
        for (int i = 0; i < 6; ++i) {
            const bool ss = (sel == i);
            if (ss) fb.drawFilledRoundedRect(Cx + 10, sy - 6, Cw - 20, 50, 10, Colors::Selected);
            fb.drawText(Cx + 20, sy + 6, statNames[i], ss ? Colors::Text : Colors::TextDim, TextStyle::Body);
            char v[80];
            if (av) snprintf(v, sizeof(v), "%u    IV %u    AV %u", statVals[i], ivVals[i], avVals[i]);
            else    snprintf(v, sizeof(v), "%u    IV %u    EV %u", statVals[i], ivVals[i], evVals[i]);
            int vw, vh; fb.measureText(v, vw, vh, TextStyle::Body);
            fb.drawText(Cx + Cw - 20 - vw, sy + 6, v, ss ? Colors::Accent : Colors::Text, TextStyle::Body);
            screen.touchButtons.push_back({ i, Cx + 10, sy - 6, Cw - 20, 50 });
            sy += 56;
        }

        auto centerEdit = [&](const char* label, const std::string& value, int field) {
            const bool ss = (sel == field);
            if (ss) fb.drawFilledRoundedRect(Cx + 10, sy - 4, Cw - 20, 40, 8, Colors::Selected);
            fb.drawText(Cx + 20, sy + 6, label, ss ? Colors::Text : Colors::TextDim, TextStyle::Body);
            int vw, vh; fb.measureText(value, vw, vh, TextStyle::Body);
            fb.drawText(Cx + Cw - 20 - vw, sy + 6, value, ss ? Colors::Accent : Colors::Text, TextStyle::Body);
            screen.touchButtons.push_back({ field, Cx + 10, sy - 4, Cw - 20, 40 });
            sy += 46;
        };
        centerEdit("Shiny", isShiny ? "Yes" : "No", 6);
        centerEdit("Nature", getNatureName(p->nature()), 7);
        centerEdit("Gender", p->genderSymbol()[0] ? p->genderSymbol() : "—", 8);
        centerEdit("Level", std::to_string(subLvl), 9);

        // =========================== RIGHT: moves + item ===========================
        const int Rx = Cx + Cw + 12, Rw = W - Rx - 24;
        fb.drawFilledRoundedRect(Rx, colY, Rw, colH, 16, Colors::Panel);
        fb.drawRoundedRect(Rx, colY, Rw, colH, 16, Colors::Border, 1);
        fb.drawText(Rx + 20, colY + 18, "Moves / Held Item", Colors::Text, TextStyle::Heading);
        int my = colY + 62;
        for (int i = 0; i < 4; ++i) {
            const bool ms = (sel == 10 + i);
            if (ms) fb.drawFilledRoundedRect(Rx + 10, my - 5, Rw - 20, 72, 10, Colors::Selected);
            uint16_t moveId = p->move(i);
            const char* moveName = (moveId == 0) ? "(none)" : getMoveName(moveId);
            fb.drawText(Rx + 20, my + 4, moveName, ms ? Colors::Text : Colors::Text, TextStyle::Body);
            char pp[48]; snprintf(pp, sizeof(pp), "PP %u   PP Ups %u", p->movePP(i), p->movePPUps(i));
            fb.drawText(Rx + 20, my + 32, pp, ms ? Colors::Accent : Colors::TextDim, TextStyle::Caption);
            screen.touchButtons.push_back({ 10 + i, Rx + 10, my - 5, Rw - 20, 72 });
            my += 80;
        }
        const bool itemSel = (sel == 14);
        if (itemSel) fb.drawFilledRoundedRect(Rx + 10, my - 4, Rw - 20, 44, 8, Colors::Selected);
        fb.drawText(Rx + 20, my + 8, "Held Item", itemSel ? Colors::Text : Colors::TextDim, TextStyle::Body);
        const char* itemName = p->heldItem() ? getItemName(p->heldItem()) : "(none)";
        int iw, ih; fb.measureText(itemName, iw, ih, TextStyle::Body);
        fb.drawText(Rx + Rw - 20 - iw, my + 8, itemName, itemSel ? Colors::Accent : Colors::Text, TextStyle::Body);
        screen.touchButtons.push_back({14, Rx + 10, my - 4, Rw - 20, 44});

        // Footer controls are drawn by TrainerViewScreen, not here.
    }
}
}
