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

    [[nodiscard]] std::optional<Pokemon::Pokemon4Mutable> createBoxDraft(
        size_t box, size_t slot, uint16_t species,
        std::string* error = nullptr) const;

    bool commitBoxPokemon(size_t box, size_t slot,
                          const Pokemon::Pokemon4Mutable& pokemon,
                          std::string* error = nullptr);

    // Create/Add transaction primitive. Unlike commitBoxPokemon(), this refuses to
    // overwrite an occupied slot. The candidate must already be a valid native
    // 0x88 stored PK4; UI draft construction stays separate from save mutation.
    bool stageCreateBoxPokemon(size_t box, size_t slot,
                               const Pokemon::Pokemon4Mutable& pokemon,
                               std::string* error = nullptr);

    // Box-only convenience actions. Both mutate only the app-owned staged image,
    // refresh the Storage CRC, strictly reparse, and roll back atomically on failure.
    bool stageCloneBoxPokemon(size_t sourceBox, size_t sourceSlot,
                              size_t destinationBox, size_t destinationSlot,
                              std::string* error = nullptr);
    bool stageReleaseBoxPokemon(size_t box, size_t slot,
                                std::string* error = nullptr);

    [[nodiscard]] std::optional<Pokemon::Pokemon4ReadOnly> partyPokemon(
        size_t slot, std::string* error = nullptr) const;

    [[nodiscard]] std::optional<Pokemon::Pokemon4Mutable> editablePartyPokemon(
        size_t slot, std::string* error = nullptr) const;

    bool commitPartyPokemon(size_t slot,
                            const Pokemon::Pokemon4Mutable& pokemon,
                            std::string* error = nullptr);

    // Change an already-present native bag stack's quantity ONLY in the
    // app-owned staged save. No Add/Remove, item-ID change or source write.
    // `visibleIndex` is the zero-based populated row within its pouch.
    bool stageBagQuantity(size_t pocket,size_t visibleIndex,uint16_t quantity,
                          std::string* error=nullptr);

    // Destructive within the APP-OWNED staged workspace only. Explicit UI
    // confirmation is required; native identity/count are rechecked and
    // the external emulator SAV4 remains byte-for-byte immutable.
    bool stageBagRemove(size_t pocket,size_t visibleIndex,
                        std::string* error=nullptr);

    void discard() noexcept { staged_ = original_; }

    [[nodiscard]] std::vector<uint8_t> finalizedBytes(std::string* error = nullptr) const;

private:
    Gen4StagedPokemonEditor(std::vector<uint8_t> source, Layout layout,
                            std::string exactGameId,
                            Enums::GameVersion sourceGroup) noexcept;

    [[nodiscard]] std::optional<size_t> boxRecordOffset(
        const Gen4ReadOnlySave& parsed, size_t box, size_t slot) const noexcept;
    [[nodiscard]] std::optional<size_t> partyRecordOffset(
        const Gen4ReadOnlySave& parsed, size_t slot) const noexcept;
    [[nodiscard]] std::optional<Gen4ReadOnlySave> reparse(
        std::string* error = nullptr) const;
    bool refreshStorageCrc(const Gen4ReadOnlySave& parsed, std::string* error = nullptr);
    bool refreshGeneralCrc(const Gen4ReadOnlySave& parsed, std::string* error = nullptr);

    std::vector<uint8_t> original_;
    std::vector<uint8_t> staged_;
    Layout layout_ = Layout::DiamondPearl;
    std::string exactGameId_;
    Enums::GameVersion sourceGroup_ = Enums::GameVersion::Invalid;
};

} // namespace PokeVault::Integration::Gen4

#endif
