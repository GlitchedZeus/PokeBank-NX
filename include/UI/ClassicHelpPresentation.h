#pragma once
#include <string>
#include <vector>
namespace PokeBank::UIModel::ClassicHelp {
enum class Page { Hub,Pokemon,Party,Items,Trainer };
// Round 5 4D: every control named here performs that action in that generation. Plus opens Review
// only in Gen V (Gen5SharedReadOnlySurface openReviewFromBrowse); in Gen I-III Bags it opens
// Inventory Options (ClassicInventoryOverlay); Gen I-IV reach Review from the Trainer page (Y) or
// the staged exit menu (B).
inline constexpr const char* kReviewRoute = "Review   Trainer page Y, or B for the staged exit menu";
inline std::vector<std::string> lines(Page page,int generation) {
    std::vector<std::string> out{
        "Original save unchanged; Export Copy writes a separate file",
        "D-pad / Left Stick   Choose a panel, row or Pokemon (hold to scroll)"};
    switch(page) {
        case Page::Hub:
            out.push_back("A   Open the highlighted Pokemon, Party, Items or Trainer panel");
            out.push_back("B   Leave this save; staged edits open the four-choice exit menu");
            if(generation==5)out.push_back("+   Review staged Pokemon, Bag and Trainer changes");
            else out.push_back("Review Changes   Available from the staged exit menu");
            break;
        case Page::Pokemon:
            out.push_back("A   Actions: View, Edit, Move, Clone, Origin or Release");
            out.push_back("A on an empty slot   Create a Pokemon draft");
            out.push_back("L / R   Previous / next box     ZL / ZR   Jump boxes");
            out.push_back(generation==5?"+   Review staged changes     B   Return to the save hub":
                "B   Return to the save hub; edits stay staged");
            if(generation!=5)out.push_back(kReviewRoute);
            out.push_back("Moving a Pokemon   Y places it, B cancels");
            break;
        case Page::Party:
            out.push_back("A or tap   View / Edit / Origin & Legality / Cancel");
            out.push_back("B   Return to the save hub; edits stay staged");
            out.push_back("Review Changes   Available from the staged exit menu");
            break;
        case Page::Items:
            out.push_back("L / R   Previous / next inventory pocket");
            out.push_back("A   Edit a stack quantity; unique items stay at one");
            out.push_back("X   Add an item at one     Y   Ask to remove the selected item");
            out.push_back("Remove question: A confirms, B cancels");
            if(generation==5)out.push_back("+   Review staged changes     B   Return to the save hub");
            else if(generation<=3)out.push_back("+   Inventory Options: Review, Export or Discard     B   Back");
            else {
                out.push_back("B   Return to the save hub; edits stay staged");
                out.push_back(kReviewRoute);
            }
            break;
        case Page::Trainer:
            out.push_back(generation==5?"A   Edit Name, Money or Play Time; other facts are read-only":
                "A   Edit Name or Money; other native facts are read-only");
            out.push_back("Y or Review button   Review every staged change");
            out.push_back("B   Return to the save hub; edits stay staged");
            break;
    }
    out.push_back("Review: X Export Copy, Y asks to Discard All, B returns");
    out.push_back("Release / Backup Delete   Only Y confirms; B cancels");
    return out;
}
}
