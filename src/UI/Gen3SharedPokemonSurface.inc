#include "Integration/Gen3/Gen3ExactFormatEditorProvider.h"
#include "Legacy/FRLGReadOnlyTrainer.h"
#include "Names/MoveNames.h"
#include "Names/SpeciesNames.h"
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

struct State {
    const TrainerViewScreen* owner = nullptr;
    bool actions = false;
    bool review = false;
    bool confirmRelease = false;
    bool speciesPicker = false;
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
    return state.actions || state.review || state.confirmRelease ||
           state.speciesPicker || workspaceActive(state);
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
}

void beginActions(TrainerViewScreen& screen) {
    auto& state = stateFor(screen);
    state.actions = true;
    state.review = false;
    state.confirmRelease = false;
    state.speciesPicker = false;
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
    state.session.begin(*pokemon, mode);
    state.actions = false;
    state.review = false;
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
    state.confirmRelease = false;
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

std::string exactOriginText(const TrainerViewScreen& screen,
                            const Gen3::StagedPokemonRecord& p) {
    const auto game = Context::exactGameName(screen.sourceGameId);
    return std::string(game.empty() ? "Gen III" : game) + " / native " +
           std::to_string(p.originGame);
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
        case 7: return std::to_string(p.heldItem);
        case 8: return std::to_string(p.friendship);
        case 9: return std::to_string(p.language);
        case 10: return exactOriginText(screen, p);
        case 11: return std::to_string(p.ball);
        case 12: return std::to_string(p.metLevel);
        case 13: return std::to_string(p.metLocation);
        case 14: return std::to_string(p.pokerus);
        default: return {};
    }
}

