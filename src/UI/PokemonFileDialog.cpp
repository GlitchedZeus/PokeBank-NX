// The one Import from file dialog (owner Oct 28, Feature A UI). See UI/PokemonFileDialog.h.
#include "UI/PokemonFileDialog.h"

#include "UI/Common.h"
#include "UI/PKSEFramebuffer.h"
#include "UI/PokemonFileDialogModel.h"
#include "UI/PokemonFileIo.h"
#include "UI/ScreenChrome.h"
#include "UI/SharedBoxedRows.h"
#include "UI/SharedEditorChrome.h"
#include "UI/SharedMenuGeometry.h"
#include "UI/SharedPopupDismiss.h"
#include "UI/SharedScrollbar.h"
#include "UI/TrainerViewScreen.h"
#include "Utils/Settings.h"

#include <algorithm>
#include <string>
#include <utility>

namespace UI::PokemonFileDialog {
namespace {

namespace Model = PokeBank::UIModel::PokemonFileDialog;
namespace Browser = PokeBank::UIModel::PokemonFileBrowser;
namespace Io = PokeBank::UIModel::PokemonFileIo;
namespace Geo = PokeBank::UIModel::SharedMenuGeometry;

constexpr int kRowTouchBase = 88000;   // + visible row (browser rows and favourite rows alike)

struct DialogState {
    const TrainerViewScreen* owner = nullptr;
    uint64_t ownerSerial = 0;   // TrainerViewScreen::instanceSerial (never reused, unlike the address)
    bool active = false;
    int generation = 0;
    Model::State model;
    int favoriteFirst = 0;
    StageFn stage;
    std::string message;        // the last refusal / notice, shown under the folder line
    SharedScrollbar::State scrollbar;
};

DialogState& stateFor(const TrainerViewScreen& screen) {
    static DialogState state;
    if (state.owner != &screen || state.ownerSerial != screen.instanceSerial) {
        state = {};
        state.owner = &screen;
        state.ownerSerial = screen.instanceSerial;
    }
    return state;
}

int rowCount(const DialogState& s) {
    return s.model.mode == Model::Mode::Favorites
        ? static_cast<int>(Utils::g_folderPreferences.favorites.size())
        : static_cast<int>(s.model.listing.entries.size());
}

Geo::Layout layoutFor(const DialogState& s) {
    return Geo::layout(Geo::MenuType::FileBrowser, rowCount(s));
}

bool showFolder(DialogState& s, const std::string& folder, const std::string& focus = {}) {
    std::string error;
    if (!Io::folderExists(folder)) {
        s.message = "Folder not found: " + folder;
        return false;
    }
    const auto raw = Io::listFolder(folder, &error);
    if (!error.empty()) {
        s.message = error;
        return false;
    }
    Model::setListing(s.model, Browser::list(folder, raw, Utils::g_folderPreferences), focus);
    s.scrollbar.reset();
    s.message.clear();
    if (s.model.listing.truncated)
        s.message = "Only the first " + std::to_string(Browser::kMaxEntries) + " entries are listed";
    return true;
}

void finish(TrainerViewScreen& screen, DialogState& s) {
    s.active = false;
    s.stage = nullptr;
    s.scrollbar.reset();
    screen.controllerNavigation.reset();
}

void refuse(TrainerViewScreen& screen, DialogState& s, const std::string& why) {
    s.message = why;
    screen.postStatus(why, 420);
}

// Runs one model command. Returns false once the dialog has closed.
bool run(TrainerViewScreen& screen, DialogState& s, const Model::Outcome& out) {
    switch (out.command) {
        case Model::Command::None: return true;
        case Model::Command::Close: finish(screen, s); return false;
        case Model::Command::OpenFolder:
            if (!showFolder(s, out.path, out.focus)) screen.postStatus(s.message, 300);
            return true;
        case Model::Command::RefuseFile: refuse(screen, s, out.message); return true;
        case Model::Command::ToggleFavorite: {
            auto& prefs = Utils::g_folderPreferences;
            const bool was = prefs.isFavorite(out.path);
            if (!prefs.toggleFavorite(out.path)) {
                refuse(screen, s, "Favourite folders are full (" + std::to_string(Utils::FolderPreferences::kMaxFavorites) +
                                  "); remove one first");
                return true;
            }
            (void)Utils::saveSettings();
            s.model.listing.favorite = prefs.isFavorite(s.model.listing.folder);
            s.message = was ? "Removed from favourites: " + out.path : "Added to favourites: " + out.path;
            return true;
        }
        case Model::Command::PickFile: {
            std::string error;
            const auto decoded = Io::loadPokemonFile(out.path, error);
            if (!decoded) {
                refuse(screen, s, error.empty() ? "The file is not a Pokemon file PokeBank NX can read" : error);
                return true;
            }
            std::string status;
            error.clear();
            if (!s.stage || !s.stage(*decoded, status, error)) {
                refuse(screen, s, error.empty() ? "The Pokemon could not be imported; nothing was staged" : error);
                return true;
            }
            Utils::g_folderPreferences.remember(Utils::FolderPreferences::Operation::ImportPokemonFile,
                                                s.model.listing.folder);
            (void)Utils::saveSettings();
            finish(screen, s);
            screen.postStatus(status, 600);
            return false;
        }
    }
    return true;
}

} // namespace

void openImport(TrainerViewScreen& screen, int generation, StageFn stage) {
    auto& s = stateFor(screen);
    s = {};
    s.owner = &screen;
    s.ownerSerial = screen.instanceSerial;
    s.generation = generation;
    s.stage = std::move(stage);
    s.active = true;
    s.model.startFolder = Browser::startFolder(Utils::g_folderPreferences,
                                               [](const std::string& path) { return Io::folderExists(path); });
    if (!showFolder(s, s.model.startFolder)) {
        // The remembered / app folder vanished between the check and the listing: start at the root.
        s.model.startFolder = std::string(Utils::FolderPreferences::kRoot);
        (void)showFolder(s, s.model.startFolder);
    }
    screen.controllerNavigation.reset();
}

bool active(const TrainerViewScreen& screen) noexcept {
    const auto& s = stateFor(screen);
    return s.active;
}

void close(TrainerViewScreen& screen) noexcept {
    auto& s = stateFor(screen);
    s.active = false;
    s.stage = nullptr;
}

bool handleInput(TrainerViewScreen& screen, uint64_t down, uint64_t held, int stickX, int stickY,
                 const TouchInput& touch) {
    auto& s = stateFor(screen);
    if (!s.active) return false;
    const auto layout = layoutFor(s);
    const bool favorites = s.model.mode == Model::Mode::Favorites;
    int& first = favorites ? s.favoriteFirst : s.model.first;

    // The scrollbar owns a drag that starts on it; nothing under the finger activates.
    const auto bar = SharedScrollbar::handleTouch(s.scrollbar, SharedScrollbar::sample(touch),
        SharedScrollbar::rows(rowCount(s), layout.visibleRows, first));
    if (bar.changed) first = bar.scroll;
    if (bar.consumed) return true;
    // Non-destructive: a completed tap outside the dialog closes it like B at the start folder.
    if (SharedPopupDismiss::tappedOutside(touch)) {
        finish(screen, s);
        return true;
    }

    const int tapped = screen.touchedButtonId(touch);
    if (tapped >= kRowTouchBase && tapped < kRowTouchBase + layout.visibleRows) {
        // A completed tap on a row focuses it and acts like A on it.
        const int row = first + (tapped - kRowTouchBase);
        if (row >= 0 && row < rowCount(s)) {
            if (favorites) s.model.favoriteCursor = row;
            else s.model.cursor = row;
            down |= HidNpadButton_A;
        }
    }

    const uint64_t nav = screen.controllerNavigation.apply(
        down, held, stickX, stickY, HidNpadButton_Up, HidNpadButton_Down, HidNpadButton_Left, HidNpadButton_Right);
    const int page = std::max(1, layout.visibleRows);
    // '-' (Cancel, Round 11 H39) comes first so it wins over A pressed in the same frame.
    const std::pair<uint64_t, Model::Key> keys[] = {
        {down & HidNpadButton_Minus, Model::Key::Cancel},
        {nav & HidNpadButton_Up, Model::Key::Up},
        {nav & HidNpadButton_Down, Model::Key::Down},
        {down & (HidNpadButton_L | HidNpadButton_Left), Model::Key::PageUp},
        {down & (HidNpadButton_R | HidNpadButton_Right), Model::Key::PageDown},
        {down & HidNpadButton_A, Model::Key::A},
        {down & HidNpadButton_B, Model::Key::B},
        {down & HidNpadButton_Y, Model::Key::Y},
        {down & HidNpadButton_X, Model::Key::X},
    };
    for (const auto& [pressed, key] : keys) {
        if (!pressed) continue;
        const auto modeBefore = s.model.mode;
        const auto out = Model::press(s.model, key, Utils::g_folderPreferences, page);
        if (modeBefore != s.model.mode) {
            s.favoriteFirst = 0;
            s.scrollbar.reset();
        }
        if (!run(screen, s, out)) return true;
        // One command per frame: a folder change or a pick ends this frame's input.
        if (out.command != Model::Command::None) break;
    }
    if (s.model.mode == Model::Mode::Favorites) {
        const int count = rowCount(s);
        s.model.favoriteCursor = std::clamp(s.model.favoriteCursor, 0, std::max(0, count - 1));
        Model::keepVisible(s.favoriteFirst, s.model.favoriteCursor, count, layoutFor(s).visibleRows);
    } else {
        Model::keepVisible(s.model.first, s.model.cursor, rowCount(s), layoutFor(s).visibleRows);
    }
    return true;
}

void drawDialog(TrainerViewScreen& screen, PKSEFramebuffer& fb) {
    auto& s = stateFor(screen);
    if (!s.active) return;
    // Owns the frame: the screen's own rows, glyph badges and footer under it are not tappable.
    SharedEditorChrome::beginModal(screen.touchButtons);
    const auto l = layoutFor(s);
    const auto& frame = l.frame;
    drawModalSurface(fb, frame.x, frame.y, frame.w, frame.h, 16);
    // Non-destructive: a tap outside this frame closes it (UI/SharedPopupDismiss.h).
    SharedPopupDismiss::publish(frame.x, frame.y, frame.w, frame.h);

    const bool favorites = s.model.mode == Model::Mode::Favorites;
    const std::string title = favorites ? "Favourite folders"
        : "Import from file: Generation " + std::to_string(s.generation);
    fb.drawText(frame.x + 24, frame.y + 10, title, Colors::TextPrimary, TextStyle::Heading);
    if (favorites) {
        fb.drawText(frame.x + 24, frame.y + 48, "A opens a folder; Y removes it from the list",
                    Colors::TextSecondary, TextStyle::Caption);
    } else {
        std::string where = s.model.listing.folder;
        if (s.model.listing.favorite) where += "   (favourite)";
        fb.drawText(frame.x + 24, frame.y + 48, where, Colors::Accent, TextStyle::Caption);
    }
    if (!s.message.empty())
        fb.drawText(frame.x + 24, frame.y + 70, s.message, Colors::Warning, TextStyle::Caption);

    const int count = rowCount(s);
    const int first = favorites ? s.favoriteFirst : s.model.first;
    const int cursor = favorites ? s.model.favoriteCursor : s.model.cursor;
    if (count == 0) {
        fb.drawText(l.list.x + 12, l.list.y + 16,
                    favorites ? "No favourite folders yet. Press Y in a folder to add it."
                              : "No folders or Pokemon files (.pk1-.pk5, .ek3-.ek5) here.",
                    Colors::TextSecondary, TextStyle::Body);
    }
    for (int i = 0; i < l.visibleRows; ++i) {
        const int index = first + i;
        if (index < 0 || index >= count) break;
        const auto cell = Geo::cellRect(l, i);
        SharedBoxedRows::State state;
        state.focused = index == cursor;
        std::string label, detail;
        if (favorites) {
            label = Utils::g_folderPreferences.favorites[static_cast<std::size_t>(index)];
        } else {
            const auto& e = s.model.listing.entries[static_cast<std::size_t>(index)];
            switch (e.kind) {
                case Browser::Entry::Kind::Parent: label = ".."; detail = "Up one folder"; break;
                case Browser::Entry::Kind::Folder: label = e.name + "/"; detail = "Folder"; break;
                case Browser::Entry::Kind::File:
                    label = e.name;
                    detail = e.tooLarge ? "Too large (" + std::to_string(e.size) + " bytes)"
                                        : std::to_string(e.size) + " bytes";
                    break;
            }
        }
        SharedBoxedRows::drawRow(fb, screen.touchButtons, kRowTouchBase + i, cell, label, state, detail);
    }
    SharedScrollbar::drawAndPublish(fb, s.scrollbar, frame.right() - 14, l.list.y, SharedScrollbar::kTrackWidth,
                                    l.list.h, SharedScrollbar::rows(count, l.visibleRows, first));
    // B names what it does here (Model::bCloses); '-' always cancels the import (Round 11 H39).
    if (favorites)
        drawNavBar(fb, {{"A", "Open"}, {"Y", "Remove"}, {"B", "Back"}, {"-", "Cancel"}});
    else
        drawNavBar(fb, {{"A", "Open"}, {"Y", s.model.listing.favorite ? "Unfavourite" : "Favourite"},
                        {"X", "Favourites"}, {"L/R", "Page"}, {"B", Model::bCloses(s.model) ? "Close" : "Up Folder"},
                        {"-", "Cancel"}});
}

} // namespace UI::PokemonFileDialog
