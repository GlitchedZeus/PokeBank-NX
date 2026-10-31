#pragma once
#include "Integration/Gen1/Gen1StagedPokemonEditor.h"
#include "Integration/Gen2/Gen2PersonalData.h"
#include "Pokemon/PersonalInfoTable.h"
#include "UI/ClassicTypeBadges.h"
#include "UI/PKSEFramebuffer.h"
#include "UI/SpriteManager.h"
#include "UI/Common.h"
#include "UI/Gen13PickerPaging.h"
#include "UI/SharedBoxedRows.h"
#include "UI/SharedMenuGeometry.h"
#include "UI/SharedScrollbar.h"
#include "UI/SharedSearchInput.h"
#include "UI/TouchGesture.h"
#include <algorithm>
#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace UI::SharedSpeciesPicker {

struct ModalGeometry {
    int x = 0;
    int y = 0;
    int width = 1080;
    int height = 520;
};

inline constexpr int ModalWidth = 1080;
inline constexpr int ModalHeight = 520;
inline constexpr int NavBarHeight = 48;

// Match release-time scroll ownership to the very same clipped list used by drawContent.
// The Switch framebuffer and touch panel both use 1280x720 logical pixels; keep optional
// dimensions for render/layout tests without exposing libnx TouchInput to this header.
inline bool gestureStartedInList(int sx, int sy, int frameW = 1280, int frameH = 720) noexcept {
    const int contentHeight = std::max(ModalHeight, frameH - NavBarHeight);
    const int modalX = (frameW - ModalWidth) / 2;
    const int modalY = (contentHeight - ModalHeight) / 2;
    constexpr int listX = 20, listY = 84, listW = 560, rowStep = 43, visible = 9;
    return sx >= modalX + listX && sx < modalX + listX + listW &&
           sy >= modalY + listY && sy < modalY + listY + visible * rowStep;
}

// One shared shell for Gen I / II / III species choice.
// Theme accent belongs on the focused row and hints, not around the entire dialog.
inline ModalGeometry modalGeometry(const PKSEFramebuffer& fb) {
    const int contentHeight = std::max(ModalHeight, fb.getHeight() - NavBarHeight);
    return {
        (fb.getWidth() - ModalWidth) / 2,
        (contentHeight - ModalHeight) / 2,
        ModalWidth,
        ModalHeight
    };
}

inline ModalGeometry drawModalChrome(
    PKSEFramebuffer& fb, std::string_view title = "Choose Species") {
    const auto g = modalGeometry(fb);
    fb.drawFilledRect(0, 0, fb.getWidth(), fb.getHeight() - NavBarHeight, Color(0, 0, 0, 105));
    fb.drawSoftShadow(g.x, g.y, g.width, g.height, 18);
    fb.drawFilledRoundedRect(g.x, g.y, g.width, g.height, 18, Colors::Panel);
    fb.drawRoundedRect(g.x, g.y, g.width, g.height, 18, Colors::Divider, 1);
    fb.drawText(g.x + 24, g.y + 18, std::string(title), Colors::Text, TextStyle::Heading);
    return g;
}

// Convert canonical type ids to the classic badge-table numbering used by the
// Gen I-IV shared picker previews.
// NEW-11: the selected-species panel reads "Pikachu #25" (name first, then the dex number);
// the list rows keep their leading dex numbers.
inline std::string selectedTitle(uint16_t species, std::string_view name) {
    return std::string(name) + " #" + std::to_string(species);
}

inline uint8_t canonicalToClassicType(uint8_t normalized) noexcept {
    if (normalized <= 5) return normalized;
    if (normalized == 6) return 7;
    if (normalized == 7) return 8;
    if (normalized == 8) return 9;
    if (normalized >= 9 && normalized <= 16)
        return static_cast<uint8_t>(20 + (normalized - 9));
    return 0xFF;
}

namespace {

// Keep the native pre-Gen-IV tables local. Later generations can pass an exact
// type pair into drawContent without making every picker user link their data table.
inline std::array<uint8_t, 2> classicPickerTypes(uint16_t species, int speciesCount) {
    if (speciesCount == 151)
        return PokeVault::Integration::Gen1::StagedPokemonEditor::personalTypes(species);
    if (speciesCount == 251) {
        if (const auto* personal = PokeVault::Integration::Gen2::personalRecord(species))
            return {personal->rawType1, personal->rawType2};
    }
    if (speciesCount == 386) {
        const auto& personal = Pokemon::getPersonalInfoG3(species);
        return {canonicalToClassicType(personal.type1),
                canonicalToClassicType(personal.type2)};
    }
    return {0, 0};
}

inline bool touchStartedInside(const TouchGestureSnapshot& touch,
                               int x, int y, int w, int h) noexcept {
    return touch.down && touch.startX >= x && touch.startX < x + w &&
           touch.startY >= y && touch.startY < y + h;
}

} // namespace

