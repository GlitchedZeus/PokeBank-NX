#pragma once

// What a Gen IV source FILE looked like when it was opened, so a later export can prove the file
// it backs up is still the one that was edited. A .dsv source is the 0x80000-byte payload followed
// by DeSmuME's documented 40-byte footer (see readSourcePayloadReadOnly); the editor only ever
// sees the payload, so the container is recorded here rather than inferred from the edited bytes.
//
// Header-only on purpose: every target that links Gen4ExportTransaction.cpp already links
// VerifiedStagedExport.cpp (sha256Hex), and nothing here writes to the file.

#include "Integration/Common/VerifiedStagedExport.h"

#include <sys/stat.h>

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace PokeVault::Integration::Gen4 {

enum class Gen4SourceContainer : uint8_t {
    Raw, // the file is exactly the 0x80000-byte payload
    Dsv, // payload followed by the DeSmuME .dsv footer
};

struct Gen4SourceSnapshot {
    Gen4SourceContainer container = Gen4SourceContainer::Raw;
    uint64_t fileSize = 0;
    int64_t modifiedSeconds = 0;
    std::string fileSha256;
};

namespace detail {

inline bool readWholeSourceFile(const std::string& path, std::vector<uint8_t>& out,
                                uint64_t& size, int64_t& modified) {
    struct stat st{};
    if (::stat(path.c_str(), &st) != 0 || !S_ISREG(st.st_mode)) return false;
    size = static_cast<uint64_t>(st.st_size);
    modified = static_cast<int64_t>(st.st_mtime);
    FILE* file = std::fopen(path.c_str(), "rb");
    if (!file) return false;
    out.assign(static_cast<size_t>(size), 0);
    const size_t got = size == 0 ? 0 : std::fread(out.data(), 1, out.size(), file);
    // One more byte means the file grew between stat and read.
    uint8_t extra = 0;
    const bool grew = std::fread(&extra, 1, 1, file) != 0;
    std::fclose(file);
    return got == out.size() && !grew;
}

} // namespace detail

// Records the file at Open. `payload` is the save image the editor was created from; it must be
// the file's leading bytes (all of a raw file, the part before a .dsv footer) or the snapshot is
// refused, so a snapshot can never describe a different file than the one being edited.
[[nodiscard]] inline std::optional<Gen4SourceSnapshot> snapshotGen4Source(
    const std::string& path, Gen4SourceContainer container, std::span<const uint8_t> payload,
    std::string* error = nullptr) {
    auto fail = [&](const char* message) -> std::optional<Gen4SourceSnapshot> {
        if (error) *error = message;
        return std::nullopt;
    };
    std::vector<uint8_t> bytes;
    Gen4SourceSnapshot snapshot;
    snapshot.container = container;
    if (!detail::readWholeSourceFile(path, bytes, snapshot.fileSize, snapshot.modifiedSeconds))
        return fail("Source save could not be read for its Open snapshot");
    if (payload.empty() || bytes.size() < payload.size() ||
        !std::equal(payload.begin(), payload.end(), bytes.begin()))
        return fail("Source save does not begin with the opened Gen IV payload");
    if (container == Gen4SourceContainer::Raw && bytes.size() != payload.size())
        return fail("Raw Gen IV source has bytes beyond its payload");
    if (container == Gen4SourceContainer::Dsv && bytes.size() == payload.size())
        return fail(".dsv Gen IV source has no container footer");
    snapshot.fileSha256 = Common::sha256Hex(bytes);
    if (error) error->clear();
    return snapshot;
}

// Re-reads the source and fails closed unless its size, modification time and SHA-256 all match
// the Open snapshot and it still begins with `payload`. On success `fileBytes` (when given) holds
// the verbatim file, container included.
[[nodiscard]] inline bool gen4SourceUnchangedSinceOpen(
    const std::string& path, const Gen4SourceSnapshot& snapshot, std::span<const uint8_t> payload,
    std::string* error = nullptr, std::vector<uint8_t>* fileBytes = nullptr) {
    auto fail = [&](const char* message) {
        if (error) *error = message;
        return false;
    };
    std::vector<uint8_t> bytes;
    uint64_t size = 0;
    int64_t modified = 0;
    if (!detail::readWholeSourceFile(path, bytes, size, modified))
        return fail("Source save is missing or unreadable; reopen it before exporting");
    if (size != snapshot.fileSize || modified != snapshot.modifiedSeconds ||
        Common::sha256Hex(bytes) != snapshot.fileSha256)
        return fail("Source save changed after it was opened; reopen it before exporting");
    if (bytes.size() < payload.size() || !std::equal(payload.begin(), payload.end(), bytes.begin()))
        return fail("Source save no longer begins with the edited Gen IV payload");
    if (fileBytes) *fileBytes = std::move(bytes);
    if (error) error->clear();
    return true;
}

[[nodiscard]] inline const char* gen4SourceContainerLabel(Gen4SourceContainer container) noexcept {
    return container == Gen4SourceContainer::Dsv ? "dsv (DeSmuME 40-byte footer)" : "raw .sav";
}

} // namespace PokeVault::Integration::Gen4
