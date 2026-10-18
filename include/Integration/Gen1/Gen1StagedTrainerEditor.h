#ifndef POKEBANK_INTEGRATION_GEN1_STAGED_TRAINER_EDITOR_H
#define POKEBANK_INTEGRATION_GEN1_STAGED_TRAINER_EDITOR_H

#include "Integration/Gen1/Gen1ReadOnlySave.h"
#include "Integration/Gen1/Gen1StagedPokemonEditor.h"

#include <array>
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace PokeVault::Integration::Gen1 {

// Staged Gen I Trainer Name / Money. Layout facts (PKHeX 6501f0ab SAV1Offsets.cs / SAV1.cs and
// PKSM-Core aa22d7a Sav1.cpp, mirrored by Gen1ReadOnlySave.cpp's layout table):
//   player name  0x2598, International 11 bytes (7 characters + 0x50 terminator / padding),
//                Japanese 6 bytes (5 characters + terminator)
//   money        International 0x25F3, Japanese 0x25EE: 3 bytes packed BCD, big-endian
//                (pret/pokered wPlayerMoney), at most 999999 (PKHeX SAV1.MaxMoney)
//   checksum     International 0x3523 over 0x2598..0x3522, Japanese 0x3594 over 0x2598..0x3593:
//                the Gen I main-data checksum (calculateDiff8, pret/pokered SAVCheckSum).
// The source save is never modified; finalize() returns a separate, strictly reparsed buffer.
inline constexpr uint32_t kGen1MaxMoney = 999999;
inline constexpr std::size_t kGen1InternationalTrainerNameMax = 7;
inline constexpr std::size_t kGen1JapaneseTrainerNameMax = 5;

struct TrainerChange {
    std::string label;
    std::string beforeValue;
    std::string afterValue;
};

// BCD3 helpers below (pret/pokered: money is three bytes of packed BCD, most significant byte first).

class StagedTrainerEditor final {
public:
    static std::unique_ptr<StagedTrainerEditor> create(const ReadOnlySave& source, std::string& error);

    RegionLayout region() const noexcept { return region_; }
    SourceGame sourceGame() const noexcept { return sourceGame_; }
    std::span<const uint8_t> originalBytes() const noexcept { return originalBytes_; }
    std::span<const uint8_t> stagedBytes() const noexcept { return stagedBytes_; }

    const std::string& trainerName() const noexcept { return name_; }
    uint32_t money() const noexcept { return money_; }
    // Name editing uses the shared Gen I English glyph encoder; Japanese kana names stay read-only.
    bool nameEditable() const noexcept { return region_ == RegionLayout::International; }
    std::size_t nameMax() const noexcept {
        return region_ == RegionLayout::Japanese ? kGen1JapaneseTrainerNameMax : kGen1InternationalTrainerNameMax;
    }

    bool stageTrainerName(std::string_view name, std::string& error);
    bool stageMoney(uint32_t money, std::string& error);

    bool hasPendingChanges() const noexcept { return stagedBytes_ != originalBytes_; }
    std::vector<TrainerChange> pendingChanges() const;
    void discard() noexcept;
    std::vector<uint8_t> finalize(std::string& error) const;

    // Byte ranges a trainer edit may change (name, money); the checksum is recomputed.
    std::size_t nameOffset() const noexcept { return 0x2598; }
    std::size_t nameLength() const noexcept { return region_ == RegionLayout::Japanese ? 6 : 11; }
    std::size_t moneyOffset() const noexcept { return region_ == RegionLayout::Japanese ? 0x25EE : 0x25F3; }

private:
    explicit StagedTrainerEditor(const ReadOnlySave& source);
    std::size_t checksumOffset() const noexcept { return region_ == RegionLayout::Japanese ? 0x3594 : 0x3523; }
    std::size_t mainLength() const noexcept { return region_ == RegionLayout::Japanese ? 0x0FFC : 0x0F8B; }
    void refreshChecksum(std::vector<uint8_t>& bytes) const;

    RegionLayout region_;
    SourceGame sourceGame_;
    std::vector<uint8_t> originalBytes_;
    std::vector<uint8_t> stagedBytes_;
    std::string originalName_;
    uint32_t originalMoney_ = 0;
    std::string name_;
    uint32_t money_ = 0;
};

// ---- inline implementation (header-only so every existing Gen I link list builds unchanged) ----


inline bool encodeMoneyBCD3(uint32_t money, std::array<uint8_t, 3>& out) noexcept {
    if (money > kGen1MaxMoney) return false;
    for (int i = 2; i >= 0; --i) {
        const uint8_t lo = static_cast<uint8_t>(money % 10); money /= 10;
        const uint8_t hi = static_cast<uint8_t>(money % 10); money /= 10;
        out[static_cast<std::size_t>(i)] = static_cast<uint8_t>((hi << 4) | lo);
    }
    return true;
}

inline bool decodeMoneyBCD3(std::span<const uint8_t, 3> raw, uint32_t& out) noexcept {
    out = 0;
    for (const uint8_t b : raw) {
        const uint8_t hi = b >> 4, lo = b & 0x0F;
        if (hi > 9 || lo > 9) return false;
        out = out * 100 + hi * 10 + lo;
    }
    return true;
}

inline StagedTrainerEditor::StagedTrainerEditor(const ReadOnlySave& source)
    : region_(source.metadata().region), sourceGame_(source.metadata().sourceGame),
      originalBytes_(source.sourceBytes().begin(), source.sourceBytes().end()),
      stagedBytes_(originalBytes_), originalName_(source.trainer().name),
      originalMoney_(source.trainer().money), name_(originalName_), money_(originalMoney_) {}

inline std::unique_ptr<StagedTrainerEditor> StagedTrainerEditor::create(const ReadOnlySave& source, std::string& error) {
    error.clear();
    if (source.sourceBytes().size() != kRawSaveSize) {
        error = "Generation I trainer editing needs a 0x8000-byte battery save";
        return nullptr;
    }
    return std::unique_ptr<StagedTrainerEditor>(new StagedTrainerEditor(source));
}

inline void StagedTrainerEditor::refreshChecksum(std::vector<uint8_t>& bytes) const {
    bytes[checksumOffset()] = calculateDiff8(std::span<const uint8_t>(bytes).subspan(0x2598, mainLength()));
}

inline bool StagedTrainerEditor::stageTrainerName(std::string_view name, std::string& error) {
    error.clear();
    if (!nameEditable()) {
        error = "Japanese Generation I trainer names are read-only (no kana encoder)";
        return false;
    }
    std::array<uint8_t, 11> encoded{};
    if (!StagedPokemonEditor::encodeName(name, kGen1InternationalTrainerNameMax, encoded, error)) return false;
    auto next = stagedBytes_;
    for (std::size_t i = 0; i < nameLength(); ++i) next[nameOffset() + i] = encoded[i];
    refreshChecksum(next);
    const auto check = parse(next, sourceGame_);
    if (!check || check.save->trainer().name != name) {
        error = "Generation I trainer name failed strict reparse";
        return false;
    }
    stagedBytes_ = std::move(next);
    name_ = std::string(name);
    return true;
}

inline bool StagedTrainerEditor::stageMoney(uint32_t money, std::string& error) {
    error.clear();
    std::array<uint8_t, 3> bcd{};
    if (!encodeMoneyBCD3(money, bcd)) {
        error = "Generation I money is 0-999999";
        return false;
    }
    auto next = stagedBytes_;
    for (std::size_t i = 0; i < 3; ++i) next[moneyOffset() + i] = bcd[i];
    refreshChecksum(next);
    const auto check = parse(next, sourceGame_);
    if (!check || check.save->trainer().money != money) {
        error = "Generation I money failed strict reparse";
        return false;
    }
    stagedBytes_ = std::move(next);
    money_ = money;
    return true;
}

inline std::vector<TrainerChange> StagedTrainerEditor::pendingChanges() const {
    std::vector<TrainerChange> out;
    if (name_ != originalName_) out.push_back({"Trainer Name", originalName_, name_});
    if (money_ != originalMoney_)
        out.push_back({"Money", "$" + std::to_string(originalMoney_), "$" + std::to_string(money_)});
    return out;
}

inline void StagedTrainerEditor::discard() noexcept {
    stagedBytes_ = originalBytes_;
    name_ = originalName_;
    money_ = originalMoney_;
}

inline std::vector<uint8_t> StagedTrainerEditor::finalize(std::string& error) const {
    error.clear();
    std::vector<uint8_t> candidate = stagedBytes_;
    refreshChecksum(candidate);
    const auto check = parse(candidate, sourceGame_);
    if (!check || check.save->trainer().name != name_ || check.save->trainer().money != money_) {
        error = "Generation I trainer edit failed strict reload";
        return {};
    }
    return candidate;
}

} // namespace PokeVault::Integration::Gen1

#endif
