#include "UI/ClassicSpeciesLevelPolicy.h"
#include "Integration/Gen1/Gen1MoveCompatibility.h"
#include "Integration/Gen1/Gen1Shiny.h"
#include "UI/Gen1PokemonDetailsPresentation.h"
#include "UI/Gen1PokemonEditorUX3Contract.h"
#include "UI/NavigationRepeat.h"

namespace UI::Gen1PokemonEditor {
namespace {

namespace UX3Contract = PokeBank::UIModel::Gen1Editor::UX3;
using PokeVault::Integration::Gen1::MoveCompatibility;
using PokeVault::Integration::Gen1::ShinyDVs;

struct UX3State {
    const TrainerViewScreen* owner = nullptr;
    std::string sourceGameId;
    PokeBank::UIModel::ControllerNavigation navigation;
    bool pickerPreviewShiny = false;
    std::optional<std::array<uint8_t,4>> preShinyDraftDVs;
    std::optional<std::array<uint8_t,4>> preShinyEditDVs;
};

UX3State& ux3StateFor(const TrainerViewScreen& screen) {
    static UX3State state;
    if (state.owner != &screen || state.sourceGameId != screen.sourceGameId) {
        state = {};
        state.owner = &screen;
        state.sourceGameId = screen.sourceGameId;
    }
    return state;
}

const char* ux3GameName(SourceGame game) noexcept {
    switch (game) {
        case SourceGame::Red: return "Pokemon Red";
        case SourceGame::Blue: return "Pokemon Blue";
        case SourceGame::Yellow: return "Pokemon Yellow";
    }
    return "this Generation I game";
}

bool ux3IsShiny(const PokemonRecord& pokemon) noexcept {
    return ShinyDVs::isShiny(storedDVs(pokemon));
}

bool ux3IsShiny(const BoxPokemonCreate& draft) noexcept {
    return ShinyDVs::isShiny(draft.dvs);
}

uint16_t ux3CurrentSpecies(TrainerViewScreen& screen) {
    auto& state = ux2StateFor(screen);
    if (state.mode == UX2Mode::AddDraft || state.subForAdd) return state.draft.species;
    std::string error;
    const auto pokemon = ux2SelectedPokemon(screen, error);
    return pokemon ? pokemon->species : 1;
}

int ux3JumpSectionRow(int row, int direction) noexcept {
    constexpr auto starts = UX3Contract::sectionStarts();
    int current = 0;
    for (int i = 0; i < static_cast<int>(starts.size()); ++i)
        if (row >= static_cast<int>(starts[static_cast<size_t>(i)])) current = i;
    current = (current + direction + static_cast<int>(starts.size())) % static_cast<int>(starts.size());
    return static_cast<int>(starts[static_cast<size_t>(current)]);
}

const char* ux3SectionForRow(int row) noexcept {
    if (row < 4) return "Identity";
    if (row < 8) return "Moves";
    if (row < 10) return "Stats";
    if (row < 12) return "Trainer";
    return "Actions";
}

void ux3ResetSessionState(TrainerViewScreen& screen) {
    auto& ux3 = ux3StateFor(screen);
    ux3.preShinyDraftDVs.reset();
    ux3.preShinyEditDVs.reset();
    ux3.pickerPreviewShiny = false;
}

void ux3SetDraftShiny(TrainerViewScreen& screen, bool shiny) {
    auto& state = ux2StateFor(screen);
    auto& ux3 = ux3StateFor(screen);
    const bool current = ux3IsShiny(state.draft);
    if (current == shiny) return;
    if (shiny) {
        ux3.preShinyDraftDVs = state.draft.dvs;
        state.draft.dvs = ShinyDVs::makeShiny(state.draft.dvs);
    } else {
        state.draft.dvs = ux3.preShinyDraftDVs.value_or(ShinyDVs::makeNormalFallback(state.draft.dvs));
        ux3.preShinyDraftDVs.reset();
    }
}

bool ux3SetExistingShiny(TrainerViewScreen& screen, bool shiny) {
    auto& ux3 = ux3StateFor(screen);
    std::string error;
    const auto pokemon = ux2SelectedPokemon(screen, error);
    if (!pokemon) {
        screen.postStatus(error.empty() ? "Pokemon unavailable" : error, 300);
        return false;
    }
    auto dvs = storedDVs(*pokemon);
    const bool current = ShinyDVs::isShiny(dvs);
    if (current == shiny) return true;
    if (shiny) {
        ux3.preShinyEditDVs = dvs;
        dvs = ShinyDVs::makeShiny(dvs);
    } else {
        dvs = ux3.preShinyEditDVs.value_or(ShinyDVs::makeNormalFallback(dvs));
        ux3.preShinyEditDVs.reset();
    }
    BoxPokemonEdit edit;
    edit.dvs = dvs;
    return ux2CommitEdit(screen, edit, shiny
        ? "Shiny appearance staged through Generation I DVs; source .srm unchanged"
        : "Normal appearance staged through Generation I DVs; source .srm unchanged");
}

std::vector<uint8_t> ux3MoveChoices(TrainerViewScreen& screen) {
    auto* e = editor(screen);
    if (!e) return {0};
    return MoveCompatibility::compatibleMoves(e->metadata().sourceGame, ux3CurrentSpecies(screen));
}

bool ux3DraftMovesCompatible(TrainerViewScreen& screen, std::optional<size_t>& badSlot) {
    auto& state = ux2StateFor(screen);
    auto* e = editor(screen);
    if (!e) { badSlot.reset(); return false; }
    badSlot = MoveCompatibility::firstIncompatible(e->metadata().sourceGame, state.draft.species, state.draft.moves);
    return !badSlot;
}

bool ux3StageAdd(TrainerViewScreen& screen) {
    auto& state = ux2StateFor(screen);
    auto* e = editor(screen);
    if (!e) return false;
    const auto bad = MoveCompatibility::firstIncompatible(e->metadata().sourceGame, state.draft.species, state.draft.moves);
    if (bad) {
        const auto move = state.draft.moves[*bad];
        screen.postStatus(std::string(Names::getMoveName(move)) + " can't be used by " +
                          Names::getSpeciesName(state.draft.species) + " in " + ux3GameName(e->metadata().sourceGame) +
                          ". Choose a compatible move before Stage Add.", 420);
        state.row = 4 + static_cast<int>(*bad);
        return false;
    }
    const bool ok = ux2StageAdd(screen);
    if (ok) ux3ResetSessionState(screen);
    return ok;
}

void ux3OpenSpeciesPicker(TrainerViewScreen& screen, uint16_t current, bool shiny) {
    auto& state = ux2StateFor(screen);
    auto& ux3 = ux3StateFor(screen);
    ux2OpenSpeciesPicker(state, current);
    ux3.pickerPreviewShiny = shiny;
}

bool ux3ApplySpeciesPicker(TrainerViewScreen& screen) {
    auto& state = ux2StateFor(screen);
    auto& ux3 = ux3StateFor(screen);
    const uint16_t species = static_cast<uint16_t>(state.pickerValue);
    if (state.mode == UX2Mode::AddDraft) {
        const bool beforeShiny = ux3IsShiny(state.draft);
        if (species != state.draft.species)
            state.draft.level = PokeBank::UIModel::classicSpeciesChangeLevel(true, state.draft.level);
        state.draft.nickname = PokeBank::UIModel::nicknameAfterSpeciesChange(state.draft.nickname, state.draft.species, species);
        state.draft.species = species; // Moves stay intact and are revalidated visibly below.
        if (beforeShiny != ux3.pickerPreviewShiny)
            ux3SetDraftShiny(screen, ux3.pickerPreviewShiny);
        state.picker = UX2Picker::None;
        return true;
    }

    if (state.mode == UX2Mode::Edit) {
        std::string error;
        const auto pokemon = ux2SelectedPokemon(screen, error);
        if (!pokemon) return false;
        const auto oldDVs = storedDVs(*pokemon);
        const bool beforeShiny = ShinyDVs::isShiny(oldDVs);
        BoxPokemonEdit edit;
        edit.species = species;
        if (species != pokemon->species)
            edit.level = PokeBank::UIModel::classicSpeciesChangeLevel(false, pokemon->level);
        const auto nickname = PokeBank::UIModel::nicknameAfterSpeciesChange(pokemon->nickname, pokemon->species, species);
        if (nickname != pokemon->nickname) edit.nickname = nickname;
        if (beforeShiny != ux3.pickerPreviewShiny) {
            auto nextDVs = oldDVs;
            if (ux3.pickerPreviewShiny) {
                ux3.preShinyEditDVs = oldDVs;
                nextDVs = ShinyDVs::makeShiny(nextDVs);
            } else {
                nextDVs = ux3.preShinyEditDVs.value_or(ShinyDVs::makeNormalFallback(nextDVs));
                ux3.preShinyEditDVs.reset();
            }
            edit.dvs = nextDVs;
        }
        const bool ok = ux2CommitEdit(screen, edit, "Species / appearance change staged; source .srm unchanged");
        if (ok) state.picker = UX2Picker::None;
        return ok;
    }
    return false;
}

void ux3HandleSpeciesPicker(TrainerViewScreen& screen, uint64_t down) {
    auto& state = ux2StateFor(screen);
    auto& ux3 = ux3StateFor(screen);
    if (down & HidNpadButton_Up) state.pickerValue = std::max(1, state.pickerValue - 1);
    if (down & HidNpadButton_Down) state.pickerValue = std::min(151, state.pickerValue + 1);
    if (down & HidNpadButton_L) state.pickerValue = std::max(1, state.pickerValue - 10);
    if (down & HidNpadButton_R) state.pickerValue = std::min(151, state.pickerValue + 10);
    if (down & HidNpadButton_Y) ux3.pickerPreviewShiny = !ux3.pickerPreviewShiny;
    if (down & HidNpadButton_B) { state.picker = UX2Picker::None; return; }
    if (down & HidNpadButton_A) ux3ApplySpeciesPicker(screen);
}

void ux3HandleMovePicker(TrainerViewScreen& screen, uint64_t down) {
    auto& state = ux2StateFor(screen);
    const auto choices = ux3MoveChoices(screen);
    if (choices.empty()) { state.picker = UX2Picker::None; return; }
    int index = 0;
    for (int i = 0; i < static_cast<int>(choices.size()); ++i)
        if (choices[static_cast<size_t>(i)] == state.pickerValue) { index = i; break; }
    if (down & HidNpadButton_Up) index = std::max(0, index - 1);
    if (down & HidNpadButton_Down) index = std::min(static_cast<int>(choices.size()) - 1, index + 1);
    if (down & HidNpadButton_L) index = std::max(0, index - 10);
    if (down & HidNpadButton_R) index = std::min(static_cast<int>(choices.size()) - 1, index + 10);
    state.pickerValue = choices[static_cast<size_t>(index)];
    if (down & HidNpadButton_B) { state.picker = UX2Picker::None; return; }
    if (!(down & HidNpadButton_A)) return;
    state.tempMove = static_cast<uint8_t>(state.pickerValue);
    state.tempPPUps = 0;
    state.tempPP = Editor::moveBasePP(state.tempMove);
    state.picker = UX2Picker::None;
}

void ux3HandleMainEditor(TrainerViewScreen& screen, uint64_t down) {
    auto& state = ux2StateFor(screen);
    const bool add = state.mode == UX2Mode::AddDraft;
    constexpr int rows = static_cast<int>(UX3Contract::workspaceRowCount());
    if (down & HidNpadButton_Up) state.row = (state.row + rows - 1) % rows;
    if (down & HidNpadButton_Down) state.row = (state.row + 1) % rows;
    if (down & HidNpadButton_L) state.row = ux3JumpSectionRow(state.row, -1);
    if (down & HidNpadButton_R) state.row = ux3JumpSectionRow(state.row, 1);
    if (add && (down & HidNpadButton_X)) { ux3StageAdd(screen); return; }
    if (down & HidNpadButton_B) {
        if (add) state.draft = BoxPokemonCreate{};
        ux3ResetSessionState(screen);
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
            ux3OpenSpeciesPicker(screen, add ? state.draft.species : pokemon->species,
                                 add ? ux3IsShiny(state.draft) : ux3IsShiny(*pokemon));
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
        case 3:
            if (add) ux3SetDraftShiny(screen, !ux3IsShiny(state.draft));
            else ux3SetExistingShiny(screen, !ux3IsShiny(*pokemon));
            break;
        case 4: case 5: case 6: case 7: ux2OpenMoveEditor(screen, state.row - 4, add); break;
        case 8: ux2OpenDVEditor(screen, add); break;
        case 9: ux2OpenStatExpEditor(screen, add); break;
        case 10: {
            const std::string initial = add
                ? (state.draft.otName.empty() ? e->view().trainer().name : state.draft.otName)
                : pokemon->originalTrainer;
            const auto result = Utils::promptText("Generation I OT", "1-7 supported Gen I characters", initial, 7);
            if (!result.accepted) break;
            if (add) state.draft.otName = result.text;
            else { BoxPokemonEdit edit; edit.otName = result.text; ux2CommitEdit(screen, edit, "OT change staged; source .srm unchanged"); }
            break;
        }
        case 11: {
            const int current = add ? state.draft.trainerId.value_or(e->view().trainer().trainerId) : pokemon->trainerId;
            const auto result = Utils::promptNumber("Generation I Trainer ID", current, 0, 65535);
            if (!result.accepted) break;
            if (add) state.draft.trainerId = static_cast<uint16_t>(result.value);
            else { BoxPokemonEdit edit; edit.trainerId = static_cast<uint16_t>(result.value); ux2CommitEdit(screen, edit, "TID change staged; source .srm unchanged"); }
            break;
        }
        case 12:
            if (add) ux3StageAdd(screen);
            else { state.mode = UX2Mode::Review; state.reviewRow = 0; }
            break;
        case 13:
            if (add) state.draft = BoxPokemonCreate{};
            ux3ResetSessionState(screen);
            state.mode = UX2Mode::Actions;
            state.row = 0;
            break;
    }
}

std::vector<std::string> ux3PendingLines(TrainerViewScreen& screen) {
    auto lines = pendingLines(screen);
    auto* e = editor(screen);
    if (!e) return lines;
    const auto original = parse(e->originalBytes(), e->metadata().sourceGame);
    if (!original || !original.save) return lines;
    const auto& beforeBoxes = original.save->boxes();
    const auto& afterBoxes = e->view().boxes();
    for (size_t box = 0; box < std::min(beforeBoxes.size(), afterBoxes.size()); ++box) {
        for (size_t slot = 0; slot < std::min(beforeBoxes[box].slots.size(), afterBoxes[box].slots.size()); ++slot) {
            const auto& before = beforeBoxes[box].slots[slot];
            const auto& after = afterBoxes[box].slots[slot];
            if (!before || !after) continue;
            const bool wasShiny = ux3IsShiny(*before);
            const bool nowShiny = ux3IsShiny(*after);
            if (wasShiny == nowShiny) continue;
            lines.insert(lines.begin(), "Box " + std::to_string(box + 1) + " Slot " + std::to_string(slot + 1) +
                         ": Appearance: " + (wasShiny ? "Shiny" : "Normal") + " -> " + (nowShiny ? "Shiny" : "Normal"));
        }
    }
    return lines;
}

std::string ux3MainRowLabel(int row) {
    static constexpr const char* labels[] = {
        "Species", "Nickname", "Level / EXP", "Shiny",
        "Move 1", "Move 2", "Move 3", "Move 4",
        "DVs", "Stat Exp", "OT", "Trainer ID",
        "Review / Stage Add", "Back / Cancel",
    };
    return row >= 0 && row < 14 ? labels[row] : "Field";
}

std::string ux3MainRowValue(TrainerViewScreen& screen, int row, bool add, const PokemonRecord* p) {
    auto& state = ux2StateFor(screen);
    auto* e = editor(screen);
    if (!e) return {};
    const auto& d = state.draft;
    if (row == 0) return speciesCompact(add ? d.species : p->species);
    if (row == 1) return add ? (d.nickname.empty() ? std::string(Names::getSpeciesName(d.species)) + " (default)" : d.nickname) : p->nickname;
    if (row == 2) return add ? ("Lv " + std::to_string(d.level) + " • EXP " + std::to_string(draftExperience(d)))
                              : ("Lv " + std::to_string(p->level) + " • EXP " + std::to_string(p->experience));
    if (row == 3) return (add ? ux3IsShiny(d) : ux3IsShiny(*p)) ? "Yes" : "No";
    if (row >= 4 && row <= 7) {
        const auto i = static_cast<size_t>(row - 4);
        const auto move = add ? d.moves[i] : p->moves[i];
        const auto pp = add ? d.pp[i] : p->pp[i];
        const auto ups = add ? d.ppUps[i] : p->ppUps[i];
        const bool compatible = MoveCompatibility::canLearnMove(e->metadata().sourceGame, add ? d.species : p->species, move);
        return (compatible ? "" : "[!] ") + ux2MoveSummary(move, pp, ups);
    }
    if (row == 8) {
        const auto dvs = add ? d.dvs : storedDVs(*p);
        return "HP " + std::to_string(Editor::derivedHPDV(dvs)) + " • A " + std::to_string(dvs[0]) +
            " D " + std::to_string(dvs[1]) + " S " + std::to_string(dvs[2]) + " Spc " + std::to_string(dvs[3]);
    }
    if (row == 9) return "Grouped five-stat values";
    if (row == 10) return add ? (d.otName.empty() ? e->view().trainer().name + " (default)" : d.otName) : p->originalTrainer;
    if (row == 11) return std::to_string(add ? d.trainerId.value_or(e->view().trainer().trainerId) : p->trainerId);
    if (row == 12) return add ? "Stage this draft" : "Open Pending Changes";
    if (row == 13) return add ? "Discard draft" : "Back to Actions";
    return {};
}

void ux3DrawEditor(TrainerViewScreen& screen, PKSEFramebuffer& fb, int x, int y, int w, int h, bool add) {
    auto& state = ux2StateFor(screen);
    auto* e = editor(screen);
    if (!e) return;
    std::string error;
    const auto pokemon = add ? std::optional<PokemonRecord>{} : ux2SelectedPokemon(screen, error);
    if (!add && !pokemon) { fb.drawText(x + 30, y + 90, error.empty() ? "Pokemon unavailable" : error, Colors::Accent); return; }

    const uint16_t species = add ? state.draft.species : pokemon->species;
    const auto dvs = add ? state.draft.dvs : storedDVs(*pokemon);
    const auto& statExp = add ? state.draft.statExperience : pokemon->statExperience;
    const uint8_t level = add ? state.draft.level : pokemon->level;
    const bool shiny = ShinyDVs::isShiny(dvs);
    const auto stats = Editor::calculateBattleStats(species, level, dvs, statExp).asArray();
    const auto badMove = MoveCompatibility::firstIncompatible(e->metadata().sourceGame, species,
        add ? state.draft.moves : pokemon->moves);

    fb.drawText(x + 24, y + 16, add ? "Create Pokemon — DRAFT" : "Edit Pokemon — STAGED", Colors::Text, TextStyle::Heading);
    fb.drawText(x + 24, y + 48,
        "Box " + std::to_string(state.box + 1) + " / Slot " + std::to_string(state.slot + 1) +
        "   •   " + ux3SectionForRow(state.row), Colors::TextDim, TextStyle::Caption);

    const int contentY = y + 78;
    const int contentH = h - 104;
    const int leftX = x + 20, leftW = 300;
    const int centerX = leftX + leftW + 14, centerW = 430;
    const int rightX = centerX + centerW + 14, rightW = x + w - 20 - rightX;
    drawPanelSurface(fb, leftX, contentY, leftW, contentH, false, 14);
    drawPanelSurface(fb, centerX, contentY, centerW, contentH, false, 14);
    drawPanelSurface(fb, rightX, contentY, rightW, contentH, false, 14);

    // Identity / live artwork panel.
    if (auto* sprite = SpriteManager::getSprite(species, shiny); sprite && sprite->data)
        fb.drawSpriteStaticContained(leftX + 45, contentY + 18, 210, 190,
                                     sprite->width, sprite->height, sprite->data, sprite->channels);
    else fb.drawText(leftX + 80, contentY + 105, "Sprite unavailable", Colors::TextDim, TextStyle::Caption);
    fb.drawText(leftX + 18, contentY + 220, speciesCompact(species), Colors::Text, TextStyle::Heading);
    const std::string nickname = add
        ? (state.draft.nickname.empty() ? std::string(Names::getSpeciesName(species)) : state.draft.nickname)
        : pokemon->nickname;
    fb.drawText(leftX + 18, contentY + 258, nickname, Colors::Text);
    fb.drawText(leftX + 18, contentY + 292, "Lv " + std::to_string(level) + "   EXP " +
                std::to_string(add ? draftExperience(state.draft) : pokemon->experience), Colors::TextDim, TextStyle::Caption);
    fb.drawText(leftX + 18, contentY + 326, shiny ? "Shiny: Yes" : "Shiny: No",
                shiny ? Colors::ShinyStar : Colors::Text, TextStyle::Caption);
    fb.drawText(leftX + 18, contentY + 360, add ? "Draft: Not staged yet" : "State: Staged workspace",
                Colors::Accent, TextStyle::Caption);
    fb.drawText(leftX + 18, contentY + 386, "Source save: Immutable", Colors::TextDim, TextStyle::Caption);

    // Main logical field column. It is one workspace, not wizard pages; contextual
    // move/DV/Stat Exp dialogs provide progressive disclosure for advanced values.
    fb.drawText(centerX + 18, contentY + 14, "Pokemon", Colors::Text, TextStyle::Heading);
    constexpr int rowH = 42;
    const int listY = contentY + 52;
    const int viewportH = contentH - 68;
    const int visible = std::max(1, viewportH / rowH);
    const int start = std::clamp(state.row - visible / 2, 0, std::max(0, 14 - visible));
    fb.setClipRect(centerX + 8, listY, centerW - 16, viewportH);
    for (int i = 0; i < visible && start + i < 14; ++i) {
        const int field = start + i;
        ux2DrawLogicalRow(fb, centerX + 10, listY + i * rowH, centerW - 20,
                          ux3MainRowLabel(field), ux3MainRowValue(screen, field, add, pokemon ? &*pokemon : nullptr),
                          field == state.row);
    }
    fb.clearClip();

    // Right information panel: player-facing status + the reusable true five-axis radar.
    fb.drawText(rightX + 18, contentY + 14, "Overview", Colors::Text, TextStyle::Heading);
    fb.drawText(rightX + 18, contentY + 54, "Move compatibility", Colors::TextDim, TextStyle::Caption);
    fb.drawText(rightX + rightW - 96, contentY + 54, badMove ? "Needs attention" : "OK",
                badMove ? Colors::Warning : Colors::Accent, TextStyle::Caption);
    fb.drawText(rightX + 18, contentY + 80, "Encounter legality", Colors::TextDim, TextStyle::Caption);
    fb.drawText(rightX + rightW - 108, contentY + 80, "Not checked", Colors::Text, TextStyle::Caption);
    fb.drawHDivider(rightX + 16, contentY + 116, rightW - 32);
    fb.drawText(rightX + 18, contentY + 132, "GEN I BATTLE STATS", Colors::Accent, TextStyle::Caption);
    ux2DrawRadar(fb, rightX + rightW / 2, contentY + 250, 88, stats);
    fb.drawText(rightX + 18, contentY + 350,
        "HP " + std::to_string(stats[0]) + "   Atk " + std::to_string(stats[1]) + "   Def " + std::to_string(stats[2]),
        Colors::Text, TextStyle::Caption);
    fb.drawText(rightX + 18, contentY + 378,
        "Spe " + std::to_string(stats[3]) + "   Special " + std::to_string(stats[4]), Colors::Text, TextStyle::Caption);
    if (badMove && add)
        fb.drawText(rightX + 18, contentY + 420, "Fix [!] move before Stage Add", Colors::Warning, TextStyle::Caption);
}

void ux3DrawPicker(TrainerViewScreen& screen, PKSEFramebuffer& fb, int x, int y, int w, int h) {
    auto& state = ux2StateFor(screen);
    auto& ux3 = ux3StateFor(screen);
    const bool speciesPicker = state.picker == UX2Picker::Species;
    fb.drawFilledRect(0, 0, fb.getWidth(), fb.getHeight() - 48, Color(0, 0, 0, 105));
    ux2DrawPanel(fb, x, y, w, h);
    fb.drawText(x + 24, y + 18, speciesPicker ? "Choose Species" : "Choose Move", Colors::Text, TextStyle::Heading);

    if (speciesPicker) {
        SharedSpeciesPicker::drawContent(fb, x, y, state.pickerValue, 151, ux3.pickerPreviewShiny,
            [](uint16_t species) { return UX::speciesPickerRow(species, Names::getSpeciesName(species)); },
            [](uint16_t species) { return speciesCompact(species); });
        return;
    }

    const auto choices = ux3MoveChoices(screen);
    int selectedIndex = 0;
    for (int i = 0; i < static_cast<int>(choices.size()); ++i)
        if (choices[static_cast<size_t>(i)] == state.pickerValue) { selectedIndex = i; break; }
    fb.drawText(x + 24, y + 50,
        "Compatible with " + speciesCompact(ux3CurrentSpecies(screen)) + " in " +
        (editor(screen) ? std::string(ux3GameName(editor(screen)->metadata().sourceGame)) : std::string("Gen I")),
        Colors::TextDim, TextStyle::Caption);
    constexpr int visible = 9;
    const int start = std::clamp(selectedIndex - visible / 2, 0, std::max(0, static_cast<int>(choices.size()) - visible));
    for (int i = 0; i < visible && start + i < static_cast<int>(choices.size()); ++i) {
        const int idx = start + i;
        const int move = choices[static_cast<size_t>(idx)];
        const int rowY = y + 86 + i * 42;
        const bool selected = idx == selectedIndex;
        if (selected) fb.drawSelectionHighlight(x + 24, rowY, w - 48, 38);
        std::string text;
        if (move == 0) text = "000 - Empty move slot";
        else { char prefix[16]{}; std::snprintf(prefix, sizeof(prefix), "%03d - ", move); text = std::string(prefix) + Names::getMoveName(move); }
        fb.drawText(x + 40, rowY + 8, text, selected ? Colors::Text : Colors::TextDim);
    }
}

void ux3DrawClone(TrainerViewScreen& screen, PKSEFramebuffer& fb, int x, int y, int w, int h) {
    auto& state = ux2StateFor(screen);
    auto* e = editor(screen);
    if (!e) return;
    std::string error;
    const auto source = ux2SelectedPokemon(screen, error);
    fb.drawText(x + 24, y + 18, "Stage Clone — Choose Destination", Colors::Text, TextStyle::Heading);
    fb.drawText(x + 24, y + 50,
        "Source: " + (source ? source->nickname : std::string("Pokemon")) + " • < Box " + std::to_string(state.cloneBox + 1) + " >",
        Colors::TextDim, TextStyle::Caption);
    const int slots = ux2SlotCount(*e, state.cloneBox);
    const int writable = ux2FirstEmpty(*e, state.cloneBox);
    constexpr int cols = 5;
    const int cellW = (w - 58) / cols;
    const int cellH = 100;
    for (int i = 0; i < slots; ++i) {
        const int col = i % cols, row = i / cols;
        const int cx = x + 20 + col * cellW;
        const int cy = y + 84 + row * cellH;
        const auto pokemon = e->boxedPokemon(static_cast<size_t>(state.cloneBox), static_cast<size_t>(i), error);
        const bool selected = i == state.cloneSlot;
        if (selected) fb.drawSelectionHighlight(cx, cy, cellW - 8, cellH - 8);
        fb.drawRoundedRect(cx, cy, cellW - 8, cellH - 8, 8, i == writable ? Colors::Accent : Colors::Divider, 1);
        fb.drawText(cx + 7, cy + 6, (i < 9 ? "0" : "") + std::to_string(i + 1), Colors::TextDim, TextStyle::Caption);
        if (pokemon) {
            const bool shiny = ux3IsShiny(*pokemon);
            if (auto* sprite = SpriteManager::getIconSprite(pokemon->species, shiny); sprite && sprite->data)
                fb.drawSpriteStaticContained(cx + 28, cy + 6, 56, 56, sprite->width, sprite->height, sprite->data, sprite->channels);
            fb.drawText(cx + 7, cy + 68, Names::getSpeciesName(pokemon->species), shiny ? Colors::ShinyStar : Colors::Text, TextStyle::Caption);
        } else if (i == writable) {
            fb.drawText(cx + 7, cy + 56, "Empty • writable", Colors::Accent, TextStyle::Caption);
        } else {
            fb.drawText(cx + 7, cy + 56, "Empty", Colors::TextDim, TextStyle::Caption);
        }
    }
}

void ux3DrawReview(TrainerViewScreen& screen, PKSEFramebuffer& fb, int x, int y, int w, int h) {
    auto& state = ux2StateFor(screen);
    const auto lines = ux3PendingLines(screen);
    fb.drawText(x + 24, y + 18, "Generation I Pending Changes", Colors::Text, TextStyle::Heading);
    if (lines.empty()) {
        fb.drawText(x + 28, y + 82, "No staged changes.", Colors::TextDim);
        return;
    }
    const int visible = std::max(1, std::min(UX3Contract::pendingGeometry(lines.size()).visibleRows,
                                             static_cast<int>(lines.size())));
    const int start = std::clamp(state.reviewRow - visible / 2, 0, std::max(0, static_cast<int>(lines.size()) - visible));
    fb.setClipRect(x + 24, y + 64, w - 48, h - 108);
    for (int i = 0; i < visible && start + i < static_cast<int>(lines.size()); ++i)
        ux2DrawLogicalRow(fb, x + 24, y + 70 + i * 48, w - 48, std::to_string(start + i + 1),
                          lines[static_cast<size_t>(start + i)], start + i == state.reviewRow);
    fb.clearClip();
}

void ux3DrawStagedView(TrainerViewScreen& screen, PKSEFramebuffer& fb) {
    std::string error;
    const auto pokemon = ux2SelectedPokemon(screen, error);
    if (!pokemon) return;
    Modals::Gen1PokemonDetailsPresentation view;
    view.species = pokemon->species;
    view.speciesName = Names::getSpeciesName(pokemon->species);
    view.nickname = pokemon->nickname;
    view.level = pokemon->level;
    view.experience = pokemon->experience;
    view.originalTrainer = pokemon->originalTrainer;
    view.trainerId = pokemon->trainerId;
    view.dvs = pokemon->dvs;
    view.statExperience = pokemon->statExperience;
    for (size_t i = 0; i < 4; ++i) {
        view.moves[i] = pokemon->moves[i];
        view.pp[i] = pokemon->pp[i];
        view.ppUps[i] = pokemon->ppUps[i];
    }
    const auto stored = storedDVs(*pokemon);
    view.shiny = ShinyDVs::isShiny(stored);
    view.battleStats = Editor::calculateBattleStats(pokemon->species, pokemon->level, stored, pokemon->statExperience).asArray();
    view.hasBattleStats = true;
    view.battleStatsCalculated = true;
    if (auto* e = editor(screen)) view.setMoveCompatibility(e->metadata().sourceGame);
    view.recordLabel = "Staged box record";
    view.sourceStateLabel = "STAGED VIEW / READ ONLY";
    Modals::drawGen1PokemonDetailsPresentation(screen, fb, view);
}

std::string ux3FooterText(const TrainerViewScreen& screen, const UX2State& state) {
    if (state.picker == UX2Picker::Species)
        return "D-pad/Stick Navigate   L/R Page   Y Shiny   A Select   B Cancel";
    if (state.picker == UX2Picker::Move)
        return "D-pad/Stick Navigate   L/R Page   A Select   B Cancel";
    switch (state.mode) {
        case UX2Mode::View: return "B Back";
        case UX2Mode::Edit: return "D-pad/Stick Navigate   A Edit   L/R Section   B Back";
        case UX2Mode::AddDraft: return "D-pad/Stick Navigate   A Edit   L/R Section   X Stage Add   B Cancel Draft";
        case UX2Mode::Actions: return "D-pad/Stick Navigate   A Select   B Back";
        case UX2Mode::LevelExpEditor:
        case UX2Mode::MoveEditor:
        case UX2Mode::DVEditor:
        case UX2Mode::StatExpEditor: return "D-pad/Stick Navigate   A Edit/Apply   B Cancel";
        case UX2Mode::CloneDestination: return "D-pad/Stick Slot   L/R Box   A Stage Clone   B Cancel";
        case UX2Mode::RemoveConfirm: return "A Confirm Remove   B Cancel";
        case UX2Mode::Review: return "D-pad/Stick Navigate   A Export Copy   Y Discard Staged   B Back";
        case UX2Mode::Provenance: return "B Back";
        case UX2Mode::Closed: break;
    }
    return ux2FooterText(screen, state);
}

void ux3DialogGeometry(TrainerViewScreen& screen, int& x, int& y, int& w, int& h) {
    auto& state = ux2StateFor(screen);
    ux2DialogGeometry(screen, x, y, w, h);
    if (state.mode == UX2Mode::Edit || state.mode == UX2Mode::AddDraft) {
        w = 1200; h = 610; x = 40; y = 31;
    } else if (state.mode == UX2Mode::Review) {
        const auto geometry = UX3Contract::pendingGeometry(ux3PendingLines(screen).size());
        w = geometry.width; h = geometry.height; x = (1280 - w) / 2; y = (672 - h) / 2;
    }
}

void ux3HandleActiveInput(TrainerViewScreen& screen, uint64_t down) {
    auto& state = ux2StateFor(screen);
    if (state.picker == UX2Picker::Species) { ux3HandleSpeciesPicker(screen, down); return; }
    if (state.picker == UX2Picker::Move) { ux3HandleMovePicker(screen, down); return; }
    if (state.mode == UX2Mode::View) {
        if (down & HidNpadButton_B) { state.mode = UX2Mode::Actions; state.row = 0; }
        return;
    }
    if (state.mode == UX2Mode::Edit || state.mode == UX2Mode::AddDraft) {
        ux3HandleMainEditor(screen, down);
        return;
    }

    const bool manualDVApply = state.mode == UX2Mode::DVEditor && state.subRow == 5 && (down & HidNpadButton_A);
    ux2HandleActiveInput(screen, down);
    if (manualDVApply) {
        auto& ux3 = ux3StateFor(screen);
        if (state.subForAdd) ux3.preShinyDraftDVs.reset();
        else ux3.preShinyEditDVs.reset();
    }
}

} // namespace

bool isGen1SourceUX(const TrainerViewScreen& screen) noexcept {
    return isGen1SourceUXBase(screen);
}

bool handleInputUX(TrainerViewScreen& screen, uint64_t down, uint64_t held, int stickX, int stickY) {
    if (!isGen1SourceUX(screen)) return false;
    auto& state = ux2StateFor(screen);
    auto& ux3 = ux3StateFor(screen);
    const uint64_t navigated = ux3.navigation.apply(
        down, held, stickX, stickY,
        HidNpadButton_Up, HidNpadButton_Down, HidNpadButton_Left, HidNpadButton_Right);

    if (state.mode != UX2Mode::Closed) {
        ux3HandleActiveInput(screen, navigated);
        return true;
    }
    // Preserve Cleanup #2's accepted empty/occupied entry behavior. Analog navigation
    // is only consumed after the Gen I overlay itself owns the active screen.
    return handleInputUXCleanup2(screen, down);
}

bool handleInputUX(TrainerViewScreen& screen, uint64_t down) {
    return handleInputUX(screen, down, 0, 0, 0);
}

void drawOverlayUX(TrainerViewScreen& screen, PKSEFramebuffer& fb) {
    if (!isGen1SourceUX(screen)) return;
    auto& state = ux2StateFor(screen);
    if (state.mode == UX2Mode::Closed) {
        const auto text = ux3FooterText(screen, state);
        if (!text.empty()) drawFooter(fb, text);
        return;
    }

    if (state.mode == UX2Mode::View) {
        ux3DrawStagedView(screen, fb);
        return;
    }

    screen.touchButtons.clear();
    int x = 0, y = 0, w = 0, h = 0;
    ux3DialogGeometry(screen, x, y, w, h);
    fb.drawFilledRect(0, 0, fb.getWidth(), fb.getHeight() - 48, Color(0, 0, 0, 105));
    ux2DrawPanel(fb, x, y, w, h);

    switch (state.mode) {
        case UX2Mode::Actions: ux2DrawActions(screen, fb, x, y, w, h); break;
        case UX2Mode::Edit: ux3DrawEditor(screen, fb, x, y, w, h, false); break;
        case UX2Mode::AddDraft: ux3DrawEditor(screen, fb, x, y, w, h, true); break;
        case UX2Mode::LevelExpEditor:
        case UX2Mode::MoveEditor:
        case UX2Mode::DVEditor:
        case UX2Mode::StatExpEditor: ux2DrawSubEditor(screen, fb, x, y, w, h); break;
        case UX2Mode::CloneDestination: ux3DrawClone(screen, fb, x, y, w, h); break;
        case UX2Mode::RemoveConfirm: ux2DrawRemoveConfirm(fb, x, y, w, h); break;
        case UX2Mode::Review: ux3DrawReview(screen, fb, x, y, w, h); break;
        case UX2Mode::Provenance: ux2DrawProvenance(screen, fb, x, y, w, h); break;
        case UX2Mode::View:
        case UX2Mode::Closed: break;
    }

    if (state.picker != UX2Picker::None) {
        const int pw = state.picker == UX2Picker::Species ? 1080 : 760;
        const int ph = 520;
        ux3DrawPicker(screen, fb, (1280 - pw) / 2, (672 - ph) / 2, pw, ph);
    }
    drawFooter(fb, ux3FooterText(screen, state));
}

} // namespace UI::Gen1PokemonEditor
