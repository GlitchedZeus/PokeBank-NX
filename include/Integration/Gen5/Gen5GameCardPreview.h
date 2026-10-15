#ifndef POKEBANK_GEN5_GAME_CARD_PREVIEW_H
#define POKEBANK_GEN5_GAME_CARD_PREVIEW_H

#include "Integration/Gen5/Gen5AssignedSource.h"
#include "Integration/Gen5/Gen5TrainerName.h"
#include "Source/HomeCardSnapshot.h"

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
    uint8_t form = 0;  // PK5 0x40 bits 3..7 (PKHeX 6501f0ab PK5.Form)
};
struct GameCardReadOnlyPreview {
    std::string exactGameId;
    std::string providerLabel;
    std::string sourceIdentity;
    std::string trainerName; // empty if Gen V text was malformed
    uint8_t trainerGender = 0;
    bool trainerGenderKnown = false;
    // Freshly initialized save: no trainer written yet (neutral presentation).
    bool newGameNoTrainer = false;
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
    const auto& t=save.trainer();
    p.newGameNoTrainer=newGameNoTrainer5(t.rawName.size(),t.tid,t.sid,t.playedHours,
                                         t.playedMinutes,t.playedSeconds);
    p.trainerGender=t.gender;
    // The gender byte of a new-game save is not a gender: neutral portrait.
    p.trainerGenderKnown=!p.newGameNoTrainer && p.trainerGender<=1;
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
                       level>=1 && level<=100 ? level : uint8_t{0},pk->shiny(),
                       pk->form()};
    }
    return p;
}

// Card state for an assigned save whose strict preview failed. These are the
// established Gen V card states (copy gate, missing trainer, relink); the
// copy gate is never bypassed by picking a copy for the user.
enum class CardFailure : uint8_t { ChooseCopy, NoTrainerData, Relink };
[[nodiscard]] inline CardFailure classifyCardFailure(const std::string& diagnostic) {
    if(diagnostic.find("two valid but different")!=std::string::npos ||
       diagnostic.find("choose it explicitly")!=std::string::npos)
        return CardFailure::ChooseCopy;
    if(diagnostic.find("no trainer data")!=std::string::npos)
        return CardFailure::NoTrainerData;
    return CardFailure::Relink;
}
[[nodiscard]] inline const char* cardFailureReason(CardFailure failure) {
    switch(failure) {
        case CardFailure::ChooseCopy:
            return "Both save copies are valid but differ; choose one in Save Instances.";
        case CardFailure::NoTrainerData:
            return "Linked save has no trainer data; relink it in Save Instances.";
        case CardFailure::Relink: break;
    }
    return "Linked save did not pass validation; relink it in Save Instances.";
}

// Generation-neutral Home snapshot from the SAME strict preview. Nothing is
// presented unless every party record validated; there is no partial party.
[[nodiscard]] inline Source::HomeCardSnapshot homeCardSnapshot(const AssignedSourceReadOnly& assigned) {
    const auto preview=previewAssignedGame(assigned);
    if(!preview)
        return Source::unavailableSnapshot(cardFailureReason(classifyCardFailure(assigned.diagnostic)));
    Source::HomeCardSnapshot s;
    s.state=Source::SnapshotState::Current;
    s.gameId=preview->exactGameId;
    s.sourceIdentity=preview->sourceIdentity;
    s.contentFingerprint=assigned.instance.contentFingerprint;
    s.trainerName=preview->trainerName;
    s.trainerGender=preview->trainerGender;
    s.trainerGenderKnown=preview->trainerGenderKnown;
    s.newGameNoTrainer=preview->newGameNoTrainer;
    s.dexSeen=preview->dexSeen;
    s.dexCaught=preview->dexCaught;
    s.dexTotal=preview->dexTotal;
    s.partyCount=preview->partyCount;
    for(size_t i=0;i<preview->party.size();++i) {
        const auto& slot=preview->party[i];
        if(!slot.occupied)continue;
        s.party[i]={slot.species,slot.level,slot.form,slot.shiny};
    }
    return s;
}
} // namespace PokeVault::Integration::Gen5
#endif
