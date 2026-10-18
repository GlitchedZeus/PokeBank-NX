#include "UI/SharedEditorPresentation.h"
#include "UI/ClassicGameContext.h"
#include "UI/SharedPokemonShell.h"
#include "UI/ClassicMovesPanel.h"
#include "UI/ClassicStatFieldState.h"

namespace UI::Gen1PokemonEditor {

namespace ClassicContext = PokeBank::UIModel::ClassicGameContext;

// The Gen I workspace panels use the shared shell geometry; the retained Gen I constants
// (drawNeutralGen1WorkspaceHeadings) must keep describing the same panels.
static_assert(SharedPokemonShell::Geometry(1280, 720, 48).y == 96);
static_assert(SharedPokemonShell::Geometry(1280, 720, 48).leftX == 24 &&
              SharedPokemonShell::Geometry(1280, 720, 48).leftW == 300);
static_assert(SharedPokemonShell::Geometry(1280, 720, 48).midX == 24 + 300 + 14 &&
              SharedPokemonShell::Geometry(1280, 720, 48).midW == 398);
static_assert(SharedPokemonShell::Geometry(1280, 720, 48).rightX == 24 + 300 + 14 + 398 + 14);

// Hardware fix 2026-09-13: picker state must own both input and drawing even while
// the underlying editor mode remains AddDraft/Edit. The original foundation gate
// looked only at UX2Mode, which left SpeciesPicker active but invisible behind the
// three-panel workspace until another sub-editor changed the mode.
bool foundationPickerActive(const TrainerViewScreen& screen) noexcept {
    return ux2StateFor(screen).picker != UX2Picker::None;
}

bool foundationMoveEditorActive(const TrainerViewScreen& screen) noexcept {
    return ux2StateFor(screen).mode == UX2Mode::MoveEditor;
}

const char* foundationGen1GameShort(SourceGame game) noexcept {
    switch (game) {
        case SourceGame::Red: return "Red";
        case SourceGame::Blue: return "Blue";
        case SourceGame::Yellow: return "Yellow";
    }
    return "Gen I";
}

const char* foundationGrowthShort(uint8_t growth) noexcept {
    switch (growth) {
        case 0: return "Medium Fast";
        case 1: return "Erratic";
        case 2: return "Fluctuating";
        case 3: return "Medium Slow";
        case 4: return "Fast";
        case 5: return "Slow";
    }
    return "Unknown";
}

std::string foundationDexLabel(uint16_t species) {
    std::string dex = std::to_string(species);
    while (dex.size() < 3) dex = "0" + dex;
    return dex;
}

std::string foundationExactGameName(const TrainerViewScreen& screen, SourceGame fallback) {
    const auto exact = ClassicContext::exactGameName(screen.sourceGameId);
    return exact.empty() ? std::string(foundationGen1GameShort(fallback)) : std::string(exact);
}

ClassicContext::SurfaceMode foundationSurfaceMode(UX2Mode mode) noexcept {
    if (mode == UX2Mode::View) return ClassicContext::SurfaceMode::ViewReadOnly;
    if (mode == UX2Mode::AddDraft) return ClassicContext::SurfaceMode::CreateDraft;
    return ClassicContext::SurfaceMode::EditStaged;
}

void drawClassicTypeSprite(PKSEFramebuffer& fb, int x, int y, uint8_t classicType, int height = 22) {
    const int normalized = ClassicContext::normalizedTypeSpriteId(classicType);
    if (normalized < 0) return;
    auto* sprite = SpriteManager::getTypeSprite(static_cast<uint8_t>(normalized));
    if (!sprite || !sprite->data || sprite->height <= 0) return;
    const int width = (sprite->width * height) / sprite->height;
    fb.drawImageScaled(x, y, sprite->width, sprite->height, width, height, sprite->data, sprite->channels);
}

int classicTypeSpriteWidth(uint8_t classicType, int height = 22) {
    const int normalized = ClassicContext::normalizedTypeSpriteId(classicType);
    if (normalized < 0) return 0;
    auto* sprite = SpriteManager::getTypeSprite(static_cast<uint8_t>(normalized));
    if (!sprite || !sprite->data || sprite->height <= 0) return 0;
    return (sprite->width * height) / sprite->height;
}

void drawClassicTypePair(PKSEFramebuffer& fb, int panelX, int panelW, int y,
                         uint8_t type1, uint8_t type2) {
    constexpr int h = 22;
    constexpr int gap = 8;
    const int w1 = classicTypeSpriteWidth(type1, h);
    const int w2 = type2 != type1 ? classicTypeSpriteWidth(type2, h) : 0;
    const int total = w1 + (w2 > 0 ? gap + w2 : 0);
    int x = panelX + (panelW - total) / 2;
    if (w1 > 0) {
        drawClassicTypeSprite(fb, x, y, type1, h);
        x += w1 + gap;
    }
    if (w2 > 0) drawClassicTypeSprite(fb, x, y, type2, h);
}

void drawClassicFocus(PKSEFramebuffer& fb, int x, int y, int w, int h, bool selected,
                      int radius = 6) {
    if (selected) fb.drawRoundedRect(x, y, w, h, radius, Colors::FocusBorder, 2);
}

void drawNeutralGen1WorkspaceHeadings(PKSEFramebuffer& fb) {
    // Retained as a permanent presentation contract for tests and older paths.
    constexpr int contentY = 96;
    constexpr int leftX = 24;
    constexpr int leftW = 300;
    constexpr int midX = leftX + leftW + 14;
    constexpr int midW = 398;
    constexpr int rightX = midX + midW + 14;
    const auto redraw = [&fb](int x, const char* text, TextStyle style) {
        fb.drawText(x + 14, contentY + 10, text, Colors::Text, style);
    };
    redraw(leftX, "DETAILS", TextStyle::Caption);
    redraw(midX, "STATS", TextStyle::Heading);
    redraw(rightX, "MOVES", TextStyle::Heading);
}

void drawExactGen1WorkspaceSubtitle(TrainerViewScreen& screen, PKSEFramebuffer& fb, UX2Mode workspaceMode) {
    auto* e = editor(screen);
    if (!e) return;
    const auto& state = ux2StateFor(screen);
    const std::string record = "Box " + std::to_string(state.box + 1) +
        " / Slot " + std::to_string(state.slot + 1);
    const std::string exact = foundationExactGameName(screen, e->metadata().sourceGame);
    const std::string context = ClassicContext::contextLine(record, exact, foundationSurfaceMode(workspaceMode));
    fb.drawText(28, 66, context, Colors::TextDim, TextStyle::Caption);
}

void drawFullscreenGen1Workspace(TrainerViewScreen& screen, PKSEFramebuffer& fb) {
    auto& state = ux2StateFor(screen);
    const auto workspaceMode = state.mode == UX2Mode::MoveEditor
        ? (state.subForAdd ? UX2Mode::AddDraft : UX2Mode::Edit) : state.mode;
    auto& foundation = foundationStateFor(screen);
    foundationSyncMainMode(screen);
    auto* e = editor(screen);
    if (!e || !foundationMainMode(workspaceMode)) return;

    const bool view = workspaceMode == UX2Mode::View;
    const bool add = workspaceMode == UX2Mode::AddDraft;
    std::string error;
    const auto pokemon = add ? std::optional<PokemonRecord>{} : ux2SelectedPokemon(screen, error);
    if (!add && !pokemon) return;

    const uint16_t species = add ? state.draft.species : pokemon->species;
    const auto dvs = add ? state.draft.dvs : storedDVs(*pokemon);
    const auto& statExp = add ? state.draft.statExperience : pokemon->statExperience;
    const uint8_t level = add ? state.draft.level : pokemon->level;
    const auto presentation = PokeBank::UIModel::presentGen1Pokemon(species, level, dvs, statExp);
    const auto& stats = presentation.battleStats;
    const auto& types = presentation.nativeTypes;
    const auto& moves = add ? state.draft.moves : pokemon->moves;
    const auto& pp = add ? state.draft.pp : pokemon->pp;
    // View is read-only but navigable: focus outlines and field targets are drawn in every
    // mode; only Edit/Create activation (A) changes values.

    screen.touchButtons.clear();
    fb.drawVerticalGradient(0, 0, fb.getWidth(), fb.getHeight(),
        Color(Colors::Background.r, Colors::Background.g, Colors::Background.b, 250),
        Color(Colors::Background.r, Colors::Background.g, Colors::Background.b, 255));

    const std::string name = foundationNickname(state, pokemon ? &*pokemon : nullptr, add, species).empty()
        ? std::string(Names::getSpeciesName(species))
        : foundationNickname(state, pokemon ? &*pokemon : nullptr, add, species);
    fb.drawText(28, 16, name, Colors::Text, TextStyle::Heading);
    if (presentation.shiny) {
        int nw = 0, nh = 0;
        fb.measureText(name, nw, nh, TextStyle::Heading);
        fb.drawShinyMark(40 + nw, 20, 18, Colors::ShinyStar);
    }

    const std::string rightHeader = "Lv. " + std::to_string(level) + "     No. " + foundationDexLabel(species);
    int rw = 0, rh = 0;
    fb.measureText(rightHeader, rw, rh);
    fb.drawText(fb.getWidth() - 28 - rw, 24, rightHeader, Colors::TextDim);

    const std::string mode = std::string(ClassicContext::modeLabel(foundationSurfaceMode(workspaceMode)));
    int mw = 0, mh = 0;
    fb.measureText(mode, mw, mh, TextStyle::Caption);
    fb.drawText((fb.getWidth() - mw) / 2, 23, mode, Colors::Accent, TextStyle::Caption);
    fb.drawFilledRect(0, 60, fb.getWidth(), 2, Colors::Accent);
    drawExactGen1WorkspaceSubtitle(screen, fb, workspaceMode);

    // Panel geometry is the shared Pokemon shell's (identical numbers to the former Gen I
    // constants), so Gen I DETAILS/STATS/MOVES sit exactly where Gen II-V panels do.
    const SharedPokemonShell::Geometry geometry(fb.getWidth(), fb.getHeight(), kNavBarH);
    const int contentY = geometry.y;
    const int contentH = geometry.h;
    const int leftX = geometry.leftX, leftW = geometry.leftW;
    const int midX = geometry.midX, midW = geometry.midW;
    const int rightX = geometry.rightX;
    const int rightW = geometry.rightW;

    drawPanelSurface(fb, leftX, contentY, leftW, contentH, false, 14);
    drawPanelSurface(fb, midX, contentY, midW, contentH, false, 14);
    drawPanelSurface(fb, rightX, contentY, rightW, contentH, false, 14);

    // DETAILS — heading remains neutral; Gen I intentionally shows no native gender.
    fb.drawText(leftX + 14, contentY + 10, "DETAILS", Colors::Text, TextStyle::Caption);
    if (auto* sprite = SpriteManager::getSprite(species, presentation.shiny); sprite && sprite->data)
        fb.drawSpriteStaticContained(leftX + 75, contentY + 34, 150, 136,
                                     sprite->width, sprite->height, sprite->data, sprite->channels);
    else
        fb.drawText(leftX + 82, contentY + 100, "Sprite unavailable", Colors::TextDim, TextStyle::Caption);
    drawClassicTypePair(fb, leftX, leftW, contentY + 176, types[0], types[1]);

    const std::array<std::string, 6> leftLabels{"Species", "Nickname", "Level", "EXP", "OT", "Trainer ID"};
    const std::array<std::string, 6> leftValues{
        speciesCompact(species),
        name,
        std::to_string(level),
        std::to_string(foundationExperience(state, pokemon ? &*pokemon : nullptr, add)),
        foundationOT(state, *e, pokemon ? &*pokemon : nullptr, add),
        std::to_string(foundationTID(state, *e, pokemon ? &*pokemon : nullptr, add)),
    };
    for (int i = 0; i < 6; ++i) {
        const int yy = contentY + 214 + i * 43;
        const bool selected = foundation.focus.panel == Foundation::Panel::Identity && foundation.focus.row == i;
        drawClassicFocus(fb, leftX + 8, yy, leftW - 16, 38, selected);
        screen.touchButtons.push_back({5000 + i, leftX + 8, yy, leftW - 16, 38});
        fb.drawText(leftX + 18, yy + 7, leftLabels[static_cast<size_t>(i)],
                    selected ? Colors::SelectedText : Colors::TextDim, TextStyle::Caption);
        fb.drawText(leftX + 108, yy + 7, leftValues[static_cast<size_t>(i)], Colors::Text, TextStyle::Caption);
    }

    // STATS — the shared field-state contract (UI/ClassicStatFieldState.h): stored DV / Stat Exp
    // are Editable (Text), the derived HP DV and the calculated Stat column are ReadOnly (gold) in
    // View, Create and Edit alike (R2-GOLD).
    fb.drawText(midX + 14, contentY + 10, "STATS", Colors::Text, TextStyle::Heading);
    constexpr int labelW = 76;
    constexpr int cellW = 96;
    const int gridX = midX + 10;
    const int gridY = contentY + 48;
    fb.drawText(gridX + labelW + 18, gridY, "DV", Colors::TextDim, TextStyle::Caption);
    fb.drawText(gridX + labelW + cellW + 8, gridY, "Stat Exp", Colors::TextDim, TextStyle::Caption);
    fb.drawText(gridX + labelW + cellW * 2 + 24, gridY, "Stat", Colors::TextDim, TextStyle::Caption);
    constexpr auto statLabels = Foundation::gen1StatLabels();
    for (int r = 0; r < 5; ++r) {
        const int rowY = gridY + 28 + r * 42;
        const bool rowSelected = foundation.focus.panel == Foundation::Panel::Values &&
                                 foundation.focus.row == r &&
                                 Foundation::valueCellEditable(static_cast<Foundation::ValueRow>(r),
                                                               static_cast<Foundation::ValueColumn>(foundation.focus.column));
        fb.drawText(gridX + 6, rowY + 9, statLabels[static_cast<size_t>(r)],
                    rowSelected ? Colors::SelectedText : Colors::TextDim, TextStyle::Caption);
        for (int c = 0; c < 3; ++c) {
            const int cellX = gridX + labelW + c * cellW;
            const bool selected =
                                  Foundation::valueCellEditable(static_cast<Foundation::ValueRow>(r),
                                                                static_cast<Foundation::ValueColumn>(c)) &&
                                  foundation.focus.panel == Foundation::Panel::Values &&
                                  foundation.focus.row == r && foundation.focus.column == c;
            drawClassicFocus(fb, cellX, rowY, cellW - 4, 34, selected);
            if (Foundation::valueCellEditable(static_cast<Foundation::ValueRow>(r),
                                                           static_cast<Foundation::ValueColumn>(c)))
                screen.touchButtons.push_back({5100 + r * 10 + c, cellX, rowY, cellW - 4, 34});
            std::string value;
            if (c == 0) value = r == 0 ? std::to_string(Editor::derivedHPDV(dvs)) + " *" : std::to_string(dvs[static_cast<size_t>(r - 1)]);
            else if (c == 1) value = std::to_string(statExp[static_cast<size_t>(r)]);
            else value = std::to_string(stats[static_cast<size_t>(r)]);
            const Color valueColor = UI::SharedEditorChrome::fieldColor(
                UI::SharedEditorChrome::FieldState::style(
                    PokeBank::UIModel::ClassicStatFields::cellState(r, c), selected).value);
            fb.drawText(cellX + 10, rowY + 8, value, valueColor, TextStyle::Caption);
        }
    }
    fb.drawText(gridX + 8, gridY + 244, "* HP DV derived / read-only", Colors::TextDim, TextStyle::Caption);
    const bool shinySelected = foundation.focus.panel == Foundation::Panel::Values && foundation.focus.row == 5;
    drawClassicFocus(fb, gridX, gridY + 276, midW - 20, 38, shinySelected);
    screen.touchButtons.push_back({5190, gridX, gridY + 276, midW - 20, 38});
    fb.drawText(gridX + 12, gridY + 286, "Shiny",
                shinySelected ? Colors::SelectedText : Colors::TextDim, TextStyle::Caption);
    fb.drawText(gridX + 130, gridY + 286, presentation.shiny ? "Yes" : "No",
                shinySelected ? Colors::SelectedText : (presentation.shiny ? Colors::ShinyStar : Colors::Text), TextStyle::Caption);

    const std::string splitSpecial = std::to_string(stats[4]);
    fb.drawText(gridX + 12, gridY + 334, "CALCULATED SPECIAL STATS", Colors::TextDim, TextStyle::Caption);
    fb.drawText(gridX + 12, gridY + 362, "SpA", Colors::TextDim, TextStyle::Caption);
    fb.drawText(gridX + 78, gridY + 362, splitSpecial, Colors::ShinyStar, TextStyle::Caption); // ReadOnly (gold)
    fb.drawText(gridX + 194, gridY + 362, "SpD", Colors::TextDim, TextStyle::Caption);
    fb.drawText(gridX + 260, gridY + 362, splitSpecial, Colors::ShinyStar, TextStyle::Caption); // ReadOnly (gold)
    fb.drawText(gridX + 12, gridY + 392, "one stored Gen I Special stat; split display only", Colors::TextDim, TextStyle::Caption);

    // MOVES — the ONE shared Moves panel (UI/ClassicMovesPanel.h), same rows as Gen II-V.
    // An unlearnable move is Invalid in Create (it must be fixed) and Preserved in Edit.
    fb.drawText(rightX + 14, contentY + 10, "MOVES", Colors::Text, TextStyle::Heading);
    {
        const auto& ups = add ? state.draft.ppUps : pokemon->ppUps;
        std::array<int, 4> maxPP{};
        std::array<ClassicMovesPanel::Status, 4> status{};
        for (int i = 0; i < 4; ++i) {
            const auto slot = static_cast<size_t>(i);
            maxPP[slot] = Editor::moveMaxPP(moves[slot], ups[slot]);
            const bool incompatible = !MoveCompatibility::canLearnMove(e->metadata().sourceGame, species, moves[slot]);
            status[slot] = !incompatible ? ClassicMovesPanel::Status::Ok
                : add ? ClassicMovesPanel::Status::Invalid : ClassicMovesPanel::Status::Preserved;
            const auto rect = ClassicMovesPanel::slotRect(i, rightX, contentY, rightW);
            screen.touchButtons.push_back({5200 + i, rect.x, rect.y, rect.w, rect.h});
        }
        const int focusedMove = foundation.focus.panel == Foundation::Panel::Moves ? foundation.focus.row : -1;
        ClassicMovesPanel::draw(fb, rightX, contentY, rightW,
                                ClassicMovesPanel::slots(moves, pp, ups, maxPP, status),
                                Enums::GameVersion::RBY, focusedMove);
    }

    const int splitY = contentY + SharedMovesPanel::kDataPaneOffset;
    const int splitH = contentH - 256;
    constexpr int inset = 12, paneGap = 10, leftPaneW = 148;
    const int innerW = rightW - inset * 2;
    const int rightPaneW = innerW - paneGap - leftPaneW;
    const int leftPaneX = rightX + inset;
    const int rightPaneX = leftPaneX + leftPaneW + paneGap;
    drawPanelSurface(fb, leftPaneX, splitY, leftPaneW, splitH, false, 12);
    drawPanelSurface(fb, rightPaneX, splitY, rightPaneW, splitH, false, 12);

    const uint32_t experience = foundationExperience(state, pokemon ? &*pokemon : nullptr, add);
    const uint8_t growth = Editor::growthRate(species);
    const uint32_t nextLevelExp = level < 100 ? Pokemon::getExpForLevel(static_cast<uint8_t>(level + 1), growth) : experience;
    const uint32_t toNextLevel = level < 100 && nextLevelExp > experience ? nextLevelExp - experience : 0;
    fb.drawText(leftPaneX + 10, splitY + 10, "GEN I DATA", Colors::Accent, TextStyle::Caption);
    fb.drawText(leftPaneX + 10, splitY + 38, "Game", Colors::TextDim, TextStyle::Caption);
    fb.drawText(leftPaneX + 10, splitY + 56, foundationExactGameName(screen, e->metadata().sourceGame), Colors::Text, TextStyle::Caption);
    fb.drawText(leftPaneX + 10, splitY + 80, "Growth", Colors::TextDim, TextStyle::Caption);
    fb.drawText(leftPaneX + 10, splitY + 98, foundationGrowthShort(growth), Colors::Text, TextStyle::Caption);
    fb.drawText(leftPaneX + 10, splitY + 122, "To next Lv", Colors::TextDim, TextStyle::Caption);
    fb.drawText(leftPaneX + 10, splitY + 140,
                level >= 100 ? std::string("MAX") : std::to_string(toNextLevel) + " EXP", Colors::Text, TextStyle::Caption);
    fb.drawText(leftPaneX + 10, splitY + 164, "Catch byte", Colors::TextDim, TextStyle::Caption);
    fb.drawText(leftPaneX + 10, splitY + 182,
                add ? std::string("Species default") : std::to_string(pokemon->catchRate), Colors::Text, TextStyle::Caption);
    // Origin & Legality (formerly under the move rows): Gen I records no origin to check.
    fb.drawText(leftPaneX + 10, splitY + 206, "Legality", Colors::TextDim, TextStyle::Caption);
    fb.drawText(leftPaneX + 10, splitY + 224, "Not checked", Colors::Text, TextStyle::Caption);

    fb.drawText(rightPaneX + 10, splitY + 10, "BATTLE STATS", Colors::Accent, TextStyle::Caption);
    StatsRadar::drawGen1Labeled(fb, rightPaneX + 8, splitY + 32, rightPaneW - 16, splitH - 40, stats);

    if (view) {
        drawNavBar(fb, {{"D-pad/Stick", "Navigate"}, {"L/R", "Panel"}, {"B", "Back"}, {"Touch", "Back"}});
    } else if (foundationStateFor(screen).editExitConfirm) {
        // The exit dialog is modal: its footer offers exactly the dialog's own B/Y/A, so a
        // footer tap (mapped to that button) never reads as a workspace edit shortcut.
        drawNavBar(fb, {{"B", "Back"}, {"Y", "Discard"}, {"A", add ? "Keep" : "Save"}});
    } else if (add) {
        drawNavBar(fb, {{"D-pad/Stick", "Navigate"}, {"L/R", "Panel"}, {"A", "Edit"},
                        {"Y", "Random DVs"}, {"X", "Add"}, {"B", "Exit"}});
    } else {
        drawNavBar(fb, {{"D-pad/Stick", "Navigate"}, {"L/R", "Panel"}, {"A", "Edit"},
                        {"Y", "Random DVs"}, {"X", "Keep"}, {"B", "Exit"}});
    }
}

// The picker owns input while the current fullscreen workspace owns its backdrop.
void drawFoundationPicker(TrainerViewScreen& screen, PKSEFramebuffer& fb) {
    auto& state = ux2StateFor(screen);
    if (!foundationMainMode(state.mode) && state.mode != UX2Mode::MoveEditor) {
        drawOverlayUXCleanup3(screen, fb);
        return;
    }
    drawFullscreenGen1Workspace(screen, fb);
    if (!foundationPickerActive(screen)) {
        // The move-details dialog retains the same workspace before/after choosing a move.
        drawOverlayUXCleanup3(screen, fb);
        return;
    }
    // drawFullscreenGen1Workspace published its background field hitboxes.
    // The modal owns the next frame's entire touch registry; discard every
    // underlying DETAILS/STATS/MOVES target before registering picker rows.
    screen.touchButtons.clear();
    // ux3DrawPicker applies the existing dimmer exactly once.
    const int w = state.picker == UX2Picker::Species ? 1080 : kGen1MovePickerW;
    constexpr int h = 520;
    ux3DrawPicker(screen, fb, (1280 - w) / 2, (672 - h) / 2, w, h);
    ux3DrawTouchNavBar(fb, state);
}

// Final Gen I presentation pass. The accepted editor still runs underneath, but this
// repaint owns the whole 1280x720 page so the browser/modal shell cannot leak through.
void drawFoundationBottomSplit(TrainerViewScreen& screen, PKSEFramebuffer& fb) {
    auto& state = ux2StateFor(screen);
    if (!foundationMainMode(state.mode) || foundationPickerActive(screen)) return;
    drawFullscreenGen1Workspace(screen, fb);
    if (foundationStateFor(screen).editExitConfirm) foundationDrawExitConfirm(screen, fb);
}

} // namespace UI::Gen1PokemonEditor
