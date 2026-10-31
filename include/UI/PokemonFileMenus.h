#pragma once
// Pokemon Actions -> "Import from file" / "Export to file" (owner Oct 28, Feature A UI).
//
// The one entry point every Gen I-V action sheet calls. Each generation's staged editor does the
// real work (PokemonFile/NativePokemonFile.h decode, then the editor's own stageImport /
// export*PokemonFile); this layer only picks the slot, runs the shared file dialog
// (UI/PokemonFileDialog.h) and reports the outcome:
//   * Import stages ONE record into the open workspace (Review lists it; Discard undoes it). Gen I /
//     II boxes are packed lists, so the record goes to the box's first empty slot; Gen III-V need the
//     chosen empty slot. Nothing is written anywhere: the source save is unchanged.
//   * Export writes ONE new file under sdmc:/switch/PokeBank-NX/pokemon/genN (never replacing a
//     file) and never changes the workspace.
// Japanese Gen I / II saves have no staged editor while JapaneseEditingGate.h is false, so their
// sheets offer neither action.

namespace UI {
class TrainerViewScreen;
namespace PokemonFileMenus {

// Opens the Import from file dialog for an empty slot of box `box` (zero-based).
bool beginImport(TrainerViewScreen& screen, int generation, int box, int slot);

// Exports the Pokemon in Box `box` / Slot `slot`, or Party `slot` when `party`, as the staged
// workspace holds it. Posts "Exported to <path>" or the reason.
bool exportPokemon(TrainerViewScreen& screen, int generation, bool party, int box, int slot);

}  // namespace PokemonFileMenus
}  // namespace UI
