namespace UI::Gen1PokemonEditor {

// Hardware fix 2026-09-13: picker state must own both input and drawing even while
// the underlying editor mode remains AddDraft/Edit. The original foundation gate
// looked only at UX2Mode, which left SpeciesPicker active but invisible behind the
// three-panel workspace until another sub-editor changed the mode.
bool foundationPickerActive(const TrainerViewScreen& screen) noexcept {
    return ux2StateFor(screen).picker != UX2Picker::None;
}

// Layout requested during the same hardware pass: reserve the lower-right workspace
// as two sibling panes. The right pane is always the generation's battle-stat radar;
// the left pane is capability-driven extra data. Generation I deliberately exposes
// no Held Item/Ribbon rows because those mechanics do not exist there.
void drawFoundationBottomSplit(TrainerViewScreen& screen, PKSEFramebuffer& fb) {
    auto& state = ux2StateFor(screen);
    if (!foundationMainMode(state.mode) || foundationPickerActive(screen)) return;

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
        fb.drawText(leftPaneX + 10, splitY + 12, "GEN I DATA", Colors::Accent, TextStyle::Caption);
        fb.drawText(leftPaneX + 10, splitY + 54, "No additional", Colors::TextDim, TextStyle::Caption);
        fb.drawText(leftPaneX + 10, splitY + 76, "native fields", Colors::TextDim, TextStyle::Caption);
    }
    fb.drawText(rightPaneX + 10, splitY + 10, "BATTLE STATS", Colors::Accent, TextStyle::Caption);
    StatsRadar::drawGen1Labeled(fb, rightPaneX + 8, splitY + 32, rightPaneW - 16, splitH - 40, stats);
}

} // namespace UI::Gen1PokemonEditor
