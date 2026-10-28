#pragma once
// Pokemon file chooser: the pure input model of the Import from file dialog (owner Oct 28,
// Feature A UI; UI/PokemonFileDialog.h draws it and runs its commands).
//
// The dialog lists ONE SD-card folder at a time (UI/PokemonFileBrowser.h list()) and never writes
// there. Controls:
//   Up / Down        move (wraps)            L / R   one page up / down
//   A                open the focused folder (".." goes up) or pick the focused file
//   B                go up one folder; at the start folder (or the SD root) close the dialog
//   Y                add / remove the current folder as a favourite
//   X                show the favourite folders (A opens one, Y removes it, B / X return)
// A file larger than the largest native record (PokemonFileBrowser::kMaxFileBytes) is listed so the
// user sees it, but picking it is refused before any byte is read.
//
// Pure: no file system, no drawing. Host-tested by tests/test_pokemon_file_dialog_model.cpp.

#include "UI/PokemonFileBrowser.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace PokeBank::UIModel::PokemonFileDialog {

namespace Browser = PokemonFileBrowser;
namespace Prefs = Utils::FolderPreferences;

enum class Key : uint8_t { None, Up, Down, PageUp, PageDown, A, B, X, Y };
enum class Mode : uint8_t { Browse, Favorites };

struct State {
    std::string startFolder;      // where the dialog opened; B there closes it
    Browser::Listing listing;     // the folder on screen
    int cursor = 0;               // focused entry of listing.entries
    int first = 0;                // first visible entry (kept so the cursor stays visible)
    Mode mode = Mode::Browse;
    int favoriteCursor = 0;
};

enum class Command : uint8_t {
    None,
    OpenFolder,       // list `path` (focus `focus` there when it is listed)
    PickFile,         // read, decode and stage `path`
    RefuseFile,       // `message` says why; nothing is read
    Close,
    ToggleFavorite,   // `path` is added to / removed from the favourites
};

struct Outcome {
    Command command = Command::None;
    std::string path;
    std::string focus;
    std::string message;
};

// "<folder>/<name>" for a canonical folder ("sdmc:/" + "a.pk3" -> "sdmc:/a.pk3").
[[nodiscard]] inline std::string joinPath(std::string_view folder, std::string_view name) {
    std::string out(folder);
    if (out.empty() || out.back() != '/') out += '/';
    out += name;
    return out;
}

[[nodiscard]] inline std::string tooLargeMessage(std::string_view name, std::size_t size) {
    return std::string(name) + " is " + std::to_string(size) + " bytes; a Pokemon file is at most " +
           std::to_string(Browser::kMaxFileBytes) + " bytes, so it was not opened";
}

// Status after a successful staged import. Box / slot are zero-based.
[[nodiscard]] inline std::string importedStatus(std::string_view species, int box, int slot) {
    return "Imported " + std::string(species) + " into Box " + std::to_string(box + 1) + " / Slot " +
           std::to_string(slot + 1) + " (staged; the source save is unchanged)";
}
[[nodiscard]] inline std::string exportedStatus(std::string_view path) {
    return "Exported to " + std::string(path);
}

// Replace the folder on screen. The cursor lands on `focus` when it is listed (going up returns to
// the folder just left), otherwise on the first entry.
inline void setListing(State& s, Browser::Listing listing, std::string_view focus = {}) {
    s.listing = std::move(listing);
    s.cursor = 0;
    s.first = 0;
    for (std::size_t i = 0; i < s.listing.entries.size(); ++i) {
        const auto& e = s.listing.entries[i];
        if (!focus.empty() && e.kind == Browser::Entry::Kind::Folder && Prefs::sameFolder(e.name, focus)) {
            s.cursor = static_cast<int>(i);
            break;
        }
    }
    s.mode = Mode::Browse;
}

// Keep the cursor inside the visible window of `pageRows` rows.
inline void keepVisible(int& first, int cursor, int count, int pageRows) noexcept {
    pageRows = std::max(1, pageRows);
    if (cursor < first) first = cursor;
    if (cursor >= first + pageRows) first = cursor - pageRows + 1;
    first = std::clamp(first, 0, std::max(0, count - pageRows));
}

[[nodiscard]] inline int countIn(const State& s, const Prefs::Preferences& prefs) noexcept {
    return s.mode == Mode::Favorites ? static_cast<int>(prefs.favorites.size())
                                     : static_cast<int>(s.listing.entries.size());
}

// Move the focus (wrapping for single steps, clamping for pages).
inline void move(int& cursor, int count, int delta, bool wrap) noexcept {
    if (count <= 0) { cursor = 0; return; }
    if (wrap) cursor = ((cursor + delta) % count + count) % count;
    else cursor = std::clamp(cursor + delta, 0, count - 1);
}

