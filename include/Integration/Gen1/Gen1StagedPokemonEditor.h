#ifndef POKEBANK_GEN1_STAGED_POKEMON_EDITOR_H
#define POKEBANK_GEN1_STAGED_POKEMON_EDITOR_H

#include "Integration/Gen1/Gen1ReadOnlySave.h"
#include <span>
#include <string_view>
#include <vector>

namespace PokeBank::PokemonFile { struct Decoded; }

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
// Create default is #25 Pikachu at L5, the same default as the Gen II-V editors. Its moves are
// Pikachu's pret/pokered base moves (data/pokemon/base_stats/pikachu.asm: THUNDERSHOCK, GROWL)
// at their base PP (data/moves/moves.asm: Thunder Shock 30, Growl 40).
struct BoxPokemonCreate {
    uint16_t species = 25;
    uint8_t level = 5;
    std::optional<uint32_t> experience;
    std::string nickname, otName;
    std::optional<uint16_t> trainerId;
    std::array<uint8_t,4> moves{84,45,0,0}, pp{30,40,0,0}, ppUps{}, dvs{};
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

// Owns native party/boxed mutations in memory. No filesystem or source-write API.
class StagedPokemonEditor final {
public:
    static std::unique_ptr<StagedPokemonEditor> create(const ReadOnlySave&, std::string& error);
    const Metadata& metadata() const noexcept { return original_->metadata(); }
    const ReadOnlySave& view() const noexcept { return *view_; }
    std::span<const uint8_t> originalBytes() const noexcept { return original_->sourceBytes(); }
    std::span<const uint8_t> stagedBytes() const noexcept { return view_->sourceBytes(); }
    std::optional<PokemonRecord> boxedPokemon(size_t box, size_t slot, std::string& error) const;
    std::optional<PokemonRecord> partyPokemon(size_t slot, std::string& error) const;
    bool stageEdit(size_t box, size_t slot, const BoxPokemonEdit&, std::string& error);
    bool stagePartyPokemonEdit(size_t slot, const BoxPokemonEdit&, std::string& error);
    // Resolve only this box: visual empty cells are not persistent sparse positions.
    std::optional<size_t> appendSlot(size_t box, std::string& error) const;
    // Explicit destination must be the first empty slot; boxes are contiguous lists.
    bool stageAdd(size_t box, size_t slot, const BoxPokemonCreate&, std::string& error);
    bool stageClone(size_t sourceBox, size_t sourceSlot, size_t box, size_t slot, std::string& error);
    bool stageRemove(size_t box, size_t slot, std::string& error);

    // Feature A (owner Oct 28): standalone .pk1 files (PokemonFile/NativePokemonFile.h).
    // stageImport places one decoded international .pk1 into the first empty slot of a box, as a
    // staged change only (Review / Keep / Discard / Export Copy as for Create). The box keeps the
    // first 33 bytes of the 44-byte party body (pret/pokered box_struct) plus the file's 11-byte
    // OT and nickname as stored. When the party level (body 0x21) is non-zero it becomes the box
    // level byte (body 0x03), as pokered engine/pokemon/add_mon.asm _MoveMon does on PARTY_TO_BOX;
    // a zero party block (aa4b72e) keeps body 0x03. The record must then pass this editor's own
    // validators. Japanese files, other generations, occupied or non-first-empty slots and full
    // boxes are refused; nothing is converted, invented or legalized.
    bool stageImport(size_t box, size_t slot, const PokeBank::PokemonFile::Decoded& file,
                     std::string& error);
    // PKHeX single-entry .pk1 list (69 bytes, PokeCrypto.cs SIZE_1LIST) of a staged box or Party
    // Pokemon. Reads only. A box record gets the party-only fields the game computes when it leaves
    // the PC (pokered _MoveMon BOX_TO_PARTY: CalcLevelFromExperience, then CalcStats with Stat Exp);
    // current HP and status stay as stored. A Party record is copied as stored.
    std::optional<std::vector<uint8_t>> exportBoxPokemonFile(size_t box, size_t slot,
                                                             std::string& error) const;
    std::optional<std::vector<uint8_t>> exportPartyPokemonFile(size_t slot, std::string& error) const;

    // Packed relocation engine used by the native classic box Y interaction. The single-Pokemon
    // entry points remain the Part 1 contract; Part 2 extends the same engine to an ordered group.
    bool beginPackedMove(size_t sourceBox, size_t sourceSlot, std::string& error);
    bool placePackedMove(size_t destinationBox, size_t destinationSlot,
                         size_t& placedSlot, std::string& error);
    bool beginPackedGroupMove(size_t sourceBox, std::span<const size_t> sourceSlots,
                              std::string& error);
    bool placePackedGroupMove(size_t destinationBox, size_t destinationSlot,
                              size_t& firstPlacedSlot, std::string& error);
    bool cancelPackedMove(std::string& error);
    bool packedMoveActive() const noexcept { return packedMove_.active; }
    size_t packedMoveCount() const noexcept { return packedMove_.sourceSlots.size(); }

    bool revertPokemon(size_t box, size_t slot, std::string& error);
    void discard() noexcept;
    bool hasPendingChanges() const noexcept { return !changes_.empty(); }
    const std::vector<PokemonChange>& pendingChanges() const noexcept { return changes_; }
    // Optional inventory candidate must have this exact baseline outside Bag/PC/checksum; an
    // optional trainer candidate (Gen1StagedTrainerEditor) outside player name/money/checksum.
    std::vector<uint8_t> finalizedBytes(std::string& error,
                                      std::span<const uint8_t> inventoryCandidate = {},
                                      std::span<const uint8_t> trainerCandidate = {}) const;
    static uint8_t derivedHPDV(const std::array<uint8_t,4>& dvs) noexcept;
    static uint8_t moveBasePP(uint16_t move) noexcept;
    static uint8_t moveMaxPP(uint16_t move, uint8_t ups) noexcept;
    static uint8_t growthRate(uint16_t species) noexcept;
    // Read-only personal-data lookup for presentation. The returned values are the exact
    // Generation I type IDs stored in the same frozen personal-data table used by stat math.
    static std::array<uint8_t,2> personalTypes(uint16_t species) noexcept;
    // Exact Generation I battle-stat preview. Uses the original five-stat model
    // HP / Attack / Defense / Speed / Special, four stored DVs plus derived HP DV,
    // and Stat Exp's ceil(sqrt(value))/4 contribution. This is display math only.
    static BattleStats calculateBattleStats(uint16_t species, uint8_t level,
                                            const std::array<uint8_t,4>& dvs,
                                            const std::array<uint16_t,5>& statExperience) noexcept;
    static bool encodeName(std::string_view, size_t maximum, std::array<uint8_t,11>&,
                           std::string& error);
private:
    struct PackedMoveState {
        bool active = false;
        size_t sourceBox = 0;
        std::vector<size_t> sourceSlots;
        std::vector<uint8_t> beforeBytes;
    };

    explicit StagedPokemonEditor(std::shared_ptr<const ReadOnlySave> source);
    bool destination(size_t box, size_t slot, std::string& error) const;
    bool commit(std::vector<uint8_t> candidate, std::string& error);
    bool stageRecordEdit(bool party, size_t box, size_t slot, const BoxPokemonEdit&, std::string& error);
    void rebuildChanges();
    std::shared_ptr<const ReadOnlySave> original_, view_;
    std::vector<PokemonChange> changes_;
    PackedMoveState packedMove_;
};
} // namespace PokeVault::Integration::Gen1
#endif
