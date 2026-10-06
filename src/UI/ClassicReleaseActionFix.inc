// Part 2 of PR #74: destructive single-Pokemon removal is user-facing Release and lives only
// behind A -> Pokemon Actions -> Release -> explicit confirmation. No bulk-release path exists.

namespace UI::Gen1PokemonEditor {
namespace {

const char* releaseActionLabel(PokeBank::UIModel::Gen1Editor::Action action) noexcept {
    using Action = PokeBank::UIModel::Gen1Editor::Action;
    switch (action) {
        case Action::View: return "View";
        case Action::Edit: return "Edit";
        case Action::Clone: return "Clone";
        case Action::Remove: return "Release";
        case Action::LegalityProvenance: return "Legality & Provenance";
        case Action::AddPokemon: return "Add Pokemon";
        case Action::ReviewPendingChanges: return "Review Pending Changes";
        case Action::Cancel: return "Cancel";
    }
    return "";
}

} // namespace

bool handleReleaseActionInput(TrainerViewScreen& screen, uint64_t down, uint64_t held,
                              int stickX, int stickY, const TouchInput& touch) {
    if (!isGen1SourceUX(screen)) return false;
    auto& state = ux2StateFor(screen);
    if (state.mode == UX2Mode::Actions) {
        const uint64_t nav = screen.controllerNavigation.apply(
            down, held, stickX, stickY,
            HidNpadButton_Up, HidNpadButton_Down,
            HidNpadButton_Left, HidNpadButton_Right);
        std::string error;
        const auto pokemon = ux2SelectedPokemon(screen, error);
        const auto actions = PokeBank::UIModel::Gen1Editor::actionsForSlot(static_cast<bool>(pokemon));
        const int rows = static_cast<int>(actions.count);
        if (rows <= 0) return true;
        if (touch.justTouchedDown()) {
            for (const auto& hit : screen.touchButtons) {
                if (hit.id < 300 || hit.id >= 300 + rows) continue;
                if (touch.x() >= hit.x && touch.x() < hit.x + hit.w &&
                    touch.y() >= hit.y && touch.y() < hit.y + hit.h) {
                    state.row = hit.id - 300;
                    break;
                }
            }
        }
        const int touchedAction = screen.touchedButtonId(touch);
        if (touchedAction >= 300 && touchedAction < 300 + rows) {
            state.row = touchedAction - 300;
            down |= HidNpadButton_A;
        }
        if (nav & HidNpadButton_Up) state.row = (state.row + rows - 1) % rows;
        if (nav & HidNpadButton_Down) state.row = (state.row + 1) % rows;
        if (down & HidNpadButton_B) {
            ux2Close(state);
            screen.controllerNavigation.reset();
            return true;
        }
        if (!(down & HidNpadButton_A)) return true;
        using Action = PokeBank::UIModel::Gen1Editor::Action;
        switch (actions[static_cast<std::size_t>(state.row)]) {
            case Action::View: state.mode = UX2Mode::View; break;
            case Action::Edit: state.mode = UX2Mode::Edit; state.row = 0; break;
            case Action::Clone: ux2OpenClone(screen); break;
            case Action::Remove: state.mode = UX2Mode::RemoveConfirm; break;
            case Action::LegalityProvenance: state.mode = UX2Mode::Provenance; break;
            case Action::AddPokemon: ux2BeginAdd(screen); break;
            case Action::ReviewPendingChanges: state.mode = UX2Mode::Review; state.reviewRow = 0; break;
            case Action::Cancel: ux2Close(state); break;
        }
        return true;
    }
    if (state.mode == UX2Mode::RemoveConfirm) {
        const int touched = screen.touchedButtonId(touch);
        if (touched == 398) down |= HidNpadButton_B;
        else if (touched == 399) down |= HidNpadButton_A;
        if (down & HidNpadButton_B) { state.mode = UX2Mode::Actions; state.row = 0; return true; }
        if (down & HidNpadButton_A) ux2StageRemove(screen);
        return true;
    }
    return false;
}

bool drawReleaseActionSurface(TrainerViewScreen& screen, PKSEFramebuffer& fb) {
    if (!isGen1SourceUX(screen)) return false;
    auto& state = ux2StateFor(screen);
    if (state.mode != UX2Mode::Actions && state.mode != UX2Mode::RemoveConfirm) return false;

    screen.touchButtons.clear();
    int x = 0, y = 0, w = 0, h = 0;
    ux3DialogGeometry(screen, x, y, w, h);
    fb.drawFilledRect(0, 0, fb.getWidth(), fb.getHeight() - 48, Color(0, 0, 0, 105));
    ux2DrawPanel(fb, x, y, w, h);

    if (state.mode == UX2Mode::RemoveConfirm) {
        fb.drawText(x + 24, y + 18, "Release Pokemon?", Colors::Text, TextStyle::Heading);
        fb.drawText(x + 28, y + 86, "Release removes this Pokemon from the staged packed box.", Colors::Text);
        fb.drawText(x + 28, y + 126, "Original RetroArch .srm remains immutable and is never written.", Colors::Accent);
        const int bw = 190, bh = TouchTargetMin, by = y + h - bh - 18;
        const int confirmX = x + w - bw - 22, cancelX = confirmX - bw - 16;
        drawGlyphButton(fb, cancelX, by, bw, bh, "B", "Cancel");
        screen.touchButtons.push_back({398, cancelX, by, bw, bh});
        drawGlyphButton(fb, confirmX, by, bw, bh, "A", "Release");
        screen.touchButtons.push_back({399, confirmX, by, bw, bh});
        drawNavBar(fb, {{"A", "Confirm Release"}, {"B", "Cancel"}});
        return true;
    }

    std::string error;
    const auto pokemon = ux2SelectedPokemon(screen, error);
    const auto actions = PokeBank::UIModel::Gen1Editor::actionsForSlot(static_cast<bool>(pokemon));
    state.row = std::clamp(state.row, 0, std::max(0, static_cast<int>(actions.count) - 1));
    fb.drawText(x + 24, y + 18, pokemon ? "Pokemon Actions" : "Empty Slot", Colors::Text, TextStyle::Heading);
    fb.drawText(x + 24, y + 52,
        "Box " + std::to_string(state.box + 1) + " / Slot " + std::to_string(state.slot + 1) +
        "  •  staged copy only", Colors::TextDim, TextStyle::Caption);
    int rowY = y + 86;
    for (std::size_t i = 0; i < actions.count; ++i) {
        const bool selected = static_cast<int>(i) == state.row;
        if (selected) fb.drawSelectionHighlight(x + 22, rowY, w - 44, 42);
        fb.drawText(x + 38, rowY + 10, releaseActionLabel(actions[i]),
                    selected ? Colors::Text : Colors::TextDim);
        screen.touchButtons.push_back({300 + static_cast<int>(i), x + 22, rowY, w - 44, 42});
        rowY += 46;
    }
    fb.drawText(x + 24, y + h - 38, "Original source is immutable", Colors::Accent, TextStyle::Caption);
    drawNavBar(fb, {{"Up/Down", "Navigate"}, {"A", "Select"}, {"B", "Back"}});
    return true;
}

} // namespace UI::Gen1PokemonEditor

