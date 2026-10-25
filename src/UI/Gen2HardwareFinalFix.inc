#include "UI/SharedPokemonShell.h"
#include "UI/ClassicFieldColor.h"
#include "UI/ClassicGameContext.h"

// Final hardware-retest presentation adapter for Generation II.
// The accepted editor/input engines remain underneath; this layer owns the final
// fullscreen pixels for View/Edit/Create and leaves the main Trainer page behavior intact.

namespace UI::Gen2PokemonEditor {
namespace {

namespace ClassicContext = PokeBank::UIModel::ClassicGameContext;
namespace UnifiedContract = PokeBank::UIModel::SharedPokemonEditor;

ClassicContext::SurfaceMode gen2SurfaceMode(Mode mode) noexcept {
    if (mode == Mode::View) return ClassicContext::SurfaceMode::ViewReadOnly;
    if (mode == Mode::Create) return ClassicContext::SurfaceMode::CreateDraft;
    return ClassicContext::SurfaceMode::EditStaged;
}

Color gen2MaleColor() noexcept {
    // Pure fixed Blue is too dark against the accepted dark/OLED surfaces on Switch hardware.
    // Keep a cyan-leaning sky blue in dark themes and a deeper companion shade for light mode.
    return g_themeMode == ThemeMode::Light ? Color(24, 112, 184) : Color(92, 205, 255);
}

Color gen2GenderColor(uint8_t gender) noexcept {
    if (static_cast<Gen2::PokemonGender>(gender) == Gen2::PokemonGender::Male) return gen2MaleColor();
    if (static_cast<Gen2::PokemonGender>(gender) == Gen2::PokemonGender::Female) return Colors::Magenta;
    return Colors::Text;
}

int gen2TypeSpriteWidth(uint8_t classicType, int height = 22) {
    const int normalized = ClassicContext::normalizedTypeSpriteId(classicType);
    if (normalized < 0) return 0;
    auto* sprite = SpriteManager::getTypeSprite(static_cast<uint8_t>(normalized));
    if (!sprite || !sprite->data || sprite->height <= 0) return 0;
    return (sprite->width * height) / sprite->height;
}

void drawGen2TypeSprite(PKSEFramebuffer& fb, int x, int y, uint8_t classicType, int height = 22) {
    const int normalized = ClassicContext::normalizedTypeSpriteId(classicType);
    if (normalized < 0) return;
    auto* sprite = SpriteManager::getTypeSprite(static_cast<uint8_t>(normalized));
    if (!sprite || !sprite->data || sprite->height <= 0) return;
    const int width = (sprite->width * height) / sprite->height;
    fb.drawImageScaled(x, y, sprite->width, sprite->height, width, height, sprite->data, sprite->channels);
}

void drawGen2TypePair(PKSEFramebuffer& fb, int x, int w, int y, const Gen2::PokemonRecord& p) {
    const auto* personal = Gen2::personalRecord(p.species);
    if (!personal) return;
    constexpr int h = 22;
    constexpr int gap = 8;
    const int w1 = gen2TypeSpriteWidth(personal->rawType1, h);
    const int w2 = personal->rawType2 != personal->rawType1 ? gen2TypeSpriteWidth(personal->rawType2, h) : 0;
    const int total = w1 + (w2 > 0 ? gap + w2 : 0);
    int xx = x + (w - total) / 2;
    if (w1 > 0) {
        drawGen2TypeSprite(fb, xx, y, personal->rawType1, h);
        xx += w1 + gap;
    }
    if (w2 > 0) drawGen2TypeSprite(fb, xx, y, personal->rawType2, h);
}

void drawGen2Focus(PKSEFramebuffer& fb, int x, int y, int w, int h, bool selected, int radius = 6) {
    if (selected) fb.drawRoundedRect(x, y, w, h, radius, Colors::FocusBorder, 2);
}

std::string gen2RecordContext(const State& state) {
    std::string record = state.party?"Party "+std::to_string(state.slot+1):"Box " + std::to_string(state.box + 1);
    if (!state.party && state.slot >= 0) record += " / Slot " + std::to_string(state.slot + 1);
    else if(!state.party) record += " / New Pokemon";
    return record;
}

std::string gen2PassiveRecordContext(const TrainerViewScreen& screen) {
    if (screen.details.source == TrainerViewScreen::EditSource::Party)
        return "Party Slot " + std::to_string(screen.details.partyIndex + 1);
    if (screen.selectedMode == TrainerViewScreen::ViewMode::Storage)
        return "Storage Save Box " + std::to_string(screen.selectedBoxIndex + 1) +
               " / Slot " + std::to_string(screen.selectedItemIndex + 1);
    return "Box " + std::to_string(screen.selectedBoxIndex + 1) +
           " / Slot " + std::to_string(screen.selectedItemIndex + 1);
}

void redrawGen2DetailsSemantics(PKSEFramebuffer& fb, const State& state,
                                const UnifiedWorkspaceState& unified,
                                int x, int y, int w, int h, bool passive) {
    const auto& p = state.working;
    fb.drawFilledRect(x + 10, y + 6, 150, 34, Colors::Panel);
    fb.drawText(x + 14, y + 10, "DETAILS", Colors::Text, TextStyle::Caption);

    // Replace legacy outlined labels with the canonical ROMFS type sprites.
    fb.drawFilledRect(x + 12, y + 174, w - 24, 38, Colors::Panel);
    drawGen2TypePair(fb, x, w, y + 180, p);

    std::vector<std::pair<std::string, std::string>> rows{
        {"Species", std::string(Names::getSpeciesName(p.species)) + " #" + std::to_string(p.species)},
        {"Nickname", p.nickname},
        {"Level", std::to_string(p.level)},
        {"Experience", std::to_string(p.experience)},
        {"OT Name", p.originalTrainer},
        {"Trainer ID", std::to_string(p.trainerId)},
        {"Held Item", itemText(p.heldItem)},
        {"Friendship", std::to_string(p.friendship)},
        {"Pokerus", Gen2Native::pokerusText(p.pokerus)},
    };
    if (unified.crystal) {
        const auto caught = Gen2Native::decodeCrystalCaughtData(p.caughtData);
        const uint8_t displayedMetTime = state.mode == Mode::View
            ? caught.timeOfDay
            : state.crystalMetTime;
        rows.push_back({"Met time", Gen2Native::crystalMetTimeName(displayedMetTime)});
        rows.push_back({"Met level", Gen2Native::crystalCaughtLevelText(caught)});
        rows.push_back({"Location", Gen2Native::crystalCaughtLocationName(caught.location)});
        const auto displayedOtGender = state.mode == Mode::View
            ? Gen2Native::crystalOtGenderChoiceFromCaught(caught)
            : state.crystalOtGender;
        rows.push_back({"OT gender", Gen2Native::crystalOtGenderChoiceText(displayedOtGender)});
    }

    // The semantic repaint is the authoritative visible Details row window. Clear the
    // previous rows first, then repaint the same eight-row window used by the shared shell.
    const int viewportY = y + 216;
    const int viewportBottom = y + h - 34;
    const int viewportH = std::max(0, viewportBottom - viewportY);
    constexpr int detailRowStep = 38;
    // Match the stable clipped touch viewport already used by drawUnifiedDetails.
    // Re-centring from focus here previously undid every finger-scroll frame.
    const int detailFirst = std::max(0, unified.detailsViewport.firstRow);
    const int detailOffset = unified.detailsTouchScroll.offset();
    fb.drawFilledRect(x + 8, viewportY, w - 16, viewportH, Colors::Panel);
    const UnifiedContract::ScrollWindow window{
        static_cast<std::size_t>(detailFirst),
        std::min<std::size_t>(8, rows.size() - static_cast<std::size_t>(detailFirst)),
        rows.size() > 8};
    fb.setClipRect(x + 8, viewportY, w - 16, viewportH);
    for (std::size_t i = 0; i < window.count; ++i) {
        const std::size_t row = window.first + i;
        const int yy = y + 224 + static_cast<int>(i) * detailRowStep + detailOffset;
        const bool selected =
            unified.focus.panel == UnifiedContract::Panel::Details &&
            row == static_cast<std::size_t>(unified.focus.row);
        // Leave a clean gutter between the selection outline and the scrollbar track.
        drawGen2Focus(fb, x + 8, yy + 2, std::max(0, w - 36), 30, selected);
        fb.drawText(x + 18, yy + 7, rows[row].first, selected ? Colors::SelectedText : Colors::TextDim,
                    TextStyle::Caption);
        fb.drawText(x + 108, yy + 7, rows[row].second, Colors::Text, TextStyle::Caption);
    }
    fb.clearClip();
    // The shared draggable scrollbar replaces the draw-only
    // SharedPokemonShell::drawVerticalScrollIndicator here: drawUnifiedDetails published this
    // exact track (x + w - 10, viewportY + 6, viewportH - 12) a moment ago, so repaint it from
    // the same geometry with the captured-thumb highlight intact.
    SharedScrollbar::paint(fb, SharedScrollbar::layout(x + w - 10, viewportY + 6,
        SharedScrollbar::kTrackWidth, std::max(0, viewportH - 12),
        SharedScrollbar::rows(rows.size(), static_cast<std::size_t>(8), window.first)),
        unified.detailsScrollbar.capturing());
    fb.drawFilledRect(x + w - 50, y + h - 34, 38, 26, Colors::Panel);
}

void redrawGen2StatsSemantics(PKSEFramebuffer& fb, const State& state,
                              const UnifiedWorkspaceState& unified,
                              int x, int y, int w, bool passive) {
    const auto& p = state.working;
    const auto battle = PokeBank::UIModel::Gen2Workspace::battleStats(p,state.mode!=Mode::View);
    const std::array<const char*, 5> labels{"HP", "Attack", "Defense", "Speed", "Special"};
    const std::array<uint8_t, 5> dvs{gen2ShownHpDv(p), p.dvs[1], p.dvs[2], p.dvs[3], p.dvs[4]};
    const int shinyY = y + 366;
    const int genderY = y + 408;

    fb.drawFilledRect(x + 10, y + 6, 150, 34, Colors::Panel);
    fb.drawText(x + 14, y + 10, "STATS", Colors::Text, TextStyle::Heading);
    // drawUnifiedValues() has already painted its legacy Shiny/Gender rows underneath this
    // hardware layer. Clear through the final Gender baseline before repainting semantics so
    // the old neutral row cannot remain visible as a duplicate beneath the colored value.
    const int clearTop = y + 38;
    const bool unown = p.species == PokeBank::UIModel::Gen2UnownLetter::kUnownSpecies;
    const int noteY = genderY + 38;
    const int semanticClearBottom = genderY + 30;
    fb.drawFilledRect(x + 10, clearTop, w - 20, semanticClearBottom - clearTop, Colors::Panel);
    fb.drawText(x + 114, y + 42, "DV", Colors::TextDim, TextStyle::Caption);
    fb.drawText(x + 184, y + 42, "Stat Exp", Colors::TextDim, TextStyle::Caption);
    fb.drawText(x + 292, y + 42, "Calculated", Colors::TextDim, TextStyle::Caption);

    for (int r = 0; r < 5; ++r) {
        const int yy = y + 70 + r * 34;
        const bool rowSelected = unified.focus.panel == UnifiedContract::Panel::Values && unified.focus.row == r;
        if (rowSelected) {
            const auto cell = UnifiedContract::cellFocus(unified.focus);
            drawGen2Focus(fb, x + cell.x, yy - 5, cell.width, 28, true);
        }
        fb.drawText(x + 18, yy, labels[static_cast<std::size_t>(r)], rowSelected ? Colors::SelectedText : Colors::TextDim, TextStyle::Caption);
        fb.drawText(x + 114, yy, std::to_string(dvs[static_cast<std::size_t>(r)]) + (r == 0 ? " *" : ""),
                    gen2StatCellColor(r, 0), TextStyle::Caption);
        fb.drawText(x + 184, yy, std::to_string(p.statExperience[static_cast<std::size_t>(r)]),
                    gen2StatCellColor(r, 1), TextStyle::Caption);
        const std::string stat = PokeBank::UIModel::Gen2Workspace::calculatedCell(battle, r);
        fb.drawText(x + 282, yy, stat, gen2StatCellColor(r, 2), TextStyle::Caption);
    }
    // One real editable Special DV/Stat Exp row. Both computed Special stats remain
    // in the Calculated column, stacked cleanly without a fake second DV row.
    // The Special row above already shows SpA via calculatedCell(battle, 4).
    fb.drawText(x + 18, y + 246, "Sp. Def", Colors::TextDim, TextStyle::Caption);
    fb.drawText(x + 282, y + 246,
                PokeBank::UIModel::Gen2Workspace::calculatedSpecialDefenseCell(battle),
                gen2StatCellColor(4, 2), TextStyle::Caption);
    fb.drawText(x + 18, y + 291, "Special DV / Stat Exp shared by SpA and SpD",
                Colors::TextDim, TextStyle::Caption);
    fb.drawText(x + 18, y + 328, "* HP DV derived / read-only", Colors::TextDim, TextStyle::Caption);

    const bool shinySelected = unified.focus.panel == UnifiedContract::Panel::Values && unified.focus.row == 5;
    const bool genderSelected = unified.focus.panel == UnifiedContract::Panel::Values && unified.focus.row == 6;
    drawGen2Focus(fb, x + 8, shinyY - 5, w - 16, 30, shinySelected);
    fb.drawText(x + 18, shinyY, "Shiny", shinySelected ? Colors::SelectedText : Colors::TextDim, TextStyle::Caption);
    fb.drawText(x + 134, shinyY, p.shiny ? "Yes" : "No",
                ::UI::ClassicFieldColor::shinyValueColor(shinySelected), TextStyle::Caption);
    drawGen2Focus(fb, x + 8, genderY - 5, w - 16, 30, genderSelected);
    if (unown) {
        // Unown is genderless; row 6 is its Letter selector. The letter is computed from the DVs
        // on every frame (pokecrystal GetUnownLetter), never stored independently.
        fb.drawText(x + 18, genderY, "Letter", genderSelected ? Colors::SelectedText : Colors::TextDim, TextStyle::Caption);
        fb.drawText(x + 134, genderY, gen2UnownLetterText(p),
                    ::UI::ClassicFieldColor::shinyValueColor(genderSelected), TextStyle::Caption);
        fb.drawFilledRect(x + 10, semanticClearBottom, w - 20, noteY + 26 - semanticClearBottom, Colors::Panel);
        fb.drawText(x + 18, noteY, gen2UnownLetterNote(), Colors::TextDim, TextStyle::Caption);
        return;
    }
    fb.drawText(x + 18, genderY, "Gender", genderSelected ? Colors::SelectedText : Colors::TextDim, TextStyle::Caption);
    fb.drawText(x + 134, genderY, genderText(p.gender), genderSelected ? Colors::SelectedText : gen2GenderColor(p.gender), TextStyle::Caption);
}

void redrawGen2MovesSemantics(PKSEFramebuffer& fb, const TrainerViewScreen& screen, const State& state,
                              const UnifiedWorkspaceState& unified,
                              int x, int y, int w, bool passive) {
    // Repaint the heading and the shared move rows over anything an older layer drew there.
    fb.drawFilledRect(x + 10, y + 6, 150, 34, Colors::Panel);
    fb.drawText(x + 14, y + 10, "MOVES", Colors::Text, TextStyle::Heading);
    fb.drawFilledRect(x + 6, y + 32, w - 12, UI::SharedMovesPanel::kDataPaneOffset - 36, Colors::Panel);
    drawGen2SharedMoveRows(fb, screen, state, unified, x, y, w, passive);
}

void drawFullscreenGen2Workspace(TrainerViewScreen& screen, PKSEFramebuffer& fb,
                                 State& state, UnifiedWorkspaceState& unified,
                                 bool passive, const std::string& recordContext) {
    const auto& p = state.working;
    screen.touchButtons.clear();
    SharedPokemonShell::drawChrome(fb, p.species, p.nickname, p.level, p.gender, p.shiny,
        gen2SurfaceMode(state.mode), recordContext, screen.sourceGameId);
    const SharedPokemonShell::Geometry geometry(fb.getWidth(), fb.getHeight(), kNavBarH);
    const int contentY = geometry.y, contentH = geometry.h;
    const int leftX = geometry.leftX, leftW = geometry.leftW;
    const int midX = geometry.midX, midW = geometry.midW;
    const int rightX = geometry.rightX, rightW = geometry.rightW;

    drawUnifiedDetails(screen, fb, state, unified, leftX, contentY, leftW, contentH, passive);
    drawUnifiedValues(fb, state, unified, midX, contentY, midW, contentH, passive);
    drawUnifiedMoves(fb, screen, state, unified, rightX, contentY, rightW, contentH, passive);
    redrawGen2DetailsSemantics(fb, state, unified, leftX, contentY, leftW, contentH, passive);
    redrawGen2StatsSemantics(fb, state, unified, midX, contentY, midW, passive);
    redrawGen2MovesSemantics(fb, screen, state, unified, rightX, contentY, rightW, passive);

    if (passive || state.mode == Mode::View) {
        screen.touchButtons.push_back({PokeBank::UIModel::passiveBackTarget, 16, fb.getHeight() - 68, 190, 58});
        drawNavBar(fb, {{"D-pad", "Navigate"}, {"L/R", "Panel"}, {"B", "Back"}});
    } else {
        // The footer of the layer that owns input (exit dialog, selector, re-roll prompt or the
        // workspace), so no visible or tappable glyph means something else (Gen2WorkspaceLayers.h).
        drawWorkspaceFooter(fb, screen);
    }
}

void drawFullscreenGen2Active(TrainerViewScreen& screen, PKSEFramebuffer& fb) {
    auto& state = stateFor(screen);
    auto& unified = unifiedStateFor(screen);
    drawFullscreenGen2Workspace(screen, fb, state, unified, state.mode == Mode::View, gen2RecordContext(state));
    if (unified.moveEditor) drawUnifiedMoveEditor(screen, fb, state, unified);
    if (state.itemPicker) drawHeldItemPicker(fb, state);
    if (state.confirmExit) drawConfirm(screen, fb, state);
    if (unified.selector.open()) drawFieldSelector(screen, fb, unified.selector);
    if (unified.progressionEditor) drawProgressionEditor(fb, state, unified);
}

void drawFullscreenGen2Passive(TrainerViewScreen& screen, PKSEFramebuffer& fb,
                               const Pokemon::Pokemon2ReadOnly& pokemon) {
    State visual{};
    visual.begin(pokemon.strictRecord(), Mode::View);
    refreshDerived(visual.working);
    auto& unified = unifiedStateFor(screen);
    unified.crystal = screen.sourceGameId == "crystal_gbc";
    drawFullscreenGen2Workspace(screen, fb, visual, unified, true, gen2PassiveRecordContext(screen));
}

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
    const Color valueColor = screen.trainer.trainerGender == 0 ? gen2MaleColor() :
                             screen.trainer.trainerGender == 1 ? Colors::Magenta : Colors::Text;

    fb.drawFilledRoundedRect(cardX, genderY, cardW, 46, 12, Colors::PanelAlt);
    fb.drawText(cardX + 24, genderY + (46 - fb.lineHeight(TextStyle::Body)) / 2,
                "Gender", Colors::TextDim, TextStyle::Body);
    int vw = 0, vh = 0;
    fb.measureText(value, vw, vh, TextStyle::Body);
    fb.drawText(cardX + cardW - 24 - vw, genderY + (46 - vh) / 2,
                value, valueColor, TextStyle::Body);
}

} // namespace

bool drawFinalGen2Surface(TrainerViewScreen& screen, PKSEFramebuffer& fb) {
    if (drawFinalGen2SurfaceBase(screen, fb)) {
        if (isGSC(screen) && stateFor(screen).mode != Mode::None) {
            // Repaint after the accepted route so the editor itself owns the full page;
            // the underlying browser and outer workspace card are no longer visible.
            drawFullscreenGen2Active(screen, fb);
        } else if (const auto* passive = parityPassiveDetailsTarget(screen)) {
            drawFullscreenGen2Passive(screen, fb, *passive);
        }
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
