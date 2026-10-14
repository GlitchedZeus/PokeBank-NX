#include "Integration/Common/VerifiedStagedExport.h"

#include "Utils/SHA256.h"

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <sys/stat.h>
#include <unistd.h>

namespace PokeVault::Integration::Common {
namespace {

bool pathExists(const std::string& path) {
    struct stat st{};
    return ::stat(path.c_str(), &st) == 0;
}

bool mkdirIfNeeded(const std::string& path) {
    if (::mkdir(path.c_str(), 0777) == 0) return true;
    return errno == EEXIST;
}

std::string join(const std::string& left, const std::string& right) {
    if (left.empty()) return right;
    if (left.back() == '/') return left + right;
    return left + "/" + right;
}

bool safeStem(const std::string& stem) {
    if (stem.empty() || stem == "." || stem == "..") return false;
    return std::all_of(stem.begin(), stem.end(), [](unsigned char c) {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
               c == '_' || c == '-' || c == '.';
    });
}

bool writeSynced(const std::string& path, std::span<const uint8_t> bytes, std::string& error) {
    FILE* out = std::fopen(path.c_str(), "wb");
    if (!out) {
        error = "Could not open export file: " + path;
        return false;
    }
    bool ok = bytes.empty() || std::fwrite(bytes.data(), 1, bytes.size(), out) == bytes.size();
    if (ok) ok = std::fflush(out) == 0;
    if (ok) {
        const int fd = ::fileno(out);
        if (fd >= 0) ok = ::fsync(fd) == 0;
    }
    const bool closeOk = std::fclose(out) == 0;
    if (!ok || !closeOk) {
        error = "Could not durably write export file: " + path;
        return false;
    }
    return true;
}

bool readAll(const std::string& path, std::vector<uint8_t>& bytes, std::string& error) {
    FILE* in = std::fopen(path.c_str(), "rb");
    if (!in) {
        error = "Could not reopen exported file: " + path;
        return false;
    }
    bool ok = std::fseek(in, 0, SEEK_END) == 0;
    const long size = ok ? std::ftell(in) : -1;
    ok = ok && size >= 0 && std::fseek(in, 0, SEEK_SET) == 0;
    if (ok) {
        bytes.resize(static_cast<size_t>(size));
        ok = bytes.empty() || std::fread(bytes.data(), 1, bytes.size(), in) == bytes.size();
    }
    const bool closeOk = std::fclose(in) == 0;
    if (!ok || !closeOk) {
        error = "Could not read exported file back: " + path;
        return false;
    }
    return true;
}

} // namespace

std::string sha256Hex(std::span<const uint8_t> bytes) {
    Utils::SHA256 digest;
    digest.update(bytes.data(), bytes.size());
    uint8_t raw[Utils::PKSE_SHA256_HASH_SIZE]{};
    digest.finalize(raw);
    static constexpr char hex[] = "0123456789abcdef";
    std::string out;
    out.reserve(Utils::PKSE_SHA256_HASH_SIZE * 2);
    for (uint8_t byte : raw) {
        out.push_back(hex[byte >> 4]);
        out.push_back(hex[byte & 0x0F]);
    }
    return out;
}

std::vector<uint8_t> mergeDisjointBlocks(std::span<const uint8_t> original,
                                         std::span<const uint8_t> first,
                                         std::span<const uint8_t> second,
                                         size_t blockSize, std::string& error) {
    error.clear();
    if (blockSize == 0 || original.size() != first.size() || original.size() != second.size() ||
        original.size() % blockSize != 0) {
        error = "Staged copies do not share the original save layout";
        return {};
    }
    std::vector<uint8_t> merged(first.begin(), first.end());
    for (size_t offset = 0; offset < original.size(); offset += blockSize) {
        const auto base = original.subspan(offset, blockSize);
        const auto a = first.subspan(offset, blockSize);
        const auto b = second.subspan(offset, blockSize);
        const bool aChanged = !std::equal(a.begin(), a.end(), base.begin());
        const bool bChanged = !std::equal(b.begin(), b.end(), base.begin());
        if (aChanged && bChanged) {
            error = "Both staged workspaces changed the same save block; export them separately";
            return {};
        }
        if (bChanged) std::copy(b.begin(), b.end(), merged.begin() + static_cast<std::ptrdiff_t>(offset));
    }
    return merged;
}

VerifiedExportResult publishVerifiedStagedExport(const VerifiedExportRequest& request) {
    VerifiedExportResult result;
    if (request.rootDirectory.empty() || !safeStem(request.directoryStem)) {
        result.error = "Export root/directory name is empty or unsafe";
        return result;
    }
    if (!safeStem(request.fileExtension.size() > 1 ? request.fileExtension.substr(1) : "") ||
        request.fileExtension.front() != '.') {
        result.error = "Export file extension is unsafe";
        return result;
    }
    if (request.originalBytes.empty() || request.editedBytes.empty()) {
        result.error = "Export source or staged bytes are empty";
        return result;
    }
    if (request.originalBytes.size() != request.editedBytes.size()) {
        result.error = "Original/staged save size mismatch";
        return result;
    }
    if (!request.validator) {
        result.error = "No strict save validator was supplied";
        return result;
    }
    for (size_t i = 0; i < request.editedBytes.size(); ++i)
        if (request.originalBytes[i] != request.editedBytes[i]) ++result.changedBytes;
    if (result.changedBytes == 0) {
        result.error = "No pending changes to export";
        return result;
    }

    // Validate the exact in-memory bytes before touching the filesystem.
    std::string parseError;
    if (!request.validator(request.editedBytes, parseError)) {
        result.error = "Strict staged reparse failed before export: " + parseError;
        return result;
    }

    result.originalSha256 = sha256Hex(request.originalBytes);
    result.editedSha256 = sha256Hex(request.editedBytes);

    if (!mkdirIfNeeded(request.rootDirectory)) {
        result.error = "Could not create export root";
        return result;
    }
    std::string finalDirectory = join(request.rootDirectory, request.directoryStem);
    for (int suffix = 2; pathExists(finalDirectory) && suffix < 10000; ++suffix)
        finalDirectory = join(request.rootDirectory,
                              request.directoryStem + "-" + std::to_string(suffix));
    if (pathExists(finalDirectory)) {
        result.error = "Could not choose a unique export directory";
        return result;
    }
    std::string temporaryDirectory = finalDirectory + ".tmp";
    for (int suffix = 2; pathExists(temporaryDirectory) && suffix < 10000; ++suffix)
        temporaryDirectory = finalDirectory + ".tmp-" + std::to_string(suffix);
    if (pathExists(temporaryDirectory) || ::mkdir(temporaryDirectory.c_str(), 0777) != 0) {
        result.error = "Could not create temporary export directory";
        return result;
    }

    const std::string backupName = "original_backup" + request.fileExtension;
    const std::string editedName = "edited" + request.fileExtension;
    const std::string backupPath = join(temporaryDirectory, backupName);
    const std::string editedPath = join(temporaryDirectory, editedName);
    const std::string manifestPath = join(temporaryDirectory, "EDIT_MANIFEST.txt");

    auto fail = [&](const std::string& message) {
        result.error = message;
        std::remove(backupPath.c_str());
        std::remove(editedPath.c_str());
        std::remove(manifestPath.c_str());
        ::rmdir(temporaryDirectory.c_str());
        result.originalSha256.clear();
        result.editedSha256.clear();
        return result;
    };

    std::string ioError;
    if (!writeSynced(backupPath, request.originalBytes, ioError)) return fail(ioError);
    if (request.failurePoint == ExportFailurePoint::AfterBackupWrite)
        return fail("Injected failure after backup write");
    if (!writeSynced(editedPath, request.editedBytes, ioError)) return fail(ioError);
    if (request.failurePoint == ExportFailurePoint::AfterEditedWrite)
        return fail("Injected failure after edited write");

    // Verify the files that actually landed on disk, not merely the buffers we intended to write.
    std::vector<uint8_t> backupReadback;
    std::vector<uint8_t> editedReadback;
    if (!readAll(backupPath, backupReadback, ioError)) return fail(ioError);
    if (!readAll(editedPath, editedReadback, ioError)) return fail(ioError);
    if (!std::equal(backupReadback.begin(), backupReadback.end(),
                    request.originalBytes.begin(), request.originalBytes.end()))
        return fail("Original backup read-back mismatch");
    if (!std::equal(editedReadback.begin(), editedReadback.end(),
                    request.editedBytes.begin(), request.editedBytes.end()))
        return fail("Edited save read-back mismatch");
    if (sha256Hex(backupReadback) != result.originalSha256)
        return fail("Original backup hash verification failed");
    if (sha256Hex(editedReadback) != result.editedSha256)
        return fail("Edited save hash verification failed");
    parseError.clear();
    if (!request.validator(editedReadback, parseError))
        return fail("Strict reparse of written edited save failed: " + parseError);
    if (request.failurePoint == ExportFailurePoint::AfterReadbackVerify)
        return fail("Injected failure after read-back verification");

    std::string manifest;
    manifest += "PokeBank NX " + request.generationLabel + " verified staged export\n";
    manifest += "GAME_ID=" + request.gameId + "\n";
    manifest += "SAVE_FORMAT=" + request.saveFormat + "\n";
    manifest += "SOURCE_PATH=" + request.sourcePath + "\n";
    manifest += "SOURCE_KIND=READ_ONLY\n";
    manifest += "SOURCE_SIZE=" + std::to_string(request.originalBytes.size()) + "\n";
    manifest += "SOURCE_SHA256=" + result.originalSha256 + "\n";
    manifest += "TIMESTAMP=" + request.timestamp + "\n";
    manifest += "APPLICATION_SHA=" + request.applicationSha + "\n";
    manifest += "ORIGINAL_BACKUP=" + backupName + "\n";
    manifest += "EDITED_SAVE=" + editedName + "\n";
    manifest += "EDITED_SIZE=" + std::to_string(request.editedBytes.size()) + "\n";
    manifest += "EDITED_SHA256=" + result.editedSha256 + "\n";
    manifest += "CHANGED_BYTES=" + std::to_string(result.changedBytes) + "\n";
    manifest += "WRITE_READBACK=VERIFIED\n";
    manifest += "STRICT_REPARSE=VERIFIED\n";
    manifest += "LIVE_EMULATOR_WRITE=DISABLED\n";
    manifest += "LIVE_INSTALLED_GAME_WRITE=DISABLED\n";
    manifest += "PENDING_CHANGES:\n";
    for (const auto& line : request.changeLines) manifest += "- " + line + "\n";
    if (!writeSynced(manifestPath, std::span<const uint8_t>(
            reinterpret_cast<const uint8_t*>(manifest.data()), manifest.size()), ioError))
        return fail(ioError);
    if (request.failurePoint == ExportFailurePoint::AfterManifestWrite)
        return fail("Injected failure after manifest write");

    // Publish only after all bytes, hashes and strict parser checks succeeded.
    if (::rename(temporaryDirectory.c_str(), finalDirectory.c_str()) != 0)
        return fail("Could not publish verified export directory");

    result.success = true;
    result.directory = finalDirectory;
    return result;
}

} // namespace PokeVault::Integration::Common
