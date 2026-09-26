#include "Utils/DurableFile.h"

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <utility>
#include <vector>

namespace PokeBank::Storage::DurableFile {
namespace {

std::string gAuditRoot;
AuditHook gAuditHook;

bool pathWithin(std::string_view path, std::string_view root) noexcept {
    if (root.empty() || path.size() < root.size() || path.substr(0, root.size()) != root) return false;
    return path.size() == root.size() || path[root.size()] == '/';
}

bool auditRootAllowed(std::string_view root) noexcept {
#ifdef __SWITCH__
    constexpr std::string_view prefix = "sdmc:/switch/PokeBank-NX/audit/physical";
    return pathWithin(root, prefix);
#else
    return root == "/tmp" || root.rfind("/tmp/", 0) == 0;
#endif
}

void audit(AuditCheckpoint checkpoint,
           std::string_view target,
           std::string_view workingPath = {}) {
    if (!gAuditHook || !pathWithin(target, gAuditRoot)) return;
    gAuditHook(checkpoint, target, workingPath);
}

std::string uniqueSibling(const std::string& target, const char* suffix) {
    for (uint64_t generation = 0; generation < 1000000; ++generation) {
        const std::string candidate =
            target + suffix + std::to_string(generation);
        struct stat st{};
        errno = 0;
        if (stat(candidate.c_str(), &st) != 0) {
            if (errno == ENOENT) return candidate;
            return {};
        }
    }
    return {};
}

bool readExact(const std::string& path, std::vector<uint8_t>& out, std::string& error) {
    FILE* f = std::fopen(path.c_str(), "rb");
    if (!f) {
        error = "open for verification failed: " + std::string(std::strerror(errno));
        return false;
    }
    if (std::fseek(f, 0, SEEK_END) != 0) {
        error = "seek failed";
        std::fclose(f);
        return false;
    }
    const long size = std::ftell(f);
    if (size < 0 || std::fseek(f, 0, SEEK_SET) != 0) {
        error = "size/rewind failed";
        std::fclose(f);
        return false;
    }
    out.assign(static_cast<size_t>(size), 0);
    const size_t got = out.empty() ? 0 : std::fread(out.data(), 1, out.size(), f);
    const bool closeOk = std::fclose(f) == 0;
    if (got != out.size()) {
        error = "verification read was short";
        return false;
    }
    if (!closeOk) {
        error = "verification close failed";
        return false;
    }
    return true;
}

bool writeTemp(const std::string& target,
               const std::string& path,
               std::span<const uint8_t> bytes,
               std::string& error) {
    audit(AuditCheckpoint::BeforeTempOpen, target, path);
    FILE* f = std::fopen(path.c_str(), "wb");
    if (!f) {
        error = "temp open failed: " + std::string(std::strerror(errno));
        return false;
    }
    audit(AuditCheckpoint::AfterTempOpen, target, path);

    audit(AuditCheckpoint::BeforeTempWrite, target, path);
    const size_t written = bytes.empty() ? 0 : std::fwrite(bytes.data(), 1, bytes.size(), f);
    audit(AuditCheckpoint::AfterTempWrite, target, path);
    bool ok = written == bytes.size();
    if (!ok) error = "temp write was short";

    audit(AuditCheckpoint::BeforeTempFlush, target, path);
    if (ok && std::fflush(f) != 0) {
        ok = false;
        error = "temp fflush failed";
    }
    audit(AuditCheckpoint::AfterTempFlush, target, path);

    audit(AuditCheckpoint::BeforeTempFsync, target, path);
    if (ok && ::fsync(fileno(f)) != 0) {
        ok = false;
        error = "temp fsync failed";
    }
    audit(AuditCheckpoint::AfterTempFsync, target, path);

    if (std::fclose(f) != 0 && ok) {
        ok = false;
        error = "temp fclose failed";
    }
    audit(AuditCheckpoint::AfterTempClose, target, path);
    return ok;
}

bool bytesEqual(std::span<const uint8_t> expected, const std::vector<uint8_t>& actual) {
    return expected.size() == actual.size() &&
           std::equal(expected.begin(), expected.end(), actual.begin());
}

} // namespace

bool installAuditHook(std::string allowedRoot, AuditHook hook) {
    if (!auditRootAllowed(allowedRoot) || !hook) return false;
    while (allowedRoot.size() > 1 && allowedRoot.back() == '/') allowedRoot.pop_back();
    gAuditRoot = std::move(allowedRoot);
    gAuditHook = std::move(hook);
    return true;
}

void clearAuditHook() noexcept {
    gAuditHook = {};
    gAuditRoot.clear();
}

const char* auditCheckpointName(AuditCheckpoint checkpoint) noexcept {
    switch (checkpoint) {
        case AuditCheckpoint::BeforeTempOpen: return "BEFORE_TEMP_OPEN";
        case AuditCheckpoint::AfterTempOpen: return "AFTER_TEMP_OPEN";
        case AuditCheckpoint::BeforeTempWrite: return "BEFORE_TEMP_WRITE";
        case AuditCheckpoint::AfterTempWrite: return "AFTER_TEMP_WRITE";
        case AuditCheckpoint::BeforeTempFlush: return "BEFORE_TEMP_FLUSH";
        case AuditCheckpoint::AfterTempFlush: return "AFTER_TEMP_FLUSH";
        case AuditCheckpoint::BeforeTempFsync: return "BEFORE_TEMP_FSYNC";
        case AuditCheckpoint::AfterTempFsync: return "AFTER_TEMP_FSYNC";
        case AuditCheckpoint::AfterTempClose: return "AFTER_TEMP_CLOSE";
        case AuditCheckpoint::BeforePreviousPreserve: return "BEFORE_PREVIOUS_PRESERVE";
        case AuditCheckpoint::AfterPreviousPreserve: return "AFTER_PREVIOUS_PRESERVE";
        case AuditCheckpoint::BeforePromote: return "BEFORE_PROMOTE";
        case AuditCheckpoint::AfterPromote: return "AFTER_PROMOTE";
        case AuditCheckpoint::BeforePromotedRead: return "BEFORE_PROMOTED_READ";
        case AuditCheckpoint::AfterPromotedRead: return "AFTER_PROMOTED_READ";
        case AuditCheckpoint::AfterPromotedValidate: return "AFTER_PROMOTED_VALIDATE";
        case AuditCheckpoint::DuringFailurePreserve: return "DURING_FAILURE_PRESERVE";
        case AuditCheckpoint::DuringRollback: return "DURING_ROLLBACK";
        case AuditCheckpoint::DuringCleanup: return "DURING_CLEANUP";
    }
    return "UNKNOWN";
}

RecoveryResult recoverMissingTarget(const std::string& target, const Validator& validator) {
    RecoveryResult out;

    struct stat current{};
    errno = 0;
    if (::stat(target.c_str(), &current) == 0) {
        out.ok = S_ISREG(current.st_mode);
        if (!out.ok) out.error = "durable target exists but is not a regular file";
        return out;
    }
    if (errno != ENOENT) {
        out.error = "could not inspect missing durable target";
        return out;
    }

    const size_t slash = target.find_last_of('/');
    const std::string directory = slash == std::string::npos ? "." : target.substr(0, slash);
    const std::string basename = slash == std::string::npos ? target : target.substr(slash + 1);
    const std::string prefix = basename + ".previous.";

    DIR* dir = ::opendir(directory.c_str());
    if (!dir) {
        out.error = "could not inspect durable recovery generations";
        return out;
    }

    std::vector<std::pair<uint64_t, std::string>> candidates;
    while (dirent* ent = ::readdir(dir)) {
        const std::string name = ent->d_name;
        if (name.rfind(prefix, 0) != 0) continue;
        const std::string suffix = name.substr(prefix.size());
        if (suffix.empty() || !std::all_of(suffix.begin(), suffix.end(),
                                            [](unsigned char ch) { return ch >= '0' && ch <= '9'; }))
            continue;
        uint64_t generation = 0;
        for (const char ch : suffix) {
            generation = generation * 10u + static_cast<uint64_t>(ch - '0');
            if (generation >= 1000000u) break;
        }
        if (generation >= 1000000u) continue;
        const std::string path = directory + "/" + name;
        struct stat st{};
        if (::stat(path.c_str(), &st) == 0 && S_ISREG(st.st_mode))
            candidates.emplace_back(generation, path);
    }
    ::closedir(dir);

    if (candidates.empty()) {
        // A missing target can be a legitimate semantic state (for example a Bank that has never
        // existed). The caller decides whether that is allowed.
        out.ok = true;
        return out;
    }

    std::sort(candidates.begin(), candidates.end(),
              [](const auto& a, const auto& b) { return a.first > b.first; });

    for (const auto& candidate : candidates) {
        std::vector<uint8_t> bytes;
        std::string readError;
        if (!readExact(candidate.second, bytes, readError)) continue;

        std::string validationError;
        if (!validator(std::span<const uint8_t>(bytes.data(), bytes.size()), validationError))
            continue;

        audit(AuditCheckpoint::DuringRollback, target, candidate.second);
        if (::rename(candidate.second.c_str(), target.c_str()) != 0) {
            out.error = "validated previous generation could not be restored";
            return out;
        }

        std::vector<uint8_t> reread;
        if (!readExact(target, reread, out.error)) return out;
        validationError.clear();
        if (!validator(std::span<const uint8_t>(reread.data(), reread.size()), validationError)) {
            out.error = validationError.empty()
                ? "restored previous generation failed validation"
                : validationError;
            return out;
        }

        out.ok = true;
        out.restored = true;
        out.restoredFrom = candidate.second;
        return out;
    }

    out.error = "durable target is missing and no preserved previous generation validates";
    return out;
}

Result replace(const std::string& target,
               std::span<const uint8_t> bytes,
               const Validator& validator) {
    Result result;

    const std::string temp = uniqueSibling(target, ".tmp.");
    if (temp.empty()) {
        result.error = "could not allocate a unique temp generation";
        return result;
    }

    if (!writeTemp(target, temp, bytes, result.error)) {
        audit(AuditCheckpoint::DuringCleanup, target, temp);
        std::remove(temp.c_str());
        return result;
    }

    std::vector<uint8_t> reread;
    if (!readExact(temp, reread, result.error) || !bytesEqual(bytes, reread)) {
        if (result.error.empty()) result.error = "temp reread differs from requested bytes";
        result.failedPath = uniqueSibling(target, ".failed.");
        audit(AuditCheckpoint::DuringFailurePreserve, target, temp);
        if (!result.failedPath.empty())
            std::rename(temp.c_str(), result.failedPath.c_str());
        else {
            audit(AuditCheckpoint::DuringCleanup, target, temp);
            std::remove(temp.c_str());
        }
        return result;
    }

    std::string validationError;
    if (!validator(std::span<const uint8_t>(reread.data(), reread.size()), validationError)) {
        result.error = validationError.empty() ? "temp validation failed" : validationError;
        result.failedPath = uniqueSibling(target, ".failed.");
        if (!result.failedPath.empty())
            std::rename(temp.c_str(), result.failedPath.c_str());
        else
            std::remove(temp.c_str());
        return result;
    }

    struct stat current{};
    errno = 0;
    const bool hadTarget = stat(target.c_str(), &current) == 0;
    if (!hadTarget && errno != ENOENT) {
        result.error = "could not inspect current target";
        std::remove(temp.c_str());
        return result;
    }

    if (hadTarget) {
        result.previousPath = uniqueSibling(target, ".previous.");
        if (result.previousPath.empty()) {
            result.error = "could not allocate a unique previous generation";
            std::remove(temp.c_str());
            return result;
        }
        audit(AuditCheckpoint::BeforePreviousPreserve, target, result.previousPath);
        if (std::rename(target.c_str(), result.previousPath.c_str()) != 0) {
            result.error = "could not preserve current target before promotion";
            std::remove(temp.c_str());
            result.previousPath.clear();
            return result;
        }
        audit(AuditCheckpoint::AfterPreviousPreserve, target, result.previousPath);
    }

    audit(AuditCheckpoint::BeforePromote, target, temp);
    if (std::rename(temp.c_str(), target.c_str()) != 0) {
        result.error = "temp promotion failed";
        if (hadTarget) {
            audit(AuditCheckpoint::DuringRollback, target, result.previousPath);
            std::rename(result.previousPath.c_str(), target.c_str());
        }
        return result;
    }
    audit(AuditCheckpoint::AfterPromote, target, target);

    reread.clear();
    validationError.clear();
    audit(AuditCheckpoint::BeforePromotedRead, target, target);
    const bool promotedReadOk = readExact(target, reread, result.error);
    audit(AuditCheckpoint::AfterPromotedRead, target, target);
    const bool promotedBytesOk = promotedReadOk && bytesEqual(bytes, reread);
    const bool promotedValidationOk =
        promotedBytesOk &&
        validator(std::span<const uint8_t>(reread.data(), reread.size()), validationError);
    if (promotedReadOk && promotedBytesOk && promotedValidationOk)
        audit(AuditCheckpoint::AfterPromotedValidate, target, target);

    if (!promotedReadOk || !promotedBytesOk || !promotedValidationOk) {
        if (result.error.empty()) {
            if (!promotedBytesOk) result.error = "promoted target differs from requested bytes";
            else result.error = validationError.empty() ? "promoted validation failed" : validationError;
        }

        result.failedPath = uniqueSibling(target, ".failed.");
        bool failedPreserved = false;
        audit(AuditCheckpoint::DuringFailurePreserve, target, target);
        if (!result.failedPath.empty())
            failedPreserved = std::rename(target.c_str(), result.failedPath.c_str()) == 0;

        if (!failedPreserved) {
            // Never overwrite a questionable promoted target just to make rollback look successful.
            result.error += "; failed promoted target could not be preserved";
            return result;
        }

        if (hadTarget) audit(AuditCheckpoint::DuringRollback, target, result.previousPath);
        if (hadTarget && std::rename(result.previousPath.c_str(), target.c_str()) != 0) {
            result.error += "; previous generation could not be restored";
            return result;
        }
        return result;
    }

    result.ok = true;
    return result;
}

} // namespace PokeBank::Storage::DurableFile
