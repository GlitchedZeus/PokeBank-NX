#pragma once

// ONE routing rule for the generic (modern-generation) Pokemon UI in TrainerViewScreenBase.inc:
// the generic action sheet (ActionSheetModel.h: Make Shiny / Legality & Provenance
// placeholders), the editable details modal, its pickers and stat dialog, and the generic
// creator. A Gen I-V session never shows them: each generation presents its Pokemon through its
// own surface (Gen I/II overlays, Gen III/IV/V shared editor). Gen VI+ sessions are unchanged.
//
// QA CG11 binds to genericUiAllowedForSession() (TrainerViewScreenBase.inc), which wraps
// genericUiAllowed() below; routeClassicTarget() decides what a Gen I-V Pokemon target opens
// instead of the generic sheet.

#include "Inventory/ItemQuantityPolicy.h"

#include <cstdint>
#include <string_view>

namespace PokeBank::UIModel::ClassicGenericUi {

enum class GenericUi : uint8_t { ActionSheet, DetailsEditor, Picker, StatEditor, Creator };

// A Gen I-V session is any exact source the item-quantity policy knows by id
// (red_gb .. white2_nds); Gen VI+ ids are not in that table.
inline bool classicSession(std::string_view sourceGameId) noexcept {
    PokeVault::Inventory::QuantityPolicy::Game game{};
    return PokeVault::Inventory::QuantityPolicy::gameFromId(sourceGameId, game);
}

inline int classicGeneration(std::string_view sourceGameId) noexcept {
    PokeVault::Inventory::QuantityPolicy::Game game{};
    return PokeVault::Inventory::QuantityPolicy::gameFromId(sourceGameId, game)
        ? PokeVault::Inventory::QuantityPolicy::generation(game) : 0;
}

constexpr bool genericUiAllowed(bool classic, GenericUi) noexcept { return !classic; }

inline constexpr const char* kForeignPokemonRefusal = "Edit this Pok\xC3\xA9mon from its own game";

enum class TargetRoute : uint8_t { NativeView, Refuse };

// sessionGeneration: 1..5. targetGeneration: the target record's format generation (0 unknown).
// Party / save-box targets belong to the session and open its own read-only View, which the
// generation's surface adopts. A Bank target opens natively only where the session surface can
// present a Bank record of its own format: Gen III (the shared Gen III View re-encodes any PK3).
// Gen IV's View reads the native PK4 source view, Gen V owns its frames, and Gen I/II have no
// Bank presenter, so other Bank targets are refused rather than shown in the modern editor.
constexpr TargetRoute routeClassicTarget(int sessionGeneration, int targetGeneration, bool bankTarget) noexcept {
    if (sessionGeneration < 1 || sessionGeneration > 5) return TargetRoute::Refuse;
    if (!bankTarget) return TargetRoute::NativeView;
    return sessionGeneration == 3 && targetGeneration == 3 ? TargetRoute::NativeView : TargetRoute::Refuse;
}

static_assert(routeClassicTarget(3, 3, true) == TargetRoute::NativeView);
static_assert(routeClassicTarget(3, 8, true) == TargetRoute::Refuse);
static_assert(routeClassicTarget(4, 4, true) == TargetRoute::Refuse);
static_assert(routeClassicTarget(1, 1, false) == TargetRoute::NativeView);
static_assert(!genericUiAllowed(true, GenericUi::ActionSheet) && genericUiAllowed(false, GenericUi::Picker));

} // namespace PokeBank::UIModel::ClassicGenericUi
