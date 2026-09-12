from pathlib import Path

p = Path('src/UI/TrainerViewScreenGSCOverlay.cpp')
s = p.read_text()

s = s.replace('#include "Legacy/GSCReadOnlyTrainer.h"\n', '#include "Legacy/GSCReadOnlyTrainer.h"\n#include "Names/MoveNames.h"\n#include "Names/SpeciesNames.h"\n', 1)
s = s.replace('#include <cstdio>\n', '#include <cstdio>\n#include <array>\n#include <sstream>\n', 1)

old_state = '''struct Gen2EditorOverlayState {
    const TrainerViewScreen* owner = nullptr;
    bool active = false;
    bool review = false;
    int selectedRow = 0;
    std::string lastExportDirectory;
};'''
new_state = '''struct Gen2EditorOverlayState {
    const TrainerViewScreen* owner = nullptr;
    bool active = false;
    bool review = false;
    bool pokemonActions = false;
    bool pokemonFields = false;
    int selectedRow = 0;
    int reviewRow = 0;
    int pokemonBox = 0;
    int pokemonSlot = 0;
    int pokemonRow = 0;
    std::string lastExportDirectory;
};'''
if old_state not in s:
    raise SystemExit('overlay state marker missing')
s = s.replace(old_state, new_state, 1)

old_open = '''    state.active = true;
    state.review = false;
    if (screen.selectedMode == TrainerViewScreen::ViewMode::Items) {'''
new_open = '''    state.active = true;
    state.review = false;
    state.pokemonActions = false;
    state.pokemonFields = false;
    state.reviewRow = 0;
    if (screen.selectedMode == TrainerViewScreen::ViewMode::Items) {'''
if old_open not in s:
    raise SystemExit('open staged editor marker missing')
s = s.replace(old_open, new_open, 1)

insert_before_handle = '''void handleStagedEditorInput(TrainerViewScreen& screen, u64 down) {'''
if insert_before_handle not in s:
    raise SystemExit('handle staged marker missing')

