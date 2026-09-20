#include "UI/SharedPokemonShell.h"
#include "Pokemon/PersonalInfoTable.h"
#include "Encryption/Encryption3FRLG.h"
#include <cstring>
#include "Integration/Gen3/Gen3ExactFormatEditorProvider.h"
#include "Integration/Gen3/Gen3LearnsetTable.h"
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
#include <string>
#include <utility>
#include <vector>

namespace UI::Gen3SharedEditorSurface {
namespace {

namespace Shared = PokeBank::UIModel::SharedPokemonEditor;
namespace SessionModel = PokeBank::UIModel::Gen3SharedEditor;
namespace Gen3 = PokeVault::Integration::Gen3;
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
    std::vector<uint16_t> pickerValues;
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
    if (name && *name) return name;
    return p.metLocation == 0 ? "(none)" : "Unknown (" + std::to_string(p.metLocation) + ")";
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
               row == 8 || row == 9 || row == 11 || row == 13;
    return row == 0 || row == 1 || row == 2 || row == 3 || row == 4 ||
           row == 7 || row == 8 || row == 9 || row == 11 || row == 12 ||
           row == 13 || row == 14;
}

std::string pickerValueLabel(const State& state, uint16_t value) {
    switch (state.pickerTarget) {
        case PickerTarget::HeldItem: return Names::getItemNameG3(value);
        case PickerTarget::Language: return Enums::getLanguageName(static_cast<uint8_t>(value));
        case PickerTarget::Ball: return Enums::getBallName(static_cast<uint8_t>(value));
        case PickerTarget::MetLocation: {
            const char* name = Names::getMetLocationName(state.session.working.originGame, value);
            return (name && *name) ? std::string(name) : "Unknown (" + std::to_string(value) + ")";
        }
        case PickerTarget::Move:
            return value == 0 ? "Empty" : std::string(Names::getMoveName(value));
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
        const auto table = Names::getLocationTable(state.session.working.originGame);
        if (table.names) {
            for (std::size_t id = 0; id < table.count; ++id)
                if (table.names[id] && *table.names[id])
                    state.pickerValues.push_back(static_cast<uint16_t>(id));
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
        case PickerTarget::MetLocation: p.metLocation = value; break;
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

    int current = 0, minimum = 0, maximum = 255;
    const char* title = "Generation III value";
    switch (row) {
        case 8: current = p.friendship; title = "Friendship"; break;
        case 12: current = p.metLevel; maximum = 100; title = "Met Level"; break;
        case 14: current = p.pokerus; title = "Pokerus byte"; break;
        default: return;
    }
    const auto r = Utils::promptNumber(title, current, minimum, maximum);
    if (!r.accepted) return;
    switch (row) {
        case 8: p.friendship = static_cast<uint8_t>(r.value); break;
        case 12: p.metLevel = static_cast<uint8_t>(r.value); break;
        case 14: p.pokerus = static_cast<uint8_t>(r.value); break;
        default: break;
    }
}

void editStatCell(TrainerViewScreen& screen, State& state) {
    if (!state.session.editable() || state.session.mode == SessionModel::Mode::Create) {
        screen.postStatus("IV/EV creation controls are deferred; safe defaults will be used", 300);
        return;
    }
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

void editMoveCell(TrainerViewScreen& screen, State& state) {
    if (!state.session.editable()) return;
    const std::size_t slot = state.focus.row;
    if (state.focus.column == 0) {
        openValuePicker(screen, state, PickerTarget::Move, static_cast<int>(slot));
        return;
    }
    if (state.session.working.moves[slot] == 0) {
        screen.postStatus("Empty move slots keep PP and PP Ups at zero");
        return;
    }
    if (state.focus.column == 1) {
        const auto r = Utils::promptNumber("Current PP", state.session.working.pp[slot], 0, 99);
        if (r.accepted) state.session.working.pp[slot] = static_cast<uint8_t>(r.value);
    } else {
        const auto r = Utils::promptNumber("PP Ups", state.session.working.ppUps[slot], 0, 3);
        if (r.accepted) state.session.working.ppUps[slot] = static_cast<uint8_t>(r.value);
    }
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
        if (state.session.setSpecies(state.speciesPreview))
            state.speciesPicker = false;
        else
            screen.postStatus("Species is not representable in Generation III");
        return true;
    }
    return true;
}

bool handleWorkspace(TrainerViewScreen& screen, State& state, u64 down) {
    if (!workspaceActive(state)) return false;
    if (handleValuePicker(screen, state, down)) return true;
    if (handleSpeciesPicker(screen, state, down)) return true;

    if (state.session.confirmExit) {
        if (down & HidNpadButton_A) {
            if (keepWorkspace(screen)) state.session.confirmExit = false;
            return true;
        }
        if (down & HidNpadButton_X) {
            state.session.discardDraft();
            screen.postStatus("Generation III edit draft discarded; staged/source bytes unchanged");
            return true;
        }
        if (down & HidNpadButton_B) {
            state.session.continueEditing();
            return true;
        }
        return true;
    }

    if (down & HidNpadButton_L)
        state.focus = Shared::switchPanel(Shared::Generation::Gen3, state.focus, -1);
    if (down & HidNpadButton_R)
        state.focus = Shared::switchPanel(Shared::Generation::Gen3, state.focus, 1);
    if (down & HidNpadButton_Up)
        state.focus = Shared::moveVertical(Shared::Generation::Gen3, state.focus, -1);
    if (down & HidNpadButton_Down)
        state.focus = Shared::moveVertical(Shared::Generation::Gen3, state.focus, 1);
    if (down & HidNpadButton_Left)
        state.focus = Shared::moveColumn(Shared::Generation::Gen3, state.focus, -1);
    if (down & HidNpadButton_Right)
        state.focus = Shared::moveColumn(Shared::Generation::Gen3, state.focus, 1);

    if (state.focus.panel == Shared::Panel::Details) state.detailsFocus = state.focus.row;

    if (down & HidNpadButton_B) {
        if (state.session.mode == SessionModel::Mode::View) {
            state.session.mode = SessionModel::Mode::None;
            if (state.passive) { screen.closeDetailsModal(); state.passive = false; }
        } else if (state.session.mode == SessionModel::Mode::Create) {
            state.session.discardDraft();
            screen.postStatus("Generation III Create draft cancelled; staged/source bytes unchanged");
        } else {
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
    const auto actions = Shared::actionsForSlot(occupied, {/*clone*/true, /*remove*/true, /*legality*/false});
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
        detailLabels.size(), state.detailsFocus, state.focus.panel == Shared::Panel::Details,
        [](std::size_t row) { return detailLabels[row]; },
        [&](std::size_t row) { return detailsValue(screen, p, static_cast<int>(row)); });
    drawPanel(fb, midX, y, midW, h, "STATS");
    fb.drawText(midX + 126, y + 50, "IV", Colors::TextDim, TextStyle::Caption);
    fb.drawText(midX + 190, y + 50, "EV", Colors::TextDim, TextStyle::Caption);
    fb.drawText(midX + 274, y + 50, "Stat", Colors::TextDim, TextStyle::Caption);
    constexpr std::array<const char*,6> statLabels{{"HP","Attack","Defense","Speed","Sp. Atk","Sp. Def"}};
    for (int row = 0; row < 6; ++row) {
        const int yy = y + 78 + row * 40;
        const bool selected = state.focus.panel == Shared::Panel::Values &&
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
    const std::array<std::pair<const char*,std::string>,5> derived{{
        {"Nature", std::to_string(p.nature)},
        {"Gender", genderText(p.gender)},
        {"Shiny", p.shiny ? "Yes" : "No"},
        {"Ability", std::to_string(p.ability) + " / slot " + std::to_string(p.abilityNumber)},
        {"PID", std::to_string(p.pid)},
    }};
    for (int i = 0; i < 5; ++i) {
        const int row = i + 6;
        const int yy = y + 328 + i * 35;
        const bool selected = state.focus.panel == Shared::Panel::Values &&
                              state.focus.row == row;
        drawFocus(fb, midX + 8, yy - 5, midW - 16, 29, selected);
        fb.drawText(midX + 18, yy, derived[static_cast<std::size_t>(i)].first,
                    Colors::TextDim, TextStyle::Caption);
        fb.drawText(midX + 130, yy, derived[static_cast<std::size_t>(i)].second,
                    Colors::Text, TextStyle::Caption);
    }

    drawPanel(fb, rightX, y, rightW, h, "MOVES");
    fb.drawText(rightX + 186, y + 42, "PP", Colors::TextDim, TextStyle::Caption);
    fb.drawText(rightX + 242, y + 42, "Ups", Colors::TextDim, TextStyle::Caption);
    for (int row = 0; row < 4; ++row) {
        const int yy = y + 62 + row * 32;
        const bool selected = state.focus.panel == Shared::Panel::Moves &&
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
    }
    fb.drawHDivider(rightX + 14, y + 184, rightW - 28);
    fb.drawText(rightX + 18, y + 194, "Move compatibility: Not checked", Colors::TextDim, TextStyle::Caption);
    fb.drawText(rightX + 18, y + 220, "Move selection: READ ONLY", Colors::TextDim, TextStyle::Caption);
    struct DataRow { std::string label, value; };
    const std::vector<DataRow> rows{
        {"Origin", std::to_string(p.originGame)}, {"Language", std::to_string(p.language)},
        {"Ball", std::to_string(p.ball)}, {"PID fields", "READ ONLY"},
        {"Legality", "Not checked"},
        {"Preview", state.previewError.empty() ? "Exact PK3" : "Invalid draft"}
    };
    SharedPokemonShell::drawDataAndGraph(fb, rightX, y + 244, rightW, h - 256,
        "GEN III DATA", rows, state.previewError.empty() ? p.calculatedStats : std::array<uint16_t,6>{});

    if (state.session.mode == SessionModel::Mode::View) {
        drawNavBar(fb, {{"B", "Back"}, {"L/R", "Panel"}});
    } else if (state.session.mode == SessionModel::Mode::Create) {
        drawNavBar(fb, {{"D-pad", "Navigate"}, {"L/R", "Panel"}, {"A", "Edit"},
                        {"X", "Add"}, {"B", "Cancel Draft"}});
    } else {
        drawNavBar(fb, {{"D-pad", "Navigate"}, {"L/R", "Panel"}, {"A", "Edit"},
                        {"X", "Keep"}, {"B", "Back"}});
    }

    if (state.session.confirmExit) {
        constexpr int w = 620, hh = 178;
        const int xx = (fb.getWidth() - w) / 2;
        const int yy = (fb.getHeight() - hh) / 2;
        fb.drawFilledRoundedRect(xx, yy, w, hh, 16, Colors::PanelAlt);
        fb.drawRoundedRect(xx, yy, w, hh, 16, Colors::FocusBorder, 2);
        fb.drawText(xx + 28, yy + 24, "Keep staged edit?", Colors::Text, TextStyle::Heading);
        fb.drawText(xx + 28, yy + 70, "A Keep    X Discard Draft    B Continue Editing",
                    Colors::TextDim, TextStyle::Body);
        fb.drawText(xx + 28, yy + 112, "Original RetroArch save remains immutable.",
                    Colors::TextDim, TextStyle::Caption);
    }

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
}

void drawActions(TrainerViewScreen& screen, PKSEFramebuffer& fb, State& state) {
    auto* staged = editor(screen);
    if (!staged) return;
    std::string error;
    const bool occupied = staged->boxedPokemon(static_cast<std::size_t>(state.box),
                                                static_cast<std::size_t>(state.slot), error).has_value();
    const auto actions = Shared::actionsForSlot(occupied, {true, true, false});

    fb.drawFilledRect(0, 0, fb.getWidth(), fb.getHeight(), Colors::Background);
    fb.drawText(32, 24, occupied ? "Pokemon Actions" : "Empty Slot", Colors::Text, TextStyle::Heading);
    fb.drawText(32, 62,
        Context::contextLine("Box " + std::to_string(state.box + 1) + " / Slot " +
                             std::to_string(state.slot + 1),
                             screen.sourceGameId, Context::SurfaceMode::ViewReadOnly),
        Colors::TextDim, TextStyle::Caption);

    constexpr int x = 210, y = 112, w = 860, rowH = 58;
    for (std::size_t i = 0; i < actions.count; ++i) {
        const int yy = y + static_cast<int>(i) * (rowH + 8);
        const bool selected = static_cast<int>(i) == state.actionRow;
        fb.drawFilledRoundedRect(x, yy, w, rowH, 10, selected ? Colors::AccentDim : Colors::Panel);
        if (selected) fb.drawRoundedRect(x, yy, w, rowH, 10, Colors::FocusBorder, 2);
        const auto action = actions[i];
        const char* label = action == Shared::Action::Remove ? "Release" : Shared::actionLabel(action);
        fb.drawText(x + 22, yy + 17, label, Colors::Text, TextStyle::Body);
        std::string note;
        if (action == Shared::Action::View) note = "Read-only shared summary";
        else if (action == Shared::Action::Edit) note = "PokeBank staged copy only";
        else if (action == Shared::Action::Clone) note = "Copy PK3 to an empty native slot";
        else if (action == Shared::Action::Remove) note = "Explicit confirmation; staged only";
        else if (action == Shared::Action::Add) note = "Create draft in this exact empty slot";
        else if (action == Shared::Action::Review) note = std::to_string(staged->pendingChanges().size()) + " pending change(s)";
        else note = "Return to boxes";
        int nw = 0, nh = 0;
        fb.measureText(note, nw, nh, TextStyle::Caption);
        fb.drawText(x + w - 22 - nw, yy + 20, note, Colors::TextDim, TextStyle::Caption);
    }
    drawNavBar(fb, {{"Up/Down", "Navigate"}, {"A", "Select"}, {"B", "Cancel"}});
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
    else if (state.review) drawReview(screen, fb, state);
    else if (workspaceActive(state)) drawWorkspace(screen, fb, state);
    else if (state.actions) drawActions(screen, fb, state);
    return true;
}

} // namespace UI::Gen3SharedEditorSurface
