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

[[nodiscard]] inline Listing list(std::string_view folder, const std::vector<RawEntry>& raw,
                                  const Prefs::Preferences& prefs) {
    Listing out;
    out.folder = Prefs::canonical(folder).value_or(std::string(Prefs::kRoot));
    out.favorite = prefs.isFavorite(out.folder);
    std::vector<Entry> folders, files;
    for (const auto& r : raw) {
        if (r.name.empty() || r.name.front() == '.') continue;          // hidden / self / parent
        if (r.directory) {
            if (childOf(out.folder, r.name)) folders.push_back({Entry::Kind::Folder, r.name, 0, false});
        } else if (PokeBank::PokemonFile::supportedFileName(r.name)) {
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