helpers = r'''
std::string gen2ItemDisplay(uint8_t item) {
    if (item == 0) return "None (0)";
    const auto name = PokeVault::Integration::Gen2::gen2ItemName(item);
    return (name.empty() ? std::string("Item") : std::string(name)) + " (" + std::to_string(item) + ")";
}

std::string gen2MoveDisplay(uint8_t move) {
    if (move == 0) return "Empty (0)";
    return std::string(Names::getMoveName(move)) + " (" + std::to_string(move) + ")";
}

std::array<uint8_t, 4> editableDVs(const PokeVault::Integration::Gen2::PokemonRecord& pokemon) {
    return {pokemon.dvs[1], pokemon.dvs[2], pokemon.dvs[3], pokemon.dvs[4]};
}

bool openPokemonActions(TrainerViewScreen& screen) {
    auto* editor = stagedEditor(screen);
    if (!editor) {
        screen.postStatus("Staged Pokemon editor is unavailable; source remains read-only");
        return false;
    }
    if (screen.selectedBoxIndex < 0 || screen.selectedItemIndex < 0 ||
        screen.selectedBoxIndex >= static_cast<int>(editor->metadata().boxCount) ||
        screen.selectedItemIndex >= static_cast<int>(editor->metadata().boxCapacity)) {
        screen.postStatus("Select a Generation II box slot first");
        return false;
    }
    auto& state = editorOverlayState(screen);
    state.active = true;
    state.review = false;
    state.pokemonActions = true;
    state.pokemonFields = false;
    state.pokemonBox = screen.selectedBoxIndex;
    state.pokemonSlot = screen.selectedItemIndex;
    state.pokemonRow = 0;
    state.reviewRow = 0;
    return true;
}

bool stagePokemonFieldEdit(TrainerViewScreen& screen, int row) {
    auto* editor = stagedEditor(screen);
    auto& state = editorOverlayState(screen);
    if (!editor) return false;
    std::string error;
    auto pokemon = editor->boxedPokemon(static_cast<std::size_t>(state.pokemonBox),
                                        static_cast<std::size_t>(state.pokemonSlot), error);
    if (!pokemon) {
        screen.postStatus(error.empty() ? "Selected Gen II box slot is empty" : error);
        return false;
    }

    PokeVault::Integration::Gen2::BoxPokemonEdit edit;
    switch (row) {
        case 0: {
            const auto r = Utils::promptNumber("Gen II Species ID", pokemon->species, 1, 251);
            if (!r.accepted) return false;
            edit.species = static_cast<uint16_t>(r.value);
            break;
        }
        case 1: {
            const auto r = Utils::promptText("Gen II Nickname", "1-10 Gen II characters",
                                             pokemon->nickname, 10);
            if (!r.accepted) return false;
            edit.nickname = r.text;
            break;
        }
        case 2: {
            const auto r = Utils::promptNumber("Gen II Level", pokemon->level, 1, 100);
            if (!r.accepted) return false;
            edit.level = static_cast<uint8_t>(r.value);
            break;
        }
        case 3: {
            const auto r = Utils::promptNumber("Gen II Experience", static_cast<int>(pokemon->experience),
                                               0, 2000000);
            if (!r.accepted) return false;
            edit.experience = static_cast<uint32_t>(r.value);
            break;
        }
        case 4: {
            const auto r = Utils::promptNumber("Gen II Held Item ID (0=None)", pokemon->heldItem, 0, 255);
            if (!r.accepted) return false;
            edit.heldItem = static_cast<uint8_t>(r.value);
            break;
        }
        case 5: case 7: case 9: case 11: {
            const int slot = (row - 5) / 2;
            auto moves = pokemon->moves;
            auto pp = pokemon->pp;
            auto ppUps = pokemon->ppUps;
            const auto r = Utils::promptNumber("Gen II Move ID (0=Empty)", moves[slot], 0, 251);
            if (!r.accepted) return false;
            moves[slot] = static_cast<uint8_t>(r.value);
            ppUps[slot] = 0;
            pp[slot] = moves[slot] == 0 ? 0 : Gen2Editor::gen2MoveBasePP(moves[slot]);
            edit.moves = moves;
            edit.pp = pp;
            edit.ppUps = ppUps;
            break;
        }
        case 6: case 8: case 10: case 12: {
            const int slot = (row - 6) / 2;
            auto pp = pokemon->pp;
            const auto r = Utils::promptNumber("Gen II Move PP", pp[slot], 0, 63);
            if (!r.accepted) return false;
            pp[slot] = static_cast<uint8_t>(r.value);
            edit.pp = pp;
            break;
        }
        case 13: case 14: case 15: case 16: {
            const int index = row - 13;
            auto dvs = editableDVs(*pokemon);
            const auto r = Utils::promptNumber("Gen II DV (0-15)", dvs[index], 0, 15);
            if (!r.accepted) return false;
            dvs[index] = static_cast<uint8_t>(r.value);
            edit.dvs = dvs;
            break;
        }
        case 17: case 18: case 19: case 20: case 21: {
            const int index = row - 17;
            auto statExperience = pokemon->statExperience;
            const auto r = Utils::promptNumber("Gen II Stat Experience (0-65535)",
                                               statExperience[index], 0, 65535);
            if (!r.accepted) return false;
            statExperience[index] = static_cast<uint16_t>(r.value);
            edit.statExperience = statExperience;
            break;
        }
        case 22: {
            const auto r = Utils::promptText("Gen II OT Name", "1-10 Gen II characters",
                                             pokemon->originalTrainer, 10);
            if (!r.accepted) return false;
            edit.otName = r.text;
            break;
        }
        case 23: {
            const auto r = Utils::promptNumber("Gen II Trainer ID", pokemon->trainerId, 0, 65535);
            if (!r.accepted) return false;
            edit.trainerId = static_cast<uint16_t>(r.value);
            break;
        }
        case 24: {
            const auto r = Utils::promptNumber("Gen II Friendship", pokemon->friendship, 0, 255);
            if (!r.accepted) return false;
            edit.friendship = static_cast<uint8_t>(r.value);
            break;
        }
        case 25: {
            const auto r = Utils::promptNumber("Gen II Pokerus byte", pokemon->pokerus, 0, 255);
            if (!r.accepted) return false;
            edit.pokerus = static_cast<uint8_t>(r.value);
            break;
        }
        case 26: {
            const auto r = Utils::promptNumber("Gen II Caught/Met data", pokemon->caughtData, 0, 65535);
            if (!r.accepted) return false;
            edit.caughtData = static_cast<uint16_t>(r.value);
            break;
        }
        default:
            return false;
    }
    if (!editor->stageBoxPokemonEdit(static_cast<std::size_t>(state.pokemonBox),
                                     static_cast<std::size_t>(state.pokemonSlot), edit, error)) {
        screen.postStatus(error.empty() ? "Gen II Pokemon edit was rejected" : error, 300);
        return false;
    }
    screen.postStatus("Pokemon change staged only; RetroArch source .srm unchanged", 240);
    return true;
}

bool stageAddPokemon(TrainerViewScreen& screen, int box) {
    auto* editor = stagedEditor(screen);
    if (!editor) return false;
    const auto species = Utils::promptNumber("Add Gen II Pokemon - Species ID", 25, 1, 251);
    if (!species.accepted) return false;
    const auto level = Utils::promptNumber("Add Gen II Pokemon - Level", 5, 1, 100);
    if (!level.accepted) return false;
    const auto nickname = Utils::promptText("Add Gen II Pokemon - Nickname",
                                            "Blank uses the species name; max 10 Gen II characters", "", 10);
    if (!nickname.accepted) return false;
    const auto held = Utils::promptNumber("Add Gen II Pokemon - Held Item ID", 0, 0, 255);
    if (!held.accepted) return false;

    PokeVault::Integration::Gen2::BoxPokemonCreate create;
    create.species = static_cast<uint16_t>(species.value);
    create.level = static_cast<uint8_t>(level.value);
    create.nickname = nickname.text;
    create.heldItem = static_cast<uint8_t>(held.value);
    for (int i = 0; i < 4; ++i) {
        const auto move = Utils::promptNumber("Add Gen II Pokemon - Move " + std::to_string(i + 1) + " ID",
                                              0, 0, 251);
        if (!move.accepted) return false;
        create.moves[i] = static_cast<uint8_t>(move.value);
    }
    for (int i = 0; i < 4; ++i) {
        const auto dv = Utils::promptNumber("Add Gen II Pokemon - " +
            std::string(i == 0 ? "Attack" : i == 1 ? "Defense" : i == 2 ? "Speed" : "Special") + " DV",
            8, 0, 15);
        if (!dv.accepted) return false;
        create.dvs[i] = static_cast<uint8_t>(dv.value);
    }
    const auto friendship = Utils::promptNumber("Add Gen II Pokemon - Friendship", 70, 0, 255);
    if (!friendship.accepted) return false;
    create.friendship = static_cast<uint8_t>(friendship.value);
    const auto caught = Utils::promptNumber("Add Gen II Pokemon - Caught/Met data", 0, 0, 65535);
    if (!caught.accepted) return false;
    create.caughtData = static_cast<uint16_t>(caught.value);

    std::size_t slot = 0;
    std::string error;
    if (!editor->stageAddBoxPokemon(static_cast<std::size_t>(box), create, slot, error)) {
        screen.postStatus(error.empty() ? "Could not add Gen II Pokemon" : error, 300);
        return false;
    }
    auto& state = editorOverlayState(screen);
    state.pokemonBox = box;
    state.pokemonSlot = static_cast<int>(slot);
    screen.postStatus("Added Pokemon to staged Box " + std::to_string(box + 1) +
                      " Slot " + std::to_string(slot + 1) + "; source unchanged", 300);
    return true;
}

void handlePokemonFieldInput(TrainerViewScreen& screen, u64 down) {
    auto& state = editorOverlayState(screen);
    constexpr int rowCount = 28;
    if (down & HidNpadButton_Up) state.pokemonRow = (state.pokemonRow - 1 + rowCount) % rowCount;
    if (down & HidNpadButton_Down) state.pokemonRow = (state.pokemonRow + 1) % rowCount;
    if (down & HidNpadButton_B) {
        state.pokemonFields = false;
        return;
    }
    if (!(down & HidNpadButton_A)) return;
    if (state.pokemonRow == rowCount - 1) {
        state.pokemonFields = false;
        return;
    }
    stagePokemonFieldEdit(screen, state.pokemonRow);
}

void handlePokemonActionInput(TrainerViewScreen& screen, u64 down) {
    auto& state = editorOverlayState(screen);
    auto* editor = stagedEditor(screen);
    if (!editor) { state = {}; state.owner = &screen; return; }
    std::string error;
    const auto pokemon = editor->boxedPokemon(static_cast<std::size_t>(state.pokemonBox),
                                              static_cast<std::size_t>(state.pokemonSlot), error);
    const bool occupied = pokemon.has_value();
    const int rowCount = occupied ? 6 : 3;
    if (down & HidNpadButton_Up) state.pokemonRow = (state.pokemonRow - 1 + rowCount) % rowCount;
    if (down & HidNpadButton_Down) state.pokemonRow = (state.pokemonRow + 1) % rowCount;
    if (down & HidNpadButton_X) { state.review = true; state.reviewRow = 0; return; }
    if (down & HidNpadButton_B) {
        if (editor->hasPendingChanges()) { state.review = true; state.reviewRow = 0; }
        else { state.active = false; state.pokemonActions = false; }
        return;
    }
    if (!(down & HidNpadButton_A)) return;

    if (!occupied) {
        if (state.pokemonRow == 0) {
            if (stageAddPokemon(screen, state.pokemonBox)) state.pokemonRow = 0;
        } else if (state.pokemonRow == 1) {
            state.review = true; state.reviewRow = 0;
        } else {
            state.active = false; state.pokemonActions = false;
        }
        return;
    }

    switch (state.pokemonRow) {
        case 0:
            state.pokemonFields = true;
            state.pokemonRow = 0;
            break;
        case 1: {
            const auto destination = Utils::promptNumber("Clone to Gen II Box", state.pokemonBox + 1, 1,
                                                         editor->metadata().boxCount);
            if (!destination.accepted) break;
            std::size_t destinationSlot = 0;
            if (!editor->stageCloneBoxPokemon(static_cast<std::size_t>(state.pokemonBox),
                                              static_cast<std::size_t>(state.pokemonSlot),
                                              static_cast<std::size_t>(destination.value - 1),
                                              destinationSlot, error))
                screen.postStatus(error.empty() ? "Clone failed" : error, 300);
            else
                screen.postStatus("Clone staged in Box " + std::to_string(destination.value) +
                                  " Slot " + std::to_string(destinationSlot + 1) + "; source unchanged", 300);
            break;
        }
        case 2: {
            if (!editor->stageBoxPokemonShiny(static_cast<std::size_t>(state.pokemonBox),
                                              static_cast<std::size_t>(state.pokemonSlot),
                                              !pokemon->shiny, error))
                screen.postStatus(error.empty() ? "Shiny toggle failed" : error, 360);
            else
                screen.postStatus(pokemon->shiny ? "Non-shiny DV state staged; source unchanged"
                                                 : "Legal Gen II shiny DV state staged; source unchanged", 300);
            break;
        }
        case 3:
            stageAddPokemon(screen, state.pokemonBox);
            break;
        case 4:
            state.review = true; state.reviewRow = 0;
            break;
        case 5:
            state.active = false; state.pokemonActions = false;
            break;
        default:
            break;
    }
}

'''
s = s.replace(insert_before_handle, helpers + insert_before_handle, 1)

