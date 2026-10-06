from pathlib import Path

def replace_once(path, old, new):
    p = Path(path)
    s = p.read_text()
    n = s.count(old)
    if n != 1:
        raise SystemExit(f"{path}: expected exactly one anchor, got {n}: {old[:120]!r}")
    p.write_text(s.replace(old, new, 1))

# Gen I UX2: publish direct hit geometry for visible rows.
path = "src/UI/Gen1PokemonEditorOverlayUXCleanup2.inc"
replace_once(path,
'''    int rowY = y + 86;
    for (std::size_t i = 0; i < actions.count; ++i) {
        const int idx = static_cast<int>(actions[i]);
        const bool selected = static_cast<int>(i) == state.row;
        if (selected) fb.drawSelectionHighlight(x + 22, rowY, w - 44, 42);
        fb.drawText(x + 38, rowY + 10, names[idx], selected ? Colors::Text : Colors::TextDim);
        rowY += 46;
    }
''',
'''    int rowY = y + 86;
    for (std::size_t i = 0; i < actions.count; ++i) {
        const int idx = static_cast<int>(actions[i]);
        const bool selected = static_cast<int>(i) == state.row;
        if (selected) fb.drawSelectionHighlight(x + 22, rowY, w - 44, 42);
        fb.drawText(x + 38, rowY + 10, names[idx], selected ? Colors::Text : Colors::TextDim);
        screen.touchButtons.push_back({1000 + static_cast<int>(i), x + 22, rowY, w - 44, 42});
        rowY += 46;
    }
''')
replace_once(path,
'''    int rowY = y + 88;
    for (int i = 0; i < static_cast<int>(rows.size()); ++i) {
        ux2DrawLogicalRow(fb, x + 22, rowY, w - 44, rows[static_cast<size_t>(i)].first,
                          rows[static_cast<size_t>(i)].second, i == state.subRow);
        rowY += 48;
    }
''',
'''    int rowY = y + 88;
    for (int i = 0; i < static_cast<int>(rows.size()); ++i) {
        ux2DrawLogicalRow(fb, x + 22, rowY, w - 44, rows[static_cast<size_t>(i)].first,
                          rows[static_cast<size_t>(i)].second, i == state.subRow);
        screen.touchButtons.push_back({1100 + i, x + 22, rowY, w - 44, 44});
        rowY += 48;
    }
''')
replace_once(path,
'''        if (selected) fb.drawSelectionHighlight(cx, cy, cellW - 8, cellH - 8);
        fb.drawRoundedRect(cx, cy, cellW - 8, cellH - 8, 8, i == writable ? Colors::Accent : Colors::Divider, 1);
        fb.drawText(cx + 8, cy + 8, (i < 9 ? "0" : "") + std::to_string(i + 1), Colors::TextDim, TextStyle::Caption);
''',
'''        if (selected) fb.drawSelectionHighlight(cx, cy, cellW - 8, cellH - 8);
        fb.drawRoundedRect(cx, cy, cellW - 8, cellH - 8, 8, i == writable ? Colors::Accent : Colors::Divider, 1);
        screen.touchButtons.push_back({1200 + i, cx, cy, cellW - 8, cellH - 8});
        fb.drawText(cx + 8, cy + 8, (i < 9 ? "0" : "") + std::to_string(i + 1), Colors::TextDim, TextStyle::Caption);
''')
replace_once(path,
'''        for (int i = 0; i < visible && start + i < static_cast<int>(lines.size()); ++i)
            ux2DrawLogicalRow(fb, x + 24, y + 76 + i * 48, w - 48, std::to_string(start + i + 1),
                              lines[static_cast<size_t>(start + i)], start + i == state.reviewRow);
        fb.clearClip();
''',
'''        for (int i = 0; i < visible && start + i < static_cast<int>(lines.size()); ++i) {
            const int row = start + i;
            const int rowY = y + 76 + i * 48;
            ux2DrawLogicalRow(fb, x + 24, rowY, w - 48, std::to_string(row + 1),
                              lines[static_cast<size_t>(row)], row == state.reviewRow);
            screen.touchButtons.push_back({1300 + row, x + 24, rowY, w - 48, 44});
        }
        fb.clearClip();
''')

