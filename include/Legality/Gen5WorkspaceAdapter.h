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
    const auto trainer = workspace.sourceSave().trainer();
    options.trainer.present = true;
    options.trainer.tid = trainer.tid;
    options.trainer.sid = trainer.sid;
    options.trainer.female = trainer.gender != 0;
    options.trainer.language = trainer.language;
    options.trainer.ot.fill(0xFFFF);
    std::copy_n(trainer.rawName.begin(), std::min(trainer.rawName.size(), size_t{7}),
                options.trainer.ot.begin());
    return analyze(pk->decodedBytes(), workspace.sourceSave().exactGameId(), options);
}
} // namespace Legality::Gen5App
