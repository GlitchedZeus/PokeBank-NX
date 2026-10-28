#ifndef POKEVAULT_GEN2_STAGED_EDITOR_H
#define POKEVAULT_GEN2_STAGED_EDITOR_H

#include "Integration/Gen2/Gen2ReadOnlyInventory.h"
#include "Integration/Gen2/Gen2ReadOnlySave.h"
#include "Save/EditableSaveCapabilities.h"

#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>
#include <utility>

namespace PokeBank::PokemonFile { struct Decoded; }

namespace PokeVault::Integration::Gen2 {

inline constexpr uint8_t kMasterBallItemId = 0x01;
inline constexpr uint8_t kPokeBallItemId = 0x05;
inline constexpr uint8_t kPotionItemId = 0x12;
inline constexpr uint32_t kMaxMoney = 999999;
inline constexpr uint8_t kMaxStackQuantity = 99;
inline constexpr std::size_t kInternationalTrainerNameMax = 7;
inline constexpr std::size_t kInternationalPokemonNameMax = 10;
inline constexpr uint16_t kMaxGen2Species = 251;
inline constexpr uint16_t kMaxGen2Move = 251;

struct StagedChange {
    std::string key;
    std::string label;
    std::string beforeValue;
    std::string afterValue;
};

struct BoxPokemonEdit {
    std::optional<uint16_t> species;
    std::optional<std::string> nickname;
    std::optional<uint8_t> level;
    std::optional<uint32_t> experience;
    std::optional<uint8_t> heldItem;
    std::optional<std::array<uint8_t, 4>> moves;
    std::optional<std::array<uint8_t, 4>> pp;
    std::optional<std::array<uint8_t, 4>> ppUps;
    std::optional<std::array<uint8_t, 4>> dvs;
    std::optional<std::array<uint16_t, 5>> statExperience;
    std::optional<std::string> otName;
    std::optional<uint16_t> trainerId;
    std::optional<uint8_t> friendship;
    std::optional<uint8_t> pokerus;
    std::optional<uint16_t> caughtData;
};

struct BoxPokemonCreate {
    uint16_t species = 0;
    uint8_t level = 5;
    // When present, EXP is canonical and level is derived with the species growth curve.
    std::optional<uint32_t> experience;
    std::string nickname;
    std::string otName;
    std::optional<uint16_t> trainerId;
    uint8_t heldItem = 0;
    std::array<uint8_t, 4> moves{};
    std::array<uint8_t, 4> pp{};
    std::array<uint8_t, 4> ppUps{};
    std::array<uint8_t, 4> dvs{};
    std::array<uint16_t, 5> statExperience{};
    uint8_t friendship = 70;
    uint8_t pokerus = 0;
    uint16_t caughtData = 0;
};

class StagedEditor final {
public:
    static std::unique_ptr<StagedEditor> create(const ReadOnlySave& source, std::string& error);

    const SaveEdit::Capabilities& capabilities() const noexcept { return capabilities_; }
    const Metadata& metadata() const noexcept { return metadata_; }
    std::span<const uint8_t> originalBytes() const noexcept { return original_; }
    std::span<const uint8_t> stagedBytes() const noexcept { return staged_; }

    const std::string& trainerName() const noexcept { return trainerName_; }
    uint16_t trainerId() const noexcept { return trainerId_; }
    uint32_t money() const noexcept { return money_; }
    uint8_t itemQuantity(InventoryPocket pocket, uint8_t itemId) const noexcept;

    bool stageTrainerEdit(std::string_view name, uint32_t money, std::string& error);
    bool stageTrainerName(std::string_view name, std::string& error);
    bool stageMoney(uint32_t money, std::string& error);

