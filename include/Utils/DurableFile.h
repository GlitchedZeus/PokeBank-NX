#ifndef UTILS_DURABLE_FILE_H
#define UTILS_DURABLE_FILE_H

#include <cstdint>
#include <functional>
#include <span>
#include <string>
#include <string_view>

namespace PokeBank::Storage::DurableFile {

using Validator = std::function<bool(std::span<const uint8_t>, std::string&)>;

// Audit-only visibility into the real durable-replacement pipeline. The hook is dormant unless
// explicitly installed and, on Switch, installation is accepted only for the PokeBank-owned
// /audit/physical subtree. Production callers never need to know about these checkpoints.
enum class AuditCheckpoint : uint8_t {
    BeforeTempOpen,
    AfterTempOpen,
    BeforeTempWrite,
    AfterTempWrite,
    BeforeTempFlush,
    AfterTempFlush,
    BeforeTempFsync,
    AfterTempFsync,
    AfterTempClose,
    BeforePreviousPreserve,
    AfterPreviousPreserve,
    BeforePromote,
    AfterPromote,
    BeforePromotedRead,
    AfterPromotedRead,
    AfterPromotedValidate,
    DuringFailurePreserve,
    DuringRollback,
    DuringCleanup,
};

using AuditHook = std::function<void(AuditCheckpoint,
                                     std::string_view target,
                                     std::string_view workingPath)>;

/// Installs a process-local audit hook. On Switch, allowedRoot must be under
/// sdmc:/switch/PokeBank-NX/audit/physical; ordinary runtime roots are rejected.
/// Host tests may use /tmp. Returns false if the root is not audit-safe.
bool installAuditHook(std::string allowedRoot, AuditHook hook);
void clearAuditHook() noexcept;
const char* auditCheckpointName(AuditCheckpoint checkpoint) noexcept;

struct RecoveryResult {
    bool ok = false;
    bool restored = false;
    std::string error;
    std::string restoredFrom;
};

/// Conservative restart repair for the narrow crash window where the authoritative filename is
/// missing after its old generation was preserved but before the new temp generation was promoted.
/// Only a previously preserved generation that rereads and passes the caller's validator may be
/// restored. Temp/failed candidates are never promoted by this helper.
RecoveryResult recoverMissingTarget(const std::string& target, const Validator& validator);

struct Result {
    bool ok = false;
    std::string error;
    std::string previousPath;
    std::string failedPath;
};

/// Destination-first durable single-file replacement.
///
/// Contract:
/// - authoritative target is untouched until a fully-written temp image rereads + validates;
/// - an existing target is moved to a unique previous generation before promotion;
/// - promoted bytes are reread + validated;
/// - a failed promoted image is preserved separately and the exact previous generation is restored;
/// - no existing recovery generation is overwritten.
///
/// Directory-entry durability on the Switch SD filesystem remains a separate hardware gate.
/// This primitive checks file flush/fsync/close and all rename/read/validation results.
Result replace(const std::string& target,
               std::span<const uint8_t> bytes,
               const Validator& validator);

} // namespace PokeBank::Storage::DurableFile

#endif
