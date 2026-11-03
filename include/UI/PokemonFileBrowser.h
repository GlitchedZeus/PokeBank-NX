#pragma once
// Pokemon file browser model (owner Oct 28, Feature A UI; adapted from PKSE PR #128's file chooser).
//
// Import reads ONE file the user picks anywhere on the SD card; it never writes there. Export writes
// a NEW file only under PokeBank NX's own folder (sdmc:/switch/PokeBank-NX/pokemon/genN) and never
// replaces an existing file: a clash gets " (2)", " (3)" ... like a desktop file manager.
//
// Pure model: the caller supplies directory listings, so every rule here is host-tested. Folder
// favourites / remembered folders come from Utils::FolderPreferences (read convenience only).

#include "PokemonFile/NativePokemonFile.h"
#include "Utils/FolderPreferences.h"
#include "Utils/PokeBankPaths.h"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace PokeBank::UIModel::PokemonFileBrowser {

namespace Prefs = Utils::FolderPreferences;

// A file larger than the largest native record (Gen IV party, 236 bytes) cannot be one; reading it
// at all is refused before any bytes are loaded.
inline constexpr std::size_t kMaxFileBytes = 236;
inline constexpr std::size_t kMaxEntries = 500;   // a folder listing is cut here (and says so)

struct RawEntry { std::string name; bool directory = false; std::size_t size = 0; };

struct Entry {
    enum class Kind : uint8_t { Parent, Folder, File };
    Kind kind = Kind::File;
    std::string name;       // display name ("..", folder or file name)
    std::size_t size = 0;
    bool tooLarge = false;  // listed (so the user sees it) but refused on open
};

struct Listing {
    std::string folder;                 // canonical
    std::vector<Entry> entries;         // Parent first, then folders, then files (each A-Z, case-insensitive)
    bool truncated = false;
    bool favorite = false;
};

[[nodiscard]] inline std::string lower(std::string_view s) {
    std::string out(s);
    std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return out;
}

[[nodiscard]] inline bool isRoot(std::string_view folder) noexcept { return folder == Prefs::kRoot; }

// Parent of a canonical folder; the root has none.
[[nodiscard]] inline std::optional<std::string> parentOf(std::string_view folder) {
    if (isRoot(folder)) return std::nullopt;
    const auto slash = folder.find_last_of('/');
    if (slash == std::string_view::npos || slash < Prefs::kRoot.size() - 1) return std::string(Prefs::kRoot);
    const std::string parent(folder.substr(0, slash));
    return parent.size() < Prefs::kRoot.size() ? std::string(Prefs::kRoot) : parent;
}

// A child folder of `folder`, canonical, or nullopt for an unacceptable name.
[[nodiscard]] inline std::optional<std::string> childOf(std::string_view folder, std::string_view name) {
    if (name.empty() || name == "." || name == ".." || name.find('/') != std::string_view::npos) return std::nullopt;
    std::string joined(folder);
    if (joined.back() != '/') joined += '/';
    joined += name;
    return Prefs::canonical(joined);
}

// Show only native record extensions appropriate for the active game, while still
// allowing directory navigation. 0 preserves the all-generation listing for callers
// without an active save.
[[nodiscard]] inline bool matchesGeneration(std::string_view name, int generation) {
    if (!PokeBank::PokemonFile::supportedFileName(name)) return false;
    if (generation == 0) return true;
    if (generation < 1 || generation > 5) return false;
    const std::string file = lower(name);
    const std::string number = std::to_string(generation);
    const auto ends = [&](const std::string& suffix) {
        return file.size() >= suffix.size() &&
               file.compare(file.size() - suffix.size(), suffix.size(), suffix) == 0;
    };
    return ends(".pk" + number) || (generation >= 3 && ends(".ek" + number));
}