# Gen I UX3 direct content touch + touch nav bar.
path = "src/UI/Gen1PokemonEditorOverlayUXCleanup3.inc"
replace_once(path,
'''        ux2DrawLogicalRow(fb, centerX + 10, listY + i * rowH, centerW - 20,
                          ux3MainRowLabel(field), ux3MainRowValue(screen, field, add, pokemon ? &*pokemon : nullptr),
                          field == state.row);
''',
'''        const int rowY = listY + i * rowH;
        ux2DrawLogicalRow(fb, centerX + 10, rowY, centerW - 20,
                          ux3MainRowLabel(field), ux3MainRowValue(screen, field, add, pokemon ? &*pokemon : nullptr),
                          field == state.row);
        screen.touchButtons.push_back({2000 + field, centerX + 10, rowY, centerW - 20, rowH - 2});
''')
replace_once(path,
'''        SharedSpeciesPicker::drawContent(
            fb, modal.x, modal.y, state.pickerValue, 151, ux3.pickerPreviewShiny,
            [](uint16_t species) { return UX::speciesPickerRow(species, Names::getSpeciesName(species)); },
            [](uint16_t species) { return speciesCompact(species); });
        return;
''',
'''        SharedSpeciesPicker::drawContent(
            fb, modal.x, modal.y, state.pickerValue, 151, ux3.pickerPreviewShiny,
            [](uint16_t species) { return UX::speciesPickerRow(species, Names::getSpeciesName(species)); },
            [](uint16_t species) { return speciesCompact(species); });
        constexpr int visibleSpecies = 9;
        const int speciesStart = std::clamp(state.pickerValue - visibleSpecies / 2, 1, 151 - visibleSpecies + 1);
        for (int i = 0; i < visibleSpecies; ++i) {
            const int species = speciesStart + i;
            screen.touchButtons.push_back({3000 + species, modal.x + 20, modal.y + 84 + i * 43, 560, 39});
        }
        return;
''')
replace_once(path,
'''        const bool selected = idx == selectedIndex;
        if (selected) fb.drawSelectionHighlight(x + 24, rowY, w - 48, 38);
        const auto text = MoveUI::rowLabel(
''',
'''        const bool selected = idx == selectedIndex;
        if (selected) fb.drawSelectionHighlight(x + 24, rowY, w - 48, 38);
        screen.touchButtons.push_back({4000 + idx, x + 24, rowY, w - 48, 38});
        const auto text = MoveUI::rowLabel(
''')
replace_once(path,
'''        const bool selected = i == state.cloneSlot;
        if (selected) fb.drawSelectionHighlight(cx, cy, cellW - 8, cellH - 8);
        fb.drawRoundedRect(cx, cy, cellW - 8, cellH - 8, 8, i == writable ? Colors::Accent : Colors::Divider, 1);
''',
'''        const bool selected = i == state.cloneSlot;
        if (selected) fb.drawSelectionHighlight(cx, cy, cellW - 8, cellH - 8);
        fb.drawRoundedRect(cx, cy, cellW - 8, cellH - 8, 8, i == writable ? Colors::Accent : Colors::Divider, 1);
        screen.touchButtons.push_back({1200 + i, cx, cy, cellW - 8, cellH - 8});
''')
replace_once(path,
'''    for (int i = 0; i < visible && start + i < static_cast<int>(lines.size()); ++i)
        ux2DrawLogicalRow(fb, x + 24, y + 70 + i * 48, w - 48, std::to_string(start + i + 1),
                          lines[static_cast<size_t>(start + i)], start + i == state.reviewRow);
    fb.clearClip();
''',
'''    for (int i = 0; i < visible && start + i < static_cast<int>(lines.size()); ++i) {
        const int row = start + i;
        const int rowY = y + 70 + i * 48;
        ux2DrawLogicalRow(fb, x + 24, rowY, w - 48, std::to_string(row + 1),
                          lines[static_cast<size_t>(row)], row == state.reviewRow);
        screen.touchButtons.push_back({1300 + row, x + 24, rowY, w - 48, 44});
    }
    fb.clearClip();
''')

