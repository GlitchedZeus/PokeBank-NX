#ifndef UTILS_DURABLE_FILE_H
#define UTILS_DURABLE_FILE_H

#include <cstdint>
#include <functional>
#include <span>
#include <string>

namespace PokeBank::Storage::DurableFile {

using Validator = std::function<bool(std::span<const uint8_t>, std::string&)>;

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
