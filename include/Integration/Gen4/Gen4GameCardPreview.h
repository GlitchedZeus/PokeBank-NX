#ifndef POKEBANK_GEN4_GAME_CARD_PREVIEW_H
#define POKEBANK_GEN4_GAME_CARD_PREVIEW_H

#include "Integration/Gen4/Gen4AssignedSource.h"
#include "Source/HomeCardSnapshot.h"
#include "Utils/StringHelpers.h"

#include <algorithm>
#include <cstddef>

namespace PokeVault::Integration::Gen4 {

// Read-only Home card snapshot of an explicitly assigned Gen IV save. It
// carries exactly what the Home hero already showed for Gen IV (trainer,
// gender, dex, party species/level/form); it only moves the strict parse out
// of every focus change and into the fingerprinted snapshot cache.
inline constexpr const char* UnavailablePartyReason = "Choose a Save Instance to view its party.";

[[nodiscard]] inline Source::HomeCardSnapshot homeCardSnapshot(const AssignedSourceReadOnly& opened) {
    if(opened.status!=OpenStatus::Ready || !opened.save)
        return Source::unavailableSnapshot(UnavailablePartyReason);
    const auto& save=*opened.save;
    Source::HomeCardSnapshot s;
    s.state=Source::SnapshotState::Current;
    s.gameId=opened.source.binding.gameIdentity;
    s.sourceIdentity=opened.source.sourceIdentity;
    s.trainerName=Utils::utf16ToUtf8(save.trainer().name);
    s.trainerGender=save.trainer().gender;
    s.trainerGenderKnown=true;
    const auto dex=save.dexProgress();
    s.dexSeen=dex.seen;
    s.dexCaught=dex.caught;
    s.dexTotal=dex.total;
    const auto party=save.party();
    s.partyCount=static_cast<uint8_t>(std::min(party.size(),s.party.size()));
    for(size_t i=0;i<s.partyCount;++i) {
        if(!party[i].valid())continue;
        // Gen IV Home shiny badges are not part of the accepted card output.
        s.party[i]={party[i].species(),party[i].partyLevel(),party[i].form(),false};
    }
    return s;
}

} // namespace PokeVault::Integration::Gen4
#endif
