/**
 * ModernLegality.h - one legality entry for Generation VI-IX records.
 *
 * Part of the shared legality analyzer: it returns the same Legality::Report, Issue,
 * Severity, CoverageSummary and Verdict as analyze() (Gen I-IV) and analyzePk5 (Gen V),
 * so callers read one verdict model. A verdict follows Report::verdict():
 *   Invalid          a verified rule rules out every history the record could have,
 *   NoProblemsFound  only with Complete coverage in every area and no Warning,
 *   Incomplete       anything else (a rule or the evidence it needs is missing).
 * Invalid outranks Incomplete. No modern format has Complete coverage yet, so a record
 * without a contradiction is Incomplete, never "No problems found".
 *
 * Input: the exact source game, the record's native format, the original or staged
 * record bytes (encrypted as stored, or decrypted), the slot it was read from and the
 * save's trainer when the caller has it. The analyzer checks the record's own
 * checksum/sanity; `integrity` carries the caller's container (save block) state, which
 * a record checksum cannot prove.
 *
 * Read-only contract: the bytes are only read; nothing is written back, no save is
 * touched, no Pokemon object is built or mutated, and the same input always gives the
 * same Report. Malformed input is reported in the Report, never thrown.
 *
 * Formats with rules today: PK6 (X/Y/OR/AS) and PK7 (SN/MN/US/UM), see
 * Gen67Legality.h. Every other modern format is accepted and reported Incomplete with
 * structure coverage None until its decoder and rules are wired in.
 */
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

#include "Legality/Legality.h"

namespace Legality::Modern {

/// Native storage format of a record. The name is PKHeX's entity class.
enum class Format : uint8_t {
    Unknown,
    PK6,  // X, Y, Omega Ruby, Alpha Sapphire
    PK7,  // Sun, Moon, Ultra Sun, Ultra Moon
    PB7,  // Let's Go Pikachu / Eevee
    PK8,  // Sword, Shield
    PB8,  // Brilliant Diamond, Shining Pearl
    PA8,  // Legends: Arceus
    PK9,  // Scarlet, Violet
    PA9,  // Legends: Z-A
};

/// Exact source game, as a PKHeX GameVersion id. Unknown (0) when the caller has none.
enum class Game : uint8_t {
    Unknown = 0,
    X = 24, Y = 25, AS = 26, OR = 27,
    SN = 30, MN = 31, US = 32, UM = 33,
    GP = 42, GE = 43,
    SW = 44, SH = 45,
    PLA = 47,
    BD = 48, SP = 49,
    SL = 50, VL = 51,
    ZA = 52,
};

/// Where the record was read from. Unknown applies no slot-specific rule (for example
/// PKHeX's party-only forms), so it can only make a report less decided, never Invalid.
enum class Slot : uint8_t { Unknown, Party, Box, Standalone };

/// The caller's container state for the bytes (save block checksum, signature, hash).
/// Unchecked or Failed leaves source-game coverage Partial; it never invalidates the
/// Pokemon itself, because a damaged container says nothing about the record's history.
enum class Integrity : uint8_t { Unchecked, Verified, Failed };

/// Optional save-side trainer. A Pokemon from another trainer is normal (trades); the
/// only finding is a Warning when the IDs match the save but the OT name or gender does
/// not (suspicious provenance).
struct Trainer {
    bool present = false;
    uint16_t tid = 0, sid = 0;
    std::array<uint16_t, 13> ot{};  // UTF-16 units as stored, 0x0000-terminated
    bool female = false;
};

struct Input {
    Format format = Format::Unknown;
    Game game = Game::Unknown;
    Slot slot = Slot::Unknown;
    /// True for stored bytes (encrypted and shuffled); false for decrypted bytes.
    bool encrypted = true;
    Integrity integrity = Integrity::Unchecked;
    Trainer trainer{};
};

struct FormatProfile {
    Format format;
    std::string_view name;
    uint8_t generation;
    size_t storedSize;  // box bytes
    size_t partySize;   // party bytes (equal to storedSize where the format has no party tail)
    bool rulesImplemented;
};

/// PKHeX SIZE_STORED / SIZE_PARTY for every modern format; nullptr for Unknown.
const FormatProfile* profile(Format format) noexcept;

/// Native format of a source game (X -> PK6, US -> PK7, ...); Unknown for Game::Unknown.
Format nativeFormat(Game game) noexcept;

/// Generation of a game's native format; 0 for Game::Unknown.
uint8_t generationOf(Game game) noexcept;

/// Analyze one modern record. See the header comment for the contract.
Report analyze(std::span<const uint8_t> record, const Input& input);

} // namespace Legality::Modern
