#include "UI/TrainerViewScreen.h"
#include "UI/LegacyPresentationRules.h"
#include "UI/Modals/Gen1PokemonDetailsModal.h"
#include "UI/Modals/Gen2PokemonDetailsModal.h"
#include "Pokemon/Pokemon.h"
#include "Pokemon/Pokemon1ReadOnly.h"
#include "Pokemon/Pokemon2ReadOnly.h"

#ifdef drawPokemonDetailsModal
#undef drawPokemonDetailsModal
#endif

namespace UI::Modals {

void drawPokemonDetailsModalDispatch(UI::TrainerViewScreen& screen, UI::PKSEFramebuffer& fb) {
    const Pokemon::Pokemon* pokemon = screen.detailsTargetPokemon();
    if (!pokemon || pokemon->speciesID() == 0) return;

    switch (pokemonDetailsLayoutFor(pokemon->getGameGroup())) {
        case PokemonDetailsLayout::Generation1:
            drawGen1PokemonDetailsModal(
                screen, fb, static_cast<const Pokemon::Pokemon1ReadOnly&>(*pokemon));
            return;
        case PokemonDetailsLayout::Generation2:
            drawGen2PokemonDetailsModal(
                screen, fb, static_cast<const Pokemon::Pokemon2ReadOnly&>(*pokemon));
            return;
        case PokemonDetailsLayout::Modern:
            drawPokemonDetailsModalModern(screen, fb);
            return;
    }
}

} // namespace UI::Modals