// ---- Live incremental species search (R2-SEARCH, Phase 3) ------------------------------------
//
// One Chooser per owning picker: the live query and its on-screen key grid (SharedSearchInput),
// the filtered dex list, the draggable scrollbar (SharedScrollbar) and the rows the last draw
// published so the owner can attach its own touch ids. The owner keeps its species variable as
// the source of truth; the chooser only offers the filtered neighbourhood of that species
// (neighbour / paged / snap) and tells the owner which input it consumed. Filtering is a
// prefix match on the normalised name (SharedSearchInput::searchKey), rebuilt on every query
// change over at most 649 names, and reads nothing from any save.
//
// Geometry: the accepted 1080x520 modal and its 560px list are kept (gestureStartedInList above
// pins them); the row cell follows Lane S's SharedMenuGeometry SpeciesPicker rule (leading
// 34 px icon, 8 px sprite gap, 12 px cell padding, Body label) so every generation's chooser
// looks the same. NEW-05: while the key grid is open the list keeps all kSearchVisibleRows
// (== kVisibleRows) rows and the grid sits in the preview column, under the artwork, which
// shrinks and moves up (kSearchArt*); the keyboard never covers the list.
inline constexpr int kVisibleRows = 9;
inline constexpr int kSearchVisibleRows = kVisibleRows;   // NEW-05: the list stays whole
inline constexpr int kRowStep = 43;
inline constexpr int kRowHeight = 39;
inline constexpr int kListX = 20;
inline constexpr int kListY = 84;
inline constexpr int kListW = 560;
inline constexpr int kKeyGridH = 176;
inline constexpr int kPreviewX = 610;                       // preview column, relative to the modal
inline constexpr int kPreviewW = ModalWidth - kPreviewX - 24;   // 446: up to the modal's right padding
inline constexpr int kKeyGridY = ModalHeight - 24 - kKeyGridH;  // 320: grid bottom-aligned in the column
inline constexpr int kSearchArtY = 140, kSearchArtW = 150, kSearchArtH = 120;   // artwork while searching

namespace MenuGeometry = PokeBank::UIModel::SharedMenuGeometry;
inline constexpr MenuGeometry::Rule kRowRule = MenuGeometry::rule(MenuGeometry::MenuType::SpeciesPicker);
static_assert(kRowRule.kind == MenuGeometry::Kind::List && kRowRule.spriteSize > 0,
              "the species chooser row rule carries a leading sprite");

struct VisibleRow {
    uint16_t species = 0;
    int x = 0, y = 0, w = 0, h = 0;
};

struct Chooser {
    SharedSearchInput::State search;
    SharedScrollbar::State scrollbar;
    std::vector<uint16_t> matches;      // filtered dex numbers, ascending
    unsigned matchedRevision = 0;
    int matchedCount = -1;
    bool matched = false;
    std::array<VisibleRow, kVisibleRows> rows{};   // published by drawContent (static positions)
    int rowCount = 0;

    void reset() { *this = Chooser{}; }
    [[nodiscard]] bool searching() const noexcept { return search.open; }
    [[nodiscard]] bool filtered() const noexcept { return !search.query.empty(); }
    [[nodiscard]] int count() const noexcept { return static_cast<int>(matches.size()); }
    [[nodiscard]] int visibleRows() const noexcept { return search.open ? kSearchVisibleRows : kVisibleRows; }
};

// Rebuild the filtered list when the query or the game's species bound changed. `available`
// says whether this game offers a dex number (Gen I: all of 1..151; Gen IV/V: the native list);
// `name` returns its display name. The result never contains a species the game cannot hold.
template <class Available, class Name>
inline void refilter(Chooser& c, int speciesCount, Available available, Name name) {
    if (c.matched && c.matchedRevision == c.search.revision && c.matchedCount == speciesCount) return;
    c.matches.clear();
    const std::string key = SharedSearchInput::searchKey(c.search.query);
    for (int s = 1; s <= speciesCount; ++s) {
        const auto species = static_cast<uint16_t>(s);
        if (!available(species)) continue;
        if (!key.empty()) {
            const std::string n = SharedSearchInput::searchKey(name(species));
            if (n.size() < key.size() || n.compare(0, key.size(), key) != 0) continue;
        }
        c.matches.push_back(species);
    }
    c.matched = true;
    c.matchedRevision = c.search.revision;
    c.matchedCount = speciesCount;
}

