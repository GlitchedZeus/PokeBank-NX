#include "UI/Gen1PokemonEditorUX3Contract.h"
#include "UI/NavigationRepeat.h"

#include <cassert>
#include <iostream>

using namespace PokeBank::UIModel::Gen1Editor::UX3;

int main() {
    static_assert(usesPkseThreeColumnSkeleton());
    static_assert(addAndEditShareWorkspace());
    static_assert(workspaceRowCount() == 14);
    static_assert(!usesThirtyRawRows());
    static_assert(!usesWizardPages());
    constexpr auto fields = workspaceFields();
    static_assert(fields[0] == Field::Species);
    static_assert(fields[3] == Field::Shiny);
    static_assert(fields[4] == Field::Move1 && fields[7] == Field::Move4);
    static_assert(fields[8] == Field::DVs && fields[9] == Field::StatExperience);
    constexpr auto starts = sectionStarts();
    static_assert(starts[0] == 0 && starts[1] == 4 && starts[2] == 8 && starts[3] == 10 && starts[4] == 12);

    static_assert(radarLivesInEditorInfoRegion());
    static_assert(radarAxisCount() == 5);
    static_assert(!radarSplitsSpecial());
    static_assert(!shinyIsStoredBoolean());
    static_assert(speciesPickerUsesNumberedRows());
    static_assert(speciesPickerShowsNormalAndShinyPreview());
    static_assert(!speciesPickerBrowseMutatesDraft());
    static_assert(!speciesPickerBrowseMutatesStagedSave());
    static_assert(movePickerIsSpeciesGameFiltered());
    static_assert(!fullEncounterLegalityIsImplemented());
    static_assert(viewIsReadOnly());
    static_assert(viewUsesSharedPkseStyleStructure());
    static_assert(cloneOccupiedCellsShowSprite());
    static_assert(!cloneBrowseMutatesStagedSave());
    static_assert(!shinyPreviewMutatesDraft());
    static_assert(!shinyPreviewMutatesStagedSave());
    static_assert(!movePickerBrowseMutatesStagedSave());
    static_assert(!liveSourceWritingEnabled());
    static_assert(!partyEditingEnabled());

    constexpr auto one = pendingGeometry(1);
    static_assert(one.width == 680 && one.height == 220 && one.visibleRows == 1 && !one.scrolls);
    constexpr auto few = pendingGeometry(4);
    static_assert(few.width == 820 && few.height == 372 && few.visibleRows == 4 && !few.scrolls);
    constexpr auto many = pendingGeometry(12);
    static_assert(many.width == 980 && many.height == 540 && many.visibleRows == 7 && many.scrolls);

    constexpr auto clone = cloneSpriteKey(25, true);
    static_assert(clone.species == 25 && clone.shiny);

    // The Cleanup #3 overlay must reuse the app's shared controller navigation implementation.
    // Prove the exact comfort behavior relied on by the Species/Move/Pending lists: neutral drift
    // does nothing, a clear tilt moves immediately, the lower release threshold prevents chatter,
    // and holding eventually repeats without repeated D-pad taps.
    constexpr std::uint64_t Up = 1, Down = 2, Left = 4, Right = 8;
    PokeBank::UIModel::ControllerNavigation nav;
    assert(nav.apply(0, 0, 4000, 5000, Up, Down, Left, Right) == 0); // deadzone
    assert((nav.apply(0, 0, 0, 20000, Up, Down, Left, Right) & Up) != 0); // immediate press
    assert(nav.apply(0, 0, 0, 12000, Up, Down, Left, Right) == 0); // hysteresis hold, no chatter
    bool repeated = false;
    for (int frame = 0; frame < 30; ++frame) {
        const auto event = nav.apply(0, 0, 0, 20000, Up, Down, Left, Right);
        if (event & Up) { repeated = true; break; }
    }
    assert(repeated);
    nav.reset();
    assert((nav.apply(0, 0, 20000, 3000, Up, Down, Left, Right) & Right) != 0); // dominant axis

    std::cout << "Gen I Cleanup #3 UX contract: PASS (PKSE skeleton, shiny, stick, pending, clone)\n";
    return 0;
}
