// Round 6 (B08, P1-RETURN, AUD-01, 4D-HELP follow-up): one visible way into the shared Review
// from Trainer, Party, Boxes, Bag, Create and Edit in Gen I-V, and back to the same screen.
//   1. ReviewEntry's button rule for every generation x screen: + everywhere, Y only where Y is
//      free (never taking Bag Remove, Gen I-IV Box Move / Multi or the Create / Edit re-roll).
//   2. The production Gen I-IV Party / Boxes router (ClassicBrowseReview.h) driven on a stub screen
//      with the real ClassicReviewOrigin: Y / + open Review, a standalone close returns to the same
//      screen with cursor, box and page intact, Y on Boxes stays Move, modals and carry block it.
//   3. The footers that advertise it (Gen II workspace model, the staged Bag footer).
// Gen V's open/return path runs on the real Gen V state machine in
// test_gen5_review_discard_export_trace (workspaceReviewReturn).
#include "UI/ClassicBrowseReview.h"
#include "UI/ClassicReviewOrigin.h"
#include "UI/Gen2WorkspaceLayers.h"
#include "UI/InventoryUIContract.h"
#include "UI/ReviewEntry.h"

#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>

namespace {
namespace Entry = PokeBank::UIModel::ReviewEntry;
namespace Origin = PokeBank::UIModel::ClassicReviewOrigin;
namespace Browse = PokeBank::UIModel::ClassicBrowseReview;

constexpr std::uint64_t kY = Entry::kButtonY;
constexpr std::uint64_t kPlus = Entry::kButtonPlus;
constexpr std::uint64_t kA = 1ull << 0;

constexpr Entry::Screen kScreens[] = {Entry::Screen::Trainer, Entry::Screen::Party, Entry::Screen::Boxes,
                                      Entry::Screen::Bag, Entry::Screen::Create, Entry::Screen::Edit};

void buttonRule() {
    for (int gen = 1; gen <= 5; ++gen) {
        for (const auto screen : kScreens) {
            // + opens Review on every screen of every generation.
            assert(Entry::opensReview(screen, gen, kPlus));
            const bool yFree = screen == Entry::Screen::Trainer || screen == Entry::Screen::Party ||
                               (screen == Entry::Screen::Boxes && gen == 5);
            assert(Entry::opensReview(screen, gen, kY) == yFree);
            assert(!Entry::opensReview(screen, gen, kA) && !Entry::opensReview(screen, gen, 0));
            const auto hint = Entry::footerHint(screen, gen);
            assert(std::strcmp(hint.label, "Review") == 0);
            assert(std::strcmp(hint.glyph, yFree ? "Y" : "+") == 0);
            assert(std::string(Entry::footerText(screen, gen)) == (yFree ? "Y: Review" : "+: Review"));
        }
        // The screens whose Y already does needed work keep it.
        assert(!Entry::opensReview(Entry::Screen::Bag, gen, kY));      // Remove Item
        assert(!Entry::opensReview(Entry::Screen::Create, gen, kY));   // re-roll / confirm Discard
        assert(!Entry::opensReview(Entry::Screen::Edit, gen, kY));
        if (gen <= 4) assert(!Entry::opensReview(Entry::Screen::Boxes, gen, kY));   // Move / Multi
    }
    // Not a Gen I-V save: nothing.
    assert(!Entry::opensReview(Entry::Screen::Party, 0, kPlus | kY));
    assert(!Entry::opensReview(Entry::Screen::Party, 8, kPlus | kY));
    std::cout << "ReviewEntry button rule (Gen I-V x 6 screens) ok\n";
}

// The parts of TrainerViewScreen the router and the Review origin read.
struct Sheet {
    bool open = false;
    [[nodiscard]] bool isOpen() const { return open; }
};
struct Details {
    bool active = false;
};
struct StubScreen {
    enum class ViewMode { Party, Boxes, Items, Trainer, Storage };
    std::string sourceGameId;
    ViewMode selectedMode = ViewMode::Party;
    bool detailViewActive = true;
    int selectedItemIndex = 0, selectedBoxIndex = 0, selectedPartyIndex = 0, currentPage = 0;
    bool currentlySelecting = false, helpOverlayActive = false, saveConfirmActive = false, pickerActive = false,
         itemEditDialogActive = false, releaseConfirmActive = false, groupMenuActive = false,
         storageExitConfirmActive = false, carryingGroup = false;
    Details details;
    Sheet actionSheet;
    bool staged = true;
    bool reviewOpen = false;
    std::string status;
    [[nodiscard]] bool carrying() const { return carryingGroup; }
    void postStatus(const std::string& message, int) { status = message; }
    // As TrainerViewScreen::openStagedReview: open this generation's shared Review and mark it
    // standalone (opened outside Box Actions).
    bool openStagedReview(bool) {
        if (!staged) return false;
        reviewOpen = true;
        Origin::markStandalone(*this);
        return true;
    }
    // As every Gen I-IV Review exit (B, or a closing Discard All): a standalone Review closes to
    // the screen underneath; otherwise it would return to the Box Actions sheet.
    bool closeReview() {
        assert(reviewOpen);
        reviewOpen = false;
        return Origin::takeStandalone(*this);
    }
};

struct Where {
    StubScreen::ViewMode mode;
    int item, box, party, page;
};
Where where(const StubScreen& s) {
    return {s.selectedMode, s.selectedItemIndex, s.selectedBoxIndex, s.selectedPartyIndex, s.currentPage};
}
bool same(const Where& a, const Where& b) {
    return a.mode == b.mode && a.item == b.item && a.box == b.box && a.party == b.party && a.page == b.page;
}

void classicRouter() {
    const char* games[] = {"red_gb", "crystal_gbc", "emerald_gba", "platinum_nds"};
    for (int gen = 1; gen <= 4; ++gen) {
        StubScreen screen;
        screen.sourceGameId = games[gen - 1];

        // Party: Y and + both open the shared Review; closing returns to Party, same slot.
        for (const std::uint64_t button : {kY, kPlus}) {
            screen.selectedMode = StubScreen::ViewMode::Party;
            screen.selectedPartyIndex = 4;
            screen.selectedItemIndex = 4;
            const Where before = where(screen);
            assert(Browse::open(screen, button, gen, true) && screen.reviewOpen);
            assert(screen.closeReview());               // standalone: back to the screen underneath
            assert(same(where(screen), before) && !screen.reviewOpen);
        }

        // Boxes: + opens Review; closing returns to the same box, slot and page.
        screen.selectedMode = StubScreen::ViewMode::Boxes;
        screen.selectedBoxIndex = 7;
        screen.selectedItemIndex = 17;
        screen.currentPage = 1;
        Where before = where(screen);
        assert(Browse::open(screen, kPlus, gen, true) && screen.reviewOpen);
        assert(screen.closeReview() && same(where(screen), before));
        // Y on Boxes stays Move / Multi: not consumed here, so the move layer receives it.
        assert(!Browse::open(screen, kY, gen, true) && !screen.reviewOpen);

        // Never over a modal, a carry, an Actions sheet or another generation overlay.
        screen.carryingGroup = true;
        assert(!Browse::open(screen, kPlus, gen, true) && !screen.reviewOpen);
        screen.carryingGroup = false;
        screen.actionSheet.open = true;
        assert(!Browse::open(screen, kPlus, gen, true));
        screen.actionSheet.open = false;
        screen.details.active = true;
        assert(!Browse::open(screen, kPlus, gen, true));
        screen.details.active = false;
        assert(!Browse::open(screen, kPlus, gen, false) && !screen.reviewOpen);   // overlay owns the frame

        // Other screens route Review through their own handlers; the save hub is not a grid.
        screen.selectedMode = StubScreen::ViewMode::Items;
        assert(!Browse::open(screen, kPlus, gen, true));
        screen.selectedMode = StubScreen::ViewMode::Party;
        screen.detailViewActive = false;
        assert(!Browse::open(screen, kPlus | kY, gen, true));
        screen.detailViewActive = true;

        // Nothing staged: the press is answered, nothing opens.
        screen.staged = false;
        assert(Browse::open(screen, kPlus, gen, true) && !screen.reviewOpen && !screen.status.empty());
        screen.staged = true;

        // A Box Actions Review (not standalone) still returns to its Actions sheet.
        screen.selectedMode = StubScreen::ViewMode::Boxes;
        screen.reviewOpen = true;   // opened from Box Actions: no standalone mark
        assert(!screen.closeReview());
    }
    // Gen V is not routed here (its own state machine owns Party / Boxes).
    StubScreen gen5;
    gen5.sourceGameId = "black_nds";
    assert(!Browse::open(gen5, kPlus | kY, 5, true));
    std::cout << "Gen I-IV Party / Boxes open Review and return to the same screen ok\n";
}

void footers() {
    namespace L = PokeBank::UIModel::Gen2WorkspaceLayers;
    for (const auto session : {L::Session::Create, L::Session::Edit}) {
        const auto f = L::footer(L::Layer::Workspace, session);
        bool review = false, reroll = false;
        for (std::size_t i = 0; i < f.count; ++i) {
            if (std::strcmp(f.hints[i].glyph, "+") == 0 && std::strcmp(f.hints[i].label, "Review") == 0)
                review = true;
            if (std::strcmp(f.hints[i].glyph, "Y") == 0 && std::strcmp(f.hints[i].label, "Random DVs") == 0)
                reroll = true;
        }
        assert(review && reroll);
        assert(std::strcmp(f.hints[f.count - 1].glyph, "B") == 0);   // B Exit stays last
    }
    // The staged Gen I-III Bag footer says + Review; installed-title Bags keep + Options.
    const std::string staged = PokeBank::UIModel::inventoryFooter(true, true, true, true,
                                                                  Entry::footerText(Entry::Screen::Bag, 2));
    assert(staged.find("+: Review") != std::string::npos && staged.find("Y: Remove Item") != std::string::npos);
    assert(PokeBank::UIModel::inventoryFooter(true, true, true, true).find("+: Options") != std::string::npos);
    std::cout << "Review footers ok\n";
}
} // namespace

int main() {
    buttonRule();
    classicRouter();
    footers();
    std::cout << "test_review_entry: PASS\n";
    return 0;
}