[[nodiscard]] inline Listing list(std::string_view folder, const std::vector<RawEntry>& raw,
                                  const Prefs::Preferences& prefs, int generation = 0) {
    Listing out;
    out.folder = Prefs::canonical(folder).value_or(std::string(Prefs::kRoot));
    out.favorite = prefs.isFavorite(out.folder);
    std::vector<Entry> folders, files;
    for (const auto& r : raw) {
        if (r.name.empty() || r.name.front() == '.') continue;          // hidden / self / parent
        if (r.directory) {
            if (childOf(out.folder, r.name)) folders.push_back({Entry::Kind::Folder, r.name, 0, false});
        } else if (matchesGeneration(r.name, generation)) {
            files.push_back({Entry::Kind::File, r.name, r.size, r.size > kMaxFileBytes});
        }
    }
    const auto byName = [](const Entry& a, const Entry& b) { return lower(a.name) < lower(b.name); };
    std::sort(folders.begin(), folders.end(), byName);
    std::sort(files.begin(), files.end(), byName);
    if (!isRoot(out.folder)) out.entries.push_back({Entry::Kind::Parent, "..", 0, false});
    for (auto& f : folders) out.entries.push_back(std::move(f));
    for (auto& f : files) out.entries.push_back(std::move(f));
    if (out.entries.size() > kMaxEntries) {
        out.entries.resize(kMaxEntries);
        out.truncated = true;
    }
    return out;
}

// Where Import opens: the remembered import folder if it still exists, else the app's pokemon folder
// (created on first export), else the SD root.
template <class Exists>
[[nodiscard]] std::string startFolder(const Prefs::Preferences& prefs, Exists&& exists) {
    const auto& last = prefs.last(Prefs::Operation::ImportPokemonFile);
    if (!last.empty() && Prefs::status(last, exists) == Prefs::FolderStatus::Present) return last;
    const std::string app = PokeBank::Paths::root() + "/pokemon";
    if (exists(app)) return app;
    return std::string(Prefs::kRoot);
}

// ---- Export naming ---------------------------------------------------------------------------
[[nodiscard]] inline std::string exportFolder(int generation) {
    return PokeBank::Paths::root() + "/pokemon/gen" + std::to_string(generation);
}

// A remembered Gen I folder must not become the default for a Gen V import.
// Keep legitimate custom folders and favourites, but skip another generation's
// PokeBank export folder. Do not create folders just to browse.
template <class Exists>
[[nodiscard]] std::string startFolderForGeneration(const Prefs::Preferences& prefs, int generation,
                                                   Exists&& exists) {
    if (generation < 1 || generation > 5) return startFolder(prefs, exists);
    const auto& last = prefs.last(Prefs::Operation::ImportPokemonFile);
    const std::string remembered = lower(last);
    bool otherGeneration = false;
    for (int other = 1; other <= 5; ++other) {
        if (other == generation) continue;
        const std::string folder = lower(exportFolder(other));
        if (remembered == folder ||
            (remembered.size() > folder.size() &&
             remembered.compare(0, folder.size(), folder) == 0 &&
             remembered[folder.size()] == '/')) {
            otherGeneration = true;
            break;
        }
    }
    if (!last.empty() && !otherGeneration &&
        Prefs::status(last, exists) == Prefs::FolderStatus::Present) return last;
    const std::string own = exportFolder(generation);
    if (exists(own)) return own;
    const std::string app = PokeBank::Paths::root() + "/pokemon";
    if (exists(app)) return app;
    return std::string(Prefs::kRoot);
}

// "<Species>_<id>" with only filename-safe ASCII; the id is the PID (Gen III-V) or the DV word
// (Gen I / II) in hex, so two exports of different Pokemon of one species do not collide.
[[nodiscard]] inline std::string baseName(std::string_view species, uint32_t id) {
    std::string out;
    for (const unsigned char c : species) {
        if (std::isalnum(c)) out += static_cast<char>(c);
        else if (c == ' ' || c == '-' || c == '.' || c == '\'') out += '_';
        // anything else (accented letters, symbols) is left out of the file name only
    }
    if (out.empty()) out = "Pokemon";
    char hex[9];
    static constexpr char kDigits[] = "0123456789ABCDEF";
    for (int i = 7; i >= 0; --i) hex[7 - i] = kDigits[(id >> (i * 4)) & 0xF];
    hex[8] = '\0';
    return out + "_" + hex;
}

// The first free name: "<base><ext>", then "<base> (2)<ext>", ... ; nullopt after 999 clashes.
template <class Exists>
[[nodiscard]] std::optional<std::string> freeExportPath(std::string_view folder, std::string_view base,
                                                       std::string_view extension, Exists&& exists) {
    for (int n = 1; n <= 999; ++n) {
        std::string path(folder);
        path += '/';
        path += base;
        if (n > 1) path += " (" + std::to_string(n) + ")";
        path += extension;
        if (!exists(path)) return path;
    }
    return std::nullopt;
}

}  // namespace PokeBank::UIModel::PokemonFileBrowser