old_handle_start = '''    if (state.review) {
        if (down & HidNpadButton_B) {'''
new_handle_start = '''    if (state.review) {
        const int changeCount = static_cast<int>(editor->pendingChanges().size());
        if (changeCount > 0 && (down & HidNpadButton_Up))
            state.reviewRow = (state.reviewRow - 1 + changeCount) % changeCount;
        if (changeCount > 0 && (down & HidNpadButton_Down))
            state.reviewRow = (state.reviewRow + 1) % changeCount;
        if (down & HidNpadButton_B) {'''
if old_handle_start not in s:
    raise SystemExit('review handler marker missing')
s = s.replace(old_handle_start, new_handle_start, 1)

review_end = '''        return;
    }

    constexpr int rowCount = 7;'''
review_route = '''        return;
    }

    if (state.pokemonFields) {
        handlePokemonFieldInput(screen, down);
        return;
    }
    if (state.pokemonActions) {
        handlePokemonActionInput(screen, down);
        return;
    }

    constexpr int rowCount = 7;'''
if review_end not in s:
    raise SystemExit('post-review route marker missing')
s = s.replace(review_end, review_route, 1)

# Make every entry into review reset its cursor.
s = s.replace('state.review = true;\n        return;', 'state.review = true;\n        state.reviewRow = 0;\n        return;')
s = s.replace('state.review = true;\n            screen.postStatus', 'state.review = true;\n            state.reviewRow = 0;\n            screen.postStatus')
s = s.replace('case 5:\n            state.review = true;', 'case 5:\n            state.review = true;\n            state.reviewRow = 0;')

