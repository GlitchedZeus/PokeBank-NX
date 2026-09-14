#include "UI/SharedPokemonEditorContract.h"
#include "UI/Gen2NativePresentation.h"

namespace UI::Gen2PokemonEditor {
namespace {

namespace SharedEditor = PokeBank::UIModel::SharedPokemonEditor;
namespace Gen2Native = PokeBank::UIModel::Gen2Native;

struct SharedSurfaceState {
    const TrainerViewScreen* owner = nullptr;
    std::string sourceGameId;
    bool cloneActive = false;
    int cloneBox = 0;
};

SharedSurfaceState& sharedSurfaceStateFor(const TrainerViewScreen& screen) {
    static SharedSurfaceState state;
    if (state.owner != &screen || state.sourceGameId != screen.sourceGameId) {
        state = {};
        state.owner = &screen;
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
    caps.hasLegalityProvenance = false;
    return caps;
}

std::string sharedActionLabel(SharedEditor::Action action) {
    switch (action) {
        case SharedEditor::Action::View: return "View";
        case SharedEditor::Action::Edit: return "Edit";
        case SharedEditor::Action::Clone: return "Clone";
        case SharedEditor::Action::Remove: return "Remove";
        case SharedEditor::Action::LegalityProvenance: return "Legality & Provenance";
        case SharedEditor::Action::Add: return "Add Pokemon";
        case SharedEditor::Action::Review: return "Review Pending Changes";
        case SharedEditor::Action::Close: return "Close";
        case SharedEditor::Action::None: return "";
    }
    return "";
}

std::string sharedActionDescription(SharedEditor::Action action, const Gen2::StagedEditor& editor,
                                    const std::optional<Gen2::PokemonRecord>& pokemon) {
    switch (action) {
        case SharedEditor::Action::View: return "Read-only shared Pokemon view";
        case SharedEditor::Action::Edit:
            return pokemon ? pokemon->nickname + "  Lv " + std::to_string(pokemon->level) : "Edit Pokemon";
        case SharedEditor::Action::Clone: return "Choose a destination box visually";
        case SharedEditor::Action::Add: return "Open Create Pokemon — DRAFT";
        case SharedEditor::Action::Review:
            return std::to_string(editor.pendingChanges().size()) + " staged change(s)";
        case SharedEditor::Action::Close: return "Return to boxes";
        default: return "";
    }
}

int firstEmptyGen2Slot(Gen2::StagedEditor& editor, int box) {
    for (int slot = 0; slot < static_cast<int>(editor.metadata().boxCapacity); ++slot) {
        std::string error;
        if (!editor.boxedPokemon(static_cast<std::size_t>(box), static_cast<std::size_t>(slot), error))
            return slot;
    }
    return -1;
}

bool handleSharedActions(TrainerViewScreen& screen, uint64_t down, uint64_t held,
                         int stickX, int stickY) {
    auto& legacy = editorOverlayState(screen);
    auto* editor = stagedEditor(screen);
    if (!editor || !legacy.active || legacy.review || !legacy.pokemonActions) return false;

    std::string error;
    auto pokemon = editor->boxedPokemon(static_cast<std::size_t>(legacy.pokemonBox),
                                        static_cast<std::size_t>(legacy.pokemonSlot), error);
    const bool occupied = pokemon.has_value();
    const auto actions = SharedEditor::actionsForSlot(occupied, gen2ActionCapabilities());
    if (actions.count == 0) return true;

    const uint64_t nav = screen.controllerNavigation.apply(
        down, held, stickX, stickY,
        HidNpadButton_Up, HidNpadButton_Down, HidNpadButton_Left, HidNpadButton_Right);
    if (nav & HidNpadButton_Up)
        legacy.pokemonRow = (legacy.pokemonRow - 1 + static_cast<int>(actions.count)) % static_cast<int>(actions.count);
    if (nav & HidNpadButton_Down)
        legacy.pokemonRow = (legacy.pokemonRow + 1) % static_cast<int>(actions.count);
    legacy.pokemonRow = std::clamp(legacy.pokemonRow, 0, static_cast<int>(actions.count) - 1);

    if (down & HidNpadButton_B) {
        if (editor->hasPendingChanges()) {
            legacy.review = true;
            legacy.reviewRow = 0;
        } else {
            legacy.active = false;
            legacy.pokemonActions = false;
        }
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
            state.begin(*pokemon, Mode::View);
            screen.controllerNavigation.reset();
            return true;
        case SharedEditor::Action::Edit:
            return beginEdit(screen, legacy.pokemonBox, legacy.pokemonSlot);
        case SharedEditor::Action::Clone: {
            if (!pokemon) return true;
            auto& shared = sharedSurfaceStateFor(screen);
            shared.cloneActive = true;
            shared.cloneBox = legacy.pokemonBox;
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

bool handleSharedClone(TrainerViewScreen& screen, uint64_t down) {
    auto& shared = sharedSurfaceStateFor(screen);
    if (!shared.cloneActive) return false;
    auto* editor = stagedEditor(screen);
    if (!editor) { shared.cloneActive = false; return true; }
    const int boxes = static_cast<int>(editor->metadata().boxCount);
    if (down & HidNpadButton_L) shared.cloneBox = (shared.cloneBox - 1 + boxes) % boxes;
    if (down & HidNpadButton_R) shared.cloneBox = (shared.cloneBox + 1) % boxes;
    if (down & HidNpadButton_B) {
        shared.cloneActive = false;
        screen.controllerNavigation.reset();
        return true;
    }
    if (!(down & HidNpadButton_A)) return true;

    const int destination = firstEmptyGen2Slot(*editor, shared.cloneBox);
    if (destination < 0) {
        screen.postStatus("That Generation II box is full", 240);
        return true;
    }
    auto& legacy = editorOverlayState(screen);
    std::size_t destinationSlot = 0;
    std::string error;
    if (!editor->stageCloneBoxPokemon(static_cast<std::size_t>(legacy.pokemonBox),
                                      static_cast<std::size_t>(legacy.pokemonSlot),
                                      static_cast<std::size_t>(shared.cloneBox),
                                      destinationSlot, error)) {
        screen.postStatus(error.empty() ? "Clone failed" : error, 300);
        return true;
    }
    shared.cloneActive = false;
    gscTrainer(screen).refreshStagedBoxPresentation(error);
    screen.postStatus("Clone staged at Box " + std::to_string(shared.cloneBox + 1) +
                      " / Slot " + std::to_string(destinationSlot + 1) +
                      "; source .srm unchanged", 320);
    return true;
}

bool handleSharedReview(TrainerViewScreen& screen, uint64_t down, uint64_t held,
                        int stickX, int stickY) {
    auto& legacy = editorOverlayState(screen);
    if (!legacy.active || !legacy.review) return false;
    auto* editor = stagedEditor(screen);
    if (!editor) return true;
    const auto& changes = editor->pendingChanges();
    const int count = std::max(1, static_cast<int>(changes.size()));
    const uint64_t nav = screen.controllerNavigation.apply(
        down, held, stickX, stickY,
        HidNpadButton_Up, HidNpadButton_Down, HidNpadButton_Left, HidNpadButton_Right);
    if (nav & HidNpadButton_Up) legacy.reviewRow = (legacy.reviewRow - 1 + count) % count;
    if (nav & HidNpadButton_Down) legacy.reviewRow = (legacy.reviewRow + 1) % count;
    if (down & HidNpadButton_B) {
        legacy.review = false;
        legacy.pokemonActions = screen.selectedMode != TrainerViewScreen::ViewMode::Trainer;
        legacy.active = legacy.pokemonActions;
        legacy.pokemonRow = 0;
        screen.controllerNavigation.reset();
        return true;
    }
    if (down & HidNpadButton_X) {
        editor->discard();
        std::string refreshError;
        gscTrainer(screen).refreshStagedBoxPresentation(refreshError);
        legacy.review = false;
        legacy.pokemonActions = screen.selectedMode != TrainerViewScreen::ViewMode::Trainer;
        legacy.active = legacy.pokemonActions;
        legacy.pokemonRow = 0;
        screen.postStatus("All staged Gen II changes discarded; original source was never changed", 320);
        return true;
    }
    if (down & HidNpadButton_A) {
        std::string error;
        const std::string directory = exportStagedCopy(screen, *editor, error);
        if (directory.empty())
            screen.postStatus((error.empty() ? "Verified staged export failed" : error) +
                              std::string("; source remains unchanged"), 360);
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
    auto pokemon = editor->boxedPokemon(static_cast<std::size_t>(legacy.pokemonBox),
                                        static_cast<std::size_t>(legacy.pokemonSlot), error);
    const bool occupied = pokemon.has_value();
    const auto actions = SharedEditor::actionsForSlot(occupied, gen2ActionCapabilities());
    legacy.pokemonRow = std::clamp(legacy.pokemonRow, 0, std::max(0, static_cast<int>(actions.count) - 1));

    fb.drawFilledRect(0, 0, fb.getWidth(), fb.getHeight() - 48, Color(0, 0, 0, 105));
    constexpr int x = 250, y = 72, w = 780, h = 560;
    drawPanelSurface(fb, x, y, w, h, false, 18);
    fb.drawText(x + 28, y + 20, SharedEditor::titleFor(SharedEditor::Surface::Actions), Colors::Text, TextStyle::Heading);
    fb.drawText(x + 28, y + 54,
                "Box " + std::to_string(legacy.pokemonBox + 1) + " / Slot " +
                std::to_string(legacy.pokemonSlot + 1) + "   •   " + sharedGameName(screen) +
                "   •   Source save immutable",
                Colors::TextDim, TextStyle::Caption);

    int yy = y + 104;
    for (std::size_t i = 0; i < actions.count; ++i) {
        const bool selected = legacy.pokemonRow == static_cast<int>(i);
        if (selected) fb.drawSelectionHighlight(x + 22, yy - 7, w - 44, 52);
        fb.drawText(x + 38, yy, sharedActionLabel(actions[i]), selected ? Colors::Accent : Colors::Text,
                    TextStyle::Body);
        fb.drawText(x + 300, yy + 2, sharedActionDescription(actions[i], *editor, pokemon),
                    Colors::TextDim, TextStyle::Caption);
        yy += 58;
    }
    drawNavBar(fb, {{"D-pad/Stick", "Navigate"}, {"A", "Select"}, {"B", "Back"}});
}

void drawSharedCloneSurface(TrainerViewScreen& screen, PKSEFramebuffer& fb) {
    auto& shared = sharedSurfaceStateFor(screen);
    auto* editor = stagedEditor(screen);
    if (!editor) return;
    const int destination = firstEmptyGen2Slot(*editor, shared.cloneBox);
    fb.drawFilledRect(0, 0, fb.getWidth(), fb.getHeight() - 48, Color(0, 0, 0, 120));
    constexpr int x = 190, y = 74, w = 900, h = 550;
    drawPanelSurface(fb, x, y, w, h, false, 18);
    fb.drawText(x + 28, y + 20, "Clone Pokemon — Destination", Colors::Text, TextStyle::Heading);
    fb.drawText(x + 28, y + 54,
                "Box " + std::to_string(shared.cloneBox + 1) + "   •   first writable slot highlighted   •   source unchanged",
                Colors::TextDim, TextStyle::Caption);

    constexpr int cols = 5;
    constexpr int cellW = 150, cellH = 72;
    const int startX = x + 58, startY = y + 104;
    for (int slot = 0; slot < static_cast<int>(editor->metadata().boxCapacity); ++slot) {
        const int col = slot % cols, row = slot / cols;
        const int xx = startX + col * cellW, yy = startY + row * cellH;
        std::string error;
        const auto pokemon = editor->boxedPokemon(static_cast<std::size_t>(shared.cloneBox),
                                                  static_cast<std::size_t>(slot), error);
        const bool target = slot == destination;
        if (target) fb.drawSelectionHighlight(xx, yy, cellW - 10, cellH - 10);
        fb.drawRoundedRect(xx, yy, cellW - 10, cellH - 10, 8, target ? Colors::Accent : Colors::Divider, 1);
        fb.drawText(xx + 10, yy + 8, "Slot " + std::to_string(slot + 1), Colors::TextDim, TextStyle::Caption);
        fb.drawText(xx + 10, yy + 34,
                    pokemon ? std::string(Names::getSpeciesName(pokemon->species)) : (target ? "Clone here" : "Empty"),
                    target ? Colors::Accent : Colors::Text, TextStyle::Caption);
    }
    if (destination < 0)
        fb.drawText(x + 28, y + h - 62, "This box is full — choose another box with L/R", Colors::Error, TextStyle::Body);
    drawNavBar(fb, {{"L/R", "Box"}, {"A", "Stage Clone"}, {"B", "Cancel"}});
}

void drawSharedReviewSurface(TrainerViewScreen& screen, PKSEFramebuffer& fb) {
    auto& legacy = editorOverlayState(screen);
    auto* editor = stagedEditor(screen);
    if (!editor) return;
    const auto& changes = editor->pendingChanges();
    fb.drawFilledRect(0, 0, fb.getWidth(), fb.getHeight() - 48, Color(0, 0, 0, 105));
    constexpr int x = 150, y = 62, w = 980, h = 575;
    drawPanelSurface(fb, x, y, w, h, false, 18);
    fb.drawText(x + 28, y + 18, SharedEditor::titleFor(SharedEditor::Surface::Review), Colors::Text, TextStyle::Heading);
    fb.drawText(x + 28, y + 52,
                std::to_string(changes.size()) + " staged change(s)   •   original source save immutable",
                Colors::TextDim, TextStyle::Caption);
    if (changes.empty()) {
        fb.drawText(x + 34, y + 112, "No pending changes", Colors::TextDim, TextStyle::Body);
    } else {
        const int first = std::clamp(legacy.reviewRow - 5, 0, std::max(0, static_cast<int>(changes.size()) - 10));
        const int last = std::min(static_cast<int>(changes.size()), first + 10);
        for (int i = first; i < last; ++i) {
            const int yy = y + 96 + (i - first) * 44;
            const bool selected = i == legacy.reviewRow;
            if (selected) fb.drawSelectionHighlight(x + 24, yy - 5, w - 48, 39);
            const auto& change = changes[static_cast<std::size_t>(i)];
            fb.drawText(x + 36, yy, change.label, selected ? Colors::Accent : Colors::Text, TextStyle::Caption);
            fb.drawText(x + 350, yy, change.beforeValue + "  ->  " + change.afterValue,
                        Colors::TextDim, TextStyle::Caption);
        }
    }
    if (!legacy.lastExportDirectory.empty())
        fb.drawText(x + 28, y + h - 64, "Last verified export: " + legacy.lastExportDirectory,
                    Colors::Success, TextStyle::Caption);
    drawNavBar(fb, {{"D-pad/Stick", "Navigate"}, {"A", "Export staged copy"},
                    {"X", "Discard staged"}, {"B", "Back"}});
}

void drawSharedDetails(PKSEFramebuffer& fb, const State& state, int x, int y, int w, int h,
                       bool passive) {
    drawPanelSurface(fb, x, y, w, h, false, 14);
    fb.drawText(x + 14, y + 10, "DETAILS",
                !passive && state.panel == Foundation::Panel::Identity ? Colors::Accent : Colors::Text,
                TextStyle::Caption);
    const auto& p = state.working;
    if (auto* sprite = SpriteManager::getSprite(p.species, p.shiny); sprite && sprite->data)
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
        {"Species", std::to_string(p.species) + " - " + Names::getSpeciesName(p.species)},
        {"Nickname", p.nickname},
        {"Level / EXP", "Lv " + std::to_string(p.level) + " / " + std::to_string(p.experience)},
        {"OT", p.originalTrainer},
        {"Trainer ID", std::to_string(p.trainerId)},
    }};
    for (int i = 0; i < 5; ++i) {
        const int yy = y + 224 + i * 43;
        const bool selected = !passive && state.panel == Foundation::Panel::Identity && state.row == i;
        if (selected) fb.drawSelectionHighlight(x + 8, yy, w - 16, 38);
        fb.drawText(x + 18, yy + 7, rows[static_cast<std::size_t>(i)].first, Colors::TextDim, TextStyle::Caption);
        fb.drawText(x + 108, yy + 7, rows[static_cast<std::size_t>(i)].second, Colors::Text, TextStyle::Caption);
    }
}

void drawSharedMoves(PKSEFramebuffer& fb, const TrainerViewScreen& screen, const State& state,
                     int x, int y, int w, int h, bool passive) {
    drawPanelSurface(fb, x, y, w, h, false, 14);
    fb.drawText(x + 14, y + 10, "MOVES",
                !passive && state.panel == Foundation::Panel::Moves ? Colors::Accent : Colors::Text,
                TextStyle::Heading);
    fb.drawText(x + 190, y + 42, "PP", Colors::TextDim, TextStyle::Caption);
    fb.drawText(x + 246, y + 42, "Ups", Colors::TextDim, TextStyle::Caption);
    for (int slot = 0; slot < 4; ++slot) {
        const int yy = y + 72 + slot * 42;
        const bool selected = !passive && state.panel == Foundation::Panel::Moves && state.row == slot;
        if (selected) fb.drawSelectionHighlight(x + 10, yy - 5, w - 20, 36);
        const uint8_t move = state.working.moves[static_cast<std::size_t>(slot)];
        const auto status = moveStatus(screen, state, slot);
        fb.drawText(x + 18, yy, move == 0 ? "Empty" : Names::getMoveName(move),
                    selected && state.column == 0 ? Colors::Accent : Colors::Text, TextStyle::Caption);
        fb.drawText(x + 190, yy, std::to_string(state.working.pp[static_cast<std::size_t>(slot)]),
                    selected && state.column == 1 ? Colors::Accent : Colors::Text, TextStyle::Caption);
        fb.drawText(x + 246, yy, std::to_string(state.working.ppUps[static_cast<std::size_t>(slot)]),
                    selected && state.column == 2 ? Colors::Accent : Colors::Text, TextStyle::Caption);
        fb.drawText(x + 292, yy, Rules::compatibilityText(status), statusColor(status), TextStyle::Caption);
    }
    const int statusY = y + 246;
    fb.drawHDivider(x + 14, statusY - 12, w - 28);
    fb.drawText(x + 18, statusY, "Encounter legality", Colors::TextDim, TextStyle::Caption);
    fb.drawText(x + 152, statusY, "Not checked", Colors::Text, TextStyle::Caption);
    fb.drawText(x + 18, statusY + 28, "GEN II DATA", Colors::Accent, TextStyle::Caption);

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
        drawNavBar(fb, {{"B", "Back"}, {"Touch", "Back"}});
    } else if (create) {
        drawNavBar(fb, {{"D-pad/Stick", "Navigate"}, {"L/R", "Panel"}, {"A", "Edit"},
                        {"Y", "Random DVs"}, {"X", "Stage Add"}, {"B", "Cancel Draft"}});
    } else {
        drawNavBar(fb, {{"D-pad/Stick", "Navigate"}, {"L/R", "Panel"}, {"A", "Edit"},
                        {"Y", "Random DVs"}, {"B", "Back"}});
    }
    if (state.itemPicker) drawHeldItemPicker(fb, state);
    if (state.confirmExit) drawConfirm(fb);
}

} // namespace

bool sharedSurfaceOwnsFrame(const TrainerViewScreen& screen) noexcept {
    if (!isGSC(screen)) return false;
    const auto& shared = sharedSurfaceStateFor(screen);
    const auto& legacy = editorOverlayState(screen);
    return shared.cloneActive || stateFor(screen).mode != Mode::None ||
           (legacy.active && (legacy.pokemonActions || legacy.review));
}

bool handleSharedSurfaceInput(TrainerViewScreen& screen, uint64_t down, uint64_t held,
                              int stickX, int stickY, const TouchInput& touch) {
    if (!isGSC(screen)) return false;
    if (handleSharedClone(screen, down)) return true;
    if (handleSharedReview(screen, down, held, stickX, stickY)) return true;
    if (handleSharedActions(screen, down, held, stickX, stickY)) return true;
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
    if (shared.cloneActive) { drawSharedCloneSurface(screen, fb); return true; }
    if (legacy.active && legacy.review) { drawSharedReviewSurface(screen, fb); return true; }
    if (stateFor(screen).mode != Mode::None) { drawSharedWorkspace(screen, fb); return true; }
    if (legacy.active && legacy.pokemonActions) { drawSharedActionSurface(screen, fb); return true; }
    return false;
}

} // namespace UI::Gen2PokemonEditor
