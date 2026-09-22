#include "UI/StatsRadar.h"

namespace UI::Gen1PokemonEditor {
namespace {

enum class UX2Mode : uint8_t {
    Closed,
    Actions,
    View,
    Edit,
    AddDraft,
    LevelExpEditor,
    MoveEditor,
    DVEditor,
    StatExpEditor,
    CloneDestination,
    RemoveConfirm,
    Review,
    Provenance,
};

enum class UX2Picker : uint8_t { None, Species, Move };

struct UX2State {
    const TrainerViewScreen* owner = nullptr;
    std::string sourceGameId;
    UX2Mode mode = UX2Mode::Closed;
    int box = 0;
    int slot = 0;
    int row = 0;
    int subRow = 0;
    int reviewRow = 0;

    UX2Picker picker = UX2Picker::None;
    int pickerValue = 0;
    uint16_t pickerCommittedSpecies = 1;
    int moveSlot = 0;
    bool subForAdd = false;

    BoxPokemonCreate draft{};
    uint8_t tempLevel = 5;
    uint32_t tempExperience = 0;
    uint8_t tempMove = 0;
    uint8_t tempPP = 0;
    uint8_t tempPPUps = 0;
    std::array<uint8_t,4> tempDVs{8,8,8,8};
    std::array<uint16_t,5> tempStatExperience{};