# Draw dedicated Pokemon action/field surfaces before the original trainer/inventory rows.
draw_marker = '''    if (state.review) {
        int ry = y + 100;'''
if draw_marker not in s:
    raise SystemExit('review draw marker missing')

draw_prefix = r'''    if (state.pokemonFields && !state.review) {
        std::string error;
        auto pokemon = editor->boxedPokemon(static_cast<std::size_t>(state.pokemonBox),
                                            static_cast<std::size_t>(state.pokemonSlot), error);
        fb.drawText(x + 28, y + 20, "Generation II Pokemon Editor", Colors::Text, TextStyle::Heading);
        fb.drawText(x + 28, y + 54,
                    "Staged copy only. DVs / Stat Experience are real Gen II fields. FULL legality is not claimed.",
                    Colors::TextDim, TextStyle::Caption);
        if (!pokemon) {
            fb.drawText(x + 32, y + 110, error.empty() ? "Pokemon is unavailable." : error,
                        Colors::Text, TextStyle::Body);
            return;
        }
        const std::array<std::pair<std::string, std::string>, 28> rows{{
            {"Species", std::string(Names::getSpeciesName(pokemon->species)) + " (" + std::to_string(pokemon->species) + ")"},
            {"Nickname", pokemon->nickname},
            {"Level", std::to_string(pokemon->level)},
            {"Experience", std::to_string(pokemon->experience)},
            {"Held Item", gen2ItemDisplay(pokemon->heldItem)},
            {"Move 1", gen2MoveDisplay(pokemon->moves[0])}, {"Move 1 PP", std::to_string(pokemon->pp[0])},
            {"Move 2", gen2MoveDisplay(pokemon->moves[1])}, {"Move 2 PP", std::to_string(pokemon->pp[1])},
            {"Move 3", gen2MoveDisplay(pokemon->moves[2])}, {"Move 3 PP", std::to_string(pokemon->pp[2])},
            {"Move 4", gen2MoveDisplay(pokemon->moves[3])}, {"Move 4 PP", std::to_string(pokemon->pp[3])},
            {"Attack DV", std::to_string(pokemon->dvs[1])},
            {"Defense DV", std::to_string(pokemon->dvs[2])},
            {"Speed DV", std::to_string(pokemon->dvs[3])},
            {"Special DV", std::to_string(pokemon->dvs[4])},
            {"HP Stat Experience", std::to_string(pokemon->statExperience[0])},
            {"Attack Stat Experience", std::to_string(pokemon->statExperience[1])},
            {"Defense Stat Experience", std::to_string(pokemon->statExperience[2])},
            {"Speed Stat Experience", std::to_string(pokemon->statExperience[3])},
            {"Special Stat Experience", std::to_string(pokemon->statExperience[4])},
            {"OT Name", pokemon->originalTrainer},
            {"Trainer ID", std::to_string(pokemon->trainerId)},
            {"Friendship", std::to_string(pokemon->friendship)},
            {"Pokerus", std::to_string(pokemon->pokerus)},
            {"Caught/Met Data", std::to_string(pokemon->caughtData)},
            {"Back", "HP DV " + std::to_string(pokemon->dvs[0]) +
                     " (derived) / Shiny " + std::string(pokemon->shiny ? "Yes" : "No")},
        }};
        constexpr int visible = 8;
        const int start = std::clamp(state.pokemonRow - visible / 2, 0,
                                     static_cast<int>(rows.size()) - visible);
        int rowY = y + 92;
        for (int i = start; i < start + visible; ++i) {
            drawEditorRow(fb, x + 30, rowY, w - 60, rows[i].first, rows[i].second,
                          i == state.pokemonRow);
            rowY += 57;
        }
        fb.drawText(x + 32, y + h - 40,
                    "A Edit    B Pokemon Actions    HP DV is derived; shiny/gender follow Gen II DVs",
                    Colors::TextDim, TextStyle::Caption);
        return;
    }

    if (state.pokemonActions && !state.review) {
        std::string error;
        const auto pokemon = editor->boxedPokemon(static_cast<std::size_t>(state.pokemonBox),
                                                  static_cast<std::size_t>(state.pokemonSlot), error);
        fb.drawText(x + 28, y + 20, "Generation II Pokemon Actions", Colors::Text, TextStyle::Heading);
        fb.drawText(x + 28, y + 54,
                    "Box " + std::to_string(state.pokemonBox + 1) + " Slot " +
                    std::to_string(state.pokemonSlot + 1) + " — source .srm stays READ ONLY",
                    Colors::TextDim, TextStyle::Caption);
        std::vector<std::pair<std::string, std::string>> rows;
        if (pokemon) {
            rows = {
                {"Edit", pokemon->nickname + " Lv. " + std::to_string(pokemon->level)},
                {"Clone", "Append to an empty box slot"},
                {pokemon->shiny ? "Make Non-Shiny" : "Make Shiny", "Real Gen II DV rules"},
                {"Add Pokemon", "Append to this box"},
                {"Review Pending Changes", std::to_string(editor->pendingChanges().size()) + " change(s)"},
                {"Close", "No source write"},
            };
        } else {
            rows = {
                {"Add Pokemon", "Append to this box"},
                {"Review Pending Changes", std::to_string(editor->pendingChanges().size()) + " change(s)"},
                {"Close", "No source write"},
            };
        }
        int rowY = y + 112;
        for (int i = 0; i < static_cast<int>(rows.size()); ++i) {
            drawEditorRow(fb, x + 30, rowY, w - 60, rows[i].first, rows[i].second,
                          i == state.pokemonRow);
            rowY += 57;
        }
        fb.drawText(x + 32, y + h - 40,
                    "A Select    X Review    B Close/Review    FORMAT VALID != full encounter legality",
                    Colors::TextDim, TextStyle::Caption);
        return;
    }

'''
s = s.replace(draw_marker, draw_prefix + draw_marker, 1)

