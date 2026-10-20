// R2-SEARCH: live incremental species search in the shared Gen I-V species chooser.
// Host-only: real species names (src/Names/SpeciesNames.cpp), the shared chooser model
// (include/UI/SharedSpeciesPicker.h) and the app-owned key grid (include/UI/SharedSearchInput.h).
#include "Names/SpeciesNames.h"
#include "UI/SharedSearchInput.h"
#include "UI/SharedSpeciesPicker.h"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

namespace Search = UI::SharedSearchInput;
namespace Picker = UI::SharedSpeciesPicker;
using Keys = PokeBank::UIModel::SharedPickerPaging::Keys;

namespace {

const auto kName = [](uint16_t species) { return std::string(Names::getSpeciesName(species)); };
const auto kAll = [](uint16_t) { return true; };

int keyOf(char c) {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= '0' && c <= '9') return 26 + (c - '0');
    assert(false);
    return -1;
}

// Brute-force expectation: dex numbers in [1, count] whose folded name starts with the folded query.
std::vector<uint16_t> expectPrefix(int count, const std::string& query) {
    std::vector<uint16_t> out;
    for (int s = 1; s <= count; ++s)
        if (Search::matchesPrefix(kName(static_cast<uint16_t>(s)), query)) out.push_back(static_cast<uint16_t>(s));
    return out;
}

Picker::Handled frame(Picker::Chooser& c, uint16_t& species, int count, const Picker::Input& in) {
    return Picker::handle(c, in, species, count, 100, 76, kAll, kName);
}

void testNormalisation() {
    assert(Search::searchKey("Mr. Mime") == "MRMIME");
    assert(Search::searchKey("Farfetch'd") == "FARFETCHD");
    assert(Search::searchKey("Porygon2") == "PORYGON2");
    assert(Search::searchKey("Nidoran\xE2\x99\x80") == "NIDORANF");   // ♀
    assert(Search::searchKey("Nidoran\xE2\x99\x82") == "NIDORANM");   // ♂
    assert(Search::searchKey("Ho-Oh") == "HOOH");
    assert(Search::searchKey("Flab\xC3\xA9" "b\xC3\xA9") == "FLABEBE");   // é folds to E
    assert(Search::searchKey("bulbasaur") == "BULBASAUR");
    assert(Search::matchesPrefix("Mr. Mime", "mrm"));
    assert(Search::matchesPrefix("Nidoran\xE2\x99\x80", "NIDORANF"));
    assert(!Search::matchesPrefix("Nidoran\xE2\x99\x80", "NIDORANM"));
    assert(Search::matchesPrefix("Charmander", ""));
    assert(!Search::matchesPrefix("Charmander", "HAR"));              // prefix, not substring
    std::cout << "normalisation ok\n";
}

void testTypingNarrowsDeletingReverts() {
    Picker::Chooser c;
    uint16_t species = 1;
    Picker::refilter(c, 151, kAll, kName);
    assert(c.count() == 151);
    c.search.openGrid();
    assert(Search::apply(c.search, keyOf('C')) == Search::Event::QueryChanged);
    Picker::refilter(c, 151, kAll, kName);
    assert(c.matches == expectPrefix(151, "C"));
    assert(c.count() > 5 && c.matches.front() == 4);                 // Charmander is the first C name
    assert(Search::apply(c.search, keyOf('O')) == Search::Event::QueryChanged);
    Picker::refilter(c, 151, kAll, kName);
    assert(c.matches == expectPrefix(151, "CO"));
    for (uint16_t s : c.matches) assert(Search::searchKey(kName(s)).rfind("CO", 0) == 0);
    assert(Search::apply(c.search, Search::Backspace) == Search::Event::QueryChanged);
    Picker::refilter(c, 151, kAll, kName);
    assert(c.matches == expectPrefix(151, "C"));                      // delete reverts to C
    assert(Search::apply(c.search, Search::Clear) == Search::Event::QueryChanged);
    Picker::refilter(c, 151, kAll, kName);
    assert(c.count() == 151 && c.search.query.empty());               // clear: full list
    assert(Search::apply(c.search, Search::Clear) == Search::Event::Consumed);   // nothing to clear
    assert(Search::apply(c.search, Search::Backspace) == Search::Event::Consumed);
    // The snapped species follows the filter: Bulbasaur is not a C name.
    species = 1;
    assert(Search::apply(c.search, keyOf('C')) == Search::Event::QueryChanged);
    Picker::Input idle;
    frame(c, species, 151, idle);
    assert(species == 4);                                             // Charmander: first C at/after 1
    std::cout << "typing / deleting / clearing ok\n";
}