inline int indexOf(const Chooser& c, uint16_t species) noexcept {
    const auto it = std::lower_bound(c.matches.begin(), c.matches.end(), species);
    return it != c.matches.end() && *it == species ? static_cast<int>(it - c.matches.begin()) : -1;
}
// The species to highlight: `species` itself when it matches, else the first match at or after
// it, else the last match; 0 when nothing matches.
inline uint16_t snap(const Chooser& c, uint16_t species) noexcept {
    if (c.matches.empty()) return 0;
    const auto it = std::lower_bound(c.matches.begin(), c.matches.end(), species);
    return it != c.matches.end() ? *it : c.matches.back();
}
inline uint16_t neighbour(const Chooser& c, uint16_t species, int steps) noexcept {
    if (c.matches.empty()) return species;
    int index = indexOf(c, species);
    if (index < 0) index = indexOf(c, snap(c, species));
    index = std::clamp(index + steps, 0, c.count() - 1);
    return c.matches[static_cast<std::size_t>(index)];
}
// Shared Gen I-V list paging (Gen13PickerPaging.h) applied to the filtered list.
inline uint16_t paged(const Chooser& c, uint16_t species,
                      const PokeBank::UIModel::SharedPickerPaging::Keys& keys) noexcept {
    if (c.matches.empty() || !keys.any()) return species;
    int index = indexOf(c, species);
    if (index < 0) index = indexOf(c, snap(c, species));
    index = PokeBank::UIModel::SharedPickerPaging::pagedRow(index, c.count(), keys);
    return c.matches[static_cast<std::size_t>(index)];
}
// First visible position (0-based) that centres `species` in the current window.
inline int firstVisible(const Chooser& c, uint16_t species) noexcept {
    const int visible = c.visibleRows();
    int index = indexOf(c, species);
    if (index < 0) index = std::max(0, indexOf(c, snap(c, species)));
    return std::clamp(index - visible / 2, 0, std::max(0, c.count() - visible));
}

inline SharedSearchInput::Rect keyGridRect(int modalX, int modalY) noexcept {
    return {modalX + kPreviewX, modalY + kKeyGridY, kPreviewW, kKeyGridH};
}

struct Input {
    PokeBank::UIModel::SharedPickerPaging::Keys nav;   // D-pad / stick (+ L/R/ZL/ZR) as resolved by the owner
    bool a = false, b = false;
    bool search = false;                                // X or Minus: open / close the search grid
    SharedSearchInput::Touch touch;
};
struct Handled {
    bool keys = false;         // D-pad / A / B went to the key grid: the owner must not browse, select or cancel
    bool touch = false;        // the frame's touch belongs to the key grid or the scrollbar
    bool queryChanged = false;
    bool blockSelect = false;  // nothing matches: A selects nothing
};

inline constexpr int ModalX = (1280 - ModalWidth) / 2;                   // 100 on the 1280x720 panel
inline constexpr int ModalY = (720 - NavBarHeight - ModalHeight) / 2;     // 76

// Snap the owner's species into the (re)filtered list; flags the no-match state.
template <class Available, class Name>
inline void settle(Chooser& c, Handled& h, uint16_t& species, int speciesCount,
                   Available available, Name name) {
    refilter(c, speciesCount, available, name);
    if (c.count() > 0) species = snap(c, species);
    else h.blockSelect = true;
}