constexpr std::array<const char*,15> detailLabels{{
    "Species", "Nickname", "Level", "Experience", "OT", "TID", "SID",
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

    int current = 0, minimum = 0, maximum = 255;
    const char* title = "Generation III value";
    switch (row) {
        case 7: current = p.heldItem; maximum = 376; title = "Held Item ID"; break;
        case 8: current = p.friendship; title = "Friendship"; break;
        case 9: current = p.language; minimum = 1; maximum = 7; title = "Language ID"; break;
        case 11: current = p.ball; minimum = 1; maximum = 12; title = "Ball ID"; break;
        case 12: current = p.metLevel; maximum = 100; title = "Met Level"; break;
        case 13: current = p.metLocation; maximum = 255; title = "Met Location"; break;
        case 14: current = p.pokerus; title = "Pokerus byte"; break;
        default: return;
    }
    const auto r = Utils::promptNumber(title, current, minimum, maximum);
    if (!r.accepted) return;
    switch (row) {
        case 7: p.heldItem = static_cast<uint16_t>(r.value); break;
        case 8: p.friendship = static_cast<uint8_t>(r.value); break;
        case 9: p.language = static_cast<uint8_t>(r.value); break;
        case 11: p.ball = static_cast<uint8_t>(r.value); break;
        case 12: p.metLevel = static_cast<uint8_t>(r.value); break;
        case 13: p.metLocation = static_cast<uint16_t>(r.value); break;
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
    if (!state.session.editable() || state.session.mode == SessionModel::Mode::Create) {
        screen.postStatus("Generation III move selection is read-only in this first safe candidate", 300);
        return;
    }
    const std::size_t slot = state.focus.row;
    if (state.focus.column == 0) {
        screen.postStatus("Move selection is READ ONLY until an exact learnset provider is pinned", 360);
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

    if (down & HidNpadButton_B) {
        if (state.session.mode == SessionModel::Mode::View) {
            state.session.mode = SessionModel::Mode::None;
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
    return Context::contextLine(
        "Box " + std::to_string(state.box + 1) + " / Slot " + std::to_string(state.slot + 1),
        screen.sourceGameId, mode);
}

void drawPanel(PKSEFramebuffer& fb, int x, int y, int w, int h, const char* title) {
    fb.drawFilledRoundedRect(x, y, w, h, 14, Colors::Panel);
    fb.drawRoundedRect(x, y, w, h, 14, Colors::PanelBorder, 1);
    fb.drawText(x + 14, y + 12, title, Colors::Text, TextStyle::Heading);
}

void drawFocus(PKSEFramebuffer& fb, int x, int y, int w, int h, bool selected) {
    if (selected) fb.drawRoundedRect(x, y, w, h, 6, Colors::FocusBorder, 2);
}

void drawWorkspace(TrainerViewScreen& screen, PKSEFramebuffer& fb, State& state) {
    const auto& p = state.session.working;
    screen.touchButtons.clear();
    fb.drawFilledRect(0, 0, fb.getWidth(), fb.getHeight(), Colors::Background);

    const std::string title =
        p.nickname.empty() ? std::string(Names::getSpeciesName(p.species)) : p.nickname;
    fb.drawText(28, 16, title, Colors::Text, TextStyle::Heading);
    const std::string levelDex =
        "Lv. " + std::to_string(p.level) + "     No. " + std::to_string(p.species);
    int rw = 0, rh = 0;
    fb.measureText(levelDex, rw, rh);
    fb.drawText(fb.getWidth() - 28 - rw, 24, levelDex, Colors::TextDim);
    const auto surface = state.session.mode == SessionModel::Mode::View ? Shared::Surface::View :
                         state.session.mode == SessionModel::Mode::Edit ? Shared::Surface::Edit :
                         Shared::Surface::CreateDraft;
    const std::string mode = Shared::titleFor(surface);
    int mw = 0, mh = 0;
    fb.measureText(mode, mw, mh, TextStyle::Caption);
    fb.drawText((fb.getWidth() - mw) / 2, 23, mode, Colors::Accent, TextStyle::Caption);
    fb.drawFilledRect(0, 60, fb.getWidth(), 2, Colors::Accent);
    fb.drawText(28, 67, surfaceContext(screen, state), Colors::TextDim, TextStyle::Caption);

    constexpr int y = 96, h = 514;
    constexpr int leftX = 24, leftW = 310;
    constexpr int gap = 14;
    constexpr int midX = leftX + leftW + gap, midW = 410;
    constexpr int rightX = midX + midW + gap;
    const int rightW = fb.getWidth() - 24 - rightX;

    drawPanel(fb, leftX, y, leftW, h, "DETAILS");
    const auto detailWindow = Shared::scrollWindow(detailLabels.size(), 9, state.focus.row);
    for (std::size_t i = 0; i < detailWindow.count; ++i) {
        const std::size_t row = detailWindow.first + i;
        const int yy = y + 54 + static_cast<int>(i) * 48;
        const bool selected = state.focus.panel == Shared::Panel::Details &&
                              state.focus.row == row;
        drawFocus(fb, leftX + 8, yy - 4, leftW - 16, 36, selected);
        fb.drawText(leftX + 16, yy, detailLabels[row], Colors::TextDim, TextStyle::Caption);
        std::string value = detailsValue(screen, p, static_cast<int>(row));
        if (value.size() > 22) value.resize(22);
        fb.drawText(leftX + 112, yy, value, Colors::Text, TextStyle::Caption);
    }

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
            const auto cell = Shared::cellFocus(state.focus);
            drawFocus(fb, midX + cell.x, yy - 5, cell.width, 30, true);
        }
        fb.drawText(midX + 18, yy, statLabels[static_cast<std::size_t>(row)], Colors::TextDim, TextStyle::Caption);
        fb.drawText(midX + 126, yy, std::to_string(p.ivs[static_cast<std::size_t>(row)]), Colors::Text, TextStyle::Caption);
        fb.drawText(midX + 190, yy, std::to_string(p.evs[static_cast<std::size_t>(row)]), Colors::Text, TextStyle::Caption);
        fb.drawText(midX + 274, yy, std::to_string(p.calculatedStats[static_cast<std::size_t>(row)]), Colors::Accent, TextStyle::Caption);
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
    fb.drawText(rightX + rightW - 174, y + 50, "PP", Colors::TextDim, TextStyle::Caption);
    fb.drawText(rightX + rightW - 98, y + 50, "Ups", Colors::TextDim, TextStyle::Caption);
    for (int row = 0; row < 4; ++row) {
        const int yy = y + 78 + row * 56;
        const bool selected = state.focus.panel == Shared::Panel::Moves &&
                              state.focus.row == row;
        if (selected) {
            const auto cell = Shared::cellFocus(state.focus);
            drawFocus(fb, rightX + cell.x, yy - 5, cell.width, 32, true);
        }
        const uint16_t move = p.moves[static_cast<std::size_t>(row)];
        fb.drawText(rightX + 18, yy, move == 0 ? "Empty" : Names::getMoveName(move),
                    Colors::Text, TextStyle::Caption);
        fb.drawText(rightX + rightW - 174, yy,
                    std::to_string(p.pp[static_cast<std::size_t>(row)]),
                    Colors::Text, TextStyle::Caption);
        fb.drawText(rightX + rightW - 98, yy,
                    std::to_string(p.ppUps[static_cast<std::size_t>(row)]),
                    Colors::Text, TextStyle::Caption);
    }
    fb.drawText(rightX + 18, y + 324, "Move selection: READ ONLY", Colors::TextDim, TextStyle::Caption);
    fb.drawText(rightX + 18, y + 350, "Exact learnset provider deferred", Colors::TextDim, TextStyle::Caption);
    fb.drawText(rightX + 18, y + 392, "Nature / Gender / Shiny", Colors::TextDim, TextStyle::Caption);
    fb.drawText(rightX + 18, y + 416, "Ability / PID: READ ONLY", Colors::TextDim, TextStyle::Caption);
    fb.drawText(rightX + 18, y + 448, "Legality: Not Checked", Colors::TextDim, TextStyle::Caption);

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
    if (!ownsFrameImpl(screen)) return false;
    auto& state = stateFor(screen);
    if (state.confirmRelease) drawReleaseConfirm(screen, fb, state);
    else if (state.review) drawReview(screen, fb, state);
    else if (workspaceActive(state)) drawWorkspace(screen, fb, state);
    else if (state.actions) drawActions(screen, fb, state);
    return true;
}

} // namespace UI::Gen3SharedEditorSurface
