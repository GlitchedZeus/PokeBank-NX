#include "UI/SharedPokemonEditorContract.h"
#include "UI/Gen2WorkspacePresentation.h"
#include "Integration/Encounter/EncounterGuardrails.h"

namespace UI::Gen2PokemonEditor {
namespace {

namespace Unified = PokeBank::UIModel::SharedPokemonEditor;
namespace Encounter = PokeVault::Integration::EncounterGuardrails;

struct UnifiedWorkspaceState {
    const TrainerViewScreen* owner = nullptr;
    std::string sourceGameId;
    Mode lastMode = Mode::None;
    Unified::Focus focus{};
    std::size_t detailsFocus = 0;
    bool crystal = false;
    bool progressionEditor = false;
    int progressionRow = 0;
    Gen2::PokemonRecord progressionBaseline{};
    bool moveEditor = false;
    int moveEditorRow = 0;
    int moveEditorSlot = 0;
    uint16_t moveBaseline = 0;
    uint8_t movePpBaseline = 0;
    uint8_t movePpUpsBaseline = 0;
};

UnifiedWorkspaceState& unifiedStateFor(const TrainerViewScreen& screen) {
    static UnifiedWorkspaceState state;
    const auto mode = stateFor(screen).mode;
    if (state.owner != &screen || state.sourceGameId != screen.sourceGameId) {
        state = {};
        state.owner = &screen;
        state.sourceGameId = screen.sourceGameId;
        state.lastMode = mode;
    } else if (state.lastMode != mode) {
        state.focus = {};
        state.detailsFocus = 0;
        state.progressionEditor = false;
        state.progressionRow = 0;
        state.lastMode = mode;
    }
    state.crystal = screen.sourceGameId == "crystal_gbc";
    return state;
}

void resetUnifiedFocus(TrainerViewScreen& screen) {
    auto& unified = unifiedStateFor(screen);
    unified.focus = {};
    unified.detailsFocus = 0;
    unified.progressionEditor = false;
    unified.progressionRow = 0;
    unified.moveEditor = false;
    unified.moveEditorRow = 0;
    unified.lastMode = stateFor(screen).mode;
    screen.controllerNavigation.reset();
}

void mirrorPickerFocus(State& state, const Unified::Focus& focus) {
    if (focus.panel == Unified::Panel::Details) {
        // Pokérus moved to shared DETAILS row 7, but the proven named picker adapter still uses
        // its internal legacy Values row 7. Keep that backend detail hidden behind the shared shell.
        if (focus.row == 7) {
            state.panel = Foundation::Panel::Values;
            state.row = 7;
            state.column = 0;
        } else {
            state.panel = Foundation::Panel::Identity;
            state.row = static_cast<int>(focus.row);
            state.column = 0;
        }
        return;
    }
    if (focus.panel == Unified::Panel::Moves) {
        state.panel = Foundation::Panel::Moves;
        state.row = focus.row;
        state.column = focus.column;
        return;
    }
    state.panel = Foundation::Panel::Values;
    state.row = static_cast<int>(focus.row);
    state.column = focus.column;
}

bool sharedPickerField(const Unified::Focus& focus) noexcept {
    return (focus.panel == Unified::Panel::Details && (focus.row == 0 || focus.row == 7)) ||
           (focus.panel == Unified::Panel::Moves && focus.column == 0);
}

void beginProgressionEditor(TrainerViewScreen& screen) {
    auto& unified = unifiedStateFor(screen);
    unified.progressionEditor = true;
    unified.progressionRow = 0;
    unified.progressionBaseline = stateFor(screen).working;
    screen.controllerNavigation.reset();
}

bool handleProgressionEditor(TrainerViewScreen& screen, uint64_t down, uint64_t held,
                             int stickX, int stickY) {
    auto& unified = unifiedStateFor(screen);
    if (!unified.progressionEditor) return false;
    auto& state = stateFor(screen);
    const uint64_t nav = screen.controllerNavigation.apply(
        down, held, stickX, stickY,
        HidNpadButton_Up, HidNpadButton_Down, HidNpadButton_Left, HidNpadButton_Right);
    if (nav & HidNpadButton_Up) unified.progressionRow = (unified.progressionRow + 3) % 4;
    if (nav & HidNpadButton_Down) unified.progressionRow = (unified.progressionRow + 1) % 4;
    if (down & HidNpadButton_B) {
        state.working = unified.progressionBaseline;
        unified.progressionEditor = false;
        screen.controllerNavigation.reset();
        return true;
    }
    if (!(down & HidNpadButton_A)) return true;

    if (unified.progressionRow == 0) {
        const auto result = Utils::promptNumber("Level", state.working.level, 1, 100);
        if (result.accepted) state.setLevel(static_cast<uint8_t>(result.value));
    } else if (unified.progressionRow == 1) {
        const auto result = Utils::promptNumber("Experience", static_cast<int>(state.working.experience),
                                                0, static_cast<int>(state.maximumExperience()));
        if (result.accepted) state.setExperience(static_cast<uint32_t>(result.value));
    } else if (unified.progressionRow == 2) {
        unified.progressionEditor = false;
        screen.controllerNavigation.reset();
    } else {
        state.working = unified.progressionBaseline;
        unified.progressionEditor = false;
        screen.controllerNavigation.reset();
    }
    return true;
}


void beginUnifiedMoveEditor(TrainerViewScreen& screen, State& state, int slot) {
    auto& unified = unifiedStateFor(screen);
    unified.moveEditor = true;
    unified.moveEditorRow = 0;
    unified.moveEditorSlot = std::clamp(slot, 0, 3);
    const auto index = static_cast<std::size_t>(unified.moveEditorSlot);
    unified.moveBaseline = state.working.moves[index];
    unified.movePpBaseline = state.working.pp[index];
    unified.movePpUpsBaseline = state.working.ppUps[index];
    screen.controllerNavigation.reset();
}

void cancelUnifiedMoveEditor(TrainerViewScreen& screen, State& state) {
    auto& unified = unifiedStateFor(screen);
    if (!unified.moveEditor) return;
    const auto index = static_cast<std::size_t>(unified.moveEditorSlot);
    state.working.moves[index] = unified.moveBaseline;
    state.working.pp[index] = unified.movePpBaseline;
    state.working.ppUps[index] = unified.movePpUpsBaseline;
    unified.moveEditor = false;
    unified.moveEditorRow = 0;
    screen.controllerNavigation.reset();
}

bool handleUnifiedMoveEditor(TrainerViewScreen& screen, State& state, uint64_t down,
                             uint64_t held, int stickX, int stickY) {
    auto& unified = unifiedStateFor(screen);
    if (!unified.moveEditor) return false;
    if (pickerStateFor(screen).model.active()) return false;

    const uint64_t nav = screen.controllerNavigation.apply(
        down, held, stickX, stickY,
        HidNpadButton_Up, HidNpadButton_Down, HidNpadButton_Left, HidNpadButton_Right);
    if (nav & HidNpadButton_Up) unified.moveEditorRow = (unified.moveEditorRow + 4) % 5;
    if (nav & HidNpadButton_Down) unified.moveEditorRow = (unified.moveEditorRow + 1) % 5;
    if (down & HidNpadButton_B) {
        cancelUnifiedMoveEditor(screen, state);
        return true;
    }
    if (!(down & HidNpadButton_A)) return true;

    const int slot = unified.moveEditorSlot;
    const auto index = static_cast<std::size_t>(slot);
    if (unified.moveEditorRow == 0) {
        Unified::Focus moveFocus{Unified::Panel::Moves, static_cast<uint8_t>(slot), 0};
        mirrorPickerFocus(state, moveFocus);
        auto& picker = pickerStateFor(screen);
        openPickerForFocusedField(screen, state, picker, HidNpadButton_A);
        return true;
    }
    if (unified.moveEditorRow == 1) {
        if (state.working.moves[index] == 0) {
            screen.postStatus("Choose a move before editing PP", 220);
            return true;
        }
        const auto result = Utils::promptNumber(
            "Move PP", std::min(state.working.pp[index], state.maximumPP(slot)),
            0, state.maximumPP(slot));
        if (result.accepted) state.setPP(slot, static_cast<uint8_t>(result.value));
        return true;
    }
    if (unified.moveEditorRow == 2) {
        if (state.working.moves[index] == 0) {
            screen.postStatus("Empty move slots keep PP Ups at zero", 220);
            return true;
        }
        const auto result = Utils::promptNumber("PP Ups", state.working.ppUps[index], 0, 3);
        if (result.accepted) state.setPPUps(slot, static_cast<uint8_t>(result.value));
        return true;
    }
    if (unified.moveEditorRow == 3) {
        unified.moveEditor = false;
        unified.moveEditorRow = 0;
        screen.controllerNavigation.reset();
        return true;
    }
    cancelUnifiedMoveEditor(screen, state);
    return true;
}

bool activateUnifiedField(TrainerViewScreen& screen, State& state, const Unified::Focus& focus) {
    auto& p = state.working;
    if (focus.panel == Unified::Panel::Details) {
        if (focus.row == 0) {
            mirrorPickerFocus(state, focus);
            return false;
        }
        if (focus.row == 1) {
            const auto result = Utils::promptText("Nickname", "1-10 Generation II characters", p.nickname, 10);
            if (result.accepted) p.nickname = result.text;
            return true;
        }
        if (focus.row == 2) {
            beginProgressionEditor(screen);
            return true;
        }
        if (focus.row == 3) {
            const auto result = Utils::promptText("OT Name", "1-10 Generation II characters",
                                                  p.originalTrainer, 10);
            if (result.accepted) p.originalTrainer = result.text;
            return true;
        }
        if (focus.row == 4) {
            const auto result = Utils::promptNumber("Trainer ID", p.trainerId, 0, 65535);
            if (result.accepted) p.trainerId = static_cast<uint16_t>(result.value);
            return true;
        }
        if (focus.row == 5) {
            openHeldItemPicker(state);
            return true;
        }
        if (focus.row == 6) {
            const auto result = Utils::promptNumber("Friendship", p.friendship, 0, 255);
            if (result.accepted) p.friendship = static_cast<uint8_t>(result.value);
            return true;
        }
        if (focus.row >= 8 && screen.sourceGameId == "crystal_gbc") {
            auto caught = Gen2Native::decodeCrystalCaughtData(p.caughtData);
            if (focus.row == 8) caught.timeOfDay = static_cast<uint8_t>((caught.timeOfDay + 1) % 4);
            if (focus.row == 9) {
                const auto choices = Encounter::forGameSpecies(screen.sourceGameId, p.species);
                const Encounter::EncounterTemplate* selected = nullptr;
                for (const auto& encounter : choices) {
                    if (encounter.location != caught.location) continue;
                    if (caught.levelCode == 1 && encounter.method == Encounter::Method::Egg) {
                        selected = &encounter; break;
                    }
                    if (caught.levelCode >= 2 && encounter.containsLevel(caught.levelCode)) {
                        selected = &encounter; break;
                    }
                    if (!selected) selected = &encounter;
                }
                if (!selected) {
                    screen.postStatus("Choose a legitimate Crystal encounter first", 300);
                    return true;
                }
                if (selected->method == Encounter::Method::Egg) {
                    caught.levelCode = 1;
                    screen.postStatus("Egg encounter uses Crystal's native egg marker", 240);
                } else if (selected->fixedLevel()) {
                    caught.levelCode = static_cast<uint8_t>(std::clamp<int>(selected->minLevel, 2, 63));
                    screen.postStatus("This encounter has fixed met level " +
                                      std::to_string(caught.levelCode), 240);
                } else {
                    const int minLevel = std::clamp<int>(selected->minLevel, 2, 63);
                    const int maxLevel = std::clamp<int>(selected->maxLevel, minLevel, 63);
                    const int current = caught.levelCode >= minLevel && caught.levelCode <= maxLevel
                        ? caught.levelCode : minLevel;
                    const auto result = Utils::promptNumber("Met Level for selected Crystal encounter",
                                                             current, minLevel, maxLevel);
                    if (!result.accepted) return true;
                    caught.levelCode = static_cast<uint8_t>(result.value);
                }
            }
            if (focus.row == 10) {
                pickerStateFor(screen).model.openLocation(
                    screen.sourceGameId, p.species, caught.location, caught.levelCode);
                if (pickerStateFor(screen).model.locationCount() == 0) {
                    pickerStateFor(screen).model.close();
                    screen.postStatus("No supported Crystal encounter templates for this species", 300);
                    return true;
                }
                screen.controllerNavigation.reset();
                return true;
            }
            if (focus.row == 11) caught.originalTrainerFemale = !caught.originalTrainerFemale;
            p.caughtData = Gen2Native::encodeCrystalCaughtData(caught);
            return true;
        }
        mirrorPickerFocus(state, focus);
        return false; // named Pokérus picker owns A.
    }

    if (focus.panel == Unified::Panel::Values) {
        if (focus.row <= 4) {
            if (focus.column == 0) {
                if (focus.row == 0) {
                    screen.postStatus("HP DV is derived from Attack, Defense, Speed and Special DVs", 260);
                    return true;
                }
                auto dvs = editableDVs(p);
                const int index = static_cast<int>(focus.row) - 1;
                const auto result = Utils::promptNumber("DV (0-15)", dvs[static_cast<std::size_t>(index)], 0, 15);
                if (result.accepted) {
                    dvs[static_cast<std::size_t>(index)] = static_cast<uint8_t>(result.value);
                    applyEditableDVs(p, dvs);
                }
                return true;
            }
            if (focus.column == 1) {
                auto statExp = p.statExperience;
                const auto result = Utils::promptNumber("Stat Experience (0-65535)",
                    statExp[static_cast<std::size_t>(focus.row)], 0, 65535);
                if (result.accepted) {
                    statExp[static_cast<std::size_t>(focus.row)] = static_cast<uint16_t>(result.value);
                    p.statExperience = statExp;
                }
                return true;
            }
            screen.postStatus("Calculated battle stats are read-only; edit DVs or Stat Exp instead", 280);
            return true;
        }
        if (focus.row == 5) { toggleShiny(screen, p); return true; }
        chooseGender(screen, p);
        return true;
    }

    // Move rows share the accepted Gen I interaction: A opens Move N first.
    // The Choose Move picker is a child of that editor, never the box Empty Slot action sheet.
    beginUnifiedMoveEditor(screen, state, static_cast<int>(focus.row));
    return true;
}

bool handleUnifiedWorkspaceInput(TrainerViewScreen& screen, uint64_t down, uint64_t held,
                                 int stickX, int stickY, const TouchInput& touch) {
    auto& state = stateFor(screen);
    if (state.mode == Mode::None) return false;
    auto& unified = unifiedStateFor(screen);

    if (state.mode == Mode::View) {
        if (PokeBank::UIModel::passiveViewAction(down & HidNpadButton_B, screen.touchedButtonId(touch)) ==
            PokeBank::UIModel::PassiveViewAction::Back) {
            closeToActions(screen);
            resetUnifiedFocus(screen);
        }
        return true;
    }

    if (pickerStateFor(screen).model.active()) return false;
    if (state.itemPicker) return handleItemPicker(screen, state, down, held, stickX, stickY);
    if (state.confirmExit) return handleConfirm(screen, state, down);
    if (handleProgressionEditor(screen, down, held, stickX, stickY)) return true;
    if (handleUnifiedMoveEditor(screen, state, down, held, stickX, stickY)) return true;

    const uint64_t nav = screen.controllerNavigation.apply(
        down, held, stickX, stickY,
        HidNpadButton_Up, HidNpadButton_Down, HidNpadButton_Left, HidNpadButton_Right);
    if (down & HidNpadButton_L) unified.focus = Unified::switchPanel(Unified::Generation::Gen2, unified.focus, -1, unified.crystal);
    if (down & HidNpadButton_R) unified.focus = Unified::switchPanel(Unified::Generation::Gen2, unified.focus, 1, unified.crystal);
    if (nav & HidNpadButton_Up) unified.focus = Unified::moveVertical(Unified::Generation::Gen2, unified.focus, -1, unified.crystal);
    if (nav & HidNpadButton_Down) unified.focus = Unified::moveVertical(Unified::Generation::Gen2, unified.focus, 1, unified.crystal);
    if (nav & HidNpadButton_Left) unified.focus = Unified::moveColumn(Unified::Generation::Gen2, unified.focus, -1, unified.crystal);
    if (nav & HidNpadButton_Right) unified.focus = Unified::moveColumn(Unified::Generation::Gen2, unified.focus, 1, unified.crystal);

    unified.detailsFocus = Unified::detailsScrollFocus(unified.focus, unified.detailsFocus);

    if (down & HidNpadButton_Y) {
        randomizeDVs(state);
        screen.postStatus("Generation II DVs randomized in this local editor session", 240);
        return true;
    }
    if ((down & HidNpadButton_X) && state.mode == Mode::Create) {
        if (stageCreate(screen)) resetUnifiedFocus(screen);
        return true;
    }
    if (down & HidNpadButton_B) {
        if (state.back()) {
            closeToActions(screen);
            resetUnifiedFocus(screen);
        } else {
            // Create/Edit now always stop here for an explicit Add/Keep, Discard,
            // or Continue decision. No local draft is lost on a stray Back press.
            screen.controllerNavigation.reset();
        }
        return true;
    }
    if (!(down & HidNpadButton_A)) return true;

    mirrorPickerFocus(state, unified.focus);
    if (!activateUnifiedField(screen, state, unified.focus))
        return false;
    return true;
}

Color unifiedStatusColor(Rules::CompatibilityState status) {
    if (status == Rules::CompatibilityState::Ok) return Colors::Success;
    if (status == Rules::CompatibilityState::UnusualPreserved) return Colors::Warning;
    return Colors::Error;
}

void drawUnifiedValues(PKSEFramebuffer& fb, const State& state, const UnifiedWorkspaceState& unified,
                       int x, int y, int w, int h, bool passive) {
    drawPanelSurface(fb, x, y, w, h, false, 14);
    fb.drawText(x + 14, y + 10, Unified::statsHeading(),
        !passive && unified.focus.panel == Unified::Panel::Values ? Colors::Accent : Colors::Text,
        TextStyle::Heading);
    fb.drawText(x + 114, y + 42, "DV", Colors::TextDim, TextStyle::Caption);
    fb.drawText(x + 184, y + 42, "Stat Exp", Colors::TextDim, TextStyle::Caption);
    fb.drawText(x + 292, y + 42, "Stat", Colors::TextDim, TextStyle::Caption);

    const auto& p = state.working;
    const auto battle = PokeBank::UIModel::Gen2Workspace::battleStats(p);
    const std::array<const char*, 5> labels{"HP", "Attack", "Defense", "Speed", "Special"};
    const std::array<uint8_t, 5> dvs{p.dvs[0], p.dvs[1], p.dvs[2], p.dvs[3], p.dvs[4]};
    for (int r = 0; r < 5; ++r) {
        const int yy = y + 70 + r * 34;
        const bool rowSelected = !passive && unified.focus.panel == Unified::Panel::Values && unified.focus.row == r;
        if (rowSelected) {
            const auto cell = Unified::cellFocus(unified.focus);
            fb.drawRoundedRect(x + cell.x, yy - 5, cell.width, 28, 6, Colors::Accent, 2);
        }
        fb.drawText(x + 18, yy, labels[static_cast<std::size_t>(r)], Colors::TextDim, TextStyle::Caption);
        fb.drawText(x + 114, yy, std::to_string(dvs[static_cast<std::size_t>(r)]) + (r == 0 ? " *" : ""),
                    rowSelected && unified.focus.column == 0 ? Colors::Accent : Colors::Text, TextStyle::Caption);
        fb.drawText(x + 184, yy, std::to_string(p.statExperience[static_cast<std::size_t>(r)]),
                    rowSelected && unified.focus.column == 1 ? Colors::Accent : Colors::Text, TextStyle::Caption);
        const std::string stat = r < 4 ? std::to_string(battle[static_cast<std::size_t>(r)]) : "—";
        fb.drawText(x + 282, yy, stat, Colors::TextDim, TextStyle::Caption);
    }
    fb.drawText(x + 18, y + 246, "CALCULATED SPECIAL STATS", Colors::TextDim, TextStyle::Caption);
    fb.drawText(x + 18, y + 274, "SpA", Colors::TextDim, TextStyle::Caption);
    fb.drawText(x + 84, y + 274, std::to_string(battle[4]), Colors::Text, TextStyle::Caption);
    fb.drawText(x + 200, y + 274, "SpD", Colors::TextDim, TextStyle::Caption);
    fb.drawText(x + 266, y + 274, std::to_string(battle[5]), Colors::Text, TextStyle::Caption);
    fb.drawText(x + 18, y + 304, "* HP DV derived • one stored Special DV / Stat Exp", Colors::TextDim, TextStyle::Caption);

    const std::array<std::pair<std::string, std::string>, 2> capabilityRows{{
        {"Shiny", p.shiny ? "Yes (DV-derived)" : "No (DV-derived)"},
        {"Gender", genderText(p.gender)},
    }};
    for (int i = 0; i < 2; ++i) {
        const int row = 5 + i;
        const int yy = y + 350 + i * 42;
        const bool selected = !passive && unified.focus.panel == Unified::Panel::Values && unified.focus.row == row;
        if (selected) {
            const auto cell = Unified::cellFocus(unified.focus);
            fb.drawRoundedRect(x + cell.x, yy - 5, cell.width, 28, 6, Colors::Accent, 2);
        }
        fb.drawText(x + 18, yy, capabilityRows[static_cast<std::size_t>(i)].first, Colors::TextDim, TextStyle::Caption);
        fb.drawText(x + 134, yy, capabilityRows[static_cast<std::size_t>(i)].second,
                    selected ? Colors::Accent : (i == 0 && p.shiny ? Colors::ShinyStar : Colors::Text),
                    TextStyle::Caption);
    }
}


void drawUnifiedMoveEditor(TrainerViewScreen& screen, PKSEFramebuffer& fb,
                           State& state, const UnifiedWorkspaceState& unified) {
    if (!unified.moveEditor) return;
    const auto slot = static_cast<std::size_t>(unified.moveEditorSlot);
    constexpr int w = 650, h = 390, navH = 48;
    const int x = (fb.getWidth() - w) / 2;
    const int y = (fb.getHeight() - navH - h) / 2;
    fb.drawFilledRect(0, 0, fb.getWidth(), fb.getHeight() - navH, Color(0, 0, 0, 95));
    drawPanelSurface(fb, x, y, w, h, false, 18);
    fb.drawRoundedRect(x, y, w, h, 18, Colors::FocusBorder, 2);
    fb.drawText(x + 24, y + 18, "Move " + std::to_string(unified.moveEditorSlot + 1),
                Colors::Text, TextStyle::Heading);
    fb.drawText(x + 24, y + 52, "Contextual editor — B always cancels this dialog",
                Colors::TextDim, TextStyle::Caption);

    const uint16_t move = state.working.moves[slot];
    const std::array<std::pair<std::string, std::string>, 5> rows{{
        {"Move", move == 0 ? std::string("Empty") : std::string(Names::getMoveName(move))},
        {"PP", std::to_string(state.working.pp[slot]) + "/" +
               std::to_string(state.maximumPP(unified.moveEditorSlot))},
        {"PP Ups", std::to_string(state.working.ppUps[slot])},
        {"Apply", "Keep these move details"},
        {"Cancel", "Discard these move details"},
    }};
    int rowY = y + 88;
    for (int i = 0; i < static_cast<int>(rows.size()); ++i) {
        const bool selected = i == unified.moveEditorRow;
        if (selected) fb.drawSelectionHighlight(x + 22, rowY, w - 44, 42);
        fb.drawText(x + 38, rowY + 10, rows[static_cast<std::size_t>(i)].first,
                    selected ? Colors::Text : Colors::TextDim);
        int valueW = 0, valueH = 0;
        fb.measureText(rows[static_cast<std::size_t>(i)].second, valueW, valueH, TextStyle::Body);
        fb.drawText(x + w - 38 - valueW, rowY + 10, rows[static_cast<std::size_t>(i)].second,
                    selected ? Colors::Text : Colors::TextDim);
        rowY += 48;
    }
    drawNavBar(fb, {{"D-pad/Stick", "Navigate"}, {"A", "Edit / Apply"}, {"B", "Cancel"}});
}

void drawUnifiedDetails(PKSEFramebuffer& fb, const State& state, const UnifiedWorkspaceState& unified,
                        int x, int y, int w, int h, bool passive) {
    drawPanelSurface(fb, x, y, w, h, false, 14);
    fb.drawText(x + 14, y + 10, "DETAILS",
        !passive && unified.focus.panel == Unified::Panel::Details ? Colors::Accent : Colors::Text,
        TextStyle::Caption);
    const auto& p = state.working;
    SharedPokemonShell::drawPortrait(fb, x, y, p.species, p.shiny);
    if (const auto* personal = Gen2::personalRecord(p.species)) {
        int badgeX = x + 18;
        sharedBadge(fb, badgeX, y + 184, sharedTypeName(personal->rawType1));
        if (personal->rawType2 != personal->rawType1) {
            int tw = 0, th = 0;
            fb.measureText(sharedTypeName(personal->rawType1), tw, th, TextStyle::Caption);
            badgeX += std::max(64, tw + 24) + 8;
            sharedBadge(fb, badgeX, y + 184, sharedTypeName(personal->rawType2));
        }
    }
    std::vector<std::pair<std::string, std::string>> rows{
        {"Species", std::to_string(p.species) + " - " + Names::getSpeciesName(p.species)},
        {"Nickname", p.nickname},
        {"Level / EXP", "Lv " + std::to_string(p.level) + " / " + std::to_string(p.experience)},
        {"OT Name", p.originalTrainer},
        {"Trainer ID", std::to_string(p.trainerId)},
        {"Held Item", itemText(p.heldItem)},
        {"Friendship", std::to_string(p.friendship)},
        {"Pokerus", Gen2Native::pokerusText(p.pokerus)},
    };
    if (unified.crystal) {
        const auto caught = Gen2Native::decodeCrystalCaughtData(p.caughtData);
        rows.push_back({"Met time", Gen2Native::crystalMetTimeName(caught.timeOfDay)});
        rows.push_back({"Met level", Gen2Native::crystalCaughtLevelText(caught)});
        rows.push_back({"Location", Gen2Native::crystalCaughtLocationName(caught.location)});
        rows.push_back({"OT gender", Gen2Native::crystalOriginalTrainerGenderText(caught)});
    }

    const auto window = Unified::scrollWindow(rows.size(), 8,
        passive ? 0 : unified.detailsFocus);
    for (std::size_t i = 0; i < window.count; ++i) {
        const std::size_t row = window.first + i;
        const int yy = y + 224 + static_cast<int>(i) * 38;
        const bool selected = !passive && unified.focus.panel == Unified::Panel::Details && unified.focus.row == row;
        if (selected) {
            const auto cell = Unified::cellFocus(unified.focus);
            fb.drawRoundedRect(x + cell.x, yy + 2, cell.width, 30, 6, Colors::Accent, 2);
        }
        fb.drawText(x + 18, yy + 7, rows[row].first, Colors::TextDim, TextStyle::Caption);
        fb.drawText(x + 108, yy + 7, rows[row].second,
                    selected ? Colors::Accent : Colors::Text, TextStyle::Caption);
    }
    if (window.scrolls)
        fb.drawText(x + w - 44, y + h - 26, "↕", Colors::TextDim, TextStyle::Caption);
}

void drawUnifiedMoves(PKSEFramebuffer& fb, const TrainerViewScreen& screen, const State& state,
                      const UnifiedWorkspaceState& unified, int x, int y, int w, int h, bool passive) {
    drawPanelSurface(fb, x, y, w, h, false, 14);
    fb.drawText(x + 14, y + 10, "MOVES",
        !passive && unified.focus.panel == Unified::Panel::Moves ? Colors::Accent : Colors::Text,
        TextStyle::Heading);
    fb.drawText(x + 190, y + 42, "PP", Colors::TextDim, TextStyle::Caption);
    fb.drawText(x + 246, y + 42, "Ups", Colors::TextDim, TextStyle::Caption);
    for (int slot = 0; slot < 4; ++slot) {
        const int yy = y + 62 + slot * 32;
        const bool selected = !passive && unified.focus.panel == Unified::Panel::Moves && unified.focus.row == slot;
        if (selected) {
            const auto cell = Unified::cellFocus(unified.focus);
            fb.drawRoundedRect(x + cell.x, yy - 5, cell.width, 28, 6, Colors::Accent, 2);
        }
        const uint8_t move = state.working.moves[static_cast<std::size_t>(slot)];
        const auto status = moveStatus(screen, state, slot);
        fb.drawText(x + 18, yy, move == 0 ? "Empty" : Names::getMoveName(move),
                    selected && unified.focus.column == 0 ? Colors::Accent : Colors::Text, TextStyle::Caption);
        fb.drawText(x + 190, yy, std::to_string(state.working.pp[static_cast<std::size_t>(slot)]),
                    selected && unified.focus.column == 1 ? Colors::Accent : Colors::Text, TextStyle::Caption);
        fb.drawText(x + 246, yy, std::to_string(state.working.ppUps[static_cast<std::size_t>(slot)]),
                    selected && unified.focus.column == 2 ? Colors::Accent : Colors::Text, TextStyle::Caption);
        fb.drawText(x + 292, yy, status == Rules::CompatibilityState::Ok ? "OK" :
            status == Rules::CompatibilityState::UnusualPreserved ? "Unusual" : "Invalid", unifiedStatusColor(status), TextStyle::Caption);
    }
    Rules::CompatibilityState summary = Rules::CompatibilityState::Ok;
    for (int slot = 0; slot < 4; ++slot) {
        const auto status = moveStatus(screen, state, slot);
        if (static_cast<int>(status) > static_cast<int>(summary)) summary = status;
    }
    fb.drawHDivider(x + 14, y + 184, w - 28);
    fb.drawText(x + 18, y + 194,
        std::string("Move compatibility: ") + (summary == Rules::CompatibilityState::NeedsCorrection
            ? "Invalid draft" : Rules::compatibilityText(summary)),
        unifiedStatusColor(summary), TextStyle::Caption);
    fb.drawText(x + 18, y + 220, "Encounter legality: Not checked", Colors::TextDim, TextStyle::Caption);
    if (const auto game = Rules::exactGameFromSourceId(screen.sourceGameId))
        Gen2WorkspacePresentation::drawDataAndGraph(fb, x, y + 244, w, h - 256, state.working, *game);
}

void drawProgressionEditor(PKSEFramebuffer& fb, const State& state, const UnifiedWorkspaceState& unified) {
    constexpr int x = 320, y = 168, w = 640, h = 330;
    fb.drawFilledRect(0, 0, fb.getWidth(), fb.getHeight() - 48, Color(0, 0, 0, 150));
    drawPanelSurface(fb, x, y, w, h, false, 18);
    fb.drawText(x + 24, y + 18, "Level / Experience", Colors::Text, TextStyle::Heading);
    fb.drawText(x + 24, y + 52, "Contextual editor — changes remain local until Add / Keep",
                Colors::TextDim, TextStyle::Caption);
    const std::array<std::pair<std::string, std::string>, 4> rows{{
        {"Level", std::to_string(state.working.level)},
        {"Experience", std::to_string(state.working.experience)},
        {"Apply", "Return to shared Pokemon editor"},
        {"Cancel", "Restore values from before this dialog"},
    }};
    for (int i = 0; i < 4; ++i) {
        const int yy = y + 96 + i * 48;
        if (unified.progressionRow == i) fb.drawSelectionHighlight(x + 22, yy - 5, w - 44, 40);
        fb.drawText(x + 38, yy + 5, rows[static_cast<std::size_t>(i)].first,
                    unified.progressionRow == i ? Colors::Accent : Colors::TextDim, TextStyle::Caption);
        fb.drawText(x + 250, yy + 5, rows[static_cast<std::size_t>(i)].second, Colors::Text, TextStyle::Caption);
    }
    drawNavBar(fb, {{"Up/Down", "Field"}, {"A", "Edit/Apply"}, {"B", "Cancel"}});
}

void drawUnifiedWorkspace(TrainerViewScreen& screen, PKSEFramebuffer& fb) {
    auto& state = stateFor(screen);
    auto& unified = unifiedStateFor(screen);
    const bool view = state.mode == Mode::View;
    const bool create = state.mode == Mode::Create;
    const auto g = Unified::geometry720p();
    screen.touchButtons.clear();
    fb.drawFilledRect(0, 0, fb.getWidth(), fb.getHeight() - 48, Color(0, 0, 0, 105));
    drawPanelSurface(fb, g.x, g.y, g.width, g.height, false, 18);
    const auto surface = view ? Unified::Surface::View : create ? Unified::Surface::CreateDraft : Unified::Surface::Edit;
    fb.drawText(g.x + 24, g.y + 14, Unified::titleFor(surface), Colors::Text, TextStyle::Heading);
    std::string subtitle = "Box " + std::to_string(state.box + 1);
    if (state.slot >= 0) subtitle += " / Slot " + std::to_string(state.slot + 1);
    subtitle += "   •   "; subtitle += sharedGameName(screen); subtitle += "   •   Source save immutable";
    fb.drawText(g.x + 24, g.y + 46, subtitle, Colors::TextDim, TextStyle::Caption);

    const int contentY = g.y + 74;
    const int contentH = g.height - 94;
    const int leftX = g.x + 18, leftW = 300;
    const int midX = leftX + leftW + 14, midW = 398;
    const int rightX = midX + midW + 14, rightW = g.x + g.width - 18 - rightX;
    drawUnifiedDetails(fb, state, unified, leftX, contentY, leftW, contentH, view);
    drawUnifiedValues(fb, state, unified, midX, contentY, midW, contentH, view);
    drawUnifiedMoves(fb, screen, state, unified, rightX, contentY, rightW, contentH, view);

    if (view) {
        screen.touchButtons.push_back({PokeBank::UIModel::passiveBackTarget, 16, fb.getHeight() - 68, 190, 58});
        drawNavBar(fb, {{"B", "Back"}, {"Touch", "Back"}});
    } else if (create) {
        drawNavBar(fb, {{"D-pad/Stick", "Navigate"}, {"L/R", "Panel"}, {"A", "Edit"},
                        {"Y", "Random DVs"}, {"X", "Add"}, {"B", "Cancel Draft"}});
    } else {
        drawNavBar(fb, {{"D-pad/Stick", "Navigate"}, {"L/R", "Panel"}, {"A", "Edit"},
                        {"Y", "Random DVs"}, {"B", "Back"}});
    }
    if (state.itemPicker) drawHeldItemPicker(fb, state);
    if (state.confirmExit) drawConfirm(fb, state);
    if (unified.progressionEditor) drawProgressionEditor(fb, state, unified);
}

} // namespace

bool unifiedGen2SurfaceOwnsFrame(const TrainerViewScreen& screen) noexcept {
    return sharedSurfaceOwnsFrame(screen);
}

bool handleUnifiedGen2SurfaceInput(TrainerViewScreen& screen, uint64_t down, uint64_t held,
                                   int stickX, int stickY, const TouchInput& touch) {
    if (!isGSC(screen)) return false;
    if (stateFor(screen).mode != Mode::None)
        return handleUnifiedWorkspaceInput(screen, down, held, stickX, stickY, touch);
    return handleSharedSurfaceInput(screen, down, held, stickX, stickY, touch);
}

bool drawUnifiedGen2Surface(TrainerViewScreen& screen, PKSEFramebuffer& fb) {
    if (!isGSC(screen)) return false;
    if (stateFor(screen).mode != Mode::None) {
        drawUnifiedWorkspace(screen, fb);
        return true;
    }
    return drawSharedSurface(screen, fb);
}

} // namespace UI::Gen2PokemonEditor