void testExactMatchAndLongLists() {
    Picker::Chooser c;
    c.search.openGrid();
    for (char ch : std::string("MEW")) Search::apply(c.search, keyOf(ch));
    Picker::refilter(c, 151, kAll, kName);
    assert(c.matches == (std::vector<uint16_t>{150, 151}));           // Mewtwo, Mew
    Search::apply(c.search, keyOf('T'));
    Picker::refilter(c, 151, kAll, kName);
    assert(c.matches == (std::vector<uint16_t>{150}));                // exact-ish: Mewtwo only
    // Gen V long list: every P name, and the chooser never offers more than the bound.
    Picker::Chooser g5;
    g5.search.openGrid();
    Search::apply(g5.search, keyOf('P'));
    Picker::refilter(g5, 649, kAll, kName);
    assert(g5.matches == expectPrefix(649, "P"));
    assert(g5.count() > 20 && g5.matches.back() <= 649);
    // Query length cap.
    Picker::Chooser cap;
    cap.search.openGrid();
    for (int i = 0; i < Search::kMaxQuery + 3; ++i) Search::apply(cap.search, keyOf('A'));
    assert(static_cast<int>(cap.search.query.size()) == Search::kMaxQuery);
    std::cout << "exact matches / long lists ok\n";
}

void testPerGenerationAvailability() {
    // Gen I never offers Gen II+ species even when the query only matches later names.
    Picker::Chooser g1;
    g1.search.openGrid();
    for (char ch : std::string("CHIK")) Search::apply(g1.search, keyOf(ch));   // Chikorita (#152)
    Picker::refilter(g1, 151, kAll, kName);
    assert(g1.matches.empty());
    Picker::refilter(g1, 251, kAll, kName);                          // same query, Gen II bound
    assert(g1.matches == (std::vector<uint16_t>{152}));
    // Gen IV/V use the game's native list: a gap in it is never shown.
    const std::vector<uint16_t> native{1, 4, 7, 25, 150, 151, 493};
    const auto available = [&](uint16_t s) {
        for (uint16_t n : native) if (n == s) return true;
        return false;
    };
    Picker::Chooser g4;
    Picker::refilter(g4, 493, available, kName);
    assert(g4.matches == native);
    g4.search.openGrid();
    Search::apply(g4.search, keyOf('C'));
    Picker::refilter(g4, 493, available, kName);
    assert(g4.matches == (std::vector<uint16_t>{4}));                 // Charmander; Caterpie (10) not native
    // No-results state blocks A.
    Search::apply(g4.search, keyOf('Z'));
    uint16_t species = 4;
    Picker::Input a; a.a = true;
    const auto h = Picker::handle(g4, a, species, 493, 100, 76, available, kName);
    assert(g4.count() == 0 && h.blockSelect && h.keys);
    std::cout << "per-generation availability ok\n";
}

