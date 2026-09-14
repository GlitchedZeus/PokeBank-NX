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
    std::string nickname;
    std::string otName;
    std::optional<uint16_t> trainerId;
    uint8_t heldItem = 0;
    std::array<uint8_t, 4> moves{};
    std::array<uint8_t, 4> pp{};
    std::array<uint8_t, 4> ppUps{};
    std::array<uint8_t, 4> dvs{8, 8, 8, 8};
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

    bool stageTrainerName(std::string_view name, std::string& error);
    bool stageMoney(uint32_t money, std::string& error);
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
    bool stageBoxPokemonEdit(std::size_t box, std::size_t slot, const BoxPokemonEdit& edit,
                             std::string& error);
    bool stageBoxPokemonShiny(std::size_t box, std::size_t slot, bool shiny,
                              std::string& error);
    bool stageCloneBoxPokemon(std::size_t sourceBox, std::size_t sourceSlot,
                              std::size_t destinationBox, std::size_t& destinationSlot,
                              std::string& error);
    bool stageAddBoxPokemon(std::size_t destinationBox, const BoxPokemonCreate& pokemon,
                            std::size_t& destinationSlot, std::string& error);

    static bool isShinyDVs(const std::array<uint8_t, 4>& dvs) noexcept;
    static uint8_t derivedHPDV(const std::array<uint8_t, 4>& dvs) noexcept;
    static uint8_t gen2MoveBasePP(uint16_t move) noexcept;

    const std::vector<StagedChange>& pendingChanges() const noexcept { return changes_; }
    bool hasPendingChanges() const noexcept { return !changes_.empty(); }
    void discard() noexcept;

    std::vector<uint8_t> finalizedBytes(std::string& error) const;

private:
    struct PokemonExpectation {
        std::size_t box = 0;
        std::size_t slot = 0;
        PokemonRecord expected;
    };

    explicit StagedEditor(const ReadOnlySave& source);

    bool encodeTrainerName(std::string_view name, std::vector<uint8_t>& encoded,
                           std::string& error) const;
    bool encodePokemonName(std::string_view name, std::vector<uint8_t>& encoded,
                           std::string& error) const;
    bool syncCurrentBoxCopy(std::size_t box, std::string& error);
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
};

} // namespace PokeVault::Integration::Gen2

#endif
