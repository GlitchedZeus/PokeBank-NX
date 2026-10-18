// Gen I / Gen II Pokemon Actions sheet (owner campaign phase 3): only real commands, in the shared
// order View, Edit, Clone, Release, then Origin & Legality, Review, Cancel. No Make Shiny
// (shiny is edited inside the editor) and no greyed placeholders. Every row keeps its native
// action index, so controller order, touch ids and dispatch are unchanged.
#include "UI/ClassicPokemonActionMenu.h"

#include <cassert>
#include <cstring>
#include <fstream>
#include <initializer_list>
#include <iostream>
#include <iterator>
#include <string>

namespace {

namespace Menu = PokeBank::UIModel::ClassicPokemonActionMenu;
namespace Narrow = PokeBank::UIModel::NarrowPokemonMenu;
namespace Shared = PokeBank::UIModel::SharedPokemonEditor;
namespace Gen1 = PokeBank::UIModel::Gen1Editor;

void expectLabels(const Narrow::Model& model, std::initializer_list<const char*> labels) {
    assert(model.count == static_cast<int>(labels.size()));
    int i = 0;
    for (const char* label : labels) {
        const auto& row = model.rows[static_cast<std::size_t>(i)];
        if (std::strcmp(row.label, label) != 0)
            std::cerr << "row " << i << ": " << row.label << " != " << label << '\n';
        assert(std::strcmp(row.label, label) == 0);
        assert(row.enabled()); // nothing greyed: every listed command is real
        ++i;
    }
}

template <typename ActionSet>
void nativeIndicesAreIdentity(const Narrow::Model& model, const ActionSet& native) {
    // Each row is the native action at the same position: controller order == visual order.
    for (int i = 0; i < model.count; ++i) {
        const int n = model.rows[static_cast<std::size_t>(i)].nativeIndex;
        assert(n >= 0 && n < static_cast<int>(native.count));
        assert(model.visualIndex(n) == i);
        if (i > 0) assert(n > model.rows[static_cast<std::size_t>(i - 1)].nativeIndex);
    }
}

std::string read(const char* path) {
    std::ifstream in(path, std::ios::binary);
    assert(in);
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}
void has(const std::string& text, const char* needle) {
    if (text.find(needle) == std::string::npos) std::cerr << "Missing: " << needle << '\n';
    assert(text.find(needle) != std::string::npos);
}
void lacks(const std::string& text, const char* needle) {
    if (text.find(needle) != std::string::npos) std::cerr << "Unexpected: " << needle << '\n';
    assert(text.find(needle) == std::string::npos);
}

} // namespace

