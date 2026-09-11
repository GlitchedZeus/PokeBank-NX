#ifndef POKEBANK_GEN2_READ_ONLY_SAVE_H
#define POKEBANK_GEN2_READ_ONLY_SAVE_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace PokeVault::Integration::Gen2 {
constexpr std::size_t kRawSaveSize32K = 0x8000;
constexpr std::size_t kRawSaveSize64K = 0x10000;
enum class RegionLayout : uint8_t { International, Japanese };
enum class VersionFamily : uint8_t { GoldSilver, Crystal };
enum class SourceGame : uint8_t { Gold, Silver, Crystal };
enum class SaveError : uint8_t { None, WrongSize, UnsupportedRegion, ChecksumMismatch, AmbiguousLayout, InvalidStructure, InvalidTrainerData, InvalidParty, InvalidBox, GameHintMismatch };
struct PokemonRecord {
    uint16_t species = 0; uint8_t heldItem = 0; std::array<uint8_t, 4> moves{}; uint16_t trainerId = 0; uint32_t experience = 0;
    std::array<uint16_t, 5> statExperience{}; std::array<uint8_t, 5> dvs{}; std::array<uint8_t, 4> pp{}; std::array<uint8_t, 4> ppUps{};
    uint8_t friendship = 0; uint8_t pokerus = 0; uint16_t caughtData = 0; uint8_t level = 0; uint8_t status = 0;
    uint16_t currentHP = 0, maxHP = 0, attack = 0, defense = 0, speed = 0, specialAttack = 0, specialDefense = 0;
    std::string originalTrainer; std::string nickname; bool partyRecord = false; bool isEgg = false; bool shiny = false; uint8_t gender = 2; uint8_t form = 0;
    SourceGame sourceGame = SourceGame::Gold; RegionLayout region = RegionLayout::International;
    std::array<uint8_t, 48> rawBody{}; std::size_t rawBodySize = 0;
};
struct TrainerRecord { std::string name; uint16_t trainerId = 0; uint32_t money = 0; std::optional<uint8_t> gender; };
struct BoxRecord { std::string name; std::vector<std::optional<PokemonRecord>> slots; };
struct Metadata {
    SourceGame sourceGame = SourceGame::Gold; VersionFamily family = VersionFamily::GoldSilver; RegionLayout region = RegionLayout::International;
    std::string sourceGameId; uint8_t currentBox = 0, boxCount = 0, boxCapacity = 0; std::size_t payloadSize = 0, rtcFooterSize = 0;
};
class ReadOnlySave; struct ParseResult;
class ReadOnlySave final {
public:
    const TrainerRecord& trainer() const noexcept { return trainer_; } const std::vector<PokemonRecord>& party() const noexcept { return party_; }
    const std::vector<BoxRecord>& boxes() const noexcept { return boxes_; } const Metadata& metadata() const noexcept { return metadata_; }
    std::span<const uint8_t> sourceBytes() const noexcept { return sourceBytes_; }
    std::span<const uint8_t> payloadBytes() const noexcept { return std::span<const uint8_t>(sourceBytes_.data(), metadata_.payloadSize); }
private:
    friend ParseResult parse(std::span<const uint8_t>, SourceGame); std::vector<uint8_t> sourceBytes_; TrainerRecord trainer_; std::vector<PokemonRecord> party_; std::vector<BoxRecord> boxes_; Metadata metadata_;
};
struct ParseResult { std::shared_ptr<const ReadOnlySave> save; SaveError error = SaveError::None; std::string detail; explicit operator bool() const noexcept { return static_cast<bool>(save); } };
ParseResult parse(std::span<const uint8_t> raw, SourceGame hint);
const char* sourceGameId(SourceGame game) noexcept; const char* regionName(RegionLayout region) noexcept; const char* familyName(VersionFamily family) noexcept; const char* errorName(SaveError error) noexcept;
uint16_t calculateChecksum(std::span<const uint8_t> payload, RegionLayout region, VersionFamily family) noexcept;
std::string decodeGen2String(std::span<const uint8_t> bytes, RegionLayout region); bool isKnownRTCFooterSize(std::size_t size) noexcept;
} // namespace PokeVault::Integration::Gen2
#endif
