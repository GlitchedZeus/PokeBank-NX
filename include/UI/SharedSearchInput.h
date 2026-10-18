#pragma once
// App-owned incremental text entry for in-modal search (R2-SEARCH, Phase 3).
//
// The Nintendo software keyboard (Utils::promptSystemText / the swkbd fallback in KeyboardDialog)
// blocks until it is answered and cannot report each keystroke, so a chooser that wants to filter
// while the user types owns its own input surface instead:
//   * an on-screen key grid (A-Z, 0-9, Backspace, Clear, Done) that lives INSIDE the modal,
//   * driven by D-pad / stick (cursor over the keys, A presses, B closes) and by touch
//     (release-confirmed key taps, same rule as every other PokeBank NX control),
//   * a query that changes on every key, never only on submit; `revision` lets the owner
//     re-filter exactly when the text changed.
// The same normalisation is used on both the query and the names it is matched against, so
// "MRM" finds "Mr. Mime", "FARFETCHD" finds "Farfetch'd", "NIDORANF" finds Nidoran♀ and
// "PORYGON2" finds Porygon2 (letters upper-cased, punctuation and spaces dropped, ♀ -> F,
// ♂ -> M, accented Latin vowels folded to their base letter). Matching is a prefix match, as
// the owner asked for: C -> every name starting with C, then CO, and Backspace reverts.
//
// Platform-neutral: no libnx, no TouchInput; owners convert their TouchInput with sample().

#include "UI/Common.h"
#include "UI/PKSEFramebuffer.h"

#include <algorithm>
#include <cstdint>
#include <string>
#include <string_view>

