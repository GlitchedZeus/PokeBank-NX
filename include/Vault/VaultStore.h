#pragma once
// Master Vault v1 storage core (docs/roadmap/VAULT_V1_IMPLEMENTATION_PLAN.md, MASTER_VAULT_SPEC.md).
//
// A PokeBank-owned, game-independent store of NATIVE Gen I-V Pokemon records. It is not a save and
// never touches one: deposit takes the exact native record a save's staged editor exports (the same
// .pk1-.pk5 plaintext bytes Export to file writes), withdraw hands back a decoded copy for the same
// generation's staged import path, which keeps every capacity / occupied-slot refusal. The Vault
// record stays in the Vault (withdraw is a COPY).
//
// On-disk layout (root = sdmc:/switch/PokeBank-NX/vault):
//   records/<vault-id>.pbvr   one self-verifying record per entity: header, metadata, the raw native
//                             bytes, SHA-256 of the raw bytes and SHA-256 of the whole file. The
//                             records are AUTHORITATIVE; nothing else is needed to rebuild the Vault.
//   index.pbvi                a metadata cache (the records' metadata, in deposit order, SHA-256
//                             trailer). Corrupt or missing -> quarantined and rebuilt from records/.
//   journal/<vault-id>.pending  written before a deposit publishes anything, removed after commit.
//                             On open a pending deposit is rolled forward only when its record
//                             re-reads and verifies, otherwise rolled back.
//   quarantine/               anything the Vault refused to trust (never deleted).
// Every file write goes through Utils/DurableFile (temp, reread, validate, promote, reread).
//
// Vault IDs are random 128-bit identifiers (UUID v4 text), independent of species, slot, file
// name and hash. No exceptions, no RTTI: every failure is a return value with a reason.

#include "PokemonFile/NativePokemonFile.h"
#include "Utils/DurableFile.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace PokeBank::Vault {

inline constexpr uint32_t kSchemaVersion = 1;
inline constexpr std::size_t kMaxRecordFileBytes = 64 * 1024;       // a record is < 1 KiB
inline constexpr std::size_t kMaxIndexFileBytes = 32 * 1024 * 1024; // ~80k entries
inline constexpr std::string_view kRecordExtension = ".pbvr";
inline constexpr std::string_view kJournalExtension = ".pending";
inline constexpr std::string_view kIndexName = "index.pbvi";

struct VaultId {
    std::array<uint8_t, 16> bytes{};
    [[nodiscard]] std::string text() const;   // 8-4-4-4-12 lower-case hex
    [[nodiscard]] static std::optional<VaultId> parse(std::string_view text);
    friend bool operator==(const VaultId&, const VaultId&) = default;
};

using Sha256 = std::array<uint8_t, 32>;
[[nodiscard]] Sha256 sha256(std::span<const uint8_t> bytes);
[[nodiscard]] std::string hex(std::span<const uint8_t> bytes);

// Display cache for browsing/search. NOT authoritative: the raw record is.
struct Summary {
    uint16_t species = 0;
    std::string speciesName;
    std::string nickname;
    std::string otName;
    uint8_t level = 0;
    bool shiny = false;
    bool egg = false;
};

// Where the deposited copy came from. Never a credential, never a full personal path.
struct Provenance {
    std::string sourceKind = "game_save";   // game_save | pokemon_file
    std::string sourceGame;                 // "FireRed", "Generation III", ...
    std::string sourceLabel;                // the save's display name / file name (hint only)
    int sourceBox = -1;                     // zero-based; -1 = party / unknown
    int sourceSlot = -1;
    bool sourceParty = false;
    bool stagedEdits = false;               // copied from a workspace with unsaved staged changes
    int64_t depositedAt = 0;                // unix seconds (0 = clock unavailable)
    std::string appVersion;
};

struct DepositRequest {
    int generation = 0;              // 1..5
    std::vector<uint8_t> raw;        // native plaintext record (what Export to file writes)
    Summary summary;
    Provenance provenance;
};

struct Entry {
    VaultId id;
    uint64_t sequence = 0;           // deposit order, never reused
    int generation = 0;
    std::string format;              // "pk1".."pk5"
    bool party = false;
    bool japanese = false;
    uint32_t rawSize = 0;
    Sha256 rawSha256{};
    std::string canonicalization = "pkhex-plaintext";   // what the raw bytes are, recorded not hidden
    Summary summary;
    Provenance provenance;
};

struct Record {
    Entry entry;
    std::vector<uint8_t> raw;
};

