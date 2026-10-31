#pragma once

namespace PokeBank::UIModel::Gen2StagedAccess {

// Source policy and workspace capability are deliberately independent. A validated
// RetroArch adapter owns a byte clone; it never grants access to the original file.
struct Capabilities {
    bool canWriteOriginalSource = false;
    bool canEditStagedCopy = false;
};
constexpr Capabilities capabilities(bool validatedAdapter, bool stagedEditorAvailable) noexcept {
    return {false, validatedAdapter && stagedEditorAvailable};
}
enum class Surface { Other, Boxes, StorageSave, Trainer };
enum class Entry { None, PokemonActions, TrainerEdit };
constexpr Entry entry(Capabilities caps, Surface surface, bool viewEntered,
                      bool blocked, bool validSlot, bool activate) noexcept {
    if (!caps.canEditStagedCopy || !viewEntered || blocked || !activate) return Entry::None;
    if (surface == Surface::Trainer) return Entry::TrainerEdit;
    if (validSlot && (surface == Surface::Boxes || surface == Surface::StorageSave))
        return Entry::PokemonActions;
    return Entry::None;
}
} // namespace PokeBank::UIModel::Gen2StagedAccess