void testControllerTransitions() {
    Picker::Chooser c;
    uint16_t species = 25;                                            // Pikachu
    Picker::Input in;
    // X / Minus opens the grid; the frame is consumed so the owner does not treat it as browsing.
    in.search = true;
    auto h = frame(c, species, 151, in);
    assert(h.keys && c.searching() && c.visibleRows() == Picker::kSearchVisibleRows);
    assert(species == 25);
    // D-pad moves the key cursor, not the list; A types the focused key.
    in = {};
    in.nav.right = true;                                              // A -> B
    h = frame(c, species, 151, in);
    assert(h.keys && c.search.cursor == keyOf('B') && species == 25);
    in = {}; in.a = true;
    h = frame(c, species, 151, in);
    assert(h.queryChanged && c.search.query == "B");
    assert(species == 69);                                            // snapped: Bellsprout, first B at/after 25
    // Down from row 0 col 1 lands on L (row 1 col 1); Up wraps to row 3.
    in = {}; in.nav.down = true;
    frame(c, species, 151, in);
    assert(c.search.cursor == keyOf('L'));
    in = {}; in.nav.up = true; frame(c, species, 151, in);
    in = {}; in.nav.up = true; frame(c, species, 151, in);
    assert(Search::keyRow(c.search.cursor) == 3);
    // Left wraps within the row; the Done key spans two columns and is reachable.
    c.search.cursor = Search::Done;
    in = {}; in.nav.right = true; frame(c, species, 151, in);
    assert(c.search.cursor == keyOf('4'));                            // wrapped to the row start
    // B closes the grid and KEEPS the filter; the owner's own B is not reached this frame.
    in = {}; in.b = true;
    h = frame(c, species, 151, in);
    assert(h.keys && !c.searching() && c.search.query == "B" && c.visibleRows() == Picker::kVisibleRows);
    // With the grid closed, keys are the owner's: nothing consumed, paging runs over the filter.
    in = {}; in.nav.down = true;
    h = frame(c, species, 151, in);
    assert(!h.keys);
    const int idx = Picker::indexOf(c, species);
    const uint16_t next = Picker::paged(c, species, in.nav);
    assert(idx >= 0 && next == c.matches[static_cast<std::size_t>((idx + 1) % c.count())]);
    assert(Search::searchKey(kName(next))[0] == 'B');
    assert(Picker::paged(c, c.matches.back(), in.nav) == c.matches.front());   // Down wraps
    Keys page; page.r = true;
    assert(Picker::paged(c, c.matches.front(), page) == c.matches[std::min<std::size_t>(10, c.matches.size() - 1)]);
    assert(Picker::neighbour(c, c.matches.front(), -3) == c.matches.front());  // drag clamps
    assert(Picker::neighbour(c, c.matches.front(), 2) == c.matches[2]);
    // Done key closes too.
    in = {}; in.search = true; frame(c, species, 151, in);
    c.search.cursor = Search::Done;
    in = {}; in.a = true;
    h = frame(c, species, 151, in);
    assert(h.keys && !c.searching() && c.search.query == "B");
    std::cout << "controller transitions ok\n";
}

