#ifndef POKEBANK_GEN5_GAME_CARD_PREVIEW_H
#define POKEBANK_GEN5_GAME_CARD_PREVIEW_H

#include "Integration/Gen5/Gen5AssignedSource.h"
#include "Integration/Gen5/Gen5TrainerName.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>

namespace PokeVault::Integration::Gen5 {

// Immutable, source-backed details for the existing Game / Save Instances UI.
// Consuming this model does not open an editor, create a Pokemon, or grant
// write access to an emulator source. An explicit validated assignment is
// required before any trainer/party/dex details are returned.
struct PartyPreviewSlot {
    bool occupied = false;
    uint16_t species = 0;
    uint16_t heldItem = 0;
    uint8_t nature = 0;
    uint8_t level = 0; // only the native party extension stores level
    bool shiny = false;
};
struct GameCardReadOnlyPreview {
    std::string exactGameId;
    std::string providerLabel;
    std::string sourceIdentity;
    std::string trainerName; // empty if Gen V text was malformed
    uint8_t trainerGender = 0;
    bool trainerGenderKnown = false;
    uint16_t dexSeen = 0;
    uint16_t dexCaught = 0;
    uint16_t dexTotal = 0;
    uint8_t partyCount = 0;
    bool backupCopySelected = false;
    std::array<PartyPreviewSlot,6> party{};
};
[[nodiscard]] inline std::optional<GameCardReadOnlyPreview> previewAssignedGame(
    const AssignedSourceReadOnly& assigned) {
    if(!assigned.ready() || !isExactGen5Id(assigned.instance.gameId) ||
       assigned.save->exactGameId()!=assigned.instance.gameId)
        return std::nullopt;
    const auto& save=*assigned.save;
    GameCardReadOnlyPreview p;
    p.exactGameId=assigned.instance.gameId;
    p.providerLabel=assigned.instance.providerLabel;
    p.sourceIdentity=assigned.instance.sourceIdentity;
    p.trainerName=displayTrainerName(save.trainer().rawName).value_or("");
    p.trainerGender=save.trainer().gender;
    p.trainerGenderKnown=p.trainerGender<=1;
    p.dexSeen=save.dexProgress().seen;
    p.dexCaught=save.dexProgress().caught;
    p.dexTotal=save.dexProgress().total;
    p.partyCount=save.partyCount();
    p.backupCopySelected=save.selectedBackupPartition();
    for(size_t slot=0;slot<p.partyCount;++slot) {
        const auto pk=save.partyPokemon(slot);
        if(!pk || !pk->valid() || pk->empty())return std::nullopt;
        const uint8_t level=pk->partyLevel();
        p.party[slot]={true,pk->species(),pk->heldItem(),pk->nature(),
                       level>=1 && level<=100 ? level : uint8_t{0},pk->shiny()};
    }
    return p;
}
} // namespace PokeVault::Integration::Gen5
#endif
