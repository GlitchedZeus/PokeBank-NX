#include "UI/SharedPokemonShell.h"
#include "Pokemon/PersonalInfoTable.h"
#include "Encryption/Encryption3FRLG.h"
#include <cstring>
#include "Integration/Gen3/Gen3ExactFormatEditorProvider.h"
#include "Integration/Gen3/Gen3LearnsetTable.h"
#include "Integration/Encounter/EncounterGuardrails.h"
#include "Legacy/FRLGReadOnlyTrainer.h"
#include "Enums/Ball.h"
#include "Enums/GameVersion.h"
#include "Enums/LanguageID.h"
#include "Names/ItemNames.h"
#include "Names/LocationNames.h"
#include "Names/MoveInfo.h"
#include "Names/MoveNames.h"
#include "Names/SpeciesNames.h"
#include "Trainer/Trainer.h"
#include "UI/ClassicGameContext.h"
#include "UI/Common.h"
#include "UI/Gen3SharedPokemonSession.h"
#include "UI/SharedPokemonEditorContract.h"
#include "UI/SharedSpeciesPicker.h"
#include "Utils/Keyboard.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace UI::Gen3SharedEditorSurface {
namespace {

namespace Shared = PokeBank::UIModel::SharedPokemonEditor;
namespace SessionModel = PokeBank::UIModel::Gen3SharedEditor;
namespace Gen3 = PokeVault::Integration::Gen3;
namespace Encounter = PokeVault::Integration::EncounterGuardrails;
namespace Context = PokeBank::UIModel::ClassicGameContext;

enum class PickerTarget : uint8_t {
    None,
    HeldItem,
    Language,
    Ball,
    MetLocation,
    Move,
};

struct State {
    const TrainerViewScreen* owner = nullptr;
    bool passive = false;
    int detailsFocus = 0;
    bool previewValid = false;
    Gen3::StagedPokemonRecord previewInput{};
    Gen3::StagedPokemonRecord previewRecord{};
    std::string previewError;
    bool actions = false;
    bool review = false;
    bool legality = false;
    bool confirmRelease = false;
    bool speciesPicker = false;
    bool valuePicker = false;
    PickerTarget pickerTarget = PickerTarget::None;
    int pickerRow = 0;
    int pickerMoveSlot = 0;
    bool moveEditor = false;
    int moveEditorRow = 0;
    int moveEditorSlot = 0;
    uint16_t moveBaseline = 0;
    uint8_t movePpBaseline = 0;
    uint8_t movePpUpsBaseline = 0;
    std::vector<uint16_t> pickerValues;
    std::vector<Encounter::EncounterTemplate> encounterChoices;
    std::optional<Encounter::EncounterTemplate> selectedEncounter;
    int actionRow = 0;
    int box = 0;
    int slot = 0;
    uint16_t speciesPreview = 25;
    Shared::Focus focus{};
    SessionModel::Session session{};
};

State& stateFor(const TrainerViewScreen& screen) {
    static State state;
    if (state.owner != &screen) {
        state = {};
        state.owner = &screen;
    }
    return state;
}

bool isGen3Gba(const TrainerViewScreen& screen) noexcept {
    return screen.sourceGameId == "ruby_gba" ||
           screen.sourceGameId == "sapphire_gba" ||
           screen.sourceGameId == "emerald_gba" ||
           screen.sourceGameId == "firered_gba" ||
           screen.sourceGameId == "leafgreen_gba";
}

PokeVault::Legacy::FRLGReadOnlyTrainer& bridge(TrainerViewScreen& screen) {
    return static_cast<PokeVault::Legacy::FRLGReadOnlyTrainer&>(screen.trainer);
}

const PokeVault::Legacy::FRLGReadOnlyTrainer& bridge(const TrainerViewScreen& screen) {
    return static_cast<const PokeVault::Legacy::FRLGReadOnlyTrainer&>(screen.trainer);
}

Gen3::StagedPokemonEditor* editor(TrainerViewScreen& screen) noexcept {
    return isGen3Gba(screen) ? bridge(screen).stagedPokemon() : nullptr;
}

bool workspaceActive(const State& state) noexcept {
    return state.session.mode != SessionModel::Mode::None;
}

bool ownsFrameImpl(const TrainerViewScreen& screen) noexcept {
    if (!isGen3Gba(screen)) return false;
    const auto& state = stateFor(screen);
    return state.actions || state.review || state.legality || state.confirmRelease ||
           state.speciesPicker || state.valuePicker || workspaceActive(state);
}

bool boxEntrySurface(const TrainerViewScreen& screen) noexcept {
    return isGen3Gba(screen) &&
           screen.selectedMode == TrainerViewScreen::ViewMode::Boxes &&
           screen.detailViewActive &&
           screen.selectedBoxIndex >= 0 &&
           screen.selectedItemIndex >= 0 &&
           !screen.helpOverlayActive &&
           !screen.details.active &&
           !screen.actionSheet.isOpen() &&
           !screen.saveConfirmActive &&
           !screen.pickerActive &&
           !screen.itemEditDialogActive &&
           !screen.carrying() &&
           !screen.swapActive &&
           !screen.currentlySelecting;
}

bool selectedRecord(TrainerViewScreen& screen, Gen3::StagedPokemonRecord& record,
                    std::string& error) {
    auto* staged = editor(screen);
    if (!staged) {
        error = bridge(screen).stagedPokemonUnavailableReason();
        if (error.empty()) error = "Generation III staged Pokemon editor is unavailable";
        return false;
    }
    const auto value = staged->boxedPokemon(
        static_cast<std::size_t>(screen.selectedBoxIndex),
        static_cast<std::size_t>(screen.selectedItemIndex), error);
    if (!value) return false;
    record = *value;
    return true;
}

bool selectedOccupied(TrainerViewScreen& screen) {
    if (!boxEntrySurface(screen)) return false;
    auto* staged = editor(screen);
    if (!staged) return false;
    std::string error;
    return staged->boxedPokemon(
        static_cast<std::size_t>(screen.selectedBoxIndex),
        static_cast<std::size_t>(screen.selectedItemIndex), error).has_value();
}

void resetWorkspaceFocus(State& state) noexcept {
    state.focus = {Shared::Panel::Details, 0, 0};
    state.speciesPicker = false;
    state.valuePicker = false;
    state.pickerTarget = PickerTarget::None;
    state.pickerValues.clear();
    state.encounterChoices.clear();
    state.selectedEncounter.reset();
    state.moveEditor = false;
    state.moveEditorRow = 0;
    state.detailsFocus = 0;
    state.previewValid = false;
}

void beginActions(TrainerViewScreen& screen) {
    auto& state = stateFor(screen);
    state.actions = true;
    state.review = false;
    state.legality = false;
    state.confirmRelease = false;
    state.speciesPicker = false;
    state.valuePicker = false;
    state.pickerTarget = PickerTarget::None;
    state.pickerValues.clear();
    state.actionRow = 0;
    state.box = screen.selectedBoxIndex;
    state.slot = screen.selectedItemIndex;
    screen.controllerNavigation.reset();
}

bool beginViewOrEdit(TrainerViewScreen& screen, SessionModel::Mode mode) {
    auto& state = stateFor(screen);
    auto* staged = editor(screen);
    if (!staged) return false;
    std::string error;
    auto pokemon = staged->boxedPokemon(static_cast<std::size_t>(state.box),
                                        static_cast<std::size_t>(state.slot), error);
    if (!pokemon) {
        screen.postStatus(error.empty() ? "Selected Generation III slot is empty" : error, 300);
        return false;
    }
    state.passive = false;
    state.session.begin(*pokemon, mode);
    state.actions = false;
    state.review = false;
    state.legality = false;
    resetWorkspaceFocus(state);
    return true;
}

bool beginCreate(TrainerViewScreen& screen, int box, int slot) {
    auto* staged = editor(screen);
    if (!staged) {
        const auto& why = bridge(screen).stagedPokemonUnavailableReason();
        screen.postStatus(why.empty() ? "Generation III staged Pokemon editor unavailable" : why, 360);
        return false;
    }
    if (box < 0 || slot < 0 || box >= staged->boxCount() || slot >= staged->boxCapacity()) {
        screen.postStatus("Select a native Generation III box slot first");
        return false;
    }
    std::string error;
    if (staged->boxedPokemon(static_cast<std::size_t>(box), static_cast<std::size_t>(slot), error)) {
        screen.postStatus("Add Pokemon requires an empty Generation III slot");
        return false;
    }
    if (!error.empty()) {
        screen.postStatus(error);
        return false;
    }

    auto& state = stateFor(screen);
    state.box = box;
    state.slot = slot;
    state.actions = false;
    state.review = false;
    state.legality = false;
    state.confirmRelease = false;
    state.passive = false;
    state.session.beginCreate(staged->trainer());
    resetWorkspaceFocus(state);
    return true;
}

void refreshPresentation(TrainerViewScreen& screen) {
    std::string error;
    if (!bridge(screen).refreshStagedPokemonPresentation(error))
        screen.postStatus(error.empty() ? "Could not refresh staged Generation III boxes" : error, 360);
}

bool keepWorkspace(TrainerViewScreen& screen) {
    auto& state = stateFor(screen);
    auto* staged = editor(screen);
    if (!staged) return false;
    std::string error;
    bool ok = false;
    if (state.session.mode == SessionModel::Mode::Edit) {
        ok = state.session.keep(*staged, static_cast<std::size_t>(state.box),
                                static_cast<std::size_t>(state.slot), error);
    } else if (state.session.mode == SessionModel::Mode::Create) {
        ok = state.session.add(*staged, static_cast<std::size_t>(state.box),
                               static_cast<std::size_t>(state.slot), error);
    }
    if (!ok) {
        screen.postStatus(error.empty() ? "Generation III staged Pokemon operation failed" : error, 360);
        return false;
    }
    refreshPresentation(screen);
    screen.selectedBoxIndex = state.box;
    screen.selectedItemIndex = state.slot;
    screen.postStatus("Generation III Pokemon change staged; source save unchanged", 300);
    return true;
}

const char* genderText(uint8_t gender) noexcept {
    return gender == 0 ? "Male" : gender == 1 ? "Female" : "Genderless";
}

std::string exactOriginText(const TrainerViewScreen&,
                            const Gen3::StagedPokemonRecord& p) {
    return Enums::getOriginGameName(p.originGame);
}

std::string metLocationText(const Gen3::StagedPokemonRecord& p) {
    const char* name = Names::getMetLocationName(p.originGame, p.metLocation);
    if (p.metLocation == 0) return "(none)";
    const std::string id = "ID " + std::to_string(p.metLocation);
    if (name && *name) return std::string(name) + " (" + id + ")";
    return "Unknown (" + id + ")";
}

Color genderValueColor(uint8_t gender) {
    if (gender == 0)
        return g_themeMode == ThemeMode::Light ? Color(24,112,184) : Color(92,205,255);
    if (gender == 1) return Colors::Magenta;
    return Colors::Text;
}

std::string detailsValue(const TrainerViewScreen& screen,
                         const Gen3::StagedPokemonRecord& p, int row) {
    switch (row) {
        case 0: return std::to_string(p.species) + " - " + Names::getSpeciesName(p.species);
        case 1: return p.nickname.empty() ? "(none)" : p.nickname;
        case 2: return std::to_string(p.level);
        case 3: return std::to_string(p.experience);
        case 4: return p.otName.empty() ? "(none)" : p.otName;
        case 5: return std::to_string(p.tid);
        case 6: return std::to_string(p.sid);
        case 7: return Names::getItemNameG3(p.heldItem);
        case 8: return std::to_string(p.friendship);
        case 9: return Enums::getLanguageName(p.language);
        case 10: return exactOriginText(screen, p);
        case 11: return Enums::getBallName(p.ball);
        case 12: return std::to_string(p.metLevel);
        case 13: return metLocationText(p);
        case 14: return std::to_string(p.pokerus);
        default: return {};
    }
}

constexpr std::array<const char*,15> detailLabels{{
    "Species", "Nickname", "Level", "Experience", "OT", "TID (read-only)", "SID (read-only)",
    "Held Item", "Friendship", "Language", "Origin", "Ball",
    "Met Level", "Met Location", "Pokerus"
}};

bool detailEditable(const State& state, int row) noexcept {
    if (!state.session.editable()) return false;
    if (state.session.mode == SessionModel::Mode::Create)
        return row == 0 || row == 1 || row == 2 || row == 7 ||
               row == 8 || row == 9 || row == 11 || row == 12 || row == 13;
    return row == 0 || row == 1 || row == 2 || row == 3 || row == 4 ||
           row == 7 || row == 8 || row == 9 || row == 11 || row == 12 ||
           row == 13 || row == 14;
}

void normalizeEditableFocus(State& state, int verticalDirection = 0, bool panelChanged = false) {
    state.focus = Shared::normalize(Shared::Generation::Gen3, state.focus);
    if (!state.session.editable()) return;

    if (state.focus.panel == Shared::Panel::Details) {
        if (detailEditable(state, state.focus.row)) return;
        int row = state.focus.row;
        const int direction = verticalDirection < 0 ? -1 : 1;
        for (int step = 0; step < static_cast<int>(detailLabels.size()); ++step) {
            row = (row + direction + static_cast<int>(detailLabels.size())) %
                  static_cast<int>(detailLabels.size());
            if (detailEditable(state, row)) {
                state.focus.row = static_cast<uint8_t>(row);
                state.focus.column = 0;
                return;
            }
        }
        return;
    }

    if (state.focus.panel == Shared::Panel::Values) {
        // Nature/Gender/Shiny/Ability/PID and the calculated Stat column remain read-only.
        if (panelChanged || state.focus.row >= 6) state.focus.row = 0;
        if (state.focus.column >= 2) state.focus.column = 1;
        return;
    }

    if (state.focus.panel == Shared::Panel::Moves) {
        if (panelChanged && state.focus.row >= 4) state.focus.row = 0;
        state.focus.row = static_cast<uint8_t>(state.focus.row % 4);
        if (state.focus.column >= 3) state.focus.column = 2;
    }
}

std::string pickerValueLabel(const State& state, uint16_t value) {
    switch (state.pickerTarget) {
        case PickerTarget::HeldItem: return Names::getItemNameG3(value);
        case PickerTarget::Language: return Enums::getLanguageName(static_cast<uint8_t>(value));
        case PickerTarget::Ball: return Enums::getBallName(static_cast<uint8_t>(value));
        case PickerTarget::MetLocation: {
            if (value >= state.encounterChoices.size()) return "Invalid encounter";
            const auto& encounter = state.encounterChoices[value];
            const char* name = Names::getMetLocationName(state.session.working.originGame, encounter.location);
            std::string label = (name && *name) ? std::string(name) : "Unknown";
            label += " (ID " + std::to_string(encounter.location) + ") — ";
            label += Encounter::methodName(encounter.method);
            label += " — Lv. " + std::to_string(encounter.minLevel);
            if (encounter.maxLevel != encounter.minLevel)
                label += "–" + std::to_string(encounter.maxLevel);
            return label;
        }
        case PickerTarget::Move: {
            std::string id = std::to_string(value);
            while (id.size() < 3) id = "0" + id;
            return value == 0 ? "000 - Empty move slot" :
                                id + " - " + std::string(Names::getMoveName(value));
        }
        default: return {};
    }
}

const char* pickerTitle(PickerTarget target) noexcept {
    switch (target) {
        case PickerTarget::HeldItem: return "Choose Held Item";
        case PickerTarget::Language: return "Choose Language";
        case PickerTarget::Ball: return "Choose Ball";
        case PickerTarget::MetLocation: return "Choose Met Location";
        case PickerTarget::Move: return "Choose Move";
        default: return "Choose Value";
    }
}

uint16_t currentPickerValue(const State& state, PickerTarget target, int moveSlot = 0) noexcept {
    const auto& p = state.session.working;
    switch (target) {
        case PickerTarget::HeldItem: return p.heldItem;
        case PickerTarget::Language: return p.language;
        case PickerTarget::Ball: return p.ball;
        case PickerTarget::MetLocation: return p.metLocation;
        case PickerTarget::Move:
            return moveSlot >= 0 && moveSlot < 4 ? p.moves[static_cast<std::size_t>(moveSlot)] : 0;
        default: return 0;
    }
}

void openValuePicker(TrainerViewScreen& screen, State& state, PickerTarget target,
                     int moveSlot = 0) {
    state.pickerTarget = target;
    state.pickerMoveSlot = moveSlot;
    state.pickerValues.clear();
    state.encounterChoices.clear();

    if (target == PickerTarget::HeldItem) {
        const auto count = Names::getItemCountG3();
        state.pickerValues.reserve(count);
        for (std::size_t id = 0; id < count; ++id) {
            const char* name = Names::getItemNameG3(static_cast<uint16_t>(id));
            if (id == 0 || (name && *name && std::string(name) != "???"))
                state.pickerValues.push_back(static_cast<uint16_t>(id));
        }
    } else if (target == PickerTarget::Language) {
        state.pickerValues = {1, 2, 3, 4, 5, 7};
    } else if (target == PickerTarget::Ball) {
        const auto balls = Enums::getBallList(Enums::GameVersion::FRLG);
        state.pickerValues.assign(balls.begin(), balls.end());
    } else if (target == PickerTarget::MetLocation) {
        state.encounterChoices = Encounter::forGameSpecies(screen.sourceGameId, state.session.working.species);
        state.pickerValues.reserve(state.encounterChoices.size());
        for (std::size_t i = 0; i < state.encounterChoices.size(); ++i)
            state.pickerValues.push_back(static_cast<uint16_t>(i));
        if (!state.encounterChoices.empty()) {
            state.pickerRow = 0;
            for (int i = 0; i < static_cast<int>(state.encounterChoices.size()); ++i) {
                const auto& encounter = state.encounterChoices[static_cast<std::size_t>(i)];
                if (encounter.location == state.session.working.metLocation &&
                    encounter.containsLevel(state.session.working.metLevel)) {
                    state.pickerRow = i;
                    break;
                }
            }
            state.valuePicker = true;
            screen.controllerNavigation.reset();
            return;
        }
    } else if (target == PickerTarget::Move) {
        const auto game = PokeVault::Integration::Gen3EditorProvider::sourceGameForId(screen.sourceGameId);
        if (game) {
            state.pickerValues = Gen3::Learnset::selectableMoves(
                *game, state.session.working.species, state.session.baseline.moves);
        }
    }

    const uint16_t current = currentPickerValue(state, target, moveSlot);
    if (std::find(state.pickerValues.begin(), state.pickerValues.end(), current) ==
        state.pickerValues.end())
        state.pickerValues.push_back(current);

    if (state.pickerValues.empty()) {
        state.pickerTarget = PickerTarget::None;
        screen.postStatus("No valid values are available for this Generation III field", 300);
        return;
    }

    state.pickerRow = 0;
    for (int i = 0; i < static_cast<int>(state.pickerValues.size()); ++i) {
        if (state.pickerValues[static_cast<std::size_t>(i)] == current) {
            state.pickerRow = i;
            break;
        }
    }
    state.valuePicker = true;
    screen.controllerNavigation.reset();
}

void applyValuePicker(State& state) {
    if (!state.valuePicker || state.pickerValues.empty()) return;
    const auto value = state.pickerValues[static_cast<std::size_t>(state.pickerRow)];
    auto& p = state.session.working;
    switch (state.pickerTarget) {
        case PickerTarget::HeldItem: p.heldItem = value; break;
        case PickerTarget::Language: p.language = static_cast<uint8_t>(value); break;
        case PickerTarget::Ball: p.ball = static_cast<uint8_t>(value); break;
        case PickerTarget::MetLocation: {
            if (value < state.encounterChoices.size()) {
                const auto encounter = state.encounterChoices[value];
                p.metLocation = encounter.location;
                p.metLevel = encounter.minLevel;
                state.selectedEncounter = encounter;
            }
            break;
        }
        case PickerTarget::Move: {
            const auto slot = static_cast<std::size_t>(state.pickerMoveSlot);
            p.moves[slot] = value;
            p.ppUps[slot] = 0;
            p.pp[slot] = value == 0 ? 0 :
                Names::getMoveBasePP(value, Enums::GameVersion::FRLG);
            break;
        }
        default: break;
    }
    state.valuePicker = false;
    state.pickerTarget = PickerTarget::None;
    state.pickerValues.clear();
    state.encounterChoices.clear();
    state.previewValid = false;
}

void editDetail(TrainerViewScreen& screen, State& state, int row) {
    auto& p = state.session.working;
    if (!detailEditable(state, row)) {
        screen.postStatus("This Generation III field is read-only in the current safe adapter", 300);
        return;
    }
    if (row == 0) {
        state.speciesPreview = std::clamp<uint16_t>(p.species, 1, 386);
        state.speciesPicker = true;
        return;
    }
    if (row == 1) {
        const auto r = Utils::promptText("Generation III Nickname", "1-10 Gen III characters",
                                         p.nickname, 10);
        if (r.accepted) p.nickname = r.text;
        return;
    }
    if (row == 2) {
        const auto r = Utils::promptNumber("Generation III Level", p.level, 1, 100);
        if (r.accepted) state.session.setLevel(static_cast<uint8_t>(r.value));
        return;
    }
    if (row == 3) {
        const uint32_t maximum =
            Pokemon::getExpForLevel(100, Pokemon::getGrowthRate(p.species));
        const auto r = Utils::promptNumber("Generation III Experience",
            static_cast<int>(std::min<uint32_t>(p.experience, 2000000000u)),
            0, static_cast<int>(std::min<uint32_t>(maximum, 2000000000u)));
        if (r.accepted) state.session.setExperience(static_cast<uint32_t>(r.value));
        return;
    }
    if (row == 4) {
        const auto r = Utils::promptText("Generation III OT", "1-7 Gen III characters",
                                         p.otName, 7);
        if (r.accepted) p.otName = r.text;
        return;
    }

    if (row == 7) { openValuePicker(screen, state, PickerTarget::HeldItem); return; }
    if (row == 9) { openValuePicker(screen, state, PickerTarget::Language); return; }
    if (row == 11) { openValuePicker(screen, state, PickerTarget::Ball); return; }
    if (row == 13) { openValuePicker(screen, state, PickerTarget::MetLocation); return; }
    if (row == 12) {
        auto choices = Encounter::forGameSpecies(screen.sourceGameId, p.species);
        std::optional<Encounter::EncounterTemplate> chosen = state.selectedEncounter;
        if (!chosen || chosen->location != p.metLocation) {
            chosen.reset();
            for (const auto& encounter : choices) {
                if (encounter.location != p.metLocation) continue;
                if (encounter.containsLevel(p.metLevel)) { chosen = encounter; break; }
                if (!chosen) chosen = encounter;
            }
        }
        if (!chosen) {
            screen.postStatus("Choose a legitimate Met Location encounter first", 300);
            return;
        }
        if (chosen->fixedLevel()) {
            p.metLevel = chosen->minLevel;
            state.selectedEncounter = *chosen;
            screen.postStatus("This encounter has fixed met level " + std::to_string(chosen->minLevel), 240);
            return;
        }
        const auto r = Utils::promptNumber("Met Level for selected encounter", p.metLevel,
                                           chosen->minLevel, chosen->maxLevel);
        if (r.accepted) {
            p.metLevel = static_cast<uint8_t>(r.value);
            state.selectedEncounter = *chosen;
        }
        return;
    }

    int current = 0, minimum = 0, maximum = 255;
    const char* title = "Generation III value";
    switch (row) {
        case 8: current = p.friendship; title = "Friendship"; break;
        case 14: current = p.pokerus; title = "Pokerus byte"; break;
        default: return;
    }
    const auto r = Utils::promptNumber(title, current, minimum, maximum);
    if (!r.accepted) return;
    switch (row) {
        case 8: p.friendship = static_cast<uint8_t>(r.value); break;
        case 14: p.pokerus = static_cast<uint8_t>(r.value); break;
        default: break;
    }
}

void editStatCell(TrainerViewScreen& screen, State& state) {
    if (!state.session.editable()) return;
    if (state.focus.row >= 6 || state.focus.column >= 2) {
        screen.postStatus("Nature, Gender, Shiny, Ability and PID are read-only for PID safety", 300);
        return;
    }
    const std::size_t stat = state.focus.row;
    if (state.focus.column == 0) {
        const auto r = Utils::promptNumber("Generation III IV",
            state.session.working.ivs[stat], 0, 31);
        if (r.accepted) state.session.working.ivs[stat] = static_cast<uint8_t>(r.value);
    } else {
        const auto r = Utils::promptNumber("Generation III EV",
            state.session.working.evs[stat], 0, 255);
        if (!r.accepted) return;
        auto evs = state.session.working.evs;
        evs[stat] = static_cast<uint8_t>(r.value);
        unsigned total = 0;
        for (uint8_t value : evs) total += value;
        if (total > 510) {
            screen.postStatus("Generation III EV total cannot exceed 510", 300);
            return;
        }
        state.session.working.evs = evs;
    }
}

int gen3MoveMaxPP(uint16_t move, uint8_t ppUps) {
    if (move == 0) return 0;
    const int base = Names::getMoveBasePP(move, Enums::GameVersion::FRLG);
    return base * (5 + std::min<int>(ppUps, 3)) / 5;
}

void beginMoveEditor(TrainerViewScreen& screen, State& state, int slot) {
    state.moveEditor = true;
    state.moveEditorRow = 0;
    state.moveEditorSlot = std::clamp(slot, 0, 3);
    const auto index = static_cast<std::size_t>(state.moveEditorSlot);
    state.moveBaseline = state.session.working.moves[index];
    state.movePpBaseline = state.session.working.pp[index];
    state.movePpUpsBaseline = state.session.working.ppUps[index];
    screen.controllerNavigation.reset();
}

void cancelMoveEditor(TrainerViewScreen& screen, State& state) {
    if (!state.moveEditor) return;
    const auto index = static_cast<std::size_t>(state.moveEditorSlot);
    state.session.working.moves[index] = state.moveBaseline;
    state.session.working.pp[index] = state.movePpBaseline;
    state.session.working.ppUps[index] = state.movePpUpsBaseline;
    state.moveEditor = false;
    state.moveEditorRow = 0;
    state.previewValid = false;
    screen.controllerNavigation.reset();
}

void editMoveCell(TrainerViewScreen& screen, State& state) {
    if (!state.session.editable()) return;
    beginMoveEditor(screen, state, static_cast<int>(state.focus.row));
}


bool handleValuePicker(TrainerViewScreen&, State& state, u64 down) {
    if (!state.valuePicker) return false;
    const int count = static_cast<int>(state.pickerValues.size());
    if (count <= 0) {
        state.valuePicker = false;
        return true;
    }
    if (down & HidNpadButton_Up) state.pickerRow = (state.pickerRow - 1 + count) % count;
    if (down & HidNpadButton_Down) state.pickerRow = (state.pickerRow + 1) % count;
    if (down & HidNpadButton_Left) state.pickerRow = std::max(0, state.pickerRow - 10);
    if (down & HidNpadButton_Right) state.pickerRow = std::min(count - 1, state.pickerRow + 10);
    if (down & HidNpadButton_B) {
        state.valuePicker = false;
        state.pickerTarget = PickerTarget::None;
        state.pickerValues.clear();
        return true;
    }
    if (down & HidNpadButton_A) {
        applyValuePicker(state);
        return true;
    }
    return true;
}

bool handleSpeciesPicker(TrainerViewScreen& screen, State& state, u64 down) {
    if (!state.speciesPicker) return false;
    if (down & HidNpadButton_Up)
        state.speciesPreview = static_cast<uint16_t>(std::max<int>(1, state.speciesPreview - 1));
    if (down & HidNpadButton_Down)
        state.speciesPreview = static_cast<uint16_t>(std::min<int>(386, state.speciesPreview + 1));
    if (down & HidNpadButton_Left)
        state.speciesPreview = static_cast<uint16_t>(std::max<int>(1, state.speciesPreview - 10));
    if (down & HidNpadButton_Right)
        state.speciesPreview = static_cast<uint16_t>(std::min<int>(386, state.speciesPreview + 10));
    if (down & HidNpadButton_B) {
        state.speciesPicker = false;
        return true;
    }
    if (down & HidNpadButton_A) {
        if (state.session.setSpecies(state.speciesPreview, screen.sourceGameId)) {
            state.speciesPicker = false;
            const auto& p = state.session.working;
            if (p.metLocation != 0 &&
                !Encounter::locationAllowed(screen.sourceGameId, p.species, p.metLocation)) {
                state.session.working.metLocation = 0;
                state.session.working.metLevel = 0;
                state.selectedEncounter.reset();
                screen.postStatus("Species changed; choose a new legitimate encounter", 300);
            }
        }
        else
            screen.postStatus("Species is not representable in Generation III");
        return true;
    }
    return true;
}

bool handleMoveEditor(TrainerViewScreen& screen, State& state, u64 down) {
    if (!state.moveEditor) return false;
    if (state.valuePicker) return false;
    if (down & HidNpadButton_Up) state.moveEditorRow = (state.moveEditorRow + 4) % 5;
    if (down & HidNpadButton_Down) state.moveEditorRow = (state.moveEditorRow + 1) % 5;
    if (down & HidNpadButton_B) {
        cancelMoveEditor(screen, state);
        return true;
    }
    if (!(down & HidNpadButton_A)) return true;

    const auto slot = static_cast<std::size_t>(state.moveEditorSlot);
    if (state.moveEditorRow == 0) {
        openValuePicker(screen, state, PickerTarget::Move, state.moveEditorSlot);
        return true;
    }
    if (state.moveEditorRow == 1) {
        if (state.session.working.moves[slot] == 0) {
            screen.postStatus("Choose a move before editing PP", 220);
            return true;
        }
        const int maximum = gen3MoveMaxPP(state.session.working.moves[slot],
                                          state.session.working.ppUps[slot]);
        const auto r = Utils::promptNumber("Current PP",
            std::min<int>(state.session.working.pp[slot], maximum), 0, maximum);
        if (r.accepted) state.session.working.pp[slot] = static_cast<uint8_t>(r.value);
        state.previewValid = false;
        return true;
    }
    if (state.moveEditorRow == 2) {
        if (state.session.working.moves[slot] == 0) {
            screen.postStatus("Empty move slots keep PP Ups at zero", 220);
            return true;
        }
        const auto r = Utils::promptNumber("PP Ups", state.session.working.ppUps[slot], 0, 3);
        if (r.accepted) {
            state.session.working.ppUps[slot] = static_cast<uint8_t>(r.value);
            state.session.working.pp[slot] = static_cast<uint8_t>(std::min<int>(
                state.session.working.pp[slot],
                gen3MoveMaxPP(state.session.working.moves[slot], state.session.working.ppUps[slot])));
        }
        state.previewValid = false;
        return true;
    }
    if (state.moveEditorRow == 3) {
        state.moveEditor = false;
        state.moveEditorRow = 0;
        state.previewValid = false;
        screen.controllerNavigation.reset();
        return true;
    }
    cancelMoveEditor(screen, state);
    return true;
}

bool handleWorkspace(TrainerViewScreen& screen, State& state, u64 down) {
    if (!workspaceActive(state)) return false;
    if (handleValuePicker(screen, state, down)) return true;
    if (handleSpeciesPicker(screen, state, down)) return true;
    if (handleMoveEditor(screen, state, down)) return true;

    if (state.session.confirmExit) {
        if (down & HidNpadButton_A) {
            if (keepWorkspace(screen)) state.session.confirmExit = false;
            return true;
        }
        if (down & HidNpadButton_X) {
            const bool create = state.session.mode == SessionModel::Mode::Create;
            state.session.discardDraft();
            screen.postStatus(create
                ? "Generation III Create draft discarded; staged/source bytes unchanged"
                : "Generation III Edit session discarded; staged/source bytes unchanged");
            return true;
        }
        if (down & HidNpadButton_B) {
            state.session.continueEditing();
            return true;
        }
        return true;
    }

    const bool panelChanged = (down & (HidNpadButton_L | HidNpadButton_R)) != 0;
    int verticalDirection = 0;
    if (down & HidNpadButton_L)
        state.focus = Shared::switchPanel(Shared::Generation::Gen3, state.focus, -1);
    if (down & HidNpadButton_R)
        state.focus = Shared::switchPanel(Shared::Generation::Gen3, state.focus, 1);
    if (down & HidNpadButton_Up) {
        state.focus = Shared::moveVertical(Shared::Generation::Gen3, state.focus, -1);
        verticalDirection = -1;
    }
    if (down & HidNpadButton_Down) {
        state.focus = Shared::moveVertical(Shared::Generation::Gen3, state.focus, 1);
        verticalDirection = 1;
    }
    if (down & HidNpadButton_Left)
        state.focus = Shared::moveColumn(Shared::Generation::Gen3, state.focus, -1);
    if (down & HidNpadButton_Right)
        state.focus = Shared::moveColumn(Shared::Generation::Gen3, state.focus, 1);

    normalizeEditableFocus(state, verticalDirection, panelChanged);
    state.detailsFocus = Shared::detailsScrollFocus(state.focus, state.detailsFocus);
    state.detailsFocus = std::min<int>(state.detailsFocus, static_cast<int>(detailLabels.size()) - 1);

    if (down & HidNpadButton_B) {
        if (state.session.mode == SessionModel::Mode::View) {
            state.session.mode = SessionModel::Mode::None;
            if (state.passive) { screen.closeDetailsModal(); state.passive = false; }
        } else {
            // Create and Edit both enter the shared explicit exit safeguard.
            state.session.back();
        }
        return true;
    }
    if ((down & HidNpadButton_X) && state.session.editable()) {
        keepWorkspace(screen);
        return true;
    }
    if (!(down & HidNpadButton_A) || !state.session.editable()) return true;

    if (state.focus.panel == Shared::Panel::Details)
        editDetail(screen, state, state.focus.row);
    else if (state.focus.panel == Shared::Panel::Values)
        editStatCell(screen, state);
    else
        editMoveCell(screen, state);
    return true;
}

bool handleActions(TrainerViewScreen& screen, State& state, u64 down) {
    if (!state.actions) return false;
    auto* staged = editor(screen);
    if (!staged) {
        state.actions = false;
        return true;
    }
    std::string error;
    const bool occupied = staged->boxedPokemon(static_cast<std::size_t>(state.box),
                                                static_cast<std::size_t>(state.slot), error).has_value();
    const auto actions = Shared::actionsForSlot(occupied, {/*clone*/true, /*remove*/true, /*legality*/true});
    if (actions.count == 0) return true;
    if (down & HidNpadButton_Up)
        state.actionRow = (state.actionRow - 1 + static_cast<int>(actions.count)) %
                          static_cast<int>(actions.count);
    if (down & HidNpadButton_Down)
        state.actionRow = (state.actionRow + 1) % static_cast<int>(actions.count);
    if (down & HidNpadButton_B) {
        state.actions = false;
        return true;
    }
    if (!(down & HidNpadButton_A)) return true;

    const auto action = actions[static_cast<std::size_t>(state.actionRow)];
    switch (action) {
        case Shared::Action::View: beginViewOrEdit(screen, SessionModel::Mode::View); break;
        case Shared::Action::Edit: beginViewOrEdit(screen, SessionModel::Mode::Edit); break;
        case Shared::Action::Add: beginCreate(screen, state.box, state.slot); break;
        case Shared::Action::LegalityProvenance:
            state.actions = false;
            state.legality = true;
            break;
        case Shared::Action::Review:
            state.actions = false;
            state.review = true;
            break;
        case Shared::Action::Remove:
            state.actions = false;
            state.confirmRelease = true;
            break;
        case Shared::Action::Clone: {
            const auto box = Utils::promptNumber("Clone to Gen III Box", state.box + 1, 1, 14);
            if (!box.accepted) break;
            const auto slot = Utils::promptNumber("Clone to Slot", state.slot + 1, 1, 30);
            if (!slot.accepted) break;
            if (!staged->stageCloneBoxPokemon(
                    static_cast<std::size_t>(state.box), static_cast<std::size_t>(state.slot),
                    static_cast<std::size_t>(box.value - 1), static_cast<std::size_t>(slot.value - 1),
                    error)) {
                screen.postStatus(error.empty() ? "Generation III Clone failed" : error, 360);
            } else {
                refreshPresentation(screen);
                screen.postStatus("Generation III Clone staged; source save unchanged", 300);
            }
            break;
        }
        case Shared::Action::Close:
            state.actions = false;
            break;
        default:
            screen.postStatus("This Generation III action is not enabled in the current milestone");
            break;
    }
    return true;
}

bool handleReview(TrainerViewScreen& screen, State& state, u64 down) {
    if (!state.review) return false;
    auto* staged = editor(screen);
    if (!staged) {
        state.review = false;
        return true;
    }
    if (down & HidNpadButton_B) {
        state.review = false;
        return true;
    }
    if (down & HidNpadButton_Y) {
        staged->discard();
        refreshPresentation(screen);
        state.review = false;
        screen.postStatus("Generation III staged changes discarded; original save never changed", 360);
        return true;
    }
    return true;
}

bool handleReleaseConfirm(TrainerViewScreen& screen, State& state, u64 down) {
    if (!state.confirmRelease) return false;
    if (down & HidNpadButton_B) {
        state.confirmRelease = false;
        state.actions = true;
        return true;
    }
    if (!(down & HidNpadButton_A)) return true;
    auto* staged = editor(screen);
    if (!staged) return true;
    std::string error;
    if (!staged->stageReleaseBoxPokemon(
            static_cast<std::size_t>(state.box), static_cast<std::size_t>(state.slot), error)) {
        screen.postStatus(error.empty() ? "Generation III Release failed" : error, 360);
    } else {
        refreshPresentation(screen);
        screen.postStatus("Generation III Release staged; source save unchanged", 300);
    }
    state.confirmRelease = false;
    return true;
}

std::string surfaceContext(const TrainerViewScreen& screen, const State& state) {
    Context::SurfaceMode mode = Context::SurfaceMode::ViewReadOnly;
    if (state.session.mode == SessionModel::Mode::Edit) mode = Context::SurfaceMode::EditStaged;
    else if (state.session.mode == SessionModel::Mode::Create) mode = Context::SurfaceMode::CreateDraft;
    if (state.passive) {
        if (screen.details.source == TrainerViewScreen::EditSource::Party)
            return "Party Slot " + std::to_string(screen.details.partyIndex + 1);
        return "Legacy Storage / Slot " + std::to_string(screen.selectedItemIndex + 1);
    }
    return Context::contextLine(
        "Box " + std::to_string(state.box + 1) + " / Slot " + std::to_string(state.slot + 1),
        screen.sourceGameId, mode);
}

void drawPanel(PKSEFramebuffer& fb, int x, int y, int w, int h, const char* title) {
    fb.drawFilledRoundedRect(x, y, w, h, 14, Colors::Panel);
    fb.drawRoundedRect(x, y, w, h, 14, Colors::Border, 1);
    fb.drawText(x + 14, y + 12, title, Colors::Text, TextStyle::Heading);
}

void drawFocus(PKSEFramebuffer& fb, int x, int y, int w, int h, bool selected) {
    if (selected) fb.drawRoundedRect(x, y, w, h, 6, Colors::FocusBorder, 2);
}

void drawValuePicker(TrainerViewScreen& screen, PKSEFramebuffer& fb, State& state) {
    if (!state.valuePicker || state.pickerValues.empty()) return;
    constexpr int x = 300, y = 74, w = 680, h = 560;
    fb.drawFilledRect(0, 0, fb.getWidth(), fb.getHeight() - kNavBarH, Color(0, 0, 0, 105));
    fb.drawFilledRoundedRect(x, y, w, h, 18, Colors::PanelAlt);
    fb.drawRoundedRect(x, y, w, h, 18, Colors::FocusBorder, 2);
    fb.drawText(x + 28, y + 20, pickerTitle(state.pickerTarget), Colors::Text, TextStyle::Heading);
    fb.drawText(x + 28, y + 55,
                std::to_string(state.pickerRow + 1) + " / " +
                    std::to_string(state.pickerValues.size()),
                Colors::TextDim, TextStyle::Caption);

    constexpr int visible = 10;
    const int count = static_cast<int>(state.pickerValues.size());
    const int first = std::clamp(state.pickerRow - visible / 2, 0, std::max(0, count - visible));
    const int last = std::min(count, first + visible);
    for (int i = first; i < last; ++i) {
        const int yy = y + 90 + (i - first) * 42;
        const bool selected = i == state.pickerRow;
        if (selected) fb.drawSelectionHighlight(x + 22, yy - 4, w - 44, 38);
        const auto value = state.pickerValues[static_cast<std::size_t>(i)];
        std::string label = pickerValueLabel(state, value);
        Color color = selected ? Colors::Text : Colors::TextDim;
        std::string suffix;
        if (state.pickerTarget == PickerTarget::Move) {
            const auto game =
                PokeVault::Integration::Gen3EditorProvider::sourceGameForId(screen.sourceGameId);
            if (game) {
                const auto status = Gen3::Learnset::classify(
                    *game, state.session.working.species, value, state.session.baseline.moves);
                if (status == Gen3::Learnset::Availability::Direct) {
                    suffix = "  OK";
                    color = selected ? Colors::Text : Colors::Success;
                } else if (status == Gen3::Learnset::Availability::Transfer) {
                    suffix = "  Transfer";
                    color = Colors::Warning;
                } else if (status == Gen3::Learnset::Availability::Preserved) {
                    suffix = "  Preserved";
                    color = Colors::Warning;
                }
            }
        }
        fb.drawText(x + 38, yy + 7, label + suffix, color, TextStyle::Body);
    }
    drawNavBar(fb, {{"Up/Down", "Browse"}, {"Left/Right", "Page"},
                    {"A", "Choose"}, {"B", "Cancel"}});
}


void drawMoveEditor(PKSEFramebuffer& fb, State& state) {
    if (!state.moveEditor) return;
    const auto slot = static_cast<std::size_t>(state.moveEditorSlot);
    constexpr int w = 650, h = 390;
    const int x = (fb.getWidth() - w) / 2;
    const int y = (fb.getHeight() - kNavBarH - h) / 2;
    fb.drawFilledRect(0, 0, fb.getWidth(), fb.getHeight() - kNavBarH, Color(0, 0, 0, 95));
    fb.drawFilledRoundedRect(x, y, w, h, 18, Colors::Panel);
    fb.drawRoundedRect(x, y, w, h, 18, Colors::FocusBorder, 2);
    fb.drawText(x + 24, y + 18, "Move " + std::to_string(state.moveEditorSlot + 1),
                Colors::Text, TextStyle::Heading);
    fb.drawText(x + 24, y + 52, "Contextual editor — B always cancels this dialog",
                Colors::TextDim, TextStyle::Caption);
    const uint16_t move = state.session.working.moves[slot];
    const std::array<std::pair<std::string, std::string>, 5> rows{{
        {"Move", move == 0 ? std::string("Empty") : std::string(Names::getMoveName(move))},
        {"PP", std::to_string(state.session.working.pp[slot]) + "/" +
               std::to_string(gen3MoveMaxPP(move, state.session.working.ppUps[slot]))},
        {"PP Ups", std::to_string(state.session.working.ppUps[slot])},
        {"Apply", "Keep these move details"},
        {"Cancel", "Discard these move details"},
    }};
    int rowY = y + 88;
    for (int i = 0; i < static_cast<int>(rows.size()); ++i) {
        const bool selected = i == state.moveEditorRow;
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

void drawWorkspace(TrainerViewScreen& screen, PKSEFramebuffer& fb, State& state) {
    // Preview uses the exact serializer on a private copy only when the draft changes.
    // Failed drafts never display stale computed stats as if they were valid.
    if (state.session.editable() && (!state.previewValid ||
        !SessionModel::sameEditableRecord(state.previewInput, state.session.working))) {
        state.previewInput = state.session.working;
        state.previewError.clear();
        if (auto* staged = editor(screen)) {
            auto preview = state.session.mode == SessionModel::Mode::Create
                ? staged->previewCreate(state.box, state.slot, state.session.createRequest(), state.previewError)
                : staged->previewEdit(state.box, state.slot, state.session.editRequest(), state.previewError);
            if (preview) state.previewRecord = *preview;
        } else state.previewError = "Exact preview unavailable";
        state.previewValid = true;
    }
    const auto& p = state.session.editable() && state.previewError.empty()
        ? state.previewRecord : state.session.working;
    screen.touchButtons.clear();
    auto mode = Context::SurfaceMode::ViewReadOnly;
    if (state.session.mode == SessionModel::Mode::Edit) mode = Context::SurfaceMode::EditStaged;
    if (state.session.mode == SessionModel::Mode::Create) mode = Context::SurfaceMode::CreateDraft;
    SharedPokemonShell::drawChrome(fb, p.species, p.nickname, p.level, p.gender, p.shiny,
        mode, state.passive ? surfaceContext(screen, state) :
        "Box " + std::to_string(state.box + 1) + " / Slot " + std::to_string(state.slot + 1), screen.sourceGameId);
    const SharedPokemonShell::Geometry geometry(fb.getWidth(), fb.getHeight(), kNavBarH);
    const int y = geometry.y, h = geometry.h;
    const int leftX = geometry.leftX, leftW = geometry.leftW;
    const int midX = geometry.midX, midW = geometry.midW;
    const int rightX = geometry.rightX, rightW = geometry.rightW;

    drawPanel(fb, leftX, y, leftW, h, "DETAILS");
    SharedPokemonShell::drawPortrait(fb, leftX, y, p.species, p.shiny);
    const auto& personal = Pokemon::getPersonalInfoG3(p.species);
    const std::array<uint8_t,2> types{personal.type1, personal.type2};
    int typeX = leftX + 66;
    for (std::size_t i = 0; i < types.size(); ++i) {
        if (types[i] == 255 || (i && types[i] == types[0])) continue;
        if (auto* sprite = SpriteManager::getTypeSprite(types[i]); sprite && sprite->data && sprite->height > 0) {
            const int width = sprite->width * 22 / sprite->height;
            fb.drawImageScaled(typeX, y + 180, sprite->width, sprite->height, width, 22, sprite->data, sprite->channels);
            typeX += width + 8;
        }
    }
    SharedPokemonShell::drawScrollableDetails(fb, leftX, y, leftW, h,
        detailLabels.size(), state.detailsFocus,
        state.session.editable() && state.focus.panel == Shared::Panel::Details,
        [](std::size_t row) { return detailLabels[row]; },
        [&](std::size_t row) { return detailsValue(screen, p, static_cast<int>(row)); });
    drawPanel(fb, midX, y, midW, h, "STATS");
    fb.drawText(midX + 126, y + 50, "IV", Colors::TextDim, TextStyle::Caption);
    fb.drawText(midX + 190, y + 50, "EV", Colors::TextDim, TextStyle::Caption);
    fb.drawText(midX + 274, y + 50, "Stat", Colors::TextDim, TextStyle::Caption);
    constexpr std::array<const char*,6> statLabels{{"HP","Attack","Defense","Speed","Sp. Atk","Sp. Def"}};
    for (int row = 0; row < 6; ++row) {
        const int yy = y + 78 + row * 40;
        const bool selected = state.session.editable() &&
                              state.focus.panel == Shared::Panel::Values &&
                              state.focus.row == row;
        if (selected) {
            const auto cell = Shared::cellFocusFor(Shared::Generation::Gen3, state.focus);
            drawFocus(fb, midX + cell.x, yy - 5, cell.width, 30, true);
        }
        fb.drawText(midX + 18, yy, statLabels[static_cast<std::size_t>(row)], Colors::TextDim, TextStyle::Caption);
        fb.drawText(midX + 126, yy, std::to_string(p.ivs[static_cast<std::size_t>(row)]), Colors::Text, TextStyle::Caption);
        fb.drawText(midX + 190, yy, std::to_string(p.evs[static_cast<std::size_t>(row)]), Colors::Text, TextStyle::Caption);
        fb.drawText(midX + 274, yy, state.previewError.empty() ? std::to_string(p.calculatedStats[static_cast<std::size_t>(row)]) : "—", Colors::Accent, TextStyle::Caption);
    }
    fb.drawHDivider(midX + 14, y + 310, midW - 28);
    fb.drawText(midX + 18, y + 318, "PID-LINKED / DERIVED (READ ONLY)",
                Colors::TextDim, TextStyle::Caption);
    const std::array<std::pair<const char*,std::string>,5> derived{{
        {"Nature", Trainer::getNatureName(p.nature)},
        {"Gender", genderText(p.gender)},
        {"Shiny", p.shiny ? "Yes" : "No"},
        {"Ability", Trainer::getAbilityName(p.ability)},
        {"PID", std::to_string(p.pid)},
    }};
    for (int i = 0; i < 5; ++i) {
        const int row = i + 6;
        const int yy = y + 346 + i * 35;
        const bool selected = false; // PID-correlated rows are intentionally frozen.
        drawFocus(fb, midX + 8, yy - 5, midW - 16, 29, selected);
        fb.drawText(midX + 18, yy, derived[static_cast<std::size_t>(i)].first,
                    Colors::TextDim, TextStyle::Caption);
        const Color valueColor = i == 1 ? genderValueColor(p.gender) : Colors::Text;
        fb.drawText(midX + 130, yy, derived[static_cast<std::size_t>(i)].second,
                    valueColor, TextStyle::Caption);
    }

    drawPanel(fb, rightX, y, rightW, h, "MOVES");
    fb.drawText(rightX + 186, y + 42, "PP", Colors::TextDim, TextStyle::Caption);
    fb.drawText(rightX + 242, y + 42, "Ups", Colors::TextDim, TextStyle::Caption);
    for (int row = 0; row < 4; ++row) {
        const int yy = y + 62 + row * 32;
        const bool selected = state.session.editable() &&
                              state.focus.panel == Shared::Panel::Moves &&
                              state.focus.row == row;
        if (selected) {
            const auto cell = Shared::cellFocusFor(Shared::Generation::Gen3, state.focus);
            drawFocus(fb, rightX + cell.x, yy - 5, cell.width, 32, true);
        }
        const uint16_t move = p.moves[static_cast<std::size_t>(row)];
        fb.drawText(rightX + 18, yy, move == 0 ? "Empty" : Names::getMoveName(move),
                    Colors::Text, TextStyle::Caption);
        fb.drawText(rightX + 186, yy,
                    std::to_string(p.pp[static_cast<std::size_t>(row)]),
                    Colors::Text, TextStyle::Caption);
        fb.drawText(rightX + 242, yy,
                    std::to_string(p.ppUps[static_cast<std::size_t>(row)]),
                    Colors::Text, TextStyle::Caption);

        bool existingSource = false;
        for (uint16_t sourceMove : state.session.baseline.moves)
            if (sourceMove != 0 && sourceMove == move) existingSource = true;
        const auto descriptor =
            PokeVault::Integration::Gen3EditorProvider::descriptorForSource(screen.sourceGameId, true);
        auto result = PokeBank::UIModel::ExactFormatEditor::MoveCompatibilityResult::Invalid;
        if (descriptor)
            result = descriptor->moves.evaluate(
                {screen.sourceGameId, p.species, 0, move, existingSource});
        const char* status = "Invalid";
        Color statusColor = Colors::Error;
        using MoveResult = PokeBank::UIModel::ExactFormatEditor::MoveCompatibilityResult;
        if (result == MoveResult::Compatible) {
            status = "OK";
            statusColor = Colors::Success;
        } else if (result == MoveResult::PreserveExisting) {
            status = "Preserved";
            statusColor = Colors::Warning;
        } else if (result == MoveResult::Unsupported) {
            status = "Transfer";
            statusColor = Colors::Warning;
        }
        int sw = 0, sh = 0;
        fb.measureText(status, sw, sh, TextStyle::Caption);
        fb.drawText(rightX + rightW - 18 - sw, yy, status, statusColor, TextStyle::Caption);
    }
    fb.drawHDivider(rightX + 14, y + 184, rightW - 28);
    fb.drawText(rightX + 18, y + 194,
                state.session.editable() ? "A: choose exact Gen III move" : "Move compatibility: checked",
                state.session.editable() ? Colors::Success : Colors::TextDim, TextStyle::Caption);
    fb.drawText(rightX + 18, y + 220,
                "OK = native   Transfer/Preserved = warning", Colors::TextDim, TextStyle::Caption);
    struct DataRow { std::string label, value; };
    const std::vector<DataRow> rows{
        {"Origin", Enums::getOriginGameName(p.originGame)},
        {"Language", Enums::getLanguageName(p.language)},
        {"Ball", Enums::getBallName(p.ball)},
        {"Met Location", metLocationText(p)},
        {"PID fields", "READ ONLY"},
        {"Legality", "Encounter not checked"}
    };
    SharedPokemonShell::drawDataAndGraph(fb, rightX, y + 244, rightW, h - 256,
        "GEN III DATA", rows, state.previewError.empty() ? p.calculatedStats : std::array<uint16_t,6>{});

    if (state.session.mode == SessionModel::Mode::View) {
        drawNavBar(fb, {{"B", "Back"}, {"L/R", "Panel"}});
    } else if (state.session.mode == SessionModel::Mode::Create) {
        drawNavBar(fb, {{"D-pad", "Navigate"}, {"L/R", "Panel"}, {"A", "Edit"},
                        {"X", "Add"}, {"B", "Exit"}});
    } else {
        drawNavBar(fb, {{"D-pad", "Navigate"}, {"L/R", "Panel"}, {"A", "Edit"},
                        {"X", "Keep"}, {"B", "Exit"}});
    }

    if (state.session.confirmExit) {
        constexpr int w = 620, hh = 178;
        const int xx = (fb.getWidth() - w) / 2;
        const int yy = (fb.getHeight() - hh) / 2;
        fb.drawFilledRoundedRect(xx, yy, w, hh, 16, Colors::PanelAlt);
        fb.drawRoundedRect(xx, yy, w, hh, 16, Colors::FocusBorder, 2);
        const bool create = state.session.mode == SessionModel::Mode::Create;
        const auto kind = create
            ? PokeBank::UIModel::PokemonEditorExitGuard::SessionKind::Create
            : PokeBank::UIModel::PokemonEditorExitGuard::SessionKind::Edit;
        fb.drawText(xx + 28, yy + 24, PokeBank::UIModel::PokemonEditorExitGuard::heading(kind),
                    Colors::Text, TextStyle::Heading);
        fb.drawText(xx + 28, yy + 70,
                    create ? "A Add Staged    X Discard Draft    B Continue Editing"
                           : "A Keep Staged   X Discard Session  B Continue Editing",
                    Colors::TextDim, TextStyle::Body);
        fb.drawText(xx + 28, yy + 112, "Original RetroArch save remains immutable.",
                    Colors::TextDim, TextStyle::Caption);
    }

    if (state.moveEditor) drawMoveEditor(fb, state);
    if (state.speciesPicker) {
        constexpr int px = 110, py = 56, pw = 1060, ph = 600;
        fb.drawFilledRoundedRect(px, py, pw, ph, 18, Colors::Panel);
        fb.drawRoundedRect(px, py, pw, ph, 18, Colors::FocusBorder, 2);
        fb.drawText(px + 24, py + 16, "Choose Species — Generation III", Colors::Text, TextStyle::Heading);
        SharedSpeciesPicker::drawContent(
            fb, px + 18, py + 12, state.speciesPreview, 386, p.shiny,
            [](uint16_t species) {
                return std::to_string(species) + "  " + Names::getSpeciesName(species);
            },
            [](uint16_t species) {
                return std::string(Names::getSpeciesName(species));
            });
        drawNavBar(fb, {{"D-pad", "Browse"}, {"A", "Choose"}, {"B", "Cancel"}});
    }
    if (state.valuePicker) drawValuePicker(screen, fb, state);
}

void drawLegality(TrainerViewScreen& screen, PKSEFramebuffer& fb, State& state) {
    screen.drawGSCOverlay(fb);
    fb.drawFilledRect(0, 0, fb.getWidth(), fb.getHeight() - 48, Color(0, 0, 0, 120));
    constexpr int x = 220, y = 92, w = 840, h = 480;
    fb.drawFilledRoundedRect(x, y, w, h, 18, Colors::Panel);
    fb.drawRoundedRect(x, y, w, h, 18, Colors::Divider, 1);
    fb.drawText(x + 24, y + 18, "Legality & Provenance", Colors::Text, TextStyle::Heading);
    fb.drawText(x + 24, y + 52, "Editor guardrails active • full legality checker is a later milestone",
                Colors::TextDim, TextStyle::Caption);
    fb.drawText(x + 34, y + 112, "Exact source", Colors::TextDim);
    fb.drawText(x + 270, y + 112, std::string(Context::exactGameName(screen.sourceGameId)), Colors::Text);
    fb.drawText(x + 34, y + 158, "Source mutation", Colors::TextDim);
    fb.drawText(x + 270, y + 158, "HARD DISABLED", Colors::Accent);
    fb.drawText(x + 34, y + 204, "Encounter guardrails", Colors::TextDim);
    fb.drawText(x + 270, y + 204, "Game/species/location/level constrained", Colors::Text);
    fb.drawText(x + 34, y + 250, "PID-linked fields", Colors::TextDim);
    fb.drawText(x + 270, y + 250, "READ ONLY", Colors::Text);
    fb.drawText(x + 34, y + 296, "Deep encounter legality", Colors::TextDim);
    fb.drawText(x + 270, y + 296, "Not checked", Colors::Warning);
    drawNavBar(fb, {{"B", "Back"}});
}

void drawActions(TrainerViewScreen& screen, PKSEFramebuffer& fb, State& state) {
    auto* staged = editor(screen);
    if (!staged) return;
    std::string error;
    const bool occupied = staged->boxedPokemon(static_cast<std::size_t>(state.box),
                                                static_cast<std::size_t>(state.slot), error).has_value();
    const auto actions = Shared::actionsForSlot(occupied, {true, true, true});

    // Match the accepted Gen I/Yellow box action sheet: keep the box visible,
    // dim it, then draw one compact red-outlined modal on top.
    screen.drawGSCOverlay(fb);
    fb.drawFilledRect(0, 0, fb.getWidth(), fb.getHeight() - kNavBarH, Color(0, 0, 0, 105));
    const int w = occupied ? 650 : 560;
    const int h = occupied ? 500 : 350;
    const int x = (fb.getWidth() - w) / 2;
    const int y = (fb.getHeight() - kNavBarH - h) / 2;
    const auto geometry = Shared::actionMenuGeometry();
    fb.drawFilledRoundedRect(x, y, w, h, 18, Colors::Panel);
    fb.drawRoundedRect(x, y, w, h, 18, Colors::FocusBorder, 2);
    fb.drawText(x + 24, y + 18, occupied ? "Pokemon Actions" : "Empty Slot",
                Colors::Text, TextStyle::Heading);
    fb.drawText(x + 24, y + 52,
        Context::contextLine("Box " + std::to_string(state.box + 1) + " / Slot " +
                             std::to_string(state.slot + 1),
                             screen.sourceGameId, Context::SurfaceMode::ViewReadOnly),
        Colors::TextDim, TextStyle::Caption);

    int rowY = y + geometry.rowStart;
    for (std::size_t i = 0; i < actions.count; ++i) {
        const bool selected = static_cast<int>(i) == state.actionRow;
        if (selected)
            fb.drawSelectionHighlight(x + 22, rowY, w - 44, geometry.rowHeight);
        const auto action = actions[i];
        const char* label = action == Shared::Action::Remove ? "Release" : Shared::actionLabel(action);
        fb.drawText(x + 38, rowY + 10, label, selected ? Colors::Text : Colors::TextDim);
        rowY += geometry.rowStep;
    }
    fb.drawText(x + 24, y + h - 38, "Original source is immutable",
                Colors::Accent, TextStyle::Caption);
    drawNavBar(fb, {{"D-pad/Stick", "Navigate"}, {"A", "Select"}, {"B", "Cancel"}});
}

void drawReview(TrainerViewScreen& screen, PKSEFramebuffer& fb, State&) {
    auto* staged = editor(screen);
    if (!staged) return;
    fb.drawFilledRect(0, 0, fb.getWidth(), fb.getHeight(), Colors::Background);
    fb.drawText(32, 24, "Review Pending Changes", Colors::Text, TextStyle::Heading);
    fb.drawText(32, 62, "Generation III staged workspace • original source bytes unchanged",
                Colors::TextDim, TextStyle::Caption);
    int y = 112;
    if (staged->pendingChanges().empty()) {
        fb.drawText(50, y, "No pending changes.", Colors::TextDim, TextStyle::Body);
    } else {
        for (const auto& change : staged->pendingChanges()) {
            if (y > 585) break;
            fb.drawFilledRoundedRect(44, y, fb.getWidth() - 88, 62, 10, Colors::Panel);
            fb.drawText(62, y + 10, change.label, Colors::Text, TextStyle::Caption);
            fb.drawText(62, y + 34, change.beforeValue + "  ->  " + change.afterValue,
                        Colors::TextDim, TextStyle::Caption);
            y += 72;
        }
    }
    drawNavBar(fb, {{"Y", "Discard All Staged"}, {"B", "Back"}});
}

void drawReleaseConfirm(TrainerViewScreen& screen, PKSEFramebuffer& fb, State& state) {
    fb.drawFilledRect(0, 0, fb.getWidth(), fb.getHeight(), Colors::Background);
    constexpr int w = 760, h = 260;
    const int x = (fb.getWidth() - w) / 2;
    const int y = (fb.getHeight() - h) / 2;
    fb.drawFilledRoundedRect(x, y, w, h, 18, Colors::Panel);
    fb.drawRoundedRect(x, y, w, h, 18, Colors::FocusBorder, 2);
    fb.drawText(x + 34, y + 30, "Release this Pokemon?", Colors::Text, TextStyle::Heading);
    fb.drawText(x + 34, y + 88,
        "Box " + std::to_string(state.box + 1) + " / Slot " + std::to_string(state.slot + 1),
        Colors::TextDim, TextStyle::Body);
    fb.drawText(x + 34, y + 130, "This removes it only from the PokeBank staged copy.",
                Colors::Text, TextStyle::Body);
    fb.drawText(x + 34, y + 166, "The original source save is never written.",
                Colors::TextDim, TextStyle::Caption);
    drawNavBar(fb, {{"A", "Confirm Release"}, {"B", "Cancel"}});
}

} // namespace

bool beginPassiveView(TrainerViewScreen& screen) {
    if (!screen.details.active) return false;
    auto* target = screen.detailsTargetPokemon();
    if (!target || target->getGameGroup() != Enums::GameVersion::FRLG || target->getData().size() < 80)
        return false;
    auto& state = stateFor(screen);
    if (state.passive && workspaceActive(state)) return true;
    auto* encrypted = Encryption::encryptArray3FRLG(target->getData());
    std::array<uint8_t,80> raw{};
    std::memcpy(raw.data(), encrypted, raw.size());
    delete[] encrypted;
    std::string error;
    auto record = Gen3::StagedPokemonEditor::decodeRecord(raw, error);
    if (!record) {
        screen.closeDetailsModal();
        screen.postStatus(error.empty() ? "Invalid native PK3 record" : error, 300);
        return true;
    }
    state.session.begin(*record, SessionModel::Mode::View);
    state.passive = true;
    resetWorkspaceFocus(state);
    return true;
}

bool ownsFrame(const TrainerViewScreen& screen) noexcept {
    return ownsFrameImpl(screen);
}

bool handleInput(TrainerViewScreen& screen, u64 down, u64 held,
                 int stickX, int stickY, const TouchInput& touch) {
    (void)held;
    (void)stickX;
    (void)stickY;
    (void)touch;
    if (!isGen3Gba(screen)) return false;

    beginPassiveView(screen);
    auto& state = stateFor(screen);
    if (state.legality) {
        if (down & HidNpadButton_B) {
            state.legality = false;
            state.actions = true;
            screen.controllerNavigation.reset();
        }
        return true;
    }
    if (handleReleaseConfirm(screen, state, down)) return true;
    if (handleReview(screen, state, down)) return true;
    if (handleWorkspace(screen, state, down)) return true;
    if (handleActions(screen, state, down)) return true;

    if (!boxEntrySurface(screen)) return false;
    const bool occupied = selectedOccupied(screen);
    const auto activation = Shared::boxActivation(
        occupied, (down & HidNpadButton_A) != 0, (down & HidNpadButton_X) != 0);
    if (activation == Shared::BoxActivation::Actions) {
        beginActions(screen);
        return true;
    }
    if (activation == Shared::BoxActivation::Add) {
        return beginCreate(screen, screen.selectedBoxIndex, screen.selectedItemIndex);
    }
    return false;
}

bool draw(TrainerViewScreen& screen, PKSEFramebuffer& fb) {
    if (isGen3Gba(screen)) beginPassiveView(screen);
    if (!ownsFrameImpl(screen)) return false;
    auto& state = stateFor(screen);
    if (state.confirmRelease) drawReleaseConfirm(screen, fb, state);
    else if (state.legality) drawLegality(screen, fb, state);
    else if (state.review) drawReview(screen, fb, state);
    else if (workspaceActive(state)) drawWorkspace(screen, fb, state);
    else if (state.actions) drawActions(screen, fb, state);
    return true;
}

} // namespace UI::Gen3SharedEditorSurface
