#ifndef UI_GEN1_POKEMON_DETAILS_PRESENTATION_H
#define UI_GEN1_POKEMON_DETAILS_PRESENTATION_H

#include <array>
#include <cstdint>
#include <string>

namespace UI { class PKSEFramebuffer; class TrainerViewScreen; }

namespace UI::Modals {

// Truthful Generation I view model shared by the mature read-only PK1 Party/Box detail path.
// It deliberately contains only fields that physically exist in the Gen I record or are
// deterministically derived from them. This is presentation-only; no mutation path lives here.
struct Gen1PokemonDetailsPresentation {
    uint16_t species = 0;
    std::string speciesName;
    std::string nickname;
    uint8_t level = 0;
    uint32_t experience = 0;
    std::string originalTrainer;
    uint16_t trainerId = 0;
    std::array<uint8_t,5> dvs{}; // HP (derived), Attack, Defense, Speed, Special
    std::array<uint16_t,5> statExperience{};
    std::array<uint16_t,4> moves{};
    std::array<uint8_t,4> pp{};
    std::array<uint8_t,4> ppUps{};
    std::array<bool,4> moveCompatible{};
    std::array<uint16_t,5> battleStats{};
    std::array<uint8_t,2> nativeTypes{}; // exact Gen I type IDs
    bool hasBattleStats = false;
    bool battleStatsCalculated = false;
    bool shiny = false;
    uint8_t catchRate = 0;
    bool moveCompatibilityChecked = false;
    bool moveCompatibilityCompatible = false;
    std::string sourceGameLabel = "Gen I";
    std::string recordLabel;
    std::string sourceStateLabel = "READ ONLY";
};

void drawGen1PokemonDetailsPresentation(TrainerViewScreen& screen, PKSEFramebuffer& fb,
                                        const Gen1PokemonDetailsPresentation& pokemon);

} // namespace UI::Modals

#endif