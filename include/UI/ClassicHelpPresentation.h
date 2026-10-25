#pragma once
#include <string>
#include <vector>
namespace PokeBank::UIModel::ClassicHelp {
enum class Page { Hub,Pokemon,Party,Items,Trainer };
// Round 5 4D: every control named here performs that action in that generation.
// Round 6 (ReviewEntry.h): + opens the shared Review from Boxes, Party, Bag, Create and Edit in
// every generation, and Y does too where Y is free (Trainer, Party; Gen V Boxes). The save hub of
// Gen I-IV still reaches Review through the staged exit menu.
inline constexpr const char* kPlusReview = "+   Review staged changes     B   Return to the save hub";
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
            out.push_back(kPlusReview);
            out.push_back("Moving a Pokemon   Y places it, B cancels");
            break;
        case Page::Party:
            out.push_back("A or tap   View / Edit / Origin & Legality / Cancel");
            out.push_back("B   Return to the save hub; edits stay staged");
            out.push_back("Y or +   Review staged changes; B there returns to this Party");
            break;
        case Page::Items:
            out.push_back("L / R   Previous / next inventory pocket");
            out.push_back("A   Edit a stack quantity; unique items stay at one");
            out.push_back("X   Add an item at one     Y   Ask to remove the selected item");
            out.push_back("Remove question: A confirms, B cancels");
            out.push_back(kPlusReview);
            break;
        case Page::Trainer:
            out.push_back(generation==5?"A   Edit Name, Money or Play Time; other facts are read-only":
                "A   Edit Name or Money; other native facts are read-only");
            out.push_back("Y, + or Review button   Review every staged change");
            out.push_back("B   Return to the save hub; edits stay staged");
            break;
    }
    out.push_back("Review: X Export Copy, Y asks to Discard All, B returns");
    out.push_back("Release / Backup Delete   Only Y confirms; B cancels");
    return out;
}
}
