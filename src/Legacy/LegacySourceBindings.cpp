#include "Legacy/LegacySourceBindings.h"

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <sys/stat.h>
#include <utility>
#include <vector>
#ifdef __SWITCH__
#include <switch/result.h>
extern "C" Result fsdevGetLastResult(void) __attribute__((weak));
#endif

namespace PokeVault::Legacy {
    namespace {
        using Owners = std::unordered_map<std::string, BindingRecord>;
        constexpr const char* Header = "# PokeBank NX legacy source bindings v1\n";
        enum class ReadStatus { Missing, Valid, Invalid };
        char hexDigit(unsigned value) noexcept {
            return static_cast<char>(value < 10 ? '0' + value : 'a' + value - 10);
        }

        int hexValue(char value) noexcept {
            if (value >= '0' && value <= '9') return value - '0';
            if (value >= 'a' && value <= 'f') return value - 'a' + 10;
            if (value >= 'A' && value <= 'F') return value - 'A' + 10;
            return -1;
        }

        std::string hexEncode(std::string_view value) {
            std::string result;
            result.reserve(value.size() * 2);
            for (unsigned char byte : value) {
                result.push_back(hexDigit(byte >> 4));
                result.push_back(hexDigit(byte & 0x0F));
            }
            return result;
        }

        bool hexDecode(std::string_view value, std::string& output) {
            if ((value.size() & 1) != 0) return false;
            output.clear();
            output.reserve(value.size() / 2);
            for (size_t index = 0; index < value.size(); index += 2) {
                const int high = hexValue(value[index]);
                const int low = hexValue(value[index + 1]);
                if (high < 0 || low < 0) return false;
                output.push_back(static_cast<char>((high << 4) | low));
            }
            return !output.empty();
        }
    }

    namespace {
        // Bounded, strict read: never expose a partially parsed ownership database.
        ReadStatus readDatabase(const std::string& path, Owners& owners, std::string& bytes) {
            owners.clear();
            bytes.clear();
            errno = 0;
            FILE* file = std::fopen(path.c_str(), "rb");
            if (!file) return errno == ENOENT ? ReadStatus::Missing : ReadStatus::Invalid;
            char buffer[4096];
            bool ok = true;
            size_t count;
            while ((count = std::fread(buffer, 1, sizeof(buffer), file)) != 0) {
                if (bytes.size() + count > 1024 * 1024) { ok = false; errno = EFBIG; break; }
                bytes.append(buffer, count);
            }
            if (std::ferror(file)) ok = false;
            int savedErrno = errno;
            if (std::fclose(file) != 0) { ok = false; savedErrno = errno; }
            if (!ok) { errno = savedErrno ? savedErrno : EIO; return ReadStatus::Invalid; }
            if (!bytes.starts_with(Header) || bytes.back() != '\n') {
                errno = EILSEQ; return ReadStatus::Invalid;
            }
            size_t offset = std::strlen(Header);
            while (offset < bytes.size()) {
                const size_t end = bytes.find('\n', offset);
                const std::string_view row(bytes.data() + offset, end - offset);
                std::vector<std::string> fields;
                size_t start = 0;
                bool decoded = true;
                do {
                    const size_t tab = row.find('\t', start);
                    std::string field;
                    if (!hexDecode(row.substr(start, tab == std::string_view::npos
                        ? row.size() - start : tab - start), field) ||
                        field.find('\0') != std::string::npos) { decoded = false; break; }
                    fields.push_back(std::move(field));
                    if (tab == std::string_view::npos) break;
                    start = tab + 1;
                } while (fields.size() <= 6);
                if (!decoded || (fields.size() != 2 && fields.size() != 3 && fields.size() != 6)) {
                    owners.clear(); errno = EILSEQ; return ReadStatus::Invalid;
                }
                BindingRecord binding;
                binding.profileIdentity = fields[1];
                if (fields.size() >= 3) binding.gameIdentity = fields[2];
                if (fields.size() == 6) {
                    binding.sourcePath = fields[3];
                    binding.sourceType = fields[4];
                    binding.expectedRawFamily = fields[5];
                }
                if (!owners.emplace(fields[0], std::move(binding)).second) {
                    owners.clear(); errno = EILSEQ; return ReadStatus::Invalid;
                }
                offset = end + 1;
            }
            return ReadStatus::Valid;
        }

