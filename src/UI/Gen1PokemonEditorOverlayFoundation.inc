#include "UI/PokemonEditorFoundationContract.h"
#include "UI/PokemonEditorExitGuard.h"
#include "UI/Gen1PokemonPresentation.h"
#include "UI/ScreenChrome.h"
#include "UI/Dialogs/DialogFrame.h"

#include <map>

namespace UI::Gen1PokemonEditor {
namespace {

namespace Foundation = PokeBank::UIModel::PokemonEditorFoundation;
namespace ExitGuard = PokeBank::UIModel::PokemonEditorExitGuard;

struct FoundationState {
    const TrainerViewScreen* owner = nullptr;
    std::string sourceGameId;
    Foundation::Focus focus{};
    PokeBank::UIModel::ControllerNavigation navigation;
    UX2Mode lastMainMode = UX2Mode::Closed;
    uint32_t randomState = 0x6D2B79F5u;
    std::optional<PokemonRecord> editBaseline;
    bool editExitConfirm = false;
};

FoundationState& foundationStateFor(const TrainerViewScreen& screen) {
    static FoundationState state;
    if (state.owner != &screen || state.sourceGameId != screen.sourceGameId) {
        state = {};
        state.owner = &screen;
        state.sourceGameId = screen.sourceGameId;
    }
    return state;
}

bool foundationMainMode(UX2Mode mode) noexcept {
    return mode == UX2Mode::View || mode == UX2Mode::Edit || mode == UX2Mode::AddDraft;
}

void foundationSyncMainMode(TrainerViewScreen& screen) {
    auto& state = ux2StateFor(screen);
    auto& foundation = foundationStateFor(screen);
    if (!foundationMainMode(state.mode)) return;
    if (foundation.lastMainMode != state.mode) {
        foundation.focus = {Foundation::Panel::Identity, 0, 0};
        foundation.lastMainMode = state.mode;
        foundation.randomState ^= static_cast<uint32_t>((state.box + 1) * 0x9E37u);
        foundation.randomState ^= static_cast<uint32_t>((state.slot + 1) * 0x85EBu);
        foundation.editExitConfirm = false;
        if (state.mode == UX2Mode::Edit) {
            std::string error;
            foundation.editBaseline = ux2SelectedPokemon(screen, error);
        } else {
            foundation.editBaseline.reset();
        }
    }
}

const char* foundationTypeName(uint8_t type) noexcept {
    switch (type) {
        case 0: return "Normal";
        case 1: return "Fighting";
        case 2: return "Flying";
        case 3: return "Poison";
        case 4: return "Ground";
        case 5: return "Rock";
        case 7: return "Bug";
        case 8: return "Ghost";
        case 20: return "Fire";
        case 21: return "Water";
        case 22: return "Grass";
        case 23: return "Electric";
        case 24: return "Psychic";
        case 25: return "Ice";
        case 26: return "Dragon";
        default: return "Unknown";
    }
}

void foundationDrawBadge(PKSEFramebuffer& fb, int x, int y, const std::string& text) {
    int tw = 0, th = 0;
    fb.measureText(text, tw, th, TextStyle::Caption);
    const int w = std::max(64, tw + 24);
    fb.drawFilledRoundedRect(x, y, w, 28, 10, Colors::Panel);
    fb.drawRoundedRect(x, y, w, 28, 10, Colors::Accent, 1);
    fb.drawText(x + 12, y + 6, text, Colors::Text, TextStyle::Caption);
}

std::string foundationNickname(const UX2State& state, const PokemonRecord* pokemon, bool add, uint16_t species) {
    if (add)
        return state.draft.nickname.empty() ? std::string(Names::getSpeciesName(species)) : state.draft.nickname;
    return pokemon ? pokemon->nickname : std::string{};
}

std::string foundationOT(const UX2State& state, const Editor& e, const PokemonRecord* pokemon, bool add) {
    if (add) return state.draft.otName.empty() ? e.view().trainer().name : state.draft.otName;
    return pokemon ? pokemon->originalTrainer : std::string{};
}

uint16_t foundationTID(const UX2State& state, const Editor& e, const PokemonRecord* pokemon, bool add) {
    if (add) return state.draft.trainerId.value_or(e.view().trainer().trainerId);
    return pokemon ? pokemon->trainerId : 0;
}

uint32_t foundationExperience(const UX2State& state, const PokemonRecord* pokemon, bool add) {
    return add ? draftExperience(state.draft) : (pokemon ? pokemon->experience : 0);
}

bool foundationSameEditablePokemon(const PokemonRecord& a, const PokemonRecord& b) noexcept {
    return a.species == b.species &&
           a.nickname == b.nickname &&
           a.level == b.level &&
           a.experience == b.experience &&
           a.originalTrainer == b.originalTrainer &&
           a.trainerId == b.trainerId &&
           a.moves == b.moves &&
           a.pp == b.pp &&
           a.ppUps == b.ppUps &&
           a.dvs == b.dvs &&
           a.statExperience == b.statExperience;
}

bool foundationEditSessionDirty(TrainerViewScreen& screen) {
    auto& foundation = foundationStateFor(screen);
    if (!foundation.editBaseline) return false;
    std::string error;
    const auto current = ux2SelectedPokemon(screen, error);
    return current && !foundationSameEditablePokemon(*foundation.editBaseline, *current);
}

bool foundationRestoreEditBaseline(TrainerViewScreen& screen) {
    auto& state = ux2StateFor(screen);
    auto& foundation = foundationStateFor(screen);
    auto* e = editor(screen);
    if (!foundation.editBaseline || !e) return true;

    const auto& baseline = *foundation.editBaseline;
    BoxPokemonEdit edit;
    edit.species = baseline.species;
    edit.nickname = baseline.nickname;
    edit.level = baseline.level;
    edit.experience = baseline.experience;
    edit.trainerId = baseline.trainerId;
    edit.moves = baseline.moves;
    edit.pp = baseline.pp;
    edit.ppUps = baseline.ppUps;
    edit.dvs = storedDVs(baseline);
    edit.statExperience = baseline.statExperience;
    edit.otName = baseline.originalTrainer;

    std::string error;
    if (!e->stageEdit(static_cast<size_t>(state.box), static_cast<size_t>(state.slot), edit, error)) {
        screen.postStatus(error.empty() ? "Could not restore the pre-edit staged Pokemon" : error, 360);
        return false;
    }
    refreshBoxes(screen);
    return true;
}

void foundationExitMainMode(TrainerViewScreen& screen) {
    auto& state = ux2StateFor(screen);
    auto& foundation = foundationStateFor(screen);
    ux3ResetSessionState(screen);
    state.mode = UX2Mode::Actions;
    state.row = 0;
    foundation.lastMainMode = UX2Mode::Closed;
    foundation.editBaseline.reset();
    foundation.editExitConfirm = false;
}

void foundationRandomizeDVs(TrainerViewScreen& screen, bool add) {
    auto& state = ux2StateFor(screen);
    auto& foundation = foundationStateFor(screen);
    auto& ux3 = ux3StateFor(screen);
    const auto dvs = Foundation::randomDVs(foundation.randomState);
    if (add) {
        state.draft.dvs = dvs;
        ux3.preShinyDraftDVs.reset();
        screen.postStatus("Generation I DVs randomized in draft only; Stage Add still required", 280);
        return;
    }
    BoxPokemonEdit edit;
    edit.dvs = dvs;
    if (ux2CommitEdit(screen, edit, "Randomized DVs staged in this edit session; source .srm unchanged"))
        ux3.preShinyEditDVs.reset();
}

void foundationEditIdentity(TrainerViewScreen& screen, uint8_t row, bool add, const PokemonRecord* pokemon) {
    auto& state = ux2StateFor(screen);
    auto* e = editor(screen);
    if (!e) return;
    switch (row) {
        case 0:
            ux3OpenSpeciesPicker(screen, add ? state.draft.species : pokemon->species,
                                 add ? ux3IsShiny(state.draft) : ux3IsShiny(*pokemon));
            break;
        case 1: {
            const uint16_t species = add ? state.draft.species : pokemon->species;
            const auto result = Utils::promptText("Generation I Nickname", "1-10 supported Gen I characters",
                                                  foundationNickname(state, pokemon, add, species), 10);
            if (!result.accepted) break;
            if (add) state.draft.nickname = result.text;
            else { BoxPokemonEdit edit; edit.nickname = result.text; ux2CommitEdit(screen, edit, "Nickname change staged in this edit session; source .srm unchanged"); }
            break;
        }
        case 2:
            ux2OpenLevelExpEditor(screen, add);
            break;
        case 3: {
            const auto result = Utils::promptText("Generation I OT", "1-7 supported Gen I characters",
                                                  foundationOT(state, *e, pokemon, add), 7);
            if (!result.accepted) break;
            if (add) state.draft.otName = result.text;
            else { BoxPokemonEdit edit; edit.otName = result.text; ux2CommitEdit(screen, edit, "OT change staged in this edit session; source .srm unchanged"); }
            break;
        }
        case 4: {
            const auto result = Utils::promptNumber("Generation I Trainer ID", foundationTID(state, *e, pokemon, add), 0, 65535);
            if (!result.accepted) break;
            if (add) state.draft.trainerId = static_cast<uint16_t>(result.value);
            else { BoxPokemonEdit edit; edit.trainerId = static_cast<uint16_t>(result.value); ux2CommitEdit(screen, edit, "TID change staged in this edit session; source .srm unchanged"); }
            break;
        }
    }
}

void foundationEditValue(TrainerViewScreen& screen, const Foundation::Focus& focus,
                         bool add, const PokemonRecord* pokemon) {
    auto& state = ux2StateFor(screen);
    if (focus.row < 5) {
        if (focus.column == static_cast<uint8_t>(Foundation::ValueColumn::DV)) {
            if (focus.row == 0) {
                screen.postStatus("HP DV is derived from Attack, Defense, Speed and Special DVs", 260);
                return;
            }
            ux2OpenDVEditor(screen, add);
            state.subRow = static_cast<int>(focus.row) - 1;
            return;
        }
        if (focus.column == static_cast<uint8_t>(Foundation::ValueColumn::StatExperience)) {
            ux2OpenStatExpEditor(screen, add);
            state.subRow = static_cast<int>(focus.row);
            return;
        }
        screen.postStatus("Calculated Generation I stats are read-only; edit DVs or Stat Exp instead", 300);
        return;
    }
    if (focus.row == static_cast<uint8_t>(Foundation::ValueRow::Shiny)) {
        if (add) ux3SetDraftShiny(screen, !ux3IsShiny(state.draft));
        else ux3SetExistingShiny(screen, !ux3IsShiny(*pokemon));
        return;
    }
    if (focus.row == static_cast<uint8_t>(Foundation::ValueRow::Level))
        ux2OpenLevelExpEditor(screen, add);
}

void foundationHandleMainInput(TrainerViewScreen& screen, uint64_t down) {
    auto& state = ux2StateFor(screen);
    auto& foundation = foundationStateFor(screen);
    foundationSyncMainMode(screen);
    const bool view = state.mode == UX2Mode::View;
    const bool add = state.mode == UX2Mode::AddDraft;
    const bool editable = !view;

    if (foundation.editExitConfirm) {
        const bool create = state.mode == UX2Mode::AddDraft;
        if (down & HidNpadButton_B) {
            foundation.editExitConfirm = false;
            screen.postStatus("Continue editing; no exit decision made", 220);
            return;
        }
        if (down & HidNpadButton_Y) {
            if (create) {
                state.draft = BoxPokemonCreate{};
                screen.postStatus("Create draft discarded; staged/source bytes unchanged", 300);
            } else {
                if (!foundationRestoreEditBaseline(screen)) return;
                screen.postStatus("Edit-session changes discarded; pre-edit staged Pokemon restored", 320);
            }
            foundationExitMainMode(screen);
            return;
        }
        if (down & HidNpadButton_A) {
            if (create) {
                if (!ux3StageAdd(screen)) return;
                foundation.lastMainMode = UX2Mode::Closed;
                foundation.editBaseline.reset();
                foundation.editExitConfirm = false;
                return;
            }
            screen.postStatus("Edit session confirmed and kept staged; source .srm unchanged", 300);
            foundationExitMainMode(screen);
            return;
        }
        return;
    }

    if (down & HidNpadButton_Up) foundation.focus = Foundation::moveFocus(foundation.focus, Foundation::Direction::Up);
    if (down & HidNpadButton_Down) foundation.focus = Foundation::moveFocus(foundation.focus, Foundation::Direction::Down);
    if (down & HidNpadButton_Left) foundation.focus = Foundation::moveFocus(foundation.focus, Foundation::Direction::Left);
    if (down & HidNpadButton_Right) foundation.focus = Foundation::moveFocus(foundation.focus, Foundation::Direction::Right);
    if (down & HidNpadButton_L) foundation.focus = Foundation::nextPanel(Foundation::nextPanel(foundation.focus));
    if (down & HidNpadButton_R) foundation.focus = Foundation::nextPanel(foundation.focus);

    if (add && (down & HidNpadButton_X)) { ux3StageAdd(screen); return; }
    if (down & HidNpadButton_B) {
        const auto exitKind = add ? ExitGuard::SessionKind::Create
                            : state.mode == UX2Mode::Edit ? ExitGuard::SessionKind::Edit
                                                         : ExitGuard::SessionKind::View;
        if (ExitGuard::requiresConfirmation(exitKind,
                state.mode == UX2Mode::Edit && foundationEditSessionDirty(screen))) {
            foundation.editExitConfirm = true;
            screen.controllerNavigation.reset();
            return;
        }
        foundationExitMainMode(screen);
        return;
    }
    if (editable && (down & HidNpadButton_Y)) {
        foundationRandomizeDVs(screen, add);
        return;
    }
    if (!(down & HidNpadButton_A)) return;
    if (!editable) {
        screen.postStatus("View Pokemon is read-only; choose Edit from Actions to stage changes", 260);
        return;
    }

    std::string error;
    const auto pokemon = add ? std::optional<PokemonRecord>{} : ux2SelectedPokemon(screen, error);
    if (!add && !pokemon) {
        screen.postStatus(error.empty() ? "Pokemon unavailable" : error, 300);
        return;
    }

    if (foundation.focus.panel == Foundation::Panel::Identity) {
        foundationEditIdentity(screen, foundation.focus.row, add, pokemon ? &*pokemon : nullptr);
        return;
    }
    if (foundation.focus.panel == Foundation::Panel::Values) {
        foundationEditValue(screen, foundation.focus, add, pokemon ? &*pokemon : nullptr);
        return;
    }
    ux2OpenMoveEditor(screen, static_cast<int>(foundation.focus.row), add);
}

void foundationDrawIdentityRow(PKSEFramebuffer& fb, int x, int y, int w,
                               const std::string& label, const std::string& value, bool selected) {
    if (selected) fb.drawSelectionHighlight(x, y, w, 38);
    fb.drawText(x + 10, y + 7, label, Colors::TextDim, TextStyle::Caption);
    fb.drawText(x + 100, y + 7, value, selected ? Colors::Text : Colors::Text, TextStyle::Caption);
}

void foundationDrawExitConfirm(TrainerViewScreen& screen, PKSEFramebuffer& fb) {
    auto& foundation = foundationStateFor(screen);
    if (!foundation.editExitConfirm) return;

    // Reuse PKSE's native creator/details confirmation chrome instead of maintaining
    // a second PokeBank-specific modal. Only the underlying staged-session actions differ.
    const auto& state = ux2StateFor(screen);
    const bool create = state.mode == UX2Mode::AddDraft;
    const int w = create ? 540 : 560;
    constexpr int h = 226;
    const int x = (fb.getWidth() - w) / 2;
    const int y = (fb.getHeight() - h) / 2;
    const char* title = create ? "New Pokémon" : "Unsaved changes";
    const Color accent = create ? Colors::Primary : Colors::Orange;
    int cy = Dialogs::drawDialogFrame(fb, x, y, w, h, title, accent);

    fb.drawText(x + 28, cy,
                create ? "Keep this new Pokémon?" : "This Pokémon has unsaved changes.",
                Colors::Text);
    fb.drawText(x + 28, cy + 34,
                create ? "Discard removes it; Back keeps editing."
                       : "Discard loses them; Back keeps editing.",
                Colors::TextDim, TextStyle::Caption);

    const int cbw = 160, cbh = TouchTargetMin, cby = y + h - cbh - 18;
    drawGlyphButton(fb, x + 24, cby, cbw, cbh, "B", "Back", Colors::PanelAlt);
    drawGlyphButton(fb, x + (w - cbw) / 2, cby, cbw, cbh, "Y", "Discard", Colors::PanelAlt);
    drawGlyphButton(fb, x + w - 24 - cbw, cby, cbw, cbh, "A",
                    create ? "Keep" : "Save", Colors::PanelAlt);
}

void foundationDrawWorkspace(TrainerViewScreen& screen, PKSEFramebuffer& fb) {
    auto& state = ux2StateFor(screen);
    auto& foundation = foundationStateFor(screen);
    foundationSyncMainMode(screen);
    auto* e = editor(screen);
    if (!e) return;
    const bool view = state.mode == UX2Mode::View;
    const bool add = state.mode == UX2Mode::AddDraft;

    std::string error;
    const auto pokemon = add ? std::optional<PokemonRecord>{} : ux2SelectedPokemon(screen, error);
    if (!add && !pokemon) {
        screen.postStatus(error.empty() ? "Pokemon unavailable" : error, 300);
        return;
    }

    const uint16_t species = add ? state.draft.species : pokemon->species;
    const auto dvs = add ? state.draft.dvs : storedDVs(*pokemon);
    const auto& statExp = add ? state.draft.statExperience : pokemon->statExperience;
    const uint8_t level = add ? state.draft.level : pokemon->level;
    const auto presentation = PokeBank::UIModel::presentGen1Pokemon(species, level, dvs, statExp);
    const bool shiny = presentation.shiny;
    const auto& stats = presentation.battleStats;
    const auto& types = presentation.nativeTypes;
    const auto& moves = add ? state.draft.moves : pokemon->moves;
    const auto& pp = add ? state.draft.pp : pokemon->pp;
    const auto badMove = MoveCompatibility::firstIncompatible(e->metadata().sourceGame, species, moves);
    const Color incompatibleColor = add ? Colors::Error : Colors::Warning;

    screen.touchButtons.clear();
    constexpr int x = 40, y = 31, w = 1200, h = 610;
    fb.drawFilledRect(0, 0, fb.getWidth(), fb.getHeight() - 48, Color(0, 0, 0, 105));
    ux2DrawPanel(fb, x, y, w, h);

    std::string title = "View Pokemon — READ ONLY";
    if (state.mode == UX2Mode::Edit) title = "Edit Pokemon — STAGED";
    if (add) title = "Create Pokemon — DRAFT";
    fb.drawText(x + 24, y + 14, title, Colors::Text, TextStyle::Heading);
    fb.drawText(x + 24, y + 46,
                "Box " + std::to_string(state.box + 1) + " / Slot " + std::to_string(state.slot + 1) +
                "   •   PKSE three-panel workspace   •   Source save immutable",
                Colors::TextDim, TextStyle::Caption);

    const int contentY = y + 74;
    const int contentH = h - 94;
    const int leftX = x + 18, leftW = 300;
    const int midX = leftX + leftW + 14, midW = 398;
    const int rightX = midX + midW + 14, rightW = x + w - 18 - rightX;
    drawPanelSurface(fb, leftX, contentY, leftW, contentH, false, 14);
    drawPanelSurface(fb, midX, contentY, midW, contentH, false, 14);
    drawPanelSurface(fb, rightX, contentY, rightW, contentH, false, 14);

    // LEFT — identity/details. Every row below the artwork is focusable in all three modes.
    fb.drawText(leftX + 14, contentY + 10, "DETAILS", Colors::Accent, TextStyle::Caption);
    if (auto* sprite = SpriteManager::getSprite(species, shiny); sprite && sprite->data)
        fb.drawSpriteStaticContained(leftX + 63, contentY + 34, 174, 146,
                                     sprite->width, sprite->height, sprite->data, sprite->channels);
    else fb.drawText(leftX + 82, contentY + 104, "Sprite unavailable", Colors::TextDim, TextStyle::Caption);
    int badgeX = leftX + 18;
    foundationDrawBadge(fb, badgeX, contentY + 184, foundationTypeName(types[0]));
    if (types[1] != types[0]) {
        int tw = 0, th = 0;
        fb.measureText(foundationTypeName(types[0]), tw, th, TextStyle::Caption);
        badgeX += std::max(64, tw + 24) + 8;
        foundationDrawBadge(fb, badgeX, contentY + 184, foundationTypeName(types[1]));
    }

    const std::array<std::string,5> leftLabels{"Species", "Nickname", "Level / EXP", "OT", "Trainer ID"};
    const std::array<std::string,5> leftValues{
        speciesCompact(species),
        foundationNickname(state, pokemon ? &*pokemon : nullptr, add, species),
        "Lv " + std::to_string(level) + " / " + std::to_string(foundationExperience(state, pokemon ? &*pokemon : nullptr, add)),
        foundationOT(state, *e, pokemon ? &*pokemon : nullptr, add),
        std::to_string(foundationTID(state, *e, pokemon ? &*pokemon : nullptr, add)),
    };
    for (int i = 0; i < 5; ++i) {
        const bool selected = foundation.focus.panel == Foundation::Panel::Identity && foundation.focus.row == i;
        foundationDrawIdentityRow(fb, leftX + 8, contentY + 224 + i * 43, leftW - 16,
                                  leftLabels[static_cast<size_t>(i)], leftValues[static_cast<size_t>(i)], selected);
    }

    // MIDDLE — actual Gen I Values interaction model: DV / Stat Exp / calculated Stat.
    fb.drawText(midX + 14, contentY + 10, "VALUES", Colors::Text, TextStyle::Heading);
    constexpr int labelW = 76;
    constexpr int cellW = 96;
    const int gridX = midX + 10;
    const int gridY = contentY + 48;
    fb.drawText(gridX + labelW + 18, gridY, "DV", Colors::TextDim, TextStyle::Caption);
    fb.drawText(gridX + labelW + cellW + 8, gridY, "Stat Exp", Colors::TextDim, TextStyle::Caption);
    fb.drawText(gridX + labelW + cellW * 2 + 24, gridY, "Stat", Colors::TextDim, TextStyle::Caption);
    constexpr auto statLabels = Foundation::gen1StatLabels();
    for (int r = 0; r < 5; ++r) {
        const int rowY = gridY + 28 + r * 48;
        fb.drawText(gridX + 6, rowY + 12, statLabels[static_cast<size_t>(r)], Colors::Text, TextStyle::Caption);
        for (int c = 0; c < 3; ++c) {
            const int cellX = gridX + labelW + c * cellW;
            const bool selected = foundation.focus.panel == Foundation::Panel::Values &&
                                  foundation.focus.row == r && foundation.focus.column == c;
            if (selected) fb.drawSelectionHighlight(cellX, rowY, cellW - 4, 40);
            std::string value;
            if (c == 0) value = r == 0 ? std::to_string(Editor::derivedHPDV(dvs)) + " *" : std::to_string(dvs[static_cast<size_t>(r - 1)]);
            else if (c == 1) value = std::to_string(statExp[static_cast<size_t>(r)]);
            else value = std::to_string(stats[static_cast<size_t>(r)]);
            const bool readOnly = c == 2 || (r == 0 && c == 0);
            fb.drawText(cellX + 10, rowY + 11, value, readOnly ? Colors::TextDim : Colors::Text, TextStyle::Caption);
        }
    }
    fb.drawText(gridX + 8, gridY + 272, "* HP DV derived", Colors::TextDim, TextStyle::Caption);
    const bool shinySelected = foundation.focus.panel == Foundation::Panel::Values && foundation.focus.row == 5;
    const bool levelSelected = foundation.focus.panel == Foundation::Panel::Values && foundation.focus.row == 6;
    if (shinySelected) fb.drawSelectionHighlight(gridX, gridY + 304, midW - 20, 40);
    fb.drawText(gridX + 12, gridY + 315, "Shiny", Colors::TextDim, TextStyle::Caption);
    fb.drawText(gridX + 130, gridY + 315, shiny ? "Yes" : "No", shiny ? Colors::ShinyStar : Colors::Text, TextStyle::Caption);
    if (levelSelected) fb.drawSelectionHighlight(gridX, gridY + 350, midW - 20, 40);
    fb.drawText(gridX + 12, gridY + 361, "Level", Colors::TextDim, TextStyle::Caption);
    fb.drawText(gridX + 130, gridY + 361, std::to_string(level), Colors::Text, TextStyle::Caption);
    fb.drawText(gridX + 12, gridY + 406, "Calculated Stat cells are read-only", Colors::TextDim, TextStyle::Caption);

    // RIGHT — four real Move rows, semantic compatibility status, then PokeBank's five-axis radar.
    fb.drawText(rightX + 14, contentY + 10, "MOVES", Colors::Text, TextStyle::Heading);
    for (int i = 0; i < 4; ++i) {
        const int rowY = contentY + 44 + i * 40;
        const bool selected = foundation.focus.panel == Foundation::Panel::Moves && foundation.focus.row == i;
        if (selected) fb.drawSelectionHighlight(rightX + 10, rowY, rightW - 20, 38);
        const bool incompatible = !MoveCompatibility::canLearnMove(e->metadata().sourceGame, species, moves[static_cast<size_t>(i)]);
        std::string moveText = moves[static_cast<size_t>(i)] == 0 ? "Empty" : std::string(Names::getMoveName(moves[static_cast<size_t>(i)]));
        if (incompatible) moveText = "[!] " + moveText;
        fb.drawText(rightX + 20, rowY + 9, moveText, incompatible ? incompatibleColor : Colors::Text, TextStyle::Caption);
        const std::string status = incompatible ? (add ? "Invalid" : "Unusual") : "OK";
        int statusW = 0, statusH = 0;
        fb.measureText(status, statusW, statusH, TextStyle::Caption);
        const int statusX = rightX + rightW - 16 - statusW;
        fb.drawText(statusX, rowY + 9, status, incompatible ? incompatibleColor : Colors::Success, TextStyle::Caption);
        const auto ups = add ? state.draft.ppUps[static_cast<size_t>(i)] : pokemon->ppUps[static_cast<size_t>(i)];
        const std::string ppText = "PP " + std::to_string(pp[static_cast<size_t>(i)]) + " Up " + std::to_string(ups);
        int ppW = 0, ppH = 0;
        fb.measureText(ppText, ppW, ppH, TextStyle::Caption);
        fb.drawText(statusX - 12 - ppW, rowY + 9, ppText, Colors::TextDim, TextStyle::Caption);
    }
    const int statusY = contentY + 214;
    fb.drawHDivider(rightX + 14, statusY - 12, rightW - 28);
    fb.drawText(rightX + 18, statusY, "Move compatibility", Colors::TextDim, TextStyle::Caption);
    const std::string compatibilityText = !badMove ? "OK" : (add ? "Needs correction" : "Unusual preserved");
    const Color compatibilityColor = !badMove ? Colors::Success : (add ? Colors::Error : Colors::Warning);
    fb.drawText(rightX + rightW - 148, statusY, compatibilityText, compatibilityColor, TextStyle::Caption);
    fb.drawText(rightX + 18, statusY + 26, "Encounter legality", Colors::TextDim, TextStyle::Caption);
    fb.drawText(rightX + rightW - 118, statusY + 26, "Not checked", Colors::Text, TextStyle::Caption);
    if (badMove) {
        const std::string warning = add ? "Change marked move before Stage Add" : "Legacy/unusual move kept unless you replace it";
        fb.drawText(rightX + 18, statusY + 54, warning, compatibilityColor, TextStyle::Caption);
    }
    // Supplemental/radar panes are drawn once by drawFoundationBottomSplit.

    std::string footer = "D-pad/Stick Navigate   L/R Panel   B Back";
    if (!view) footer = "D-pad/Stick Navigate   A Edit   Y Randomize DVs   L/R Panel   B Back";
    if (add) footer = "D-pad/Stick Navigate   A Edit   Y Randomize DVs   L/R Panel   X Stage Add   B Cancel Draft";
    drawFooter(fb, footer);
    foundationDrawExitConfirm(screen, fb);
}

} // namespace

bool isGen1SourceUX(const TrainerViewScreen& screen) noexcept {
    return isGen1SourceUXCleanup3(screen);
}

bool handleInputUX(TrainerViewScreen& screen, uint64_t down, uint64_t held, int stickX, int stickY) {
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
    return handleInputUX(screen, down, 0, 0, 0);
}

void drawOverlayUX(TrainerViewScreen& screen, PKSEFramebuffer& fb) {
    if (!isGen1SourceUX(screen)) return;
    auto& state = ux2StateFor(screen);
    if (foundationMainMode(state.mode)) {
        foundationDrawWorkspace(screen, fb);
        return;
    }
    drawOverlayUXCleanup3(screen, fb);
}

} // namespace UI::Gen1PokemonEditor
