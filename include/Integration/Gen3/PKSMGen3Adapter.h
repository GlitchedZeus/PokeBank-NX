/*
 * PokeBank NX read-only Generation III adapter.
 *
 * This boundary deliberately exposes no PKSM-Core types. PKSM-Core is GPLv3 and its
 * additional 7.b/7.c notices apply; see vendor/PKSM-Core/LICENSE and
 * docs/PKSM_CORE_INTEGRATION.md. PokeBank NX copies source bytes before handing them to
 * the engine and never calls a mutation or resigning API from this adapter.
 */
#ifndef POKEBANK_INTEGRATION_GEN3_PKSM_GEN3_ADAPTER_H
#define POKEBANK_INTEGRATION_GEN3_PKSM_GEN3_ADAPTER_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace PokeVault::Integration::Gen3 {
    inline constexpr std::string_view PKSM_CORE_REVISION =
        "aa22d7a4f87c0351baf7da5962ba5acd01039a7c";

    enum class SourceGame : uint8_t {
        FireRedGBA,
        LeafGreenGBA,
    };

    enum class SaveError : uint8_t {
        None,
        WrongSize,
        InvalidSectorId,
        DuplicateSector,
        MissingSector,
        BadSectorSignature,
        MismatchedSectorCounters,
        BadSectorChecksum,
        UnsupportedGame,
        InvalidPartyCount,
        CoreRejected,
        MalformedPokemon,
    };

    struct PokemonLocation {
        enum class Kind : uint8_t { Party, Box } kind = Kind::Party;
        uint8_t box = 0;
        uint8_t slot = 0;
    };

    struct PokemonRecord {
        PokemonLocation location{};
        std::vector<uint8_t> originalBytes;
        uint16_t species = 0;
        uint32_t pid = 0;
        uint16_t tid = 0;
        uint16_t sid = 0;
        uint32_t experience = 0;
        // Cross-generation item id exposed by PKSM-Core.
        uint16_t heldItem = 0;
        // Exact Generation III item id preserved for historical/raw inspection.
        uint16_t heldItemGen3 = 0;
        std::array<uint16_t, 4> moves{};
        std::array<uint8_t, 4> pp{};
        // Gen III order: HP, Attack, Defense, Speed, Special Attack, Special Defense.
        std::array<uint8_t, 6> ivs{};
        std::array<uint8_t, 6> evs{};
        std::string nickname;
        std::string otName;
        bool checksumValid = false;
        // PKSM-Core decrypt -> clone -> encrypt reproduced originalBytes exactly.
        bool byteIdenticalRoundTrip = false;
    };

    struct SaveMetadata {
        SourceGame sourceGame = SourceGame::FireRedGBA;
        std::string_view sourceGameId;
        uint8_t activeSlot = 0;
        uint32_t saveCounter = 0;
        uint8_t partyCount = 0;
        uint8_t boxCount = 14;
        uint8_t slotsPerBox = 30;
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
        std::unique_ptr<ReadOnlySave> save;
        SaveError error = SaveError::None;
        std::string detail;

        [[nodiscard]] explicit operator bool() const noexcept { return save != nullptr; }
    };

    // FireRed and LeafGreen use the same on-save family marker. The exact release identity
    // therefore comes from the detected source selected by the caller, never from display text.
    [[nodiscard]] ParseResult parse(std::span<const uint8_t> bytes, SourceGame sourceGame);
    [[nodiscard]] std::string_view sourceGameId(SourceGame game) noexcept;
    [[nodiscard]] std::string_view errorMessage(SaveError error) noexcept;
}

#endif
