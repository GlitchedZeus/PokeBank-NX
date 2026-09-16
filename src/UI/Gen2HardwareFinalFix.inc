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

std::string gen2DexLabel(uint16_t species) {
    std::string dex = std::to_string(species);
    while (dex.size() < 3) dex = "0" + dex;
    return dex;
}

Color gen2GenderColor(uint8_t gender) noexcept {
    if (static_cast<Gen2::PokemonGender>(gender) == Gen2::PokemonGender::Male) return Colors::Blue;
    if (static_cast<Gen2::PokemonGender>(gender) == Gen2::PokemonGender::Female) return Colors::Magenta;
    return Colors::Text;
}

void drawGen2HeaderGender(PKSEFramebuffer& fb, int x, int y, uint8_t gender) {
    if (static_cast<Gen2::PokemonGender>(gender) == Gen2::PokemonGender::Male)
        fb.drawSymbol(x, y, "\xE2\x99\x82", Colors::Blue);
    else if (static_cast<Gen2::PokemonGender>(gender) == Gen2::PokemonGender::Female)
        fb.drawSymbol(x, y, "\xE2\x99\x80", Colors::Magenta);
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
    std::string record = "Box " + std::to_string(state.box + 1);
    if (state.slot >= 0) record += " / Slot " + std::to_string(state.slot + 1);
    else record += " / New Pokemon";
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
        {"Species", std::to_string(p.species) + " - " + Names::getSpeciesName(p.species)},
        {"Nickname", p.nickname},
        {"Level / EXP", "Lv " + std::to_string(p.level) + " / " + std::to_string(p.experience)},
        {"OT", p.originalTrainer},
        {"Trainer ID", std::to_string(p.trainerId)},
        {"Held Item", itemText(p.heldItem)},
        {"Friendship", std::to_string(p.friendship)},
        {"Pokerus", Gen2Native::pokerusText(p.pokerus)},
    };
    if (unified.crystal) {
        const auto caught = Gen2Native::decodeCrystalCaughtData(p.caughtData);
        rows.push_back({"Met time", Gen2Native::crystalMetTimeName(caught.timeOfDay)});
        rows.push_back({"Met level", Gen2Native::crystalCaughtLevelText(caught)});
        rows.push_back({"Location", Gen2Native::crystalCaughtLocationName(caught.location)});
        rows.push_back({"OT gender", Gen2Native::crystalOriginalTrainerGenderText(caught)});
    }

    const auto window = UnifiedContract::scrollWindow(rows.size(), 5, passive ? 0 : unified.detailsFocus);
    for (std::size_t i = 0; i < window.count; ++i) {
        const std::size_t row = window.first + i;
        const int yy = y + 224 + static_cast<int>(i) * 43;
        const bool selected = !passive && unified.focus.panel == UnifiedContract::Panel::Details && unified.focus.row == row;
        fb.drawFilledRect(x + 8, yy + 2, w - 16, 32, Colors::Panel);
        drawGen2Focus(fb, x + 8, yy + 2, w - 16, 30, selected);
        fb.drawText(x + 18, yy + 7, rows[row].first, Colors::TextDim, TextStyle::Caption);
        fb.drawText(x + 108, yy + 7, rows[row].second, Colors::Text, TextStyle::Caption);
    }
    fb.drawFilledRect(x + w - 50, y + h - 34, 38, 26, Colors::Panel);
}

