#pragma once

// ClassicPartyActions.h - L06 / E02 / NEW-20 (Round 3 hardware): in Gen I-III, A or a tap on an
// occupied Party slot opens the shared Pokemon Actions popup (View / Edit / Origin & Legality /
// Cancel, the Gen IV Party order) instead of jumping straight into View.
//
// The Gen I-III staged editors stage BOX records only (Gen IV is the first with a party record
// API), so Edit is listed but disabled -- shown greyed, never focusable, never activated -- rather
// than hidden or faked. View is the existing read-only View route; Origin & Legality is the one
// shared panel (UI/SharedOriginLegality.h) filled from the party record.

#include "UI/NarrowPokemonActionMenu.h"

#include <array>

namespace PokeBank::UIModel::ClassicPartyActions {

enum class Action : unsigned char { View, Edit, Legality, Cancel };

struct Entry {
    Action action;
    const char* label;
    bool enabled;
};

inline constexpr std::array<Entry, 4> kEntries{{
    {Action::View, "View", true},
    {Action::Edit, "Edit", false},
    {Action::Legality, NarrowPokemonMenu::kOriginLegalityLabel, true},
    {Action::Cancel, "Cancel", true},
}};

inline constexpr const char* kEditUnavailable =
    "Party Pok\xC3\xA9mon are edited from a PC box in this generation (staged edits are per box)";

// The generations this popup serves (Gen IV and V draw their own Party Actions).
constexpr bool appliesTo(int generation) noexcept { return generation >= 1 && generation <= 3; }

// Up / Down move between ENABLED rows only, wrapping.
constexpr int moveRow(int row, int delta) noexcept {
    constexpr int count = static_cast<int>(kEntries.size());
    if (delta == 0) return row;
    const int step = delta < 0 ? -1 : 1;
    int next = row;
    for (int i = 0; i < count; ++i) {
        next = ((next + step) % count + count) % count;
        if (kEntries[static_cast<std::size_t>(next)].enabled) return next;
    }
    return row;
}

static_assert(kEntries[0].action == Action::View && kEntries[3].action == Action::Cancel,
              "shared order: View first, Cancel last");
static_assert(moveRow(0, 1) == 2 && moveRow(2, -1) == 0 && moveRow(3, 1) == 0 && moveRow(0, -1) == 3,
              "the disabled Edit row is never focused");

} // namespace PokeBank::UIModel::ClassicPartyActions
