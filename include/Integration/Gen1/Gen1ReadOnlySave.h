#ifndef POKEBANK_GEN1_READ_ONLY_SAVE_H
#define POKEBANK_GEN1_READ_ONLY_SAVE_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace PokeVault::Integration::Gen1 {

// Raw Gen I battery-save support is intentionally read-only. The format facts in this adapter are
// pinned to PKSM-Core aa22d7a4f87c0351baf7da5962ba5acd01039a7c (Sav1/PK1) and independently
// cross-checked against PKHeX e15d2467b32da7bc26ce7cc8e5c4ede32740e20a (SAV1/PK1/PokeList1).
constexpr std::size_t kRawSaveSize = 0x8000;

enum class RegionLayout : uint8_t {
    International,
    Japanese,
};

enum class SourceGame : uint8_t {
    Red,
    Blue,
    Yellow,
};

enum class VersionEvidence : uint8_t {
    Unknown,
    RedBlueFamily,
    Yellow,
};

enum class SaveError : uint8_t {
    None,
    WrongSize,
    ChecksumMismatch,
    AmbiguousRegion,
    InvalidStructure,
    InvalidTrainerData,
    InvalidParty,
    InvalidBox,
    GameHintMismatch,
};

struct PokemonRecord {
    uint8_t rawSpecies = 0;
    uint16_t species = 0;
    uint16_t currentHP = 0;
    uint8_t status = 0;
    uint8_t type1 = 0;
    uint8_t type2 = 0;
    uint8_t catchRate = 0;
    std::array<uint8_t, 4> moves{};
    uint16_t trainerId = 0;
    uint32_t experience = 0;
    std::array<uint16_t, 5> statExperience{}; // HP / Atk / Def / Spe / Special
    std::array<uint8_t, 5> dvs{};             // HP / Atk / Def / Spe / Special (0..15)
    std::array<uint8_t, 4> pp{};
    std::array<uint8_t, 4> ppUps{};
    uint8_t level = 0;
    uint16_t maxHP = 0;
    uint16_t attack = 0;
    uint16_t defense = 0;
    uint16_t speed = 0;
    uint16_t special = 0;
    std::string originalTrainer;
    std::string nickname;
    bool partyRecord = false;
    std::array<uint8_t, 44> rawBody{};
    std::size_t rawBodySize = 0;
};

struct TrainerRecord {
    std::string name;
    uint16_t trainerId = 0;
    uint32_t money = 0;
};

struct BoxRecord {
    std::vector<std::optional<PokemonRecord>> slots;
};

struct Metadata {
    SourceGame sourceGame = SourceGame::Red;
    std::string sourceGameId;
    RegionLayout region = RegionLayout::International;
    VersionEvidence versionEvidence = VersionEvidence::Unknown;
    uint8_t currentBox = 0;
    bool boxesInitialized = false;
};

class ReadOnlySave final {
public:
    const TrainerRecord& trainer() const noexcept { return trainer_; }
    const std::vector<PokemonRecord>& party() const noexcept { return party_; }
    const std::vector<BoxRecord>& boxes() const noexcept { return boxes_; }
    const Metadata& metadata() const noexcept { return metadata_; }
    std::span<const uint8_t> sourceBytes() const noexcept { return sourceBytes_; }

private:
    friend struct ParseResult;
    friend ParseResult parse(std::span<const uint8_t>, SourceGame);

    std::vector<uint8_t> sourceBytes_;
    TrainerRecord trainer_;
    std::vector<PokemonRecord> party_;
    std::vector<BoxRecord> boxes_;
    Metadata metadata_;
};

struct ParseResult {
    std::shared_ptr<const ReadOnlySave> save;
    SaveError error = SaveError::None;
    std::string detail;
    explicit operator bool() const noexcept { return static_cast<bool>(save); }
};

ParseResult parse(std::span<const uint8_t> raw, SourceGame game);

const char* sourceGameId(SourceGame game) noexcept;
const char* regionName(RegionLayout region) noexcept;
const char* errorName(SaveError error) noexcept;

// Exposed for focused oracle/regression tests.
uint8_t calculateDiff8(std::span<const uint8_t> bytes) noexcept;
uint16_t gen1InternalToNational(uint8_t rawSpecies) noexcept;
std::string decodeGen1String(std::span<const uint8_t> bytes, RegionLayout region);

} // namespace PokeVault::Integration::Gen1

#endif
