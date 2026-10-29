// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include "Integration/Gen3/Gen3StagedPokemonEditor.h"
#include "Legality/Gen3BerryContext.h"
#include "Legality/Legality.h"
#include "Pokemon/Pokemon3FRLG.h"
#include <optional>

namespace Legality::Gen3App {
namespace G = PokeVault::Integration::Gen3;
struct Result {
    Report report;
    G::SourceGame sourceGame;
    G::PokemonLocation location;
    uint8_t activeSlot;
    uint32_t saveCounter;
};

// The provider selects the Pokemon AND evidence from the same validated save.
// There is deliberately no overload taking an unrelated Bank Pokemon and a save.
inline std::optional<Result> analyzeSave(const G::ReadOnlySave& save,
                                        const G::PokemonLocation& location) {
    const bool party = location.kind == G::PokemonLocation::Kind::Party;
    if ((party && (location.box != 0 || location.slot >= save.metadata().partyCount)) ||
        (!party && (location.box >= save.metadata().boxCount ||
                    location.slot >= save.metadata().slotsPerBox))) return std::nullopt;
    const auto records = party ? save.party() : save.boxes();
    if (save.lastEnumerationError() != G::SaveError::None) return std::nullopt;
    for (const auto& record : records) {
        if (record.location.kind != location.kind || record.location.box != location.box ||
            record.location.slot != location.slot) continue;
        if (!record.species || !record.checksumValid || !record.byteIdenticalRoundTrip ||
            record.originalBytes.size() != (party ? 100u : 80u)) return std::nullopt;
        const Pokemon::Pokemon3FRLG pk(std::as_bytes(std::span(record.originalBytes)));
        if (!pk.checksumValid() || pk.speciesID() != record.species || pk.pid() != record.pid ||
            pk.tid16() != record.tid || pk.sid16() != record.sid) return std::nullopt;
        const auto& metadata = save.metadata();
        const auto berry = Gen3BerryContext::fromSave(save.sourceBytes(), metadata.sourceGameId,
                                                     save.trainer().language);
        // Invalid berry data remains unavailable evidence inside a valid save;
        // the production analyzer decides whether this Pokemon needs that evidence.
        auto report = Legality::analyze(pk, Context{Enums::GameVersion::FRLG,
                                      metadata.sourceGameId, berry ? &*berry : nullptr});
        return Result{std::move(report), metadata.sourceGame, location,
                      metadata.activeSlot, metadata.saveCounter};
    }
    return std::nullopt;
}

inline std::optional<Result> analyzeWorkspace(const G::StagedPokemonEditor& workspace,
                                             const G::PokemonLocation& location) {
    // Strict reparse includes native sector checksums and active-copy selection.
    // Reading stagedBytes(), rather than originalBytes(), makes edits/moves visible.
    auto parsed = G::parse(workspace.stagedBytes(), workspace.sourceGame());
    if (!parsed) return std::nullopt;
    return analyzeSave(*parsed.save, location);
}
} // namespace Legality::Gen3App
