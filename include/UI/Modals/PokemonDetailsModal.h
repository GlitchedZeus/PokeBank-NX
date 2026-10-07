#ifndef UI_MODALS_POKEMON_DETAILS_MODAL_H
#define UI_MODALS_POKEMON_DETAILS_MODAL_H

// Forward declarations
namespace UI {
    class PKSEFramebuffer;
    class TrainerViewScreen;
}

namespace UI {
namespace Modals {
    struct ReportScrollViewport {
        int x = 0;
        int y = 0;
        int w = 0;
        int h = 0;
        static constexpr int RowHeight = 30;

        [[nodiscard]] constexpr bool contains(int px, int py) const noexcept {
            return px >= x && px < x + w && py >= y && py < y + h;
        }
    };

    [[nodiscard]] constexpr ReportScrollViewport reportScrollViewport(
        int screenW, int screenH, int panelWidth) noexcept {
        constexpr int panelY = 50;
        const int panelH = screenH - 100;
        const int panelX = (screenW - panelWidth) / 2;
        const int listTop = panelY + 78;
        const int listBottom = panelY + panelH - 72;
        return {panelX + 20, listTop, panelWidth - 40, listBottom - listTop};
    }

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
