#ifndef UI_MODALS_GEN1_POKEMON_DETAILS_MODAL_H
#define UI_MODALS_GEN1_POKEMON_DETAILS_MODAL_H

namespace Pokemon { class Pokemon1ReadOnly; }
namespace UI { class PKSEFramebuffer; class TrainerViewScreen; }

namespace UI::Modals {
    // Generation I has a materially different data model from later formats (DVs/stat experience,
    // no nature/ability/held item/SID/modern met metadata). This renderer keeps the normal View
    // Pokemon entry point while presenting only fields that exist in the validated PK1 record.
    void drawGen1PokemonDetailsModal(UI::TrainerViewScreen& screen, UI::PKSEFramebuffer& fb,
                                     const Pokemon::Pokemon1ReadOnly& pokemon);
}

#endif