anchor = '''void ux3DialogGeometry(TrainerViewScreen& screen, int& x, int& y, int& w, int& h) {
'''
insert = '''void ux3DrawTouchNavBar(PKSEFramebuffer& fb, const UX2State& state) {
    if (state.picker == UX2Picker::Species) {
        drawNavBar(fb, {{"D-pad/Stick", "Navigate"}, {"L/R", "Page"}, {"Y", "Shiny"}, {"A", "Select"}, {"B", "Cancel"}});
        return;
    }
    if (state.picker == UX2Picker::Move) {
        drawNavBar(fb, {{"D-pad/Stick", "Navigate"}, {"L/R", "Page"}, {"A", "Select"}, {"B", "Cancel"}});
        return;
    }
    switch (state.mode) {
        case UX2Mode::View:
        case UX2Mode::Provenance: drawNavBar(fb, {{"B", "Back"}}); return;
        case UX2Mode::Edit:
            drawNavBar(fb, {{"D-pad/Stick", "Navigate"}, {"L/R", "Panel"}, {"A", "Edit"}, {"Y", "Random DVs"}, {"B", "Exit"}}); return;
        case UX2Mode::AddDraft:
            drawNavBar(fb, {{"D-pad/Stick", "Navigate"}, {"L/R", "Panel"}, {"A", "Edit"}, {"Y", "Random DVs"}, {"X", "Add"}, {"B", "Exit"}}); return;
        case UX2Mode::Actions:
            drawNavBar(fb, {{"Up/Down", "Navigate"}, {"A", "Select"}, {"B", "Back"}}); return;
        case UX2Mode::LevelExpEditor:
        case UX2Mode::MoveEditor:
        case UX2Mode::DVEditor:
        case UX2Mode::StatExpEditor:
            drawNavBar(fb, {{"Up/Down", "Navigate"}, {"A", "Edit / Apply"}, {"B", "Cancel"}}); return;
        case UX2Mode::CloneDestination:
            drawNavBar(fb, {{"D-pad/Stick", "Slot"}, {"L/R", "Box"}, {"A", "Stage Clone"}, {"B", "Cancel"}}); return;
        case UX2Mode::RemoveConfirm:
            drawNavBar(fb, {{"A", "Confirm Remove"}, {"B", "Cancel"}}); return;
        case UX2Mode::Review:
            drawNavBar(fb, {{"Up/Down", "Browse"}, {"A", "Export Copy"}, {"Y", "Discard Staged"}, {"B", "Back"}}); return;
        case UX2Mode::Closed: return;
    }
}

uint64_t ux3DirectTouchInput(TrainerViewScreen& screen, uint64_t down, const TouchInput& touch) {
    auto& state = ux2StateFor(screen);
    const int pressed = touchedButtonDownId(touch);
    const int tapped = touchedButtonId(touch);
    auto selectedId = [&](int base, int limit, int id) -> int {
        return id >= base && id < base + limit ? id - base : -1;
    };

    if (state.picker == UX2Picker::Species) {
        int species = selectedId(3000, 152, pressed);
        if (species >= 1) state.pickerValue = species;
        species = selectedId(3000, 152, tapped);
        if (species >= 1) { state.pickerValue = species; down |= HidNpadButton_A; }
        if (touch.justReleased() && touch.dragged()) {
            const int steps = std::max(1, std::abs(touch.deltaY()) / 43);
            if (touch.deltaY() < 0) state.pickerValue = std::min(151, state.pickerValue + steps);
            else if (touch.deltaY() > 0) state.pickerValue = std::max(1, state.pickerValue - steps);
        }
        return down;
    }

    if (state.picker == UX2Picker::Move) {
        const auto choices = ux3MoveChoices(screen);
        int index = selectedId(4000, static_cast<int>(choices.size()), pressed);
        if (index >= 0) state.pickerValue = choices[static_cast<size_t>(index)];
        index = selectedId(4000, static_cast<int>(choices.size()), tapped);
        if (index >= 0) { state.pickerValue = choices[static_cast<size_t>(index)]; down |= HidNpadButton_A; }
        if (touch.justReleased() && touch.dragged() && !choices.empty()) {
            int current = 0;
            for (int i = 0; i < static_cast<int>(choices.size()); ++i)
                if (choices[static_cast<size_t>(i)] == state.pickerValue) { current = i; break; }
            const int steps = std::max(1, std::abs(touch.deltaY()) / 42);
            if (touch.deltaY() < 0) current = std::min(static_cast<int>(choices.size()) - 1, current + steps);
            else if (touch.deltaY() > 0) current = std::max(0, current - steps);
            state.pickerValue = choices[static_cast<size_t>(current)];
        }
        return down;
    }

    if (state.mode == UX2Mode::Actions) {
        std::string error;
        const auto pokemon = ux2SelectedPokemon(screen, error);
        const auto actions = UX::actionsForSlot(static_cast<bool>(pokemon), hasPending(screen));
        int row = selectedId(1000, static_cast<int>(actions.count), pressed);
        if (row >= 0) state.row = row;
        row = selectedId(1000, static_cast<int>(actions.count), tapped);
        if (row >= 0) { state.row = row; down |= HidNpadButton_A; }
    } else if (state.mode == UX2Mode::LevelExpEditor || state.mode == UX2Mode::MoveEditor ||
               state.mode == UX2Mode::DVEditor || state.mode == UX2Mode::StatExpEditor) {
        int rowCount = state.mode == UX2Mode::LevelExpEditor ? 4 :
                       state.mode == UX2Mode::MoveEditor ? 5 : 7;
        int row = selectedId(1100, rowCount, pressed);
        if (row >= 0) state.subRow = row;
        row = selectedId(1100, rowCount, tapped);
        if (row >= 0) { state.subRow = row; down |= HidNpadButton_A; }
    } else if (state.mode == UX2Mode::CloneDestination) {
        auto* e = editor(screen);
        const int slots = e ? ux2SlotCount(*e, state.cloneBox) : 0;
        int slot = selectedId(1200, slots, pressed);
        if (slot >= 0) state.cloneSlot = slot;
        slot = selectedId(1200, slots, tapped);
        if (slot >= 0) { state.cloneSlot = slot; down |= HidNpadButton_A; }
    } else if (state.mode == UX2Mode::Review) {
        const auto lines = ux3PendingLines(screen);
        int row = selectedId(1300, static_cast<int>(lines.size()), pressed);
        if (row >= 0) state.reviewRow = row;
        row = selectedId(1300, static_cast<int>(lines.size()), tapped);
        if (row >= 0) state.reviewRow = row;
        if (touch.justReleased() && touch.dragged() && !lines.empty()) {
            const int steps = std::max(1, std::abs(touch.deltaY()) / 48);
            if (touch.deltaY() < 0) state.reviewRow = std::min(static_cast<int>(lines.size()) - 1, state.reviewRow + steps);
            else if (touch.deltaY() > 0) state.reviewRow = std::max(0, state.reviewRow - steps);
        }
    }
    return down;
}

'''
replace_once(path, anchor, insert + anchor)
replace_once(path,
'''bool handleInputUX(TrainerViewScreen& screen, uint64_t down) {
    return handleInputUX(screen, down, 0, 0, 0);
}
''',
'''bool handleInputUX(TrainerViewScreen& screen, uint64_t down, uint64_t held, int stickX, int stickY,
                   const TouchInput& touch) {
    return handleInputUX(screen, ux3DirectTouchInput(screen, down, touch), held, stickX, stickY);
}

bool handleInputUX(TrainerViewScreen& screen, uint64_t down) {
    return handleInputUX(screen, down, 0, 0, 0);
}
''')
replace_once(path,
'''    drawFooter(fb, ux3FooterText(screen, state));
}
''',
'''    ux3DrawTouchNavBar(fb, state);
}
''')

