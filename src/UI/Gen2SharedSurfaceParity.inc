namespace UI::Gen2PokemonEditor {
namespace {

void drawParityGen2Actions(TrainerViewScreen& screen, PKSEFramebuffer& fb) {
    auto& legacy = editorOverlayState(screen);
    auto* editor = stagedEditor(screen);
    if (!editor) return;
    std::string error;
    const auto pokemon = editor->boxedPokemon(static_cast<std::size_t>(legacy.pokemonBox),
                                               static_cast<std::size_t>(legacy.pokemonSlot), error);
    const auto actions = SharedEditor::actionsForSlot(static_cast<bool>(pokemon), gen2ActionCapabilities());
    legacy.pokemonRow = std::clamp(legacy.pokemonRow, 0, std::max(0, static_cast<int>(actions.count) - 1));

    const int w = pokemon ? 650 : 560;
    const int h = pokemon ? 500 : 350;
    const int x = (1280 - w) / 2;
    const int y = (672 - h) / 2;
    fb.drawFilledRect(0, 0, fb.getWidth(), fb.getHeight() - 48, Color(0, 0, 0, 105));
    drawPanelSurface(fb, x, y, w, h, false, 18);
    fb.drawText(x + 24, y + 18, pokemon ? "Pokemon Actions" : "Empty Slot", Colors::Text, TextStyle::Heading);
    fb.drawText(x + 24, y + 52,
        "Box " + std::to_string(legacy.pokemonBox + 1) + " / Slot " +
        std::to_string(legacy.pokemonSlot + 1) + "  •  staged copy only",
        Colors::TextDim, TextStyle::Caption);

    int rowY = y + 86;
    for (std::size_t i = 0; i < actions.count; ++i) {
        const bool selected = static_cast<int>(i) == legacy.pokemonRow;
        if (selected) fb.drawSelectionHighlight(x + 22, rowY, w - 44, 42);
        fb.drawText(x + 38, rowY + 10, sharedActionLabel(actions[i]),
                    selected ? Colors::Text : Colors::TextDim);
        rowY += 46;
    }
    fb.drawText(x + 24, y + h - 38, "Original source is immutable", Colors::Accent, TextStyle::Caption);
    drawNavBar(fb, {{"D-pad/Stick", "Navigate"}, {"A", "Select"}, {"B", "Back"}});
}

void drawParityGen2Review(TrainerViewScreen& screen, PKSEFramebuffer& fb) {
    auto& legacy = editorOverlayState(screen);
    auto* editor = stagedEditor(screen);
    if (!editor) return;
    const auto& changes = editor->pendingChanges();
    constexpr int w = 980, h = 540;
    constexpr int x = (1280 - w) / 2;
    constexpr int y = (672 - h) / 2;
    fb.drawFilledRect(0, 0, fb.getWidth(), fb.getHeight() - 48, Color(0, 0, 0, 105));
    drawPanelSurface(fb, x, y, w, h, false, 18);
    fb.drawText(x + 24, y + 18, "Review Pending Changes", Colors::Text, TextStyle::Heading);
    fb.drawText(x + 24, y + 52,
        std::to_string(changes.size()) + " staged change(s)  •  original source is immutable",
        Colors::TextDim, TextStyle::Caption);
    if (changes.empty()) {
        fb.drawText(x + 30, y + 100, "No pending changes", Colors::TextDim, TextStyle::Body);
    } else {
        const int first = std::clamp(legacy.reviewRow - 4, 0, std::max(0, static_cast<int>(changes.size()) - 9));
        const int last = std::min(static_cast<int>(changes.size()), first + 9);
        for (int i = first; i < last; ++i) {
            const int rowY = y + 88 + (i - first) * 45;
            const bool selected = i == legacy.reviewRow;
            if (selected) fb.drawSelectionHighlight(x + 22, rowY, w - 44, 42);
            const auto& change = changes[static_cast<std::size_t>(i)];
            fb.drawText(x + 38, rowY + 10, change.label, selected ? Colors::Text : Colors::TextDim);
            fb.drawText(x + 350, rowY + 10, change.beforeValue + "  ->  " + change.afterValue,
                        Colors::Text, TextStyle::Caption);
        }
    }
    if (!legacy.lastExportDirectory.empty())
        fb.drawText(x + 24, y + h - 44, "Last verified export: " + legacy.lastExportDirectory,
                    Colors::Success, TextStyle::Caption);
    drawNavBar(fb, {{"D-pad/Stick", "Navigate"}, {"A", "Export Copy"},
                    {"X", "Discard Staged"}, {"B", "Back"}});
}

void drawParityGen2Clone(TrainerViewScreen& screen, PKSEFramebuffer& fb) {
    auto& shared = sharedSurfaceStateFor(screen);
    auto* editor = stagedEditor(screen);
    if (!editor) return;
    const int destination = firstEmptyGen2Slot(*editor, shared.cloneBox);
    constexpr int w = 930, h = 540;
    constexpr int x = (1280 - w) / 2;
    constexpr int y = (672 - h) / 2;
    fb.drawFilledRect(0, 0, fb.getWidth(), fb.getHeight() - 48, Color(0, 0, 0, 105));
    drawPanelSurface(fb, x, y, w, h, false, 18);
    fb.drawText(x + 24, y + 18, "Clone Pokemon — Destination", Colors::Text, TextStyle::Heading);
    fb.drawText(x + 24, y + 52,
        "Box " + std::to_string(shared.cloneBox + 1) + "  •  first writable slot  •  staged copy only",
        Colors::TextDim, TextStyle::Caption);

    constexpr int cols = 5;
    constexpr int cellW = 164, cellH = 72;
    const int startX = x + 48, startY = y + 96;
    for (int slot = 0; slot < static_cast<int>(editor->metadata().boxCapacity); ++slot) {
        const int col = slot % cols, row = slot / cols;
        const int xx = startX + col * cellW, yy = startY + row * cellH;
        std::string error;
        const auto pokemon = editor->boxedPokemon(static_cast<std::size_t>(shared.cloneBox),
                                                   static_cast<std::size_t>(slot), error);
        const bool target = slot == destination;
        if (target) fb.drawSelectionHighlight(xx, yy, cellW - 10, cellH - 10);
        fb.drawRoundedRect(xx, yy, cellW - 10, cellH - 10, 8, target ? Colors::Accent : Colors::Divider, 1);
        fb.drawText(xx + 10, yy + 8, "Slot " + std::to_string(slot + 1), Colors::TextDim, TextStyle::Caption);
        fb.drawText(xx + 10, yy + 34,
            pokemon ? std::string(Names::getSpeciesName(pokemon->species)) : (target ? "Clone here" : "Empty"),
            target ? Colors::Accent : Colors::Text, TextStyle::Caption);
    }
    if (destination < 0)
        fb.drawText(x + 24, y + h - 44, "This box is full — choose another box with L/R", Colors::Error, TextStyle::Caption);
    drawNavBar(fb, {{"L/R", "Box"}, {"A", "Stage Clone"}, {"B", "Cancel"}});
}

} // namespace

bool drawFinalGen2Surface(TrainerViewScreen& screen, PKSEFramebuffer& fb) {
    if (!isGSC(screen)) return false;
    auto& legacy = editorOverlayState(screen);
    const auto& shared = sharedSurfaceStateFor(screen);
    if (stateFor(screen).mode != Mode::None) {
        drawUnifiedWorkspace(screen, fb);
        return true;
    }
    if (shared.cloneActive) { drawParityGen2Clone(screen, fb); return true; }
    if (legacy.active && legacy.review) { drawParityGen2Review(screen, fb); return true; }
    if (legacy.active && legacy.pokemonActions) { drawParityGen2Actions(screen, fb); return true; }
    return false;
}

bool finalGen2SurfaceOwnsFrame(const TrainerViewScreen& screen) noexcept {
    return unifiedGen2SurfaceOwnsFrame(screen);
}

bool handleFinalGen2SurfaceInput(TrainerViewScreen& screen, uint64_t down, uint64_t held,
                                 int stickX, int stickY, const TouchInput& touch) {
    return handleUnifiedGen2SurfaceInput(screen, down, held, stickX, stickY, touch);
}

} // namespace UI::Gen2PokemonEditor