// File-system seam. defaultIo() is the real SD card / host file system (DurableFile + POSIX);
// tests wrap it to inject full-SD, rename and read faults.
struct Io {
    std::function<Storage::DurableFile::Result(const std::string&, std::span<const uint8_t>,
                                               const Storage::DurableFile::Validator&)> replace;
    // Reads at most maxBytes; a larger file is refused (false).
    std::function<bool(const std::string&, std::size_t maxBytes, std::vector<uint8_t>&, std::string&)> read;
    std::function<bool(const std::string& from, const std::string& to)> rename;
    std::function<bool(const std::string&)> remove;
    std::function<bool(const std::string& dir, std::vector<std::string>& names)> list;
    std::function<bool(const std::string&)> exists;
    std::function<bool(const std::string&, std::string*)> ensureDir;
    std::function<bool(uint8_t*, std::size_t)> random;
    std::function<int64_t()> now;
};
[[nodiscard]] Io defaultIo();

struct OpenReport {
    bool createdNew = false;
    bool indexRebuilt = false;        // the index was missing/corrupt and rebuilt from records/
    std::size_t rolledForward = 0;    // interrupted deposits completed from a verified record
    std::size_t rolledBack = 0;       // interrupted deposits that never published a valid record
    std::size_t quarantined = 0;      // files moved to quarantine/
    std::size_t missingRecords = 0;   // indexed entries whose record file is gone (not listed)
    bool readOnly = false;            // a newer schema: browse only, never written
    std::vector<std::string> notes;
};

// Points at which a test may "pull the plug" (the operation stops at once, no cleanup).
enum class Step : uint8_t { JournalWritten, RecordPublished, IndexPublished };

class Store {
public:
    explicit Store(Io io = defaultIo());

    // Creates the folders when missing, recovers interrupted deposits, loads (or rebuilds) the index.
    bool open(const std::string& root, OpenReport* report, std::string& error);
    [[nodiscard]] bool isOpen() const noexcept { return opened; }
    [[nodiscard]] bool readOnly() const noexcept { return readOnlyMode; }
    [[nodiscard]] const std::string& root() const noexcept { return rootPath; }

    // Deposit order. Metadata only: no record file is read to list the Vault.
    [[nodiscard]] const std::vector<Entry>& entries() const noexcept { return list; }
    [[nodiscard]] const Entry* find(const VaultId& id) const noexcept;

    // Copies one native record into the Vault under a NEW Vault ID. The bytes must decode as a
    // plaintext record of `generation` (PokemonFile::decode) and are stored exactly.
    std::optional<VaultId> deposit(const DepositRequest& request, std::string& error);

    // Reads one record back and verifies both SHA-256 values and the metadata against the index.
    [[nodiscard]] std::optional<Record> read(const VaultId& id, std::string& error) const;

    // Withdraw-as-copy: the verified record decoded for the SAME generation's staged import. The
    // Vault is not changed. A record of another generation is refused (no conversion in v1).
    [[nodiscard]] std::optional<PokemonFile::Decoded> withdrawCopy(const VaultId& id, int targetGeneration,
                                                                   std::string& error) const;

    // Test seam: return true from the hook to stop an operation dead at that step.
    void setCrashHookForTesting(std::function<bool(Step)> hook) { crashHook = std::move(hook); }

    // Serialisation, exposed for tests and recovery tools.
    [[nodiscard]] static std::vector<uint8_t> encodeRecord(const Entry& entry, std::span<const uint8_t> raw);
    [[nodiscard]] static std::optional<Record> decodeRecord(std::span<const uint8_t> file, std::string& error);
    [[nodiscard]] static std::vector<uint8_t> encodeIndex(const std::vector<Entry>& entries, uint64_t nextSequence);
    [[nodiscard]] static bool decodeIndex(std::span<const uint8_t> file, std::vector<Entry>& entries,
                                          uint64_t& nextSequence, uint32_t& schema, std::string& error);

    [[nodiscard]] std::string recordPath(const VaultId& id) const;
    [[nodiscard]] std::string journalPath(const VaultId& id) const;
    [[nodiscard]] std::string indexPath() const;
    [[nodiscard]] std::string quarantineDir() const;

private:
    bool crashed(Step step) const { return crashHook && crashHook(step); }
    bool writeIndex(const std::vector<Entry>& entries, uint64_t nextSeq, std::string& error);
    bool quarantine(const std::string& path, std::string_view why, OpenReport* report);
    bool rebuildFromRecords(std::vector<Entry>& out, uint64_t& nextSeq, OpenReport* report);
    std::optional<VaultId> newId();
    void pruneIndexGenerations();

    Io io;
    std::function<bool(Step)> crashHook;
    std::string rootPath;
    std::vector<Entry> list;
    uint64_t nextSequence = 1;
    bool opened = false;
    bool readOnlyMode = false;
};

}  // namespace PokeBank::Vault