void redrawGen2StatsSemantics(PKSEFramebuffer& fb, const State& state,
                              const UnifiedWorkspaceState& unified,
                              int x, int y, int w, bool passive) {
    const auto& p = state.working;
    const auto battle = PokeBank::UIModel::Gen2Workspace::battleStats(p);
    const std::array<const char*, 5> labels{"HP", "Attack", "Defense", "Speed", "Special"};
    const std::array<uint8_t, 5> dvs{p.dvs[0], p.dvs[1], p.dvs[2], p.dvs[3], p.dvs[4]};

    fb.drawFilledRect(x + 10, y + 6, 150, 34, Colors::Panel);
    fb.drawText(x + 14, y + 10, "STATS", Colors::Text, TextStyle::Heading);
    fb.drawFilledRect(x + 10, y + 38, w - 20, 330, Colors::Panel);
    fb.drawText(x + 114, y + 42, "DV", Colors::TextDim, TextStyle::Caption);
    fb.drawText(x + 184, y + 42, "Stat Exp", Colors::TextDim, TextStyle::Caption);
    fb.drawText(x + 292, y + 42, "Stat", Colors::TextDim, TextStyle::Caption);

    for (int r = 0; r < 5; ++r) {
        const int yy = y + 70 + r * 34;
        const bool rowSelected = !passive && unified.focus.panel == UnifiedContract::Panel::Values && unified.focus.row == r;
        if (rowSelected) {
            const auto cell = UnifiedContract::cellFocus(unified.focus);
            drawGen2Focus(fb, x + cell.x, yy - 5, cell.width, 28, true);
        }
        fb.drawText(x + 18, yy, labels[static_cast<std::size_t>(r)], Colors::TextDim, TextStyle::Caption);
        fb.drawText(x + 114, yy, std::to_string(dvs[static_cast<std::size_t>(r)]) + (r == 0 ? " *" : ""),
                    Colors::Text, TextStyle::Caption);
        fb.drawText(x + 184, yy, std::to_string(p.statExperience[static_cast<std::size_t>(r)]),
                    Colors::Text, TextStyle::Caption);
        const std::string stat = r < 4 ? std::to_string(battle[static_cast<std::size_t>(r)]) : "—";
        fb.drawText(x + 282, yy, stat, Colors::Accent, TextStyle::Caption);
    }
    fb.drawText(x + 18, y + 246, "CALCULATED SPECIAL STATS", Colors::TextDim, TextStyle::Caption);
    fb.drawText(x + 18, y + 274, "SpA", Colors::TextDim, TextStyle::Caption);
    fb.drawText(x + 84, y + 274, std::to_string(battle[4]), Colors::Accent, TextStyle::Caption);
    fb.drawText(x + 200, y + 274, "SpD", Colors::TextDim, TextStyle::Caption);
    fb.drawText(x + 266, y + 274, std::to_string(battle[5]), Colors::Accent, TextStyle::Caption);
    fb.drawText(x + 18, y + 304, "* HP DV derived / read-only • one stored Special DV / Stat Exp",
                Colors::TextDim, TextStyle::Caption);

    const int shinyY = y + 350;
    const int genderY = y + 392;
    const bool shinySelected = !passive && unified.focus.panel == UnifiedContract::Panel::Values && unified.focus.row == 5;
    const bool genderSelected = !passive && unified.focus.panel == UnifiedContract::Panel::Values && unified.focus.row == 6;
    drawGen2Focus(fb, x + 8, shinyY - 5, w - 16, 30, shinySelected);
    fb.drawText(x + 18, shinyY, "Shiny", Colors::TextDim, TextStyle::Caption);
    fb.drawText(x + 134, shinyY, p.shiny ? "Yes (DV-derived)" : "No (DV-derived)",
                p.shiny ? Colors::ShinyStar : Colors::Text, TextStyle::Caption);
    drawGen2Focus(fb, x + 8, genderY - 5, w - 16, 30, genderSelected);
    fb.drawText(x + 18, genderY, "Gender", Colors::TextDim, TextStyle::Caption);
    fb.drawText(x + 134, genderY, genderText(p.gender), gen2GenderColor(p.gender), TextStyle::Caption);
}

void redrawGen2MovesSemantics(PKSEFramebuffer& fb, const TrainerViewScreen& screen, const State& state,
                              const UnifiedWorkspaceState& unified,
                              int x, int y, int w, bool passive) {
    fb.drawFilledRect(x + 10, y + 6, 150, 34, Colors::Panel);
    fb.drawText(x + 14, y + 10, "MOVES", Colors::Text, TextStyle::Heading);
    fb.drawFilledRect(x + 10, y + 38, w - 20, 146, Colors::Panel);
    fb.drawText(x + 190, y + 42, "PP", Colors::TextDim, TextStyle::Caption);
    fb.drawText(x + 246, y + 42, "Ups", Colors::TextDim, TextStyle::Caption);
    for (int slot = 0; slot < 4; ++slot) {
        const int yy = y + 62 + slot * 32;
        const bool selected = !passive && unified.focus.panel == UnifiedContract::Panel::Moves && unified.focus.row == slot;
        if (selected) {
            const auto cell = UnifiedContract::cellFocus(unified.focus);
            drawGen2Focus(fb, x + cell.x, yy - 5, cell.width, 28, true);
        }
        const uint8_t move = state.working.moves[static_cast<std::size_t>(slot)];
        const auto status = moveStatus(screen, state, slot);
        fb.drawText(x + 18, yy, move == 0 ? "Empty" : Names::getMoveName(move), Colors::Text, TextStyle::Caption);
        if (move != 0) {
            fb.drawText(x + 190, yy, std::to_string(state.working.pp[static_cast<std::size_t>(slot)]), Colors::Text, TextStyle::Caption);
            fb.drawText(x + 246, yy, std::to_string(state.working.ppUps[static_cast<std::size_t>(slot)]), Colors::Text, TextStyle::Caption);
        }
        fb.drawText(x + 292, yy,
                    status == Rules::CompatibilityState::Ok ? "OK" :
                    status == Rules::CompatibilityState::UnusualPreserved ? "Unusual" : "Invalid",
                    unifiedStatusColor(status), TextStyle::Caption);
    }
}

