#ifndef POKEBANK_INTEGRATION_GEN4_STAGED_POKEMON_EDITOR_H
#define POKEBANK_INTEGRATION_GEN4_STAGED_POKEMON_EDITOR_H

#include "Integration/Gen4/Gen4ReadOnlySave.h"
#include "Pokemon/Pokemon4Mutable.h"
#include "Pokemon/Pokemon4ReadOnly.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace PokeVault::Integration::Gen4 {

class Gen4StagedPokemonEditor {
public:
    static std::optional<Gen4StagedPokemonEditor> create(
        std::span<const uint8_t> source,
        Layout layout,
        std::string_view exactGameId,
        std::string* error = nullptr);

    [[nodiscard]] const std::vector<uint8_t>& originalBytes() const noexcept {
        return original_;
    }
    [[nodiscard]] const std::vector<uint8_t>& stagedBytes() const noexcept {
        return staged_;
    }
    [[nodiscard]] Layout layout() const noexcept { return layout_; }
    [[nodiscard]] Enums::GameVersion sourceGroup() const noexcept { return sourceGroup_; }
    [[nodiscard]] std::string_view exactGameId() const noexcept { return exactGameId_; }
    [[nodiscard]] bool hasChanges() const noexcept { return staged_ != original_; }

    [[nodiscard]] std::optional<Pokemon::Pokemon4ReadOnly> boxedPokemon(
        size_t box, size_t slot, std::string* error = nullptr) const;

    [[nodiscard]] std::optional<Pokemon::Pokemon4Mutable> editableBoxPokemon(
        size_t box, size_t slot, std::string* error = nullptr) const;

    bool commitBoxPokemon(size_t box, size_t slot,
                          const Pokemon::Pokemon4Mutable& pokemon,
                          std::string* error = nullptr);

    void discard() noexcept { staged_ = original_; }

    [[nodiscard]] std::vector<uint8_t> finalizedBytes(std::string* error = nullptr) const;

private:
    Gen4StagedPokemonEditor(std::vector<uint8_t> source, Layout layout,
                            std::string exactGameId,
                            Enums::GameVersion sourceGroup) noexcept;

    [[nodiscard]] std::optional<size_t> boxRecordOffset(
        const Gen4ReadOnlySave& parsed, size_t box, size_t slot) const noexcept;
    bool reparse(Gen4ReadOnlySave& out, std::string* error = nullptr) const;
    bool refreshStorageCrc(const Gen4ReadOnlySave& parsed, std::string* error = nullptr);

    std::vector<uint8_t> original_;
    std::vector<uint8_t> staged_;
    Layout layout_ = Layout::DiamondPearl;
    std::string exactGameId_;
    Enums::GameVersion sourceGroup_ = Enums::GameVersion::Invalid;
};

} // namespace PokeVault::Integration::Gen4

#endif
