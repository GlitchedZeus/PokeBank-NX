#pragma once
// SD-card side of standalone Pokemon files (owner Oct 28, Feature A UI). Header-only POSIX
// (dirent / stat / stdio), so the same code runs on the Switch and in host tests.
//
// * listFolder() reads one directory listing for UI/PokemonFileBrowser.h list(). Nothing is written.
// * readPokemonFile() refuses a file larger than PokemonFileBrowser::kMaxFileBytes from its size
//   alone, BEFORE opening it, and never reads more than that many bytes plus one.
// * exportPokemonFile() writes ONE new file under PokemonFileBrowser::exportFolder(generation)
//   (folders created when missing), named baseName(species, id) + plainExtension(generation) at the
//   first free name (freeExportPath: never replaces a file), through the app's durable writer
//   (Utils/DurableFile.h: temp file, reread, validate, promote, reread). The file is then read back
//   and must decode to the same record. It never touches a save or the workspace.

#include "PokemonFile/NativePokemonFile.h"
#include "UI/PokemonFileBrowser.h"
#include "UI/PokemonFileDialogModel.h"
#include "Utils/DurableFile.h"
#include "Utils/PokeBankPaths.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <dirent.h>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <sys/stat.h>
#include <vector>

namespace PokeBank::UIModel::PokemonFileIo {

namespace Browser = PokemonFileBrowser;

// Host-test probe: how many files readPokemonFile() has opened (an oversized file must not count).
inline std::size_t& openedFileCount() noexcept {
    static std::size_t count = 0;
    return count;
}

[[nodiscard]] inline bool pathExists(const std::string& path) noexcept {
    struct stat st{};
    return !path.empty() && ::stat(path.c_str(), &st) == 0;
}
[[nodiscard]] inline bool folderExists(const std::string& path) noexcept {
    return PokeBank::Paths::directoryExists(path);
}

// One folder's entries ("." / ".." left out). The listing is capped well above what the browser
// shows (PokemonFileBrowser::kMaxEntries) so a huge folder cannot exhaust memory.
[[nodiscard]] inline std::vector<Browser::RawEntry> listFolder(const std::string& folder, std::string* error = nullptr) {
    std::vector<Browser::RawEntry> out;
    DIR* dir = ::opendir(folder.c_str());
    if (!dir) {
        if (error) *error = "Could not open folder " + folder;
        return out;
    }
    while (dirent* ent = ::readdir(dir)) {
        const std::string name = ent->d_name;
        if (name == "." || name == "..") continue;
        struct stat st{};
        const std::string path = PokemonFileDialog::joinPath(folder, name);
        if (::stat(path.c_str(), &st) != 0) continue;
        if (!S_ISDIR(st.st_mode) && !S_ISREG(st.st_mode)) continue;
        out.push_back({name, S_ISDIR(st.st_mode), S_ISREG(st.st_mode) ? static_cast<std::size_t>(st.st_size) : 0});
        if (out.size() >= 4 * Browser::kMaxEntries) break;
    }
    ::closedir(dir);
    if (error) error->clear();
    return out;
}

// Reads one candidate Pokemon file. A file over kMaxFileBytes is refused from its size alone.
[[nodiscard]] inline std::optional<std::vector<uint8_t>> readPokemonFile(const std::string& path, std::string& error) {
    struct stat st{};
    if (::stat(path.c_str(), &st) != 0 || !S_ISREG(st.st_mode)) {
        error = "The file could not be found: " + path;
        return std::nullopt;
    }
    const auto slash = path.find_last_of('/');
    const std::string name = slash == std::string::npos ? path : path.substr(slash + 1);
    if (static_cast<std::size_t>(st.st_size) > Browser::kMaxFileBytes) {
        error = PokemonFileDialog::tooLargeMessage(name, static_cast<std::size_t>(st.st_size));
        return std::nullopt;
    }
    std::FILE* file = std::fopen(path.c_str(), "rb");
    if (!file) {
        error = "The file could not be opened: " + name;
        return std::nullopt;
    }
    ++openedFileCount();
    std::vector<uint8_t> bytes(Browser::kMaxFileBytes + 1);
    const std::size_t got = std::fread(bytes.data(), 1, bytes.size(), file);
    const bool failed = std::ferror(file) != 0;
    std::fclose(file);
    if (failed) {
        error = "The file could not be read: " + name;
        return std::nullopt;
    }
    if (got > Browser::kMaxFileBytes) {   // it grew after the size check
        error = PokemonFileDialog::tooLargeMessage(name, got);
        return std::nullopt;
    }
    bytes.resize(got);
    error.clear();
    return bytes;
}

// Reads and decodes one file the user picked.
[[nodiscard]] inline std::optional<PokeBank::PokemonFile::Decoded> loadPokemonFile(const std::string& path,
                                                                                std::string& error) {
    const auto bytes = readPokemonFile(path, error);
    if (!bytes) return std::nullopt;
    const auto slash = path.find_last_of('/');
    const std::string name = slash == std::string::npos ? path : path.substr(slash + 1);
    return PokeBank::PokemonFile::decode(*bytes, name, error);
}

[[nodiscard]] inline bool sameRecord(const PokeBank::PokemonFile::Decoded& a, const PokeBank::PokemonFile::Decoded& b) noexcept {
    return a.format == b.format && a.generation == b.generation && a.party == b.party &&
           a.japanese == b.japanese && a.plain == b.plain;
}

struct ExportResult {
    bool ok = false;
    std::string path;
    std::string error;
};

[[nodiscard]] inline ExportResult exportPokemonFile(int generation, std::string_view species,
                                                    std::span<const uint8_t> bytes) {
    namespace PF = PokeBank::PokemonFile;
    ExportResult result;
    if (generation < 1 || generation > 5 || bytes.empty()) {
        result.error = "Nothing to export";
        return result;
    }
    const std::string extension(PF::plainExtension(generation));
    const std::string base = Browser::baseName(species, PokemonFileDialog::fileId(generation, bytes.data(), bytes.size()));
    // The record must already be a file decode() accepts before anything is written.
    std::string error;
    const auto expected = PF::decode(bytes, base + extension, error);
    if (!expected || expected->generation != generation || expected->wasEncrypted) {
        result.error = error.empty() ? "The exported record did not verify; no file was written" : error;
        return result;
    }
    const std::string folder = Browser::exportFolder(generation);
    if (!PokeBank::Paths::ensureDirectoryTree(folder, &error)) {
        result.error = error.empty() ? "Could not create " + folder : error;
        return result;
    }
    const auto path = Browser::freeExportPath(folder, base, extension, [](const std::string& p) { return pathExists(p); });
    if (!path) {
        result.error = "Too many exports of this Pokemon in " + folder;
        return result;
    }
    const std::string fileName = path->substr(folder.size() + 1);
    const auto validator = [&](std::span<const uint8_t> written, std::string& why) {
        std::string decodeError;
        const auto again = PF::decode(written, fileName, decodeError);
        if (!again || !sameRecord(*again, *expected)) {
            why = decodeError.empty() ? "The written file does not decode to the exported Pokemon" : decodeError;
            return false;
        }
        return true;
    };
    const auto written = PokeBank::Storage::DurableFile::replace(*path, bytes, validator);
    if (!written.ok) {
        result.error = "Export failed: " + (written.error.empty() ? std::string("write error") : written.error);
        return result;
    }
    // Read the promoted file back through the same reader Import uses.
    const auto reread = loadPokemonFile(*path, error);
    if (!reread || !sameRecord(*reread, *expected)) {
        result.error = "The exported file did not read back as the same Pokemon: " + *path;
        return result;
    }
    result.ok = true;
    result.path = *path;
    return result;
}

}  // namespace PokeBank::UIModel::PokemonFileIo
