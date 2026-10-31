// VerifiedStagedExport optional verbatim source file + container facts (Gen IV .dsv backup).
// Unset: the backup and manifest are byte-identical to the established export. Set: the backup
// is the source FILE verbatim (payload + emulator metadata + footer) with SOURCE_FILE_* and the
// Gen V container keys; payload facts stay on the payload. A file that does not wrap the
// payload, an unsafe container token, or a backup corrupted on disk all fail closed with no
// published directory. Synthetic bytes only.
#include "Integration/Common/VerifiedStagedExport.h"

#include <cassert>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <span>
#include <string>
#include <unistd.h>
#include <vector>

namespace fs = std::filesystem;
namespace C = PokeVault::Integration::Common;

namespace {

std::vector<uint8_t> slurp(const fs::path& path) {
    std::ifstream in(path, std::ios::binary);
    assert(in);
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}
std::string text(const fs::path& path) {
    const auto bytes = slurp(path);
    return {bytes.begin(), bytes.end()};
}

struct Fixture {
    std::vector<uint8_t> payload = std::vector<uint8_t>(0x400, 0x11);
    std::vector<uint8_t> edited;
    std::vector<uint8_t> file; // payload + metadata + footer
    Fixture() {
        edited = payload;
        edited[0x10] = 0x22;
        edited[0x11] = 0x33;
        file = payload;
        for (int i = 0; i < 82; ++i) file.push_back(static_cast<uint8_t>(0xA0 + i % 16));
        const char marker[] = "|-DESMUME SAVE-|";
        for (int i = 0; i < 24; ++i) file.push_back(0);
        file.insert(file.end(), marker, marker + 16);
    }
    C::VerifiedExportRequest request(const fs::path& root, const char* stem) const {
        C::VerifiedExportRequest r;
        r.rootDirectory = root.string();
        r.directoryStem = stem;
        r.generationLabel = "Generation IV";
        r.fileExtension = ".sav";
        r.sourcePath = "/synthetic/source.dsv";
        r.gameId = "platinum_nds";
        r.saveFormat = "SYNTHETIC";
        r.timestamp = "20261015_103000";
        r.applicationSha = "host";
        r.originalBytes = payload;
        r.editedBytes = edited;
        r.changeLines = {"Box 1 slot 1: edited"};
        r.validator = [](std::span<const uint8_t> bytes, std::string& error) {
            if (bytes.size() != 0x400) { error = "size"; return false; }
            return true;
        };
        return r;
    }
};

void unsetIsByteIdentical(const fs::path& root, const Fixture& f) {
    const auto result = C::publishVerifiedStagedExport(f.request(root, "unset"));
    assert(result.success && result.backupSha256 == result.originalSha256);
    const fs::path dir = result.directory;
    assert(slurp(dir / "original_backup.sav") == f.payload);
    assert(slurp(dir / "edited.sav") == f.edited);
    const std::string expected =
        "PokeBank NX Generation IV verified staged export\n"
        "GAME_ID=platinum_nds\n"
        "SAVE_FORMAT=SYNTHETIC\n"
        "SOURCE_PATH=/synthetic/source.dsv\n"
        "SOURCE_KIND=READ_ONLY\n"
        "SOURCE_SIZE=1024\n"
        "SOURCE_SHA256=" + result.originalSha256 + "\n"
        "TIMESTAMP=20261015_103000\n"
        "APPLICATION_SHA=host\n"
        "ORIGINAL_BACKUP=original_backup.sav\n"
        "EDITED_SAVE=edited.sav\n"
        "EDITED_SIZE=1024\n"
        "EDITED_SHA256=" + result.editedSha256 + "\n"
        "CHANGED_BYTES=2\n"
        "WRITE_READBACK=VERIFIED\n"
        "STRICT_REPARSE=VERIFIED\n"
        "LIVE_EMULATOR_WRITE=DISABLED\n"
        "LIVE_INSTALLED_GAME_WRITE=DISABLED\n"
        "PENDING_CHANGES:\n"
        "- Box 1 slot 1: edited\n";
    assert(text(dir / "EDIT_MANIFEST.txt") == expected);
}

void verbatimBackupAndContainerFacts(const fs::path& root, const Fixture& f) {
    auto r = f.request(root, "verbatim");
    r.fileExtension = ".dsv";
    r.editedFileExtension = ".sav"; // the raw payload must not look like a .dsv container
    r.backupBytes = f.file;
    r.sourceContainer = "dsv-footer";
    r.editedContainer = "raw-nds-battery";
    r.sourceSha256Scope = "0x400_SYNTHETIC_PAYLOAD";
    const auto result = C::publishVerifiedStagedExport(r);
    assert(result.success);
    const fs::path dir = result.directory;
    assert(slurp(dir / "original_backup.dsv") == f.file);   // verbatim, footer included
    assert(slurp(dir / "edited.sav") == f.edited);          // edited stays the raw payload, .sav
    assert(!fs::exists(dir / "edited.dsv"));
    assert(result.backupSha256 == C::sha256Hex(f.file));
    assert(result.originalSha256 == C::sha256Hex(f.payload));
    const auto manifest = text(dir / "EDIT_MANIFEST.txt");
    for (const std::string& line : {
             std::string("SOURCE_SIZE=1024\n"),
             "SOURCE_SHA256=" + result.originalSha256 + "\n",
             std::string("SOURCE_SHA256_SCOPE=0x400_SYNTHETIC_PAYLOAD\n"),
             std::string("SOURCE_CONTAINER=dsv-footer\n"),
             "SOURCE_FILE_SIZE=" + std::to_string(f.file.size()) + "\n",
             "SOURCE_FILE_SHA256=" + result.backupSha256 + "\n",
             std::string("ORIGINAL_BACKUP=original_backup.dsv\n"),
             std::string("EDITED_SAVE=edited.sav\n"),
             "ORIGINAL_BACKUP_SHA256=" + result.backupSha256 + "\n",
             std::string("ORIGINAL_BACKUP_IS=VERBATIM_SOURCE_FILE\n"),
             std::string("EDITED_CONTAINER=raw-nds-battery\n"),
             std::string("CHANGED_BYTES=2\n")})
        assert(manifest.find(line) != std::string::npos);
}

void refusalsFailClosed(const fs::path& root, const Fixture& f) {
    const auto entries = [&] {
        size_t n = 0;
        for ([[maybe_unused]] const auto& e : fs::directory_iterator(root)) ++n;
        return n;
    };
    const size_t before = entries();
    // A file that does not start with the payload is refused before any I/O.
    auto r = f.request(root, "mismatch");
    auto wrong = f.file;
    wrong[0] ^= 0xFF;
    r.backupBytes = wrong;
    assert(!C::publishVerifiedStagedExport(r).success);
    // A file shorter than the payload is refused.
    const std::vector<uint8_t> shorter(f.file.begin(), f.file.begin() + 0x100);
    r.backupBytes = shorter;
    assert(!C::publishVerifiedStagedExport(r).success);
    // An unsafe edited extension is refused.
    r = f.request(root, "edited-ext");
    r.editedFileExtension = "../x";
    assert(!C::publishVerifiedStagedExport(r).success);
    // Container facts are tokens only; no manifest line injection.
    r = f.request(root, "token");
    r.sourceContainer = "dsv\nSOURCE_SHA256=forged";
    assert(!C::publishVerifiedStagedExport(r).success);
    // A backup corrupted on disk fails read-back, verbatim or not, and leaves nothing behind.
    for (const bool verbatim : {false, true}) {
        r = f.request(root, verbatim ? "corrupt-verbatim" : "corrupt-payload");
        if (verbatim) r.backupBytes = f.file;
        r.failurePoint = C::ExportFailurePoint::CorruptBackupAfterWrite;
        const auto result = C::publishVerifiedStagedExport(r);
        assert(!result.success && result.error.find("read-back") != std::string::npos);
    }
    assert(entries() == before);
}

} // namespace

int main() {
    char pattern[] = "/tmp/pokebank_verbatim_export_XXXXXX";
    assert(::mkdtemp(pattern));
    const fs::path root = pattern;
    const Fixture f;
    unsetIsByteIdentical(root, f);
    verbatimBackupAndContainerFacts(root, f);
    refusalsFailClosed(root, f);
    fs::remove_all(root);
    std::cout << "Verified export: unset byte-identical, verbatim source file backup with "
                 "container facts, refusals and corrupted read-back fail closed PASS\n";
}
