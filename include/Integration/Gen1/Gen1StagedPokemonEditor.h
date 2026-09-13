#ifndef POKEBANK_GEN1_STAGED_POKEMON_EDITOR_H
#define POKEBANK_GEN1_STAGED_POKEMON_EDITOR_H

#include "Integration/Gen1/Gen1ReadOnlySave.h"
#include <string_view>

namespace PokeVault::Integration::Gen1 {
struct PokemonChange {
    std::string key, label, beforeValue, afterValue;
};
struct BoxPokemonEdit {
    std::optional<uint16_t> species;
    std::optional<std::string> nickname, otName;
    std::optional<uint8_t> level;
    std::optional<uint32_t> experience;
    std::optional<uint16_t> trainerId;
    std::optional<std::array<uint8_t,4>> moves, pp, ppUps, dvs;
    std::optional<std::array<uint16_t,5>> statExperience;
};
struct BoxPokemonCreate {
    uint16_t species = 1;
    uint8_t level = 5;
    std::string nickname, otName;
    std::optional<uint16_t> trainerId;
    std::array<uint8_t,4> moves{33,0,0,0}, pp{35,0,0,0}, ppUps{}, dvs{8,8,8,8};
    std::array<uint16_t,5> statExperience{};
};
struct BattleStats {
    uint16_t hp = 0;
    uint16_t attack = 0;
    uint16_t defense = 0;
    uint16_t speed = 0;
    uint16_t special = 0;

    constexpr std::array<uint16_t,5> asArray() const noexcept {
        return {hp, attack, defense, speed, special};
    }
};

// Owns boxed-Pokemon mutations only. No filesystem or source-write API.
class StagedPokemonEditor final {
public:
    static std::unique_ptr<StagedPokemonEditor> create(const ReadOnlySave&, std::string& error);
    const Metadata& metadata() const noexcept { return original_->metadata(); }
    const ReadOnlySave& view() const noexcept { return *view_; }
    std::span<const uint8_t> originalBytes() const noexcept { return original_->sourceBytes(); }
    std::span<const uint8_t> stagedBytes() const noexcept { return view_->sourceBytes(); }
    std::optional<PokemonRecord> boxedPokemon(size_t box, size_t slot, std::string& error) const;
    bool stageEdit(size_t box, size_t slot, const BoxPokemonEdit&, std::string& error);
    // Explicit destination must be the first empty slot; boxes are contiguous lists.
    bool stageAdd(size_t box, size_t slot, const BoxPokemonCreate&, std::string& error);
    bool stageClone(size_t sourceBox, size_t sourceSlot, size_t box, size_t slot, std::string& error);
    bool stageRemove(size_t box, size_t slot, std::string& error);
    bool revertPokemon(size_t box, size_t slot, std::string& error);
    void discard() noexcept;
    bool hasPendingChanges() const noexcept { return !changes_.empty(); }
    const std::vector<PokemonChange>& pendingChanges() const noexcept { return changes_; }
    // Optional inventory candidate must have this exact baseline outside Bag/PC/checksum.
    std::vector<uint8_t> finalizedBytes(std::string& error,
                                      std::span<const uint8_t> inventoryCandidate = {}) const;
    static uint8_t derivedHPDV(const std::array<uint8_t,4>& dvs) noexcept;
    static uint8_t moveBasePP(uint16_t move) noexcept;
    static uint8_t moveMaxPP(uint16_t move, uint8_t ups) noexcept;
    static uint8_t growthRate(uint16_t species) noexcept;
    // Exact Generation I battle-stat preview. Uses the original five-stat model
    // HP / Attack / Defense / Speed / Special, four stored DVs plus derived HP DV,
    // and Stat Exp's ceil(sqrt(value))/4 contribution. This is display math only.
    static BattleStats calculateBattleStats(uint16_t species, uint8_t level,
                                            const std::array<uint8_t,4>& dvs,
                                            const std::array<uint16_t,5>& statExperience) noexcept;
    static bool encodeName(std::string_view, size_t maximum, std::array<uint8_t,11>&,
                           std::string& error);
private:
    explicit StagedPokemonEditor(std::shared_ptr<const ReadOnlySave> source);
    bool destination(size_t box, size_t slot, std::string& error) const;
    bool commit(std::vector<uint8_t> candidate, std::string& error);
    void rebuildChanges();
    std::shared_ptr<const ReadOnlySave> original_, view_;
    std::vector<PokemonChange> changes_;
};
} // namespace PokeVault::Integration::Gen1
#endif
