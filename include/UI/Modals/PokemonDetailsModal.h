#ifndef UI_MODALS_POKEMON_DETAILS_MODAL_H
#define UI_MODALS_POKEMON_DETAILS_MODAL_H

// Forward declarations
namespace UI {
    class PKSEFramebuffer;
    class TrainerViewScreen;
}

namespace UI {
namespace Modals {
    // The accepted large modern renderer keeps its implementation source byte-for-byte. When this
    // header is reached from TrainerViewScreen.h, the public call token is redirected to the small
    // generation dispatcher. When PokemonDetailsModal.cpp includes the header directly first, its
    // existing definition is compiled under the Modern symbol instead. This avoids rewriting the
    // large accepted renderer solely to add one early Gen II branch.
    void drawPokemonDetailsModalModern(UI::TrainerViewScreen& screen, UI::PKSEFramebuffer& fb);
    void drawPokemonDetailsModalDispatch(UI::TrainerViewScreen& screen, UI::PKSEFramebuffer& fb);
}
}

#ifdef UI_TRAINER_VIEW_SCREEN_H
#define drawPokemonDetailsModal drawPokemonDetailsModalDispatch
#else
#define drawPokemonDetailsModal drawPokemonDetailsModalModern
#endif

#endif
