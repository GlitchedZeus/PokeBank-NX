namespace UI::Gen1PokemonEditor {

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

// Hardware fix 2026-09-15: panel focus is communicated only by the compact red
// field/cell cursor. Main workspace headings stay neutral in every mode and Gen I
// uses the same DETAILS / STATS / MOVES vocabulary as the shared Gen II surface.
void drawNeutralGen1WorkspaceHeadings(PKSEFramebuffer& fb) {
    constexpr int workspaceX = 40;
    constexpr int workspaceY = 31;
    constexpr int contentY = workspaceY + 74;
    constexpr int leftX = workspaceX + 18;
    constexpr int leftW = 300;
    constexpr int midX = leftX + leftW + 14;
    constexpr int midW = 398;
    constexpr int rightX = midX + midW + 14;

    const auto redraw = [&fb](int x, const char* text, TextStyle style) {
        fb.drawFilledRect(x + 10, contentY + 6, 150, 34, Colors::Panel);
        fb.drawText(x + 14, contentY + 10, text, Colors::Text, style);
    };
    redraw(leftX, "DETAILS", TextStyle::Caption);
    redraw(midX, "STATS", TextStyle::Heading);
    redraw(rightX, "MOVES", TextStyle::Heading);
}

// Layout requested during the same hardware pass: reserve the lower-right workspace
// as two sibling panes. The right pane is always the generation's battle-stat radar;
// the left pane is capability-driven extra data. Generation I deliberately exposes
// no Held Item/Ribbon rows because those mechanics do not exist there. Instead it
// shows truthful format-native/source information that is useful while editing.
void drawFoundationBottomSplit(TrainerViewScreen& screen, PKSEFramebuffer& fb) {
    auto& state = ux2StateFor(screen);
    if (!foundationMainMode(state.mode) || foundationPickerActive(screen)) return;
    if (foundationStateFor(screen).editExitConfirm) return;

    auto* e = editor(screen);
    if (!e) return;
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
    const uint32_t experience = foundationExperience(state, pokemon ? &*pokemon : nullptr, add);
    const uint8_t growth = Editor::growthRate(species);
    const uint32_t nextLevelExp = level < 100 ? Pokemon::getExpForLevel(static_cast<uint8_t>(level + 1), growth) : experience;
    const uint32_t toNextLevel = level < 100 && nextLevelExp > experience ? nextLevelExp - experience : 0;

    // Keep these coordinates tied to the permanent foundation workspace geometry.
    constexpr int workspaceX = 40;
    constexpr int workspaceY = 31;
    constexpr int workspaceW = 1200;
    constexpr int contentY = workspaceY + 74;
    constexpr int leftX = workspaceX + 18;
    constexpr int leftW = 300;
    constexpr int midX = leftX + leftW + 14;
    constexpr int midW = 398;
    constexpr int rightX = midX + midW + 14;
    constexpr int rightW = workspaceX + workspaceW - 18 - rightX;

    drawNeutralGen1WorkspaceHeadings(fb);

    // One authoritative split renderer, below move/status rows.
    constexpr int splitY = contentY + 288;
    constexpr int splitH = 216;
    constexpr int inset = 12;
    constexpr int gap = 10;
    constexpr int innerW = rightW - inset * 2;
    constexpr int leftPaneW = 136;
    constexpr int rightPaneW = innerW - gap - leftPaneW;
    constexpr int leftPaneX = rightX + inset;
    constexpr int rightPaneX = leftPaneX + leftPaneW + gap;

    drawPanelSurface(fb, leftPaneX, splitY, leftPaneW, splitH, false, 12);
    drawPanelSurface(fb, rightPaneX, splitY, rightPaneW, splitH, false, 12);

    const auto& gen1Caps = presentation.capabilities;
    if (!gen1Caps.supportsHeldItem && !gen1Caps.supportsRibbons) {
        fb.drawText(leftPaneX + 10, splitY + 10, "GEN I DATA", Colors::Accent, TextStyle::Caption);
        fb.drawText(leftPaneX + 10, splitY + 40, "Game", Colors::TextDim, TextStyle::Caption);
        fb.drawText(leftPaneX + 10, splitY + 58, foundationGen1GameShort(e->metadata().sourceGame), Colors::Text, TextStyle::Caption);
        fb.drawText(leftPaneX + 10, splitY + 82, "Growth", Colors::TextDim, TextStyle::Caption);
        fb.drawText(leftPaneX + 10, splitY + 100, foundationGrowthShort(growth), Colors::Text, TextStyle::Caption);
        fb.drawText(leftPaneX + 10, splitY + 124, "To next Lv", Colors::TextDim, TextStyle::Caption);
        fb.drawText(leftPaneX + 10, splitY + 142,
                    level >= 100 ? std::string("MAX") : std::to_string(toNextLevel) + " EXP",
                    Colors::Text, TextStyle::Caption);
        fb.drawText(leftPaneX + 10, splitY + 166, "Catch byte", Colors::TextDim, TextStyle::Caption);
        fb.drawText(leftPaneX + 10, splitY + 184,
                    add ? std::string("Species default") : std::to_string(pokemon->catchRate),
                    Colors::Text, TextStyle::Caption);
    }
    fb.drawText(rightPaneX + 10, splitY + 10, "BATTLE STATS", Colors::Accent, TextStyle::Caption);
    StatsRadar::drawGen1Labeled(fb, rightPaneX + 8, splitY + 32, rightPaneW - 16, splitH - 40, stats);
}

} // namespace UI::Gen1PokemonEditor
