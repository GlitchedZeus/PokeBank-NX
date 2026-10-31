#ifndef POKEBANK_INTEGRATION_GEN3_GEN3_STAGED_POKEMON_EDITOR_H
#define POKEBANK_INTEGRATION_GEN3_GEN3_STAGED_POKEMON_EDITOR_H

#include "Integration/Gen3/PKSMGen3Adapter.h"
#include "PokemonFile/NativePokemonFile.h"

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
    std::optional<std::array<uint16_t, 4>> moves;
    std::optional<std::array<uint8_t, 4>> pp;
    std::optional<std::array<uint8_t, 4>> ppUps;
    std::optional<std::array<uint8_t, 6>> ivs;
    std::optional<std::array<uint8_t, 6>> evs;
    std::optional<std::string> otName;
    std::optional<uint16_t> tid;
    std::optional<uint8_t> language;
    std::optional<uint8_t> friendship;
    std::optional<uint8_t> pokerus;
    std::optional<uint8_t> ball;
    std::optional<uint8_t> metLevel;
    std::optional<uint16_t> metLocation;
    std::optional<uint8_t> nature;
    std::optional<uint8_t> gender;
    std::optional<bool> shiny;
    std::optional<uint8_t> abilityNumber;
    std::optional<uint8_t> form; // Unown letter 0..27 (PID-derived); refused for other species
};

struct BoxPokemonCreate {
    std::optional<std::string> otName;
    std::optional<uint16_t> tid;
    uint16_t species = 25;
    uint8_t level = 5;
    std::optional<uint32_t> experience;
    std::string nickname;
    uint16_t heldItem = 0;
    std::array<uint16_t, 4> moves{};
    std::array<uint8_t, 4> pp{};
    std::array<uint8_t, 4> ppUps{};
    std::array<uint8_t, 6> ivs{};
    std::array<uint8_t, 6> evs{};
    uint8_t language = 2; // English in PK3
    uint8_t friendship = 70;
    uint8_t pokerus = 0;
    uint8_t ball = 4;     // Poke Ball in PK3
    uint8_t metLevel = 5;
    uint16_t metLocation = 0;
    std::optional<uint8_t> nature;
    std::optional<uint8_t> gender;
    std::optional<bool> shiny;
    std::optional<uint8_t> abilityNumber;
    std::optional<uint8_t> form; // Unown letter 0..27 (PID-derived); refused for other species
};

struct StagedPokemonChange {
    uint8_t box = 0;
    uint8_t slot = 0;
    std::string label;
    std::string beforeValue;
    std::string afterValue;
    bool party = false;
    bool createdDetail = false;   // CG74: one value of a created Pokemon; there is no "before"
};

class StagedPokemonEditor {
public:
    static std::unique_ptr<StagedPokemonEditor> create(
        std::span<const uint8_t> source, SourceGame game, std::string& error);

    [[nodiscard]] SourceGame sourceGame() const noexcept { return sourceGame_; }
    [[nodiscard]] uint8_t originGame() const noexcept;
    [[nodiscard]] uint8_t activeSlot() const noexcept { return activeSlot_; }
    [[nodiscard]] uint32_t saveCounter() const noexcept { return saveCounter_; }
    [[nodiscard]] uint8_t boxCount() const noexcept { return 14; }
    [[nodiscard]] uint8_t boxCapacity() const noexcept { return 30; }
    [[nodiscard]] const TrainerRecord& trainer() const noexcept { return trainer_; }
    [[nodiscard]] const std::vector<uint8_t>& originalBytes() const noexcept { return original_; }
    [[nodiscard]] const std::vector<uint8_t>& stagedBytes() const noexcept { return staged_; }
    [[nodiscard]] const std::vector<StagedPokemonChange>& pendingChanges() const noexcept {
        return changes_;
    }
    [[nodiscard]] bool hasPendingChanges() const noexcept { return !changes_.empty(); }

    [[nodiscard]] std::optional<StagedPokemonRecord> boxedPokemon(
        std::size_t box, std::size_t slot, std::string& error) const;
    [[nodiscard]] std::optional<StagedPokemonRecord> partyPokemon(std::size_t slot,std::string& error) const;
    bool stagePartyPokemonEdit(std::size_t slot,const BoxPokemonEdit&,std::string& error);
    std::optional<StagedPokemonRecord> previewPartyEdit(std::size_t slot,const BoxPokemonEdit&,std::string& error) const;