    // P3 (Round 5): Play Time (Gold / Silver / Crystal) and Crystal Trainer Gender, staged in this
    // image. Offsets are the ones Gen2ReadOnlySave already reads (PKHeX 6042efa241a9 SAV2Offsets):
    // play time = big-endian u16 hours, then minutes and seconds bytes, at 0x2053 (international
    // Gold / Silver) or 0x2052 (international Crystal), inside the checksummed main block (the
    // checksum and backup mirror are repaired at finalize). Crystal gender = one byte at 0x3E3D,
    // 0 male / 1 female. Gold / Silver have no gender byte. Hours are capped at 999 (the shared
    // Gen III-V editor bound); the timer's other bytes are never touched.
    struct PlayTime {
        uint16_t hours = 0;
        uint8_t minutes = 0;
        uint8_t seconds = 0;
        friend bool operator==(const PlayTime&, const PlayTime&) = default;
    };
    static constexpr uint16_t kMaxPlayedHours = 999;
    PlayTime playTime() const noexcept;
    bool playTimeEditable() const noexcept;
    bool stagePlayTime(PlayTime time, std::string& error);
    uint8_t trainerGender() const noexcept;
    bool trainerGenderEditable() const noexcept;
    bool stageTrainerGender(uint8_t gender, std::string& error);
    bool stageItemQuantity(InventoryPocket pocket, uint8_t itemId, uint8_t quantity,
                           std::string& error);

    // Universal staged inventory path used by the normal PKSE-style Items screen. Unlike the
    // original first-milestone helper above, this supports all five retail GSC containers and
    // validates exact Gold/Silver/Crystal catalog membership before mutation.
    std::vector<InventoryItem> inventoryEntries(InventoryPocket pocket, std::string& error) const;
    uint8_t stagedInventoryQuantity(InventoryPocket pocket, uint8_t itemId) const;
    bool stageInventoryQuantity(InventoryPocket pocket, uint8_t itemId, uint8_t quantity,
                                std::string& error);

    std::optional<PokemonRecord> boxedPokemon(std::size_t box, std::size_t slot,
                                               std::string& error) const;
    std::optional<PokemonRecord> partyPokemon(std::size_t slot, std::string& error) const;
    bool stagePartyPokemonEdit(std::size_t slot, const BoxPokemonEdit& edit, std::string& error);
    bool stageBoxPokemonEdit(std::size_t box, std::size_t slot, const BoxPokemonEdit& edit,
                             std::string& error);
    bool stageBoxPokemonShiny(std::size_t box, std::size_t slot, bool shiny,
                              std::string& error);
    bool stageCloneBoxPokemon(std::size_t sourceBox, std::size_t sourceSlot,
                              std::size_t destinationBox, std::size_t& destinationSlot,
                              std::string& error);
    bool stageAddBoxPokemon(std::size_t destinationBox, const BoxPokemonCreate& pokemon,
                            std::size_t& destinationSlot, std::string& error);
    bool stageReleaseBoxPokemon(std::size_t box, std::size_t slot, std::string& error);

    // Feature A (owner Oct 28): standalone .pk2 files (PokemonFile/NativePokemonFile.h).
    // stageImportBoxPokemon places one decoded international .pk2 into the first empty slot of a
    // box, as a staged change only (Review / Keep / Discard / Export Copy as for Add and Clone).
    // The box keeps the first 32 bytes of the 48-byte party body (pret/pokecrystal box_struct),
    // the file's list species byte (0xFD for an Egg, which imports as an Egg exactly as stored) and
    // its 11-byte OT and nickname, all byte for byte. The record must pass the strict parser and
    // this editor's own validators (species, level/EXP, held item, moves/PP, Pokerus). Japanese
    // files, other generations, occupied or non-first-empty slots and full boxes are refused;
    // nothing is converted, invented or legalized.
    bool stageImportBoxPokemon(std::size_t box, std::size_t slot,
                               const PokeBank::PokemonFile::Decoded& file, std::string& error);
    // PKHeX single-entry .pk2 list (73 bytes, PokeCrypto.cs SIZE_2LIST) of a staged box or Party
    // Pokemon. Reads only. A box record gets the party-only fields the game computes when it is
    // withdrawn (pokecrystal engine/pokemon/move_mon.asm SendGetMonIntoFromBox, PC_WITHDRAW:
    // CalcLevel into MON_LEVEL, CalcMonStats with Stat Exp, status 0, HP = max HP or 0 for an Egg).
    // A Party record is copied as stored.
    std::optional<std::vector<uint8_t>> exportBoxPokemonFile(std::size_t box, std::size_t slot,
                                                             std::string& error) const;
    std::optional<std::vector<uint8_t>> exportPartyPokemonFile(std::size_t slot,
                                                               std::string& error) const;

