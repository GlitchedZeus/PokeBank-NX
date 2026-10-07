#include "UI/BattleStatRadarModel.h"
#include "UI/Gen2PokemonEditorRules.h"
#include "UI/LegacyPresentationRules.h"
#include "UI/PokemonEditorFoundationContract.h"
#include "UI/ExactSaveCapabilities.h"

#include <algorithm>
#include <array>
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

    constexpr auto rby = Foundation::capabilitiesForSourceId("yellow_gb");
    constexpr auto gold = Foundation::capabilitiesForSourceId("gold_gbc");
    constexpr auto silver = Foundation::capabilitiesForSourceId("silver_gbc");
    constexpr auto crystal = Foundation::capabilitiesForSourceId("crystal_gbc");
    constexpr auto lgpe = Foundation::capabilitiesForSourceId("letsgo_eevee_switch");
    constexpr auto pla = Foundation::capabilitiesForSourceId("legends_arceus_switch");
    constexpr auto sv = Foundation::capabilitiesForSourceId("scarlet_switch");
    static_assert(rby && gold && silver && crystal && lgpe && pla && sv);
    static_assert(rby->statModel != gold->statModel && !rby->fields.supportsHeldItem);
    static_assert(gold->fields.supportsHeldItem && gold->supportsPokerus);
    static_assert(!gold->supportsCrystalCaughtData && !silver->fields.supportsMetLevel);
    static_assert(crystal->supportsCrystalCaughtData && crystal->fields.supportsMetLevel);
    static_assert(gold->geneticMaximum == 15 && gold->trainingMaximum == 65535);
    static_assert(lgpe->fields.usesIVs && lgpe->usesAVs && !lgpe->fields.usesEVs);
    static_assert(lgpe->geneticMaximum == 31 && lgpe->trainingMaximum == 200);
    static_assert(!lgpe->fields.supportsAbility && !lgpe->fields.supportsHeldItem);
    static_assert(pla->usesEffortLevels && pla->trainingMaximum == 10 && !pla->fields.usesEVs);
    static_assert(pla->statModel != sv->statModel && sv->fields.usesIVs && sv->fields.usesEVs);
    static_assert(!Foundation::capabilitiesForSourceId("unknown"));
    constexpr auto wrong = Foundation::exactSaveCapabilities({"gold_gbc",PokeVault::Games::Platform::GameBoyColor,
        Foundation::Generation::Gen2,Foundation::SaveFormat::PA8});
    static_assert(!wrong);
    constexpr auto jp = Foundation::exactSaveCapabilities({"crystal_gbc",PokeVault::Games::Platform::GameBoyColor,
        Foundation::Generation::Gen2,Foundation::SaveFormat::GSCJapanese,1});
    static_assert(jp && jp->identity.revision == 1 && jp->supportsCrystalCaughtData);

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
    assert(Gen2Rules::usableHeldItem(1)); // Master Ball is an authentic Generation II item.
    assert(!Gen2Rules::usableHeldItem(6)); // TERU-SAMA placeholder is not selectable.
    for (uint8_t item : {7,54,66,70,115,116,129,175,178,195,220,243,249,250,255})
        assert(!Gen2Rules::usableHeldItem(item));
    for (uint8_t item : {1,3,8,157,191,196,221,242}) assert(Gen2Rules::usableHeldItem(item));
    const auto heldItems = Gen2Rules::heldItemChoices();
    assert(!heldItems.empty() && heldItems.front() == 0);
    assert(std::find(heldItems.begin(), heldItems.end(), static_cast<uint8_t>(6)) == heldItems.end());

    // Internal battle-stat storage and the shared Gen II/III radar presentation use
    // HP/Atk/Def/Spe/SpA/SpD. No UI-only reorder remains.
    constexpr std::array<uint16_t, 6> internalStats{101, 102, 103, 106, 104, 105};
    constexpr auto radarStats = PokeBank::UIModel::canonicalGen2RadarStats(internalStats);
    static_assert(radarStats == internalStats);
    static_assert(PokeBank::UIModel::gen2RadarLabels[0][0] == 'H');
    static_assert(PokeBank::UIModel::gen2RadarLabels[3][0] == 'S');
    static_assert(PokeBank::UIModel::gen2RadarLabels[4][2] == 'A');
    static_assert(PokeBank::UIModel::gen2RadarLabels[5][2] == 'D');

    std::cout << "GSC shared editor UI/capability rules: PASS\n";
}
