#include "UI/SharedEditorPresentation.h"
#include "UI/Gen12DVReroll.h"
#include "UI/ClassicPokemonActionMenu.h"
#include "UI/SharedPokemonEditorContract.h"
#include "UI/Gen2NativePresentation.h"
#include "UI/Gen12ReviewDiscardConfirm.h"
#include "UI/ClassicReviewOrigin.h"

namespace UI::Gen2PokemonEditor {
namespace {

namespace SharedEditor = PokeBank::UIModel::SharedPokemonEditor;
namespace Gen2Native = PokeBank::UIModel::Gen2Native;

struct SharedSurfaceState {
    const TrainerViewScreen* owner = nullptr;
    uint64_t ownerSerial = 0;   // TrainerViewScreen::instanceSerial (never reused, unlike the address)
    std::string sourceGameId;
    bool provenanceActive = false;
    PokeBank::UIModel::Gen12ReviewDiscard::State reviewDiscard;
};

SharedSurfaceState& sharedSurfaceStateFor(const TrainerViewScreen& screen) {
    static SharedSurfaceState state;
    if ((state.owner != &screen || state.ownerSerial != screen.instanceSerial) || state.sourceGameId != screen.sourceGameId) {
        state = {};
        state.owner = &screen; state.ownerSerial = screen.instanceSerial;
        state.sourceGameId = screen.sourceGameId;
    }
    return state;
}

const char* sharedGameName(const TrainerViewScreen& screen) noexcept {
    if (screen.sourceGameId == "gold_gbc") return "Gold";
    if (screen.sourceGameId == "silver_gbc") return "Silver";
    if (screen.sourceGameId == "crystal_gbc") return "Crystal";
    return "Generation II";
}

const char* sharedTypeName(uint8_t type) noexcept {
    switch (type) {
        case 0: return "Normal";
        case 1: return "Fighting";
        case 2: return "Flying";
        case 3: return "Poison";
        case 4: return "Ground";
        case 5: return "Rock";
        case 7: return "Bug";
        case 8: return "Ghost";
        case 9: return "Steel";
        case 20: return "Fire";
        case 21: return "Water";
        case 22: return "Grass";
        case 23: return "Electric";
        case 24: return "Psychic";
        case 25: return "Ice";
        case 26: return "Dragon";
        case 27: return "Dark";
        default: return "Unknown";
    }
}

void sharedBadge(PKSEFramebuffer& fb, int x, int y, const std::string& text) {
    int tw = 0, th = 0;
    fb.measureText(text, tw, th, TextStyle::Caption);
    const int w = std::max(64, tw + 24);
    fb.drawFilledRoundedRect(x, y, w, 28, 10, Colors::Panel);
    fb.drawRoundedRect(x, y, w, 28, 10, Colors::Accent, 1);
    fb.drawText(x + 12, y + 6, text, Colors::Text, TextStyle::Caption);
}

SharedEditor::ActionCapabilities gen2ActionCapabilities() noexcept {
    SharedEditor::ActionCapabilities caps;
    caps.canClone = true;
    caps.canRemove = false; // Gen2 StagedEditor has no proven remove/compaction adapter yet.
    // This is an informational shell: encounter legality remains explicitly PokeBank::UIModel::SharedEditorPresentation::legalityLabel(PokeBank::UIModel::SharedEditorPresentation::LegalityState::NotChecked).
    caps.hasLegalityProvenance = true;
    return caps;
}

std::string sharedActionLabel(SharedEditor::Action action) {
    return SharedEditor::actionLabel(action);
}

std::string sharedActionDescription(SharedEditor::Action action, const Gen2::StagedEditor& editor,
                                    const std::optional<Gen2::PokemonRecord>& pokemon) {
    switch (action) {
        case SharedEditor::Action::View: return "Read-only shared Pokemon view";
        case SharedEditor::Action::Edit:
            return pokemon ? pokemon->nickname + "  Lv " + std::to_string(pokemon->level) : "Edit Pokemon";
        case SharedEditor::Action::Clone: return "Carry a copy to an empty slot";
        case SharedEditor::Action::LegalityProvenance: return "Source, validation and truthful legality status";
        case SharedEditor::Action::Add: return "Open Create Pokemon — DRAFT";
        case SharedEditor::Action::Review:
            return std::to_string(editor.pendingChanges().size()) + " staged change(s)";
        case SharedEditor::Action::Close: return "Return to box";
        default: return "";
    }
}

bool handleSharedActions(TrainerViewScreen& screen, uint64_t down, uint64_t held,
                         int stickX, int stickY, const TouchInput& touch) {
    auto& legacy = editorOverlayState(screen);
    auto* editor = stagedEditor(screen);
    if (!editor || !legacy.active || legacy.review || !legacy.pokemonActions) return false;

    std::string error;
    auto pokemon = editor->boxedPokemon(static_cast<std::size_t>(legacy.pokemonBox),
                                        static_cast<std::size_t>(legacy.pokemonSlot), error);
    const bool occupied = pokemon.has_value();
    const auto actions = SharedEditor::actionsForSlot(occupied, gen2ActionCapabilities(), !editor->pendingChanges().empty());
    if (actions.count == 0) return true;

    const uint64_t nav = screen.controllerNavigation.apply(
        down, held, stickX, stickY,
        HidNpadButton_Up, HidNpadButton_Down, HidNpadButton_Left, HidNpadButton_Right);
    if (nav & HidNpadButton_Up)
        legacy.pokemonRow = (legacy.pokemonRow - 1 + static_cast<int>(actions.count)) % static_cast<int>(actions.count);
    if (nav & HidNpadButton_Down)
        legacy.pokemonRow = (legacy.pokemonRow + 1) % static_cast<int>(actions.count);
    legacy.pokemonRow = std::clamp(legacy.pokemonRow, 0, static_cast<int>(actions.count) - 1);

    // Native Gen II menu rows are directly touchable, not merely controller
    // hints. Ignore drag releases; a tap commits only its own visible row.
    const int touched = screen.touchedButtonId(touch);
    if (touched >= 31600 && touched < 31600 + static_cast<int>(actions.count)) {
        legacy.pokemonRow = touched - 31600;
        down |= HidNpadButton_A;
    }
    if (down & HidNpadButton_B) {
        legacy.active = false;
        legacy.pokemonActions = false;
        screen.controllerNavigation.reset();
        return true;
    }
    if (!(down & HidNpadButton_A)) return true;

    auto& state = stateFor(screen);
    switch (actions[static_cast<std::size_t>(legacy.pokemonRow)]) {
        case SharedEditor::Action::View:
            if (!pokemon) return true;
            state.box = legacy.pokemonBox;
            state.slot = legacy.pokemonSlot;
            state.party = legacy.pokemonParty;
            state.begin(*pokemon, Mode::View);
            screen.controllerNavigation.reset();
            return true;
        case SharedEditor::Action::Edit:
            return beginEdit(screen, legacy.pokemonBox, legacy.pokemonSlot);
        case SharedEditor::Action::Clone: {
            if (!pokemon) return true;
            // Round 11 H31: the shared visual Clone carry (the same as the live Actions sheet).
            const int box = legacy.pokemonBox, slot = legacy.pokemonSlot;
            legacy.active = false;
            legacy.pokemonActions = false;
            screen.controllerNavigation.reset();
            if (!ClassicPackedMove::beginCloneFromAction(screen, box, slot))
                screen.postStatus("Clone could not start from here", 240);
            return true;
        }
        case SharedEditor::Action::LegalityProvenance: {
            auto& shared = sharedSurfaceStateFor(screen);
            shared.provenanceActive = true;
            legacy.pokemonActions = false;
            screen.controllerNavigation.reset();
            return true;
        }
        case SharedEditor::Action::Add:
            return beginCreate(screen, legacy.pokemonBox);
        case SharedEditor::Action::Review:
            legacy.review = true;
            legacy.reviewRow = 0;
            screen.controllerNavigation.reset();
            return true;
        case SharedEditor::Action::Close:
            legacy.active = false;
            legacy.pokemonActions = false;
            screen.controllerNavigation.reset();
            return true;
        default:
            return true;
    }
}

bool handleSharedProvenance(TrainerViewScreen& screen, uint64_t down) {
    auto& shared = sharedSurfaceStateFor(screen);
    if (!shared.provenanceActive) return false;
    if (!(down & (HidNpadButton_A | HidNpadButton_B))) return true;
    shared.provenanceActive = false;
    auto& legacy = editorOverlayState(screen);
    legacy.active = true;
    legacy.pokemonActions = true;
    legacy.review = false;
    screen.controllerNavigation.reset();
    return true;
}

bool handleSharedReview(TrainerViewScreen& screen, uint64_t down, uint64_t held,
                        int stickX, int stickY) {
    auto& legacy = editorOverlayState(screen);
    if (!legacy.active || !legacy.review) return false;
    auto* editor = stagedEditor(screen);
    if (!editor) return true;
    // Discard Staged is two explicit steps: X asks, only Y in the confirmation discards.
    namespace Discard = PokeBank::UIModel::Gen12ReviewDiscard;
    switch (Discard::step(sharedSurfaceStateFor(screen).reviewDiscard,
                          (down & HidNpadButton_Y) != 0, (down & HidNpadButton_Y) != 0,
                          (down & HidNpadButton_B) != 0)) {
        case Discard::Outcome::NotHandled:
            break;
        case Discard::Outcome::AskConfirm:
        case Discard::Outcome::Cancelled:
            screen.controllerNavigation.reset();
            return true;
        case Discard::Outcome::Swallowed:
            return true;
        case Discard::Outcome::Discard: {
            editor->discard();
            std::string refreshError;
            gscTrainer(screen).refreshStagedBoxPresentation(refreshError);
            legacy.review = false;
            legacy.pokemonActions = screen.selectedMode != TrainerViewScreen::ViewMode::Trainer;
            if (PokeBank::UIModel::ClassicReviewOrigin::takeStandalone(screen)) legacy.pokemonActions = false; // standalone Review
            legacy.active = legacy.pokemonActions;
            legacy.pokemonRow = 0;
            screen.controllerNavigation.reset();
            screen.postStatus("All staged changes discarded - still editing; the original save is unchanged", 300);
            return true;
        }
    }
    const auto& changes = editor->pendingChanges();
    const int count = std::max(1, static_cast<int>(changes.size()));
    const uint64_t nav = screen.controllerNavigation.apply(
        down, held, stickX, stickY,
        HidNpadButton_Up, HidNpadButton_Down, HidNpadButton_Left, HidNpadButton_Right);
    namespace Review=PokeBank::UIModel::SharedReview;
    if (nav & (HidNpadButton_Left|HidNpadButton_Up|HidNpadButton_L))
        legacy.reviewRow=static_cast<int>(Review::move(legacy.reviewRow,-1,count));
    if (nav & (HidNpadButton_Right|HidNpadButton_Down|HidNpadButton_R))
        legacy.reviewRow=static_cast<int>(Review::move(legacy.reviewRow,1,count));
    if (down & HidNpadButton_B) {
        legacy.review = false;
        legacy.pokemonActions = screen.selectedMode != TrainerViewScreen::ViewMode::Trainer;
        if (PokeBank::UIModel::ClassicReviewOrigin::takeStandalone(screen)) legacy.pokemonActions = false; // standalone Review
        legacy.active = legacy.pokemonActions;
        legacy.pokemonRow = 0;
        screen.controllerNavigation.reset();
        return true;
    }
    if (down & HidNpadButton_X) {
        std::string error;
        const std::string directory = exportStagedCopy(screen, *editor, error);
        if (directory.empty())
            screen.postStatus(error.empty() ? "Verified staged export failed" : error, 360);
        else {
            legacy.lastExportDirectory = directory;
            screen.postStatus("Verified staged copy exported: " + directory, 360);
        }
        return true;
    }
    return true;
}

void drawSharedActionSurface(TrainerViewScreen& screen, PKSEFramebuffer& fb) {
    auto& legacy = editorOverlayState(screen);
    auto* editor = stagedEditor(screen);
    if (!editor) return;
    std::string error;
    const auto pokemon = editor->boxedPokemon(static_cast<std::size_t>(legacy.pokemonBox),
                                              static_cast<std::size_t>(legacy.pokemonSlot), error);
    const auto actions = SharedEditor::actionsForSlot(
        pokemon.has_value(), gen2ActionCapabilities(), !editor->pendingChanges().empty());
    legacy.pokemonRow = std::clamp(
        legacy.pokemonRow, 0, std::max(0, static_cast<int>(actions.count) - 1));
    const auto model = PokeBank::UIModel::ClassicPokemonActionMenu::fromShared(actions);
    drawNarrowPokemonActions(fb, screen.touchButtons, model, legacy.pokemonRow,
                            pokemon ? "Pokemon Actions" : "Empty Slot",
                            "GEN II / READ-ONLY SOURCE", 31600);
    UI::SharedEditorChrome::drawPickerFooter(fb, {true, false}); // Browse, A Select, B Cancel
}

void drawSharedProvenanceSurface(TrainerViewScreen& screen, PKSEFramebuffer& fb) {
    namespace P=PokeBank::UIModel::SharedEditorPresentation;
    auto* editor=stagedEditor(screen);if(!editor)return;
    const auto& legacy=editorOverlayState(screen);std::string error;
    const auto p=legacy.pokemonParty?editor->partyPokemon(static_cast<size_t>(legacy.pokemonSlot),error):
        editor->boxedPokemon(static_cast<size_t>(legacy.pokemonBox),static_cast<size_t>(legacy.pokemonSlot),error);
    std::vector<std::pair<std::string,std::string>> rows{
        {"Source game",sharedGameName(screen)},
        {"Location",legacy.pokemonParty?"Party "+std::to_string(legacy.pokemonSlot+1):
            "Box "+std::to_string(legacy.pokemonBox+1)+" / Slot "+std::to_string(legacy.pokemonSlot+1)},
        {"Native format","PK2; save checksums verified on export"},
        {"Encounter / transfer rules","Not evaluated"},
        {"Rule coverage","Editor guardrails only; no complete legality check"}};
    if(p) {
        rows.push_back({"Pokemon",std::string(Names::getSpeciesName(p->species))+" #"+std::to_string(p->species)});
        rows.push_back({"Nickname",p->nickname});rows.push_back({"Original Trainer",p->originalTrainer});
        rows.push_back({"Trainer ID",std::to_string(p->trainerId)});
        if(screen.sourceGameId=="crystal_gbc") {
            const auto caught=Gen2Native::decodeCrystalCaughtData(p->caughtData);
            rows.push_back({"Met level",Gen2Native::crystalCaughtLevelText(caught)});
            rows.push_back({"Met location",Gen2Native::crystalCaughtLocationName(caught.location)});
        }
        for(size_t i=0;i<4;++i)rows.push_back({"Move "+std::to_string(i+1),p->moves[i]?Names::getMoveName(p->moves[i]):"Empty"});
    }
    UI::SharedEditorChrome::drawOriginLegalityPanel(fb,screen.touchButtons,rows,P::LegalityState::NotChecked);
}

void drawSharedReviewSurface(TrainerViewScreen& screen, PKSEFramebuffer& fb) {
    auto* editor=stagedEditor(screen);
    if(!editor)return;
    PokeBank::UIModel::SharedReview::Rows rows;
    // An empty "before" is a new value (a created Pokemon's details, round 6 P3).
    for(const auto& change:editor->pendingChanges())
        rows.emplace_back(change.label,change.beforeValue.empty()?change.afterValue
                                                                 :change.beforeValue+" -> "+change.afterValue);
    SharedEditorChrome::drawReviewPanel(fb,screen.touchButtons,rows,
        static_cast<size_t>(editorOverlayState(screen).reviewRow),editor->hasPendingChanges(),"Generation II");
    if(sharedSurfaceStateFor(screen).reviewDiscard.confirming)
        SharedEditorChrome::drawReviewDiscardConfirm(fb,screen.touchButtons);
}

void drawSharedDetails(PKSEFramebuffer& fb, const State& state, int x, int y, int w, int h,
                       bool passive) {
    drawPanelSurface(fb, x, y, w, h, false, 14);
    PokeBank::UIModel::PanelHeading::draw(fb, x, y, "DETAILS");
    const auto& p = state.working;
    // Unown's letter is the form computed from the DVs (pokecrystal GetUnownLetter, Gen12DVReroll::unownLetter); the
    // sprite follows the draft's own DVs so a shiny / Random DVs / typed-DV change shows the new letter.
    const uint8_t spriteForm = p.species == 201
        ? static_cast<uint8_t>(PokeBank::UIModel::Gen12DVReroll::unownLetter({p.dvs[1], p.dvs[2], p.dvs[3], p.dvs[4]}))
        : uint8_t{0};
    if (auto* sprite = SpriteManager::getSprite(p.species, spriteForm, p.shiny); sprite && sprite->data)
        fb.drawSpriteStaticContained(x + 63, y + 34, 174, 146,
                                     sprite->width, sprite->height, sprite->data, sprite->channels);
    else
        fb.drawText(x + 82, y + 104, "Sprite unavailable", Colors::TextDim, TextStyle::Caption);

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

    const std::array<std::pair<std::string, std::string>, 5> rows{{
        {"Species", std::string(Names::getSpeciesName(p.species)) + " #" + std::to_string(p.species)},
        {"Nickname", p.nickname},
        {"Level / EXP", "Lv " + std::to_string(p.level) + " / " + std::to_string(p.experience)},
        {"OT", p.originalTrainer},
        {"Trainer ID", std::to_string(p.trainerId)},
    }};
    for (int i = 0; i < 5; ++i) {
        const int yy = y + 224 + i * 43;
        const bool selected = !passive && state.panel == Foundation::Panel::Identity && state.row == i;
        if (selected) fb.drawSelectionHighlight(x + 8, yy, w - 16, 38);
        fb.drawText(x + 18, yy + 7, rows[static_cast<std::size_t>(i)].first,
                    selected ? Colors::SelectedText : Colors::TextDim, TextStyle::Caption);
        fb.drawText(x + 108, yy + 7, rows[static_cast<std::size_t>(i)].second, Colors::Text, TextStyle::Caption);
    }
}

void drawSharedMoves(PKSEFramebuffer& fb, const TrainerViewScreen& screen, const State& state,
                     int x, int y, int w, int h, bool passive) {
    drawPanelSurface(fb, x, y, w, h, false, 14);
    PokeBank::UIModel::PanelHeading::draw(fb, x, y, "MOVES");
    fb.drawText(x + 190, y + 42, "PP", Colors::TextDim, TextStyle::Caption);
    fb.drawText(x + 246, y + 42, "Ups", Colors::TextDim, TextStyle::Caption);
    for (int slot = 0; slot < 4; ++slot) {
        const int yy = y + 72 + slot * 42;
        const bool selected = !passive && state.panel == Foundation::Panel::Moves && state.row == slot;
        if (selected) fb.drawSelectionHighlight(x + 10, yy - 5, w - 20, 36);
        const uint8_t move = state.working.moves[static_cast<std::size_t>(slot)];
        const auto status = moveStatus(screen, state, slot);
        fb.drawText(x + 18, yy, move == 0 ? "Empty" : Names::getMoveName(move),
                    Colors::Text, TextStyle::Caption);
        fb.drawText(x + 190, yy, std::to_string(state.working.pp[static_cast<std::size_t>(slot)]),
                    Colors::Text, TextStyle::Caption);
        fb.drawText(x + 246, yy, std::to_string(state.working.ppUps[static_cast<std::size_t>(slot)]),
                    Colors::Text, TextStyle::Caption);
        fb.drawText(x + 292, yy, Rules::compatibilityText(status), statusColor(status), TextStyle::Caption);
    }
    const int statusY = y + 246;
    fb.drawHDivider(x + 14, statusY - 12, w - 28);
    fb.drawText(x + 18, statusY, "Legality", Colors::TextDim, TextStyle::Caption);   // the pane below is titled Origin & Legality
    fb.drawText(x + 152, statusY, PokeBank::UIModel::SharedEditorPresentation::legalityLabel(PokeBank::UIModel::SharedEditorPresentation::LegalityState::NotChecked), Colors::Text, TextStyle::Caption);
    fb.drawText(x + 18, statusY + 28, PokeBank::UIModel::SharedEditorPresentation::kOriginLegalityHeading, Colors::Accent, TextStyle::Caption);

    const auto& p = state.working;
    const auto* personal = Gen2::personalRecord(p.species);
    const auto battle = Gen2::calculateBattleStats(p.species, p.level, editableDVs(p), p.statExperience).asArray();
    const uint32_t nextExp = personal && p.level < 100
        ? Pokemon::getExpForLevel(static_cast<uint8_t>(p.level + 1), personal->experienceGrowth)
        : p.experience;
    const uint32_t remaining = nextExp > p.experience ? nextExp - p.experience : 0;

    int infoY = statusY + 58;
    auto infoRow = [&](const char* label, const std::string& value) {
        fb.drawText(x + 18, infoY, label, Colors::TextDim, TextStyle::Caption);
        fb.drawText(x + 104, infoY, value, Colors::Text, TextStyle::Caption);
        infoY += 24;
    };
    infoRow("Game", sharedGameName(screen));
    infoRow("Gender", genderText(p.gender));
    infoRow("Held", itemText(p.heldItem));
    infoRow("Friend", std::to_string(p.friendship));
    infoRow("Pokerus", Gen2Native::pokerusText(p.pokerus));
    if (const auto party = Gen2Native::partyViewData(p)) {
        infoRow("Party HP", std::to_string(party->currentHP) + " / " + std::to_string(party->maxHP));
        infoRow("Status", party->statusText);
    } else {
        infoRow("Growth", personal ? Rules::growthGroupName(personal->experienceGrowth) : "Unknown");
        infoRow("EXP next", std::to_string(remaining));
    }

    if (screen.sourceGameId == "crystal_gbc") {
        const auto caught = Gen2Native::decodeCrystalCaughtData(p.caughtData);
        const std::string met = std::string(Gen2Native::crystalMetTimeName(caught.timeOfDay)) + " / " +
                                Gen2Native::crystalCaughtLevelText(caught);
        fb.drawText(x + 18, y + h - 42, "Met", Colors::TextDim, TextStyle::Caption);
        fb.drawText(x + 58, y + h - 42, met + " / " + Gen2Native::crystalCaughtLocationName(caught.location),
                    Colors::Text, TextStyle::Caption);
    }
    StatsRadar::drawGen2Labeled(fb, x + w - 180, statusY + 62, 164, 150, battle);
}

void drawSharedWorkspace(TrainerViewScreen& screen, PKSEFramebuffer& fb) {
    auto& state = stateFor(screen);
    const bool view = state.mode == Mode::View;
    const bool create = state.mode == Mode::Create;
    const auto g = SharedEditor::geometry720p();

    screen.touchButtons.clear();
    fb.drawFilledRect(0, 0, fb.getWidth(), fb.getHeight() - 48, Color(0, 0, 0, 105));
    drawPanelSurface(fb, g.x, g.y, g.width, g.height, false, 18);
    const auto surface = view ? SharedEditor::Surface::View :
                         create ? SharedEditor::Surface::CreateDraft : SharedEditor::Surface::Edit;
    fb.drawText(g.x + 24, g.y + 14, SharedEditor::titleFor(surface), Colors::Text, TextStyle::Heading);
    std::string subtitle = "Box " + std::to_string(state.box + 1);
    if (state.slot >= 0) subtitle += " / Slot " + std::to_string(state.slot + 1);
    subtitle += "   •   ";
    subtitle += sharedGameName(screen);
    subtitle += "   •   Source save immutable";
    fb.drawText(g.x + 24, g.y + 46, subtitle, Colors::TextDim, TextStyle::Caption);

    const int contentY = g.y + 74;
    const int contentH = g.height - 94;
    const int leftX = g.x + 18, leftW = 300;
    const int midX = leftX + leftW + 14, midW = 398;
    const int rightX = midX + midW + 14, rightW = g.x + g.width - 18 - rightX;

    drawSharedDetails(fb, state, leftX, contentY, leftW, contentH, view);
    State visual = state;
    if (view) {
        visual.panel = static_cast<Foundation::Panel>(255);
        visual.row = -1;
        visual.column = 0;
    }
    drawValues(fb, visual, midX, contentY, midW, contentH);
    drawSharedMoves(fb, screen, visual, rightX, contentY, rightW, contentH, view);

    if (view) {
        screen.touchButtons.push_back({PokeBank::UIModel::passiveBackTarget, 16, fb.getHeight() - 68, 190, 58});
        drawNavBar(fb, {{"B", "Back"}});
    } else if (create) {
        drawNavBar(fb, {{"D-pad/Stick", "Navigate"}, {"L/R", "Panel"}, {"A", "Edit"},
                        {"Y", "Random DVs"}, {"X", "Add"}, {"B", "Exit"}});
    } else {
        drawNavBar(fb, {{"D-pad/Stick", "Navigate"}, {"L/R", "Panel"}, {"A", "Edit"},
                        {"Y", "Random DVs"}, {"B", "Exit"}});
    }
    if (state.itemPicker) drawHeldItemPicker(fb, state);
    if (state.confirmExit) drawConfirm(screen, fb, state);
}

} // namespace

bool sharedSurfaceOwnsFrame(const TrainerViewScreen& screen) noexcept {
    if (!isGSC(screen)) return false;
    const auto& shared = sharedSurfaceStateFor(screen);
    const auto& legacy = editorOverlayState(screen);
    return shared.provenanceActive || stateFor(screen).mode != Mode::None ||
           (legacy.active && (legacy.pokemonActions || legacy.review));
}

bool handleSharedSurfaceInput(TrainerViewScreen& screen, uint64_t down, uint64_t held,
                              int stickX, int stickY, const TouchInput& touch) {
    if (!isGSC(screen)) return false;
    if (handleSharedProvenance(screen, down)) return true;
    if (handleSharedReview(screen, down, held, stickX, stickY)) return true;
    if (handleSharedActions(screen, down, held, stickX, stickY, touch)) return true;
    auto& state = stateFor(screen);
    if (state.mode == Mode::View) {
        if (PokeBank::UIModel::passiveViewAction(down & HidNpadButton_B, screen.touchedButtonId(touch)) ==
            PokeBank::UIModel::PassiveViewAction::Back)
            closeToActions(screen);
        return true;
    }
    return false;
}

bool drawSharedSurface(TrainerViewScreen& screen, PKSEFramebuffer& fb) {
    if (!isGSC(screen)) return false;
    const auto& shared = sharedSurfaceStateFor(screen);
    auto& legacy = editorOverlayState(screen);
    if (shared.provenanceActive) { drawSharedProvenanceSurface(screen, fb); return true; }
    if (legacy.active && legacy.review) { drawSharedReviewSurface(screen, fb); return true; }
    if (stateFor(screen).mode != Mode::None) { drawSharedWorkspace(screen, fb); return true; }
    if (legacy.active && legacy.pokemonActions) { drawSharedActionSurface(screen, fb); return true; }
    return false;
}

} // namespace UI::Gen2PokemonEditor