// Touch phase, BEFORE the owner's own row tap / drag handling: the key grid (while open) and the
// scrollbar own their touches; `species` follows the thumb and is snapped into the filter.
template <class Available, class Name>
inline Handled handleTouch(Chooser& c, const SharedSearchInput::Touch& touch, uint16_t& species,
                           int speciesCount, int modalX, int modalY, Available available, Name name) {
    Handled h;
    using SharedSearchInput::Event;
    const Event te = SharedSearchInput::handleTouch(c.search, touch, keyGridRect(modalX, modalY));
    if (te != Event::None) h.touch = true;
    if (te == Event::QueryChanged) h.queryChanged = true;
    if (te == Event::Closed) h.keys = true;   // the Done key's release closed the grid: nothing else this frame
    refilter(c, speciesCount, available, name);
    if (!h.touch && c.count() > 0) {
        const int visible = c.visibleRows();
        const SharedScrollbar::Touch st{touch.down, touch.justDown, touch.justReleased,
                                        touch.x, touch.y, touch.startX, touch.startY};
        const auto sb = SharedScrollbar::handleTouch(c.scrollbar, st,
            SharedScrollbar::rows(c.count(), visible, firstVisible(c, species)));
        if (sb.changed)
            species = c.matches[static_cast<std::size_t>(std::clamp(sb.scroll + visible / 2, 0, c.count() - 1))];
        if (sb.consumed) h.touch = true;
    }
    settle(c, h, species, speciesCount, available, name);
    return h;
}

// Keys phase, BEFORE the owner's own browsing / A / B: X or Minus (`search`) toggles the grid;
// while it is open the D-pad / stick, A and B drive the keys and the owner must not browse,
// select or cancel this frame. Y is never consumed (the owner keeps its shiny toggle).
template <class Available, class Name>
inline Handled handleKeys(Chooser& c, const PokeBank::UIModel::SharedPickerPaging::Keys& nav,
                          bool a, bool b, bool search, uint16_t& species, int speciesCount,
                          Available available, Name name) {
    Handled h;
    using SharedSearchInput::Event;
    if (search) { c.search.toggleGrid(); h.keys = true; }
    if (c.search.open) {
        const SharedSearchInput::Keys k{nav.up, nav.down, nav.left, nav.right, a, b, nav.l, nav.r};
        const Event ke = SharedSearchInput::handleKeys(c.search, k);
        if (ke != Event::None) h.keys = true;
        if (ke == Event::QueryChanged) h.queryChanged = true;
    }
    settle(c, h, species, speciesCount, available, name);
    if (h.blockSelect && a) h.keys = true;   // nothing to select
    return h;
}

// Both phases in one call for owners whose touch and keys arrive together.
template <class Available, class Name>
inline Handled handle(Chooser& c, const Input& in, uint16_t& species, int speciesCount,
                      int modalX, int modalY, Available available, Name name) {
    Handled t = handleTouch(c, in.touch, species, speciesCount, modalX, modalY, available, name);
    const Handled k = handleKeys(c, in.nav, in.a, in.b, in.search, species, speciesCount, available, name);
    t.keys = t.keys || k.keys;
    t.queryChanged = t.queryChanged || k.queryChanged;
    t.blockSelect = k.blockSelect;
    return t;
}

