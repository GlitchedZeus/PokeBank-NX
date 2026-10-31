#pragma once
// Favourite folders and remembered last folders (owner Oct 28, Feature C; adapted from PKSE f4fd79a and
// c671aa4 after hardening the settings layer they depend on).
//
// Only plain SD-card folders are accepted: "sdmc:/" followed by components, no "." / ".." components, no
// control bytes, bounded length (SettingsFile::kMaxValue). A folder is stored in canonical form: "//"
// collapsed, the trailing "/" removed except for the root. Duplicates are compared case-insensitively
// because the SD card is FAT (upstream compared case-sensitively, so "a/" and "A" both loaded). A
// remembered folder that no longer exists is reported, never silently dropped: the user may have the
// card in another device. Favourites are a convenience for reading files (Pokemon files, emulator save
// copies); they never grant write access to any save.

#include "Utils/SettingsFile.h"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace Utils::FolderPreferences {

inline constexpr std::string_view kRoot = "sdmc:/";
inline constexpr std::size_t kMaxFavorites = 20;

// Operations that remember their own last folder.
enum class Operation { ImportPokemonFile, ExportPokemonFile, EmulatorSaveCopy };

[[nodiscard]] inline std::string_view settingsKey(Operation op) noexcept {
    switch (op) {
        case Operation::ImportPokemonFile: return "lastImportFolder";
        case Operation::ExportPokemonFile: return "lastExportFolder";
        case Operation::EmulatorSaveCopy: return "lastSaveCopyFolder";
    }
    return {};
}
inline constexpr std::string_view kFavoriteKey = "favoriteFolder";

// Canonical folder or nullopt when the text is not an acceptable SD-card folder.
[[nodiscard]] inline std::optional<std::string> canonical(std::string_view path) {
    if (path.size() < kRoot.size() || path.size() > SettingsFile::kMaxValue) return std::nullopt;
    if (path.substr(0, kRoot.size()) != kRoot) return std::nullopt;
    std::string out(kRoot);
    std::string_view rest = path.substr(kRoot.size());
    while (!rest.empty()) {
        const auto slash = rest.find('/');
        const std::string_view part = rest.substr(0, slash);
        rest = slash == std::string_view::npos ? std::string_view{} : rest.substr(slash + 1);
        if (part.empty()) continue;                         // "//" or a trailing "/"
        if (part == "." || part == "..") return std::nullopt;
        for (unsigned char c : part)
            if (c < 0x20 || c == 0x7F || c == '\\' || c == ':') return std::nullopt;
        if (out.back() != '/') out += '/';
        out.append(part);
    }
    return out;
}

[[nodiscard]] inline bool sameFolder(std::string_view a, std::string_view b) noexcept {
    return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin(), [](unsigned char x, unsigned char y) {
        return std::tolower(x) == std::tolower(y);
    });
}

struct Preferences {
    std::vector<std::string> favorites;              // canonical, unique (case-insensitive), in order added
    std::string lastImport, lastExport, lastSaveCopy; // canonical or empty

    [[nodiscard]] std::string& last(Operation op) noexcept {
        return op == Operation::ImportPokemonFile ? lastImport : op == Operation::ExportPokemonFile ? lastExport : lastSaveCopy;
    }
    [[nodiscard]] const std::string& last(Operation op) const noexcept {
        return op == Operation::ImportPokemonFile ? lastImport : op == Operation::ExportPokemonFile ? lastExport : lastSaveCopy;
    }
    [[nodiscard]] bool isFavorite(std::string_view folder) const {
        const auto c = canonical(folder);
        return c && std::any_of(favorites.begin(), favorites.end(), [&](const std::string& f) { return sameFolder(f, *c); });
    }
    // Adds or removes a favourite. Returns false (and changes nothing) for an unacceptable path or a full list.
    bool toggleFavorite(std::string_view folder) {
        const auto c = canonical(folder);
        if (!c) return false;
        const auto it = std::find_if(favorites.begin(), favorites.end(), [&](const std::string& f) { return sameFolder(f, *c); });
        if (it != favorites.end()) { favorites.erase(it); return true; }
        if (favorites.size() >= kMaxFavorites) return false;
        favorites.push_back(*c);
        return true;
    }
    // Remembers the folder an operation last used; an unacceptable path is ignored.
    bool remember(Operation op, std::string_view folder) {
        const auto c = canonical(folder);
        if (!c) return false;
        last(op) = *c;
        return true;
    }
};

// Reads the preference keys from parsed settings entries; anything malformed is skipped.
[[nodiscard]] inline Preferences fromSettings(const std::vector<SettingsFile::Entry>& entries) {
    Preferences p;
    for (const auto& e : entries) {
        if (e.key == kFavoriteKey) {
            const auto c = canonical(e.value);
            if (c && !p.isFavorite(*c) && p.favorites.size() < kMaxFavorites) p.favorites.push_back(*c);
        } else {
            for (const auto op : {Operation::ImportPokemonFile, Operation::ExportPokemonFile, Operation::EmulatorSaveCopy})
                if (e.key == settingsKey(op)) p.remember(op, e.value);
        }
    }
    return p;
}

// key=value lines for settings.cfg (appended after the existing keys by saveSettings).
[[nodiscard]] inline std::string toSettingsText(const Preferences& p) {
    std::string text;
    for (const auto op : {Operation::ImportPokemonFile, Operation::ExportPokemonFile, Operation::EmulatorSaveCopy}) {
        const auto& v = p.last(op);
        if (!v.empty()) { text += settingsKey(op); text += '='; text += v; text += '\n'; }
    }
    for (const auto& f : p.favorites) { text += kFavoriteKey; text += '='; text += f; text += '\n'; }
    return text;
}

// Status of a remembered folder at the moment it is offered.
enum class FolderStatus { Present, Missing };
template <class Exists>
[[nodiscard]] FolderStatus status(std::string_view folder, Exists&& exists) {
    return !folder.empty() && exists(std::string(folder)) ? FolderStatus::Present : FolderStatus::Missing;
}

}  // namespace Utils::FolderPreferences