void testTouchKeysAndRows() {
    Picker::Chooser c;
    uint16_t species = 1;
    Picker::Input in;
    in.search = true;
    frame(c, species, 151, in);
    const auto grid = Picker::keyGridRect(100, 76);
    assert(grid.y == 76 + Picker::kListY + Picker::kSearchVisibleRows * Picker::kRowStep + 8);
    assert(grid.y + grid.h <= 76 + Picker::ModalHeight - 24);          // the grid stays inside the modal
    const auto keyC = Search::keyRect(grid, keyOf('C'));
    // Touch down on C, release on C: types C. The frame's touch is the grid's.
    in = {}; in.touch = {true, true, false, keyC.x + 5, keyC.y + 5, keyC.x + 5, keyC.y + 5};
    auto h = frame(c, species, 151, in);
    assert(h.touch && c.search.pressedKey == keyOf('C') && c.search.query.empty());
    in = {}; in.touch = {false, false, true, keyC.x + 5, keyC.y + 5, keyC.x + 5, keyC.y + 5};
    h = frame(c, species, 151, in);
    assert(h.touch && h.queryChanged && c.search.query == "C" && species == 4);
    // Down on C, slide off, release elsewhere: nothing typed.
    const auto keyD = Search::keyRect(grid, keyOf('D'));
    in = {}; in.touch = {true, true, false, keyC.x + 5, keyC.y + 5, keyC.x + 5, keyC.y + 5};
    frame(c, species, 151, in);
    in = {}; in.touch = {true, false, false, keyD.x + 5, keyD.y + 5, keyC.x + 5, keyC.y + 5};
    h = frame(c, species, 151, in);
    assert(h.touch && c.search.pressedKey < 0);
    in = {}; in.touch = {false, false, true, keyD.x + 5, keyD.y + 5, keyC.x + 5, keyC.y + 5};
    h = frame(c, species, 151, in);
    assert(h.touch && !h.queryChanged && c.search.query == "C");
    // A touch away from the grid and the scrollbar is the owner's (row tap / drag).
    in = {}; in.touch = {true, true, false, 300, 150, 300, 150};
    h = frame(c, species, 151, in);
    assert(!h.touch && !h.keys);
    // Scrollbar: published by draw; emulate the publish and drag the thumb to the end.
    // (The nine C names fit the window, so widen back to the full dex first.)
    Search::apply(c.search, Search::Clear);
    c.search.closeGrid();
    in = {};
    frame(c, species, 151, in);
    assert(c.count() == 151);
    c.scrollbar.track = UI::SharedScrollbar::layout(100 + Picker::kListX + Picker::kListW + 8, 76 + Picker::kListY,
        UI::SharedScrollbar::kTrackWidth, Picker::kVisibleRows * Picker::kRowStep - 4,
        UI::SharedScrollbar::rows(c.count(), Picker::kVisibleRows, 0));
    assert(c.scrollbar.track.visible);
    const int tx = c.scrollbar.track.x + 2, ty = c.scrollbar.track.thumbY + 4;
    in = {}; in.touch = {true, true, false, tx, ty, tx, ty};
    h = frame(c, species, 151, in);
    assert(h.touch && c.scrollbar.dragging);
    in = {}; in.touch = {true, false, false, tx, ty + 2000, tx, ty};
    h = frame(c, species, 151, in);
    assert(h.touch && species >= c.matches[static_cast<std::size_t>(c.count() - Picker::kVisibleRows)]);  // window at the end
    in = {}; in.touch = {false, false, true, tx, ty + 2000, tx, ty};
    h = frame(c, species, 151, in);
    assert(h.touch && !c.scrollbar.dragging);                         // release never selects
    std::cout << "touch keys / rows / scrollbar ok\n";
}

void testSelectedTitle() {
    // NEW-11: the selected-species panel reads "Pikachu #25" (name first); lists keep "025 ...".
    assert(Picker::selectedTitle(25, "Pikachu") == "Pikachu #25");
    assert(Picker::selectedTitle(493, "Arceus") == "Arceus #493");
    std::cout << "selected title ok\n";
}

void testGridGeometry() {
    const Search::Rect grid{0, 0, 560, 176};
    // Every key has a cell, cells do not overlap, and hit == rect.
    for (int k = 0; k < Search::kKeyCount; ++k) {
        const auto r = Search::keyRect(grid, k);
        assert(r.w > 0 && r.h > 0);
        assert(Search::hitKey(grid, r.x + 1, r.y + 1) == k);
        for (int j = 0; j < k; ++j) {
            const auto o = Search::keyRect(grid, j);
            const bool overlap = r.x < o.x + o.w && o.x < r.x + r.w && r.y < o.y + o.h && o.y < r.y + r.h;
            assert(!overlap);
        }
    }
    assert(Search::keyAt(2, 6) == keyOf('0') && Search::keyAt(3, 9) == Search::Done);
    assert(Search::keySpan(Search::Done) == 2);
    assert(Search::hitKey(grid, 600, 10) < 0);
    std::cout << "grid geometry ok\n";
}

} // namespace

int main() {
    testNormalisation();
    testTypingNarrowsDeletingReverts();
    testExactMatchAndLongLists();
    testPerGenerationAvailability();
    testControllerTransitions();
    testTouchKeysAndRows();
    testGridGeometry();
    testSelectedTitle();
    std::cout << "test_species_search_gen15: all passed\n";
    return 0;
}