# Foundation main workspace touch dispatcher + confirmation hit targets.
path = "src/UI/Gen1PokemonEditorOverlayFoundation.inc"
replace_once(path,
'''    drawGlyphButton(fb, x + 24, cby, cbw, cbh, "B", "Back", Colors::PanelAlt);
    drawGlyphButton(fb, x + (w - cbw) / 2, cby, cbw, cbh, "Y", "Discard", Colors::PanelAlt);
    drawGlyphButton(fb, x + w - 24 - cbw, cby, cbw, cbh, "A",
                    create ? "Keep" : "Save", Colors::PanelAlt);
''',
'''    drawGlyphButton(fb, x + 24, cby, cbw, cbh, "B", "Back", Colors::PanelAlt);
    drawGlyphButton(fb, x + (w - cbw) / 2, cby, cbw, cbh, "Y", "Discard", Colors::PanelAlt);
    drawGlyphButton(fb, x + w - 24 - cbw, cby, cbw, cbh, "A",
                    create ? "Keep" : "Save", Colors::PanelAlt);
    screen.touchButtons.push_back({5900, x + 24, cby, cbw, cbh});
    screen.touchButtons.push_back({5901, x + (w - cbw) / 2, cby, cbw, cbh});
    screen.touchButtons.push_back({5902, x + w - 24 - cbw, cby, cbw, cbh});
''')
anchor = '''bool handleInputUX(TrainerViewScreen& screen, uint64_t down, uint64_t held, int stickX, int stickY) {
'''
insert = '''uint64_t foundationDirectTouchInput(TrainerViewScreen& screen, uint64_t down, const TouchInput& touch) {
    auto& foundation = foundationStateFor(screen);
    const int pressed = touchedButtonDownId(touch);
    const int tapped = touchedButtonId(touch);
    auto focusId = [&](int id, bool activate) {
        if (id >= 5000 && id < 5006) {
            foundation.focus = {Foundation::Panel::Identity, static_cast<uint8_t>(id - 5000), 0};
            if (activate) down |= HidNpadButton_A;
            return true;
        }
        if (id >= 5100 && id < 5150) {
            const int encoded = id - 5100;
            const int row = encoded / 10, col = encoded % 10;
            if (row >= 0 && row < 5 && col >= 0 && col < 3) {
                foundation.focus = {Foundation::Panel::Values, static_cast<uint8_t>(row), static_cast<uint8_t>(col)};
                if (activate) down |= HidNpadButton_A;
                return true;
            }
        }
        if (id == 5190) {
            foundation.focus = {Foundation::Panel::Values, static_cast<uint8_t>(Foundation::ValueRow::Shiny), 0};
            if (activate) down |= HidNpadButton_A;
            return true;
        }
        if (id >= 5200 && id < 5204) {
            foundation.focus = {Foundation::Panel::Moves, static_cast<uint8_t>(id - 5200), 0};
            if (activate) down |= HidNpadButton_A;
            return true;
        }
        return false;
    };
    if (foundation.editExitConfirm) {
        if (tapped == 5900) down |= HidNpadButton_B;
        else if (tapped == 5901) down |= HidNpadButton_Y;
        else if (tapped == 5902) down |= HidNpadButton_A;
        return down;
    }
    focusId(pressed, false);
    focusId(tapped, true);
    return down;
}

'''
replace_once(path, anchor, insert + anchor)
replace_once(path,
'''bool handleInputUX(TrainerViewScreen& screen, uint64_t down, uint64_t held, int stickX, int stickY) {
    if (!isGen1SourceUX(screen)) return false;
    auto& state = ux2StateFor(screen);
    if (!foundationMainMode(state.mode))
        return handleInputUXCleanup3(screen, down, held, stickX, stickY);

    auto& foundation = foundationStateFor(screen);
    const uint64_t navigated = foundation.navigation.apply(
        down, held, stickX, stickY,
        HidNpadButton_Up, HidNpadButton_Down, HidNpadButton_Left, HidNpadButton_Right);
    foundationHandleMainInput(screen, navigated);
    return true;
}

bool handleInputUX(TrainerViewScreen& screen, uint64_t down) {
''',
'''bool handleInputUX(TrainerViewScreen& screen, uint64_t down, uint64_t held, int stickX, int stickY) {
    if (!isGen1SourceUX(screen)) return false;
    auto& state = ux2StateFor(screen);
    if (!foundationMainMode(state.mode))
        return handleInputUXCleanup3(screen, down, held, stickX, stickY);

    auto& foundation = foundationStateFor(screen);
    const uint64_t navigated = foundation.navigation.apply(
        down, held, stickX, stickY,
        HidNpadButton_Up, HidNpadButton_Down, HidNpadButton_Left, HidNpadButton_Right);
    foundationHandleMainInput(screen, navigated);
    return true;
}

bool handleInputUX(TrainerViewScreen& screen, uint64_t down, uint64_t held, int stickX, int stickY,
                   const TouchInput& touch) {
    if (!isGen1SourceUX(screen)) return false;
    auto& state = ux2StateFor(screen);
    if (!foundationMainMode(state.mode))
        return handleInputUXCleanup3(screen, down, held, stickX, stickY, touch);

    auto& foundation = foundationStateFor(screen);
    const uint64_t touched = foundationDirectTouchInput(screen, down, touch);
    const uint64_t navigated = foundation.navigation.apply(
        touched, held, stickX, stickY,
        HidNpadButton_Up, HidNpadButton_Down, HidNpadButton_Left, HidNpadButton_Right);
    foundationHandleMainInput(screen, navigated);
    return true;
}

bool handleInputUX(TrainerViewScreen& screen, uint64_t down) {
''')