[[nodiscard]] inline Outcome press(State& s, Key key, const Prefs::Preferences& prefs, int pageRows) {
    Outcome out;
    pageRows = std::max(1, pageRows);
    if (s.mode == Mode::Favorites) {
        const int count = static_cast<int>(prefs.favorites.size());
        s.favoriteCursor = std::clamp(s.favoriteCursor, 0, std::max(0, count - 1));
        switch (key) {
            case Key::Up: move(s.favoriteCursor, count, -1, true); break;
            case Key::Down: move(s.favoriteCursor, count, 1, true); break;
            case Key::PageUp: move(s.favoriteCursor, count, -pageRows, false); break;
            case Key::PageDown: move(s.favoriteCursor, count, pageRows, false); break;
            case Key::B:
            case Key::X: s.mode = Mode::Browse; break;
            case Key::A:
                if (count > 0) {
                    s.mode = Mode::Browse;
                    out.command = Command::OpenFolder;
                    out.path = prefs.favorites[static_cast<std::size_t>(s.favoriteCursor)];
                }
                break;
            case Key::Y:
                if (count > 0) {
                    out.command = Command::ToggleFavorite;
                    out.path = prefs.favorites[static_cast<std::size_t>(s.favoriteCursor)];
                }
                break;
            case Key::None: break;
        }
        return out;
    }

    const int count = static_cast<int>(s.listing.entries.size());
    s.cursor = std::clamp(s.cursor, 0, std::max(0, count - 1));
    switch (key) {
        case Key::Up: move(s.cursor, count, -1, true); break;
        case Key::Down: move(s.cursor, count, 1, true); break;
        case Key::PageUp: move(s.cursor, count, -pageRows, false); break;
        case Key::PageDown: move(s.cursor, count, pageRows, false); break;
        case Key::X:
            s.mode = Mode::Favorites;
            s.favoriteCursor = 0;
            break;
        case Key::Y:
            out.command = Command::ToggleFavorite;
            out.path = s.listing.folder;
            break;
        case Key::B: {
            const auto parent = Browser::parentOf(s.listing.folder);
            if (!parent || Prefs::sameFolder(s.listing.folder, s.startFolder)) {
                out.command = Command::Close;
            } else {
                out.command = Command::OpenFolder;
                out.path = *parent;
                const auto slash = s.listing.folder.find_last_of('/');
                out.focus = slash == std::string::npos ? std::string{} : s.listing.folder.substr(slash + 1);
            }
            break;
        }
        case Key::A: {
            if (count <= 0) break;
            const auto& e = s.listing.entries[static_cast<std::size_t>(s.cursor)];
            if (e.kind == Browser::Entry::Kind::Parent) {
                if (const auto parent = Browser::parentOf(s.listing.folder)) {
                    out.command = Command::OpenFolder;
                    out.path = *parent;
                    const auto slash = s.listing.folder.find_last_of('/');
                    out.focus = slash == std::string::npos ? std::string{} : s.listing.folder.substr(slash + 1);
                }
            } else if (e.kind == Browser::Entry::Kind::Folder) {
                if (const auto child = Browser::childOf(s.listing.folder, e.name)) {
                    out.command = Command::OpenFolder;
                    out.path = *child;
                }
            } else if (e.tooLarge) {
                out.command = Command::RefuseFile;
                out.path = joinPath(s.listing.folder, e.name);
                out.message = tooLargeMessage(e.name, e.size);
            } else {
                out.command = Command::PickFile;
                out.path = joinPath(s.listing.folder, e.name);
            }
            break;
        }
        case Key::None: break;
    }
    keepVisible(s.first, s.cursor, count, pageRows);
    return out;
}

// Gen I / II files name the Pokemon by its DV word, Gen III-V by its PID (PokemonFileBrowser::baseName).
// `bytes` is one plain file as PokeBank NX writes it (PokemonFile::plainExtension): the Gen I / II
// single-entry list (count, species, 0xFF, party body: pret box_struct DVs at body 0x1B (pokered) /
// 0x15 (pokecrystal), big-endian), or the PKHeX plaintext PKn whose first u32 is the PID.
[[nodiscard]] inline uint32_t fileId(int generation, const uint8_t* bytes, std::size_t size) noexcept {
    constexpr std::size_t kBody = 3;
    if (generation == 1 || generation == 2) {
        const std::size_t at = kBody + (generation == 1 ? 0x1B : 0x15);
        return size >= at + 2 ? (static_cast<uint32_t>(bytes[at]) << 8) | bytes[at + 1] : 0;
    }
    if (size < 4) return 0;
    return static_cast<uint32_t>(bytes[0]) | (static_cast<uint32_t>(bytes[1]) << 8) |
           (static_cast<uint32_t>(bytes[2]) << 16) | (static_cast<uint32_t>(bytes[3]) << 24);
}

}  // namespace PokeBank::UIModel::PokemonFileDialog