namespace UI::SharedSearchInput {

inline constexpr int kMaxQuery = 12;

// ---- Normalisation ------------------------------------------------------------------------------

// Folds one UTF-8 text into the comparison key: [A-Z0-9]* only.
inline std::string searchKey(std::string_view utf8) {
    std::string out;
    out.reserve(utf8.size());
    for (std::size_t i = 0; i < utf8.size();) {
        const unsigned char c = static_cast<unsigned char>(utf8[i]);
        if (c < 0x80) {
            if (c >= 'a' && c <= 'z') out.push_back(static_cast<char>(c - 'a' + 'A'));
            else if ((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')) out.push_back(static_cast<char>(c));
            ++i;
            continue;
        }
        // Two-byte Latin-1 supplement: é/É -> E, etc. (Flabébé-style names, not needed in Gen I-V
        // but folded consistently); three-byte ♀ (E2 99 80) -> F, ♂ (E2 99 82) -> M.
        if (c >= 0xC0 && c < 0xE0 && i + 1 < utf8.size()) {
            const unsigned char d = static_cast<unsigned char>(utf8[i + 1]);
            if (c == 0xC3) {
                const unsigned v = d & 0x1F;   // same row for upper (0x80..) and lower (0xA0..)
                char base = 0;
                if (v <= 0x05) base = 'A';          // À Á Â Ã Ä Å
                else if (v == 0x07) base = 'C';     // Ç
                else if (v >= 0x08 && v <= 0x0B) base = 'E';
                else if (v >= 0x0C && v <= 0x0F) base = 'I';
                else if (v == 0x11) base = 'N';
                else if ((v >= 0x12 && v <= 0x16) || v == 0x18) base = 'O';
                else if (v >= 0x19 && v <= 0x1C) base = 'U';
                else if (v == 0x1D) base = 'Y';
                if (base) out.push_back(base);
            }
            i += 2;
            continue;
        }
        if (c >= 0xE0 && c < 0xF0 && i + 2 < utf8.size()) {
            const unsigned char d = static_cast<unsigned char>(utf8[i + 1]);
            const unsigned char e = static_cast<unsigned char>(utf8[i + 2]);
            if (c == 0xE2 && d == 0x99 && e == 0x80) out.push_back('F');       // ♀
            else if (c == 0xE2 && d == 0x99 && e == 0x82) out.push_back('M');  // ♂
            i += 3;
            continue;
        }
        i += c >= 0xF0 ? 4 : 1;
    }
    return out;
}

// Prefix match on the folded keys. An empty query matches everything.
inline bool matchesPrefix(std::string_view name, std::string_view query) {
    const std::string q = searchKey(query);
    if (q.empty()) return true;
    const std::string n = searchKey(name);
    return n.size() >= q.size() && n.compare(0, q.size(), q) == 0;
}

// ---- Key grid -----------------------------------------------------------------------------------
//
//   row 0: A B C D E F G H I J
//   row 1: K L M N O P Q R S T
//   row 2: U V W X Y Z 0 1 2 3
//   row 3: 4 5 6 7 8 9 Back Clear [ Done ]
inline constexpr int kColumns = 10;
inline constexpr int kRows = 4;
inline constexpr int kKeyCount = 39;
enum Special : int { Backspace = 36, Clear = 37, Done = 38 };

constexpr char keyChar(int key) noexcept {
    if (key >= 0 && key < 26) return static_cast<char>('A' + key);
    if (key >= 26 && key < 36) return static_cast<char>('0' + (key - 26));
    return 0;
}
constexpr std::string_view keyLabel(int key) noexcept {
    switch (key) {
        case Backspace: return "Back";
        case Clear: return "Clear";
        case Done: return "Done";
        default: return {};
    }
}
constexpr int keyRow(int key) noexcept { return key < 30 ? key / kColumns : 3; }
constexpr int keyCol(int key) noexcept {
    if (key < 30) return key % kColumns;
    if (key < 36) return key - 30;
    return key == Backspace ? 6 : key == Clear ? 7 : 8;
}
constexpr int keySpan(int key) noexcept { return key == Done ? 2 : 1; }
// The key occupying (row, col), -1 for an empty cell.
constexpr int keyAt(int row, int col) noexcept {
    if (row < 0 || row >= kRows || col < 0 || col >= kColumns) return -1;
    if (row < 3) return row * kColumns + col;
    if (col < 6) return 30 + col;
    if (col == 6) return Backspace;
    if (col == 7) return Clear;
    return Done;
}

struct Rect {
    int x = 0, y = 0, w = 0, h = 0;
    constexpr bool contains(int px, int py) const noexcept {
        return px >= x && px < x + w && py >= y && py < y + h;
    }
};

constexpr Rect keyRect(const Rect& grid, int key) noexcept {
    if (key < 0 || key >= kKeyCount) return {};
    const int cellW = grid.w / kColumns, cellH = grid.h / kRows;
    return {grid.x + keyCol(key) * cellW, grid.y + keyRow(key) * cellH, cellW * keySpan(key), cellH};
}
constexpr int hitKey(const Rect& grid, int px, int py) noexcept {
    for (int key = 0; key < kKeyCount; ++key)
        if (keyRect(grid, key).contains(px, py)) return key;
    return -1;
}
// D-pad movement over the grid: rows and columns wrap; a column with no key snaps to Done.
constexpr int moveCursor(int key, int dRow, int dCol) noexcept {
    key = std::clamp(key, 0, kKeyCount - 1);
    int row = keyRow(key), col = keyCol(key);
    if (dCol != 0) {
        const int span = row == 3 ? 9 : kColumns;   // row 3 has 9 logical slots (Done spans 2)
        col = ((col + dCol) % span + span) % span;
        if (row == 3 && col > 8) col = 8;
    }
    if (dRow != 0) row = ((row + dRow) % kRows + kRows) % kRows;
    const int next = keyAt(row, col);
    return next >= 0 ? next : Done;
}

// ---- State machine ------------------------------------------------------------------------------

struct State {
    bool open = false;          // the key grid is shown and owns D-pad / A / B
    std::string query;          // live text, already in key form ([A-Z0-9])
    int cursor = 0;             // focused key
    int pressedKey = -1;        // key under a held finger
    bool gridGesture = false;   // a touch that began on the grid (consumed until release)
    unsigned revision = 0;      // bumps on every query change; owners re-filter when it moves

    void openGrid() noexcept { open = true; }
    void closeGrid() noexcept { open = false; pressedKey = -1; gridGesture = false; }
    void toggleGrid() noexcept { if (open) closeGrid(); else openGrid(); }
};

enum class Event { None, Consumed, QueryChanged, Closed };

inline Event apply(State& s, int key) noexcept {
    if (const char c = keyChar(key)) {
        if (static_cast<int>(s.query.size()) >= kMaxQuery) return Event::Consumed;
        s.query.push_back(c);
        ++s.revision;
        return Event::QueryChanged;
    }
    switch (key) {
        case Backspace:
            if (s.query.empty()) return Event::Consumed;
            s.query.pop_back();
            ++s.revision;
            return Event::QueryChanged;
        case Clear:
            if (s.query.empty()) return Event::Consumed;
            s.query.clear();
            ++s.revision;
            return Event::QueryChanged;
        case Done:
            s.closeGrid();
            return Event::Closed;
        default:
            return Event::Consumed;
    }
}

struct Keys {
    bool up = false, down = false, left = false, right = false;
    bool a = false, b = false;
    constexpr bool any() const noexcept { return up || down || left || right || a || b; }
};

// Controller input while the grid is open. B closes the grid and KEEPS the query (the list stays
// filtered); the owner's own B then cancels the chooser as before.
inline Event handleKeys(State& s, const Keys& k) noexcept {
    if (!s.open || !k.any()) return Event::None;
    if (k.b) { s.closeGrid(); return Event::Closed; }
    if (k.up) s.cursor = moveCursor(s.cursor, -1, 0);
    if (k.down) s.cursor = moveCursor(s.cursor, 1, 0);
    if (k.left) s.cursor = moveCursor(s.cursor, 0, -1);
    if (k.right) s.cursor = moveCursor(s.cursor, 0, 1);
    if (k.a) return apply(s, s.cursor);
    return Event::Consumed;
}

struct Touch {
    bool down = false, justDown = false, justReleased = false;
    int x = 0, y = 0, startX = 0, startY = 0;
};
template <class TouchInputLike>
inline Touch sample(const TouchInputLike& t) noexcept {
    return {t.isDown(), t.justTouchedDown(), t.justReleased(),
            t.x(), t.y(), t.x() - t.deltaX(), t.y() - t.deltaY()};
}

// Touch while the grid is open: a key activates on release over the same key it was pressed on;
// any touch that started on the grid belongs to the grid until release.
inline Event handleTouch(State& s, const Touch& t, const Rect& grid) noexcept {
    if (!s.open) { s.pressedKey = -1; s.gridGesture = false; return Event::None; }
    if (t.justDown) {
        s.pressedKey = hitKey(grid, t.x, t.y);
        s.gridGesture = s.pressedKey >= 0 || grid.contains(t.x, t.y);
        if (s.pressedKey >= 0) s.cursor = s.pressedKey;
        return s.gridGesture ? Event::Consumed : Event::None;
    }
    if (!s.gridGesture) return Event::None;
    if (t.down) {
        if (s.pressedKey >= 0 && hitKey(grid, t.x, t.y) != s.pressedKey) s.pressedKey = -1;
        return Event::Consumed;
    }
    // Released (or the finger vanished): resolve the tap once.
    s.gridGesture = false;
    const int key = s.pressedKey;
    s.pressedKey = -1;
    if (t.justReleased && key >= 0 && hitKey(grid, t.x, t.y) == key) return apply(s, key);
    return Event::Consumed;
}

// ---- Drawing ------------------------------------------------------------------------------------

inline void drawGrid(PKSEFramebuffer& fb, const State& s, const Rect& grid) {
    for (int key = 0; key < kKeyCount; ++key) {
        const Rect r = keyRect(grid, key);
        const int x = r.x + 3, y = r.y + 3, w = r.w - 6, h = r.h - 6;
        const bool pressed = key == s.pressedKey;
        const bool focused = key == s.cursor;
        fb.drawFilledRoundedRect(x, y, w, h, 8, pressed ? Colors::Accent : Colors::PanelAlt);
        if (focused) fb.drawRoundedRect(x, y, w, h, 8, Colors::FocusBorder, 2);
        std::string label;
        if (const char c = keyChar(key)) label.assign(1, c);
        else label = std::string(keyLabel(key));
        int tw = 0, th = 0;
        fb.measureText(label, tw, th, TextStyle::Body);
        fb.drawText(x + (w - tw) / 2, y + (h - th) / 2, label,
                    pressed ? Colors::SelectedText : focused ? Colors::Text : Colors::TextSecondary,
                    TextStyle::Body);
    }
}

// The live query line: "<label> QUERY|" while open; "<label> QUERY" when closed with a filter.
inline void drawField(PKSEFramebuffer& fb, int x, int y, const State& s, std::string_view label,
                      std::string_view hint) {
    const std::string head = std::string(label) + " ";
    fb.drawText(x, y, head, Colors::TextDim, TextStyle::Caption);
    int hw = 0, hh = 0;
    fb.measureText(head, hw, hh, TextStyle::Caption);
    const std::string text = s.query + (s.open ? "_" : "");
    fb.drawText(x + hw, y, text.empty() ? std::string("(all)") : text,
                s.open ? Colors::Accent : Colors::Text, TextStyle::Caption);
    int tw = 0, th = 0;
    fb.measureText(text.empty() ? std::string("(all)") : text, tw, th, TextStyle::Caption);
    if (!hint.empty())
        fb.drawText(x + hw + tw + 18, y, std::string(hint), Colors::TextDim, TextStyle::Caption);
}

} // namespace UI::SharedSearchInput
