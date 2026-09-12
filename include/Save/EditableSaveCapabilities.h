#ifndef POKEVAULT_SAVE_EDITABLE_SAVE_CAPABILITIES_H
#define POKEVAULT_SAVE_EDITABLE_SAVE_CAPABILITIES_H

#include <cstdint>

namespace PokeVault::SaveEdit {

enum class Capability : uint32_t {
    TrainerIdentity   = 1u << 0,
    Money             = 1u << 1,
    Inventory         = 1u << 2,
    Balls             = 1u << 3,
    Medicine          = 1u << 4,
    BattleItems       = 1u << 5,
    TMsHMs            = 1u << 6,
    Berries           = 1u << 7,
    KeyItems          = 1u << 8,
    Badges            = 1u << 9,
    PlayerPosition    = 1u << 10,
    PartyPokemon      = 1u << 11,
    BoxPokemon        = 1u << 12,
    PCItems           = 1u << 13,
    PokemonEditing    = 1u << 14,
    PokemonCreation   = 1u << 15,
    PokemonClone      = 1u << 16,
    PokemonShinyToggle= 1u << 17,
};

class Capabilities final {
public:
    constexpr Capabilities() = default;
    constexpr explicit Capabilities(uint32_t bits) : bits_(bits) {}

    constexpr bool supports(Capability capability) const noexcept {
        return (bits_ & static_cast<uint32_t>(capability)) != 0;
    }
    constexpr uint32_t bits() const noexcept { return bits_; }

    constexpr Capabilities& add(Capability capability) noexcept {
        bits_ |= static_cast<uint32_t>(capability);
        return *this;
    }

private:
    uint32_t bits_ = 0;
};

} // namespace PokeVault::SaveEdit

#endif
