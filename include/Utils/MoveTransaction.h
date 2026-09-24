#ifndef UTILS_MOVE_TRANSACTION_H
#define UTILS_MOVE_TRANSACTION_H

#include <array>
#include <cstdint>
#include <functional>
#include <span>
#include <string>
#include <vector>

#include "Utils/DurableFile.h"

namespace PokeBank::Storage::MoveTx {

inline constexpr uint16_t kSchemaVersion = 1;
inline constexpr size_t kMaxMoves = 512;

using Digest = std::array<uint8_t, 32>;

Digest sha256(std::span<const uint8_t> bytes) noexcept;
std::string digestHex(const Digest& digest);

enum class State : uint8_t {
    Prepared = 1,
    DestinationWritten = 2,
    DestinationVerified = 3,
    SourceRetirePending = 4,
    SourceRetired = 5,
    Committed = 6,
};

enum class StoreType : uint8_t {
    Bank = 1,
    MutableWorkspaceSingleFile = 2,
    MutableWorkspaceFileSet = 3, // reserved for future BDSP/file-set integration
};

struct StoreDescriptor {
    StoreType type = StoreType::Bank;
    std::string profile;
    std::string gameId;
    std::string workspace;
    std::string fileId;

    bool valid() const noexcept;
    bool operator==(const StoreDescriptor& other) const noexcept;
    bool operator!=(const StoreDescriptor& other) const noexcept { return !(*this == other); }
};

struct MoveRecord {
    std::string sourceSlot;
    std::string destinationSlot;
    Digest sourcePayload{};
    Digest destinationPayload{};
    uint16_t species = 0;
    uint8_t form = 0;
};

struct Transaction {
    uint16_t schemaVersion = kSchemaVersion;
    std::string id;
    State state = State::Prepared;
    StoreDescriptor source;
    StoreDescriptor destination;
    Digest sourceBefore{};
    Digest sourceRetired{};
    Digest destinationBefore{};
    Digest destinationAfter{};
    std::vector<MoveRecord> moves;
};

enum class LoadStatus : uint8_t {
    Ok,
    NotFound,
    Corrupt,
    UnsupportedVersion,
};

struct LoadResult {
    LoadStatus status = LoadStatus::NotFound;
    Transaction transaction;
    std::string error;
};

struct ScanEntry {
    std::string id;
    LoadStatus status = LoadStatus::NotFound;
    State state = State::Prepared;
    std::string error;
};

class Journal {
public:
    explicit Journal(std::string root);

    bool ensure(std::string& error) const;
    std::string allocateTransactionId(std::string& error) const;

    std::string journalPath(const std::string& id) const;
    std::string destinationEvidencePath(const std::string& id) const;
    std::string sourceRetiredEvidencePath(const std::string& id) const;

    bool idAvailable(const std::string& id) const;
    bool persist(const Transaction& transaction, std::string& error) const;
    LoadResult load(const std::string& id) const;
    std::vector<ScanEntry> scan() const;

    bool storeEvidence(const Transaction& transaction,
                       std::span<const uint8_t> destinationAfter,
                       std::span<const uint8_t> sourceRetired,
                       std::string& error) const;
    bool loadEvidence(const Transaction& transaction,
                      std::vector<uint8_t>& destinationAfter,
                      std::vector<uint8_t>& sourceRetired,
                      std::string& error) const;

private:
    std::string root_;
    std::string recordsRoot_;
};

class Store {
public:
    virtual ~Store() = default;
    virtual const StoreDescriptor& descriptor() const noexcept = 0;
    virtual bool read(std::vector<uint8_t>& out, std::string& error) const = 0;
    virtual bool validate(std::span<const uint8_t> bytes, std::string& error) const = 0;
    virtual bool replace(std::span<const uint8_t> bytes, std::string& error) = 0;
};

enum class FaultPoint : uint8_t {
    None,
    AfterPreparedJournal,
    AfterDestinationWrite,
    AfterDestinationVerify,
    AfterDestinationVerifiedJournal,
    BeforeSourceRetire,
    AfterSourceWrite,
    AfterSourceVerify,
    AfterSourceRetiredJournal,
    BeforeCommitted,
};

enum class RecoveryStatus : uint8_t {
    Committed,
    Interrupted,
    Conflict,
    Corrupt,
    UnsupportedVersion,
    Failed,
};

struct RecoveryResult {
    RecoveryStatus status = RecoveryStatus::Failed;
    State state = State::Prepared;
    std::string error;
};

using PersistGate = std::function<bool(State)>;

class Engine {
public:
    explicit Engine(std::string root, PersistGate persistGate = {});

    Journal& journal() noexcept { return journal_; }
    const Journal& journal() const noexcept { return journal_; }

    bool prepare(Transaction& transaction,
                 Store& source,
                 Store& destination,
                 std::span<const uint8_t> destinationAfter,
                 std::span<const uint8_t> sourceRetired,
                 std::string& error);

    RecoveryResult recover(const std::string& transactionId,
                           Store& source,
                           Store& destination,
                           FaultPoint fault = FaultPoint::None);

private:
    bool persistState(Transaction& transaction, State next, std::string& error);

    Journal journal_;
    PersistGate persistGate_;
};

} // namespace PokeBank::Storage::MoveTx

#endif