        std::string serialize(const Owners& owners) {
            std::vector<std::pair<std::string, BindingRecord>> ordered(owners.begin(), owners.end());
            std::sort(ordered.begin(), ordered.end(),
                      [](const auto& a, const auto& b) { return a.first < b.first; });
            std::string bytes = Header;
            for (const auto& [source, binding] : ordered) {
                bytes += hexEncode(source) + "\t" + hexEncode(binding.profileIdentity);
                if (!binding.gameIdentity.empty())
                    bytes += "\t" + hexEncode(binding.gameIdentity);
                if (!binding.sourcePath.empty())
                    bytes += "\t" + hexEncode(binding.sourcePath) + "\t" +
                        hexEncode(binding.sourceType) + "\t" + hexEncode(binding.expectedRawFamily);
                bytes += "\n";
            }
            return bytes;
        }
    }

    LegacySourceBindings::LegacySourceBindings(std::string path, BindingFileOps ops)
        : storagePath_(std::move(path)), ops_(ops) {}

    bool LegacySourceBindings::fail(const char* stage) const {
        const int error = errno;
        char diagnostic[256];
#ifdef __SWITCH__
        // Optional richer libnx diagnostic. Keep persistence compatible when this API is absent.
        if (fsdevGetLastResult) {
            const unsigned native = fsdevGetLastResult();
            std::snprintf(diagnostic, sizeof(diagnostic),
                "%s: errno=%d (%s), native_result=0x%08x",
                stage, error, std::strerror(error), native);
        } else {
            std::snprintf(diagnostic, sizeof(diagnostic),
                "%s: errno=%d (%s), native_result=unavailable",
                stage, error, std::strerror(error));
        }
#else
        std::snprintf(diagnostic, sizeof(diagnostic),
            "%s: errno=%d (%s)", stage, error, std::strerror(error));
#endif
        lastError_ = diagnostic;
        return false;
    }

    bool LegacySourceBindings::checkpoint(const char* stage) const {
        errno = 0;
        if (ops_.checkpoint && ops_.checkpoint(stage) != 0) return fail(stage);
        return true;
    }

    bool LegacySourceBindings::load() {
        lastError_.clear();
        if (storagePath_.empty()) { errno = EINVAL; return fail("load-path"); }
        Owners candidate;
        std::string bytes;
        const auto primary = readDatabase(storagePath_, candidate, bytes);
        if (primary == ReadStatus::Valid) { owners_ = std::move(candidate); return true; }
        const int primaryError = errno;
        const auto backup = readDatabase(storagePath_ + ".bak", candidate, bytes);
        if (backup == ReadStatus::Valid) {
            owners_ = std::move(candidate);
            lastError_ = "Recovered bindings from .bak; primary missing or invalid";
            return true;
        }
        if (primary == ReadStatus::Missing && backup == ReadStatus::Missing) {
            owners_.clear(); // A .tmp alone is uncommitted, never automatically promoted.
            return true;
        }
        errno = primary == ReadStatus::Invalid ? primaryError : errno;
        return fail("load-database"); // Preserve previous in-memory state on failure.
    }

