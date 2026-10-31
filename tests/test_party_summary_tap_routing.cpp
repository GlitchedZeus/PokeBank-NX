// Round 2 phase 13 / L06: a tap on the Boxes summary panel or on a Party Pokemon opens that
// generation's NATIVE Actions sheet (Gen III / IV / V), never the inherited generic sheet.
// 1. The pure routing rules, exhaustively: Gen4BoxTouchActions (Gen III + IV) and
//    decodeGen5BoxTouch (Gen V) over every hit id the box panel publishes.
// 2. The call sites, read from the shipped sources: the summary (BoxPokemonPanel id 2000) and party
//    tap branches reach the native beginActions / native A path before any generic fallthrough.
//
// Owner verification (one line): in Gen III, IV and V Boxes tap the selected Pokemon's summary panel,
// and in Party tap a Pokemon; each must open that game's own Actions sheet (no "Make Shiny" row).
#include "Integration/Gen5/Gen5BoxTouchNavigation.h"
#include "UI/Gen4BoxTouchActions.h"

#include <cassert>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

namespace B4 = PokeBank::UIModel::Gen4BoxTouchActions;
namespace G5 = PokeVault::Integration::Gen5;

namespace {

std::string slurp(const char* path) {
    std::ifstream in(path);
    assert(in && "run from the repository root");
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}
std::size_t at(const std::string& text, const std::string& needle, std::size_t from = 0) {
    const std::size_t pos = text.find(needle, from);
    if (pos == std::string::npos) std::cerr << "missing: " << needle << "\n";
    assert(pos != std::string::npos);
    return pos;
}

void gen34Rules() {
    constexpr int slots = 30;
    assert(B4::SummaryPanelTouchId == 2000); // the id BoxPokemonPanel publishes for the summary card
    for (int selected = -1; selected <= slots; ++selected) {
        const bool validSelection = selected >= 0 && selected < slots;
        // The summary panel opens the native sheet for exactly the selected slot.
        assert(B4::opensSelectedSlotActions(B4::SummaryPanelTouchId, selected, slots) == validSelection);
        for (int hit = -2; hit <= slots + 1; ++hit) {
            const bool cell = hit >= 0 && hit < slots;
            assert(B4::selectsTappedBoxSlot(hit, slots) == cell);
            assert(B4::opensSelectedSlotActions(hit, selected, slots) == (validSelection && hit == selected));
        }
    }
    // No geometry, no actions.
    assert(!B4::opensSelectedSlotActions(B4::SummaryPanelTouchId, 0, 0));
    assert(!B4::selectsTappedBoxSlot(0, 0));
}

void gen5Rules() {
    using K = G5::BoxTouchKind5;
    constexpr int slots = 30;
    for (int selected = -1; selected <= slots; ++selected) {
        const bool validSelection = selected >= 0 && selected < slots;
        const auto summary = G5::decodeGen5BoxTouch(2000, selected, slots);
        assert(summary.kind == (validSelection ? K::OpenSummary : K::Ignore));
        if (validSelection) assert(summary.slot == selected);
        for (int hit = 0; hit < slots; ++hit) {
            const auto a = G5::decodeGen5BoxTouch(hit, selected, slots);
            assert(a.slot == hit && a.kind == (hit == selected ? K::OpenSlot : K::FocusSlot));
        }
    }
    // Inherited header-rename / storage ids never reach anything.
    for (int hit : {1002, 1003, 3000, 6200, -1, 31})
        assert(G5::decodeGen5BoxTouch(hit, 0, slots).kind == K::Ignore);
}

// The summary-panel tap branch must call the native beginActions before the inherited party handler.
void gen34CallSites() {
    for (const char* path : {"src/UI/Gen3SharedPokemonSurface.inc", "src/UI/Gen4SharedPokemonSurface.inc"}) {
        const std::string s = slurp(path);
        const std::size_t summary = at(s, "tapped == PokeBank::UIModel::Gen4BoxTouchActions::SummaryPanelTouchId");
        const std::size_t native = at(s, "beginActions(screen);", summary);
        assert(native - summary < 400); // the very branch, not a later unrelated call
        const std::size_t generic = s.find("handleBasePartyTouch", summary);
        assert(generic == std::string::npos || native < generic);
    }
    // Gen IV party: the tapped Pokemon is selected first, then the native sheet opens.
    const std::string g4 = slurp("src/UI/Gen4SharedPokemonSurface.inc");
    const std::size_t party = at(g4, "if (tapped >= 6200 && tapped < 6206 &&");
    const std::size_t select = at(g4, "screen.selectedPartyIndex = tapped - 6200;", party);
    const std::size_t open = at(g4, "beginActions(screen);", select);
    assert(open - party < 500);
}

// Gen V turns both a party tap and the summary tap into the native A path (Browse -> beginActions).
void gen5CallSites() {
    const std::string s = slurp("src/UI/Gen5SharedReadOnlySurface.inc");
    const std::size_t party = at(s, "screen.selectedPartyIndex=tap-kGen5PartyTouchBase;");
    assert(at(s, "down|=HidNpadButton_A;", party) - party < 80);
    const std::size_t summary = at(s, "case TouchKind::OpenSummary:down|=HidNpadButton_A;break;");
    assert(at(s, "case TouchKind::OpenSlot:") < summary);
    assert(s.find("if(keys&HidNpadButton_A)beginActions(screen);") != std::string::npos);
}

} // namespace

int main() {
    gen34Rules();
    gen5Rules();
    gen34CallSites();
    gen5CallSites();
    std::cout << "L06 party / summary-panel taps open the native Actions sheet (Gen III, IV, V) PASS\n";
}
