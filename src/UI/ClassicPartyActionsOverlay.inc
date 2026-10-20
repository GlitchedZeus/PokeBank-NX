// ClassicPartyActionsOverlay.inc - L06 / E02 / NEW-20: the Gen I-III Party Actions popup.
// Pure rules: include/UI/ClassicPartyActions.h. Included by TrainerViewScreenCompositeOverlay.cpp
// after every generation surface so it can reuse their Origin & Legality report builders.
//
// Input order: the composite update() calls handleInput() right after Gen V's isolation and before
// every Gen I-III handler, so the Party A / tap reaches this popup instead of the base route that
// opened View directly. While the popup (or its Origin & Legality panel) is up it consumes every
// input, so nothing reaches the Party view underneath. View keeps the existing read-only route.

#include "UI/ClassicPartyActions.h"
#include "UI/ClassicGenericUiGuard.h"
#include "Pokemon/Pokemon1ReadOnly.h"
#include "Pokemon/Pokemon2ReadOnly.h"

namespace UI::ClassicPartyActions {
namespace {

namespace Model = PokeBank::UIModel::ClassicPartyActions;
namespace OL = PokeBank::UIModel::OriginLegality;

constexpr int kTouchBase = 77950;       // popup rows
constexpr int kPartyTouchBase = 6200;   // the base Party slots (publishBasePartyTouchTargets)

struct State {
    const TrainerViewScreen* owner = nullptr;
    std::string sourceGameId;
    bool open = false;
    bool legality = false;
    int slot = 0;
    int row = 0;
};

State& stateFor(const TrainerViewScreen& screen) {
    static State state;
    if (state.owner != &screen || state.sourceGameId != screen.sourceGameId) {
        state = {};
        state.owner = &screen;
        state.sourceGameId = screen.sourceGameId;
    }
    return state;
}

int generation(const TrainerViewScreen& screen) noexcept {
    return PokeBank::UIModel::ClassicGenericUi::classicGeneration(screen.sourceGameId);
}

// The Party view with nothing else open (the same guard the base Party touch path uses).
bool partySurface(const TrainerViewScreen& screen) noexcept {
    return Model::appliesTo(generation(screen)) && screen.detailViewActive &&
           screen.selectedMode == TrainerViewScreen::ViewMode::Party &&
           !screen.helpOverlayActive && !screen.details.active && !screen.actionSheet.isOpen() &&
           !screen.saveConfirmActive && !screen.pickerActive && !screen.itemEditDialogActive &&
           !screen.releaseConfirmActive && !screen.storageExitConfirmActive && !screen.groupMenuActive &&
           !screen.carrying() && !screen.swapActive && !screen.currentlySelecting;
}

const Pokemon::Pokemon* partyPokemon(const TrainerViewScreen& screen, int slot) {
    if (slot < 0 || slot >= static_cast<int>(screen.trainer.party.size())) return nullptr;
    const auto* pokemon = screen.trainer.party[static_cast<std::size_t>(slot)].get();
    return pokemon && pokemon->speciesID() != 0 ? pokemon : nullptr;
}

const char* sourceTag(int gen) noexcept {
    return gen == 1 ? " GB SOURCE / READ ONLY" : gen == 2 ? " GBC SOURCE / READ ONLY" : " GBA SOURCE / READ ONLY";
}

// Gen III party record (PK3 through the base Pokemon interface): its stored checksum, origin and the
// same exact-game move guardrail as the Gen III box editor. PK3 stores no met date.
OL::Report gen3PartyReport(const TrainerViewScreen& screen, const Pokemon::Pokemon& p) {
    OL::Report report;
    report.exactGame = std::string(PokeBank::UIModel::ClassicGameContext::exactGameName(screen.sourceGameId));
    report.format = "PK3";
    report.structure = p.checksum() == p.calculateChecksum() ? OL::Structure::Valid : OL::Structure::Invalid;
    report.originGame = OL::value(Enums::getOriginGameName(p.originGame()));
    report.metLocation = OL::value(Names::getMetLocationDisplayName(p.originGame(), p.metLocation()));
    report.metLevel = OL::value(p.metLevel() == 0 ? std::string("Hatched (level 0)") : std::to_string(p.metLevel()));
    report.metDate = OL::notStored();
    const auto descriptor =
        PokeVault::Integration::Gen3EditorProvider::descriptorForSource(screen.sourceGameId, true);
    using MoveResult = PokeBank::UIModel::ExactFormatEditor::MoveCompatibilityResult;
    for (int i = 0; i < 4; ++i) {
        const uint16_t move = p.move(i);
        if (move == 0) {
            report.moves.push_back({"Empty", "Empty", OL::Tone::Dim});
            continue;
        }
        auto result = MoveResult::Invalid;
        if (descriptor)
            result = descriptor->moves.evaluate({screen.sourceGameId, p.speciesID(), 0, move, true});
        OL::MoveCheck check{std::string(Names::getMoveName(move)), "Invalid", OL::Tone::Caution};
        if (result == MoveResult::Compatible) { check.status = "OK"; check.tone = OL::Tone::Good; }
        else if (result == MoveResult::PreserveExisting) check.status = "Preserved";
        else if (result == MoveResult::Unsupported) check.status = "Transfer";
        report.moves.push_back(std::move(check));
    }
    return report;
}

// The shared report for one party record. Each classic trainer builds exactly one party type
// (RBY -> Pokemon1ReadOnly, GSC -> Pokemon2ReadOnly), and this build has no RTTI, so the cast is
// selected by the exact source generation; Gen I / II then reuse their box report builders.
OL::Report partyReport(TrainerViewScreen& screen, const Pokemon::Pokemon& p) {
    switch (generation(screen)) {
        case 1:
            return Gen1PokemonEditor::gen1LegalityReport(
                screen, static_cast<const Pokemon::Pokemon1ReadOnly&>(p).strictRecord());
        case 2:
            return Gen2PokemonEditor::gen2LegalityReport(
                screen, static_cast<const Pokemon::Pokemon2ReadOnly&>(p).strictRecord());
        default:
            return gen3PartyReport(screen, p);
    }
}

void openPopup(TrainerViewScreen& screen, State& state, int slot) {
    screen.selectedPartyIndex = slot;
    state.open = true;
    state.legality = false;
    state.slot = slot;
    state.row = 0;
    screen.controllerNavigation.reset();
}

void activate(TrainerViewScreen& screen, State& state) {
    const auto entry = Model::kEntries[static_cast<std::size_t>(state.row)];
    if (!entry.enabled) {
        screen.postStatus(Model::kEditUnavailable, 300);
        return;
    }
    switch (entry.action) {
        case Model::Action::View: {
            const int slot = state.slot;
            state.open = false;
            // The existing Gen I-III read-only View route (unchanged).
            screen.openPokemonActionSheet({PokeVault::UIModel::PokemonLocation::Party, 0, slot});
            return;
        }
        case Model::Action::Legality: state.legality = true; return;
        case Model::Action::Cancel: state.open = false; return;
        case Model::Action::Edit: return;
    }
}

} // namespace

bool active(const TrainerViewScreen& screen) {
    return Model::appliesTo(generation(screen)) && stateFor(screen).open;
}

bool handleInput(TrainerViewScreen& screen, u64 down, u64 held, int stickX, int stickY, const TouchInput& touch) {
    if (!Model::appliesTo(generation(screen))) return false;
    auto& state = stateFor(screen);
    if (!state.open) {
        if (!partySurface(screen)) return false;
        const int tap = screen.touchedButtonId(touch);
        if (tap >= kPartyTouchBase && tap < kPartyTouchBase + 6) {
            const int slot = tap - kPartyTouchBase;
            screen.selectedPartyIndex = slot;
            if (partyPokemon(screen, slot)) openPopup(screen, state, slot);
            return true;
        }
        if ((down & HidNpadButton_A) && partyPokemon(screen, screen.selectedPartyIndex)) {
            openPopup(screen, state, screen.selectedPartyIndex);
            return true;
        }
        return false;
    }
    // The popup and its panel own every input until they close.
    if (!partyPokemon(screen, state.slot)) {
        state.open = false;
        return true;
    }
    if (state.legality) {
        if (down & HidNpadButton_B) state.legality = false;
        return true;
    }
    const int tap = screen.touchedButtonId(touch);
    if (tap >= kTouchBase && tap < kTouchBase + static_cast<int>(Model::kEntries.size())) {
        state.row = tap - kTouchBase;
        activate(screen, state);
        return true;
    }
    if (down & HidNpadButton_B) {
        state.open = false;
        return true;
    }
    const u64 keys = screen.controllerNavigation.apply(down, held, stickX, stickY,
        HidNpadButton_Up, HidNpadButton_Down, HidNpadButton_Left, HidNpadButton_Right);
    if (keys & HidNpadButton_Up) state.row = Model::moveRow(state.row, -1);
    if (keys & HidNpadButton_Down) state.row = Model::moveRow(state.row, 1);
    if (down & HidNpadButton_A) activate(screen, state);
    return true;
}

bool draw(TrainerViewScreen& screen, PKSEFramebuffer& fb) {
    if (!active(screen)) return false;
    auto& state = stateFor(screen);
    const auto* pokemon = partyPokemon(screen, state.slot);
    if (!pokemon) return false;
    screen.drawGSCOverlay(fb);
    const std::string title = Names::getSpeciesName(pokemon->speciesID());
    if (state.legality) {
        SharedEditorChrome::drawOriginLegalityPanel(fb, screen.touchButtons,
                                        title + "  \xE2\x80\xA2  Party " + std::to_string(state.slot + 1),
                                        partyReport(screen, *pokemon));
        return true;
    }
    PokeBank::UIModel::NarrowPokemonMenu::Model model{};
    for (std::size_t i = 0; i < Model::kEntries.size(); ++i)
        model.append(Model::kEntries[i].label, Model::kEntries[i].enabled ? static_cast<int>(i) : -1);
    drawNarrowPokemonActions(fb, screen.touchButtons, model, state.row, title,
                             screen.legacyProviderLabel() + sourceTag(generation(screen)), kTouchBase);
    SharedEditorChrome::Presentation::PickerFooterSpec footer;
    footer.page = false;
    SharedEditorChrome::drawPickerFooter(fb, footer);
    return true;
}

} // namespace UI::ClassicPartyActions
