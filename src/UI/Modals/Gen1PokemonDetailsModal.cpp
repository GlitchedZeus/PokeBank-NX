#include "UI/Modals/Gen1PokemonDetailsModal.h"

#include "Integration/Gen1/Gen1Shiny.h"
#include "Pokemon/Pokemon1ReadOnly.h"
#include "UI/Gen1PokemonDetailsPresentation.h"
#include "UI/PKSEFramebuffer.h"
#include "UI/TrainerViewScreen.h"
#include "Utils/StringHelpers.h"

#include <array>
#include <string>

namespace UI::Modals {

void drawGen1PokemonDetailsModal(TrainerViewScreen& screen, PKSEFramebuffer& fb,
                                 const Pokemon::Pokemon1ReadOnly& p) {
    Gen1PokemonDetailsPresentation view;
    view.species = p.speciesID();
    view.speciesName = p.species();
    view.nickname = Utils::utf16ToUtf8(p.nickname());
    view.level = p.level();
    view.experience = p.exp();
    view.originalTrainer = Utils::utf16ToUtf8(p.otName());
    view.trainerId = p.tid16();
    view.dvs = {static_cast<uint8_t>(p.dvHP()), static_cast<uint8_t>(p.dvATK()),
                static_cast<uint8_t>(p.dvDEF()), static_cast<uint8_t>(p.dvSPE()),
                static_cast<uint8_t>(p.dvSpecial())};
    view.statExperience = {static_cast<uint16_t>(p.statExpHP()), static_cast<uint16_t>(p.statExpATK()),
                           static_cast<uint16_t>(p.statExpDEF()), static_cast<uint16_t>(p.statExpSPE()),
                           static_cast<uint16_t>(p.statExpSpecial())};
    for (int slot = 0; slot < 4; ++slot) {
        const auto i = static_cast<size_t>(slot);
        view.moves[i] = static_cast<uint16_t>(p.move(slot));
        view.pp[i] = static_cast<uint8_t>(p.movePP(slot));
        view.ppUps[i] = static_cast<uint8_t>(p.movePPUps(slot));
    }

    const std::array<uint8_t,4> storedDVs{view.dvs[1], view.dvs[2], view.dvs[3], view.dvs[4]};
    view.shiny = PokeVault::Integration::Gen1::ShinyDVs::isShiny(storedDVs);
    view.recordLabel = p.isPartyRecord() ? "Party (44 bytes)" : "Box (33 bytes)";
    view.sourceStateLabel = "READ ONLY";

    if (p.isPartyRecord()) {
        view.battleStats = {static_cast<uint16_t>(p.statHPMax()), static_cast<uint16_t>(p.statATK()),
                            static_cast<uint16_t>(p.statDEF()), static_cast<uint16_t>(p.statSPE()),
                            static_cast<uint16_t>(p.gen1Special())};
        view.hasBattleStats = true;
        view.battleStatsCalculated = false;
    }

    drawGen1PokemonDetailsPresentation(screen, fb, view);
}

} // namespace UI::Modals
