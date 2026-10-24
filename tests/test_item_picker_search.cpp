// NEW-06 (Round 3): Held Item search alongside sorting (UI/ItemPickerSearch.h), and its wiring in
// the Gen II, Gen III and Gen IV/V Held Item pickers. Host-only; the presentation copy is filtered,
// the native list is never changed.
#include "UI/ItemPickerSearch.h"

#include <cassert>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <string>
#include <vector>

namespace {
namespace Search = UI::ItemPickerSearch;
using Mode = UI::ItemPickerSort::Mode;

const std::map<uint16_t, std::string> kNames{
    {0, "None"}, {1, "Master Ball"}, {13, "Potion"}, {139, "Oran Berry"}, {149, "Cheri Berry"},
    {187, "King's Rock"}, {200, "Leftovers"}, {289, "TM01 \xE2\x80\x94 Focus Punch"}, {4, "Pok\xC3\xA9 Ball"}};
const auto nameFor = [](uint16_t id) { return kNames.at(id); };

std::string read(const char* path) {
    std::ifstream in(path);
    assert(in && "source readable");
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}

void testMatching() {
    assert(Search::matches("King's Rock", "kings"));            // punctuation and case ignored
    assert(Search::matches("King's Rock", "s ro"));             // spaces ignored
    assert(Search::matches("Oran Berry", "BERRY"));
    assert(!Search::matches("Leftovers", "berry"));
    assert(Search::matches("Anything", ""));                    // empty query matches all
    assert(Search::matches("Pok\xC3\xA9 Ball", "pok\xC3\xA9"));   // UTF-8 kept
    assert(Search::matches("TM01 \xE2\x80\x94 Focus Punch", "focus"));
    std::cout << "matching ok\n";
}

void testApplyKeepsSelectionAndNative() {
    const std::vector<uint16_t> native{0, 1, 4, 13, 139, 149, 187, 200, 289};
    std::vector<uint16_t> shown = native;
    int row = 4;   // Oran Berry
    row = Search::apply(shown, native, Mode::Native, "berry", row, nameFor);
    assert((shown == std::vector<uint16_t>{139, 149}) && row == 0);         // native order kept
    assert(native.size() == 9);                                            // native list untouched
    row = Search::apply(shown, native, Mode::NameAZ, "berry", 1, nameFor); // Cheri focused
    assert((shown == std::vector<uint16_t>{149, 139}) && row == 0);         // A-Z, Cheri still focused
    // A query with no match leaves the list and focus exactly as they were.
    const auto before = shown;
    assert(Search::apply(shown, native, Mode::NameAZ, "zzz", row, nameFor) == -1 && shown == before);
    // Clearing the query restores the whole list in the current sort, focus kept.
    row = Search::apply(shown, native, Mode::NameAZ, "", 0, nameFor);
    assert(shown.size() == native.size() && shown[static_cast<std::size_t>(row)] == 149);
    // A filtered-out focus falls back to the first match.
    shown = native;
    row = Search::apply(shown, native, Mode::Native, "rock", 1 /* Master Ball */, nameFor);
    assert((shown == std::vector<uint16_t>{187}) && row == 0);
    std::cout << "apply keeps selection / native ok\n";
}

void testSortKeepsSearch() {
    // X re-sorts the whole native list, then narrow() re-applies the active query.
    const std::vector<uint16_t> native{0, 1, 4, 13, 139, 149, 187, 200, 289};
    std::vector<uint16_t> shown{139, 149};
    int row = UI::ItemPickerSort::reorder(shown, native, Mode::NameZA, 1, nameFor);
    row = Search::narrow(shown, "berry", row, nameFor);
    assert((shown == std::vector<uint16_t>{139, 149}) && shown[static_cast<std::size_t>(row)] == 149);
    assert(Search::narrow(shown, "", 1, nameFor) == 1 && shown.size() == 2);   // no query: unchanged
    assert(Search::tag("") .empty() && Search::tag("berry") == "Search: berry");
    std::cout << "sort keeps search ok\n";
}

void testWiring() {
    struct Surface { const char* path; const char* query; const char* open; };
    for (const Surface s : {Surface{"src/UI/Gen2PokemonEditorFoundation.inc", "state.itemQuery", "state.itemQuery.clear();"},
                            Surface{"src/UI/Gen3SharedPokemonSurface.inc", "state.pickerQuery", "state.pickerQuery.clear();"},
                            Surface{"src/UI/SharedPokemonEditorSurface.inc", "state.pickerQuery", "state.pickerQuery.clear();"}}) {
        const std::string src = read(s.path);
        assert(src.find("#include \"UI/ItemPickerSearch.h\"") != std::string::npos);
        assert(src.find("HidNpadButton_Minus)) {") != std::string::npos || src.find("HidNpadButton_Minus) {") != std::string::npos);
        assert(src.find("Utils::promptText(\"Search Held Items\", \"Part of an item name\"") != std::string::npos);
        assert(src.find("ItemPickerSearch::apply(") != std::string::npos);    // live, never-empty filter
        assert(src.find("ItemPickerSearch::narrow(") != std::string::npos);   // X sort keeps the search
        assert(src.find("ItemPickerSearch::tag(") != std::string::npos);      // header shows the query
        assert(src.find("footer.minusLabel = \"Search\";") != std::string::npos);
        assert(src.find(s.open) != std::string::npos);                       // a new picker starts unfiltered
        assert(src.find(std::string("Utils::promptText(\"Search Held Items\", \"Part of an item name\", ") + s.query) != std::string::npos);
    }
    const std::string footer = read("include/UI/SharedEditorPresentation.h");
    assert(footer.find("if (spec.minusLabel) f.push(\"-\", spec.minusLabel);") != std::string::npos);
    std::cout << "Gen II / III / IV-V held item search wiring ok\n";
}
} // namespace

int main() {
    testMatching();
    testApplyKeepsSelectionAndNative();
    testSortKeepsSearch();
    testWiring();
    std::cout << "test_item_picker_search: all passed\n";
    return 0;
}