void drawFullscreenGen2Workspace(TrainerViewScreen& screen, PKSEFramebuffer& fb,
                                 State& state, UnifiedWorkspaceState& unified,
                                 bool passive, const std::string& recordContext) {
    const auto& p = state.working;
    screen.touchButtons.clear();
    fb.drawVerticalGradient(0, 0, fb.getWidth(), fb.getHeight(),
        Color(Colors::Background.r, Colors::Background.g, Colors::Background.b, 250),
        Color(Colors::Background.r, Colors::Background.g, Colors::Background.b, 255));

    const std::string name = p.nickname.empty() ? std::string(Names::getSpeciesName(p.species)) : p.nickname;
    fb.drawText(28, 16, name, Colors::Text, TextStyle::Heading);
    int nw = 0, nh = 0;
    fb.measureText(name, nw, nh, TextStyle::Heading);
    drawGen2HeaderGender(fb, 40 + nw, 22, p.gender);
    if (p.shiny) fb.drawShinyMark(66 + nw, 20, 18, Colors::ShinyStar);

    const std::string rightHeader = "Lv. " + std::to_string(p.level) + "     No. " + gen2DexLabel(p.species);
    int rw = 0, rh = 0;
    fb.measureText(rightHeader, rw, rh);
    fb.drawText(fb.getWidth() - 28 - rw, 24, rightHeader, Colors::TextDim);

    const auto surfaceMode = gen2SurfaceMode(state.mode);
    const std::string mode = std::string(ClassicContext::modeLabel(surfaceMode));
    int mw = 0, mh = 0;
    fb.measureText(mode, mw, mh, TextStyle::Caption);
    fb.drawText((fb.getWidth() - mw) / 2, 23, mode, Colors::Accent, TextStyle::Caption);
    fb.drawFilledRect(0, 60, fb.getWidth(), 2, Colors::Accent);
    fb.drawText(28, 66, ClassicContext::contextLine(recordContext, screen.sourceGameId, surfaceMode),
                Colors::TextDim, TextStyle::Caption);

    constexpr int contentY = 96;
    const int contentH = fb.getHeight() - contentY - kNavBarH - 8;
    constexpr int leftX = 24, leftW = 300;
    constexpr int gap = 14;
    constexpr int midX = leftX + leftW + gap, midW = 398;
    constexpr int rightX = midX + midW + gap;
    const int rightW = fb.getWidth() - 24 - rightX;

    drawUnifiedDetails(fb, state, unified, leftX, contentY, leftW, contentH, passive);
    drawUnifiedValues(fb, state, unified, midX, contentY, midW, contentH, passive);
    drawUnifiedMoves(fb, screen, state, unified, rightX, contentY, rightW, contentH, passive);
    redrawGen2DetailsSemantics(fb, state, unified, leftX, contentY, leftW, contentH, passive);
    redrawGen2StatsSemantics(fb, state, unified, midX, contentY, midW, passive);
    redrawGen2MovesSemantics(fb, screen, state, unified, rightX, contentY, rightW, passive);

    if (passive || state.mode == Mode::View) {
        screen.touchButtons.push_back({PokeBank::UIModel::passiveBackTarget, 16, fb.getHeight() - 68, 190, 58});
        drawNavBar(fb, {{"B", "Back"}, {"Touch", "Back"}});
    } else if (state.mode == Mode::Create) {
        drawNavBar(fb, {{"D-pad/Stick", "Navigate"}, {"L/R", "Panel"}, {"A", "Edit"},
                        {"Y", "Random DVs"}, {"X", "Add"}, {"B", "Cancel Draft"}});
    } else {
        drawNavBar(fb, {{"D-pad/Stick", "Navigate"}, {"L/R", "Panel"}, {"A", "Edit"},
                        {"Y", "Random DVs"}, {"B", "Back"}});
    }
}

void drawFullscreenGen2Active(TrainerViewScreen& screen, PKSEFramebuffer& fb) {
    auto& state = stateFor(screen);
    auto& unified = unifiedStateFor(screen);
    drawFullscreenGen2Workspace(screen, fb, state, unified, state.mode == Mode::View, gen2RecordContext(state));
    if (state.itemPicker) drawHeldItemPicker(fb, state);
    if (state.confirmExit) drawConfirm(fb);
    if (unified.progressionEditor) drawProgressionEditor(fb, state, unified);
}

void drawFullscreenGen2Passive(TrainerViewScreen& screen, PKSEFramebuffer& fb,
                               const Pokemon::Pokemon2ReadOnly& pokemon) {
    State visual{};
    visual.begin(pokemon.strictRecord(), Mode::View);
    refreshDerived(visual.working);
    UnifiedWorkspaceState unified{};
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
    const Color valueColor = screen.trainer.trainerGender == 0 ? Colors::Blue :
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