# Replace pending-change draw loop with a bounded semantic window.
old_review_draw = '''        } else {
            for (const auto& change : editor->pendingChanges()) {
                fb.drawText(x + 32, ry, change.label + ":", Colors::TextDim, TextStyle::Body);
                fb.drawText(x + 250, ry, change.beforeValue + "  ->  " + change.afterValue,
                            Colors::Text, TextStyle::Body);
                ry += 54;
            }
        }'''
new_review_draw = '''        } else {
            constexpr int visible = 7;
            const int count = static_cast<int>(editor->pendingChanges().size());
            const int start = std::clamp(state.reviewRow - visible / 2, 0, std::max(0, count - visible));
            const int end = std::min(count, start + visible);
            for (int i = start; i < end; ++i) {
                const auto& change = editor->pendingChanges()[static_cast<std::size_t>(i)];
                fb.drawText(x + 32, ry, change.label + ":", i == state.reviewRow ? Colors::Accent : Colors::TextDim, TextStyle::Body);
                fb.drawText(x + 320, ry, change.beforeValue + "  ->  " + change.afterValue,
                            Colors::Text, TextStyle::Body);
                ry += 54;
            }
        }'''
if old_review_draw not in s:
    raise SystemExit('review draw loop marker missing')
s = s.replace(old_review_draw, new_review_draw, 1)
s = s.replace('"A Export Edited Save    Y Discard Changes    B Back",',
              '"Up/Down Browse    A Export Edited Save    Y Discard Changes    B Back",', 1)

# Intercept X on a selected box slot before inherited read-only handlers.
entry_marker = '''        const bool editorEntrySurface = detailViewActive &&
            (selectedMode == ViewMode::Trainer || selectedMode == ViewMode::Items) &&'''
entry_insert = '''        const bool pokemonEntrySurface = detailViewActive && selectedMode == ViewMode::Boxes &&
            selectedItemIndex >= 0 && !helpOverlayActive && !details.active && !actionSheet.isOpen() &&
            !saveConfirmActive && !pickerActive && !itemEditDialogActive;
        if (pokemonEntrySurface && (down & HidNpadButton_X)) {
            openPokemonActions(*this);
            return;
        }

        const bool editorEntrySurface = detailViewActive &&
            (selectedMode == ViewMode::Trainer || selectedMode == ViewMode::Items) &&'''
if entry_marker not in s:
    raise SystemExit('entry surface marker missing')
s = s.replace(entry_marker, entry_insert, 1)

p.write_text(s)
