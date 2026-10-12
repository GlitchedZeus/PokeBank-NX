#ifndef POKEBANK_GEN5_GAME_SOURCE_CATALOG_H
#define POKEBANK_GEN5_GAME_SOURCE_CATALOG_H

#include "Integration/Gen5/Gen5SaveInstanceAdapter.h"
#include "Legacy/LegacySourceBindings.h"

#include <cstddef>
#include <span>
#include <string_view>
#include <vector>

namespace PokeVault::Integration::Gen5 {

// Read-only Save Instances presentation input, NOT a production screen or
// automatic binding/assignment operation. Caller supplies strictly inspected
// raw/DSV rows; no filesystem discovery or game launching happens here.
struct GameSourceCatalog {
    std::vector<Source::SaveInstance> rows;
    size_t wrongGame = 0;
    size_t invalidOrAmbiguous = 0;
    size_t ownedByOtherProfile = 0;
    bool validQuery = false;
};

[[nodiscard]] inline GameSourceCatalog forGameAndProfile(
    std::span<const Source::SaveInstance> discovered,
    const Legacy::LegacySourceBindings& bindings,
    std::string_view profile,std::string_view exactGame) {

    GameSourceCatalog out;
    if(profile.empty() || !isExactGen5Id(exactGame))return out;
    out.validQuery=true;

    // Deduplicate before applying profile ownership: a physical file with
    // multiple path aliases must never expose one unclaimed alias while the
    // other alias is owned by another profile.
    std::vector<Source::SaveInstance> normalized;
    for(const auto& observed:discovered) {
        if(observed.generation!=5 || observed.platformLabel!="Nintendo DS")continue;
        if(!observed.ready()) {++out.invalidOrAmbiguous;continue;}
        if(observed.gameId!=exactGame) {++out.wrongGame;continue;}
        if(observed.sourceIdentity.empty() || observed.physicalIdentity.empty() ||
           observed.providerLabel.empty() || observed.providerId.empty() ||
           observed.sourcePath.empty() || observed.contentFingerprint.empty() ||
           !observed.readOnly()) {
            ++out.invalidOrAmbiguous;continue;
        }
        Source::appendDeduplicated(normalized,observed);
    }
    for(auto& row:normalized) {
        bindings.applyClaims(row);
        if(!Source::visibleToProfile(row,profile)) {
            ++out.ownedByOtherProfile;
            continue;
        }
        row.rememberedSource=bindings.isPreferredGameSource(row,profile,exactGame);
        out.rows.push_back(std::move(row));
    }
    Source::sortNewestFirst(out.rows);
    return out;
}

} // namespace PokeVault::Integration::Gen5
#endif