    bool LegacySourceBindings::save() const {
        lastError_.clear();
        if (storagePath_.empty()) { errno = EINVAL; return fail("save-path"); }
        const auto renameFile = ops_.renameFile ? ops_.renameFile : std::rename;
        const std::string tmp = storagePath_ + ".tmp";
        const std::string bak = storagePath_ + ".bak";
        Owners previous, checked;
        std::string priorBytes, checkedBytes;
        auto primary = readDatabase(storagePath_, previous, priorBytes);
        if (primary == ReadStatus::Invalid) return fail("read-existing");
        if (primary == ReadStatus::Missing) {
            const auto backup = readDatabase(bak, previous, priorBytes);
            if (backup == ReadStatus::Invalid) return fail("read-recovery-backup");
            if (backup == ReadStatus::Valid) {
                if (!checkpoint("recover-backup")) return false;
                if (renameFile(bak.c_str(), storagePath_.c_str()) != 0)
                    return fail("recover-backup");
                primary = ReadStatus::Valid;
            }
        }
        const std::string bytes = serialize(owners_);
        if (!checkpoint("write-temp")) return false;
        FILE* file = std::fopen(tmp.c_str(), "wb");
        if (!file) return fail("open-temp");
        bool ok = std::fwrite(bytes.data(), 1, bytes.size(), file) == bytes.size();
        if (!ok) fail("write-temp");
        if (ok && std::fflush(file) != 0) { fail("flush-temp"); ok = false; }
        if (ok && ::fsync(::fileno(file)) != 0) { fail("sync-temp"); ok = false; }
        // Every handle is closed, even on earlier failure, before any rename/delete.
        if (std::fclose(file) != 0 && ok) { fail("close-temp"); ok = false; }
        if (!ok) return false; // Existing primary remains valid; .tmp is never authoritative.
        if (!checkpoint("validate-temp")) return false;
        if (readDatabase(tmp, checked, checkedBytes) != ReadStatus::Valid ||
            checkedBytes != bytes) { errno = EILSEQ; return fail("validate-temp"); }

        const bool hasPrevious = primary == ReadStatus::Valid;
        if (hasPrevious) {
            // Only retire an older backup while the fully validated current primary still exists.
            if (!checkpoint("retire-backup")) return false;
            errno = 0;
            if (std::remove(bak.c_str()) != 0 && errno != ENOENT) return fail("retire-backup");
            if (!checkpoint("old-to-backup")) return false;
            if (renameFile(storagePath_.c_str(), bak.c_str()) != 0) return fail("old-to-backup");
        }
        const auto rollback = [&](bool promoted) {
            const std::string originalError = lastError_;
            bool restored = true;
            if (promoted && std::remove(storagePath_.c_str()) != 0) restored = false;
            if (hasPrevious && restored &&
                renameFile(bak.c_str(), storagePath_.c_str()) != 0) restored = false;
            if (!restored) {
                fail("rollback"); lastError_ = originalError + "; " + lastError_ +
                    "; previous database retained at .bak";
            } else lastError_ = originalError;
            return false;
        };
        if (!checkpoint("temp-to-target")) return rollback(false);
        if (renameFile(tmp.c_str(), storagePath_.c_str()) != 0) {
            fail("temp-to-target"); return rollback(false);
        }
        if (!checkpoint("validate-target")) return rollback(true);
        if (readDatabase(storagePath_, checked, checkedBytes) != ReadStatus::Valid ||
            checkedBytes != bytes) {
            errno = EILSEQ; fail("validate-target"); return rollback(true);
        }
        // Retain the verified old primary at .bak after success. On restart a missing/invalid
        // primary can use it; a valid primary always wins over stale .tmp/.bak.
        return true;
    }

    bool LegacySourceBindings::assignAndSave(std::string_view source, std::string_view profile) {
        const auto before = owners_;
        if (!assign(source, profile)) { errno = EINVAL; return fail("assign"); }
        if (save()) return true;
        owners_ = before;
        return false;
    }

    bool LegacySourceBindings::assignAndSave(std::string_view source, std::string_view profile,
                                             std::string_view game) {
        const auto before = owners_;
        if (!assign(source, profile, game)) { errno = EINVAL; return fail("assign"); }
        if (save()) return true;
        owners_ = before;
        return false;
    }

    bool LegacySourceBindings::assign(std::string_view sourceIdentity,
                                      std::string_view profileIdentity) {
        if (sourceIdentity.empty() || profileIdentity.empty()) return false;
        auto [it, inserted] = owners_.try_emplace(
            std::string(sourceIdentity),
            BindingRecord{std::string(profileIdentity), {}, {}, {}, {}});
        if (!inserted) it->second.profileIdentity = profileIdentity;
        return true;
    }

