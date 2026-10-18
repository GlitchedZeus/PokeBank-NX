#pragma once

// Review presentation rows for the Gen V staged workspace (owner round 1 S04 follow-up: a staged
// Move showed as two Review rows, "Clone to Box" + "Remove from Box", so one action read as two).
//
// Gen5StagedPokemonWorkspace::moveToEmptyBox stages a Move as one atomic pair of the already
// verified replacements: a CloneBox at the destination whose cloneSource is the original slot,
// plus a RemoveBox of that original slot. The staged operations are unchanged; this only GROUPS
// that pair into one "Move" row for Review. A Clone into an empty slot plus a Release of its
// unchanged original has the same net effect and is therefore also shown as one Move.
// Everything else is one row per staged record, in pendingReview() order.

#include "Integration/Gen5/Gen5StagedPokemonWorkspace.h"

#include <cstddef>
#include <optional>
#include <vector>

namespace PokeVault::Integration::Gen5 {

struct ReviewRow5 {
    enum class Kind : uint8_t { Record, Move };
    Kind kind = Kind::Record;
    std::size_t change = 0;   // index into pendingReview(): the record, or a Move's Clone half
    std::size_t removal = 0;  // Move only: index of the Remove half (the original slot)
};

[[nodiscard]] inline std::vector<ReviewRow5> groupReviewRows5(
    const std::vector<Gen5StagedPokemonWorkspace::PendingChange>& pending) {
    using Kind = Gen5StagedPokemonWorkspace::ChangeKind;
    const auto sameSlot = [](const auto& a, const auto& b) {
        return a.region == b.region && a.box == b.box && a.slot == b.slot;
    };
    // For each Clone, the Remove of its own (unchanged) original slot, if one is staged.
    std::vector<std::optional<std::size_t>> pairedRemoval(pending.size());
    std::vector<bool> folded(pending.size(), false);
    for (std::size_t c = 0; c < pending.size(); ++c) {
        const auto& clone = pending[c];
        if (clone.kind != Kind::CloneBox) continue;
        for (std::size_t r = 0; r < pending.size(); ++r) {
            const auto& removal = pending[r];
            if (folded[r] || removal.kind != Kind::RemoveBox ||
                !sameSlot(removal.location, clone.cloneSource) ||
                removal.before != clone.cloneSourceOriginal) continue;
            pairedRemoval[c] = r;
            folded[r] = true;
            break;
        }
    }
    std::vector<ReviewRow5> rows;
    for (std::size_t i = 0; i < pending.size(); ++i) {
        if (folded[i]) continue;
        if (pairedRemoval[i]) rows.push_back({ReviewRow5::Kind::Move, i, *pairedRemoval[i]});
        else rows.push_back({ReviewRow5::Kind::Record, i, 0});
    }
    return rows;
}

[[nodiscard]] inline std::size_t reviewRecordRows5(const Gen5StagedPokemonWorkspace& workspace) {
    return groupReviewRows5(workspace.pendingReview()).size();
}

} // namespace PokeVault::Integration::Gen5
