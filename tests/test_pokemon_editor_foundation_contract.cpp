#include "UI/PokemonEditorFoundationContract.h"

#include <cassert>

using namespace PokeBank::UIModel::PokemonEditorFoundation;

int main() {
    // All three permanent workspace panels are interactive.
    assert(panelIsInteractive(Panel::Identity));
    assert(panelIsInteractive(Panel::Values));
    assert(panelIsInteractive(Panel::Moves));

    // Picker/sub-editor surfaces always take ownership over the foundation workspace.
    // This is the hardware-regression contract for Species A -> picker, and prevents a
    // stale picker from hiding behind Level/EXP or any other contextual editor.
    static_assert(surfaceOwnerFor(true, false) == SurfaceOwner::FoundationWorkspace);
    static_assert(surfaceOwnerFor(true, true) == SurfaceOwner::Cleanup3);
    static_assert(surfaceOwnerFor(false, false) == SurfaceOwner::Cleanup3);
    static_assert(surfaceOwnerFor(false, true) == SurfaceOwner::Cleanup3);

    // Focus crosses the full workspace rather than living in one decorative middle column.
    Focus f{Panel::Identity, 1, 0};
    f = moveFocus(f, Direction::Right);
    assert(f.panel == Panel::Values);
    f = moveFocus(f, Direction::Right);
    assert(f.panel == Panel::Values && f.column == 1);
    f = moveFocus(f, Direction::Right);
    assert(f.panel == Panel::Values && f.column == 2);
    f = moveFocus(f, Direction::Right);
    assert(f.panel == Panel::Moves);
    f = moveFocus(f, Direction::Left);
    assert(f.panel == Panel::Values && f.column == 2);

    // Vertical movement remains local to the focused panel and wraps cleanly.
    assert((moveFocus({Panel::Identity, 0, 0}, Direction::Up) == Focus{Panel::Identity, 4, 0}));
    assert((moveFocus({Panel::Moves, 3, 0}, Direction::Down) == Focus{Panel::Moves, 0, 0}));

    // Gen I Values are truthful: HP DV and calculated stats are display-only, other DVs and
    // all five Stat Exp values are editable, while Shiny and Level remain real focusable rows.
    assert(hpDVIsDerived());
    assert(!valueCellEditable(ValueRow::HP, ValueColumn::DV));
    assert(valueCellEditable(ValueRow::Attack, ValueColumn::DV));
    assert(valueCellEditable(ValueRow::Defense, ValueColumn::DV));
    assert(valueCellEditable(ValueRow::Speed, ValueColumn::DV));
    assert(valueCellEditable(ValueRow::Special, ValueColumn::DV));
    assert(valueCellEditable(ValueRow::HP, ValueColumn::StatExperience));
    assert(valueCellEditable(ValueRow::Special, ValueColumn::StatExperience));
    assert(!valueCellEditable(ValueRow::Attack, ValueColumn::CalculatedStat));
    assert(calculatedStatsAreReadOnly());
    assert(moveRowsAreIndividuallyFocusable());
    assert(!radarIsFocusable());

    // Randomize DVs is generation-correct: every generated stored DV is exactly 0..15.
    uint32_t randomState = 0x13579BDFu;
    for (int i = 0; i < 256; ++i) {
        const auto dvs = randomDVs(randomState);
        for (const auto dv : dvs) assert(dv <= 15);
    }

    constexpr auto gen1 = capabilitiesForGeneration(Generation::Gen1);
    static_assert(gen1.usesDVs && gen1.usesStatExp && gen1.supportsShiny);
    static_assert(!gen1.usesIVs && !gen1.usesEVs && !gen1.hasSplitSpecial);
    static_assert(!gen1.supportsAbility && !gen1.supportsHeldItem && !gen1.supportsNature);
    static_assert(!gen1.supportsFriendship && !gen1.supportsEgg && !gen1.supportsMetLevel);
    static_assert(!gen1.supportsRibbons && !gen1.supportsMarks && !gen1.supportsGender);
    static_assert(!gen1HasFakeModernFields());
    static_assert(!supplementalPanelShowsHeldItem(Generation::Gen1));
    static_assert(!supplementalPanelShowsRibbons(Generation::Gen1));

    constexpr auto gen2 = capabilitiesForGeneration(Generation::Gen2);
    static_assert(gen2.supportsHeldItem && gen2.supportsFriendship && gen2.supportsGender);
    static_assert(gen2.usesDVs && gen2.usesStatExp);
    static_assert(!gen2.supportsNature && !gen2.supportsAbility && !gen2.supportsRibbons);
    static_assert(supplementalPanelShowsHeldItem(Generation::Gen2));
    static_assert(!supplementalPanelShowsRibbons(Generation::Gen2));

    // Synthetic Gen III contract proves the same shell can expose later-generation concepts
    // without implementing any unfinished serializer here.
    constexpr auto gen3 = capabilitiesForGeneration(Generation::Gen3);
    static_assert(gen3.supportsHeldItem && gen3.supportsNature && gen3.supportsAbility);
    static_assert(gen3.supportsRibbons && gen3.usesIVs && gen3.usesEVs);
    static_assert(gen3.hasSplitSpecial && gen3.supportsGender);
    static_assert(!gen3.usesDVs && !gen3.usesStatExp && !gen3.supportsMarks);
    static_assert(supplementalPanelShowsHeldItem(Generation::Gen3));
    static_assert(supplementalPanelShowsRibbons(Generation::Gen3));

    static_assert(gen1StatLabels()[0][0] == 'H');
    static_assert(gen1StatLabels()[4][0] == 'S');

    // Safety/read-only boundary is permanent UI architecture, not a mutable feature flag.
    static_assert(!previewNavigationMutatesSource());
    static_assert(!viewAllowsMutation());
    static_assert(!liveRetroArchWriteEnabled());
    static_assert(!liveInstalledGameWriteEnabled());
    static_assert(!liveOtherEmulatorWriteEnabled());
    static_assert(!partyEditEnabled());

    return 0;
}
