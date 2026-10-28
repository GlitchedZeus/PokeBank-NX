#pragma once

#include <string>
#include <vector>

// Round 7 (owner B04 / B03 follow-up, 26186464): ONE Inventory Help for the Gen I-V Bag, laid out
// like the Gen IV / V Bag the owner prefers, titled "Inventory Controls" everywhere (the Gen V Bag
// used to say "Game Browser Controls"; the Gen I-III Bag had its own "Inventory Help" with stale
// "+ Inventory Options" and "X Add remains available" rows).
//
// Every row after the banner starts with real controller keys (drawn as badges by
// GlyphInstruction::helpLine) and names what that key does on the Bag of that generation:
//   - A edits a stack quantity (Gen I-III ClassicInventoryOverlay classicInventoryAction, Gen IV
//     Gen4BagQuantity, Gen V Gen5BagScreen availability); unique items keep 1;
//   - X adds from the exact game's catalog; Y asks to remove; A / B answer that question
//     (classicRemoveConfirmStep, the Gen IV item-remove confirm, Gen V ConfirmRemove);
//   - L / R change category (Gen I-III) or pocket (Gen IV / V);
//   - + opens the shared Review (ReviewEntry: + is the Bag's Review key in every generation; the
//     Gen V Bag offers it once something is staged, as its footer does);
//   - B goes back; B or - closes this Help.
// tests/test_classic_inventory_ui_model.cpp pins the rows; tests/test_footer_help_handlers.py maps every
// row key to each generation's Bag input code.
namespace PokeBank::UIModel::InventoryHelp {

inline constexpr const char* kTitle = "Inventory Controls";
inline constexpr const char* kCloseHelp = "B / -   Close this Help";

inline std::vector<std::string> lines(int generation) {
    const bool classic = generation >= 1 && generation <= 3;
    std::vector<std::string> out{
        "Original save unchanged; Bag edits are staged until Export Copy",
        "D-pad / Left Stick   Choose an item (hold to scroll)",
        classic ? "L / R   Previous / next category" : "L / R   Previous / next pocket",
        "A   Edit the quantity (items with an editable quantity; unique items stay at 1)",
        "X   Add an item from this game's catalog     Y   Ask to remove the selected item",
        "A / B   Remove question: A removes, B cancels",
        generation == 5 ? "+   Review staged changes (shown once something is staged)"
                        : "+   Review staged changes (Export Copy and Discard All are there)",
        "B   Back to the save hub; edits stay staged",
        kCloseHelp,
    };
    return out;
}

} // namespace PokeBank::UIModel::InventoryHelp
