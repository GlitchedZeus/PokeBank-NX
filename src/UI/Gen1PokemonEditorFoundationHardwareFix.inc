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
    const auto stats = Editor::calculateBattleStats(species, level, dvs, statExp).asArray();

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

    // Cover only the old single-radar region; move/status rows above remain untouched.
    constexpr int splitY = contentY + 306;
    constexpr int splitH = 198;
    constexpr int inset = 12;
    constexpr int gap = 10;
    constexpr int innerW = rightW - inset * 2;
    constexpr int leftPaneW = (innerW - gap) / 2;
    constexpr int rightPaneW = innerW - gap - leftPaneW;
    constexpr int leftPaneX = rightX + inset;
    constexpr int rightPaneX = leftPaneX + leftPaneW + gap;

    // Mask the previous full-width radar rendering before placing the two new panes.
    fb.drawFilledRect(rightX + 6, splitY - 8, rightW - 12, splitH + 16, Colors::Panel);

    drawPanelSurface(fb, leftPaneX, splitY, leftPaneW, splitH, false, 12);
    drawPanelSurface(fb, rightPaneX, splitY, rightPaneW, splitH, false, 12);

    constexpr auto gen1Caps = Foundation::capabilitiesForGeneration(Foundation::Generation::Gen1);
    static_assert(!gen1Caps.supportsHeldItem && !gen1Caps.supportsRibbons);

    fb.drawText(leftPaneX + 12, splitY + 12, "GEN I DATA", Colors::Accent, TextStyle::Caption);
    fb.drawText(leftPaneX + 12, splitY + 52, "No additional fields", Colors::Text, TextStyle::Caption);
    fb.drawText(leftPaneX + 12, splitY + 78, "for Generation I", Colors::TextDim, TextStyle::Caption);
    fb.drawText(leftPaneX + 12, splitY + 126, "Capability-driven", Colors::TextDim, TextStyle::Caption);
    fb.drawText(leftPaneX + 12, splitY + 150, "for later generations", Colors::TextDim, TextStyle::Caption);

    fb.drawText(rightPaneX + 12, splitY + 12, "BATTLE STATS", Colors::Accent, TextStyle::Caption);
    ux2DrawRadar(fb, rightPaneX + rightPaneW / 2, splitY + 98, 48, stats);
    fb.drawText(rightPaneX + 10, splitY + 154,
                "HP " + std::to_string(stats[0]) + " Atk " + std::to_string(stats[1]) +
                " Def " + std::to_string(stats[2]),
                Colors::Text, TextStyle::Caption);
    fb.drawText(rightPaneX + 10, splitY + 176,
                "Spe " + std::to_string(stats[3]) + " Spc " + std::to_string(stats[4]),
                Colors::Text, TextStyle::Caption);
}

} // namespace UI::Gen1PokemonEditor
