#ifndef POKEVAULT_PKSM_GEN3_ADAPTER_H
#define POKEVAULT_PKSM_GEN3_ADAPTER_H

#include <array>
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace PokeVault::Integration::Gen3 {
    enum class SourceGame : uint8_t { RubyGBA, SapphireGBA, EmeraldGBA, FireRedGBA, LeafGreenGBA };
    enum class SaveError : uint8_t {
        None, WrongSize, InvalidSectorId, DuplicateSectorId, MissingSector, SignatureMismatch,
        ChecksumMismatch, SaveCounterMismatch, FamilyMismatch, InvalidPartyCount, CoreRejected,
        MalformedPokemon,
    };
    struct TrainerRecord {
        std::string name; uint8_t gender = 0; uint16_t tid16 = 0; uint16_t sid16 = 0;
        uint32_t id32 = 0; uint32_t money = 0;
    };
    enum class InventoryPouch : uint8_t { Items, KeyItems, PokeBalls, TMCase, BerryPouch, PCItems };
    struct InventoryItemRecord { uint16_t itemId = 0; uint16_t count = 0; };
    struct InventoryPouchRecord { InventoryPouch pouch = InventoryPouch::Items; std::vector<InventoryItemRecord> items; };
    struct PokemonLocation { enum class Kind : uint8_t { Party, Box } kind = Kind::Party; uint8_t box = 0; uint8_t slot = 0; };
    struct PokemonRecord {
        PokemonLocation location{}; std::vector<uint8_t> originalBytes; uint16_t species = 0;
        uint32_t pid = 0; uint16_t tid = 0; uint16_t sid = 0; uint32_t experience = 0;
        uint16_t heldItem = 0; uint16_t heldItemGen3 = 0; std::array<uint16_t, 4> moves{};
        std::array<uint8_t, 4> pp{}; std::array<uint8_t, 6> ivs{}; std::array<uint8_t, 6> evs{};
        std::string nickname; std::string otName; bool checksumValid = false; bool byteIdenticalRoundTrip = false;
    };
    struct SaveMetadata {
        SourceGame sourceGame = SourceGame::FireRedGBA; std::string_view sourceGameId; uint8_t activeSlot = 0;
        uint32_t saveCounter = 0; uint8_t partyCount = 0; uint8_t boxCount = 14; uint8_t slotsPerBox = 30;
    };
    struct ParseResult;
    class ReadOnlySave {
    public:
        ReadOnlySave(ReadOnlySave&&) noexcept;
        ReadOnlySave& operator=(ReadOnlySave&&) noexcept;
        ~ReadOnlySave();
        ReadOnlySave(const ReadOnlySave&) = delete;
        ReadOnlySave& operator=(const ReadOnlySave&) = delete;
        [[nodiscard]] const SaveMetadata& metadata() const noexcept;
        [[nodiscard]] const TrainerRecord& trainer() const noexcept;
        [[nodiscard]] const std::vector<InventoryPouchRecord>& inventory() const noexcept;
        [[nodiscard]] std::span<const uint8_t> sourceBytes() const noexcept;
        [[nodiscard]] std::vector<PokemonRecord> party() const;
        [[nodiscard]] std::vector<PokemonRecord> boxes() const;
        [[nodiscard]] SaveError lastEnumerationError() const noexcept;
    private:
        struct Impl;
        explicit ReadOnlySave(std::unique_ptr<Impl> impl) noexcept;
        std::unique_ptr<Impl> impl_;
        friend struct ParseResult;
        friend ParseResult parse(std::span<const uint8_t>, SourceGame);
    };
    struct ParseResult {
        std::unique_ptr<ReadOnlySave> save; SaveError error = SaveError::None; std::string detail;
        [[nodiscard]] explicit operator bool() const noexcept { return save != nullptr; }
    };
    [[nodiscard]] ParseResult parse(std::span<const uint8_t> bytes, SourceGame sourceGame);
    [[nodiscard]] std::string_view sourceGameId(SourceGame game) noexcept;
    [[nodiscard]] std::string_view errorMessage(SaveError error) noexcept;
}

#endif
