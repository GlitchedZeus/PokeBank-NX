#include "Integration/Gen1/Gen1MoveCompatibility.h"
#include "UI/Gen1PokemonDetailsPresentation.h"
#include "UI/PokemonViewActions.h"

namespace UI::Gen1PokemonEditor {
namespace {

std::string foundationPassiveSourceGameLabel(const std::string& id) {
    if (id.find("yellow") != std::string::npos || id.find("Yellow") != std::string::npos) return "Yellow";
    if (id.find("blue") != std::string::npos || id.find("Blue") != std::string::npos) return "Blue";
    if (id.find("red") != std::string::npos || id.find("Red") != std::string::npos) return "Red";
    return "Gen I";
}

} // namespace

bool foundationPassiveViewActive(const TrainerViewScreen& screen) noexcept {
    return ux2StateFor(screen).mode == UX2Mode::View;
}

bool handleFoundationPassiveViewInput(TrainerViewScreen& screen, uint64_t down,
                                      int touchTarget = -1) {
    if (!foundationPassiveViewActive(screen)) return false;

    if (PokeBank::UIModel::passiveViewAction(down & HidNpadButton_B, touchTarget) ==
        PokeBank::UIModel::PassiveViewAction::Back)
        foundationExitMainMode(screen);
    return true;
}

void drawFoundationPassiveView(TrainerViewScreen& screen, PKSEFramebuffer& fb) {
    if (!foundationPassiveViewActive(screen)) return;

    auto& state = ux2StateFor(screen);
    auto* e = editor(screen);
    if (!e) return;

    std::string error;
    const auto pokemon = ux2SelectedPokemon(screen, error);
    if (!pokemon) {
        screen.postStatus(error.empty() ? "Pokemon unavailable" : error, 300);
        return;
    }

    Modals::Gen1PokemonDetailsPresentation view;
    view.species = pokemon->species;
    view.speciesName = std::string(Names::getSpeciesName(pokemon->species));
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

    const auto presentation = PokeBank::UIModel::presentGen1Pokemon(
        pokemon->species, pokemon->level, storedDVs(*pokemon), pokemon->statExperience);
    view.shiny = presentation.shiny;
    view.nativeTypes = presentation.nativeTypes;
    view.battleStats = presentation.battleStats;
    view.hasBattleStats = true;
    view.battleStatsCalculated = true;
    view.catchRate = pokemon->catchRate;
    view.setMoveCompatibility(e->metadata().sourceGame);
    view.sourceGameLabel = foundationPassiveSourceGameLabel(screen.sourceGameId);
    view.recordLabel = "Box " + std::to_string(state.box + 1) + " / Slot " + std::to_string(state.slot + 1);
    view.sourceStateLabel = "STAGED READ ONLY";

    Modals::drawGen1PokemonDetailsPresentation(screen, fb, view);
    // Keep View aligned with Create/Edit: panel titles are neutral and the
    // Gen I middle panel uses the shared STATS vocabulary instead of VALUES.
    drawNeutralGen1WorkspaceHeadings(fb);
    screen.touchButtons.push_back({PokeBank::UIModel::passiveBackTarget,
                                   16, fb.getHeight() - 68, 190, 58});
}

} // namespace UI::Gen1PokemonEditor