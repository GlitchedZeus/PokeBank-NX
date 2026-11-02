#ifndef POKEBANK_GEN5_HUB_ROUTING_H
#define POKEBANK_GEN5_HUB_ROUTING_H

#include <cstdint>

namespace PokeVault::Integration::Gen5 {

// Gen V opens on the SAME loaded-game hub as Gen I-IV (Pokemon, Party,
// Storage, Items, Trainer). The base hub owns cursor movement, focus memory
// and help; this pure model only decides what Gen V must intercept so that no
// inherited Bank/source writer is reachable from a Gen V session.
// Tile order and touch ids mirror TrainerViewScreen's home menu (100..104).
enum class HubTile5 : uint8_t { Pokemon = 0, Party = 1, Storage = 2, Items = 3, Trainer = 4 };
inline constexpr int kHubTileCount5 = 5;
inline constexpr int kHubTouchBase5 = 100;

enum class HubCommand5 : uint8_t {
    PassToBase,      // D-pad/stick focus, Minus help: base hub handles it unchanged
    Enter,           // open the tile's panel (Gen V-owned Party/Boxes/Items/Trainer)
    StorageRefused,  // Legacy Storage (Bank) has no PK5 conversion: explicit status
    Exit,            // B on the hub: leave via the Gen V staged-changes confirmation
    Review,          // + on the hub: shared Review of staged changes
};
struct HubDecision5 {
    HubCommand5 command = HubCommand5::PassToBase;
    int tile = 0;
};

[[nodiscard]] constexpr bool validHubTile5(int tile) noexcept {
    return tile >= 0 && tile < kHubTileCount5;
}

// focusedTile: the base hub's homeMenuIndex. tapId: release-confirmed touch id
// (or -1). A completed tap on a tile focuses it and activates it, exactly as A.
[[nodiscard]] constexpr HubDecision5 routeHub5(int focusedTile, bool a, bool b, bool plus,
                                               int tapId) noexcept {
    int tile = validHubTile5(focusedTile) ? focusedTile : 0;
    bool activate = a;
    if (tapId >= kHubTouchBase5 && tapId < kHubTouchBase5 + kHubTileCount5) {
        tile = tapId - kHubTouchBase5;
        activate = true;
    }
    if (b && !activate) return {HubCommand5::Exit, tile};
    if (plus && !activate) return {HubCommand5::Review, tile};
    if (!activate) return {HubCommand5::PassToBase, tile};
    if (tile == static_cast<int>(HubTile5::Storage)) return {HubCommand5::StorageRefused, tile};
    return {HubCommand5::Enter, tile};
}

// B inside an entered panel (with no Gen V modal open) returns to the hub with
// the tile still focused. The Product Home Items shortcut has no hub behind
// it, so its B leaves the session through the staged-changes confirmation.
enum class PanelBack5 : uint8_t { ToHub, ExitSession };
[[nodiscard]] constexpr PanelBack5 panelBack5(bool itemsShortcutActive) noexcept {
    return itemsShortcutActive ? PanelBack5::ExitSession : PanelBack5::ToHub;
}

} // namespace PokeVault::Integration::Gen5
#endif