int main() {
    // Gen II: the live capability sets (ClassicReleaseActionFix: clone + release + legality).
    Shared::ActionCapabilities caps;
    caps.canClone = true;
    caps.canRemove = true;
    caps.hasLegalityProvenance = true;
    for (const bool pending : {false, true}) {
        const auto occupied = Shared::actionsForSlot(true, caps, pending);
        const auto model = Menu::fromShared(occupied);
        if (pending)
            expectLabels(model, {"View", "Edit", "Clone", "Release", "Origin & Legality",
                                 "Review Pending Changes", "Cancel"});
        else
            expectLabels(model, {"View", "Edit", "Clone", "Release", "Origin & Legality", "Cancel"});
        nativeIndicesAreIdentity(model, occupied);
        const auto empty = Shared::actionsForSlot(false, caps, pending);
        const auto emptyModel = Menu::fromShared(empty);
        if (pending) expectLabels(emptyModel, {"Add Pokemon", "Review Pending Changes", "Cancel"});
        else expectLabels(emptyModel, {"Add Pokemon", "Cancel"});
        nativeIndicesAreIdentity(emptyModel, empty);
    }
    // A capability the backend lacks is absent, not greyed.
    Shared::ActionCapabilities noRemove = caps;
    noRemove.canRemove = false;
    expectLabels(Menu::fromShared(Shared::actionsForSlot(true, noRemove, false)),
                 {"View", "Edit", "Clone", "Origin & Legality", "Cancel"});

    // Gen I: its padded table lists Cancel once.
    for (const bool pending : {false, true}) {
        const auto occupied = Gen1::actionsForSlot(true, pending);
        const auto model = Menu::fromGen1(occupied);
        if (pending)
            expectLabels(model, {"View", "Edit", "Clone", "Release", "Origin & Legality",
                                 "Review Pending Changes", "Cancel"});
        else
            expectLabels(model, {"View", "Edit", "Clone", "Release", "Origin & Legality", "Cancel"});
        nativeIndicesAreIdentity(model, occupied);
        const auto empty = Menu::fromGen1(Gen1::actionsForSlot(false, pending));
        if (pending) expectLabels(empty, {"Add Pokemon", "Review Pending Changes", "Cancel"});
        else expectLabels(empty, {"Add Pokemon", "Cancel"});
    }

    // Every Gen I/II sheet fits on screen at the shared compact BoxMenu row height (46 px rows,
    // 40 px cards; SharedMenuGeometry::rule(BoxMenu)).
    for (int count = 2; count <= 7; ++count) {
        const auto g = Narrow::geometry(count);
        assert(g.rowHeight == 46 && g.y >= 0 && g.y + g.height <= 720 - 56);
    }

    // The live Gen I / Gen II sheets build from this model; no Make Shiny anywhere in their lane.
    for (const char* path : {"src/UI/ClassicReleaseActionFix.inc", "src/UI/Gen2SharedSurfaceParity.inc",
                             "src/UI/Gen2SharedPokemonSurface.inc",
                             "src/UI/Gen1PokemonEditorOverlayUXCleanup2.inc"}) {
        const auto text = read(path);
        has(text, "ClassicPokemonActionMenu::from");
        lacks(text, "NarrowPokemonMenu::classic(");
        lacks(text, "NarrowPokemonMenu::gen1(");
        lacks(text, "\"Make Shiny\"");
    }
    const auto legacy = read("src/UI/TrainerViewScreenGSCOverlay.inc");
    lacks(legacy, "Make Shiny");
    lacks(legacy, "Make Non-Shiny");
    lacks(legacy, "SlotAction::Shiny");
    lacks(read("include/UI/PokemonViewActions.h"), "SlotAction::Shiny");

    // Gen I/II use the ONE shared builder: identical rows (label + native index) to S's
    // NarrowPokemonMenu for every capability / pending combination.
    auto same = [](const Narrow::Model& a, const Narrow::Model& b) {
        assert(a.count == b.count);
        for (int i = 0; i < a.count; ++i) {
            assert(std::strcmp(a.rows[static_cast<std::size_t>(i)].label, b.rows[static_cast<std::size_t>(i)].label) == 0);
            assert(a.rows[static_cast<std::size_t>(i)].nativeIndex == b.rows[static_cast<std::size_t>(i)].nativeIndex);
        }
    };
    for (int bits = 0; bits < 16; ++bits) {
        Shared::ActionCapabilities caps;
        caps.canClone = bits & 1;
        caps.canRemove = bits & 2;
        caps.hasLegalityProvenance = bits & 4;
        const bool pending = bits & 8;
        for (bool occupied : {true, false})
            same(Menu::fromShared(Shared::actionsForSlot(occupied, caps, pending)),
                 Narrow::classic(Shared::actionsForSlot(occupied, caps, pending)));
    }
    for (bool occupied : {true, false})
        for (bool pending : {true, false})
            same(Menu::fromGen1(Gen1::actionsForSlot(occupied, pending)),
                 Narrow::gen1(Gen1::actionsForSlot(occupied, pending)));
    std::cout << "Gen I/II Pokemon Actions sheet: real commands only, shared order PASS\n";
    return 0;
}
