#ifndef POKEBANK_GEN5_SHARED_REVIEW_H
#define POKEBANK_GEN5_SHARED_REVIEW_H

#include "Integration/Gen5/Gen5StagedPokemonWorkspace.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace PokeVault::Integration::Gen5 {

// Read-only evidence for the one shared Review Pending Changes surface.
// Independently reconstruct each staged PK5 through ONLY the four authorized
// field transactions. An unsupported byte difference produces no verified
// diff; it is never silently labeled legal or applied to any source SAV5.
struct VerifiedFieldChange {
    StagedPokemon5Record::Field field = StagedPokemon5Record::Field::Nature;
    size_t stat = 0;
    uint32_t before = 0;
    uint32_t after = 0;
};
struct VerifiedSlotReview {
    Gen5StagedPokemonWorkspace::Slot location;
    std::vector<VerifiedFieldChange> fields;
};

[[nodiscard]] inline std::optional<VerifiedSlotReview> verifyStagedReview(
    const Gen5StagedPokemonWorkspace::PendingChange& change,
    std::string* error = nullptr) {
    using Field = StagedPokemon5Record::Field;
    const auto fail = [&](const char* why) -> std::optional<VerifiedSlotReview> {
        if(error)*error=why;
        return std::nullopt;
    };
    if(!Gen5StagedPokemonWorkspace::canonicalSlot(change.location))
        return fail("Gen V review slot identity is noncanonical");
    const Pokemon5ReadOnly before(std::span<const uint8_t>(change.before));
    const Pokemon5ReadOnly after(std::span<const uint8_t>(change.after));
    if(!before.valid() || before.empty() || !after.valid() || after.empty() ||
       before.partyRecord()!=(change.location.region==
                              Gen5StagedPokemonWorkspace::Region::Party) ||
       after.partyRecord()!=before.partyRecord()) {
        return fail("Gen V pending review contains unvalidated or wrong-format PK5");
    }
    auto candidate=StagedPokemon5Record::create(
        std::span<const uint8_t>(change.before),error);
    if(!candidate)return std::nullopt;
    VerifiedSlotReview result;
    result.location=change.location;
    const auto apply=[&](Field field,size_t stat,uint32_t from,uint32_t to) {
        if(from==to)return true;
        if(!candidate->stage(field,stat,to,error))return false;
        result.fields.push_back({field,stat,from,to});
        return true;
    };
    if(!apply(Field::Nature,0,before.nature(),after.nature()) ||
       !apply(Field::Friendship,0,before.friendship(),after.friendship()))
        return std::nullopt;
    const auto bi=before.ivs(),ai=after.ivs();
    for(size_t i=0;i<6;++i)
        if(!apply(Field::IV,i,bi[i],ai[i]))return std::nullopt;
    const auto be=before.evs(),ae=after.evs();
    // Match the atomic Keep order so a fully legal final EV distribution is
    // not falsely rejected by an intermediate value above the 510 cap.
    for(size_t i=0;i<6;++i)
        if(ae[i]<be[i] && !apply(Field::EV,i,be[i],ae[i]))
            return std::nullopt;
    for(size_t i=0;i<6;++i)
        if(ae[i]>be[i] && !apply(Field::EV,i,be[i],ae[i]))
            return std::nullopt;
    if(result.fields.empty())
        return fail("Gen V pending review contains no verified field changes");
    const auto replay=candidate->stagedBytes();
    if(replay.size()!=change.after.size() ||
       !std::equal(replay.begin(),replay.end(),change.after.begin()))
        return fail("Gen V pending review includes unrelated PK5 byte changes");
    if(error)error->clear();
    return result;
}

} // namespace PokeVault::Integration::Gen5
#endif
