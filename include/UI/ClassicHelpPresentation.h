#pragma once
#include "UI/InventoryHelp.h"

#include <string>
#include <vector>
namespace PokeBank::UIModel::ClassicHelp {
enum class Page { Hub,Pokemon,Party,Items,Trainer };
// Round 5 4D: every control named here performs that action in that generation.
// Round 7 (ReviewEntry.h): one Review key per page, the one its footer shows: Y on Trainer, Party
// and Gen V Boxes; + on Gen I-IV Boxes and the Bag (Y moves / removes there). The Gen I-IV save
// hub still reaches Review through the staged exit menu (B, then A in that menu:
// ClassicStagedExit::choose).
//
// Round 6 E05: every row after the banner starts with real controller keys, so the Help window
// draws them as the coloured badges (GlyphInstruction::helpLine; "A or tap" or "Review Changes"
// as a key part used to fall back to plain text). One Review route per page: the old trailing
// "Review: X Export Copy..." and "Review Changes   Available..." rows repeated it and described
// another screen's buttons. tests/test_classic_help_accuracy.cpp ties each row to its handler.
inline constexpr const char* kPlusReview = "+   Review staged changes     B   Return to the save hub";
inline constexpr const char* kYReview = "Y   Review staged changes     B   Return to the save hub";
inline constexpr const char* kExitMenu = "A / X / Y   Exit menu: A Review, X Export Copy, Y Discard and leave";
// ClassicPackedMoveOverlay (ClassicCarryRules::carryKey): A places a carried Pokemon or clone, B cancels.
inline constexpr const char* kCarryPlace = "A   While moving or cloning: place it here     B   While carrying: cancel";
// TrainerViewScreenBase: B or - closes this Help window (helpOverlayActive).
inline constexpr const char* kCloseHelp = "B / -   Close this Help";
inline std::vector<std::string> lines(Page page,int generation) {
    std::vector<std::string> out{
        "Original save unchanged; Export Copy writes a separate file",
        "D-pad / Left Stick   Choose a panel, row or Pokemon (hold to scroll)"};
    switch(page) {
        case Page::Hub:
            out.push_back("A   Open the highlighted Pokemon, Party, Items or Trainer panel");
            out.push_back("B   Leave this save; staged edits open the four-choice exit menu");
            out.push_back(kExitMenu);
            if(generation==5)out.push_back("+   Review staged Pokemon, Bag and Trainer changes");
            break;
        case Page::Pokemon:
            out.push_back("A   Actions: View, Edit, Move, Clone, Origin or Release");
            out.push_back("A   On an empty slot: create a Pokemon draft");
            out.push_back("L / R   Previous / next box     ZL / ZR   Jump boxes");
            out.push_back(generation==5?kYReview:kPlusReview);   // Gen I-IV Boxes: Y is Move
            // Round 11 H23 (owner): Y picks up, A places, B cancels (ClassicCarryRules, every
            // generation). Gen V's Y is Review, so its pickup is A -> Move / Clone (the first row).
            if(generation!=5)out.push_back("Y   Tap: pick up to move     Y   Hold: multi-select");
            out.push_back(kCarryPlace);
            out.push_back("Y   Release question: only Y releases; B cancels");
            break;
        case Page::Party:
            out.push_back("A   Open the action menu for the highlighted Party Pokemon");
            out.push_back("A   In that menu: View, Edit, Origin & Legality or Cancel (tap works too)");
            out.push_back(kYReview);
            break;
        case Page::Items:
            // Round 7 (B04): the Bag has ONE Help, shared with the Gen I-III inventory overlay
            // (InventoryHelp.h, "Inventory Controls").
            return InventoryHelp::lines(generation);
        case Page::Trainer:
            out.push_back(generation==5?"A   Edit Name, Money or Play Time; other facts are read-only":
                "A   Edit Name or Money; other native facts are read-only");
            out.push_back("Y   Review every staged change (or tap Review Changes)");
            out.push_back("B   Return to the save hub; edits stay staged");
            break;
    }
    out.push_back(kCloseHelp);
    return out;
}
}
