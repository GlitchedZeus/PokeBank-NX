#ifndef POKEBANK_INTEGRATION_GEN4_STAGED_POKEMON_EDITOR_H
#define POKEBANK_INTEGRATION_GEN4_STAGED_POKEMON_EDITOR_H

#include "Integration/Gen4/Gen4ReadOnlySave.h"
#include "Pokemon/Pokemon4Mutable.h"
#include "Pokemon/Pokemon4ReadOnly.h"
#include "PokemonFile/NativePokemonFile.h"

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

    // Atomic move into an empty slot or swap with another occupied slot.
    // Copies exactly two native encrypted 0x88 PK4 slots in APP-OWNED staged
    // bytes; updates Storage CRC; strictly reparses/validates both slots.
    bool stageMoveBoxPokemon(size_t sourceBox, size_t sourceSlot,
                             size_t destinationBox, size_t destinationSlot,
                             std::string* error = nullptr);

    // Feature A (owner Oct 28): standalone Pokemon files. Import places ONE decoded .pk4 / .ek4
    // record (PokeBank::PokemonFile::decode, PKHeX plaintext PK4) into an EMPTY box slot of the
    // APP-OWNED staged image only; it then shows up in Review like any staged change and Discard
    // undoes it. A party-sized (236-byte) file contributes its stored 136 bytes, as PKHeX writes a box
    // slot (PKM.EncryptedBoxData; the party battle-stat cache is not part of a box record).
    // Every stored byte is kept as given: no PID/IV reroll, no checksum repair, no legalization,
    // no Gen III/V conversion. The record must be one the editor's own Gen IV validators accept
    // for the open game group (species/form in that game's personal table, holdable item, native
    // move range, language). Same Storage CRC refresh, strict reparse and atomic rollback as Clone.
    [[nodiscard]] bool canImportPokemonFile(const PokeBank::PokemonFile::Decoded& file,
                                            std::string* error = nullptr) const;
    bool stageImportBoxPokemon(size_t box, size_t slot,
                               const PokeBank::PokemonFile::Decoded& file,
                               std::string* error = nullptr);

    // Non-mutating export of a Pokemon in the open (staged) save as the PKHeX plaintext PK4 a
    // .pk4 file holds: decrypted, blocks in canonical ABCD order, checksum at 0x06 (PKHeX
    // PK4.DecryptedBoxData / DecryptedPartyData). Box records are 136 bytes, party records 236,
    // exactly as stored. PokeBank::PokemonFile::decode() returns the same record.
    [[nodiscard]] std::optional<std::vector<uint8_t>> exportBoxPokemonFile(
        size_t box, size_t slot, std::string* error = nullptr) const;
    [[nodiscard]] std::optional<std::vector<uint8_t>> exportPartyPokemonFile(
        size_t slot, std::string* error = nullptr) const;

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

    // Add one strictly catalog-allowed Gen IV item to an empty native slot
    // in the app-owned image, never an external save. No arbitrary ID
    // reassignment, overwrite of existing stacks, or native writes.
    bool stageBagAdd(size_t pocket,uint16_t itemId,uint16_t quantity,
                     std::string* error=nullptr);

    // R2-BUG-002: the Trainer page's Name / Money, staged in the SAME app-owned image as every
    // other Gen IV edit (selected General block only; its CRC refreshed; the other General copy
    // and the source are never touched). Trainer struct offsets as read by Gen4ReadOnlySave
    // (PKHeX SAV4DP / SAV4Pt / SAV4HGSS Trainer1): name +0x00 (8 u16, max 7 characters,
    // ClearZero), money +0x14 (u32, PKHeX MaxMoney 999999).
    static constexpr uint32_t kMaxMoney = 999999;
    static constexpr size_t kTrainerNameMax = 7;
    [[nodiscard]] std::optional<TrainerReadOnly> stagedTrainer(std::string* error = nullptr) const;
    // Name editing is offered only for the international languages whose 7-character keyboard
    // the field matches; Japanese / Korean names stay read-only (money is still editable).
    [[nodiscard]] bool trainerNameEditable() const;
    bool stageTrainerName(const std::u16string& name, std::string* error = nullptr);
    bool stageTrainerMoney(uint32_t money, std::string* error = nullptr);
    // P3 (Round 5): Trainer Gender +0x18 (u8 0 / 1) and Play Time +0x22 (u16 hours, u8 minutes,
    // u8 seconds), the offsets pinned PKSM-Core aa22d7a Sav4::gender / playedHours /
    // playedMinutes / playedSeconds read and write (vendor/PKSM-Core/source/sav/Sav4.cpp). Same
    // staged image, General CRC refresh and strict reparse as Name / Money. Gender is offered only
    // when the stored byte is a native 0 / 1. Hours are capped at 999 (the shared Gen V editor bound).
    static constexpr uint16_t kMaxPlayedHours = 999;
    [[nodiscard]] bool trainerGenderEditable() const;
    bool stageTrainerGender(uint8_t gender, std::string* error = nullptr);
    bool stageTrainerPlayTime(uint16_t hours, uint8_t minutes, uint8_t seconds,
                              std::string* error = nullptr);

    void discard() noexcept { staged_ = original_; }

    [[nodiscard]] std::vector<uint8_t> finalizedBytes(std::string* error = nullptr) const;

private:
    Gen4StagedPokemonEditor(std::vector<uint8_t> source, Layout layout,
                            std::string exactGameId,
                            Enums::GameVersion sourceGroup) noexcept;

    [[nodiscard]] std::optional<size_t> boxRecordOffset(
        const Gen4ReadOnlySave& parsed, size_t box, size_t slot) const noexcept;
    // True when the source held a valid empty record at this box slot, at the same offset in the
    // same selected Storage copy as the staged parse.
    [[nodiscard]] bool originalEmptyRecordAt(const Gen4ReadOnlySave& stagedParse, size_t box,
                                             size_t slot, size_t offset) const;
    [[nodiscard]] std::optional<size_t> partyRecordOffset(
        const Gen4ReadOnlySave& parsed, size_t slot) const noexcept;
    [[nodiscard]] std::optional<Gen4ReadOnlySave> reparse(
        std::string* error = nullptr) const;
    bool refreshStorageCrc(const Gen4ReadOnlySave& parsed, std::string* error = nullptr);
    bool refreshGeneralCrc(const Gen4ReadOnlySave& parsed, std::string* error = nullptr);
    [[nodiscard]] std::optional<size_t> trainerRecordOffset(const Gen4ReadOnlySave& parsed) const noexcept;
    // Writes `bytes` at the selected General block's trainer struct + `field`, refreshes the
    // General CRC, strictly reparses and checks `verify`; restores the previous image on failure.
    template <class Verify>
    bool stageTrainerBytes(size_t field, std::span<const uint8_t> bytes, Verify&& verify,
                           std::string* error);

    std::vector<uint8_t> original_;
    std::vector<uint8_t> staged_;
    Layout layout_ = Layout::DiamondPearl;
    std::string exactGameId_;
    Enums::GameVersion sourceGroup_ = Enums::GameVersion::Invalid;
};

} // namespace PokeVault::Integration::Gen4

#endif
