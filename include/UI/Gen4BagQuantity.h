#pragma once

// The ONE per-item quantity query of the Gen IV bag, shared by the Gen IV surface
// (Gen4SharedPokemonSurface.inc: A / Add) and the Items footer in TrainerViewScreenBase.inc.
// Backed by Thread I's source-backed Inventory/ItemQuantityPolicy.h (d08c5aa8) and never above the
// native writer's own bound (Gen4BagCatalog gen4BagMaxQuantity), so the UI can never offer a
// value the staged editor would refuse. Unique items (Key Items, HMs) get no quantity editor.

#include "Integration/Gen4/Gen4BagCatalog.h"
#include "Inventory/ItemQuantityPolicy.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace PokeBank::UIModel::Gen4BagQuantity {

struct Rule { bool valid = false; bool editable = false; uint16_t maximum = 0; };

// The exact-game policy rule for one Gen IV pocket index (Gen4ReadOnlyInventory.h BagPocket order).
inline PokeVault::Inventory::QuantityPolicy::ItemQuantityRule policyRule(
    PokeVault::Inventory::QuantityPolicy::Game game, std::size_t pocket, uint16_t id) noexcept {
    namespace QP = PokeVault::Inventory::QuantityPolicy;
    namespace Gen4 = PokeVault::Integration::Gen4;
    static constexpr std::array<QP::Pocket, Gen4::BagPocketCount> kPockets{{
        QP::Pocket::Items, QP::Pocket::KeyItems, QP::Pocket::TMHM, QP::Pocket::Mail,
        QP::Pocket::Medicine, QP::Pocket::Berries, QP::Pocket::Balls, QP::Pocket::BattleItems}};
    if (pocket >= kPockets.size() || QP::generation(game) != 4) return {};
    return QP::quantityRule(game, kPockets[pocket], id);
}

inline Rule rule(PokeVault::Inventory::QuantityPolicy::Game game, std::size_t pocket,
                 uint16_t id) noexcept {
    namespace Gen4 = PokeVault::Integration::Gen4;
    const auto r = policyRule(game, pocket, id);
    const uint16_t nativeCap = pocket < Gen4::BagPocketCount ? Gen4::gen4BagMaxQuantity(pocket, id) : 0;
    if (!r.valid || nativeCap == 0) return {};
    const uint16_t maximum = std::min(r.maximum, nativeCap);
    return {true, r.editable && maximum > 1, maximum};
}

inline Rule rule(std::string_view sourceGameId, std::size_t pocket, uint16_t id) noexcept {
    PokeVault::Inventory::QuantityPolicy::Game game{};
    if (!PokeVault::Inventory::QuantityPolicy::gameFromId(sourceGameId, game)) return {};
    return rule(game, pocket, id);
}

} // namespace PokeBank::UIModel::Gen4BagQuantity