    int cloneBox = 0;
    int cloneSlot = 0;
    std::string lastExportDirectory;
};

UX2State& ux2StateFor(const TrainerViewScreen& screen) {
    static UX2State state;
    if (state.owner != &screen || state.sourceGameId != screen.sourceGameId) {
        state = {};
        state.owner = &screen;
        state.sourceGameId = screen.sourceGameId;
    }
    return state;
}

std::optional<PokemonRecord> ux2SelectedPokemon(TrainerViewScreen& screen, std::string& error) {
    const auto& state = ux2StateFor(screen);
    return pokemonAt(screen, state.box, state.slot, error);
}

int ux2BoxCount(const Editor& e) noexcept {
    return static_cast<int>(e.view().boxes().size());
}

int ux2SlotCount(const Editor& e, int box) noexcept {
    if (box < 0 || box >= ux2BoxCount(e)) return 0;
    return static_cast<int>(e.view().boxes()[static_cast<size_t>(box)].slots.size());
}

int ux2FirstEmpty(const Editor& e, int box) {
    if (box < 0 || box >= ux2BoxCount(e)) return -1;
    const auto& slots = e.view().boxes()[static_cast<size_t>(box)].slots;
    for (int slot = 0; slot < static_cast<int>(slots.size()); ++slot)
        if (!slots[static_cast<size_t>(slot)]) return slot;
    return -1;
}

const char* ux2SectionForRow(int row) noexcept {
    if (row < 3) return "Summary";
    if (row < 7) return "Moves";
    if (row < 9) return "Stats";
    if (row < 11) return "Trainer";
    return "Actions";
}

int ux2JumpSectionRow(int row, int direction) noexcept {
    constexpr auto starts = UX::logicalSectionStarts();
    int current = 0;
    for (int i = 0; i < static_cast<int>(starts.size()); ++i)
        if (row >= static_cast<int>(starts[static_cast<size_t>(i)])) current = i;
    current = (current + direction + static_cast<int>(starts.size())) % static_cast<int>(starts.size());
    return static_cast<int>(starts[static_cast<size_t>(current)]);
}

void ux2Close(UX2State& state) {
    state.mode = UX2Mode::Closed;
    state.row = 0;
    state.subRow = 0;
    state.picker = UX2Picker::None;
}

void ux2OpenActions(TrainerViewScreen& screen) {
    auto& state = ux2StateFor(screen);
    auto* trainer = rbyTrainer(screen);
    if (!trainer || !trainer->stagedPokemonAvailable()) {
        const std::string why = trainer ? trainer->stagedPokemonUnavailableReason() : std::string{};
        screen.postStatus(why.empty() ? "Generation I boxed editing is unavailable; source remains read-only" : why, 300);
        return;
    }
    if (screen.selectedBoxIndex < 0 || screen.selectedBoxIndex >= static_cast<int>(screen.trainer.getBoxCount()) ||
        screen.selectedItemIndex < 0 || screen.selectedItemIndex >= static_cast<int>(screen.trainer.getSlotsPerBox())) {
        screen.postStatus("Select a Generation I box slot first");
        return;
    }
    state.mode = UX2Mode::Actions;
    state.box = screen.selectedBoxIndex;
    state.slot = screen.selectedItemIndex;
    state.row = 0;
    state.picker = UX2Picker::None;
}

void ux2BeginAdd(TrainerViewScreen& screen) {
    auto& state = ux2StateFor(screen);
    auto* e = editor(screen);
    if (!e) return;
    std::string error;
    const auto destination = e->appendSlot(static_cast<size_t>(state.box), error);
    if (!destination) {
        screen.postStatus(error, 300);
        return;
    }
    state.slot = static_cast<int>(*destination);
    state.draft = BoxPokemonCreate{};
    state.mode = UX2Mode::AddDraft;
    state.row = 0;
    state.picker = UX2Picker::None;
}

bool ux2CommitEdit(TrainerViewScreen& screen, const BoxPokemonEdit& edit, const char* success) {
    auto& state = ux2StateFor(screen);
    auto* e = editor(screen);
    if (!e) return false;
    std::string error;
    if (!e->stageEdit(static_cast<size_t>(state.box), static_cast<size_t>(state.slot), edit, error)) {
        screen.postStatus(error.empty() ? "Generation I edit was rejected" : error, 300);
        return false;
    }
    refreshBoxes(screen);
    screen.postStatus(success ? success : "Pokemon change staged only; original .srm unchanged", 240);
    return true;
}

bool ux2StageAdd(TrainerViewScreen& screen) {
    auto& state = ux2StateFor(screen);
    auto* e = editor(screen);
    if (!e) return false;
    std::string error;
    if (!e->stageAdd(static_cast<size_t>(state.box), static_cast<size_t>(state.slot), state.draft, error)) {
        screen.postStatus(error.empty() ? "Generation I Add was rejected" : error, 360);
        return false;
    }
    refreshBoxes(screen);
    screen.postStatus("Pokemon staged in box; original RetroArch .srm unchanged", 300);
    ux2Close(state);
    return true;
}

bool ux2StageRemove(TrainerViewScreen& screen) {
    auto& state = ux2StateFor(screen);
    auto* e = editor(screen);
    if (!e) return false;
    std::string error;
    if (!e->stageRemove(static_cast<size_t>(state.box), static_cast<size_t>(state.slot), error)) {
        screen.postStatus(error.empty() ? "Generation I removal was rejected" : error, 300);
        return false;
    }
    refreshBoxes(screen);
    screen.postStatus("Removal staged with Gen I box compaction; original .srm unchanged", 300);
    ux2Close(state);
    return true;
}

void ux2OpenSpeciesPicker(UX2State& state, uint16_t current) {
    state.picker = UX2Picker::Species;
    state.pickerValue = std::clamp<int>(current, 1, 151);
    state.pickerCommittedSpecies = current;
}

void ux2OpenMovePicker(UX2State& state) {
    state.picker = UX2Picker::Move;
    state.pickerValue = std::clamp<int>(state.tempMove, 0, 165);
}

uint16_t ux2PreviewSpecies(TrainerViewScreen& screen) {
    auto& state = ux2StateFor(screen);
    if (state.picker == UX2Picker::Species)
        return static_cast<uint16_t>(state.pickerValue);
    if (state.mode == UX2Mode::AddDraft) return state.draft.species;
    std::string error;
    const auto pokemon = ux2SelectedPokemon(screen, error);
    return pokemon ? pokemon->species : 0;
}

bool ux2ApplySpeciesPicker(TrainerViewScreen& screen) {
    auto& state = ux2StateFor(screen);
    const auto selected = static_cast<uint16_t>(state.pickerValue);
    if (state.mode == UX2Mode::AddDraft) {
        state.draft.species = selected;
        state.picker = UX2Picker::None;
        return true;
    }
    if (state.mode == UX2Mode::Edit) {
        BoxPokemonEdit edit;
        edit.species = selected;
        const bool ok = ux2CommitEdit(screen, edit, "Species change staged; source .srm unchanged");
        state.picker = UX2Picker::None;
        return ok;
    }
    return false;
}

void ux2OpenLevelExpEditor(TrainerViewScreen& screen, bool add) {
    auto& state = ux2StateFor(screen);
    state.subForAdd = add;
    state.subRow = 0;
    if (add) {
        state.tempLevel = state.draft.level;
        state.tempExperience = draftExperience(state.draft);
    } else {
        std::string error;
        const auto pokemon = ux2SelectedPokemon(screen, error);
        if (!pokemon) { screen.postStatus(error.empty() ? "Pokemon unavailable" : error, 300); return; }
        state.tempLevel = pokemon->level;
        state.tempExperience = pokemon->experience;
    }
    state.mode = UX2Mode::LevelExpEditor;
}

void ux2OpenMoveEditor(TrainerViewScreen& screen, int slot, bool add) {
    auto& state = ux2StateFor(screen);
    state.moveSlot = std::clamp(slot, 0, 3);
    state.subForAdd = add;
    state.subRow = 0;
    if (add) {
        state.tempMove = state.draft.moves[static_cast<size_t>(state.moveSlot)];
        state.tempPP = state.draft.pp[static_cast<size_t>(state.moveSlot)];
        state.tempPPUps = state.draft.ppUps[static_cast<size_t>(state.moveSlot)];
    } else {
        std::string error;
        const auto pokemon = ux2SelectedPokemon(screen, error);
        if (!pokemon) { screen.postStatus(error.empty() ? "Pokemon unavailable" : error, 300); return; }
        state.tempMove = pokemon->moves[static_cast<size_t>(state.moveSlot)];
        state.tempPP = pokemon->pp[static_cast<size_t>(state.moveSlot)];
        state.tempPPUps = pokemon->ppUps[static_cast<size_t>(state.moveSlot)];
    }
    state.mode = UX2Mode::MoveEditor;
}

void ux2OpenDVEditor(TrainerViewScreen& screen, bool add) {
    auto& state = ux2StateFor(screen);
    state.subForAdd = add;
    state.subRow = 0;
    if (add) state.tempDVs = state.draft.dvs;
    else {
        std::string error;
        const auto pokemon = ux2SelectedPokemon(screen, error);
        if (!pokemon) { screen.postStatus(error.empty() ? "Pokemon unavailable" : error, 300); return; }
        state.tempDVs = storedDVs(*pokemon);
    }
    state.mode = UX2Mode::DVEditor;
}

void ux2OpenStatExpEditor(TrainerViewScreen& screen, bool add) {
    auto& state = ux2StateFor(screen);
    state.subForAdd = add;
    state.subRow = 0;
    if (add) state.tempStatExperience = state.draft.statExperience;
    else {
        std::string error;
        const auto pokemon = ux2SelectedPokemon(screen, error);
        if (!pokemon) { screen.postStatus(error.empty() ? "Pokemon unavailable" : error, 300); return; }
        state.tempStatExperience = pokemon->statExperience;
    }
    state.mode = UX2Mode::StatExpEditor;
}

void ux2ReturnFromSubEditor(UX2State& state) {
    state.mode = state.subForAdd ? UX2Mode::AddDraft : UX2Mode::Edit;
    state.subRow = 0;
    state.picker = UX2Picker::None;
}

bool ux2ApplyLevelExp(TrainerViewScreen& screen) {
    auto& state = ux2StateFor(screen);
    if (state.subForAdd) {
        state.draft.level = state.tempLevel;
        ux2ReturnFromSubEditor(state);
        return true;
    }
    BoxPokemonEdit edit;
    edit.level = state.tempLevel;
    edit.experience = state.tempExperience;
    if (!ux2CommitEdit(screen, edit, "Level / EXP change staged; source .srm unchanged")) return false;
    ux2ReturnFromSubEditor(state);
    return true;
}

bool ux2ApplyMove(TrainerViewScreen& screen) {
    auto& state = ux2StateFor(screen);
    const auto i = static_cast<size_t>(state.moveSlot);
    if (state.subForAdd) {
        state.draft.moves[i] = state.tempMove;
        state.draft.pp[i] = state.tempPP;
        state.draft.ppUps[i] = state.tempPPUps;
        ux2ReturnFromSubEditor(state);
        return true;
    }
    std::string error;
    auto pokemon = ux2SelectedPokemon(screen, error);
    if (!pokemon) { screen.postStatus(error.empty() ? "Pokemon unavailable" : error, 300); return false; }
    auto moves = pokemon->moves;
    auto pp = pokemon->pp;
    auto ups = pokemon->ppUps;
    moves[i] = state.tempMove;
    pp[i] = state.tempPP;
    ups[i] = state.tempPPUps;
    BoxPokemonEdit edit;
    edit.moves = moves;
    edit.pp = pp;
    edit.ppUps = ups;
    if (!ux2CommitEdit(screen, edit, "Move change staged; source .srm unchanged")) return false;
    ux2ReturnFromSubEditor(state);
    return true;
}

bool ux2ApplyDVs(TrainerViewScreen& screen) {
    auto& state = ux2StateFor(screen);
    if (state.subForAdd) {
        state.draft.dvs = state.tempDVs;
        ux2ReturnFromSubEditor(state);
        return true;
    }
    BoxPokemonEdit edit;
    edit.dvs = state.tempDVs;
    if (!ux2CommitEdit(screen, edit, "DV changes staged; source .srm unchanged")) return false;
    ux2ReturnFromSubEditor(state);
    return true;
}

bool ux2ApplyStatExp(TrainerViewScreen& screen) {
    auto& state = ux2StateFor(screen);
    if (state.subForAdd) {
        state.draft.statExperience = state.tempStatExperience;
        ux2ReturnFromSubEditor(state);
        return true;
    }
    BoxPokemonEdit edit;
    edit.statExperience = state.tempStatExperience;
    if (!ux2CommitEdit(screen, edit, "Stat Exp changes staged; source .srm unchanged")) return false;
    ux2ReturnFromSubEditor(state);
    return true;
}

void ux2OpenClone(TrainerViewScreen& screen) {
    auto& state = ux2StateFor(screen);
    auto* e = editor(screen);
    if (!e) return;
    state.cloneBox = std::clamp(state.box, 0, std::max(0, ux2BoxCount(*e) - 1));
    const int first = ux2FirstEmpty(*e, state.cloneBox);
    state.cloneSlot = first >= 0 ? first : 0;
    state.mode = UX2Mode::CloneDestination;
}

bool ux2StageClone(TrainerViewScreen& screen) {
    auto& state = ux2StateFor(screen);
    auto* e = editor(screen);
    if (!e) return false;
    const int slots = ux2SlotCount(*e, state.cloneBox);
    if (state.cloneSlot < 0 || state.cloneSlot >= slots) return false;
    std::string error;
    const auto occupied = e->boxedPokemon(static_cast<size_t>(state.cloneBox), static_cast<size_t>(state.cloneSlot), error);
    if (!error.empty()) { screen.postStatus(error, 300); return false; }
    if (occupied) {
        screen.postStatus("Choose an empty destination slot", 240);
        return false;
    }
    const int writable = ux2FirstEmpty(*e, state.cloneBox);
    if (writable < 0) {
        screen.postStatus("Destination Generation I box is full", 240);
        return false;
    }
    if (state.cloneSlot != writable) {
        screen.postStatus("Gen I boxes are contiguous; only the next writable empty slot can store a clone", 360);
        return false;
    }
    if (!e->stageClone(static_cast<size_t>(state.box), static_cast<size_t>(state.slot),
                       static_cast<size_t>(state.cloneBox), static_cast<size_t>(state.cloneSlot), error)) {
        screen.postStatus(error.empty() ? "Generation I clone was rejected" : error, 300);
        return false;
    }
    refreshBoxes(screen);
    screen.postStatus("Clone staged at chosen writable slot; source Pokemon and original .srm unchanged", 300);
    ux2Close(state);
    return true;
}

void ux2HandlePicker(TrainerViewScreen& screen, uint64_t down) {
    auto& state = ux2StateFor(screen);
    const int minValue = state.picker == UX2Picker::Species ? 1 : 0;
    const int maxValue = state.picker == UX2Picker::Species ? 151 : 165;
    if (down & HidNpadButton_Up) state.pickerValue = std::max(minValue, state.pickerValue - 1);
    if (down & HidNpadButton_Down) state.pickerValue = std::min(maxValue, state.pickerValue + 1);
    if (down & HidNpadButton_L) state.pickerValue = std::max(minValue, state.pickerValue - 10);
    if (down & HidNpadButton_R) state.pickerValue = std::min(maxValue, state.pickerValue + 10);
    if (down & HidNpadButton_B) {
        state.picker = UX2Picker::None;
        return;
    }
    if (!(down & HidNpadButton_A)) return;
    if (state.picker == UX2Picker::Species) {
        ux2ApplySpeciesPicker(screen);
        return;
    }
    state.tempMove = static_cast<uint8_t>(state.pickerValue);
    state.tempPPUps = 0;
    state.tempPP = Editor::moveBasePP(state.tempMove);
    state.picker = UX2Picker::None;
}

void ux2HandleMainEditor(TrainerViewScreen& screen, uint64_t down) {
    auto& state = ux2StateFor(screen);
    const bool add = state.mode == UX2Mode::AddDraft;
    constexpr int rows = static_cast<int>(UX::logicalWorkspaceRowCount());
    if (down & HidNpadButton_Up) state.row = (state.row + rows - 1) % rows;
    if (down & HidNpadButton_Down) state.row = (state.row + 1) % rows;
    if (down & HidNpadButton_L) state.row = ux2JumpSectionRow(state.row, -1);
    if (down & HidNpadButton_R) state.row = ux2JumpSectionRow(state.row, 1);
    if (add && (down & HidNpadButton_X)) { ux2StageAdd(screen); return; }
    if (down & HidNpadButton_B) {
        if (add) state.draft = BoxPokemonCreate{};
        state.mode = UX2Mode::Actions;
        state.row = 0;
        return;
    }
    if (!(down & HidNpadButton_A)) return;

    auto* e = editor(screen);
    if (!e) return;
    std::string error;
    const auto pokemon = add ? std::optional<PokemonRecord>{} : ux2SelectedPokemon(screen, error);
    if (!add && !pokemon) { screen.postStatus(error.empty() ? "Pokemon unavailable" : error, 300); return; }

    switch (state.row) {
        case 0:
            ux2OpenSpeciesPicker(state, add ? state.draft.species : pokemon->species);
            break;
        case 1: {
            const std::string initial = add
                ? (state.draft.nickname.empty() ? std::string(Names::getSpeciesName(state.draft.species)) : state.draft.nickname)
                : pokemon->nickname;
            const auto result = Utils::promptText("Generation I Nickname", "1-10 supported Gen I characters", initial, 10);
            if (!result.accepted) break;
            if (add) state.draft.nickname = result.text;
            else { BoxPokemonEdit edit; edit.nickname = result.text; ux2CommitEdit(screen, edit, "Nickname change staged; source .srm unchanged"); }
            break;
        }
        case 2: ux2OpenLevelExpEditor(screen, add); break;
        case 3: case 4: case 5: case 6: ux2OpenMoveEditor(screen, state.row - 3, add); break;
        case 7: ux2OpenDVEditor(screen, add); break;
        case 8: ux2OpenStatExpEditor(screen, add); break;
        case 9: {
            const std::string initial = add
                ? (state.draft.otName.empty() ? e->view().trainer().name : state.draft.otName)
                : pokemon->originalTrainer;
            const auto result = Utils::promptText("Generation I OT", "1-7 supported Gen I characters", initial, 7);
            if (!result.accepted) break;
            if (add) state.draft.otName = result.text;
            else { BoxPokemonEdit edit; edit.otName = result.text; ux2CommitEdit(screen, edit, "OT change staged; source .srm unchanged"); }
            break;
        }
        case 10: {
            const int current = add ? state.draft.trainerId.value_or(e->view().trainer().trainerId) : pokemon->trainerId;
            const auto result = Utils::promptNumber("Generation I Trainer ID", current, 0, 65535);
            if (!result.accepted) break;
            if (add) state.draft.trainerId = static_cast<uint16_t>(result.value);
            else { BoxPokemonEdit edit; edit.trainerId = static_cast<uint16_t>(result.value); ux2CommitEdit(screen, edit, "TID change staged; source .srm unchanged"); }
            break;
        }
        case 11:
            if (add) ux2StageAdd(screen);
            else { state.mode = UX2Mode::Review; state.reviewRow = 0; }
            break;
        case 12:
            if (add) state.draft = BoxPokemonCreate{};
            state.mode = UX2Mode::Actions;
            state.row = 0;
            break;
    }
}

void ux2HandleSubEditor(TrainerViewScreen& screen, uint64_t down) {
    auto& state = ux2StateFor(screen);
    int rows = 0;
    switch (state.mode) {
        case UX2Mode::LevelExpEditor: rows = 4; break;
        case UX2Mode::MoveEditor: rows = 5; break;
        case UX2Mode::DVEditor: rows = 7; break;
        case UX2Mode::StatExpEditor: rows = 7; break;
        default: return;
    }
    if (down & HidNpadButton_Up) state.subRow = (state.subRow + rows - 1) % rows;
    if (down & HidNpadButton_Down) state.subRow = (state.subRow + 1) % rows;
    if (down & HidNpadButton_B) { ux2ReturnFromSubEditor(state); return; }
    if (!(down & HidNpadButton_A)) return;

    if (state.mode == UX2Mode::LevelExpEditor) {
        if (state.subRow == 0) {
            const auto result = Utils::promptNumber("Generation I Level", state.tempLevel, 1, 100);
            if (!result.accepted) return;
            state.tempLevel = static_cast<uint8_t>(result.value);
            uint16_t species = state.draft.species;
            if (!state.subForAdd) { std::string error; const auto p = ux2SelectedPokemon(screen, error); if (p) species = p->species; }
            state.tempExperience = Pokemon::getExpForLevel(state.tempLevel, Editor::growthRate(species));
        } else if (state.subRow == 1) {
            if (state.subForAdd) {
                screen.postStatus("Add-draft EXP is generation-correct and derived from Species + Level", 260);
                return;
            }
            std::string error;
            const auto p = ux2SelectedPokemon(screen, error);
            if (!p) return;
            const int maxExp = static_cast<int>(Pokemon::getExpForLevel(100, Editor::growthRate(p->species)));
            const auto result = Utils::promptNumber("Generation I Experience", static_cast<int>(state.tempExperience), 0, maxExp);
            if (!result.accepted) return;
            state.tempExperience = static_cast<uint32_t>(result.value);
            state.tempLevel = Pokemon::getLevelFromExp(state.tempExperience, Editor::growthRate(p->species));
        } else if (state.subRow == 2) ux2ApplyLevelExp(screen);
        else ux2ReturnFromSubEditor(state);
        return;
    }

    if (state.mode == UX2Mode::MoveEditor) {
        if (state.subRow == 0) ux2OpenMovePicker(state);
        else if (state.subRow == 1) {
            const int maxPP = Editor::moveMaxPP(state.tempMove, state.tempPPUps);
            const auto result = Utils::promptNumber("Generation I Move PP", state.tempPP, 0, maxPP);
            if (result.accepted) state.tempPP = static_cast<uint8_t>(result.value);
        } else if (state.subRow == 2) {
            const auto result = Utils::promptNumber("Generation I PP Ups", state.tempPPUps, 0, 3);
            if (result.accepted) {
                state.tempPPUps = static_cast<uint8_t>(result.value);
                state.tempPP = std::min<uint8_t>(state.tempPP, Editor::moveMaxPP(state.tempMove, state.tempPPUps));
            }
        } else if (state.subRow == 3) ux2ApplyMove(screen);
        else ux2ReturnFromSubEditor(state);
        return;
    }

    if (state.mode == UX2Mode::DVEditor) {
        if (state.subRow < 4) {
            const auto result = Utils::promptNumber("Generation I DV (0-15)", state.tempDVs[static_cast<size_t>(state.subRow)], 0, 15);
            if (result.accepted) state.tempDVs[static_cast<size_t>(state.subRow)] = static_cast<uint8_t>(result.value);
        } else if (state.subRow == 4) {
            screen.postStatus("HP DV is derived from Attack/Defense/Speed/Special low bits", 260);
        } else if (state.subRow == 5) ux2ApplyDVs(screen);
        else ux2ReturnFromSubEditor(state);
        return;
    }

    if (state.mode == UX2Mode::StatExpEditor) {
        if (state.subRow < 5) {
            const auto i = static_cast<size_t>(state.subRow);
            const auto result = Utils::promptNumber("Generation I Stat Experience", state.tempStatExperience[i], 0, 65535);
            if (result.accepted) state.tempStatExperience[i] = static_cast<uint16_t>(result.value);
        } else if (state.subRow == 5) ux2ApplyStatExp(screen);
        else ux2ReturnFromSubEditor(state);
    }
}

void ux2HandleActiveInput(TrainerViewScreen& screen, uint64_t down) {
    auto& state = ux2StateFor(screen);
    auto* e = editor(screen);
    if (!e) { ux2Close(state); return; }
    if (state.picker != UX2Picker::None) { ux2HandlePicker(screen, down); return; }

    if (state.mode == UX2Mode::Actions) {
        std::string error;
        const auto pokemon = ux2SelectedPokemon(screen, error);
        const auto actions = UX::actionsForSlot(static_cast<bool>(pokemon));
        const int rows = static_cast<int>(actions.count);
        if (down & HidNpadButton_Up) state.row = (state.row + rows - 1) % rows;
        if (down & HidNpadButton_Down) state.row = (state.row + 1) % rows;
        if (down & HidNpadButton_B) { ux2Close(state); return; }
        if (!(down & HidNpadButton_A)) return;
        switch (actions[static_cast<size_t>(state.row)]) {
            case UX::Action::View: state.mode = UX2Mode::View; break;
            case UX::Action::Edit: state.mode = UX2Mode::Edit; state.row = 0; break;
            case UX::Action::Clone: ux2OpenClone(screen); break;
            case UX::Action::Remove: state.mode = UX2Mode::RemoveConfirm; break;
            case UX::Action::LegalityProvenance: state.mode = UX2Mode::Provenance; break;
            case UX::Action::AddPokemon: ux2BeginAdd(screen); break;
            case UX::Action::ReviewPendingChanges: state.mode = UX2Mode::Review; state.reviewRow = 0; break;
            case UX::Action::Cancel: ux2Close(state); break;
        }
        return;
    }

    if (state.mode == UX2Mode::View || state.mode == UX2Mode::Provenance) {
        if (down & (HidNpadButton_A | HidNpadButton_B)) { state.mode = UX2Mode::Actions; state.row = 0; }
        return;
    }

    if (state.mode == UX2Mode::Edit || state.mode == UX2Mode::AddDraft) {
        ux2HandleMainEditor(screen, down);
        return;
    }

    if (state.mode == UX2Mode::LevelExpEditor || state.mode == UX2Mode::MoveEditor ||
        state.mode == UX2Mode::DVEditor || state.mode == UX2Mode::StatExpEditor) {
        ux2HandleSubEditor(screen, down);
        return;
    }

    if (state.mode == UX2Mode::CloneDestination) {
        const int boxes = ux2BoxCount(*e);
        const int slots = ux2SlotCount(*e, state.cloneBox);
        constexpr int cols = 5;
        if (down & HidNpadButton_L) {
            state.cloneBox = (state.cloneBox + boxes - 1) % boxes;
            const int first = ux2FirstEmpty(*e, state.cloneBox); state.cloneSlot = first >= 0 ? first : 0;
        }
        if (down & HidNpadButton_R) {
            state.cloneBox = (state.cloneBox + 1) % boxes;
            const int first = ux2FirstEmpty(*e, state.cloneBox); state.cloneSlot = first >= 0 ? first : 0;
        }
        if (slots > 0) {
            if (down & HidNpadButton_Left) state.cloneSlot = (state.cloneSlot + slots - 1) % slots;
            if (down & HidNpadButton_Right) state.cloneSlot = (state.cloneSlot + 1) % slots;
            if (down & HidNpadButton_Up) state.cloneSlot = (state.cloneSlot - cols + slots) % slots;
            if (down & HidNpadButton_Down) state.cloneSlot = (state.cloneSlot + cols) % slots;
        }
        if (down & HidNpadButton_B) { state.mode = UX2Mode::Actions; state.row = 0; return; }
        if (down & HidNpadButton_A) ux2StageClone(screen);
        return;
    }

    if (state.mode == UX2Mode::RemoveConfirm) {
        if (down & HidNpadButton_B) { state.mode = UX2Mode::Actions; state.row = 0; return; }
        if (down & HidNpadButton_A) ux2StageRemove(screen);
        return;
    }

    if (state.mode == UX2Mode::Review) {
        const auto lines = pendingLines(screen);
        const int count = static_cast<int>(lines.size());
        if (count > 0 && (down & HidNpadButton_Up)) state.reviewRow = (state.reviewRow + count - 1) % count;
        if (count > 0 && (down & HidNpadButton_Down)) state.reviewRow = (state.reviewRow + 1) % count;
        if (down & HidNpadButton_B) { state.mode = UX2Mode::Actions; state.row = 0; return; }
        if (down & HidNpadButton_Y) {
            discardAll(screen);
            screen.postStatus("All Generation I staged changes discarded; original .srm was never modified", 300);
            state.reviewRow = 0;
            return;
        }
        if (down & HidNpadButton_A) {
            std::string error;
            const auto path = exportStagedCopy(screen, error);
            if (path.empty()) screen.postStatus(error.empty() ? "Generation I export failed" : error, 360);
            else { state.lastExportDirectory = path; screen.postStatus("Edited copy exported; original .srm unchanged", 360); }
        }
    }
}

void ux2DrawPanel(PKSEFramebuffer& fb, int x, int y, int w, int h) {
    fb.drawSoftShadow(x, y, w, h, 18);
    fb.drawFilledRoundedRect(x, y, w, h, 18, Colors::Panel);
    fb.drawRoundedRect(x, y, w, h, 18, Colors::Divider, 1);
}

void ux2DrawLogicalRow(PKSEFramebuffer& fb, int x, int y, int w,
                       const std::string& label, const std::string& value, bool selected) {
    if (selected) fb.drawSelectionHighlight(x, y, w, 44);
    fb.drawText(x + 14, y + 12, label, selected ? Colors::Text : Colors::TextDim, TextStyle::Caption);
    int valueWidth = 0, valueHeight = 0;
    fb.measureText(value, valueWidth, valueHeight, TextStyle::Body);
    const int valueX = std::max(x + 190, x + w - 14 - valueWidth);
    fb.drawText(valueX, y + 10, value, Colors::Text, TextStyle::Body);
}

std::array<uint16_t,5> ux2StatsForDisplay(TrainerViewScreen& screen, uint16_t species,
                                          const PokemonRecord* pokemon, const BoxPokemonCreate* draft) {
    if (draft)
        return Editor::calculateBattleStats(species, draft->level, draft->dvs, draft->statExperience).asArray();
    if (pokemon)
        return Editor::calculateBattleStats(species, pokemon->level, storedDVs(*pokemon), pokemon->statExperience).asArray();
    return {};
}

void ux2DrawRadar(PKSEFramebuffer& fb, int cx, int cy, int radius, const std::array<uint16_t,5>& stats) {
    const auto model = PokeBank::UIModel::gen1RadarModel(stats);
    StatsRadar::draw(fb, cx, cy, radius, model.normalized, 1.0f,
                     Colors::Divider, Colors::FocusBorder);
}

void ux2DrawPreview(TrainerViewScreen& screen, PKSEFramebuffer& fb, int x, int y, int w,
                    const PokemonRecord* pokemon, const BoxPokemonCreate* draft) {
    auto* e = editor(screen);
    if (!e) return;
    const uint16_t committed = draft ? draft->species : (pokemon ? pokemon->species : 0);
    const uint16_t species = ux2PreviewSpecies(screen) ? ux2PreviewSpecies(screen) : committed;
    const uint8_t level = draft ? draft->level : (pokemon ? pokemon->level : 0);
    const std::string nickname = draft
        ? (draft->nickname.empty() ? std::string(Names::getSpeciesName(committed)) : draft->nickname)
        : (pokemon ? pokemon->nickname : std::string{});
    const uint32_t exp = draft ? draftExperience(*draft) : (pokemon ? pokemon->experience : 0);
    const auto stats = ux2StatsForDisplay(screen, species, pokemon, draft);

    fb.drawText(x, y, draft ? "LIVE DRAFT" : "STAGED POKEMON", Colors::Accent, TextStyle::Caption);
    if (species >= 1 && species <= 151) {
        if (auto* sprite = SpriteManager::getSprite(species, false); sprite && sprite->data)
            fb.drawSpriteStaticContained(x, y + 22, w, 142, sprite->width, sprite->height, sprite->data, sprite->channels);
        else fb.drawText(x + 28, y + 86, "Sprite unavailable", Colors::TextDim);
    }
    fb.drawText(x, y + 168, speciesCompact(species), Colors::Text, TextStyle::Heading);
    fb.drawText(x, y + 200, nickname, Colors::Text);
    fb.drawText(x, y + 228, "Lv " + std::to_string(level) + "   EXP " + std::to_string(exp), Colors::TextDim, TextStyle::Caption);
    fb.drawText(x, y + 260, "GEN I BATTLE STATS", Colors::Accent, TextStyle::Caption);
    ux2DrawRadar(fb, x + w / 2, y + 344, 68, stats);
    fb.drawText(x, y + 420,
        "HP " + std::to_string(stats[0]) + "  Atk " + std::to_string(stats[1]) + "  Def " + std::to_string(stats[2]),
        Colors::Text, TextStyle::Caption);
    fb.drawText(x, y + 444,
        "Spe " + std::to_string(stats[3]) + "  Special " + std::to_string(stats[4]),
        Colors::Text, TextStyle::Caption);
    if (draft)
        fb.drawText(x, y + 476, "Preview only — nothing staged until Stage Add", Colors::Accent, TextStyle::Caption);
    else
        fb.drawText(x, y + 476, "Source .srm remains immutable", Colors::Accent, TextStyle::Caption);
}

std::string ux2MainRowLabel(int row) {
    static constexpr const char* labels[] = {
        "Species", "Nickname", "Level / EXP",
        "Move 1", "Move 2", "Move 3", "Move 4",
        "DVs", "Stat Exp", "OT", "Trainer ID",
        "Review / Stage Add", "Back / Cancel",
    };
    return row >= 0 && row < 13 ? labels[row] : "Field";
}

std::string ux2MoveSummary(uint8_t move, uint8_t pp, uint8_t ups) {
    const int maxPP = Editor::moveMaxPP(move, ups);
    return moveDisplay(move) + "  •  PP " + std::to_string(pp) + "/" + std::to_string(maxPP) +
           "  •  Ups " + std::to_string(ups);
}

std::string ux2MainRowValue(TrainerViewScreen& screen, int row, bool add, const PokemonRecord* p) {
    auto& state = ux2StateFor(screen);
    auto* e = editor(screen);
    if (!e) return {};
    const auto& d = state.draft;
    if (row == 0) return speciesCompact(add ? d.species : p->species);
    if (row == 1) return add ? (d.nickname.empty() ? std::string(Names::getSpeciesName(d.species)) + " (default)" : d.nickname) : p->nickname;
    if (row == 2) return add ? ("Lv " + std::to_string(d.level) + "  •  EXP " + std::to_string(draftExperience(d)))
                              : ("Lv " + std::to_string(p->level) + "  •  EXP " + std::to_string(p->experience));
    if (row >= 3 && row <= 6) {
        const auto i = static_cast<size_t>(row - 3);
        return add ? ux2MoveSummary(d.moves[i], d.pp[i], d.ppUps[i])
                   : ux2MoveSummary(p->moves[i], p->pp[i], p->ppUps[i]);
    }
    if (row == 7) {
        const auto dvs = add ? d.dvs : storedDVs(*p);
        return "HP " + std::to_string(Editor::derivedHPDV(dvs)) + " • A " + std::to_string(dvs[0]) +
            " D " + std::to_string(dvs[1]) + " S " + std::to_string(dvs[2]) + " Spc " + std::to_string(dvs[3]);
    }
    if (row == 8) {
        const auto& stats = add ? d.statExperience : p->statExperience;
        return "HP " + std::to_string(stats[0]) + " • A " + std::to_string(stats[1]) +
            " D " + std::to_string(stats[2]) + " S " + std::to_string(stats[3]) + " Spc " + std::to_string(stats[4]);
    }
    if (row == 9) return add ? (d.otName.empty() ? e->view().trainer().name + " (default)" : d.otName) : p->originalTrainer;
    if (row == 10) return std::to_string(add ? d.trainerId.value_or(e->view().trainer().trainerId) : p->trainerId);
    if (row == 11) return add ? "Stage this complete draft" : "Open semantic Pending Changes";
    if (row == 12) return add ? "Discard draft — no staged mutation" : "Back to Actions";
    return {};
}

void ux2DrawEditor(TrainerViewScreen& screen, PKSEFramebuffer& fb, int x, int y, int w, int h, bool add) {
    auto& state = ux2StateFor(screen);
    std::string error;
    const auto pokemon = add ? std::optional<PokemonRecord>{} : ux2SelectedPokemon(screen, error);
    if (!add && !pokemon) { fb.drawText(x + 30, y + 80, error.empty() ? "Pokemon unavailable" : error, Colors::Accent); return; }

    fb.drawText(x + 26, y + 18, add ? "Add Pokemon — DRAFT" : "Edit Pokemon — STAGED", Colors::Text, TextStyle::Heading);
    fb.drawText(x + 26, y + 51,
        "Box " + std::to_string(state.box + 1) + " / Slot " + std::to_string(state.slot + 1) +
        "   |   " + ux2SectionForRow(state.row), Colors::TextDim, TextStyle::Caption);

    const int leftX = x + 28;
    const int leftW = 278;
    ux2DrawPreview(screen, fb, leftX, y + 84, leftW, pokemon ? &*pokemon : nullptr, add ? &state.draft : nullptr);

    const int listX = x + 338;
    const int listW = w - 370;
    const int listY = y + 92;
    const int viewportH = h - 184;
    constexpr int rowH = 48;
    const int visible = std::max(1, viewportH / rowH);
    const int start = std::clamp(state.row - visible / 2, 0, std::max(0, 13 - visible));

    fb.setClipRect(listX, listY, listW, viewportH);
    for (int i = 0; i < visible && start + i < 13; ++i) {
        const int field = start + i;
        ux2DrawLogicalRow(fb, listX, listY + i * rowH, listW,
            ux2MainRowLabel(field), ux2MainRowValue(screen, field, add, pokemon ? &*pokemon : nullptr), field == state.row);
    }
    fb.clearClip();

    const int statusY = y + h - 66;
    fb.drawHDivider(listX, statusY - 10, listW);
    fb.drawText(listX, statusY,
        "Row " + std::to_string(state.row + 1) + "/13   •   " + ux2SectionForRow(state.row),
        Colors::TextDim, TextStyle::Caption);
}

void ux2DrawActions(TrainerViewScreen& screen, PKSEFramebuffer& fb, int x, int y, int w, int h) {
    auto& state = ux2StateFor(screen);
    std::string error;
    const auto pokemon = ux2SelectedPokemon(screen, error);
    const auto actions = UX::actionsForSlot(static_cast<bool>(pokemon));
    fb.drawText(x + 24, y + 18, pokemon ? "Pokemon Actions" : "Empty Slot", Colors::Text, TextStyle::Heading);
    fb.drawText(x + 24, y + 52,
        "Box " + std::to_string(state.box + 1) + " / Slot " + std::to_string(state.slot + 1) + "  •  staged copy only",
        Colors::TextDim, TextStyle::Caption);
    static constexpr const char* names[] = {
        "View", "Edit", "Clone", "Remove", "Legality & Provenance",
        "Add Pokemon", "Review Pending Changes", "Cancel",
    };
    int rowY = y + 86;
    for (std::size_t i = 0; i < actions.count; ++i) {
        const int idx = static_cast<int>(actions[i]);
        const bool selected = static_cast<int>(i) == state.row;
        if (selected) fb.drawSelectionHighlight(x + 22, rowY, w - 44, 42);
        fb.drawText(x + 38, rowY + 10, names[idx], selected ? Colors::Text : Colors::TextDim);
        rowY += 46;
    }
    fb.drawText(x + 24, y + h - 38, "Original source is immutable", Colors::Accent, TextStyle::Caption);
}

void ux2DrawFiveStatLabels(PKSEFramebuffer& fb, int x, int y, const std::array<uint16_t,5>& stats) {
    fb.drawText(x, y, "HP " + std::to_string(stats[0]) + "   Attack " + std::to_string(stats[1]) +
        "   Defense " + std::to_string(stats[2]), Colors::Text, TextStyle::Caption);
    fb.drawText(x, y + 26, "Speed " + std::to_string(stats[3]) + "   Special " + std::to_string(stats[4]),
        Colors::Text, TextStyle::Caption);
}

void ux2DrawView(TrainerViewScreen& screen, PKSEFramebuffer& fb, int x, int y, int w, int h) {
    std::string error;
    const auto p = ux2SelectedPokemon(screen, error);
    if (!p) { fb.drawText(x + 30, y + 90, error.empty() ? "Pokemon unavailable" : error, Colors::Accent); return; }
    const auto stats = Editor::calculateBattleStats(p->species, p->level, storedDVs(*p), p->statExperience).asArray();
    fb.drawText(x + 26, y + 18, "Generation I Pokemon Summary", Colors::Text, TextStyle::Heading);
    fb.drawText(x + 26, y + 52, "Five-stat R/B/Y summary — Special is one stat", Colors::TextDim, TextStyle::Caption);

    const int lx = x + 36;
    if (auto* sprite = SpriteManager::getSprite(p->species, false); sprite && sprite->data)
        fb.drawSpriteStaticContained(lx, y + 90, 270, 230, sprite->width, sprite->height, sprite->data, sprite->channels);
    fb.drawText(lx, y + 324, speciesCompact(p->species), Colors::Text, TextStyle::Heading);
    fb.drawText(lx, y + 362, p->nickname + "   Lv " + std::to_string(p->level), Colors::Text);
    fb.drawText(lx, y + 396, "OT " + p->originalTrainer + "   TID " + std::to_string(p->trainerId), Colors::TextDim);

    const int radarCx = x + 555;
    const int radarCy = y + 245;
    ux2DrawRadar(fb, radarCx, radarCy, 120, stats);
    fb.drawText(radarCx - 14, radarCy - 144, "HP", Colors::Text, TextStyle::Caption);
    fb.drawText(radarCx + 118, radarCy - 44, "Attack", Colors::Text, TextStyle::Caption);
    fb.drawText(radarCx + 88, radarCy + 104, "Defense", Colors::Text, TextStyle::Caption);
    fb.drawText(radarCx - 148, radarCy + 104, "Speed", Colors::Text, TextStyle::Caption);
    fb.drawText(radarCx - 164, radarCy - 44, "Special", Colors::Text, TextStyle::Caption);
    ux2DrawFiveStatLabels(fb, x + 395, y + 392, stats);

    const int rx = x + 790;
    fb.drawText(rx, y + 90, "MOVES", Colors::Accent, TextStyle::Caption);
    for (int i = 0; i < 4; ++i)
        fb.drawText(rx, y + 124 + i * 48,
            std::to_string(i + 1) + ". " + moveDisplay(p->moves[static_cast<size_t>(i)]), Colors::Text);
    fb.drawText(rx, y + 324, "DVs", Colors::Accent, TextStyle::Caption);
    fb.drawText(rx, y + 352,
        "HP " + std::to_string(p->dvs[0]) + "  A " + std::to_string(p->dvs[1]) + "  D " + std::to_string(p->dvs[2]),
        Colors::Text, TextStyle::Caption);
    fb.drawText(rx, y + 378,
        "S " + std::to_string(p->dvs[3]) + "  Special " + std::to_string(p->dvs[4]), Colors::Text, TextStyle::Caption);
}

void ux2DrawSubEditor(TrainerViewScreen& screen, PKSEFramebuffer& fb, int x, int y, int w, int h) {
    auto& state = ux2StateFor(screen);
    std::vector<std::pair<std::string,std::string>> rows;
    std::string title;
    if (state.mode == UX2Mode::LevelExpEditor) {
        title = "Level / Experience";
        rows = {{"Level", std::to_string(state.tempLevel)},
                {"Experience", std::to_string(state.tempExperience) + (state.subForAdd ? " (derived)" : "")},
                {"Apply", "Commit to current staged/draft editor"}, {"Cancel", "Discard this sub-editor"}};
    } else if (state.mode == UX2Mode::MoveEditor) {
        title = "Move " + std::to_string(state.moveSlot + 1);
        rows = {{"Move", moveDisplay(state.tempMove)}, {"PP", std::to_string(state.tempPP) + "/" + std::to_string(Editor::moveMaxPP(state.tempMove, state.tempPPUps))},
                {"PP Ups", std::to_string(state.tempPPUps)}, {"Apply", "Keep these move details"}, {"Cancel", "Discard these move details"}};
    } else if (state.mode == UX2Mode::DVEditor) {
        title = "Generation I DVs";
        rows = {{"Attack", std::to_string(state.tempDVs[0])}, {"Defense", std::to_string(state.tempDVs[1])},
                {"Speed", std::to_string(state.tempDVs[2])}, {"Special", std::to_string(state.tempDVs[3])},
                {"HP DV", std::to_string(Editor::derivedHPDV(state.tempDVs)) + " (derived / read-only)"},
                {"Apply", "Keep these DVs"}, {"Cancel", "Discard these DV edits"}};
    } else {
        title = "Generation I Stat Exp";
        static constexpr const char* names[] = {"HP", "Attack", "Defense", "Speed", "Special"};
        for (int i = 0; i < 5; ++i) rows.push_back({names[i], std::to_string(state.tempStatExperience[static_cast<size_t>(i)])});
        rows.push_back({"Apply", "Keep Stat Exp edits"}); rows.push_back({"Cancel", "Discard Stat Exp edits"});
    }
    fb.drawText(x + 24, y + 18, title, Colors::Text, TextStyle::Heading);
    fb.drawText(x + 24, y + 52, "Contextual editor — B always cancels this dialog", Colors::TextDim, TextStyle::Caption);
    int rowY = y + 88;
    for (int i = 0; i < static_cast<int>(rows.size()); ++i) {
        ux2DrawLogicalRow(fb, x + 22, rowY, w - 44, rows[static_cast<size_t>(i)].first,
                          rows[static_cast<size_t>(i)].second, i == state.subRow);
        rowY += 48;
    }
}

void ux2DrawPicker(TrainerViewScreen& screen, PKSEFramebuffer& fb, int x, int y, int w, int h) {
    auto& state = ux2StateFor(screen);
    fb.drawFilledRect(0, 0, fb.getWidth(), fb.getHeight() - 48, Color(0, 0, 0, 95));
    ux2DrawPanel(fb, x, y, w, h);
    const bool species = state.picker == UX2Picker::Species;
    fb.drawText(x + 24, y + 18, species ? "Choose Species" : "Choose Move", Colors::Text, TextStyle::Heading);
    fb.drawText(x + 24, y + 52,
        species ? "Highlight previews immediately • A Select • B restores committed species" : "A Select • B Cancel",
        Colors::TextDim, TextStyle::Caption);
    const int minValue = species ? 1 : 0;
    const int maxValue = species ? 151 : 165;
    constexpr int visible = 9;
    const int start = std::clamp(state.pickerValue - visible / 2, minValue, std::max(minValue, maxValue - visible + 1));
    const int listW = species ? w - 280 : w - 48;
    for (int i = 0; i < visible; ++i) {
        const int value = start + i;
        if (value > maxValue) break;
        const int rowY = y + 86 + i * 42;
        const bool selected = value == state.pickerValue;
        if (selected) fb.drawSelectionHighlight(x + 24, rowY, listW - 24, 38);
        std::string text;
        if (species) text = UX::speciesPickerRow(static_cast<uint16_t>(value), Names::getSpeciesName(value));
        else if (value == 0) text = "000 - Empty move slot";
        else { char prefix[16]{}; std::snprintf(prefix, sizeof(prefix), "%03d - ", value); text = std::string(prefix) + Names::getMoveName(value); }
        fb.drawText(x + 40, rowY + 8, text, Colors::Text);
    }
    if (species) {
        const uint16_t preview = static_cast<uint16_t>(state.pickerValue);
        const int sx = x + w - 238;
        fb.drawText(sx, y + 92, "HOVER PREVIEW", Colors::Accent, TextStyle::Caption);
        if (auto* sprite = SpriteManager::getSprite(preview, false); sprite && sprite->data)
            fb.drawSpriteStaticContained(sx, y + 124, 200, 210, sprite->width, sprite->height, sprite->data, sprite->channels);
        fb.drawText(sx, y + 348, speciesCompact(preview), Colors::Text, TextStyle::Caption);
        fb.drawText(sx, y + 378, "Preview only", Colors::TextDim, TextStyle::Caption);
    }
}

void ux2DrawClone(TrainerViewScreen& screen, PKSEFramebuffer& fb, int x, int y, int w, int h) {
    auto& state = ux2StateFor(screen);
    auto* e = editor(screen);
    if (!e) return;
    std::string error;
    const auto source = ux2SelectedPokemon(screen, error);
    fb.drawText(x + 24, y + 18, "Stage Clone — Choose Destination", Colors::Text, TextStyle::Heading);
    fb.drawText(x + 24, y + 52,
        "Source: " + (source ? source->nickname : std::string("Pokemon")) + " — Box " + std::to_string(state.box + 1) +
        " / Slot " + std::to_string(state.slot + 1), Colors::TextDim, TextStyle::Caption);
    fb.drawText(x + 24, y + 82, "Current Save   •   < Box " + std::to_string(state.cloneBox + 1) + " >",
        Colors::Accent, TextStyle::Caption);

    const int slots = ux2SlotCount(*e, state.cloneBox);
    const int writable = ux2FirstEmpty(*e, state.cloneBox);
    constexpr int cols = 5;
    const int cellW = (w - 70) / cols;
    const int cellH = 82;
    for (int i = 0; i < slots; ++i) {
        const int col = i % cols, row = i / cols;
        const int cx = x + 24 + col * cellW;
        const int cy = y + 118 + row * cellH;
        const auto p = e->boxedPokemon(static_cast<size_t>(state.cloneBox), static_cast<size_t>(i), error);
        const bool selected = i == state.cloneSlot;
        if (selected) fb.drawSelectionHighlight(cx, cy, cellW - 8, cellH - 8);
        fb.drawRoundedRect(cx, cy, cellW - 8, cellH - 8, 8, i == writable ? Colors::Accent : Colors::Divider, 1);
        fb.drawText(cx + 8, cy + 8, (i < 9 ? "0" : "") + std::to_string(i + 1), Colors::TextDim, TextStyle::Caption);
        if (p) {
            fb.drawText(cx + 8, cy + 34, Names::getSpeciesName(p->species), Colors::Text, TextStyle::Caption);
        } else if (i == writable) {
            fb.drawText(cx + 8, cy + 34, "Empty • writable", Colors::Accent, TextStyle::Caption);
        } else {
            fb.drawText(cx + 8, cy + 34, "Empty", Colors::TextDim, TextStyle::Caption);
        }
    }
    fb.drawText(x + 24, y + h - 44,
        "Gen I box records are contiguous: the outlined next empty slot is the only structurally writable destination.",
        Colors::TextDim, TextStyle::Caption);
}

void ux2DrawReview(TrainerViewScreen& screen, PKSEFramebuffer& fb, int x, int y, int w, int h) {
    auto& state = ux2StateFor(screen);
    const auto lines = pendingLines(screen);
    fb.drawText(x + 24, y + 18, "Generation I Pending Changes", Colors::Text, TextStyle::Heading);
    if (lines.empty()) fb.drawText(x + 28, y + 94, "No staged changes.", Colors::TextDim);
    else {
        const int visible = std::max(1, (h - 150) / 48);
        const int start = std::clamp(state.reviewRow - visible / 2, 0, std::max(0, static_cast<int>(lines.size()) - visible));
        fb.setClipRect(x + 24, y + 72, w - 48, h - 130);
        for (int i = 0; i < visible && start + i < static_cast<int>(lines.size()); ++i)
            ux2DrawLogicalRow(fb, x + 24, y + 76 + i * 48, w - 48, std::to_string(start + i + 1),
                              lines[static_cast<size_t>(start + i)], start + i == state.reviewRow);
        fb.clearClip();
    }
    if (!state.lastExportDirectory.empty())
        fb.drawText(x + 28, y + h - 42, "Last export: " + state.lastExportDirectory, Colors::TextDim, TextStyle::Caption);
}

void ux2DrawProvenance(TrainerViewScreen& screen, PKSEFramebuffer& fb, int x, int y, int w, int h) {
    auto* trainer = rbyTrainer(screen);
    auto* e = editor(screen);
    fb.drawText(x + 24, y + 18, "Generation I Legality & Provenance", Colors::Text, TextStyle::Heading);
    const std::array<std::pair<const char*,std::string>,8> rows{{
        {"Source game", screen.sourceGameId},
        {"Region/layout", trainer && trainer->japaneseLayout() ? "Japanese — editing disabled" : "International — validated editor"},
        {"Source mutation", "HARD DISABLED"},
        {"Party mutation", "DEFERRED"},
        {"Format validation", "Strict reparse + checksum on export"},
        {"Encounter legality", "Not checked yet"},
        {"Original bytes", e ? std::to_string(e->originalBytes().size()) + " immutable bytes" : "Unavailable"},
        {"Pending semantic changes", std::to_string(pendingLines(screen).size())},
    }};
    const int lx = x + 34, vx = x + 300;
    for (int i = 0; i < static_cast<int>(rows.size()); ++i) {
        fb.drawText(lx, y + 82 + i * 46, rows[static_cast<size_t>(i)].first, Colors::TextDim);
        fb.drawText(vx, y + 82 + i * 46, rows[static_cast<size_t>(i)].second,
                    i == 2 || i == 5 ? Colors::Accent : Colors::Text);
    }
    fb.drawText(lx, y + h - 42, "Structural validity is not encounter-legality proof.", Colors::TextDim, TextStyle::Caption);
}

void ux2DrawRemoveConfirm(PKSEFramebuffer& fb, int x, int y, int w, int h) {
    fb.drawText(x + 24, y + 18, "Stage Removal?", Colors::Text, TextStyle::Heading);
    fb.drawText(x + 28, y + 86, "This compacts the Gen I box in staged memory only.", Colors::Text);
    fb.drawText(x + 28, y + 126, "Original RetroArch .srm is never written.", Colors::Accent);
}

std::string ux2FooterText(const TrainerViewScreen& screen, const UX2State& state) {
    if (state.picker == UX2Picker::Species || state.picker == UX2Picker::Move)
        return "D-pad Navigate   L/R Page ±10   A Select   B Cancel";
    switch (state.mode) {
        case UX2Mode::Actions: return "D-pad Navigate   A Select   B Back";
        case UX2Mode::View: return "A/B Back";
        case UX2Mode::Edit: return "D-pad Scroll   A Edit   L/R Section   B Back";
        case UX2Mode::AddDraft: return "D-pad Scroll   A Edit   L/R Section   X Stage Add   B Cancel Draft";
        case UX2Mode::LevelExpEditor: return "D-pad Navigate   A Edit/Apply   B Cancel";
        case UX2Mode::MoveEditor: return "D-pad Navigate   A Edit/Apply   B Cancel";
        case UX2Mode::DVEditor: return "D-pad Navigate   A Edit/Apply   B Cancel";
        case UX2Mode::StatExpEditor: return "D-pad Navigate   A Edit/Apply   B Cancel";
        case UX2Mode::CloneDestination: return "D-pad Slot   L/R Box   A Stage Clone   B Cancel";
        case UX2Mode::RemoveConfirm: return "A Confirm Remove   B Cancel";
        case UX2Mode::Review: return "D-pad Navigate   A Export Copy   Y Discard Staged   B Back";
        case UX2Mode::Provenance: return "A/B Back";
        case UX2Mode::Closed: break;
    }
    if (entrySurface(screen)) {
        if (!editorAvailable(screen)) return "A View   L/R Box   B Back";
        std::string error;
        auto* mutableScreen = const_cast<TrainerViewScreen*>(&screen);
        const auto p = pokemonAt(*mutableScreen, screen.selectedBoxIndex, screen.selectedItemIndex, error);
        if (p) return "A Actions   Y Remove   L/R Box   B Back";
        return "A Actions   X Add   L/R Box   B Back";
    }
    return {};
}

void ux2DialogGeometry(TrainerViewScreen& screen, int& x, int& y, int& w, int& h) {
    auto& state = ux2StateFor(screen);
    switch (state.mode) {
        case UX2Mode::Actions: {
            std::string error; const auto p = ux2SelectedPokemon(screen, error);
            w = p ? 650 : 560; h = p ? 500 : 350; break;
        }
        case UX2Mode::RemoveConfirm: w = 620; h = 230; break;
        case UX2Mode::LevelExpEditor: w = 620; h = 330; break;
        case UX2Mode::MoveEditor: w = 650; h = 390; break;
        case UX2Mode::DVEditor: w = 650; h = 480; break;
        case UX2Mode::StatExpEditor: w = 650; h = 480; break;
        case UX2Mode::CloneDestination: w = 930; h = 540; break;
        case UX2Mode::Provenance: w = 850; h = 500; break;
        case UX2Mode::Review: w = 980; h = 540; break;
        case UX2Mode::View:
        case UX2Mode::Edit:
        case UX2Mode::AddDraft: w = 1120; h = 610; break;
        case UX2Mode::Closed: w = 0; h = 0; return;
    }
    x = (1280 - w) / 2;
    y = (672 - h) / 2;
}

} // namespace

bool isGen1SourceUX(const TrainerViewScreen& screen) noexcept {
    return isGen1SourceUXBase(screen);
}

bool handleInputUX(TrainerViewScreen& screen, uint64_t down) {
    if (!isGen1SourceUX(screen)) return false;
    auto& state = ux2StateFor(screen);
    if (state.mode != UX2Mode::Closed) {
        ux2HandleActiveInput(screen, down);
        return true;
    }
    if (!entrySurface(screen) || !editorAvailable(screen)) return false;
    if (!(down & (HidNpadButton_A | HidNpadButton_X | HidNpadButton_Y))) return false;

    state.box = screen.selectedBoxIndex;
    state.slot = screen.selectedItemIndex;
    state.row = 0;
    state.picker = UX2Picker::None;
    std::string error;
    const auto pokemon = ux2SelectedPokemon(screen, error);
    if (down & HidNpadButton_X) {
        if (pokemon) { screen.postStatus("X Add is available on an empty writable Gen I slot", 240); return true; }
        ux2BeginAdd(screen); return true;
    }
    if (down & HidNpadButton_Y) {
        if (!pokemon) { screen.postStatus("Y Remove is available only on an occupied Gen I slot", 240); return true; }
        state.mode = UX2Mode::RemoveConfirm; return true;
    }
    ux2OpenActions(screen);
    return true;
}

void drawOverlayUX(TrainerViewScreen& screen, PKSEFramebuffer& fb) {
    if (!isGen1SourceUX(screen)) return;
    auto& state = ux2StateFor(screen);
    if (state.mode == UX2Mode::Closed) {
        const auto text = ux2FooterText(screen, state);
        if (!text.empty()) drawFooter(fb, text);
        return;
    }

    screen.touchButtons.clear();
    int x = 0, y = 0, w = 0, h = 0;
    ux2DialogGeometry(screen, x, y, w, h);
    fb.drawFilledRect(0, 0, fb.getWidth(), fb.getHeight() - 48, Color(0, 0, 0, 105));
    ux2DrawPanel(fb, x, y, w, h);

    switch (state.mode) {
        case UX2Mode::Actions: ux2DrawActions(screen, fb, x, y, w, h); break;
        case UX2Mode::View: ux2DrawView(screen, fb, x, y, w, h); break;
        case UX2Mode::Edit: ux2DrawEditor(screen, fb, x, y, w, h, false); break;
        case UX2Mode::AddDraft: ux2DrawEditor(screen, fb, x, y, w, h, true); break;
        case UX2Mode::LevelExpEditor:
        case UX2Mode::MoveEditor:
        case UX2Mode::DVEditor:
        case UX2Mode::StatExpEditor: ux2DrawSubEditor(screen, fb, x, y, w, h); break;
        case UX2Mode::CloneDestination: ux2DrawClone(screen, fb, x, y, w, h); break;
        case UX2Mode::RemoveConfirm: ux2DrawRemoveConfirm(fb, x, y, w, h); break;
        case UX2Mode::Review: ux2DrawReview(screen, fb, x, y, w, h); break;
        case UX2Mode::Provenance: ux2DrawProvenance(screen, fb, x, y, w, h); break;
        case UX2Mode::Closed: break;
    }

    if (state.picker != UX2Picker::None) {
        const int pw = state.picker == UX2Picker::Species ? 820 : 650;
        const int ph = 520;
        ux2DrawPicker(screen, fb, (1280 - pw) / 2, (672 - ph) / 2, pw, ph);
    }
    drawFooter(fb, ux2FooterText(screen, state));
}

} // namespace UI::Gen1PokemonEditor