    static std::optional<StagedPokemonRecord> decodeRecord(
        const std::array<uint8_t,80>& raw, std::string& error);
    std::optional<StagedPokemonRecord> previewEdit(std::size_t box, std::size_t slot,
        const BoxPokemonEdit& edit, std::string& error) const;
    std::optional<StagedPokemonRecord> previewCreate(std::size_t box, std::size_t slot,
        const BoxPokemonCreate& create, std::string& error) const;

    bool stageBoxPokemonEdit(std::size_t box, std::size_t slot,
                             const BoxPokemonEdit& edit, std::string& error);
    bool stageAddBoxPokemon(std::size_t box, std::size_t slot,
                            const BoxPokemonCreate& create, std::string& error);
    bool stageCloneBoxPokemon(std::size_t sourceBox, std::size_t sourceSlot,
                              std::size_t destinationBox, std::size_t destinationSlot,
                              std::string& error);
    bool stageReleaseBoxPokemon(std::size_t box, std::size_t slot, std::string& error);

    // Feature A (standalone files). Import places a decoded .pk3/.ek3 record (PokemonFile::decode)
    // into an EMPTY box slot of the staged workspace, byte for byte: a box slot holds the 80-byte
    // BoxPokemon, so a 100-byte party file contributes its first 80 bytes (pret pokeemerald /
    // pokefirered include/pokemon.h struct Pokemon { struct BoxPokemon box; ... }, and
    // include/pokemon_storage_system.h boxes[][] of struct BoxPokemon). Nothing is rerolled,
    // repaired or converted; a record the editor's own field validators reject is refused.
    bool stageImportBoxPokemon(std::size_t box, std::size_t slot,
                               const PokeBank::PokemonFile::Decoded& file, std::string& error);
    // Non-mutating export of the record as the open workspace holds it, in PKHeX plaintext PK3
    // layout (decrypted, substructures in canonical Growth/Attacks/EVs/Misc order): 80 bytes for a
    // box slot, 100 for a party slot with the party-only tail exactly as stored (Gen III never
    // encrypts it: PKHeX PK3.EncryptParty is a no-op).
    [[nodiscard]] std::optional<std::vector<uint8_t>> exportBoxPokemonFile(
        std::size_t box, std::size_t slot, std::string& error) const;
    [[nodiscard]] std::optional<std::vector<uint8_t>> exportPartyPokemonFile(
        std::size_t slot, std::string& error) const;

    // Sparse carry is transactional: exact slots only, no compaction or overwrite.
    bool beginSparseMove(std::size_t box, const std::vector<std::size_t>& slots, std::string& error);
    bool placeSparseMove(std::size_t box, std::size_t anchor, std::string& error);
    bool cancelSparseMove(std::string& error);
    bool carryingSparseMove() const noexcept { return !carrySlots_.empty(); }
    void discard() noexcept;
    [[nodiscard]] std::vector<uint8_t> finalizedBytes(std::string& error) const;

    // PID-correlated fields remain read-only in the first Gen III device candidate.
    static constexpr bool pidCorrelatedEditingEnabled() noexcept { return true; }
    static constexpr bool moveSelectionEditingEnabled() noexcept { return false; }

private:
    StagedPokemonEditor(std::span<const uint8_t> source, SourceGame game, uint8_t activeSlot,
                        uint32_t saveCounter, const std::array<std::size_t, 14>& offsets,
                        TrainerRecord trainer);

    bool validateStaged(std::string& error) const;
    std::vector<uint8_t> readPartyRaw(std::size_t slot,std::string& error) const;
    bool stageRecordEdit(bool party,std::size_t box,std::size_t slot,const BoxPokemonEdit&,std::string& error);
    bool writeBoxRaw(std::size_t box, std::size_t slot, std::span<const uint8_t> raw,
                     std::string& error);
    [[nodiscard]] std::array<uint8_t, 80> readBoxRaw(
        std::size_t box, std::size_t slot, bool& ok) const noexcept;
    // True when the slot holds a nonzero species index that boxedPokemon cannot read (an internal
    // index with no National Dex number): such bytes are not an empty slot and are never overwritten.
    [[nodiscard]] bool holdsUnreadableRecord(std::size_t box, std::size_t slot) const;

    std::vector<std::size_t> carrySlots_;
    std::vector<std::array<uint8_t, 80>> carryRecords_;
    std::vector<uint8_t> beforeCarry_;
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
