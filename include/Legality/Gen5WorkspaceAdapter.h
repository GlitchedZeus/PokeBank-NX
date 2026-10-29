// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include "Legality/Gen5AppAdapter.h"
#include <algorithm>
#include <optional>

namespace Legality::Gen5App {
// The app workspace owns the validated save and current staged slot. Keeping this
// bridge templated avoids importing an unfinished app lane into the engine lane.
// Instantiated against the exact #307 workspace by tests/app (see the handoff).
template<class Workspace>
std::optional<Result> analyzeWorkspace(const Workspace& workspace,
                                      const typename Workspace::Slot& slot) {
    if (!Workspace::canonicalSlot(slot)) return std::nullopt;
    const bool party = slot.region == Workspace::Region::Party;
    const auto pk = party ? workspace.viewParty(slot.slot)
                          : workspace.viewBox(slot.box, slot.slot);
    if (!pk) return std::nullopt;
    Options options;
    options.encrypted = false;
    options.slot = party ? Pk5Slot::Party : Pk5Slot::Box;
    const auto original = party ? workspace.sourceSave().partyPokemon(slot.slot)
                               : workspace.sourceSave().boxPokemon(slot.box, slot.slot);
    if (!original) return std::nullopt;
    options.source = std::equal(pk->decodedBytes().begin(), pk->decodedBytes().end(),
                                original->decodedBytes().begin(), original->decodedBytes().end())
        ? RecordSource::OriginalSlot : RecordSource::StagedSlot;
    // Trainer edits and Pokemon edits belong to the same current workspace.
    // Do not silently fall back to the original trainer if its current view
    // cannot be decoded, or normalize an invalid native gender to "female".
    const auto trainer = workspace.trainerPresentation();
    options.stagedTrainer = workspace.trainerDirty();
    if (trainer) {
        options.trainer.present = trainer->gender <= 1 && trainer->rawName.size() <= 7 &&
            trainer->game == versionFromSourceGameId(workspace.sourceSave().exactGameId());
        options.trainer.tid = trainer->tid;
        options.trainer.sid = trainer->sid;
        options.trainer.female = trainer->gender == 1;
        options.trainer.language = trainer->language;
        options.trainer.ot.fill(0xFFFF);
        std::copy_n(trainer->rawName.begin(), std::min(trainer->rawName.size(), size_t{7}),
                    options.trainer.ot.begin());
    }
    return analyze(pk->decodedBytes(), workspace.sourceSave().exactGameId(), options);
}
} // namespace Legality::Gen5App