# Final fullscreen renderer owns the actual visible geometry.
path = "src/UI/Gen1PokemonEditorFoundationHardwareFix.inc"
replace_once(path,
'''        drawClassicFocus(fb, leftX + 8, yy, leftW - 16, 38, selected);
        fb.drawText(leftX + 18, yy + 7, leftLabels[static_cast<size_t>(i)],
''',
'''        drawClassicFocus(fb, leftX + 8, yy, leftW - 16, 38, selected);
        if (editable) screen.touchButtons.push_back({5000 + i, leftX + 8, yy, leftW - 16, 38});
        fb.drawText(leftX + 18, yy + 7, leftLabels[static_cast<size_t>(i)],
''')
replace_once(path,
'''            drawClassicFocus(fb, cellX, rowY, cellW - 4, 34, selected);
            std::string value;
''',
'''            drawClassicFocus(fb, cellX, rowY, cellW - 4, 34, selected);
            if (editable && Foundation::valueCellEditable(static_cast<Foundation::ValueRow>(r),
                                                           static_cast<Foundation::ValueColumn>(c)))
                screen.touchButtons.push_back({5100 + r * 10 + c, cellX, rowY, cellW - 4, 34});
            std::string value;
''')
replace_once(path,
'''    drawClassicFocus(fb, gridX, gridY + 276, midW - 20, 38, shinySelected);
    fb.drawText(gridX + 12, gridY + 286, "Shiny",
''',
'''    drawClassicFocus(fb, gridX, gridY + 276, midW - 20, 38, shinySelected);
    if (editable) screen.touchButtons.push_back({5190, gridX, gridY + 276, midW - 20, 38});
    fb.drawText(gridX + 12, gridY + 286, "Shiny",
''')
replace_once(path,
'''        const bool selected = foundation.focus.panel == Foundation::Panel::Moves && foundation.focus.row == i;
        if (selected) fb.drawSelectionHighlight(rightX + 10, rowY, rightW - 20, 38);
''',
'''        const bool selected = foundation.focus.panel == Foundation::Panel::Moves && foundation.focus.row == i;
        if (selected) fb.drawSelectionHighlight(rightX + 10, rowY, rightW - 20, 38);
        if (editable) screen.touchButtons.push_back({5200 + i, rightX + 10, rowY, rightW - 20, 38});
''')
replace_once(path,
'''    drawFooter(fb, ux3FooterText(screen, state));
}
''',
'''    ux3DrawTouchNavBar(fb, state);
}
''')

