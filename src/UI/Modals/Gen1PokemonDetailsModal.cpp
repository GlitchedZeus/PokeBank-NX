#include "UI/Modals/Gen1PokemonDetailsModal.h"

#include "Pokemon/Pokemon1ReadOnly.h"
#include "UI/Gen1PokemonDetailsPresentation.h"
#include "UI/Gen1PokemonPresentation.h"
#include "UI/PKSEFramebuffer.h"
#include "UI/TrainerViewScreen.h"
#include "Utils/StringHelpers.h"

#include <array>
#include <string>

namespace UI::Modals {
namespace {

std::string sourceGameLabel(const std::string& id) {
    if (id.find("yellow") != std::string::npos || id.find("Yellow") != std::string::npos) return "Yellow";
    if (id.find("blue") != std::string::npos || id.find("Blue") != std::string::npos) return "Blue";
    if (id.find("red") != std::string::npos || id.find("Red") != std::string::npos) return "Red";
    return "Gen I";
}

} // namespace

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

    const auto presentation = PokeBank::UIModel::presentGen1Pokemon(p.strictRecord());
    view.shiny = presentation.shiny;
    view.nativeTypes = presentation.nativeTypes;
    view.catchRate = p.strictRecord().catchRate;
    view.sourceGameLabel = sourceGameLabel(screen.sourceGameId);
    view.recordLabel = p.isPartyRecord() ? "Party (44 bytes)" : "Box (33 bytes)";
    view.sourceStateLabel = "READ ONLY";

    if (p.isPartyRecord()) {
        // Party records physically store their battle stats. Show exactly those values.
        view.battleStats = {static_cast<uint16_t>(p.statHPMax()), static_cast<uint16_t>(p.statATK()),
                            static_cast<uint16_t>(p.statDEF()), static_cast<uint16_t>(p.statSPE()),
                            static_cast<uint16_t>(p.gen1Special())};
        view.battleStatsCalculated = false;
    } else {
        // Box records do not store battle stats; use the same trusted calculation path as the
        // accepted Gen I editor so Storage/Box View and editor View agree.
        view.battleStats = presentation.battleStats;
        view.battleStatsCalculated = true;
    }
    view.hasBattleStats = true;

    drawGen1PokemonDetailsPresentation(screen, fb, view);
}

} // namespace UI::Modals
