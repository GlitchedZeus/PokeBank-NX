namespace UI::Gen1PokemonEditor {
namespace {

// Hardware-cleanup parity layer only. The accepted Cleanup3 editor remains the owner of layout,
// navigation and mutation; this adds the same green per-move compatibility marker used by the
// shared Gen II presentation without changing any Gen I mechanics or legality claims.
void drawGen1MoveStatusParity(TrainerViewScreen& screen, PKSEFramebuffer& fb) {
    if (!isGen1SourceUX(screen)) return;
    auto& state = ux2StateFor(screen);
    if (state.mode != UX2Mode::Edit && state.mode != UX2Mode::AddDraft) return;
    auto* e = editor(screen);
    if (!e) return;

    const bool add = state.mode == UX2Mode::AddDraft;
    std::string error;
    const auto pokemon = add ? std::optional<PokemonRecord>{} : ux2SelectedPokemon(screen, error);
    if (!add && !pokemon) return;
    const uint16_t species = add ? state.draft.species : pokemon->species;
    const auto& moves = add ? state.draft.moves : pokemon->moves;

    constexpr int workspaceX = 40;
    constexpr int workspaceY = 31;
    constexpr int workspaceW = 1200;
    constexpr int workspaceH = 610;
    constexpr int leftW = 300;
    constexpr int centerW = 430;
    constexpr int rowH = 42;
    const int contentY = workspaceY + 78;
    const int contentH = workspaceH - 104;
    const int centerX = workspaceX + 20 + leftW + 14;
    const int listY = contentY + 52;
    const int viewportH = contentH - 68;
    const int visible = std::max(1, viewportH / rowH);
    const int start = std::clamp(state.row - visible / 2, 0, std::max(0, 14 - visible));

    for (int slot = 0; slot < 4; ++slot) {
        const int field = 4 + slot;
        if (field < start || field >= start + visible) continue;
        const uint16_t move = moves[static_cast<std::size_t>(slot)];
        if (move == 0 || !MoveCompatibility::canLearnMove(e->metadata().sourceGame, species, move)) continue;
        const int rowY = listY + (field - start) * rowH;
        // Place the status between the row label and right-aligned value so PP/Ups stay readable.
        fb.drawText(centerX + 142, rowY + 12, "OK", Colors::Success, TextStyle::Caption);
    }
}

} // namespace
} // namespace UI::Gen1PokemonEditor
