#include "UI/ClassicGameContext.h"

namespace UI::Gen1PokemonEditor {

namespace ClassicContext = PokeBank::UIModel::ClassicGameContext;

// Hardware fix 2026-09-13: picker state must own both input and drawing even while
// the underlying editor mode remains AddDraft/Edit. The original foundation gate
// looked only at UX2Mode, which left SpeciesPicker active but invisible behind the
// three-panel workspace until another sub-editor changed the mode.
bool foundationPickerActive(const TrainerViewScreen& screen) noexcept {
    return ux2StateFor(screen).picker != UX2Picker::None;
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

void drawExactGen1WorkspaceSubtitle(TrainerViewScreen& screen, PKSEFramebuffer& fb) {
    auto* e = editor(screen);
    if (!e) return;
    const auto& state = ux2StateFor(screen);
    const std::string record = "Box " + std::to_string(state.box + 1) +
        " / Slot " + std::to_string(state.slot + 1);
    const std::string exact = foundationExactGameName(screen, e->metadata().sourceGame);
    const std::string context = ClassicContext::contextLine(record, exact, foundationSurfaceMode(state.mode));
    fb.drawText(28, 66, context, Colors::TextDim, TextStyle::Caption);
}

void drawFullscreenGen1Workspace(TrainerViewScreen& screen, PKSEFramebuffer& fb) {
    auto& state = ux2StateFor(screen);
    auto& foundation = foundationStateFor(screen);
    foundationSyncMainMode(screen);
    auto* e = editor(screen);
    if (!e || !foundationMainMode(state.mode) || foundationPickerActive(screen)) return;

    const bool view = state.mode == UX2Mode::View;
    const bool add = state.mode == UX2Mode::AddDraft;
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
    const auto badMove = MoveCompatibility::firstIncompatible(e->metadata().sourceGame, species, moves);
    const Color incompatibleColor = add ? Colors::Error : Colors::Warning;
    const bool editable = !view;

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

    const std::string mode = std::string(ClassicContext::modeLabel(foundationSurfaceMode(state.mode)));
    int mw = 0, mh = 0;
    fb.measureText(mode, mw, mh, TextStyle::Caption);
    fb.drawText((fb.getWidth() - mw) / 2, 23, mode, Colors::Accent, TextStyle::Caption);
    fb.drawFilledRect(0, 60, fb.getWidth(), 2, Colors::Accent);
    drawExactGen1WorkspaceSubtitle(screen, fb);

    constexpr int contentY = 96;
    const int contentH = fb.getHeight() - contentY - kNavBarH - 8;
    constexpr int leftX = 24, leftW = 300;
    constexpr int gap = 14;
    constexpr int midX = leftX + leftW + gap, midW = 398;
    constexpr int rightX = midX + midW + gap;
    const int rightW = fb.getWidth() - 24 - rightX;

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

    const std::array<std::string, 5> leftLabels{"Species", "Nickname", "Level / EXP", "OT", "Trainer ID"};
    const std::array<std::string, 5> leftValues{
        speciesCompact(species),
        name,
        "Lv " + std::to_string(level) + " / " + std::to_string(foundationExperience(state, pokemon ? &*pokemon : nullptr, add)),
        foundationOT(state, *e, pokemon ? &*pokemon : nullptr, add),
        std::to_string(foundationTID(state, *e, pokemon ? &*pokemon : nullptr, add)),
    };
    for (int i = 0; i < 5; ++i) {
        const int yy = contentY + 214 + i * 43;
        const bool selected = editable && foundation.focus.panel == Foundation::Panel::Identity && foundation.focus.row == i;
        drawClassicFocus(fb, leftX + 8, yy, leftW - 16, 38, selected);
        fb.drawText(leftX + 18, yy + 7, leftLabels[static_cast<size_t>(i)], Colors::TextDim, TextStyle::Caption);
        fb.drawText(leftX + 108, yy + 7, leftValues[static_cast<size_t>(i)], Colors::Text, TextStyle::Caption);
    }

    // STATS — DV and Stat Exp stay neutral; calculated Stat is semantic accent.
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
        fb.drawText(gridX + 6, rowY + 9, statLabels[static_cast<size_t>(r)], Colors::TextDim, TextStyle::Caption);
        for (int c = 0; c < 3; ++c) {
            const int cellX = gridX + labelW + c * cellW;
            const bool selected = editable && foundation.focus.panel == Foundation::Panel::Values &&
                                  foundation.focus.row == r && foundation.focus.column == c;
            drawClassicFocus(fb, cellX, rowY, cellW - 4, 34, selected);
            std::string value;
            if (c == 0) value = r == 0 ? std::to_string(Editor::derivedHPDV(dvs)) + " *" : std::to_string(dvs[static_cast<size_t>(r - 1)]);
            else if (c == 1) value = std::to_string(statExp[static_cast<size_t>(r)]);
            else value = std::to_string(stats[static_cast<size_t>(r)]);
            const Color valueColor = c == 2 ? Colors::Accent : Colors::Text;
            fb.drawText(cellX + 10, rowY + 8, value, valueColor, TextStyle::Caption);
        }
    }
    fb.drawText(gridX + 8, gridY + 244, "* HP DV derived / read-only", Colors::TextDim, TextStyle::Caption);
    const bool shinySelected = editable && foundation.focus.panel == Foundation::Panel::Values && foundation.focus.row == 5;
    const bool levelSelected = editable && foundation.focus.panel == Foundation::Panel::Values && foundation.focus.row == 6;
    drawClassicFocus(fb, gridX, gridY + 276, midW - 20, 38, shinySelected);
    fb.drawText(gridX + 12, gridY + 286, "Shiny", Colors::TextDim, TextStyle::Caption);
    fb.drawText(gridX + 130, gridY + 286, presentation.shiny ? "Yes" : "No",
                presentation.shiny ? Colors::ShinyStar : Colors::Text, TextStyle::Caption);
    drawClassicFocus(fb, gridX, gridY + 320, midW - 20, 38, levelSelected);
    fb.drawText(gridX + 12, gridY + 330, "Level", Colors::TextDim, TextStyle::Caption);
    fb.drawText(gridX + 130, gridY + 330, std::to_string(level), Colors::Text, TextStyle::Caption);

    const std::string splitSpecial = std::to_string(stats[4]);
    fb.drawText(gridX + 12, gridY + 374, "CALCULATED SPECIAL STATS", Colors::TextDim, TextStyle::Caption);
    fb.drawText(gridX + 12, gridY + 402, "SpA", Colors::TextDim, TextStyle::Caption);
    fb.drawText(gridX + 78, gridY + 402, splitSpecial, Colors::Accent, TextStyle::Caption);
    fb.drawText(gridX + 194, gridY + 402, "SpD", Colors::TextDim, TextStyle::Caption);
    fb.drawText(gridX + 260, gridY + 402, splitSpecial, Colors::Accent, TextStyle::Caption);
    fb.drawText(gridX + 12, gridY + 432, "one stored Gen I Special stat; split display only", Colors::TextDim, TextStyle::Caption);

    // MOVES — selected row gets only the compact focus outline; text does not become red.
    fb.drawText(rightX + 14, contentY + 10, "MOVES", Colors::Text, TextStyle::Heading);
    for (int i = 0; i < 4; ++i) {
        const int rowY = contentY + 44 + i * 40;
        const bool selected = editable && foundation.focus.panel == Foundation::Panel::Moves && foundation.focus.row == i;
        drawClassicFocus(fb, rightX + 10, rowY, rightW - 20, 36, selected);
        const bool incompatible = !MoveCompatibility::canLearnMove(e->metadata().sourceGame, species, moves[static_cast<size_t>(i)]);
        std::string moveText = moves[static_cast<size_t>(i)] == 0 ? "Empty" : std::string(Names::getMoveName(moves[static_cast<size_t>(i)]));
        if (incompatible) moveText = "[!] " + moveText;
        fb.drawText(rightX + 20, rowY + 8, moveText, incompatible ? incompatibleColor : Colors::Text, TextStyle::Caption);
        const std::string status = incompatible ? (add ? "Invalid" : "Unusual") : "OK";
        int statusW = 0, statusH = 0;
        fb.measureText(status, statusW, statusH, TextStyle::Caption);
        const int statusX = rightX + rightW - 16 - statusW;
        fb.drawText(statusX, rowY + 8, status, incompatible ? incompatibleColor : Colors::Success, TextStyle::Caption);
        const auto ups = add ? state.draft.ppUps[static_cast<size_t>(i)] : pokemon->ppUps[static_cast<size_t>(i)];
        if (moves[static_cast<size_t>(i)] != 0) {
            const std::string ppText = "PP " + std::to_string(pp[static_cast<size_t>(i)]) + "  Up " + std::to_string(ups);
            int ppW = 0, ppH = 0;
            fb.measureText(ppText, ppW, ppH, TextStyle::Caption);
            fb.drawText(statusX - 12 - ppW, rowY + 8, ppText, Colors::TextDim, TextStyle::Caption);
        }
    }
    const int statusY = contentY + 214;
    fb.drawHDivider(rightX + 14, statusY - 12, rightW - 28);
    fb.drawText(rightX + 18, statusY, "Move compatibility", Colors::TextDim, TextStyle::Caption);
    const std::string compatibilityText = !badMove ? "OK" : (add ? "Needs correction" : "Unusual preserved");
    const Color compatibilityColor = !badMove ? Colors::Success : (add ? Colors::Error : Colors::Warning);
    fb.drawText(rightX + rightW - 154, statusY, compatibilityText, compatibilityColor, TextStyle::Caption);
    fb.drawText(rightX + 18, statusY + 26, "Encounter legality", Colors::TextDim, TextStyle::Caption);
    fb.drawText(rightX + rightW - 120, statusY + 26, "Not checked", Colors::Text, TextStyle::Caption);

    const int splitY = contentY + 276;
    const int splitH = contentH - 288;
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

    fb.drawText(rightPaneX + 10, splitY + 10, "BATTLE STATS", Colors::Accent, TextStyle::Caption);
    StatsRadar::drawGen1Labeled(fb, rightPaneX + 8, splitY + 32, rightPaneW - 16, splitH - 40, stats);

    if (view) {
        drawNavBar(fb, {{"B", "Back"}, {"Touch", "Back"}});
    } else if (add) {
        drawNavBar(fb, {{"D-pad/Stick", "Navigate"}, {"L/R", "Panel"}, {"A", "Edit"},
                        {"Y", "Random DVs"}, {"X", "Add"}, {"B", "Cancel Draft"}});
    } else {
        drawNavBar(fb, {{"D-pad/Stick", "Navigate"}, {"L/R", "Panel"}, {"A", "Edit"},
                        {"Y", "Random DVs"}, {"B", "Back"}});
    }
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