    bool LegacySourceBindings::assign(std::string_view sourceIdentity,
                                      std::string_view profileIdentity,
                                      std::string_view gameIdentity) {
        if (sourceIdentity.empty() || profileIdentity.empty() || gameIdentity.empty()) return false;
        owners_[std::string(sourceIdentity)] =
            BindingRecord{std::string(profileIdentity), std::string(gameIdentity), {}, {}, {}};
        return true;
    }

    bool LegacySourceBindings::assignFileAndSave(std::string_view source, BindingRecord binding) {
        const auto safe = [](std::string_view value) {
            return !value.empty() && value.find('\0') == std::string_view::npos;
        };
        if (!safe(source) || !safe(binding.profileIdentity) || !safe(binding.gameIdentity) ||
            !safe(binding.sourcePath) || !safe(binding.sourceType) || !safe(binding.expectedRawFamily)) {
            errno = EINVAL; return fail("assign-file");
        }
        // Metadata transaction paths must never alias an assigned source save.
        struct stat sourceInfo{};
        const bool sourceExists = ::stat(binding.sourcePath.c_str(), &sourceInfo) == 0;
        for (const auto& metadataPath : {storagePath_, storagePath_ + ".tmp", storagePath_ + ".bak"}) {
            struct stat metadataInfo{};
            const bool sameFile = sourceExists && ::stat(metadataPath.c_str(), &metadataInfo) == 0 &&
                sourceInfo.st_ino != 0 && sourceInfo.st_dev == metadataInfo.st_dev &&
                sourceInfo.st_ino == metadataInfo.st_ino;
            if (binding.sourcePath == metadataPath || sameFile) {
                errno = EINVAL; return fail("assignment-aliases-metadata");
            }
        }
        const auto before = owners_;
        owners_[std::string(source)] = std::move(binding);
        if (save()) return true;
        owners_ = before;
        return false;
    }

    AssignedFile LegacySourceBindings::resolveFileForGame(std::string_view profile,
                                                          std::string_view game) const {
        AssignedFile result;
        for (const auto& [identity, binding] : owners_) {
            if (binding.profileIdentity != profile || binding.gameIdentity != game) continue;
            if (!result.sourceIdentity.empty()) {
                result.status = AssignedFileStatus::Ambiguous;
                return result; // Never pick an arbitrary one, even if one file is missing.
            }
            result.sourceIdentity = identity;
            result.binding = binding;
        }
        if (result.sourceIdentity.empty()) return result;
        if (result.binding.sourcePath.empty()) return result; // Old catalog-only assignment.
        struct stat info{};
        if (::stat(result.binding.sourcePath.c_str(), &info) != 0) {
            result.status = errno == ENOENT || errno == ENOTDIR
                ? AssignedFileStatus::Missing : AssignedFileStatus::Unreadable;
        } else {
            result.status = S_ISREG(info.st_mode) ? AssignedFileStatus::Ready : AssignedFileStatus::Unreadable;
        }
        return result;
    }

    bool LegacySourceBindings::unassign(std::string_view sourceIdentity) {
        return owners_.erase(std::string(sourceIdentity)) != 0;
    }

    bool LegacySourceBindings::isAssigned(std::string_view sourceIdentity) const {
        return owners_.find(std::string(sourceIdentity)) != owners_.end();
    }

    bool LegacySourceBindings::isVisibleTo(std::string_view sourceIdentity,
                                           std::string_view profileIdentity) const {
        const auto found = owners_.find(std::string(sourceIdentity));
        return found != owners_.end() && found->second.profileIdentity == profileIdentity;
    }

    std::string LegacySourceBindings::assignedProfile(std::string_view sourceIdentity) const {
        const auto found = owners_.find(std::string(sourceIdentity));
        return found == owners_.end() ? std::string{} : found->second.profileIdentity;
    }

    std::string LegacySourceBindings::assignedGame(std::string_view sourceIdentity) const {
        const auto found = owners_.find(std::string(sourceIdentity));
        return found == owners_.end() ? std::string{} : found->second.gameIdentity;
    }
}
