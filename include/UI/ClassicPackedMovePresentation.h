#ifndef UI_CLASSIC_PACKED_MOVE_PRESENTATION_H
#define UI_CLASSIC_PACKED_MOVE_PRESENTATION_H

// Read-only presentation hint for the classic shared box carry overlay.
// Gen IV deliberately defers edits to PK4 until a validated A drop. The
// source slot still exists in the staged save during carry, but the box grid
// and summary must present it as empty, like Gen I-III's picked-up source.
// This NEVER deletes, relocates or writes the source record.
namespace UI {
class TrainerViewScreen;
namespace ClassicPackedMove {
bool hidesGen4CarriedSource(const TrainerViewScreen& screen, int box, int slot);
// R2-CHECK-009: Gen I Box Actions -> Move. Starts the SAME Boxes-view carry as a Y tap, with this
// Pokemon picked up (A places, B cancels). False when no carry could start.
bool beginGen1FromAction(TrainerViewScreen& screen, int box, int slot);
// Round 11 H31: Gen I-V Actions -> Clone. The caller closes its Actions sheet first; the shared carry
// then holds a ghost copy (nothing staged) until A places it on an empty slot or B cancels. `party`
// (Gen V only) clones Party slot `slot` into a box. False when no carry could start.
bool beginCloneFromAction(TrainerViewScreen& screen, int box, int slot, bool party = false);
} // namespace ClassicPackedMove
} // namespace UI

#endif
