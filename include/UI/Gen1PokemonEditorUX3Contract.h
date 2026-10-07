#ifndef POKEBANK_UI_GEN1_POKEMON_EDITOR_UX3_CONTRACT_H
#define POKEBANK_UI_GEN1_POKEMON_EDITOR_UX3_CONTRACT_H

#include <array>
#include <cstddef>
#include <cstdint>

namespace PokeBank::UIModel::Gen1Editor::UX3 {

enum class Field : uint8_t {
    Species,
    Nickname,
    LevelExperience,
    Shiny,
    Move1,
    Move2,
    Move3,
    Move4,
    DVs,
    StatExperience,
    OriginalTrainer,
    TrainerID,
    ReviewOrStage,
    BackOrCancel,
};

constexpr std::array<Field,14> workspaceFields() noexcept {
    return {Field::Species, Field::Nickname, Field::LevelExperience, Field::Shiny,
            Field::Move1, Field::Move2, Field::Move3, Field::Move4,
            Field::DVs, Field::StatExperience, Field::OriginalTrainer, Field::TrainerID,
            Field::ReviewOrStage, Field::BackOrCancel};
}

constexpr std::array<std::size_t,5> sectionStarts() noexcept { return {0,4,8,10,12}; }
constexpr std::size_t workspaceRowCount() noexcept { return workspaceFields().size(); }
constexpr bool usesPkseThreeColumnSkeleton() noexcept { return true; }
constexpr bool addAndEditShareWorkspace() noexcept { return true; }
constexpr bool usesThirtyRawRows() noexcept { return false; }
constexpr bool usesWizardPages() noexcept { return false; }
constexpr bool radarLivesInEditorInfoRegion() noexcept { return true; }
constexpr std::size_t radarAxisCount() noexcept { return 5; }
constexpr bool radarSplitsSpecial() noexcept { return false; }
constexpr bool shinyIsStoredBoolean() noexcept { return false; }
constexpr bool speciesPickerUsesNumberedRows() noexcept { return true; }
constexpr bool speciesPickerShowsNormalAndShinyPreview() noexcept { return true; }
constexpr bool speciesPickerBrowseMutatesDraft() noexcept { return false; }
constexpr bool speciesPickerBrowseMutatesStagedSave() noexcept { return false; }
constexpr bool movePickerIsSpeciesGameFiltered() noexcept { return true; }
constexpr bool fullEncounterLegalityIsImplemented() noexcept { return false; }
constexpr bool viewIsReadOnly() noexcept { return true; }
constexpr bool viewUsesSharedPkseStyleStructure() noexcept { return true; }
constexpr bool cloneBrowseMutatesStagedSave() noexcept { return false; }
constexpr bool cloneOccupiedCellsShowSprite() noexcept { return true; }

struct PendingGeometry {
    int width;
    int height;
    int visibleRows;
    bool scrolls;
};

constexpr PendingGeometry pendingGeometry(std::size_t changes) noexcept {
    if (changes <= 1) return {680, 220, 1, false};
    if (changes <= 4) return {820, 180 + static_cast<int>(changes) * 48,
                              static_cast<int>(changes), false};
    constexpr int cappedRows = 7;
    return {980, 540, cappedRows, changes > static_cast<std::size_t>(cappedRows)};
}

struct CloneSpriteKey {
    uint16_t species;
    bool shiny;
};
constexpr CloneSpriteKey cloneSpriteKey(uint16_t species, bool shiny) noexcept {
    return {species, shiny};
}

// Preview and staged-save boundaries are explicit to prevent a shiny/hover UI
// convenience from accidentally becoming a source mutation path.
constexpr bool shinyPreviewMutatesDraft() noexcept { return false; }
constexpr bool shinyPreviewMutatesStagedSave() noexcept { return false; }
constexpr bool movePickerBrowseMutatesStagedSave() noexcept { return false; }
constexpr bool liveSourceWritingEnabled() noexcept { return false; }
constexpr bool partyEditingEnabled() noexcept { return false; }

} // namespace PokeBank::UIModel::Gen1Editor::UX3

#endif