namespace UI::Gen2PokemonEditor {
namespace {

struct ReleaseConfirmState {
    const TrainerViewScreen* owner = nullptr;
    std::string sourceGameId;
    bool active = false;
};

ReleaseConfirmState& releaseConfirmStateFor(const TrainerViewScreen& screen) {
    static ReleaseConfirmState state;
    if (state.owner != &screen || state.sourceGameId != screen.sourceGameId) {
        state = {};
        state.owner = &screen;
        state.sourceGameId = screen.sourceGameId;
    }
    return state;
}

PokeBank::UIModel::SharedPokemonEditor::ActionCapabilities releaseActionCapabilities() noexcept {
    PokeBank::UIModel::SharedPokemonEditor::ActionCapabilities caps;
    caps.canClone = true;
    caps.canRemove = true;
    caps.hasLegalityProvenance = true;
    return caps;
}

const char* releaseActionLabel(PokeBank::UIModel::SharedPokemonEditor::Action action) noexcept {
    using Action = PokeBank::UIModel::SharedPokemonEditor::Action;
    if (action == Action::Remove) return "Release";
    return PokeBank::UIModel::SharedPokemonEditor::actionLabel(action);
}

void dismissActionSheetForWorkspace(TrainerViewScreen& screen) {
    auto& legacy = editorOverlayState(screen);
    legacy.active = false;
    legacy.review = false;
    legacy.pokemonActions = false;
    legacy.pokemonFields = false;
    legacy.pokemonRow = 0;
}

bool confirmRelease(TrainerViewScreen& screen) {
    auto* editor = stagedEditor(screen);
    auto& legacy = editorOverlayState(screen);
    auto& confirm = releaseConfirmStateFor(screen);
    if (!editor) { confirm.active = false; return true; }

    std::string error;
    const int oldSlot = legacy.pokemonSlot;
    if (!editor->stageReleaseBoxPokemon(static_cast<std::size_t>(legacy.pokemonBox),
                                        static_cast<std::size_t>(legacy.pokemonSlot), error)) {
        screen.postStatus(error.empty() ? "Release was rejected" : error, 340);
        return true;
    }
    if (!gscTrainer(screen).refreshStagedBoxPresentation(error)) {
        screen.postStatus(error.empty() ? "Release staged, but box presentation refresh failed" : error, 360);
    } else {
        screen.postStatus("Pokemon Release staged only; original RetroArch .srm unchanged", 300);
    }

    int count = 0;
    for (int slot = 0; slot < static_cast<int>(editor->metadata().boxCapacity); ++slot) {
        std::string ignored;
        if (!editor->boxedPokemon(static_cast<std::size_t>(legacy.pokemonBox),
                                  static_cast<std::size_t>(slot), ignored)) break;
        ++count;
    }
    screen.selectedBoxIndex = legacy.pokemonBox;
    screen.selectedItemIndex = count == 0 ? 0 : std::min(oldSlot, count - 1);
    legacy.pokemonSlot = screen.selectedItemIndex;
    legacy.active = false;
    legacy.review = false;
    legacy.pokemonActions = false;
    legacy.pokemonFields = false;
    confirm.active = false;
    screen.controllerNavigation.reset();
    return true;
}

bool handlePassiveViewTouch(TrainerViewScreen& screen, const TouchInput& touch) {
    if (!parityPassiveDetailsTarget(screen)) return false;
    if (!touch.justTouchedDown() && !touch.justTapped() &&
        !(touch.justReleased() && touch.dragged())) return false;

    auto& unified = unifiedStateFor(screen);
    unified.crystal = screen.sourceGameId == "crystal_gbc";
    const SharedPokemonShell::Geometry geometry(1280, 720, kNavBarH);
    const int contentY = geometry.y, contentH = geometry.h;
    const int leftX = geometry.leftX, leftW = geometry.leftW;
    const int midX = geometry.midX, midW = geometry.midW;
    const int rightX = geometry.rightX, rightW = geometry.rightW;

    auto inside = [](int px, int py, int x, int y, int w, int h) noexcept {
        return px >= x && px < x + w && py >= y && py < y + h;
    };
    auto focusPoint = [&](int px, int py) {
        const std::size_t detailCount = unified.crystal ? 13u : 9u;
        const auto window = Unified::scrollWindow(detailCount, 8, unified.detailsFocus);
        for (std::size_t i = 0; i < window.count; ++i) {
            const int yy = contentY + 224 + static_cast<int>(i) * 38;
            if (!inside(px, py, leftX + 8, yy + 1, leftW - 36, 34)) continue;
            unified.focus = {Unified::Panel::Details, static_cast<uint8_t>(window.first + i), 0};
            unified.detailsFocus = Unified::detailsScrollFocus(unified.focus, unified.detailsFocus);
            return true;
        }

        for (int row = 0; row < 5; ++row) {
            const int yy = contentY + 70 + row * 34;
            for (int col = 0; col < 2; ++col) {
                Unified::Focus candidate{Unified::Panel::Values,
                                         static_cast<uint8_t>(row),
                                         static_cast<uint8_t>(col)};
                const auto cell = Unified::cellFocus(candidate);
                if (!inside(px, py, midX + cell.x, yy - 5, cell.width, 28)) continue;
                unified.focus = candidate;
                return true;
            }
        }

        const std::array<int, 2> capabilityY{contentY + 366, contentY + 408};
        for (int i = 0; i < 2; ++i) {
            if (!inside(px, py, midX + 8, capabilityY[static_cast<std::size_t>(i)] - 5,
                        midW - 16, 30)) continue;
            unified.focus = {Unified::Panel::Values, static_cast<uint8_t>(5 + i), 0};
            return true;
        }

        const auto moveCell = Unified::moveRowFocus(rightW);
        for (int slot = 0; slot < 4; ++slot) {
            const int yy = contentY + 62 + slot * 32;
            if (!inside(px, py, rightX + moveCell.x, yy - 5, moveCell.width, 28)) continue;
            unified.focus = {Unified::Panel::Moves, static_cast<uint8_t>(slot), 0};
            return true;
        }
        return false;
    };

    if (touch.justReleased() && touch.dragged()) {
        const int viewportY = contentY + 216;
        const int viewportBottom = contentY + contentH - 34;
        if (!inside(touch.startX(), touch.startY(), leftX + 8, viewportY,
                    leftW - 16, std::max(0, viewportBottom - viewportY))) return false;
        const int detailCount = unified.crystal ? 13 : 9;
        const int distance = touch.deltaY() < 0 ? -touch.deltaY() : touch.deltaY();
        const int steps = std::max(1, distance / 38);
        const int current = unified.focus.panel == Unified::Panel::Details
            ? static_cast<int>(unified.focus.row) : unified.detailsFocus;
        const int next = std::clamp(current + (touch.deltaY() < 0 ? steps : -steps),
                                    0, detailCount - 1);
        unified.focus = {Unified::Panel::Details, static_cast<uint8_t>(next), 0};
        unified.detailsFocus = Unified::detailsScrollFocus(unified.focus, unified.detailsFocus);
        return true;
    }

    if (touch.justTouchedDown()) return focusPoint(touch.x(), touch.y());
    if (touch.justTapped()) return focusPoint(touch.startX(), touch.startY());
    return false;
}

} // namespace

bool handleReleaseActionInput(TrainerViewScreen& screen, uint64_t down, uint64_t held,
                              int stickX, int stickY, const TouchInput& touch) {
    if (!isGSC(screen) || stateFor(screen).mode != Mode::None) return false;
    if (handlePassiveViewTouch(screen, touch)) return true;
    auto& legacy = editorOverlayState(screen);
    if (!legacy.active || legacy.review || !legacy.pokemonActions) return false;
    auto* editor = stagedEditor(screen);
    if (!editor) return false;
    auto& confirm = releaseConfirmStateFor(screen);

    if (confirm.active) {
        const int touched = screen.touchedButtonId(touch);
        if (touched == 498) down |= HidNpadButton_B;
        else if (touched == 499) down |= HidNpadButton_A;
        if (down & HidNpadButton_B) {
            confirm.active = false;
            screen.controllerNavigation.reset();
            return true;
        }
        if (down & HidNpadButton_A) return confirmRelease(screen);
        return true;
    }

    std::string error;
    auto pokemon = editor->boxedPokemon(static_cast<std::size_t>(legacy.pokemonBox),
                                        static_cast<std::size_t>(legacy.pokemonSlot), error);
    const bool occupied = pokemon.has_value();
    const auto actions = PokeBank::UIModel::SharedPokemonEditor::actionsForSlot(
        occupied, releaseActionCapabilities());
    if (actions.count == 0) return true;

    if (touch.justTouchedDown()) {
        for (const auto& hit : screen.touchButtons) {
            if (hit.id < 500 || hit.id >= 500 + static_cast<int>(actions.count)) continue;
            if (touch.x() >= hit.x && touch.x() < hit.x + hit.w &&
                touch.y() >= hit.y && touch.y() < hit.y + hit.h) {
                legacy.pokemonRow = hit.id - 500;
                break;
            }
        }
    }
    const int touchedAction = screen.touchedButtonId(touch);
    if (touchedAction >= 500 && touchedAction < 500 + static_cast<int>(actions.count)) {
        legacy.pokemonRow = touchedAction - 500;
        down |= HidNpadButton_A;
    }

    const uint64_t nav = screen.controllerNavigation.apply(
        down, held, stickX, stickY,
        HidNpadButton_Up, HidNpadButton_Down, HidNpadButton_Left, HidNpadButton_Right);
    if (nav & HidNpadButton_Up)
        legacy.pokemonRow = (legacy.pokemonRow - 1 + static_cast<int>(actions.count)) % static_cast<int>(actions.count);
    if (nav & HidNpadButton_Down)
        legacy.pokemonRow = (legacy.pokemonRow + 1) % static_cast<int>(actions.count);
    legacy.pokemonRow = std::clamp(legacy.pokemonRow, 0, static_cast<int>(actions.count) - 1);

    if (down & HidNpadButton_B) {
        legacy.active = false;
        legacy.pokemonActions = false;
        screen.controllerNavigation.reset();
        return true;
    }
    if (!(down & HidNpadButton_A)) return true;

    auto& editState = stateFor(screen);
    using Action = PokeBank::UIModel::SharedPokemonEditor::Action;
    switch (actions[static_cast<std::size_t>(legacy.pokemonRow)]) {
        case Action::View:
            if (!pokemon) return true;
            editState.box = legacy.pokemonBox;
            editState.slot = legacy.pokemonSlot;
            editState.begin(*pokemon, PokeBank::UIModel::Gen2PokemonEditor::SessionMode::View);
            dismissActionSheetForWorkspace(screen);
            screen.controllerNavigation.reset();
            return true;
        case Action::Edit:
            if (beginEdit(screen, legacy.pokemonBox, legacy.pokemonSlot))
                dismissActionSheetForWorkspace(screen);
            return true;
        case Action::Clone: {
            if (!pokemon) return true;
            auto& shared = sharedSurfaceStateFor(screen);
            shared.cloneActive = true;
            shared.cloneBox = legacy.pokemonBox;
            screen.controllerNavigation.reset();
            return true;
        }
        case Action::Remove:
            if (!pokemon) return true;
            confirm.active = true;
            screen.controllerNavigation.reset();
            return true;
        case Action::LegalityProvenance: {
            auto& shared = sharedSurfaceStateFor(screen);
            shared.provenanceActive = true;
            legacy.pokemonActions = false;
            screen.controllerNavigation.reset();
            return true;
        }
        case Action::Add:
            if (beginCreate(screen, legacy.pokemonBox))
                dismissActionSheetForWorkspace(screen);
            return true;
        case Action::Review:
            legacy.review = true;
            legacy.reviewRow = 0;
            screen.controllerNavigation.reset();
            return true;
        case Action::Close:
            legacy.active = false;
            legacy.pokemonActions = false;
            screen.controllerNavigation.reset();
            return true;
        case Action::None:
            return true;
    }
    return true;
}

bool drawReleaseActionSurface(TrainerViewScreen& screen, PKSEFramebuffer& fb) {
    if (!isGSC(screen) || stateFor(screen).mode != Mode::None) return false;
    auto& legacy = editorOverlayState(screen);
    if (!legacy.active || legacy.review || !legacy.pokemonActions) return false;
    auto* editor = stagedEditor(screen);
    if (!editor) return false;
    auto& confirm = releaseConfirmStateFor(screen);

    // Draw the actual box browser once before the compact action-sheet dimmer.
    // The legacy GSC overlay would draw a second popup here, so use its base page.
    screen.drawLegacyBase(fb);
    screen.touchButtons.clear();
    fb.drawFilledRect(0, 0, fb.getWidth(), fb.getHeight() - 48, Color(0, 0, 0, 105));
    if (confirm.active) {
        constexpr int w = 650, h = 240;
        constexpr int x = (1280 - w) / 2, y = (672 - h) / 2;
        drawPanelSurface(fb, x, y, w, h, false, 18);
        // Quiet modal chrome: focus belongs to actionable controls, never the whole dialog.
        // drawPanelSurface() already supplies the neutral theme Divider border.
        fb.drawText(x + 24, y + 18, "Release Pokemon?", Colors::Text, TextStyle::Heading);
        fb.drawText(x + 28, y + 86, "Release compacts the Generation II staged box only.", Colors::Text);
        fb.drawText(x + 28, y + 126, "Original RetroArch .srm remains immutable and is never written.", Colors::Accent);
        const int bw = 190, bh = TouchTargetMin, by = y + h - bh - 18;
        const int confirmX = x + w - bw - 22, cancelX = confirmX - bw - 16;
        drawGlyphButton(fb, cancelX, by, bw, bh, "B", "Cancel");
        screen.touchButtons.push_back({498, cancelX, by, bw, bh});
        drawGlyphButton(fb, confirmX, by, bw, bh, "A", "Release");
        screen.touchButtons.push_back({499, confirmX, by, bw, bh});
        drawNavBar(fb, {{"A", "Confirm Release"}, {"B", "Cancel"}});
        return true;
    }

    std::string error;
    const auto pokemon = editor->boxedPokemon(static_cast<std::size_t>(legacy.pokemonBox),
                                               static_cast<std::size_t>(legacy.pokemonSlot), error);
    const auto actions = PokeBank::UIModel::SharedPokemonEditor::actionsForSlot(
        static_cast<bool>(pokemon), releaseActionCapabilities());
    legacy.pokemonRow = std::clamp(legacy.pokemonRow, 0, std::max(0, static_cast<int>(actions.count) - 1));
    const int w = pokemon ? 650 : 560;
    const int h = pokemon ? 500 : 350;
    const int x = (1280 - w) / 2;
    const int y = (672 - h) / 2;
    const auto geometry = PokeBank::UIModel::SharedPokemonEditor::actionMenuGeometry();
    drawPanelSurface(fb, x, y, w, h, false, 18);
    // Keep the action-sheet frame neutral. The selected action row below owns FocusBorder.
    fb.drawText(x + 24, y + 18, pokemon ? "Pokemon Actions" : "Empty Slot", Colors::Text, TextStyle::Heading);
    fb.drawText(x + 24, y + 52,
        "Box " + std::to_string(legacy.pokemonBox + 1) + " / Slot " +
        std::to_string(legacy.pokemonSlot + 1) + "  •  staged copy only",
        Colors::TextDim, TextStyle::Caption);
    int rowY = y + geometry.rowStart;
    for (std::size_t i = 0; i < actions.count; ++i) {
        const bool selected = static_cast<int>(i) == legacy.pokemonRow;
        if (selected) fb.drawSelectionHighlight(x + 22, rowY, w - 44, geometry.rowHeight);
        fb.drawText(x + 38, rowY + 10, releaseActionLabel(actions[i]),
                    selected ? Colors::Text : Colors::TextDim);
        screen.touchButtons.push_back({500 + static_cast<int>(i), x + 22, rowY, w - 44, geometry.rowHeight});
        rowY += geometry.rowStep;
    }
    fb.drawText(x + 24, y + h - 38, "Original source is immutable", Colors::Accent, TextStyle::Caption);
    drawNavBar(fb, {{"D-pad/Stick", "Navigate"}, {"A", "Select"}, {"B", "Back"}});
    return true;
}

} // namespace UI::Gen2PokemonEditor