# Composite dispatcher passes real TouchInput through Gen I's active layers.
path = "src/UI/TrainerViewScreenCompositeOverlay.cpp"
replace_once(path,
'''[[nodiscard]] bool handleInputUXCleanup3(TrainerViewScreen& screen, uint64_t down, uint64_t held,
                                         int stickX, int stickY);
''',
'''[[nodiscard]] bool handleInputUXCleanup3(TrainerViewScreen& screen, uint64_t down, uint64_t held,
                                         int stickX, int stickY);
[[nodiscard]] bool handleInputUXCleanup3(TrainerViewScreen& screen, uint64_t down, uint64_t held,
                                         int stickX, int stickY, const TouchInput& touch);
''')
replace_once(path,
'''[[nodiscard]] bool handleInputUX(TrainerViewScreen& screen, uint64_t down, uint64_t held,
                                 int stickX, int stickY);
''',
'''[[nodiscard]] bool handleInputUX(TrainerViewScreen& screen, uint64_t down, uint64_t held,
                                 int stickX, int stickY);
[[nodiscard]] bool handleInputUX(TrainerViewScreen& screen, uint64_t down, uint64_t held,
                                 int stickX, int stickY, const TouchInput& touch);
''')
replace_once(path,
'''    if (Gen1PokemonEditor::isGen1SourceUX(*this) && Gen1PokemonEditor::foundationPickerActive(*this)) {
        if (Gen1PokemonEditor::handleInputUXCleanup3(*this, down, held, stick.x, stick.y)) return;
    }
''',
'''    if (Gen1PokemonEditor::isGen1SourceUX(*this) && Gen1PokemonEditor::foundationPickerActive(*this)) {
        if (Gen1PokemonEditor::handleInputUXCleanup3(*this, down, held, stick.x, stick.y, touch)) return;
    }
''')
replace_once(path,
'''    if (Gen1PokemonEditor::handleInputUX(*this, down, held, stick.x, stick.y)) return;
''',
'''    if (Gen1PokemonEditor::handleInputUX(*this, down, held, stick.x, stick.y, touch)) return;
''')

print("Gen I professional direct-touch patch staged")