// Extracted from the accepted Gen I visual picker and shared with Generation II.
// Browsing is preview-only; the generation supplies only its dex bound and text.
// With a Chooser the list is the filtered one, the search field / key grid and the draggable
// scrollbar are drawn, and chooser->rows receives the drawn rows for the owner's touch ids.
template <class RowText, class TitleText>
void drawContent(PKSEFramebuffer& fb, int x, int y, int selectedSpecies,
                 int speciesCount, bool previewShiny, RowText rowText, TitleText titleText,
                 const TouchGestureSnapshot* gesture = nullptr,
                 bool shinyToggleAvailable = true,
                 std::array<uint8_t, 2> previewTypes = {0xFF, 0xFF},
                 Chooser* chooser = nullptr, int nativeUnownForm = -1) {
        const bool searching = chooser && chooser->searching();
        if (!chooser) {
            fb.drawText(x + 24, y + 50,
                        shinyToggleAvailable ? "Drag to browse • tap to choose • Y Normal/Shiny"
                                             : "Drag to browse • tap to choose",
                        Colors::TextDim, TextStyle::Caption);
        } else if (searching) {
            SharedSearchInput::drawField(fb, x + 24, y + 50, chooser->search, "Search:",
                                         "A / tap types • B or Done closes search");
        } else if (chooser->filtered()) {
            SharedSearchInput::drawField(fb, x + 24, y + 50, chooser->search, "Filter:",
                                         shinyToggleAvailable ? "X / − Search • Y Normal/Shiny" : "X / − Search");
        } else {
            fb.drawText(x + 24, y + 50,
                        shinyToggleAvailable ? "Drag to browse • tap to choose • X / − Search • Y Normal/Shiny"
                                             : "Drag to browse • tap to choose • X / − Search",
                        Colors::TextDim, TextStyle::Caption);
        }
        const int visible = chooser ? chooser->visibleRows() : kVisibleRows;
        constexpr int rowStep = kRowStep;
        constexpr int rowHeight = kRowHeight;
        const int listX = x + kListX, listW = kListW;
        const int listY = y + kListY;
        const int listH = visible * rowStep;

        // Positions are 1-based rows of the list being shown: the whole dex without a chooser,
        // the filtered matches with one. speciesAt maps a position back to its dex number.
        const int count = chooser ? chooser->count() : speciesCount;
        const auto speciesAt = [&](int position) -> uint16_t {
            if (!chooser) return static_cast<uint16_t>(position);
            return chooser->matches[static_cast<std::size_t>(std::clamp(position, 1, std::max(1, count)) - 1)];
        };
        int selectedPosition = chooser ? indexOf(*chooser, static_cast<uint16_t>(selectedSpecies)) + 1
                                       : std::clamp(selectedSpecies, 1, speciesCount);
        if (selectedPosition < 1) selectedPosition = 1;
        if (chooser) chooser->rowCount = 0;

        if (count <= 0) {
            fb.drawText(listX + 16, listY + 12,
                        "No species match \"" + (chooser ? chooser->search.query : std::string()) + "\"",
                        Colors::TextDim, TextStyle::Body);
            fb.drawText(listX + 16, listY + 44, "Backspace or Clear to widen the search",
                        Colors::TextDim, TextStyle::Caption);
            if (chooser) {
                chooser->scrollbar.track = {};
                if (searching) SharedSearchInput::drawGrid(fb, chooser->search, keyGridRect(x, y));
            }
            return;
        }

        // Direct-manipulation preview: while a finger is physically dragging this list, move the
        // rendered rows by the exact pixel delta. Whole rows are folded into a temporary visual
        // species and only the sub-row remainder is translated. The semantic selection is still
        // committed by the owning input handler on release, so drag can never become an accidental A.
        int visualSpecies = std::clamp(selectedPosition, 1, count);
        int liveOffset = 0;
        const bool barHeld = chooser && chooser->scrollbar.capturing();
        if (gesture && !barHeld && touchStartedInside(*gesture, listX, listY, listW, listH) &&
            permitsVerticalPreview(*gesture)) {
            const int requestedRows = -gesture->deltaY / rowStep;
            visualSpecies = std::clamp(selectedPosition + requestedRows, 1, count);
            const int appliedRows = visualSpecies - selectedPosition;
            liveOffset = gesture->deltaY + appliedRows * rowStep;
            // Resist rather than expose empty space at either edge.
            if ((visualSpecies == 1 && liveOffset > 0) ||
                (visualSpecies == count && liveOffset < 0))
                liveOffset /= 3;
        }

        const int start = std::clamp(visualSpecies - visible / 2, 1,
                                     std::max(1, count - visible + 1));
        fb.setClipRect(listX, listY, listW, listH - 2);
        const int firstDraw = std::max(1, start - 1);
        const int lastDraw = std::min(count, start + visible);
        const int labelX = chooser ? listX + kRowRule.cellPadX + kRowRule.spriteSize + kRowRule.spriteGap
                                   : listX + 16;
        for (int position = firstDraw; position <= lastDraw; ++position) {
            const int rowY = listY + (position - start) * rowStep + liveOffset;
            const bool selected = position == visualSpecies;
            const uint16_t species = speciesAt(position);
            // R2-BUG-012: every species row is its own box; the focused one carries the yellow
            // outline (UI/SharedBoxedRows.h), same as every other Gen I-V chooser.
            SharedBoxedRows::drawBox(fb, SharedBoxedRows::rect(listX, rowY, listW, rowHeight), {selected});
            if (chooser) {
                // Leading icon per the shared SpeciesPicker row rule (SharedMenuGeometry.h).
                const auto form = static_cast<uint8_t>(species == 201 && nativeUnownForm >= 0 &&
                    nativeUnownForm < 26 ? nativeUnownForm : 0);
                if (auto* icon = SpriteManager::getIconSprite(species, form, previewShiny); icon && icon->data)
                    fb.drawSpriteStaticContained(listX + kRowRule.cellPadX,
                        rowY + (rowHeight - kRowRule.spriteSize) / 2,
                        kRowRule.spriteSize, kRowRule.spriteSize,
                        icon->width, icon->height, icon->data, icon->channels);
            }
            fb.drawText(labelX, rowY + 9, rowText(species), selected ? Colors::Text : Colors::TextDim);
        }
        fb.clearClip();
        if (chooser) {
            // Static row rectangles (no live offset) for the owner's touch ids, the same contract
            // every generation used before the chooser existed.
            for (int position = start; position < start + visible && position <= count; ++position) {
                auto& row = chooser->rows[static_cast<std::size_t>(chooser->rowCount++)];
                row = {speciesAt(position), listX, listY + (position - start) * rowStep, listW, rowHeight};
            }
            SharedScrollbar::drawAndPublish(fb, chooser->scrollbar, listX + listW + 8, listY,
                SharedScrollbar::kTrackWidth, listH - 4,
                SharedScrollbar::rows(count, visible, start - 1));
            if (searching) SharedSearchInput::drawGrid(fb, chooser->search, keyGridRect(x, y));
        }

        const uint16_t preview = speciesAt(visualSpecies);
        // Gen II's native Unown letter comes from its DVs; other species are unchanged.
        const auto form = static_cast<uint8_t>(preview == 201 && nativeUnownForm >= 0 &&
            nativeUnownForm < 26 ? nativeUnownForm : 0);
        const int previewX = x + kPreviewX;
        constexpr int previewW = 353;
        fb.drawText(previewX, y + 86, titleText(preview), Colors::Text, TextStyle::Heading);
        fb.drawText(previewX, y + 122, "NORMAL", !previewShiny ? Colors::Accent : Colors::TextDim, TextStyle::Caption);
        fb.drawText(previewX + 190, y + 122, "SHINY", previewShiny ? Colors::ShinyStar : Colors::TextDim, TextStyle::Caption);
        if (searching) {
            // NEW-05: compact artwork above the key grid (drawn earlier in this column).
            if (auto* normal = SpriteManager::getSprite(preview, form, false); normal && normal->data)
                fb.drawSpriteStaticContained(previewX, y + kSearchArtY, kSearchArtW, kSearchArtH,
                                             normal->width, normal->height, normal->data, normal->channels);
            if (auto* shiny = SpriteManager::getSprite(preview, form, true); shiny && shiny->data)
                fb.drawSpriteStaticContained(previewX + 190, y + kSearchArtY, kSearchArtW, kSearchArtH,
                                             shiny->width, shiny->height, shiny->data, shiny->channels);
            const auto types = (previewTypes[0] == 0xFF && previewTypes[1] == 0xFF)
                ? classicPickerTypes(preview, speciesCount)
                : previewTypes;
            ClassicTypeBadges::drawPairCentered(fb, previewX, previewW, y + kSearchArtY + kSearchArtH + 10,
                                                types[0], types[1], 20);
            return;
        }
        if (auto* normal = SpriteManager::getSprite(preview, form, false); normal && normal->data)
            fb.drawSpriteStaticContained(previewX, y + 152, 165, 190, normal->width, normal->height, normal->data, normal->channels);
        else fb.drawText(previewX + 20, y + 235, "No sprite", Colors::TextDim, TextStyle::Caption);
        if (auto* shiny = SpriteManager::getSprite(preview, form, true); shiny && shiny->data)
            fb.drawSpriteStaticContained(previewX + 188, y + 152, 165, 190, shiny->width, shiny->height, shiny->data, shiny->channels);
        else fb.drawText(previewX + 208, y + 235, "No shiny sprite", Colors::TextDim, TextStyle::Caption);

        // Generation-correct type badges update with the highlighted species.
        const auto types = (previewTypes[0] == 0xFF && previewTypes[1] == 0xFF)
            ? classicPickerTypes(preview, speciesCount)
            : previewTypes;
        ClassicTypeBadges::drawPairCentered(fb, previewX, previewW, y + 344, types[0], types[1]);

        fb.drawText(previewX, y + 374,
            std::string("Shiny: ") + (previewShiny ? "Yes" : "No"),
            previewShiny ? Colors::ShinyStar : Colors::Accent, TextStyle::Body);
        // The picker's own footer (SharedEditorPresentation::pickerFooter) carries A Select /
        // B Cancel; the preview pane shows no developer or staging copy.
}
} // namespace UI::SharedSpeciesPicker
