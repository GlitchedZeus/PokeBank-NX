#include "Integration/Gen2/Gen2ExportTransaction.h"

#include "Utils/SHA256.h"

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

namespace PokeVault::Integration::Gen2 {
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

bool writeSynced(const std::string& path, std::span<const uint8_t> bytes, std::string& error) {
    FILE* out = std::fopen(path.c_str(), "wb");
    if (!out) {
        error = "Could not open export file: " + path;
        return false;
    }
    bool ok = std::fwrite(bytes.data(), 1, bytes.size(), out) == bytes.size();
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

bool writeSyncedText(const std::string& path, const std::string& text, std::string& error) {
    return writeSynced(path, std::span<const uint8_t>(
        reinterpret_cast<const uint8_t*>(text.data()), text.size()), error);
}

bool readAll(const std::string& path, std::vector<uint8_t>& bytes, std::string& error) {
    FILE* in = std::fopen(path.c_str(), "rb");
    if (!in) {
        error = "Could not reopen exported file: " + path;
        return false;
    }
    if (std::fseek(in, 0, SEEK_END) != 0) {
        std::fclose(in);
        error = "Could not size exported file: " + path;
        return false;
    }
    const long size = std::ftell(in);
    if (size < 0 || std::fseek(in, 0, SEEK_SET) != 0) {
        std::fclose(in);
        error = "Could not seek exported file: " + path;
        return false;
    }
    bytes.resize(static_cast<std::size_t>(size));
    const bool ok = bytes.empty() || std::fread(bytes.data(), 1, bytes.size(), in) == bytes.size();
    const bool closeOk = std::fclose(in) == 0;
    if (!ok || !closeOk) {
        error = "Could not read exported file back: " + path;
        return false;
    }
    return true;
}

void cleanupTemporaryDirectory(const std::string& directory) noexcept {
    if (directory.empty()) return;
    std::remove(join(directory, "original_backup.srm").c_str());
    std::remove(join(directory, "edited.srm").c_str());
    std::remove(join(directory, "EDIT_MANIFEST.txt").c_str());
    ::rmdir(directory.c_str());
}

std::string uniquePath(const std::string& root, const std::string& stem) {
    std::string candidate = join(root, stem);
    if (!pathExists(candidate)) return candidate;
    for (int suffix = 2; suffix < 10000; ++suffix) {
        candidate = join(root, stem + "-" + std::to_string(suffix));
        if (!pathExists(candidate)) return candidate;
    }
    return {};
}

std::string manifestFor(const ExportTransactionRequest& request,
                        const std::string& originalHash,
                        const std::string& editedHash) {
    std::string manifest;
    manifest += "PokeBank NX Generation II verified staged export\n";
    manifest += "GAME_ID=" + request.gameId + "\n";
    manifest += "SAVE_FORMAT=" + request.saveFormat + "\n";
    manifest += "SOURCE_PATH=" + request.sourcePath + "\n";
    manifest += "SOURCE_KIND=RetroArchLegacy_READ_ONLY\n";
    manifest += "SOURCE_SIZE=" + std::to_string(request.originalBytes.size()) + "\n";
    manifest += "SOURCE_SHA256=" + originalHash + "\n";
    manifest += "TIMESTAMP=" + request.timestamp + "\n";
    manifest += "APPLICATION_SHA=" + request.applicationSha + "\n";
    manifest += "ORIGINAL_BACKUP=original_backup.srm\n";
    manifest += "EDITED_SAVE=edited.srm\n";
    manifest += "EDITED_SIZE=" + std::to_string(request.editedBytes.size()) + "\n";
    manifest += "EDITED_SHA256=" + editedHash + "\n";
    manifest += "WRITE_READBACK=VERIFIED\n";
    manifest += "STRICT_REPARSE=VERIFIED\n";
    manifest += "LIVE_RETROARCH_WRITE=DISABLED\n";
    manifest += "LIVE_INSTALLED_GAME_WRITE=DISABLED\n";
    manifest += "PENDING_CHANGES:\n";
    for (const auto& change : request.pendingChanges)
        manifest += "- " + change.label + ": " + change.beforeValue + " -> " + change.afterValue + "\n";
    return manifest;
}

} // namespace

ExportTransactionResult publishVerifiedExport(const ExportTransactionRequest& request) {
    ExportTransactionResult result;
    if (request.rootDirectory.empty() || request.directoryStem.empty()) {
        result.error = "Export root/directory name is empty";
        return result;
    }
    if (request.originalBytes.empty() || request.editedBytes.empty()) {
        result.error = "Export source or staged bytes are empty";
        return result;
    }

    // Validate the exact in-memory bytes before touching the filesystem.
    const auto beforeWrite = parse(request.editedBytes, request.sourceGame);
    if (!beforeWrite) {
        result.error = "Strict staged reparse failed before export: " + beforeWrite.detail;
        return result;
    }

    result.originalSha256 = sha256Hex(request.originalBytes);
    result.editedSha256 = sha256Hex(request.editedBytes);

    if (!mkdirIfNeeded(request.rootDirectory)) {
        result.error = "Could not create export root";
        return result;
    }
    const std::string finalDirectory = uniquePath(request.rootDirectory, request.directoryStem);
    if (finalDirectory.empty()) {
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

    auto fail = [&](const std::string& message) {
        result.error = message;
        cleanupTemporaryDirectory(temporaryDirectory);
        return result;
    };

    std::string ioError;
    const std::string backupPath = join(temporaryDirectory, "original_backup.srm");
    const std::string editedPath = join(temporaryDirectory, "edited.srm");
    const std::string manifestPath = join(temporaryDirectory, "EDIT_MANIFEST.txt");

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
    if (backupReadback.size() != request.originalBytes.size() ||
        !std::equal(backupReadback.begin(), backupReadback.end(), request.originalBytes.begin()))
        return fail("Original backup read-back mismatch");
    if (editedReadback.size() != request.editedBytes.size() ||
        !std::equal(editedReadback.begin(), editedReadback.end(), request.editedBytes.begin()))
        return fail("Edited save read-back mismatch");
    if (sha256Hex(backupReadback) != result.originalSha256)
        return fail("Original backup hash verification failed");
    if (sha256Hex(editedReadback) != result.editedSha256)
        return fail("Edited save hash verification failed");

    const auto writtenParse = parse(editedReadback, request.sourceGame);
    if (!writtenParse)
        return fail("Strict reparse of written edited save failed: " + writtenParse.detail);
    if (request.failurePoint == ExportFailurePoint::AfterReadbackVerify)
        return fail("Injected failure after read-back verification");

    const std::string manifest = manifestFor(request, result.originalSha256, result.editedSha256);
    if (!writeSyncedText(manifestPath, manifest, ioError)) return fail(ioError);
    if (request.failurePoint == ExportFailurePoint::AfterManifestWrite)
        return fail("Injected failure after manifest write");

    if (request.failurePoint == ExportFailurePoint::DestinationAppearsAsEmptyDirectory)
        (void)::mkdir(finalDirectory.c_str(), 0777);
    if (request.failurePoint == ExportFailurePoint::DestinationAppearsAsFile) {
        FILE* other = std::fopen(finalDirectory.c_str(), "wb");
        if (other) std::fclose(other);
    }

    // Publish only after all bytes, hashes and strict parser checks succeeded. Everything stays
    // inside the app-owned export root; no source-save path is ever opened for writing.
    // rename(2) of a directory silently REPLACES an existing empty directory, so never rename onto
    // the destination. Claim it exclusively instead: mkdir fails with EEXIST if anything
    // (directory, file, link) appeared at that path since it was chosen, and nothing of the other
    // writer is touched. Files then move in, manifest last, so a reader never sees a manifest
    // without both verified saves beside it (same protocol as Integration/Common, QA-F3).
    if (::mkdir(finalDirectory.c_str(), 0777) != 0)
        return fail(errno == EEXIST
            ? "Export destination appeared before publication; nothing was replaced"
            : "Could not publish verified export directory");
    const std::string publishedBackup = join(finalDirectory, "original_backup.srm");
    const std::string publishedEdited = join(finalDirectory, "edited.srm");
    const std::string publishedManifest = join(finalDirectory, "EDIT_MANIFEST.txt");
    auto unpublish = [&](const std::string& message) {
        // Return moved files to the temporary directory and drop ONLY the directory this call
        // created, then clean up as for any other failure.
        (void)::rename(publishedBackup.c_str(), backupPath.c_str());
        (void)::rename(publishedEdited.c_str(), editedPath.c_str());
        (void)::rename(publishedManifest.c_str(), manifestPath.c_str());
        ::rmdir(finalDirectory.c_str());
        return fail(message);
    };
    if (::rename(backupPath.c_str(), publishedBackup.c_str()) != 0)
        return unpublish("Could not publish verified export directory");
    if (request.failurePoint == ExportFailurePoint::AfterFirstPublishedFile)
        return unpublish("Injected failure during publication");
    if (::rename(editedPath.c_str(), publishedEdited.c_str()) != 0 ||
        ::rename(manifestPath.c_str(), publishedManifest.c_str()) != 0)
        return unpublish("Could not publish verified export directory");
    ::rmdir(temporaryDirectory.c_str());

    result.success = true;
    result.directory = finalDirectory;
    return result;
}

ExportTransactionResult publishVerifiedStagedEditorExport(
    const StagedEditor& editor, const StagedEditorExportRequest& request) {
    ExportTransactionResult result;
    if (!editor.hasPendingChanges()) {
        result.error = "No pending changes to export";
        return result;
    }

    std::string finalizeError;
    const auto edited = editor.finalizedBytes(finalizeError);
    if (edited.empty()) {
        result.error = finalizeError.empty() ? "Staged save finalization failed" : finalizeError;
        return result;
    }

    ExportTransactionRequest transaction;
    transaction.rootDirectory = request.rootDirectory;
    transaction.directoryStem = request.directoryStem;
    transaction.sourcePath = request.sourcePath;
    transaction.gameId = request.gameId;
    transaction.saveFormat = request.saveFormat;
    transaction.timestamp = request.timestamp;
    transaction.applicationSha = request.applicationSha;
    transaction.sourceGame = editor.metadata().sourceGame;
    transaction.originalBytes = editor.originalBytes();
    transaction.editedBytes = edited;
    transaction.pendingChanges = editor.pendingChanges();
    return publishVerifiedExport(transaction);
}

} // namespace PokeVault::Integration::Gen2
