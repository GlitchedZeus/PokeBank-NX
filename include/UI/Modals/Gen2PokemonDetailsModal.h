#ifndef UI_MODALS_GEN2_POKEMON_DETAILS_MODAL_H
#define UI_MODALS_GEN2_POKEMON_DETAILS_MODAL_H

namespace Pokemon { class Pokemon2ReadOnly; }
namespace UI { class PKSEFramebuffer; class TrainerViewScreen; }

namespace UI::Modals {
// Generation II PK2 uses DVs/stat experience and has no Nature, Ability, SID, modern met/Ball
// metadata, ribbons/marks, HOME tracker, or native modern IV/EV semantics. Keep the normal View
// Pokemon entry point while rendering only fields actually carried by the validated PK2 model.
void drawGen2PokemonDetailsModal(UI::TrainerViewScreen& screen, UI::PKSEFramebuffer& fb,
                                 const Pokemon::Pokemon2ReadOnly& pokemon);
}

#endif
