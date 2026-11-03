#include "Legacy/LegacySourceBindings.h"
#include "Integration/Gen5/Gen5SourcePathIdentity.h"
#include "Utils/PokeBankPaths.h"

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <initializer_list>
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
        bool validSourceExtension(const BindingRecord& binding) {
            if (binding.sourceVariant.empty())
                return binding.physicalIdentity.empty() && binding.contentFingerprint.empty();
            if (!binding.contentFingerprint.empty() &&
                (binding.physicalIdentity.empty() || binding.contentFingerprint.size() != 64 ||
                 !std::all_of(binding.contentFingerprint.begin(),binding.contentFingerprint.end(),
                    [](unsigned char ch) { return (ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f'); })))
                return false;
            const bool bw = binding.gameIdentity == "black_nds" || binding.gameIdentity == "white_nds";
            const bool sequel = binding.gameIdentity == "black2_nds" || binding.gameIdentity == "white2_nds";
            return !binding.sourcePath.empty() &&
                ((bw && binding.expectedRawFamily == "BW") ||
                 (sequel && binding.expectedRawFamily == "B2W2")) &&
                (binding.sourceVariant == "gen5-copy-0-primary" ||
                 binding.sourceVariant == "gen5-copy-1-backup") &&
                binding.physicalIdentity.find('\0') == std::string::npos;
        }
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
        bool parseRow(std::string_view row, std::string& key, BindingRecord& binding) {
            std::vector<std::string> fields;
            size_t start = 0;
            do {
                const size_t tab = row.find('\t', start);
                std::string field;
                if (!hexDecode(row.substr(start, tab == std::string_view::npos
                    ? row.size() - start : tab - start), field) ||
                    field.find('\0') != std::string::npos) return false;
                fields.push_back(std::move(field));
                if (tab == std::string_view::npos) break;
                start = tab + 1;
            } while (fields.size() <= 9);
            if (fields.size() != 2 && fields.size() != 3 && fields.size() != 6 &&
                fields.size() != 7 && fields.size() != 8 && fields.size() != 9) return false;
            binding = {};
            binding.profileIdentity = fields[1];
            if (fields.size() >= 3) binding.gameIdentity = fields[2];
            if (fields.size() >= 6) {
                binding.sourcePath = fields[3];
                binding.sourceType = fields[4];
                binding.expectedRawFamily = fields[5];
            }
            if (fields.size() >= 7) binding.sourceVariant = fields[6];
            if (fields.size() >= 8) binding.physicalIdentity = fields[7];
            if (fields.size() == 9) binding.contentFingerprint = fields[8];
            if (!validSourceExtension(binding)) return false;
            key = std::move(fields[0]);
            return true;
        }

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
                std::string key;
                BindingRecord binding;
                if (!parseRow(row, key, binding) || !owners.emplace(std::move(key), std::move(binding)).second) {
                    owners.clear(); errno = EILSEQ; return ReadStatus::Invalid;
                }
                offset = end + 1;
            }
            return ReadStatus::Valid;
        }

        // Session recovery only: rows of an unreadable committed database that still parse,
        // used in memory to keep enforcing other profiles' claims. Never written back by itself.
        Owners salvageRows(const std::string& path) {
            Owners owners;
            FILE* file = std::fopen(path.c_str(), "rb");
            if (!file) return owners;
            std::string bytes;
            char buffer[4096];
            size_t count;
            while (bytes.size() <= 1024 * 1024 && (count = std::fread(buffer, 1, sizeof(buffer), file)) != 0)
                bytes.append(buffer, count);
            std::fclose(file);
            size_t offset = 0;
            while (offset < bytes.size()) {
                size_t end = bytes.find('\n', offset);
                if (end == std::string::npos) break; // a torn final row is never trusted
                const std::string_view row(bytes.data() + offset, end - offset);
                std::string key;
                BindingRecord binding;
                if (!row.empty() && row.front() != '#' && parseRow(row, key, binding) &&
                    owners.find(key) == owners.end())
                    owners.emplace(std::move(key), std::move(binding));
                offset = end + 1;
            }
            return owners;
        }

        long fileSize(const std::string& path) {
            struct stat info{};
            return ::stat(path.c_str(), &info) == 0 ? static_cast<long>(info.st_size) : -1;
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
                if (!binding.sourceVariant.empty()) {
                    bytes += "\t" + hexEncode(binding.sourceVariant);
                    if (!binding.physicalIdentity.empty())
                        bytes += "\t" + hexEncode(binding.physicalIdentity);
                    if (!binding.contentFingerprint.empty())
                        bytes += "\t" + hexEncode(binding.contentFingerprint);
                }
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
        const std::string_view s(stage);
        const auto is = [&](std::initializer_list<std::string_view> names) {
            return std::find(names.begin(), names.end(), s) != names.end();
        };
        if (s.find("conflict") != std::string_view::npos)
            lastResult_ = BindingResult::BelongsToAnotherProfile;
        else if (s == "ensure-config-root" || (s == "open-temp" && (error == ENOENT || error == ENOTDIR)))
            lastResult_ = BindingResult::MetadataDirUnavailable;
        else if (is({"validate-temp", "validate-target"}))
            lastResult_ = BindingResult::MetadataValidationFailed;
        else if (is({"read-existing", "read-recovery-backup", "load-database"}))
            lastResult_ = BindingResult::ExistingMetadataCorrupt;
        else if (is({"open-temp", "write-temp", "flush-temp", "sync-temp", "close-temp", "retire-backup",
                     "old-to-backup", "temp-to-target", "recover-backup", "rollback", "quarantine-corrupt"}))
            lastResult_ = BindingResult::MetadataWriteFailed;
        else
            lastResult_ = BindingResult::ValidationFailed;
        return false;
    }

    const char* bindingResultText(BindingResult result) noexcept {
        switch (result) {
            case BindingResult::Succeeded: return "Assigned";
            case BindingResult::ValidationFailed: return "The assignment request was not valid";
            case BindingResult::BelongsToAnotherProfile: return "That save is already linked to another profile or game";
            case BindingResult::MetadataDirUnavailable: return "The PokeBank NX config folder is unavailable";
            case BindingResult::MetadataWriteFailed: return "The assignment file could not be written";
            case BindingResult::MetadataValidationFailed: return "The written assignment file did not read back correctly";
            case BindingResult::ExistingMetadataCorrupt: return "The existing assignment file is damaged (kept unchanged)";
            case BindingResult::SourceChanged: return "That save changed since it was listed";
            case BindingResult::UnsupportedSave: return "That file is not a save for this game";
            case BindingResult::KeptForSession: return "Opened for this session; the choice could not be stored";
        }
        return "Unknown assignment result";
    }

    bool LegacySourceBindings::commit(Owners&& before) {
        if (save()) { unsaved_ = false; unsavedReason_.clear(); return true; }
        const bool persistence = lastResult_ == BindingResult::MetadataDirUnavailable ||
            lastResult_ == BindingResult::MetadataWriteFailed ||
            lastResult_ == BindingResult::MetadataValidationFailed ||
            lastResult_ == BindingResult::ExistingMetadataCorrupt;
        if (sessionRecovery_ && persistence) {
            // The on-disk generation is untouched (save() keeps/rolls back to it). The validated
            // choice stays in memory so every route agrees for this session.
            unsaved_ = true;
            unsavedReason_ = lastError_;
            lastResult_ = BindingResult::KeptForSession;
            return true;
        }
        owners_ = std::move(before);
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
        lastResult_ = BindingResult::Succeeded;
        const auto primary = readDatabase(storagePath_, candidate, bytes);
        if (primary == ReadStatus::Valid) {
            owners_ = std::move(candidate); trusted_ = true; loadState_ = "primary";
            return true;
        }
        const int primaryError = errno;
        const auto backup = readDatabase(storagePath_ + ".bak", candidate, bytes);
        if (backup == ReadStatus::Valid) {
            owners_ = std::move(candidate);
            trusted_ = true;
            loadState_ = primary == ReadStatus::Missing ? "backup (primary missing)" : "backup (primary damaged)";
            lastError_ = "Recovered bindings from .bak; primary missing or invalid";
            return true;
        }
        if (primary == ReadStatus::Missing && backup == ReadStatus::Missing) {
            owners_.clear(); // A .tmp alone is uncommitted, never automatically promoted.
            trusted_ = true; loadState_ = "new (no database yet)";
            return true;
        }
        errno = primary == ReadStatus::Invalid ? primaryError : errno;
        const bool keptPrevious = trusted_;
        fail("load-database"); // Preserve previous in-memory state on failure.
        if (sessionRecovery_ && !keptPrevious && owners_.empty()) {
            // Keep enforcing whatever claims are still legible; the damaged file stays untouched.
            owners_ = salvageRows(primary == ReadStatus::Invalid ? storagePath_ : storagePath_ + ".bak");
            loadState_ = "damaged; " + std::to_string(owners_.size()) + " legible row(s) used for this session";
        } else {
            loadState_ = "damaged";
        }
        return false;
    }

    bool LegacySourceBindings::save() const {
        lastError_.clear();
        lastResult_ = BindingResult::Succeeded;
        if (storagePath_.empty()) { errno = EINVAL; return fail("save-path"); }
        // On an SD card with no prior PokeBank NX settings, nothing guarantees /config
        // exists. Both discovered Save Instances and the manual SD picker hit this
        // writer, so omitting this step blocks EVERY classic game's first assignment.
        // Only provision the app-owned production location; test/custom paths retain
        // their existing no-implicit-directory-creation and failure semantics.
        if (storagePath_ == PokeBank::Paths::legacySourceBindingsFile()) {
            std::string directoryError;
            if (!PokeBank::Paths::ensureConfigRoot(&directoryError))
                return fail("ensure-config-root");
        }
        const auto renameFile = ops_.renameFile ? ops_.renameFile : std::rename;
        const std::string tmp = storagePath_ + ".tmp";
        const std::string bak = storagePath_ + ".bak";
        Owners previous, checked;
        std::string priorBytes, checkedBytes;
        auto primary = readDatabase(storagePath_, previous, priorBytes);
        if (primary == ReadStatus::Invalid && sessionRecovery_ && trusted_ && errno == EILSEQ) {
            // Round 16 P0 (hardware): a committed primary torn by a crash/power loss used to block
            // every later assignment forever. With a trusted generation in memory, move the damaged
            // bytes aside unchanged (never deleted, never overwritten) and continue from .bak.
            std::string target;
            for (int n = 1; n <= 99 && target.empty(); ++n) {
                const std::string candidate = storagePath_ + ".corrupt-" + std::to_string(n);
                struct stat info{};
                if (::stat(candidate.c_str(), &info) != 0 && errno == ENOENT) target = candidate;
            }
            if (target.empty()) { errno = EEXIST; return fail("quarantine-corrupt"); }
            if (!checkpoint("quarantine-corrupt")) return false;
            if (renameFile(storagePath_.c_str(), target.c_str()) != 0) return fail("quarantine-corrupt");
            quarantined_ = target;
            primary = ReadStatus::Missing;
        }
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
        auto before = owners_;
        if (!assign(source, profile)) { errno = EINVAL; return fail("assign"); }
        return commit(std::move(before));
    }

    bool LegacySourceBindings::assignAndSave(std::string_view source, std::string_view profile,
                                             std::string_view game) {
        auto before = owners_;
        if (!assign(source, profile, game)) { errno = EINVAL; return fail("assign"); }
        return commit(std::move(before));
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
            !safe(binding.sourcePath) || !safe(binding.sourceType) || !safe(binding.expectedRawFamily) ||
            !validSourceExtension(binding)) {
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
            const bool sdAlias = Integration::Gen5::gen5BindingGame(binding.gameIdentity) &&
                Integration::Gen5::sameSdOwnershipPath(binding.sourcePath, metadataPath);
            if (binding.sourcePath == metadataPath || sameFile || sdAlias) {
                errno = EINVAL; return fail("assignment-aliases-metadata");
            }
        }
        auto before = owners_;
        owners_[std::string(source)] = std::move(binding);
        return commit(std::move(before));
    }

    bool LegacySourceBindings::replaceFileAssignmentAndSave(
        std::string_view sourceIdentity, BindingRecord binding) {
        return replaceFileAssignment(sourceIdentity, std::move(binding), false);
    }

    bool LegacySourceBindings::assignClassicFileAndSave(
        std::string_view sourceIdentity, BindingRecord binding) {
        return replaceFileAssignment(sourceIdentity, std::move(binding), true);
    }

    bool LegacySourceBindings::replaceFileAssignment(
        std::string_view sourceIdentity, BindingRecord binding, bool keepCatalogClaims) {
        const auto safe = [](std::string_view value) {
            return !value.empty() && value.find('\0') == std::string_view::npos;
        };
        if (!safe(sourceIdentity) || !safe(binding.profileIdentity) ||
            !safe(binding.gameIdentity) || !safe(binding.sourcePath) ||
            !safe(binding.sourceType) || !safe(binding.expectedRawFamily) || !validSourceExtension(binding)) {
            errno = EINVAL;
            return fail("replace-file-assignment");
        }

        Source::SaveInstance selected;
        selected.sourceIdentity = sourceIdentity;
        selected.sourcePath = binding.sourcePath;
        selected.gameId = binding.gameIdentity;
        selected.physicalIdentity = binding.physicalIdentity;
        selected.contentFingerprint = binding.contentFingerprint;
        applyClaims(selected);
        if (!Source::visibleToProfile(selected, binding.profileIdentity)) {
            errno = EEXIST; return fail("replace-file-assignment-conflict");
        }
        const auto existing = owners_.find(std::string(sourceIdentity));
        const bool ownCatalogClaim = keepCatalogClaims && existing != owners_.end() &&
            existing->second.sourcePath.empty() &&
            existing->second.profileIdentity == binding.profileIdentity;
        if (existing != owners_.end() && !ownCatalogClaim &&
            (existing->second.profileIdentity != binding.profileIdentity ||
             existing->second.gameIdentity != binding.gameIdentity)) {
            errno = EEXIST;
            return fail("replace-file-assignment-conflict");
        }

        struct stat sourceInfo{};
        const bool sourceExists = ::stat(binding.sourcePath.c_str(), &sourceInfo) == 0;
        for (const auto& metadataPath : {storagePath_, storagePath_ + ".tmp", storagePath_ + ".bak"}) {
            struct stat metadataInfo{};
            const bool sameFile = sourceExists && ::stat(metadataPath.c_str(), &metadataInfo) == 0 &&
                sourceInfo.st_ino != 0 && sourceInfo.st_dev == metadataInfo.st_dev &&
                sourceInfo.st_ino == metadataInfo.st_ino;
            const bool sdAlias = Integration::Gen5::gen5BindingGame(binding.gameIdentity) &&
                Integration::Gen5::sameSdOwnershipPath(binding.sourcePath, metadataPath);
            if (binding.sourcePath == metadataPath || sameFile || sdAlias) {
                errno = EINVAL;
                return fail("assignment-aliases-metadata");
            }
        }

        auto before = owners_;
        for (auto it = owners_.begin(); it != owners_.end();) {
            if (it->second.profileIdentity == binding.profileIdentity &&
                it->second.gameIdentity == binding.gameIdentity) {
                // A Gen I-III catalog preference keeps its profile claim (the other discovered
                // save stays private to this profile); only its game preference moves to the file.
                if (keepCatalogClaims && it->second.sourcePath.empty() &&
                    it->first != sourceIdentity) {
                    it->second.gameIdentity.clear();
                    ++it;
                } else {
                    it = owners_.erase(it);
                }
            } else {
                ++it;
            }
        }
        owners_[std::string(sourceIdentity)] = std::move(binding);
        return commit(std::move(before));
    }

    void LegacySourceBindings::applyClaims(Source::SaveInstance& instance) const {
        struct stat selected{};
        const bool exists = ::stat(instance.path().c_str(), &selected) == 0 && selected.st_ino != 0;
        for (const auto& [identity, binding] : owners_) {
            bool matches = identity == instance.sourceIdentity ||
                std::find(instance.sourceAliases.begin(), instance.sourceAliases.end(), identity) != instance.sourceAliases.end();
            if (Integration::Gen5::gen5BindingGame(binding.gameIdentity) &&
                Integration::Gen5::gen5BindingGame(instance.gameId))
                matches |= Integration::Gen5::sameSdOwnershipPath(binding.sourcePath, instance.path());
            if (!binding.sourcePath.empty()) {
                struct stat bound{};
                matches |= binding.sourcePath == instance.path() || (exists &&
                    ::stat(binding.sourcePath.c_str(), &bound) == 0 &&
                    selected.st_dev == bound.st_dev && selected.st_ino == bound.st_ino);
            }
            // Inodes can be reused after deletion. Live path/inode aliases
            // above retain ownership through in-place emulator changes. A
            // renamed source needs its original physical ID AND snapshot.
            const bool samePhysical = !binding.physicalIdentity.empty() &&
                binding.physicalIdentity == instance.physicalIdentity;
            if (samePhysical && !matches) {
                if (!binding.contentFingerprint.empty() && !instance.contentFingerprint.empty() &&
                    binding.contentFingerprint == instance.contentFingerprint)
                    matches = true;
                else if (instance.gameId == binding.gameIdentity) {
                    // Could be a changed moved source OR a reused inode for
                    // another same-title save. Do not fabricate its owner.
                    instance.claimConflict = true;
                    instance.diagnostic = "Gen V source ownership is ambiguous; repair or forget the stale remembered assignment before choosing this file";
                }
            }
            if (matches) {
                Source::mergeProfileClaim(instance, binding.profileIdentity);
                if (!instance.gameId.empty() && !binding.gameIdentity.empty() &&
                    instance.gameId != binding.gameIdentity) instance.claimConflict = true;
            }
        }
    }

    bool LegacySourceBindings::claimInstanceAndSave(const Source::SaveInstance& instance,
                                                    std::string_view profile) {
        auto checked = instance;
        applyClaims(checked);
        if (profile.empty() || !instance.ready() || !Source::visibleToProfile(checked, profile)) {
            errno = EEXIST; return fail("claim-source-conflict");
        }
        auto before = owners_;
        if (!assign(instance.sourceIdentity, profile)) { errno = EINVAL; return fail("claim-source"); }
        for (const auto& alias : instance.sourceAliases) {
            if (!assign(alias, profile)) { owners_ = before; errno = EINVAL; return fail("claim-alias"); }
        }
        return commit(std::move(before));
    }

    bool LegacySourceBindings::preferGameSourceAndSave(
        const Source::SaveInstance& instance, std::string_view profileIdentity,
        std::string_view gameIdentity) {
        lastError_.clear();
        auto checked = instance;
        applyClaims(checked);
        if (profileIdentity.empty() || gameIdentity.empty() || !instance.ready() ||
            instance.gameId != gameIdentity ||
            !Source::visibleToProfile(checked, profileIdentity)) {
            errno = EEXIST;
            return fail("prefer-game-source-conflict");
        }

        std::vector<std::string> identities;
        identities.reserve(instance.sourceAliases.size() + 1);
        identities.push_back(instance.sourceIdentity);
        for (const auto& alias : instance.sourceAliases) {
            if (!alias.empty() &&
                std::find(identities.begin(), identities.end(), alias) == identities.end())
                identities.push_back(alias);
        }
        if (identities.front().empty()) {
            errno = EINVAL;
            return fail("prefer-game-source-identity");
        }

        // Validate every identity before changing the in-memory map. File-backed Gen IV rows are
        // deliberately not repurposed as classic catalog preferences.
        for (const auto& identity : identities) {
            const auto found = owners_.find(identity);
            if (found == owners_.end()) continue;
            // Round 16 P0: the same physical save already assigned to THIS profile and THIS game
            // through the SD picker (a file row) is the same choice, not a conflict. A file row for
            // another game, or any row of another profile, is still refused.
            if (found->second.profileIdentity != profileIdentity ||
                (!found->second.sourcePath.empty() && found->second.gameIdentity != gameIdentity)) {
                errno = EEXIST;
                return fail("prefer-game-source-conflict");
            }
        }

        auto before = owners_;
        // Clear only the prior catalog preference. Keep its profile claim so choosing save B never
        // makes save A suddenly visible to another profile.
        for (auto it = owners_.begin(); it != owners_.end();) {
            auto& binding = it->second;
            if (binding.profileIdentity == profileIdentity && binding.gameIdentity == gameIdentity) {
                if (binding.sourcePath.empty()) {
                    binding.gameIdentity.clear();
                } else if (std::find(identities.begin(), identities.end(), it->first) == identities.end()) {
                    // Round 15 P0 (Change): a manually assigned file for this exact profile/game is
                    // replaced by the newly chosen save, never left as a second "preferred" record.
                    it = owners_.erase(it);
                    continue;
                }
            }
            ++it;
        }

        auto& primary = owners_[instance.sourceIdentity];
        primary.profileIdentity = std::string(profileIdentity);
        primary.gameIdentity = std::string(gameIdentity);
        for (size_t index = 1; index < identities.size(); ++index) {
            auto& alias = owners_[identities[index]];
            alias.profileIdentity = std::string(profileIdentity);
            // Only the canonical identity carries the preference, otherwise one physical file's
            // aliases would look like multiple preferred saves.
            alias.gameIdentity.clear();
        }

        return commit(std::move(before));
    }

    bool LegacySourceBindings::isPreferredGameSource(
        const Source::SaveInstance& instance, std::string_view profileIdentity,
        std::string_view gameIdentity) const {
        if (profileIdentity.empty() || gameIdentity.empty() || instance.gameId != gameIdentity)
            return false;
        const auto matches = [&](const std::string& identity) {
            const auto found = owners_.find(identity);
            return found != owners_.end() &&
                found->second.profileIdentity == profileIdentity &&
                found->second.gameIdentity == gameIdentity;
        };
        if (matches(instance.sourceIdentity)) return true;
        return std::any_of(instance.sourceAliases.begin(), instance.sourceAliases.end(), matches);
    }

    bool LegacySourceBindings::unassignGameAndSave(
        std::string_view profileIdentity, std::string_view gameIdentity) {
        if (profileIdentity.empty() || gameIdentity.empty()) {
            errno = EINVAL;
            return fail("unassign-game");
        }
        auto before = owners_;
        size_t removed = 0;
        for (auto it = owners_.begin(); it != owners_.end();) {
            if (it->second.profileIdentity == profileIdentity &&
                it->second.gameIdentity == gameIdentity) {
                it = owners_.erase(it);
                ++removed;
            } else {
                ++it;
            }
        }
        if (removed == 0) {
            errno = ENOENT;
            return fail("unassign-game");
        }
        return commit(std::move(before));
    }

    bool LegacySourceBindings::forgetGameAndSave(
        std::string_view profileIdentity, std::string_view gameIdentity,
        const std::vector<std::string>& releaseIdentities) {
        lastError_.clear();
        if (profileIdentity.empty() || gameIdentity.empty()) {
            errno = EINVAL;
            return fail("forget-game");
        }
        auto before = owners_;
        size_t removed = 0;
        for (auto it = owners_.begin(); it != owners_.end();) {
            const bool exactGame = it->second.profileIdentity == profileIdentity &&
                it->second.gameIdentity == gameIdentity;
            const bool bareClaim = it->second.profileIdentity == profileIdentity &&
                it->second.gameIdentity.empty() && it->second.sourcePath.empty() &&
                std::find(releaseIdentities.begin(), releaseIdentities.end(), it->first) !=
                    releaseIdentities.end();
            if (exactGame || bareClaim) {
                it = owners_.erase(it);
                ++removed;
            } else {
                ++it;
            }
        }
        if (removed == 0) {
            errno = ENOENT;
            return fail("forget-game");
        }
        return commit(std::move(before));
    }

    bool LegacySourceBindings::hasGameRecord(std::string_view profileIdentity,
                                             std::string_view gameIdentity) const {
        if (profileIdentity.empty() || gameIdentity.empty()) return false;
        return std::any_of(owners_.begin(), owners_.end(), [&](const auto& entry) {
            return entry.second.profileIdentity == profileIdentity &&
                   entry.second.gameIdentity == gameIdentity;
        });
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

    bool LegacySourceBindings::unassignAndSave(std::string_view sourceIdentity) {
        auto before = owners_;
        if (!unassign(sourceIdentity)) { errno = EINVAL; return fail("unassign"); }
        return commit(std::move(before));
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

    std::string LegacySourceBindings::storageDiagnostics() const {
        std::string out = "bindings db=" + storagePath_ + " load=" + loadState_ +
            " rows=" + std::to_string(owners_.size()) +
            " size(cfg/bak/tmp)=" + std::to_string(fileSize(storagePath_)) + "/" +
            std::to_string(fileSize(storagePath_ + ".bak")) + "/" + std::to_string(fileSize(storagePath_ + ".tmp"));
        const size_t slash = storagePath_.rfind('/');
        if (slash == std::string::npos) return out + " probe=skipped(no directory)";
        const std::string probe = storagePath_.substr(0, slash) + "/.write-probe";
        const std::string probeTmp = probe + ".tmp";
        const auto step = [&]() -> std::string {
            std::remove(probe.c_str());
            std::remove(probeTmp.c_str());
            errno = 0;
            FILE* file = std::fopen(probeTmp.c_str(), "wb");
            if (!file) return "open errno=" + std::to_string(errno);
            const char text[] = "probe\n";
            bool ok = std::fwrite(text, 1, sizeof(text) - 1, file) == sizeof(text) - 1 && std::fflush(file) == 0;
            int err = errno;
            if (ok && ::fsync(::fileno(file)) != 0) { ok = false; err = errno; }
            if (std::fclose(file) != 0 && ok) { ok = false; err = errno; }
            if (!ok) return "write/sync errno=" + std::to_string(err);
            if (std::rename(probeTmp.c_str(), probe.c_str()) != 0) return "rename errno=" + std::to_string(errno);
            FILE* back = std::fopen(probe.c_str(), "rb");
            if (!back) return "reopen errno=" + std::to_string(errno);
            char read[16] = {};
            const size_t got = std::fread(read, 1, sizeof(read), back);
            std::fclose(back);
            if (got != sizeof(text) - 1 || std::memcmp(read, text, got) != 0) return "readback mismatch";
            return "ok";
        };
        const std::string result = step();
        std::remove(probe.c_str());
        std::remove(probeTmp.c_str());
        return out + " probe=" + result;
    }
}
