#ifndef POKEBANK_INTEGRATION_GEN3_GEN3_STAGED_POKEMON_EDITOR_H
#define POKEBANK_INTEGRATION_GEN3_GEN3_STAGED_POKEMON_EDITOR_H

#include "Integration/Gen3/PKSMGen3Adapter.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace PokeVault::Integration::Gen3 {

struct StagedPokemonRecord {
    uint16_t species = 0;
    std::string nickname;
    uint8_t level = 1;
    uint32_t experience = 0;
    uint16_t heldItem = 0;
    std::array<uint16_t, 4> moves{};
    std::array<uint8_t, 4> pp{};
    std::array<uint8_t, 4> ppUps{};
    std::array<uint8_t, 6> ivs{};
    std::array<uint8_t, 6> evs{};
    std::array<uint16_t, 6> calculatedStats{};
    std::string otName;
    uint16_t tid = 0;
    uint16_t sid = 0;
    uint32_t pid = 0;
    uint8_t language = 0;
    uint8_t friendship = 0;
    uint8_t pokerus = 0;
    uint8_t originGame = 0;
    uint8_t ball = 0;
    uint8_t metLevel = 0;
    uint16_t metLocation = 0;
    uint8_t nature = 0;
    uint8_t gender = 0;
    bool shiny = false;
    uint16_t ability = 0;
    uint8_t abilityNumber = 0;
    uint8_t form = 0;
    std::array<uint8_t, 80> encryptedBytes{};
};

struct BoxPokemonEdit {
    std::optional<uint16_t> species;
    std::optional<std::string> nickname;
    std::optional<uint8_t> level;
    std::optional<uint32_t> experience;
    std::optional<uint16_t> heldItem;
    std::optional<std::array<uint8_t, 4>> pp;
    std::optional<std::array<uint8_t, 4>> ppUps;
    std::optional<std::array<uint8_t, 6>> ivs;
    std::optional<std::array<uint8_t, 6>> evs;
    std::optional<std::string> otName;
    std::optional<uint8_t> language;
    std::optional<uint8_t> friendship;
    std::optional<uint8_t> pokerus;
    std::optional<uint8_t> ball;
    std::optional<uint8_t> metLevel;
    std::optional<uint16_t> metLocation;
};

struct BoxPokemonCreate {
    uint16_t species = 25;
    uint8_t level = 5;
    std::string nickname;
    uint16_t heldItem = 0;
    uint8_t language = 2; // English in PK3
    uint8_t friendship = 70;
    uint8_t ball = 4;     // Poke Ball in PK3
    uint16_t metLocation = 0;
};

struct StagedPokemonChange {
    uint8_t box = 0;
    uint8_t slot = 0;
    std::string label;
    std::string beforeValue;
    std::string afterValue;
};

class StagedPokemonEditor {
public:
    static std::unique_ptr<StagedPokemonEditor> create(
        std::span<const uint8_t> source, SourceGame game, std::string& error);

    [[nodiscard]] SourceGame sourceGame() const noexcept { return sourceGame_; }
    [[nodiscard]] uint8_t activeSlot() const noexcept { return activeSlot_; }
    [[nodiscard]] uint32_t saveCounter() const noexcept { return saveCounter_; }
    [[nodiscard]] uint8_t boxCount() const noexcept { return 14; }
    [[nodiscard]] uint8_t boxCapacity() const noexcept { return 30; }
    [[nodiscard]] const std::vector<uint8_t>& originalBytes() const noexcept { return original_; }
    [[nodiscard]] const std::vector<uint8_t>& stagedBytes() const noexcept { return staged_; }
    [[nodiscard]] const std::vector<StagedPokemonChange>& pendingChanges() const noexcept {
        return changes_;
    }
    [[nodiscard]] bool hasPendingChanges() const noexcept { return !changes_.empty(); }

    [[nodiscard]] std::optional<StagedPokemonRecord> boxedPokemon(
        std::size_t box, std::size_t slot, std::string& error) const;

    bool stageBoxPokemonEdit(std::size_t box, std::size_t slot,
                             const BoxPokemonEdit& edit, std::string& error);
    bool stageAddBoxPokemon(std::size_t box, std::size_t slot,
                            const BoxPokemonCreate& create, std::string& error);
    bool stageCloneBoxPokemon(std::size_t sourceBox, std::size_t sourceSlot,
                              std::size_t destinationBox, std::size_t destinationSlot,
                              std::string& error);
    bool stageReleaseBoxPokemon(std::size_t box, std::size_t slot, std::string& error);

    void discard() noexcept;
    [[nodiscard]] std::vector<uint8_t> finalizedBytes(std::string& error) const;

    // PID-correlated fields remain read-only in the first Gen III device candidate.
    static constexpr bool pidCorrelatedEditingEnabled() noexcept { return false; }
    static constexpr bool moveSelectionEditingEnabled() noexcept { return false; }

private:
    StagedPokemonEditor(std::span<const uint8_t> source, SourceGame game, uint8_t activeSlot,
                        uint32_t saveCounter, const std::array<std::size_t, 14>& offsets,
                        TrainerRecord trainer);

    bool validateStaged(std::string& error) const;
    bool writeBoxRaw(std::size_t box, std::size_t slot, std::span<const uint8_t> raw,
                     std::string& error);
    [[nodiscard]] std::array<uint8_t, 80> readBoxRaw(
        std::size_t box, std::size_t slot, bool& ok) const noexcept;

    SourceGame sourceGame_;
    uint8_t activeSlot_;
    uint32_t saveCounter_;
    std::array<std::size_t, 14> sectorOffsets_{};
    TrainerRecord trainer_;
    std::vector<uint8_t> original_;
    std::vector<uint8_t> staged_;
    std::vector<StagedPokemonChange> changes_;
};

} // namespace PokeVault::Integration::Gen3

#endif
