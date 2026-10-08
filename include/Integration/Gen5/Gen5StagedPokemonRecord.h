#ifndef POKEBANK_GEN5_STAGED_RECORD_H
#define POKEBANK_GEN5_STAGED_RECORD_H

#include "Pokemon/Pokemon5ReadOnly.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace PokeVault::Integration::Gen5 {

// Entity-only Gen V staging. Not a SAV writer, source provider, legality
// auto-fixer, or shared-editor UI. Every transaction operates on app-owned
// buffers and must pass a decrypt/encrypt/reparse postcondition.
class StagedPokemon5Record {
public:
    enum class Field : uint8_t { Nature, Friendship, IV, EV };

    static std::optional<StagedPokemon5Record> create(
        std::span<const uint8_t> source, std::string* error = nullptr) {
        const Pokemon5ReadOnly parsed(source);
        if (!parsed.valid() || parsed.empty()) {
            if (error) *error = "Gen V edit requires a valid occupied PK5 record";
            return std::nullopt;
        }
        if (error) error->clear();
        return StagedPokemon5Record(std::vector<uint8_t>(source.begin(), source.end()));
    }

    [[nodiscard]] std::span<const uint8_t> originalBytes() const noexcept { return original_; }
    [[nodiscard]] std::span<const uint8_t> stagedBytes() const noexcept { return staged_; }
    [[nodiscard]] bool dirty() const noexcept { return staged_ != original_; }
    [[nodiscard]] Pokemon5ReadOnly current() const { return Pokemon5ReadOnly(staged_); }
    void rollback() noexcept { staged_ = original_; }

    // Field-specific bounded mutation. Move legality, ability/form coupling,
    // party-stat regeneration and native SAV block writes are NOT implemented.
    [[nodiscard]] bool stage(Field field, size_t stat, uint32_t value,
                             std::string* error = nullptr) {
        auto fail = [&](const char* msg) {
            if (error) *error = msg;
            return false;
        };
        std::vector<uint8_t> candidate = Crypto::decrypt(staged_);
        if (!Crypto::recordSizeValid(candidate.size()))
            return fail("Gen V staged source has invalid PK5 size");

        switch (field) {
            case Field::Nature:
                if (stat != 0 || value > 24)
                    return fail("Gen V nature index must be in [0,24]");
                candidate[0x41] = static_cast<uint8_t>(value);
                break;
            case Field::Friendship:
                if (stat != 0 || value > 255)
                    return fail("Gen V friendship must be in [0,255]");
                candidate[0x14] = static_cast<uint8_t>(value);
                break;
            case Field::IV: {
                if (stat >= 6 || value > 31)
                    return fail("Gen V IV stat/value is out of native range");
                constexpr size_t offset = 0x38;
                const uint32_t shift = static_cast<uint32_t>(stat * 5);
                uint32_t iv32 = Crypto::read32(candidate, offset);
                iv32 = (iv32 & ~(0x1Fu << shift)) | (value << shift);
                Crypto::write16(candidate, offset, static_cast<uint16_t>(iv32));
                Crypto::write16(candidate, offset+2, static_cast<uint16_t>(iv32 >> 16));
                break;
            }
            case Field::EV: {
                if (stat >= 6 || value > 255)
                    return fail("Gen V EV stat/value is out of native range");
                uint32_t total = value;
                for (size_t i=0; i<6; ++i)
                    if (i != stat) total += candidate[0x18+i];
                if (total > 510)
                    return fail("Gen V EV total cannot exceed 510");
                candidate[0x18+stat] = static_cast<uint8_t>(value);
                break;
            }
        }

        const auto encrypted = Crypto::encryptCandidate(candidate);
        const Pokemon5ReadOnly strict(encrypted);
        if (!strict.valid() || strict.empty())
            return fail("Gen V staged PK5 failed strict round-trip validation");
        const auto decoded = Crypto::decrypt(encrypted);
        // PK5 encryption is deterministic. Header checksum bytes 6-7 are the
        // *only* expected serialization change beyond the intended edit.
        Crypto::write16(candidate, 6, Crypto::checksum(candidate));
        if (decoded != candidate)
            return fail("Gen V staged PK5 candidate altered unrelated fields");

        // Commit atomically only after all checks have succeeded.
        staged_ = encrypted;
        if (error) error->clear();
        return true;
    }

private:
    explicit StagedPokemon5Record(std::vector<uint8_t> source)
        : original_(source), staged_(std::move(source)) {}

    std::vector<uint8_t> original_;
    std::vector<uint8_t> staged_;
};

} // namespace PokeVault::Integration::Gen5
#endif
