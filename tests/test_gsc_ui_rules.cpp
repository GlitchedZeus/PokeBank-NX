#include "UI/Gen2PokemonEditorRules.h"
#include "UI/LegacyPresentationRules.h"
#include "UI/PokemonEditorFoundationContract.h"

#include <algorithm>
#include <cassert>
#include <iostream>

int main() {
    using Enums::GameVersion;
    using UI::PokemonDetailsLayout;
    namespace Foundation = PokeBank::UIModel::PokemonEditorFoundation;
    namespace Gen2Rules = PokeBank::UIModel::Gen2PokemonEditor;

    static_assert(PokeVault::Integration::Gen1::kInventoryCategoryCount == 2);
    static_assert(PokeVault::Integration::Gen2::kInventoryPocketCount == 5);

    assert(UI::legacyInventoryCategoryCount(GameVersion::RBY) == 2);
    assert(UI::legacyInventoryCategoryCount(GameVersion::GSC) == 5);
    assert(UI::legacyInventoryCategoryCount(GameVersion::FRLG) == 0);

    assert(UI::pokemonDetailsLayoutFor(GameVersion::RBY) == PokemonDetailsLayout::Generation1);
    assert(UI::pokemonDetailsLayoutFor(GameVersion::GSC) == PokemonDetailsLayout::Generation2);
    assert(UI::pokemonDetailsLayoutFor(GameVersion::FRLG) == PokemonDetailsLayout::Modern);
    assert(UI::pokemonDetailsLayoutFor(GameVersion::SV) == PokemonDetailsLayout::Modern);

    constexpr auto gen2 = Foundation::capabilitiesForGeneration(Foundation::Generation::Gen2);
    static_assert(gen2.supportsHeldItem);
    static_assert(gen2.supportsFriendship);
    static_assert(gen2.supportsGender);
    static_assert(gen2.supportsShiny);
    static_assert(gen2.usesDVs && gen2.usesStatExp);
    static_assert(gen2.hasSplitSpecial);
    static_assert(!gen2.supportsAbility && !gen2.supportsNature);
    static_assert(!gen2.supportsRibbons && !gen2.supportsMarks);
    static_assert(!gen2.usesIVs && !gen2.usesEVs);
    static_assert(Foundation::gen2UsesSingleSpecialDVForSplitBattleStats());
    static_assert(!Foundation::partyEditEnabled());
    static_assert(!Foundation::liveRetroArchWriteEnabled());
    static_assert(!Foundation::liveInstalledGameWriteEnabled());
    static_assert(!Foundation::liveOtherEmulatorWriteEnabled());

    static_assert(Gen2Rules::passiveViewCompatibility(true) == Gen2Rules::CompatibilityState::Ok);
    static_assert(Gen2Rules::passiveViewCompatibility(false) == Gen2Rules::CompatibilityState::UnusualPreserved);
    static_assert(Gen2Rules::createMoveCompatibility(false) == Gen2Rules::CompatibilityState::NeedsCorrection);
    static_assert(Gen2Rules::editMoveCompatibility(false, true) == Gen2Rules::CompatibilityState::UnusualPreserved);
    static_assert(Gen2Rules::editMoveCompatibility(false, false) == Gen2Rules::CompatibilityState::NeedsCorrection);
    static_assert(Gen2Rules::mayCommit(Gen2Rules::CompatibilityState::UnusualPreserved));
    static_assert(!Gen2Rules::mayCommit(Gen2Rules::CompatibilityState::NeedsCorrection));

    assert(Gen2Rules::exactGameFromSourceId("gold_gbc") == PokeVault::Integration::Gen2::SourceGame::Gold);
    assert(Gen2Rules::exactGameFromSourceId("silver_gbc") == PokeVault::Integration::Gen2::SourceGame::Silver);
    assert(Gen2Rules::exactGameFromSourceId("crystal_gbc") == PokeVault::Integration::Gen2::SourceGame::Crystal);
    assert(!Gen2Rules::exactGameFromSourceId("yellow_gb"));

    assert(Gen2Rules::usableHeldItem(0));
    assert(Gen2Rules::usableHeldItem(PokeVault::Integration::Gen2::kMasterBallItemId));
    assert(!Gen2Rules::usableHeldItem(6)); // TERU-SAMA placeholder is not selectable.
    const auto heldItems = Gen2Rules::heldItemChoices();
    assert(!heldItems.empty() && heldItems.front() == 0);
    assert(std::find(heldItems.begin(), heldItems.end(), static_cast<uint8_t>(6)) == heldItems.end());

    std::cout << "GSC shared editor UI/capability rules: PASS\n";
}