    // Packed relocation engine used by the native classic box Y interaction. The single-Pokemon
    // entry points remain the Part 1 contract; Part 2 extends the same engine to an ordered group.
    bool beginPackedMove(std::size_t sourceBox, std::size_t sourceSlot, std::string& error);
    bool placePackedMove(std::size_t destinationBox, std::size_t destinationSlot,
                         std::size_t& placedSlot, std::string& error);
    bool beginPackedGroupMove(std::size_t sourceBox, std::span<const std::size_t> sourceSlots,
                              std::string& error);
    bool placePackedGroupMove(std::size_t destinationBox, std::size_t destinationSlot,
                              std::size_t& firstPlacedSlot, std::string& error);
    bool cancelPackedMove(std::string& error);
    bool packedMoveActive() const noexcept { return packedMove_.active; }
    std::size_t packedMoveCount() const noexcept { return packedMove_.sourceSlots.size(); }

    static bool isShinyDVs(const std::array<uint8_t, 4>& dvs) noexcept;
    static uint8_t derivedHPDV(const std::array<uint8_t, 4>& dvs) noexcept;
    static uint8_t gen2MoveBasePP(uint16_t move) noexcept;
    static uint8_t gen2MoveMaxPP(uint8_t move, uint8_t ppUps) noexcept;

    const std::vector<StagedChange>& pendingChanges() const noexcept { return changes_; }
    bool hasPendingChanges() const noexcept { return !changes_.empty(); }
    void discard() noexcept;

    std::vector<uint8_t> finalizedBytes(std::string& error) const;
    // Display-only bytes: the staged image repaired for a strict reload, also while a packed move
    // is carrying Pokemon (finalizedBytes refuses then). Never exported or written anywhere.
    std::vector<uint8_t> presentationBytes(std::string& error) const;

private:
    struct PokemonExpectation {
        std::size_t box = 0;
        std::size_t slot = 0;
        PokemonRecord expected;
    };

    struct PackedMoveState {
        bool active = false;
        std::size_t sourceBox = 0;
        std::vector<std::size_t> sourceSlots;
        std::vector<PokemonRecord> carried;
        std::vector<uint8_t> stagedBefore;
        std::vector<StagedChange> changesBefore;
        std::vector<PokemonExpectation> expectationsBefore;
    };

    explicit StagedEditor(const ReadOnlySave& source);

    bool encodeTrainerName(std::string_view name, std::vector<uint8_t>& encoded,
                           std::string& error) const;
    bool encodePokemonName(std::string_view name, std::vector<uint8_t>& encoded,
                           std::string& error) const;
    bool encodeOTName(std::string_view name, std::vector<uint8_t>& encoded,
                      std::string& error) const;
    bool syncCurrentBoxCopy(std::size_t box, std::string& error);
    bool stageRecordEdit(bool party, std::size_t box, std::size_t slot,
                         const BoxPokemonEdit&, std::string& error);
    void rememberPokemonExpectation(std::size_t box, std::size_t slot,
                                    const PokemonRecord& expected);
    void setChange(std::string key, std::string label,
                   std::string beforeValue, std::string afterValue);
    void clearChange(std::string_view key);

    Metadata metadata_;
    SourceGame sourceGame_ = SourceGame::Gold;
    SaveEdit::Capabilities capabilities_;
    std::vector<uint8_t> original_;
    std::vector<uint8_t> staged_;
    std::string originalTrainerName_;
    std::string trainerName_;
    uint16_t trainerId_ = 0;
    uint32_t originalMoney_ = 0;
    uint32_t money_ = 0;
    std::vector<StagedChange> changes_;
    std::vector<PokemonExpectation> pokemonExpectations_;
    std::vector<std::pair<std::size_t, PokemonRecord>> partyExpectations_;
    PackedMoveState packedMove_;
};

} // namespace PokeVault::Integration::Gen2

#endif
